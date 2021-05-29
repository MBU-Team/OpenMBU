//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Portions of Volume Light by Parashar Krishnamachari
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "sceneGraph/sceneGraph.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxCanon.h"
#include "volLight.h"

#include "platform/event.h"
#include "platform/gameInterface.h"


void sgPopulateVertBuffer(GFXVertexPCT& vert, const Point3F& point, const Point2F& tcoord, const ColorF& color)
{
    vert.point = point;
    vert.texCoord = tcoord;
    vert.color = color;
}

void VolumeLight::makeDIPCall()
{
    GFX->drawIndexedPrimitive(GFXTriangleList, 0, mVertCount, 0, mPrimCount);
}

IMPLEMENT_CO_NETOBJECT_V1(VolumeLight);

VolumeLight::VolumeLight()
{
    // Setup NetObject.
    mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);

    mLightHandle = NULL;

    mDataBlock = NULL;

    mLightTexture = StringTable->insert("");

    mLPDistance = 8.0f;
    mShootDistance = 15.0f;
    mXExtent = 6.0f;
    mYExtent = 6.0f;

    mSubdivideU = 32;
    mSubdivideV = 32;

    mFootColor = ColorF(1.f, 1.f, 1.f, 0.2f);
    mTailColor = ColorF(0.f, 0.f, 0.f, 0.0f);
}

VolumeLight::~VolumeLight()
{

}

void VolumeLight::initBuffers()
{
    // How many verts are we talking about?
    U32 max = (((mSubdivideU + mSubdivideV + 2) * 2) + 1) * 4;

    mVertCount = max;

    mVerts.set(GFX, max, GFXBufferTypeStatic);

    Point3F lightpoint;

    // This is where the hypothetical poU32 source of the spot will be
    // The volume slices are projected along the lines from
    lightpoint.x = lightpoint.y = 0.0f;
    lightpoint.z = -mLPDistance;

    F32 ax = mXExtent / 2;
    F32 ay = mYExtent / 2;

    GFXVertexPCT* buf = mVerts.lock();

    // The first poly is going to be our Bottom foot
    buf[0].point.x = -ax;
    buf[0].point.y = -ay;
    buf[0].point.z = 0;
    buf[0].texCoord.x = buf[0].texCoord.y = 0;

    buf[1].point.x = ax;
    buf[1].point.y = -ay;
    buf[1].point.z = 0;
    buf[1].texCoord.x = 1;
    buf[1].texCoord.y = 0;

    buf[2].point.x = ax;
    buf[2].point.y = ay;
    buf[2].point.z = 0;
    buf[2].texCoord.x = 1;
    buf[2].texCoord.y = 1;

    buf[3].point.x = -ax;
    buf[3].point.y = ay;
    buf[3].point.z = 0;
    buf[3].texCoord.x = 0;
    buf[3].texCoord.y = 1;

    buf[0].color = buf[1].color = buf[2].color = buf[3].color = mFootColor;

    // Okay...  vertices 1-4 are taking up the bottom foot now.
    // Now let's get going with the volume slices.

    // Current offset in the buffer
    U32 index = 4;

    // Start with slices in X/U space
    for (U32 i = 0; i <= mSubdivideU; i++)
    {
        F32 k = ((F32)i) / mSubdivideU;            // use for the texture coord
        F32 bx = k * mXExtent - ax;               // use for poU32 positions

        // These are the two endpoints for a slice at the foot
        Point3F end1(bx, -ay, 0.0f);
        Point3F end2(bx, ay, 0.0f);

        end1 -= lightpoint;      // get a vector from poU32 to lightsource
        end1.normalize();      // normalize vector
        end1 *= mShootDistance;   // multiply it out by shootlength

        end1.x += bx;         // Add the original poU32 location to the vector
        end1.y -= ay;

        // Do it again for the other point.
        end2 -= lightpoint;
        end2.normalize();
        end2 *= mShootDistance;

        end2.x += bx;
        end2.y += ay;

        //sgRenderY(Point3F(bx, ay, 0.0f), end1, end2, mfootColour, mtailColour, k);
        Point3F near1 = Point3F(bx, ay, 0.0f);
        Point3F far1 = end1;
        Point3F far2 = end2;
        F32 off = k;
        ColorF nearcol = mFootColor;
        ColorF farcol = mTailColor;

        // Start writing the vertices
        sgPopulateVertBuffer(buf[index++], Point3F(near1.x, 0.0f, 0.0f), Point2F(off, 0.5f), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far1.x, 0.0, far1.z), Point2F(off, 0.5f), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far1.x, far1.y, far1.z), Point2F(off, 0.0f), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(near1.x, -near1.y, 0.0f), Point2F(off, 0.0f), nearcol);

        sgPopulateVertBuffer(buf[index++], Point3F(near1.x, 0.0f, 0.0f), Point2F(off, 0.5f), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(near1.x, near1.y, 0.0f), Point2F(off, 1.0f), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far2.x, far2.y, far2.z), Point2F(off, 1.0f), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far1.x, 0.0, far1.z), Point2F(off, 0.5f), farcol);
    }

    // Now let's do the slices in Y/V space
    for (U32 i = 0; i <= mSubdivideV; i++)
    {
        F32 k = ((F32)i) / mSubdivideV;            // use for the texture coord
        F32 by = k * mXExtent - ay;               // use for poU32 positions

        // These are the two endpoints for a slice at the foot
        Point3F end1(-ax, by, 0.0f);
        Point3F end2(ax, by, 0.0f);

        end1 -= lightpoint;      // get a vector from poU32 to lightsource
        end1.normalize();      // normalize vector
        end1 *= mShootDistance;   // extend it out by shootlength

        end1.x -= ax;         // Add the original poU32 location to the vector
        end1.y += by;

        // Do it again for the other point.
        end2 -= lightpoint;
        end2.normalize();
        end2 *= mShootDistance;

        end2.x += ax;
        end2.y += by;

        //sgRenderX(Point3F(ax, by, 0.0f), end1, end2, mfootColour, mtailColour, k);
        Point3F near1 = Point3F(ax, by, 0.0f);
        Point3F far1 = end1;
        Point3F far2 = end2;
        F32 off = k;
        ColorF nearcol = mFootColor;
        ColorF farcol = mTailColor;

        // Start writing the vertices
        sgPopulateVertBuffer(buf[index++], Point3F(0.0f, near1.y, 0.0f), Point2F(0.5f, off), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(0.0f, far1.y, far1.z), Point2F(0.5f, off), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far1.x, far1.y, far1.z), Point2F(0.0f, off), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(-near1.x, near1.y, 0.0f), Point2F(0.0f, off), nearcol);

        sgPopulateVertBuffer(buf[index++], Point3F(0.0f, near1.y, 0.0f), Point2F(0.5f, off), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(near1.x, near1.y, 0.0f), Point2F(1.0f, off), nearcol);
        sgPopulateVertBuffer(buf[index++], Point3F(far2.x, far2.y, far2.z), Point2F(1.0f, off), farcol);
        sgPopulateVertBuffer(buf[index++], Point3F(0.0f, far1.y, far1.z), Point2F(0.5f, off), farcol);
    }

    mVerts.unlock();

    // Now generate the index buffers.
    U32 quadCount = ((mSubdivideU + mSubdivideV) * 2) + 3;
    U32 indexCount = 6 * quadCount;

    mPrimCount = 2 * quadCount;

    mPrims.set(GFX, indexCount, 0, GFXBufferTypeStatic);

    U16* idx = NULL;

    mPrims.lock(&idx);


    for (U32 i = 0; i < quadCount; i++)
    {
        // Emit a quad...
        U16 a = i * 4 + 0;
        U16 b = i * 4 + 1;
        U16 c = i * 4 + 2;
        U16 d = i * 4 + 3;

        idx[0] = a;
        idx[1] = b;
        idx[2] = c;
        idx[3] = a;
        idx[4] = c;
        idx[5] = d;

        idx += 6;
    }

    mPrims.unlock();
}


///////////////////////////////////////////////////////////////////////////////
// NetObject functions
///////////////////////////////////////////////////////////////////////////////

bool VolumeLight::onNewDataBlock(GameBaseData* dptr)
{
    if (Parent::onNewDataBlock(dptr) == false)
        return false;

    mDataBlock = dynamic_cast<sgLightObjectData*>(dptr);

    if (!mDataBlock)
        return false;

    return true;
}

U32 VolumeLight::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    // Pack Parent.
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Position.and rotation from Parent
    if (stream->writeFlag(mask & VolLightMask))
    {
        stream->writeAffineTransform(mObjToWorld);

        stream->writeString(mLightTexture);

        // Dimensions
        stream->write(mLPDistance);
        stream->write(mShootDistance);
        stream->write(mXExtent);
        stream->write(mYExtent);

        // Subdivisions
        stream->write(mSubdivideU);
        stream->write(mSubdivideV);

        // Colors
        stream->write(mFootColor);
        stream->write(mTailColor);
    }

    return retMask;
}

void VolumeLight::unpackUpdate(NetConnection* con, BitStream* stream)
{
    // Unpack Parent.
    Parent::unpackUpdate(con, stream);

    if (stream->readFlag())
    {
        MatrixF ObjectMatrix;

        // Position.and rotation
        stream->readAffineTransform(&ObjectMatrix);

        mLightTexture = stream->readSTString();

        // Dimensions
        stream->read(&mLPDistance);
        stream->read(&mShootDistance);
        stream->read(&mXExtent);
        stream->read(&mYExtent);

        // Subdivisions
        stream->read(&mSubdivideU);
        stream->read(&mSubdivideV);

        // Colors
        stream->read(&mFootColor);
        stream->read(&mTailColor);

        // Set Transforms.
        setTransform(ObjectMatrix);
        setRenderTransform(mObjToWorld);

        // Very rough, and not complete estimate of the bounding box.
        // A complete one would actually shoot out rays from the hypothetical lightpoint
        // through the vertices to find the extents of the light volume.

        // But there's no poU32 doing so here.  There's no real collision with
        // beams of light.

        buildObjectBox();
        initBuffers();
        mLightHandle.set(mLightTexture, &GFXDefaultStaticDiffuseProfile);
    }
}

//------------------------------------------------------------------------

bool VolumeLight::onAdd()
{
    // Add Parent.
    if (!Parent::onAdd())
        return(false);

    buildObjectBox();
    initBuffers();
    mLightHandle.set(mLightTexture, &GFXDefaultStaticDiffuseProfile);

    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

    return true;
}

void VolumeLight::onRemove()
{
    // remove the texture handle
    mLightHandle = NULL;

    // Do Parent.
    Parent::onRemove();
}

void VolumeLight::inspectPostApply()
{
    // Reset the World Box.
    resetWorldBox();
    // Set the Render Transform.
    setRenderTransform(mObjToWorld);

    // Set Parent.
    Parent::inspectPostApply();

    setMaskBits(VolLightMask);
}


///////////////////////////////////////////////////////////////////////////////
// SceneObject functions  -- The only meaty part
///////////////////////////////////////////////////////////////////////////////

bool VolumeLight::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState)
{
    // Return if last state.
    if (isLastState(state, stateKey))
        return false;

    // Set Last State.
    setLastState(state, stateKey);

    // no need to render if it isn't enabled
    if ((!mEnable) || (!mLightHandle))
        return false;

    // Is Object Rendered?
    if (state->isObjectRendered(this))
    {
        /*      // Yes, so get a SceneRenderImage.
              SceneRenderImage* image = new SceneRenderImage;

              // Populate it.
              image->obj = this;
              image->isTranslucent = true;
              image->sortType = SceneRenderImage::Point;
              state->setImageRefPoint(this, image);

              // Insert it into the scene images.
              state->insertRenderImage(image);*/

        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_ObjectTranslucent;
        ri->translucent = true;
        ri->calcSortPoint(this, state->getCameraPosition());
        gRenderInstManager.addInst(ri);
    }

    return false;
}

void VolumeLight::renderObject(SceneState* state, RenderInst* ri)
{
    Parent::renderObject(state, ri);

    if (mLightHandle.isNull() || !mDataBlock)
        return;

    //GFX_Canonizer("VolumeLight_renderObject", __FILE__, __LINE__);

    // Transform appropriately.
    GFX->pushWorldMatrix();
    GFX->multWorld(mObjToWorld);

    GFX->disableShaders();

    GFX->setBaseRenderState();
    GFX->setTexture(0, NULL);

    U32 stage1ColorOp = GFX->getTextureStageState(1, GFXTSSColorOp);
    U32 stage1AlphaOp = GFX->getTextureStageState(1, GFXTSSAlphaOp);
    //U32 magic = GFX->getTextureStageState(0, 3);

    GFX->setTextureStageColorOp(1, GFXTOPDisable);
    GFX->setTextureStageAlphaOp(1, GFXTOPDisable);

    U32 colorarg2 = GFX->getTextureStageState(0, GFXTSSColorArg2);

    U32 alphaOp = GFX->getTextureStageState(0, GFXTSSAlphaOp);
    U32 alphaArg1 = GFX->getTextureStageState(0, GFXTSSAlphaArg1);
    U32 alphaArg2 = GFX->getTextureStageState(0, GFXTSSAlphaArg2);

    // Texture
    //GFX->setTextureStageAddressModeU (0, GFXAddressClamp);
    //GFX->setTextureStageAddressModeV (0, GFXAddressClamp);
    //GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);

    GFX->setTextureStageColorArg1(0, GFXTATexture);
    GFX->setTextureStageColorArg2(0, GFXTADiffuse);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);

    GFX->setTextureStageAlphaArg1(0, GFXTATexture);
    GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);
    GFX->setTextureStageAlphaOp(0, GFXTOPModulate);

    // Z
    GFX->setZWriteEnable(false);

    // Alpha
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendOne);

    GFX->setAlphaTestEnable(true);
    GFX->setAlphaFunc(GFXCmpGreater);
    GFX->setAlphaRef(0);

    GFX->setCullMode(GFXCullNone);

    // Geometry
    GFX->setTexture(0, mLightHandle);

    GFX->setVertexBuffer(mVerts);
    GFX->setPrimitiveBuffer(mPrims);
    makeDIPCall();
    GFX->setVertexBuffer(NULL);
    GFX->setPrimitiveBuffer(NULL);

    GFX->setTexture(0, NULL);

    // Cleanup
    GFX->setTextureStageColorOp(0, GFXTOPSelectARG1);
    GFX->setTextureStageColorArg1(0, GFXTATexture);
    GFX->setTextureStageColorArg2(0, colorarg2);

    GFX->setTextureStageAlphaOp(0, GFXTOPSelectARG1);//(GFXTextureOp)alphaOp);
    GFX->setTextureStageAlphaArg1(0, alphaArg1);
    GFX->setTextureStageAlphaArg2(0, alphaArg2);

    GFX->setTextureStageColorOp(1, (GFXTextureOp)stage1ColorOp);
    GFX->setTextureStageAlphaOp(1, (GFXTextureOp)stage1AlphaOp);

    //GFX->setTextureStageAddressModeU(0, GFXAddressWrap);
    //GFX->setTextureStageAddressModeV(0, GFXAddressWrap);

    GFX->setCullMode(GFXCullCCW);

    GFX->setZWriteEnable(true);

    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);
    GFX->setAlphaBlendEnable(false);

    GFX->setAlphaFunc(GFXCmpGreaterEqual);
    GFX->setAlphaRef(1);
    GFX->setAlphaTestEnable(false);

    //GFX->setStencilEnable(false);
    //GFX->setZEnable(true);
    GFX->setZWriteEnable(true);

    GFX->setBaseRenderState();

    GFX->popWorldMatrix();
}

void VolumeLight::initPersistFields()
{
    // Initialise parents' persistent fields.
    Parent::initPersistFields();

    addGroup("Media", "Media for the volume light.");
    addField("texture", TypeFilename, Offset(mLightTexture, VolumeLight));
    endGroup("Media");

    // Dimensions
    addGroup("Dimensions", "Information about the size and quality of the volume light.");
    addField("lpDistance", TypeF32, Offset(mLPDistance, VolumeLight));
    addField("shootDistance", TypeF32, Offset(mShootDistance, VolumeLight));
    addField("xExtent", TypeF32, Offset(mXExtent, VolumeLight));
    addField("yExtent", TypeF32, Offset(mYExtent, VolumeLight));
    addField("subdivideU", TypeS32, Offset(mSubdivideU, VolumeLight));
    addField("subdivideV", TypeS32, Offset(mSubdivideV, VolumeLight));
    endGroup("Dimensions");

    // Colors
    addGroup("Colors", "Colors for the volume light.");
    addField("footColor", TypeColorF, Offset(mFootColor, VolumeLight));
    addField("tailColor", TypeColorF, Offset(mTailColor, VolumeLight));
    endGroup("Colors");
}
