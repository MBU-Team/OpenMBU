//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasInstance.h"
#include "atlas/atlasInstanceEntry.h"
#include "atlas/atlasResource.h"
#include "atlas/atlasResourceEntry.h"
#include "util/frustrumCuller.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "materials/sceneData.h"
#include "platform/profiler.h"
#include "core/bitStream.h"
#include "math/mathIO.h"
#include "gfx/primBuilder.h"
#include "game/game.h"
#include "renderInstance/renderInstMgr.h"

IMPLEMENT_CO_NETOBJECT_V1(AtlasInstance);

bool AtlasInstance::smShowChunkBounds = false;
bool AtlasInstance::smRenderWireframe = false;
bool AtlasInstance::smLockCamera = false;
bool AtlasInstance::smCollideChunkBoxes = false;
bool AtlasInstance::smCollideGeomBoxes = false;
FrustrumCuller AtlasInstance::smCuller;

// Try to be a little safer.
#ifdef TORQUE_MULTITHREAD
bool AtlasInstance::smSynchronousLoad = false;
#else
bool AtlasInstance::smSynchronousLoad = true;
#endif

AtlasInstance::AtlasInstance()
{
    // Set up our flags.
    mTypeMask = AtlasObjectType | StaticObjectType | StaticRenderedObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);

    // Set up some default paths. These will make your life hell.
    mChunkPath = StringTable->insert("demo/data/terrains/large.chu");
    mTQTPath = StringTable->insert("demo/data/terrains/generated2.tqt");
    mMaterialName = StringTable->insert("AtlasMaterial");
    mDetailName = StringTable->insert("terrain_water_demo/data/terrains/details/detail1");

    mAtlasResource = NULL;
    mChunks = NULL;
    mMaterial = NULL;
}

AtlasInstance::~AtlasInstance()
{
    SAFE_DELETE(mMaterial);
    SAFE_DELETE_ARRAY(mChunks);
}

bool AtlasInstance::onAdd()
{
    if (!Parent::onAdd())
        return false;

    if (isClientObject())
    {
        // Set up the shader for rendering the terrain surface.
        mMaterial = new MatInstance(mMaterialName);

        if (!mMaterial || !mMaterial->getMaterial())
        {
            Con::errorf("AtlasInstance::onAdd - failed to locate material '%s'!", mMaterialName);
            return false;
        }

        mDetailTex.set(mDetailName, &GFXDefaultStaticDiffuseProfile);
    }

    // Load chunk data.
    mAtlasResource = ResourceManager->load(mChunkPath);

    if (mAtlasResource.isNull())
    {
        Con::errorf("AtlasInstance::onAdd - couldn't load chunk file '%s'!", mChunkPath);
        return false;
    }

    mAtlasResource->incOwnership();

    // Only the client will page textures...
    if (!mAtlasResource->initialize(mChunkPath, (isClientObject() ? mTQTPath : NULL)))
    {
        Con::errorf("AtlasInstance::onAdd - mAtlasResource->initialize('%s', '%s') failed!", mChunkPath, mTQTPath);
        return false;
    }

    AtlasInstanceEntry::setInstance(this);
    AtlasResource::setCurrentResource(mAtlasResource);

    mAtlasResource->startLoader();

    mObjBox = mAtlasResource->mChunks[0].mBounds;

    // Set up our local instance entries.
    mChunks = new AtlasInstanceEntry[mAtlasResource->mEntryCount];

    for (S32 i = 0; i < mAtlasResource->mEntryCount; i++)
    {
        constructInPlace(&mChunks[i]);
        mChunks[i].init(&mAtlasResource->mChunks[i]);
    }

    if (isServerObject())
    {
        // Load server-necessary information... in this case we're talking about
        // all the leaves, so we can do collision.
        mAtlasResource->loadServerInfo(this);
    }

    if (isClientObject())
    {
        // Get the root chunk loaded, kthx!
        mChunks[0].warmUpData(1.f);
    }

#ifndef TORQUE_MULTITHREAD
    Con::errorf("AtlasInstance::onAdd - WARNING! Multithreading NOT enabled. Atlas Terrain will NOT background load.");
    Con::errorf("AtlasInstance::onAdd - WARNING! Multithreading NOT enabled. Expect hitching and bad performance!");
#endif

    AtlasResource::setCurrentResource(NULL);
    AtlasInstanceEntry::setInstance(NULL);

    // Do general render initialization stuff.
    resetWorldBox();
    setRenderTransform(mObjToWorld);
    addToScene();

    return true;
}

void AtlasInstance::onRemove()
{
    Parent::onRemove();

    removeFromScene();

    // And blast our loaded resources just to be sure. This can cause
    // strangeness in multi-instance situations.
    if (!mAtlasResource.isNull())
    {
        mAtlasResource->decOwnership();
        mAtlasResource = NULL;
    }
}

bool AtlasInstance::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // Make sure we play nice if we should be drawn no matter what.
    bool render = false;
    if (state->isTerrainOverridden())
        render = true;
    else
        render = state->isObjectRendered(this);

    if (render)
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Object;
        gRenderInstManager.addInst(ri);
    }

    return false;
}

void AtlasInstance::renderObject(SceneState* state, RenderInst* ri)
{
    PROFILE_START(ChunkInstance_renderObject);

    GFX->pushState();

    // Set up rendering state
    GFX->disableShaders();

    GFX->setTextureStageColorOp(0, GFXTOPModulate);
    GFX->setTextureStageColorOp(1, GFXTOPModulate);
    GFX->setTextureStageColorOp(2, GFXTOPModulate);
    GFX->setTextureStageColorOp(3, GFXTOPModulate);
    GFX->setTextureStageColorOp(4, GFXTOPDisable);
    GFX->setAlphaBlendEnable(false);

    GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);
    GFX->setTextureStageMipFilter(0, GFXTextureFilterLinear);

    GFX->setTextureStageMagFilter(1, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(1, GFXTextureFilterLinear);
    GFX->setTextureStageMipFilter(1, GFXTextureFilterLinear);

    GFX->setTextureStageMagFilter(2, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(2, GFXTextureFilterLinear);
    GFX->setTextureStageMipFilter(2, GFXTextureFilterLinear);

    GFX->setTextureStageMagFilter(3, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(3, GFXTextureFilterLinear);
    GFX->setTextureStageMipFilter(3, GFXTextureFilterLinear);

    GFX->setTextureStageMagFilter(4, GFXTextureFilterLinear);
    GFX->setTextureStageMinFilter(4, GFXTextureFilterLinear);
    GFX->setTextureStageMipFilter(4, GFXTextureFilterLinear);

    GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(0, GFXAddressClamp);
    // ^=- This texture is set by the quad tree as we recurse.

    GFX->setTextureStageAddressModeU(1, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(1, GFXAddressClamp);
    GFX->setTexture(1, getCurrentClientSceneGraph()->getFogTexture());

    // And the detail textures...
    GFX->setTextureStageAddressModeU(2, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(2, GFXAddressWrap);
    GFX->setTexture(2, mDetailTex);

    GFX->setTextureStageAddressModeU(3, GFXAddressWrap);
    GFX->setTextureStageAddressModeV(3, GFXAddressWrap);
    GFX->setTexture(3, mDetailTex);

    if (smRenderWireframe)
        GFX->setFillMode(GFXFillWireframe);

    GFX->setCullMode(GFXCullCCW);

    SceneGraphData sgData;

    // Set up projection and world transform info.
    MatrixF proj = GFX->getProjectionMatrix();
    GFX->pushWorldMatrix();
    GFX->multWorld(getRenderTransform());
    MatrixF world = GFX->getWorldMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    // Store object and camera transform data
    sgData.objTrans = getRenderTransform();
    sgData.camPos = state->getCameraPosition();

    // Set up fogging.
    sgData.useFog = true;
    sgData.fogTex = getCurrentClientSceneGraph()->getFogTexture();
    sgData.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
    sgData.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
    sgData.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

    // grab the sun data from the light manager
    const LightInfo* sunlight = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
    VectorF sunVector = sunlight->mDirection; //first light is always sun

    // set the sun data into scenegraph data
    sgData.light.mDirection.set(sunVector);
    sgData.light.mPos.set(sunVector * -10000.0);
    sgData.light.mAmbient = sunlight->mAmbient;
    sgData.light.mColor = sunlight->mColor;


    PROFILE_START(ChunkInstance_render);

    // Synch with the AtlasResource, process our LOD, and render...
    mAtlasResource->sync();
    processLOD(state, getRenderTransform());

    mMaterial->init(sgData, (GFXVertexFlags)getGFXVertFlags((GFXAtlasVert*)(NULL)));

    while (mMaterial->setupPass(sgData))
    {
        if (getCurrentClientSceneGraph()->isReflectPass())
            GFX->setCullMode(GFXCullCW);
        else
            GFX->setCullMode(GFXCullCCW);

        render(&sgData);
    }

    PROFILE_END();

    if (smRenderWireframe)
        GFX->setFillMode(GFXFillSolid);

    GFX->popWorldMatrix();
    GFX->popState();

    PROFILE_END();
}


void AtlasInstance::consoleInit()
{
    Con::addVariable("$AtlasInstance::showChunkBounds", TypeBool, &smShowChunkBounds);
    Con::addVariable("$AtlasInstance::renderWireframe", TypeBool, &smRenderWireframe);
    Con::addVariable("$AtlasInstance::lockCamera", TypeBool, &smLockCamera);
    Con::addVariable("$AtlasInstance::collideChunkBoxes", TypeBool, &smCollideChunkBoxes);
    Con::addVariable("$AtlasInstance::collideGeomBoxes", TypeBool, &smCollideGeomBoxes);

    // If we don't have multithread, no sense letting people touch this.
#ifdef TORQUE_MULTITHREAD
    Con::addVariable("$AtlasInstance::synchronousLoad", TypeBool, &smSynchronousLoad);
#endif
}

void AtlasInstance::initPersistFields()
{
    Parent::initPersistFields();

    addField("chunkFile", TypeFilename, Offset(mChunkPath, AtlasInstance),
        "Path to the file we will be loading geometry data from.");
    addField("tqtFile", TypeFilename, Offset(mTQTPath, AtlasInstance),
        "Path to the file we will be loading texture data from.");
    addField("materialName", TypeString, Offset(mMaterialName, AtlasInstance),
        "Name of our Material. Must be specialized for terrain usage.");
    addField("detailTexture", TypeFilename, Offset(mDetailName, AtlasInstance),
        "Detail texture.");
}

U32 AtlasInstance::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(conn, mask, stream);

    mathWrite(*stream, getTransform());
    stream->writeString(mChunkPath);
    stream->writeString(mTQTPath);
    stream->writeString(mMaterialName);
    stream->writeString(mDetailName);
    return retMask;
}

void AtlasInstance::unpackUpdate(NetConnection* conn, BitStream* stream)
{
    Parent::unpackUpdate(conn, stream);

    MatrixF tmp;
    mathRead(*stream, &tmp);

    setTransform(tmp);
    setRenderTransform(tmp);

    mChunkPath = stream->readSTString();
    mTQTPath = stream->readSTString();
    mMaterialName = stream->readSTString();
    mDetailName = stream->readSTString();
}

bool AtlasInstance::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    bool res;

    AtlasInstanceEntry::setInstance(this);
    res = mAtlasResource->castRay(start, end, info, false);
    AtlasInstanceEntry::setInstance(NULL);

    return res;
}

void AtlasInstance::buildConvex(const Box3F& box, Convex* convex)
{
    // Get the box into local space and pass it off to the AtlasResource.
    Box3F localBox = box;
    mWorldToObj.mul(localBox);

    AtlasInstanceEntry::setInstance(this);
    if (mAtlasResource)
        mAtlasResource->buildCollisionInfo(localBox, convex, NULL);
    AtlasInstanceEntry::setInstance(NULL);
}

bool AtlasInstance::buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    // Get the box into local space and pass it off to the AtlasResource.
    Box3F localBox = box;
    mWorldToObj.mul(localBox);

    polyList->setObject(this);
    polyList->setTransform(&getTransform(), getScale());

    // Since the polylist transforms for us we don't have to deal with
    // the transform stuff convexes do.
    return mAtlasResource->buildCollisionInfo(localBox, NULL, polyList);
}

void AtlasInstance::purge()
{
    if (mAtlasResource)
        mAtlasResource->purge();
}

ConsoleMethod(AtlasInstance, purge, void, 2, 2, "() - Purge all loaded chunks.")
{
    object->purge();
}

//------------------------------------------------------------------------


U16 AtlasInstance::computeLod(const Box3F& bounds, const Point3F& camPos)
{
    const F32 d = AtlasInstance::smCuller.getBoxDistance(bounds);
    return mClamp(((mAtlasResource->mTreeDepth << 8) - 1) - S32(getFastBinLog2(getMax(F32(1), d / mDistance_LODMax)) * 256.f), 0, 0xFFFF);
}

S32 AtlasInstance::computeTextureLod(const Box3F& bounds, const Point3F& camPos)
{
    const F32 d = AtlasInstance::smCuller.getBoxDistance(bounds);
    return mAtlasResource->mTreeDepth - 1 - S32(getFastBinLog2(getMax(F32(1), F32(d / mTextureDistance_LODMax))));
}

//------------------------------------------------------------------------

void AtlasInstance::processLOD(SceneState* state, const MatrixF& renderTrans)
{
    AtlasResource::setCurrentResource(mAtlasResource);
    AtlasInstanceEntry::setInstance(this);

    // Set our statics, unless we're locking the camera.
    if (!smLockCamera)
    {
        AtlasResource::smCurCamPos = state->getCameraPosition();
        AtlasInstance::smCuller.init(state);
    }

    // Store the inverse object space transform.
    AtlasResource::smInvObjTrans = renderTrans;
    AtlasResource::smInvObjTrans.inverse();
    AtlasResource::smInvObjTrans.mulP(state->getCameraPosition(), &AtlasResource::smCurObjCamPos);

    // Do the processing if we're not in a reflection.
    if (!getCurrentClientSceneGraph()->isReflectPass())
    {

        const F32 tan_half_FOV = tanf(0.5f * GameGetCameraFov() /** (float) M_PI / 180.0f */);
        const F32 K = GFX->getViewport().extent.x / tan_half_FOV;

        const F32 maxPixelError = 16.f;
        const F32 max_texel_size = 16.f;

        // Set some silly default LOD settings. These should be programmatically
        // calculated. They define the distance under which we should be at 
        // maximum LOD.
        mDistance_LODMax = 100;
        mTextureDistance_LODMax = 90;

        PROFILE_START(AtlasData__clear);
        mChunks[0].clear();
        PROFILE_END();

        PROFILE_START(AtlasData__updateLOD);
        mChunks[0].updateLOD();
        PROFILE_END();

        PROFILE_START(AtlasData__updateTexture);
        mChunks[0].updateTexture();
        PROFILE_END();
    }

    AtlasResource::setCurrentResource(NULL);
    AtlasInstanceEntry::setInstance(NULL);
}

void AtlasInstance::render(SceneGraphData* sgd)
{
    // Set statics.
    AtlasResource::smCurSGD = sgd;
    AtlasResource::setCurrentResource(mAtlasResource);
    AtlasInstanceEntry::setInstance(this);

    // Render.
    mChunks[0].render(FrustrumCuller::AllClipPlanes);

    AtlasResource::setCurrentResource(NULL);
    AtlasInstanceEntry::setInstance(NULL);
}
