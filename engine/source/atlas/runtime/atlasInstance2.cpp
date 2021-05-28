//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "atlas/runtime/atlasInstance2.h"
#include "atlas/resource/atlasResourceConfigTOC.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "game/game.h"
#include "core/bitStream.h"
#include "gfx/debugDraw.h"
#include "renderInstance/renderInstMgr.h"
#include "lightingSystem/sgLightingModel.h"
#include "math/mathUtils.h"
#include "atlas/runtime/atlasClipMapUniqueCache.h"
#include "atlas/runtime/atlasClipMapBlenderCache.h"
#include "atlas/runtime/atlasClipMap.h"
#include "atlas/runtime/atlasClipMapBatcher.h"
#include "../../game/shaders/shdrConsts.h"

IMPLEMENT_CO_NETOBJECT_V1(AtlasInstance2);
S32 AtlasInstance2::smRayCollisionDebugLevel = AtlasInstance2::RayCollisionDebugToTriangles;
S32 AtlasInstance2::smRenderChunkBoundsDebugLevel = AtlasInstance2::ChunkBoundsDebugNone;
bool AtlasInstance2::smLockFrustrum = false;
bool AtlasInstance2::smRenderWireframe = false;


void AtlasInstance2::findDeepestStubs(Vector<AtlasResourceGeomTOC::StubType*>& stubs)
{
    if (!mGeomTOC)
        return;

    AtlasResourceGeomTOC* toc = mGeomTOC->getResourceTOC();
    U32 treeDepth = toc->getTreeDepth();

    for (S32 x = 0; x < BIT(treeDepth - 1); x++)
    {
        for (S32 y = 0; y < BIT(treeDepth - 1); y++)
        {
            stubs.push_back(toc->getStub(treeDepth - 1, Point2I(x, y)));
        }
    }
}

void AtlasInstance2::consoleInit()
{
    Con::addVariable("AtlasInstance2::rayCollisionDebugLevel", TypeS32, &smRayCollisionDebugLevel);
    Con::addVariable("AtlasInstance2::renderChunkBoundsDebugLevel", TypeS32, &smRenderChunkBoundsDebugLevel);
    Con::addVariable("AtlasInstance2::lockFrustrum", TypeBool, &smLockFrustrum);
    Con::addVariable("AtlasInstance2::renderWireframe", TypeBool, &smRenderWireframe);
    Con::addVariable("AtlasInstance2::renderDebugTextures", TypeBool, &AtlasClipMapBatcher::smRenderDebugTextures);
}

void AtlasInstance2::initPersistFields()
{
    Parent::initPersistFields();

    addField("detailTex", TypeFilename, Offset(mDetailTexFileName, AtlasInstance2));
    addField("atlasFile", TypeFilename, Offset(mAtlasFileName, AtlasInstance2));

    addField("lightMapSize", TypeS32, Offset(lightMapSize, AtlasInstance2));
}

//-----------------------------------------------------------------------------

AtlasInstance2::AtlasInstance2()
{
    mTypeMask = AtlasObjectType | StaticObjectType |
        StaticRenderedObjectType | ShadowCasterObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);

    lightMapDirty = false;
    lightMapSize = 1024;
    lightMapTex = NULL;

    mAtlasFile = NULL;
    mGeomTOC = NULL;
    mDetailTexFileName = StringTable->insert("terrain_water_demo/data/terrains/details/detail1");
    mAtlasFileName = StringTable->insert("terrain_water_demo/clean.atlas");

    mClipMap = NULL;
}

AtlasInstance2::~AtlasInstance2()
{
}

bool AtlasInstance2::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mAtlasFile = AtlasFile::load(mAtlasFileName);

    if (mAtlasFile.isNull())
    {
        Con::errorf("AtlasInstance::onAdd - cannot open atlas file.");
        return false;
    }

    mAtlasFile->startLoaderThreads();

    // Bind the TOCs.
    AtlasResourceGeomTOC* argtoc = NULL;
    AtlasResourceConfigTOC* arctoc = NULL;
    mAtlasFile->getTocBySlot(0, argtoc);
    mAtlasFile->getTocBySlot(0, arctoc);

    mGeomTOC = new AtlasInstanceGeomTOC;
    mGeomTOC->initializeTOC(argtoc);

    // Do configuration based initialization.
    if (arctoc)
    {
        if (isClientObject())
        {
            AtlasConfigChunk* acc = NULL;
            if (arctoc->getConfig("schema", acc))
            {
                // Is this a unique or a blender TOC?
                const char* schemaType = NULL;
                if (acc->getEntry("type", schemaType))
                {
                    if (!dStricmp(schemaType, "unique"))
                    {
                        // Grab the TOC - first texture TOC is what we want.
                        AtlasResourceTexTOC* arttoc;
                        if (mAtlasFile->getTocBySlot(0, arttoc))
                        {
                            // Set up a clipmap with appropriate settings.
                            mClipMap = new AtlasClipMap();
                            mClipMap->mClipMapSize = Con::getIntVariable("Pref::Atlas::clipMapSize", 512);

                            // Do unique texture setup - this also initializes the texture size on clipmap.
                            AtlasClipMapImageCache_TextureTOC* acmic_ttoc = new AtlasClipMapImageCache_TextureTOC();
                            acmic_ttoc->setTOC(arttoc, mClipMap);

                            // Assign cache to clipmap, and bob's our uncle. HI BOB.
                            mClipMap->setCache(acmic_ttoc);
                        }
                        else
                        {
                            Con::errorf("AtlasInstance2::onAdd - no texture TOC present, cannot initialize unique texturing!");
                        }

                    }
                    else if (!dStricmp(schemaType, "blender"))
                    {
                        // Get what slots our TOCs are in.
                        const char* opacityTocSlot, * shadowTocSlot;
                        bool gotTocSlotsOk = true;
                        gotTocSlotsOk &= acc->getEntry("opacityMapSlot", opacityTocSlot);
                        gotTocSlotsOk &= acc->getEntry("shadowMapSlot", shadowTocSlot);

                        if (!gotTocSlotsOk)
                        {
                            Con::errorf("AtlasInstance2::onAdd - unable to get opacity or shadow TOC"
                                " slot information from config block.");
                            goto endOfBlenderInitBlock;
                        }

                        // Grab TOCs.
                        bool tocInitOk = true;
                        AtlasResourceTexTOC* shadowToc = NULL, * opacityToc = NULL;
                        tocInitOk &= mAtlasFile->getTocBySlot(dAtoi(shadowTocSlot), shadowToc);
                        tocInitOk &= mAtlasFile->getTocBySlot(dAtoi(opacityTocSlot), opacityToc);

                        if (!tocInitOk)
                        {
                            Con::errorf("AtlasInstance2::onAdd - unable to grab opacity or shadow TOC.");
                            goto endOfBlenderInitBlock;
                        }

                        // Initialize the clipmap.
                        mClipMap = new AtlasClipMap();
                        mClipMap->mClipMapSize = Con::getIntVariable("Pref::Atlas::clipMapSize", 512);

                        // What size should the virtual texture be?
                        const char* virtualTexSize;
                        if (!acc->getEntry("virtualTexSize", virtualTexSize) || dAtoi(virtualTexSize) == 0)
                        {
                            Con::errorf("AtlasInstance2::onAdd - no virtual texture Size specified in config block or got zero size.");
                            goto endOfBlenderInitBlock;
                        }

                        mClipMap->mTextureSize = dAtoi(virtualTexSize);

                        // Set up the blender image cache.
                        AtlasClipMapImageCache_Blender* acmic_b = new AtlasClipMapImageCache_Blender();
                        acmic_b->setTOC(opacityToc, shadowToc, mClipMap);
                        mClipMap->setCache(acmic_b);

                        // And grab source images.
                        const char* srcImageCount;
                        if (!acc->getEntry("sourceImageCount", srcImageCount))
                        {
                            Con::errorf("AtlasInstance2::onAdd - no source image count specified.");
                            goto endOfBlenderInitBlock;
                        }

                        S32 srcCount = dAtoi(srcImageCount);
                        for (S32 i = 0; i < srcCount; i++)
                        {
                            const char* srcImage;
                            if (!acc->getEntry(avar("sourceImage%d", i), srcImage))
                            {
                                Con::errorf("AtlasInstance2::onAdd - no source image specified at index %d!", i);
                                continue;
                            }

                            acmic_b->registerSourceImage(srcImage);
                        }

                    endOfBlenderInitBlock:;

                    }
                    else
                    {
                        Con::errorf("AtlasInstance2::onAdd - unknown texture schema type '%s'", schemaType);
                    }
                }
                else
                {
                    Con::errorf("AtlasInstance2::onAdd - no texture schema type specified", schemaType);
                }
            }
            else
            {
                Con::errorf("AtlasInstance2::onAdd - no texture schema present.");
            }

            initLightMap();

            if (mClipMap)
            {
                // We need to initialize the clipmap before we can do stuff to it!
                mClipMap->initClipStack();

                // If there is a clipmap, make sure it's all loaded and ready to go.
                U32 time = Platform::getRealMilliseconds();
                while (!mClipMap->fillWithTextureData())
                {
                    mAtlasFile->syncThreads();
                    Platform::sleep(10);
                }
                U32 postTime = Platform::getRealMilliseconds();
                Con::printf("AtlasInstance2::onAdd - took %d ms to fill clipmap with texture data.", postTime - time);
            }
            else
            {
                Con::errorf("AtlasInstance2::onAdd - failed to initialize clipmap!");
            }

            mDetailTex.set(mDetailTexFileName, &GFXDefaultStaticDiffuseProfile);
        }
    }

    // Root node contains all children, so we can just grab its bounds and go from there.
    AtlasResourceGeomStub* args = mGeomTOC->getResourceStub(mGeomTOC->getStub(0));
    mObjBox = args->mBounds;

    // Get our collision info as well.
    mGeomTOC->loadCollisionLeafChunks();

    // Do general render initialization stuff.
    setRenderTransform(mObjToWorld);
    resetWorldBox();
    addToScene();

    // Ok, all done.
    return true;
}

void AtlasInstance2::onRemove()
{
    // Clean up our Atlas references.
    SAFE_DELETE(mGeomTOC);

    mAtlasFile = NULL;

    // And let the rest of cleanup happen.
    removeFromScene();
    Parent::onRemove();
}

void AtlasInstance2::inspectPostApply()
{
    setMaskBits(0xffffffff);
}

bool AtlasInstance2::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 startZone, const bool modifyBaseZoneState)
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

    if (lightMapDirty)
    {
        lightMapTex.refresh();
        lightMapDirty = false;
    }

    return render;
}

void AtlasInstance2::renderObject(SceneState* state, RenderInst*)
{
    PROFILE_START(AtlasInstance2_renderObject);

    GFX->pushState();

    // Set up projection and world transform info.
    GFX->pushWorldMatrix();
    GFX->multWorld(getRenderTransform());
    MatrixF scaleMat(1);
    scaleMat.scale(getScale());
    GFX->multWorld(scaleMat);
    MatrixF world = GFX->getWorldMatrix();

    MatrixF proj = GFX->getProjectionMatrix();
    proj.mul(world);
    proj.transpose();
    GFX->setVertexShaderConstF(0, (float*)&proj, 4);

    Point4F terrainsize((mObjBox.max.x - mObjBox.min.x), (mObjBox.max.y - mObjBox.min.y), 0.0, 0.0);
    GFX->setVertexShaderConstF(46, (float*)&terrainsize, 1);

    // Synch up with the atlas file.
    if (!getCurrentClientSceneGraph()->isReflectPass())
    {
        // Set focus point.
        Point3F pos = state->getCameraPosition();
        mWorldToObj.mulP(pos);

        // Clamp focus point to the terrain bounds.
        Point3F focusPos;
        focusPos = mObjBox.getClosestPoint(pos);

        // Now, cast a ray to figure our texcoords at this location and recenter the clipmap.
        Point3F endPos;
        RayInfo ri;

        // First cast down...
        endPos = focusPos - Point3F(0, 0, 2.0 * mObjBox.len_z());

        if (mGeomTOC->castRay(focusPos, endPos, &ri, false))
        {
            mClipMap->recenter(ri.texCoord);
        }
        else
        {
            // Cast up.
            endPos = focusPos + Point3F(0, 0, 2.0 * mObjBox.len_z());
            if (mGeomTOC->castRay(focusPos, endPos, &ri, false))
                mClipMap->recenter(ri.texCoord);
        }

        // Sync up with threads once a frame - don't bother checking more than that.
        // Internally the file will filter down to one every 10ms anyway.
        mAtlasFile->syncThreads();

        // This clear time logic is actually kind of broken but it's ok as we
        // don't do anything in the clear() that uses dT. It would probably be
        // better to do heat dissipation from the processTick or somewhere else.
        static U32 lastTime = Sim::getCurrentTime();
        mGeomTOC->clear(F32(Sim::getCurrentTime() - lastTime) / 1000.f);
        lastTime = Sim::getCurrentTime();

        // And do our LOD processing.
        if (!smLockFrustrum)
            mGeomTOC->processLOD(state);
    }

    // Set up rendering state
    GFX->setBaseRenderState();
    GFX->disableShaders();

    // First draw the bounds, if desired.
    if (smRenderChunkBoundsDebugLevel)
        mGeomTOC->renderBounds(smRenderChunkBoundsDebugLevel);

    if (smRenderWireframe)
        GFX->setFillMode(GFXFillWireframe);

    GFX->setCullMode(state->mFlipCull ? GFXCullCW : GFXCullCCW);

    // Set up the object transform and eye position.
    Point3F eyePos = state->getCameraPosition();
    MatrixF objTrans = getRenderTransform() * scaleMat;

    objTrans.transpose();
    GFX->setVertexShaderConstF(VC_OBJ_TRANS, (float*)&objTrans, 4);

    objTrans = getRenderTransform() * scaleMat;
    objTrans.affineInverse();
    objTrans.mulP(eyePos);
    GFX->setVertexShaderConstF(VC_EYE_POS, (float*)&eyePos, 1);

    // Let our batcher draw everything.
    mBatcher.init(mClipMap, state);
    mGeomTOC->renderGeometry(&mBatcher);
    mBatcher.sort();
    mBatcher.render();

#if 0
    if (mat.dynamicLightingMaterial && false)
    {
        // render dynamic lighting...
        for (U32 i = 0; i < renderedStubs.size(); i++)
        {
            AtlasInstanceGeomTOC::RenderedStub& renderedstub = renderedStubs[i];
            LightInfoList lights;

            // convert bounds to world space...
            Box3F box;
            MathUtils::transformBoundingBox(renderedstub.stub->mBounds, sgData.objTrans, box);

            getCurrentClientSceneGraph()->getLightManager()->sgSetupLights(this, box,
                LightManager::sgAtlasMaxDynamicLights);
            getCurrentClientSceneGraph()->getLightManager()->sgGetBestLights(lights);

            GFX->setTexture(0, renderedstub.texture);

            for (U32 l = 0; l < lights.size(); l++)
            {
                LightInfo* light = lights[l];
                if ((light->mType != LightInfo::Point) && (light->mType != LightInfo::Spot))
                    continue;

                // get the model...
                sgLightingModel& lightingmodel = sgLightingModelManager::sgGetLightingModel(light->sgLightingModelName);
                lightingmodel.sgSetState(light);

                // get the info...
                F32 maxrad = lightingmodel.sgGetMaxRadius(true);
                light->sgTempModelInfo[0] = 0.5f / maxrad;

                // get the dynamic lighting texture...
                if ((light->mType == LightInfo::Spot) || (light->mType == LightInfo::SGStaticSpot))
                    sgData.dynamicLight = lightingmodel.sgGetDynamicLightingTextureSpot();
                else
                    sgData.dynamicLight = lightingmodel.sgGetDynamicLightingTextureOmni();

                // reset the model...
                lightingmodel.sgResetState();

                dMemcpy(&sgData.lights[0], light, sizeof(LightInfo));
                sgData.lightingTransform = light->sgLightingTransform;

                while (mat.dynamicLightingMaterial->setupPass(sgData))
                {
                    GFX->setAlphaBlendEnable(true);
                    GFX->setSrcBlend(GFXBlendSrcAlpha);
                    GFX->setDestBlend(GFXBlendOne);

                    GFX->setAlphaTestEnable(true);
                    GFX->setAlphaFunc(GFXCmpGreater);
                    GFX->setAlphaRef(0);

                    renderedstub.stub->mChunk->render();
                }
            }

            getCurrentClientSceneGraph()->getLightManager()->sgResetLights();
        }

        GFX->setAlphaTestEnable(false);
        GFX->setAlphaBlendEnable(false);
    }

#endif

    if (smRenderWireframe)
        GFX->setFillMode(GFXFillSolid);

    GFX->popWorldMatrix();
    GFX->popState();

    PROFILE_END();
}

U32 AtlasInstance2::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(conn, mask, stream);

    stream->writeString(mDetailTexFileName);
    stream->writeString(mAtlasFileName);

    stream->write(lightMapSize);

    mathWrite(*stream, getTransform());
    mathWrite(*stream, getScale());

    return retMask;
}

void AtlasInstance2::unpackUpdate(NetConnection* conn, BitStream* stream)
{
    Parent::unpackUpdate(conn, stream);

    mDetailTexFileName = stream->readSTString();
    mAtlasFileName = stream->readSTString();

    stream->read(&lightMapSize);

    MatrixF tmp;
    mathRead(*stream, &tmp);

    Point3F scale;
    mathRead(*stream, &scale);

    setTransform(tmp);
    setRenderTransform(tmp);

    setScale(scale);

    resetWorldBox();

    initLightMap();
}

bool AtlasInstance2::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    // Check to see if we're doing object box debug collision.
    if (smRayCollisionDebugLevel == RayCollisionDebugToObjectBox)
    {
        F32 t;
        Point3F n;
        bool ret = false;

        if (ret = mObjBox.collideLine(start, end, &t, &n))
        {
            info->object = this;
            info->t = t;
            info->normal = n;
        }

        return ret;
    }

    // As we get our castray coords already in object space, we don't have to
    // do any transformation, just pass it on to the geometry TOC.
    bool ret = mGeomTOC->castRay(start, end, info, false);

    if (ret)
        info->object = this;

    return ret;
}

void AtlasInstance2::buildConvex(const Box3F& box, Convex* convex)
{

    // Get the box into local space and pass it off to the TOC.
    Box3F localBox = box;
    mWorldToObj.mul(localBox);

    mGeomTOC->buildCollisionInfo(localBox, convex, NULL, this);
}

bool AtlasInstance2::buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere)
{
    // Get the box into local space and pass it off to the TOC.
    Box3F localBox = box;
    mWorldToObj.mul(localBox);

    polyList->setObject(this);
    polyList->setTransform(&getTransform(), getScale());

    // Since the polylist transforms for us we don't have to deal with
    // the transform stuff that convexes do.
    return mGeomTOC->buildCollisionInfo(localBox, NULL, polyList, this);
}

void AtlasInstance2::refillClipMap()
{
    mClipMap->fillWithTextureData();
}


ConsoleMethod(AtlasInstance2, purgeClipmap, void, 2, 2, "() - Refill clipmap from image cache.")
{
    U32 time = Platform::getRealMilliseconds();
    object->refillClipMap();
    U32 postTime = Platform::getRealMilliseconds();
    Con::printf("AtlasInstance2::purgeClipmap - took %d ms to refill clipmap.", postTime - time);

}

ConsoleMethod(AtlasInstance2, purgeClipmapTimed, void, 2, 2, "() - Refill clipmap from image cache, time test.")
{
    U32 time = Platform::getRealMilliseconds();
    for (S32 i = 0; i < 4; i++)
        object->refillClipMap();
    U32 postTime = Platform::getRealMilliseconds();
    Con::printf("AtlasInstance2::purgeClipmap - took %d avg ms to refill clipmap (4 trials).", (postTime - time) / 4);

}

#ifdef TORQUE_DEBUG

ConsoleFunction(atlasEmitCastRayTest, void, 2, 2, "(pos) - draw a bunch of debug raycasts from the specified point.")
{
    Point3F pos;

    dSscanf(argv[1], "%f %f %f", &pos.x, &pos.y, &pos.z);

    for (S32 i = -2; i < 3; i++)
    {
        for (S32 j = -2; j < 3; j++)
        {
            const F32 stepFactor = 10;
            const F32 probeDepth = -10;

            Point3F start = pos;
            Point3F end = start + Point3F(F32(i) * stepFactor, F32(j) * stepFactor, probeDepth);

            RayInfo r;
            bool ret;

            // Do Client...
            ret = getCurrentClientContainer()->castRay(start, end, AtlasObjectType, &r);
            if (ret)
                gDebugDraw->drawLine(start, r.point, ColorF(0, 1, 1));
            else
                gDebugDraw->drawLine(start, end, ColorF(1, 0, 1));

            // Do server...
            ret = getCurrentServerContainer()->castRay(start, end, AtlasObjectType, &r);
            if (ret)
                gDebugDraw->drawLine(start, r.point, ColorF(0, 1, 0));
            else
                gDebugDraw->drawLine(start, end, ColorF(1, 0, 0));

            gDebugDraw->setLastTTL(5000);
        }
    }
}

ConsoleFunction(testPNGCompression, void, 1, 1, "")
{
    GBitmap* gb = GBitmap::load("terrain_water_demo/alpha");

    // Seperate pixels, apply delta encoding and serialized.
    FileStream fs;
    fs.open("terrain_water_demo/alphaOut.raw", FileStream::Write);

    for (S32 channel = 0; channel < 4; channel++)
    {
        S32 pixelCount = gb->getWidth() * gb->getHeight();
        const U8* bits = gb->getBits();

        U8 lastPix = 0;
        for (S32 i = 0; i < pixelCount; i++)
        {
            // Encode...
            const U8 pixel = bits[i * gb->bytesPerPixel + channel];
            U8 encPixel = pixel; // - lastPix;
            lastPix = pixel;

            // Serialize...
            fs.write(encPixel);
        }
    }

    fs.close();
}

#endif
