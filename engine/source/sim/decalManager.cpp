//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sim/decalManager.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "ts/tsShapeInstance.h"
#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderInstMgr.h"

bool DecalManager::smDecalsOn = true;

bool DecalManager::sgThisIsSelfIlluminated = false;
bool DecalManager::sgLastWasSelfIlluminated = false;

const U32 DecalManager::csmFreePoolBlockSize = 256;
U32       DecalManager::smMaxNumDecals = 256;
U32       DecalManager::smDecalTimeout = 5000;

DecalManager* gDecalManager = NULL;
IMPLEMENT_CONOBJECT(DecalManager);
IMPLEMENT_CO_DATABLOCK_V1(DecalData);

namespace {

    int QSORT_CALLBACK cmpDecalInstance(const void* p1, const void* p2)
    {
        const DecalInstance** pd1 = (const DecalInstance**)p1;
        const DecalInstance** pd2 = (const DecalInstance**)p2;

        return int(((char*)(*pd1)->decalData) - ((char*)(*pd2)->decalData));
    }

} // namespace {}


//--------------------------------------------------------------------------
DecalData::DecalData()
{
    sizeX = 1;
    sizeY = 1;
    textureName = "";

    selfIlluminated = false;
    lifeSpan = DecalManager::smDecalTimeout;
}

DecalData::~DecalData()
{
    if (gDecalManager)
        gDecalManager->dataDeleted(this);
}


void DecalData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->write(sizeX);
    stream->write(sizeY);
    stream->writeString(textureName);

    stream->write(selfIlluminated);
    stream->write(lifeSpan);
}

void DecalData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    stream->read(&sizeX);
    stream->read(&sizeY);
    textureName = stream->readSTString();

    stream->read(&selfIlluminated);
    stream->read(&lifeSpan);
}

bool DecalData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    if (sizeX < 0.0) {
        Con::warnf("DecalData::preload: sizeX < 0");
        sizeX = 0;
    }
    if (sizeY < 0.0) {
        Con::warnf("DecalData::preload: sizeX < 0");
        sizeY = 0;
    }
    if (textureName == NULL || textureName[0] == '\0') {
        Con::errorf("No texture name for decal!");
        return false;
    }

    if (!server)
    {
        if (!textureHandle.set(textureName, &GFXDefaultStaticDiffuseProfile))
        {
            Con::errorf("Unable to load texture: %s for decal!", textureName);
            return false;
        }
    }

    return true;
}

//IMPLEMENT_SETDATATYPE(DecalData)
//IMPLEMENT_GETDATATYPE(DecalData)

void DecalData::initPersistFields()
{
    //Con::registerType("DecalDataPtr", TypeDecalDataPtr, sizeof(DecalData*),
    //                  REF_GETDATATYPE(DecalData),
    //                  REF_SETDATATYPE(DecalData));

    addField("sizeX", TypeF32, Offset(sizeX, DecalData));
    addField("sizeY", TypeF32, Offset(sizeY, DecalData));
    addField("textureName", TypeFilename, Offset(textureName, DecalData));

    addField("selfIlluminated", TypeBool, Offset(selfIlluminated, DecalData));
    addField("lifeSpan", TypeS32, Offset(lifeSpan, DecalData));
}


DecalManager::DecalManager()
{
    mTypeMask |= DecalManagerObjectType;

    mObjBox.min.set(-1e7, -1e7, -1e7);
    mObjBox.max.set(1e7, 1e7, 1e7);
    mWorldBox.min.set(-1e7, -1e7, -1e7);
    mWorldBox.max.set(1e7, 1e7, 1e7);

    mFreePool = NULL;
    VECTOR_SET_ASSOCIATION(mDecalQueue);
    VECTOR_SET_ASSOCIATION(mFreePoolBlocks);
}


DecalManager::~DecalManager()
{
    mFreePool = NULL;
    for (S32 i = 0; i < mFreePoolBlocks.size(); i++)
    {
        delete[] mFreePoolBlocks[i];
    }
    mDecalQueue.clear();
}


DecalInstance* DecalManager::allocateDecalInstance()
{
    if (mFreePool == NULL)
    {
        // Allocate a new block of decals
        mFreePoolBlocks.push_back(new DecalInstance[csmFreePoolBlockSize]);

        // Init them onto the free pool chain
        DecalInstance* pNewInstances = mFreePoolBlocks.last();
        for (U32 i = 0; i < csmFreePoolBlockSize - 1; i++)
            pNewInstances[i].next = &pNewInstances[i + 1];
        pNewInstances[csmFreePoolBlockSize - 1].next = NULL;
        mFreePool = pNewInstances;
    }
    AssertFatal(mFreePool != NULL, "Error, should always have a free pool available here!");

    DecalInstance* pRet = mFreePool;
    mFreePool = pRet->next;
    pRet->next = NULL;
    return pRet;
}


void DecalManager::freeDecalInstance(DecalInstance* trash)
{
    AssertFatal(trash != NULL, "Error, no trash pointer to free!");

    trash->next = mFreePool;
    mFreePool = trash;
}


void DecalManager::dataDeleted(DecalData* data)
{
    for (S32 i = mDecalQueue.size() - 1; i >= 0; i--)
    {
        DecalInstance* inst = mDecalQueue[i];
        if (inst->decalData == data)
        {
            freeDecalInstance(inst);
            mDecalQueue.erase(U32(i));
        }
    }
}

void DecalManager::consoleInit()
{
    Con::addVariable("$pref::decalsOn", TypeBool, &smDecalsOn);
    Con::addVariable("$pref::Decal::maxNumDecals", TypeS32, &smMaxNumDecals);
    Con::addVariable("$pref::Decal::decalTimeout", TypeS32, &smDecalTimeout);
}

void DecalManager::addDecal(const Point3F& pos,
    Point3F normal,
    DecalData* decalData)
{
    if (smMaxNumDecals == 0)
        return;

    // DMM: Rework this, should be based on time
    if (mDecalQueue.size() >= smMaxNumDecals)
    {
        findSpace();
    }

    Point3F vecX, vecY;
    DecalInstance* newDecal = allocateDecalInstance();
    newDecal->decalData = decalData;
    newDecal->allocTime = Platform::getVirtualMilliseconds();

    if (mFabs(normal.z) > 0.9f)
        mCross(normal, Point3F(0.0f, 1.0f, 0.0f), &vecX);
    else
        mCross(normal, Point3F(0.0f, 0.0f, 1.0f), &vecX);

    mCross(vecX, normal, &vecY);

    normal.normalizeSafe();
    Point3F position = Point3F(pos.x + (normal.x * 0.008), pos.y + (normal.y * 0.008), pos.z + (normal.z * 0.008));

    vecX.normalizeSafe();
    vecY.normalizeSafe();

    vecX *= decalData->sizeX;
    vecY *= decalData->sizeY;

    newDecal->point[0] = position + vecX + vecY;
    newDecal->point[1] = position + vecX - vecY;
    newDecal->point[2] = position - vecX - vecY;
    newDecal->point[3] = position - vecX + vecY;

    mDecalQueue.push_back(newDecal);
    mQueueDirty = true;
}

void DecalManager::addDecal(const Point3F& pos,
    const Point3F& rot,
    Point3F normal,
    DecalData* decalData)
{
    if (smMaxNumDecals == 0)
        return;

    addDecal(pos, rot, normal, Point3F(1, 1, 1), decalData);
}

void DecalManager::addDecal(const Point3F& pos,
    const Point3F& rot,
    Point3F normal,
    const Point3F& scale,
    DecalData* decalData)
{
    if (smMaxNumDecals == 0)
        return;

    if (mDot(rot, normal) < 0.98)
    {
        // DMM: Rework this, should be based on time
        if (mDecalQueue.size() >= smMaxNumDecals)
        {
            findSpace();
        }

        Point3F vecX, vecY;
        DecalInstance* newDecal = allocateDecalInstance();
        newDecal->decalData = decalData;
        newDecal->allocTime = Platform::getVirtualMilliseconds();

        mCross(rot, normal, &vecX);
        mCross(normal, vecX, &vecY);

        normal.normalize();
        Point3F position = Point3F(pos.x + (normal.x * 0.008), pos.y + (normal.y * 0.008), pos.z + (normal.z * 0.008));

        vecX.normalize();
        vecX.convolve(scale);
        vecY.normalize();
        vecY.convolve(scale);

        vecX *= decalData->sizeX;
        vecY *= decalData->sizeY;

        newDecal->point[0] = position + vecX + vecY;
        newDecal->point[1] = position + vecX - vecY;
        newDecal->point[2] = position - vecX - vecY;
        newDecal->point[3] = position - vecX + vecY;

        mDecalQueue.push_back(newDecal);
        mQueueDirty = true;
    }
}

bool DecalManager::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (!smDecalsOn) return false;

    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    if (mDecalQueue.size() == 0)
        return false;

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
 /*   SceneRenderImage* image = new SceneRenderImage;
    image->obj = this;
    image->isTranslucent = true;
    image->sortType      = SceneRenderImage::BeginSort;
    state->insertRenderImage(image);
 */
    RenderInst* ri = gRenderInstManager.allocInst();
    ri->obj = this;
    ri->state = state;
    ri->type = RenderInstManager::RIT_Decal;
    //ri->calcSortPoint(this, state->getCameraPosition());
    gRenderInstManager.addInst(ri);

    U32 currMs = Platform::getVirtualMilliseconds();
    for (S32 i = mDecalQueue.size() - 1; i >= 0; i--)
    {
        U32 age = currMs - mDecalQueue[i]->allocTime;
        U32 timeout = mDecalQueue[i]->decalData->lifeSpan;
        if (age > timeout)
        {
            freeDecalInstance(mDecalQueue[i]);
            mDecalQueue.erase(i);
        }
        else if (age > ((3 * timeout) / 4))
        {
            mDecalQueue[i]->fade = 1.0f - (F32(age - ((3 * timeout) / 4)) / F32(timeout / 4));
        }
        else
        {
            mDecalQueue[i]->fade = 1.0f;
        }
    }

    if (mQueueDirty == true)
    {
        // Sort the decals based on the data pointers...
        dQsort(mDecalQueue.address(),
            mDecalQueue.size(),
            sizeof(DecalInstance*),
            cmpDecalInstance);
        mQueueDirty = false;
    }

    return false;
}

void DecalManager::renderObject(SceneState* state, RenderInst*)
{
    if (!smDecalsOn) return;

    //AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

    //RectI viewport;
    //glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    //dglGetViewport(&viewport);
    MatrixF projection = GFX->getProjectionMatrix();
    RectI viewport = GFX->getViewport();

    GFX->pushWorldMatrix();
    //MatrixF m;
    //m.identity();
    //GFX->setWorldMatrix(m);
    GFX->disableShaders();

    //state->setupBaseProjection();

    //glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();

    renderDecal();

    GFX->setTextureStageColorOp(0, GFXTOPDisable);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    //glMatrixMode(GL_MODELVIEW);
    //glPopMatrix();

    //glMatrixMode(GL_PROJECTION);
    //glPopMatrix();
    //glMatrixMode(GL_MODELVIEW);
    //dglSetViewport(viewport);

    GFX->popWorldMatrix();

    GFX->setViewport(viewport);
    GFX->setProjectionMatrix(projection);

    //AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

void DecalManager::renderDecal()
{
    GFX->setCullMode(GFXCullNone);
    GFX->setLightingEnable(false);
    GFX->setAlphaBlendEnable(true);
    GFX->setZEnable(true);
    GFX->setZFunc(GFXCmpLessEqual);
    GFX->setZWriteEnable(false);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);
    GFX->setAlphaTestEnable(true);
    GFX->setAlphaFunc(GFXCmpGreater);
    GFX->setAlphaRef(0);
    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPDisable);

    //glEnable(GL_TEXTURE_2D);
    //glEnable(GL_BLEND);
    //glEnable(GL_ALPHA_TEST);
    //glDepthMask(GL_FALSE);
    //glAlphaFunc(GL_GREATER, 0.1f);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    sgThisIsSelfIlluminated = false;
    sgLastWasSelfIlluminated = false;

    F32 depthbias = -0.00002f;
    F32 slopebias = -0.06f;
    GFX->setZBias(*((U32*)&depthbias));
    GFX->setSlopeScaleDepthBias(*((U32*)&slopebias));

    //glEnable(GL_POLYGON_OFFSET_FILL);
     //glPolygonOffset(-1,-1);
     //glDepthMask(GL_FALSE);

    DecalData* pLastData = NULL;
    for (S32 x = 0; x < mDecalQueue.size(); x++)
    {
        if (mDecalQueue[x]->decalData != pLastData)
        {
            //glBindTexture(GL_TEXTURE_2D, mDecalQueue[x]->decalData->textureHandle.getGLName());
            GFX->setTexture(0, mDecalQueue[x]->decalData->textureHandle);
            pLastData = mDecalQueue[x]->decalData;
        }

        sgThisIsSelfIlluminated = mDecalQueue[x]->decalData->selfIlluminated;
        if (sgThisIsSelfIlluminated != sgLastWasSelfIlluminated)
        {
            if (sgThisIsSelfIlluminated)
            {
                GFX->setSrcBlend(GFXBlendSrcAlpha);
                GFX->setDestBlend(GFXBlendOne);
            }
            else
            {
                GFX->setSrcBlend(GFXBlendSrcAlpha);
                GFX->setDestBlend(GFXBlendInvSrcAlpha);
            }
            sgLastWasSelfIlluminated = sgThisIsSelfIlluminated;
        }

        PrimBuild::color4f(1, 1, 1, mDecalQueue[x]->fade);
        PrimBuild::begin(GFXTriangleFan, 4);
        PrimBuild::texCoord2f(0, 0);
        PrimBuild::vertex3fv(mDecalQueue[x]->point[3]);
        PrimBuild::texCoord2f(0, 1);
        PrimBuild::vertex3fv(mDecalQueue[x]->point[2]);
        PrimBuild::texCoord2f(1, 1);
        PrimBuild::vertex3fv(mDecalQueue[x]->point[1]);
        PrimBuild::texCoord2f(1, 0);
        PrimBuild::vertex3fv(mDecalQueue[x]->point[0]);
        PrimBuild::end();
    }

    GFX->setAlphaBlendEnable(false);
    GFX->setZEnable(true);
    GFX->setZWriteEnable(true);
    GFX->setAlphaTestEnable(false);
    GFX->setTextureStageColorOp(0, GFXTOPDisable);

    GFX->setZBias(0);
    GFX->setSlopeScaleDepthBias(0);

    //glDisable(GL_POLYGON_OFFSET_FILL);
    //glDepthMask(GL_TRUE);

   //glDisable(GL_BLEND);
   //glDisable(GL_TEXTURE_2D);
   //glDisable(GL_ALPHA_TEST);
}

void DecalManager::findSpace()
{
    S32 besttime = S32_MAX;
    U32 bestindex = 0;
    DecalInstance* bestdecal = NULL;

    U32 time = Platform::getVirtualMilliseconds();

    for (U32 i = 0; i < mDecalQueue.size(); i++)
    {
        DecalInstance* inst = mDecalQueue[i];
        U32 age = time - inst->allocTime;
        U32 timeleft = inst->decalData->lifeSpan - age;
        if (besttime > timeleft)
        {
            besttime = timeleft;
            bestindex = i;
            bestdecal = inst;
        }
    }

    AssertFatal((bestdecal), "No good decals?");

    mDecalQueue.erase_fast(bestindex);
    freeDecalInstance(bestdecal);
}

void DecalManager::addDecal(const Point3F& pos, const Point3F& rot, Point3F normal,
    const Point3F& scale, DecalData* decaldata, U32 ownerid)
{
    if (smMaxNumDecals == 0)
        return;

    if (mDot(rot, normal) < 0.98)
    {
        if (mDecalQueue.size() >= smMaxNumDecals)
            findSpace();

        Point3F vecX, vecY;
        DecalInstance* newDecal = allocateDecalInstance();
        newDecal->decalData = decaldata;
        newDecal->allocTime = Platform::getVirtualMilliseconds();
        newDecal->ownerId = ownerid;

        mCross(rot, normal, &vecX);
        mCross(normal, vecX, &vecY);

        normal.normalize();
        Point3F position = Point3F(pos.x + (normal.x * 0.008), pos.y + (normal.y * 0.008), pos.z + (normal.z * 0.008));

        vecX.normalize();
        vecX.convolve(scale);
        vecY.normalize();
        vecY.convolve(scale);

        vecX *= decaldata->sizeX;
        vecY *= decaldata->sizeY;

        newDecal->point[0] = position + vecX + vecY;
        newDecal->point[1] = position + vecX - vecY;
        newDecal->point[2] = position - vecX - vecY;
        newDecal->point[3] = position - vecX + vecY;

        mDecalQueue.push_back(newDecal);
        mQueueDirty = true;
    }
}

void DecalManager::ageDecal(U32 ownerid)
{
    for (U32 i = 0; i < mDecalQueue.size(); i++)
    {
        DecalInstance* inst = mDecalQueue[i];
        if (inst->ownerId == ownerid)
        {
            freeDecalInstance(inst);
            mDecalQueue.erase(U32(i));
        }
    }
}

