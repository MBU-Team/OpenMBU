//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/shadow.h"
#include "sim/sceneObject.h"
#include "core/fileStream.h"
#include "core/bitRender.h"
#include "platform/platform.h"
#include "gfx/primBuilder.h"

DepthSortList Shadow::smDepthSortList;
GFXTexHandle* Shadow::smGenericShadowTexture = NULL;
S32 Shadow::smGenericShadowDim = 32;
S32 Shadow::smInstanceCount = 0;
U32 Shadow::smShadowMask = AtlasObjectType | TerrainObjectType | InteriorObjectType;
F32 Shadow::smShapeDetailScale = 1.0f;
S32 Shadow::smShapeDetailMin = 2; // never select anything less than this detail unless other value supplied
F32 Shadow::smSmallestVisibleSize = 6.0f; // stop drawing shadows when less than 6 pixels in size
F32 Shadow::smLightDirSkew = 0.0f;
F32 Shadow::smLightLenSkew = 0.0f;
F32 Shadow::smGenericRadiusSkew = 0.4f; // shrink radius of shape when it always uses generic shadow...
bool Shadow::smAlwaysUseGenericBmp = false;
F32 Shadow::smGlobalShadowDetail = 1.0f;

Vector<U32> gShadowBits(__FILE__, __LINE__);
Box3F gShadowBox;
SphereF gShadowSphere;
Point3F gShadowPoly[4];

Shadow::DistanceDetail Shadow::smDefaultDistanceDetails[] =
{
   {    0.0f, 0.0f, 0.0f },
   {  100.0f, 0.0f, 0.0f },
   {  500.0f, 1.0f, 0.5f },
   { 1000.0f, 1.0f, 0.7f }
};

Shadow::PixelSizeDetail Shadow::smDefaultPixelSizeDetails[] =
{
   {   130.0f,  25, 64, 1, false },
   {    25.0f, 100, 64, 1, false },
   {    10.0f, 100, 32, 0, false },
   {     0.0f,   0,  0, 0, true }
};

void Shadow::setGlobalShadowDetailLevel(F32 d)
{
    smGlobalShadowDetail = d;

    smShapeDetailScale = 0.5f + d * 0.5f;
    smShapeDetailMin = d > 0.49f ? 2 : 3;
    smSmallestVisibleSize = d > 0.7f ? 6 : (d > 0.4f ? 7 : (d > 0.3f ? 8 : (d > 0.2f ? 9 : 10)));
    smLightDirSkew = 0.85f * (1.0f - d);
    smLightLenSkew = (1.0f - d) * 0.5f;
}

//--------------------------------------------------------------

Shadow::Shadow()
{
    mBitmap = NULL;
    mRadius = 0.0f;
    mSettings.alwaysUseGenericBmp = false;
    mSettings.noAnimate = false;
    mSettings.noMove = false;

    setDefaultDetailTables();

    if (smInstanceCount == 0)
    {
        GBitmap* bitmap = generateGenericShadowBitmap(smGenericShadowDim);
        smGenericShadowTexture = new GFXTexHandle(bitmap, &GFXDefaultStaticDiffuseProfile, true);
        // bitmap now owned by texture manager, so we don't delete it
    }
    smInstanceCount++;
}

Shadow::~Shadow()
{
    AssertFatal(smInstanceCount > 0, "Error, more destructors than constructors?");
    smInstanceCount--;
    /*
       if (smInstanceCount == 0) {
          delete smGenericShadowTexture;
          smGenericShadowTexture = NULL;
       }
    */
}

GBitmap* Shadow::generateGenericShadowBitmap(S32 dim)
{
    GBitmap* bitmap = new GBitmap(dim, dim, false);
    U8* bits = bitmap->getWritableBits();
    S32 center = dim >> 1;
    F32 invRadiusSq = 1.0f / (F32)(center * center);
    F32 tmpF;
    for (S32 i = 0; i < dim; i++)
        for (S32 j = 0; j < dim; j++)
        {
            tmpF = (F32)((i - center) * (i - center) + (j - center) * (j - center)) * invRadiusSq;
            bits[i * dim + j] = (U8)(tmpF > 0.99f ? 0 : 180 - 180.0f * tmpF); // 180 out of 255 max
        }
    return bitmap;
}

//--------------------------------------------------------------

void Shadow::setDetailTables(const DistanceDetail* dd, S32 ddCount, const PixelSizeDetail* psd, S32 psdCount)
{
    mDistanceDetails = dd;
    mPixelSizeDetails = psd;
    mNumDistanceDetails = ddCount;
    mNumPixelSizeDetails = psdCount;
}

void Shadow::setDefaultDetailTables()
{
    setDetailTables(smDefaultDistanceDetails,
        sizeof(smDefaultDistanceDetails) / sizeof(DistanceDetail),
        smDefaultPixelSizeDetails,
        sizeof(smDefaultPixelSizeDetails) / sizeof(PixelSizeDetail));
}

void Shadow::findDistanceDetail(F32 dist, DistanceDetail* dd)
{
    for (S32 i = 0; i < mNumDistanceDetails; i++)
    {
        if (dist <= mDistanceDetails[i].dist)
        {
            if (i == 0)
                *dd = mDistanceDetails[i];
            else
            {
                F32 interp = (dist - mDistanceDetails[i].dist) / (mDistanceDetails[i - 1].dist - mDistanceDetails[i].dist);
                dd->directionSkew = interp * mDistanceDetails[i - 1].directionSkew + (1.0f - interp) * mDistanceDetails[i].directionSkew;
                dd->lengthSkew = interp * mDistanceDetails[i - 1].lengthSkew + (1.0f - interp) * mDistanceDetails[i].lengthSkew;
            }
            return;
        }
    }
    *dd = mDistanceDetails[mNumDistanceDetails - 1];
}

void Shadow::findPixelSizeDetail(F32 pixelSize, const PixelSizeDetail** psd)
{
    *psd = NULL;
    for (S32 i = 0; i < mNumPixelSizeDetails; i++)
    {
        *psd = &mPixelSizeDetails[i];
        if (pixelSize >= mPixelSizeDetails[i].size)
            return;
    }
}

S32 Shadow::selectShapeDetail(TSShapeInstance* shapeInstance, F32 dist, F32 scale, S32 detailMin)
{
    /*
       if (detailMin<0)
          detailMin = Shadow::smShapeDetailMin;

       S32 dl = shapeInstance->selectCurrentDetail( dglProjectRadius(dist,scale * shapeInstance->getShape()->radius * dglGetPixelScale() * TSShapeInstance::smDetailAdjust * Shadow::smShapeDetailScale));
       if (dl>=0 && dl < detailMin)
          shapeInstance->setCurrentDetail(getMin(detailMin,shapeInstance->getShape()->mSmallestVisibleDL));
    */
    return shapeInstance->getCurrentDetail();
}

//--------------------------------------------------------------

void Shadow::setLightMatrices(const Point3F& lightDir, const Point3F& pos)
{
    AssertFatal(mDot(lightDir, lightDir) > 0.0001f, "Shadow::setLightDir: light direction must be a non-zero vector.");

    // construct light matrix
    Point3F x, z;
    if (mFabs(lightDir.z) > 0.001f)
    {
        // mCross(Point3F(1,0,0),lightDir,&z);
        z.x = 0.0f;
        z.y = lightDir.z;
        z.z = -lightDir.y;
        z.normalize();
        mCross(lightDir, z, &x);
    }
    else
    {
        mCross(lightDir, Point3F(0, 0, 1), &x);
        x.normalize();
        mCross(x, lightDir, &z);
    }

    mLightToWorld.identity();
    mLightToWorld.setColumn(0, x);
    mLightToWorld.setColumn(1, lightDir);
    mLightToWorld.setColumn(2, z);
    mLightToWorld.setColumn(3, pos);

    mWorldToLight = mLightToWorld;
    mWorldToLight.inverse();
}

void Shadow::setRadius(F32 radius)
{
    mRadius = radius;
}

void Shadow::setRadius(TSShapeInstance* shapeInstance, const Point3F& scale)
{
    const Box3F& bounds = shapeInstance->getShape()->bounds;
    F32 dx = 0.5f * (bounds.max.x - bounds.min.x) * scale.x;
    F32 dy = 0.5f * (bounds.max.y - bounds.min.y) * scale.y;
    F32 dz = 0.5f * (bounds.max.z - bounds.min.z) * scale.z;
    mRadius = mSqrt(dx * dx + dy * dy + dz * dz);
}

//--------------------------------------------------------------

void Shadow::beginRenderToBitmap()
{
    AssertFatal(mBitmap, "Shadow::beginRenderToBitmap had no bitmap to render to!");

    // clean slate...
    gShadowBits.setSize(mSettings.bmpDim * (mSettings.bmpDim >> 5));
    dMemset(gShadowBits.address(), 0, mSettings.bmpDim * (mSettings.bmpDim >> 3)); // dMemset deals in bytes not words, hence the shift by 3 not 5
}

void Shadow::endRenderToBitmap()
{
    // copy to bitmap
    if (mSettings.blur == 1)
        // blur
        BitRender::bitTo8Bit_3(gShadowBits.address(), (U32*)mBitmap->getWritableBits(), mSettings.bmpDim);
    else
        // non-blur version:
        BitRender::bitTo8Bit(gShadowBits.address(), (U32*)mBitmap->getWritableBits(), mSettings.bmpDim);

    //   mShadowTexture.refresh();
}

void Shadow::renderToBitmap(TSShapeInstance* shapeInstance, const MatrixF& transform, const Point3F& pos, Point3F scale)
{
    AssertFatal(mBitmap, "Shadow::renderToShadow: must call beginRenderToBitmap first");

    MatrixF mat;
    mat.mul(mWorldToLight, transform);

    // everything gets scaled by this amount so that we map onto the bitmap correctly
    F32 k = ((F32)mSettings.bmpDim) / (2.0f * mRadius);

    // this scales everything but the last row of the matrix (we do that below)
    scale *= k;
    mat.scale(scale);

    // we want pos to map to bitmap center...
    // the following is a bit convoluted...but it is correct...
    Point3F p, p2;
    mat.getColumn(3, &p);
    mWorldToLight.mulP(pos, &p2);
    p -= p2;
    p *= k;
    F32 halfDim = mSettings.bmpDim >> 1;
    p.x += halfDim;
    p.z += halfDim;
    mat.setColumn(3, p); // shape center now falls on bitmap center...

    shapeInstance->animate();
    shapeInstance->renderShadow(shapeInstance->getCurrentDetail(), mat, mSettings.bmpDim, gShadowBits.address());
}

//--------------------------------------------------------------

bool Shadow::prepare(const Point3F& pos, Point3F lightDir, F32 shadowLen, const Point3F& scale, F32 dist, F32 fogAmount, TSShapeInstance* shapeInstance)
{
    // 0. use pixel size to do early reject test
    // 1. use distance from camera to do 1st pass detail selection (light direction, shadow volume distance)
    // 2. get polys to project shadow onto, build shadow partition
    // 3. use pixel size to do 2nd pass detail selection (shape detail level, bitmap dimension, blur routine,
    //    sample frequency, substitute generic shadow bitmap)

    // NOTES:
    // If using cached partition, do 1&2 only if we don't have a partition yet...
    // If non-animating shape, do step 3 and re-compute bitmap only if detail info changes...

    // --------------------------------------
    // 0.
    F32 maxScale = getMax(scale.x, getMax(scale.y, scale.z));
    F32 pixelSize = GFX->projectRadius(dist / maxScale, shapeInstance->getShape()->radius) * 1.0f * TSShapeInstance::smDetailAdjust;
    F32 smallest = getMax(Shadow::smSmallestVisibleSize, shapeInstance->getShape()->mSmallestVisibleSize);
    if (pixelSize * Shadow::smShapeDetailScale < smallest)
        return false;

    // fade over distance from viewer
    F32 pseudoFog = smallest / (pixelSize * Shadow::smShapeDetailScale);
    if (pseudoFog > 0.5f)
        pseudoFog = 2.0f * (pseudoFog - 0.5f);
    else
        pseudoFog = 0.0f;
    if (fogAmount < pseudoFog)
    {
        if (pseudoFog >= 0.99f)
            // shadow faded out
            return false;
        fogAmount = pseudoFog;
    }

    // find detail information
    DistanceDetail dd;
    findDistanceDetail(mSettings.noMove ? 0.0f : dist, &dd);
    const PixelSizeDetail* psd;
    findPixelSizeDetail(pixelSize, &psd);

    if (!mSettings.noMove || mPartition.empty())
    {
        // --------------------------------------
        // 1.
        F32 dirMult = (1.0f - dd.directionSkew) * (1.0f - smLightDirSkew);
        if (dirMult < 0.99f)
        {
            lightDir.z *= dirMult;
            lightDir.z -= 1.0f - dirMult;
        }
        lightDir.normalize();
        shadowLen *= (1.0f - dd.lengthSkew) * (1.0f - smLightLenSkew);

        // --------------------------------------
        // 2. get polys
        F32 radius = mRadius;
        if (psd->genericShadowBmp || mSettings.alwaysUseGenericBmp || smAlwaysUseGenericBmp)
            radius *= smGenericRadiusSkew;
        buildPartition(pos, lightDir, radius, shadowLen);
    }
    updatePartition(fogAmount);
    if (mPartition.empty())
        // no need to draw shadow if nothing to cast it onto
        return false;

    // --------------------------------------
    // 3.
    // do we need a new bitmap?  anim rate, bmp dim, generic vs generated
    mSettings.needBmp = false;
    if (mSettings.alwaysUseGenericBmp || smAlwaysUseGenericBmp || psd->genericShadowBmp)
        // use generic bitmap -- get rid of old bmp if it's there
        mBitmap = NULL;
    else
    {
        U32 time = Platform::getVirtualMilliseconds();
        bool expired = time - mSettings.lastBmpTime > psd->frameExpiration;
        bool propertyChange = !mBitmap || psd->bmpDim != mSettings.bmpDim || psd->blur != mSettings.blur;
        if ((expired && !mSettings.noAnimate) || propertyChange)
        {
            // need to generate a new bmp
            mSettings.blur = psd->blur;
            mSettings.lastBmpTime = Platform::getVirtualMilliseconds();
            mSettings.needBmp = true;

            if (mSettings.bmpDim != psd->bmpDim || !mBitmap)
            {
                // allocate new bitmap...register texture (no need to delete old one, owned by texture handle)
                mSettings.bmpDim = psd->bmpDim;
                //mBitmap = new GBitmap(mSettings.bmpDim,mSettings.bmpDim,false);
                //mShadowTexture.set(NULL,mBitmap);
            }
        }
    }

    return true;
}

//--------------------------------------------------------------

void Shadow::buildPartition(const Point3F& p, const Point3F& lightDir, F32 radius, F32 shadowLen)
{
    setLightMatrices(lightDir, p);

    Point3F extent(2.0f * radius, shadowLen, 2.0f * radius);
    smDepthSortList.clear();
    smDepthSortList.set(mWorldToLight, extent);
    smDepthSortList.setInterestNormal(lightDir);

    if (shadowLen < 1.0f)
        // no point in even this short of a shadow...
        shadowLen = 1.0f;
    mInvShadowDistance = 1.0f / shadowLen;

    // build world space box and sphere around shadow

    Point3F x, y, z;
    mLightToWorld.getColumn(0, &x);
    mLightToWorld.getColumn(1, &y);
    mLightToWorld.getColumn(2, &z);
    x *= radius;
    y *= shadowLen;
    z *= radius;
    gShadowBox.max.set(mFabs(x.x) + mFabs(y.x) + mFabs(z.x),
        mFabs(x.y) + mFabs(y.y) + mFabs(z.y),
        mFabs(x.z) + mFabs(y.z) + mFabs(z.z));
    y *= 0.5f;
    gShadowSphere.radius = gShadowBox.max.len();
    gShadowSphere.center = p + y;
    gShadowBox.min = y + p - gShadowBox.max;
    gShadowBox.max += y + p;

    // get polys

    getCurrentClientContainer()->findObjects(smShadowMask, Shadow::collisionCallback, this);

    // setup partition list
    gShadowPoly[0].set(-radius, 0, -radius);
    gShadowPoly[1].set(-radius, 0, radius);
    gShadowPoly[2].set(radius, 0, radius);
    gShadowPoly[3].set(radius, 0, -radius);

    mPartition.clear();
    mPartitionVerts.clear();
    smDepthSortList.depthPartition(gShadowPoly, 4, mPartition, mPartitionVerts);

    // now set up tverts & colors
    mPartitionColors.setSize(mPartitionVerts.size());
    mPartitionTVerts.setSize(mPartitionVerts.size());
    F32 invRadius = 1.0f / radius;
    for (S32 i = 0; i < mPartitionVerts.size(); i++)
        mPartitionTVerts[i].set(0.5f + 0.5f * mPartitionVerts[i].x * invRadius, 0.5f + 0.5f * mPartitionVerts[i].z * invRadius);
}

void Shadow::updatePartition(F32 fog)
{
    fog = 1.0f - fog;
    for (S32 i = 0; i < mPartitionVerts.size(); i++)
    {
        F32 val = 1.0f - mPartitionVerts[i].y * mInvShadowDistance;
        mPartitionColors[i].set(val * fog,
            val * fog,
            val * fog,
            val * fog);
    }
}

//--------------------------------------------------------------

void Shadow::collisionCallback(SceneObject* obj, void* thisPtr)
{
    Shadow* shadow = reinterpret_cast<Shadow*>(thisPtr);
    if (obj->getWorldBox().isOverlapped(gShadowBox))
        obj->buildPolyList(&smDepthSortList, gShadowBox, gShadowSphere);
}

//--------------------------------------------------------------

void Shadow::render()
{
    // push light matrix
    GFX->pushWorldMatrix();

    GFX->setWorldMatrix(mLightToWorld);

    // set up texture environment
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendZero);
    GFX->setDestBlend(GFXBlendInvSrcColor);
    GFX->setZWriteEnable(false);

    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    if (mBitmap)
    {
        //glBindTexture(GL_TEXTURE_2D, mShadowTexture.getGLName());
        GFX->setTexture(0, mShadowTexture);
    }
    else
    {
        AssertFatal(smGenericShadowTexture != NULL, "Error, shadow texture not initialized!");
        //glBindTexture(GL_TEXTURE_2D, smGenericShadowTexture->getGLName());
        GFX->setTexture(0, *smGenericShadowTexture);
    }

    // draw
    if (TSMesh::getOverrideFade() < 1.0f)
    {
        F32 color = TSMesh::getOverrideFade();
        PrimBuild::color4f(color, color, color, color);
    }

    for (U32 i = 0; i < mPartition.size(); i++)
    {
        // setup geometry and draw
        PrimBuild::begin(GFXTriangleFan, 4);

        for (U32 j = 0; j < mPartition[i].vertexCount; j++)
        {
            if (TSMesh::getOverrideFade() >= 1.0f)
                PrimBuild::color4f(mPartitionColors[mPartition[i].vertexStart + j].x,
                    mPartitionColors[mPartition[i].vertexStart + j].y,
                    mPartitionColors[mPartition[i].vertexStart + j].z,
                    mPartitionColors[mPartition[i].vertexStart + j].w);

            PrimBuild::texCoord2f(mPartitionTVerts[mPartition[i].vertexStart + j].x,
                mPartitionTVerts[mPartition[i].vertexStart + j].y);
            PrimBuild::vertex3f(mPartitionVerts[mPartition[i].vertexStart + j].x,
                mPartitionVerts[mPartition[i].vertexStart + j].y,
                mPartitionVerts[mPartition[i].vertexStart + j].z);
        }

        GFX->setupGenericShaders(GFXDevice::GSModColorTexture);
        PrimBuild::end();
    }


    GFX->popWorldMatrix();
}

//--------------------------------------------------------------

#ifdef TORQUE_DEBUG
void Shadow::dumpSort()
{
#define printDump(a) {const char * b = a; file.write(dStrlen(b),b);}

    FileStream file;

    file.open("poly.txt", FileStream::Write);
    S32 i;
    printDump("Pre-sort: \r\n\r\n");
    for (i = 0; i < smDepthSortList.mPolyIndexList.size(); i++)
    {
        DepthSortList::Poly* poly;
        DepthSortList::PolyExtents* polyExtent;
        smDepthSortList.getOrderedPoly((U32)i, &poly, &polyExtent);
        for (S32 j = 0; j < poly->vertexCount; j++)
        {
            Point3F p = smDepthSortList.mVertexList[smDepthSortList.mIndexList[poly->vertexStart + j]].point;
            printDump(avar("(%5.3f, %5.3f, %5.3f) ", p.x, p.y, p.z));
        }
        printDump("\r\n");
    }
    smDepthSortList.sort();

    printDump("\r\n\r\nPost-sort: \r\n\r\n");
    for (i = 0; i < smDepthSortList.mPolyIndexList.size(); i++)
    {
        DepthSortList::Poly* poly;
        DepthSortList::PolyExtents* polyExtent;
        smDepthSortList.getOrderedPoly(i, &poly, &polyExtent);
        for (S32 j = 0; j < poly->vertexCount; j++)
        {
            Point3F p = smDepthSortList.mVertexList[smDepthSortList.mIndexList[poly->vertexStart + j]].point;
            printDump(avar("(%5.3f, %5.3f, %5.3f) ", p.x, p.y, p.z));
        }
        printDump("\r\n");
    }

    file.close();
}
#endif



