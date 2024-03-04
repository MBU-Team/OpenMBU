//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "editor/editTSCtrl.h"
#include "editor/worldEditor.h"
#include "game/shadow.h"
#include "game/vehicles/wheeledVehicle.h"
#include "game/gameConnection.h"
#include "sceneGraph/sceneGraph.h"
#ifdef TORQUE_TERRAIN
#include "terrain/terrRender.h"
#endif
#include "game/shapeBase.h"
#include "gui/core/guiCanvas.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "game/staticShape.h"
#include "game/tsStatic.h"
#include "collision/concretePolyList.h"
#include "lightingSystem/sgSceneLighting.h"


namespace
{
    static void findObjectsCallback(SceneObject* obj, void* val)
    {
        Vector<SceneObject*>* list = (Vector<SceneObject*>*)val;
        list->push_back(obj);
    }

    bool              gTerminateLighting = false;
    F32               gLightingProgress = 0.f;
    const char* gCompleteCallback = 0;
    U32               gConnectionMissionCRC = 0xffffffff;
}

SceneLighting* gLighting = 0;
F32 gParellelVectorThresh = 0.01;
F32 gPlaneNormThresh = 0.999;
F32 gPlaneDistThresh = 0.001;


bool SceneLighting::smUseVertexLighting = false;


void SceneLighting::sgNewEvent(U32 light, S32 object, U32 event)
{
    Sim::postEvent(this, new sgSceneLightingProcessEvent(light, object, event),
        Sim::getTargetTime() + 1);
}

//-----------------------------------------------
/*
* Called once per scenelighting - entry point for event system
*/
void SceneLighting::sgLightingStartEvent()
{
    Con::printf("");
    Con::printf("//-----------------------------------------------");
    Con::printf("Synapse Gaming Lighting Pack");
    Con::printf("");
    Con::printf("Starting scene lighting...");

    sgTimeTemp2 = Platform::getRealMilliseconds();

    // clear interior light maps
    for (ObjectProxy** proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
    {
        // is there an object?
        if (!(*proxyItr)->getObject())
        {
            AssertFatal(0, "SceneLighting:: missing sceneobject on light start");
            Con::errorf(ConsoleLogEntry::General, "   SceneLighting:: missing sceneobject on light start");
            continue;
        }

        InteriorInstance* interior = dynamic_cast<InteriorInstance*>((*proxyItr)->getObject());
        if (!interior)
            continue;

        for (U32 d = 0; d < interior->getResource()->getNumDetailLevels(); d++)
        {
            Interior* detail = interior->getResource()->getDetailLevel(d);
            gInteriorLMManager.clearLightmaps(detail->getLMHandle(), interior->getLMHandle());
        }
    }

    Canvas->paint();
    sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgTGEPassSetupEventType);
    //sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgSGPassSetupEventType);
}

/*
* Called once per scenelighting - exit from event system
*/
void SceneLighting::sgLightingCompleteEvent()
{
    // save out the lighting?
    if (Con::getBoolVariable("$pref::sceneLighting::cacheLighting", true))
    {
        if (!savePersistInfo(mFileName))
            Con::errorf(ConsoleLogEntry::General, "SceneLighting::light: unable to persist lighting!");
        else
            Con::printf("Successfully saved mission lighting file: '%s'", mFileName);
    }

    Con::printf("Scene lighting complete (%3.3f seconds)", (Platform::getRealMilliseconds() - sgTimeTemp2) / 1000.f);
    Con::printf("//-----------------------------------------------");
    Con::printf("");
    Canvas->paint();

    completed(true);
    deleteObject();
}

//-----------------------------------------------
/*
* Called once per scenelighting - used for prepping the
* event system for TGE style scenelighting
*/
void SceneLighting::sgTGEPassSetupEvent()
{
    Con::printf("  Starting TGE based scene lighting...");

    Canvas->paint();
    sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgTGELightStartEventType);
}

/*
* Called once per light - used for calling preLight on all objects
* Only TGE lights call prelight and continue on to the process event
*/
void SceneLighting::sgTGELightStartEvent(U32 light)
{
    // catch bad light index and jump to complete event
    if (light >= mLights.size())
    {
        sgNewEvent(light, 0, sgSceneLightingProcessEvent::sgTGELightCompleteEventType);
        return;
    }

    // can we use the light?
    if (mLights[light]->mType != LightInfo::Vector)
    {
        sgNewEvent((light + 1), 0, sgSceneLightingProcessEvent::sgTGELightStartEventType);
        return;
    }

    // process pre-lighting
    Con::printf("    Lighting with light #%d (TGE vector light)...", (light + 1));
    LightInfo* lightobj = mLights[light];
    mLitObjects.clear();

    for (ObjectProxy** proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
    {
        // is there an object?
        if (!(*proxyItr)->getObject())
        {
            AssertFatal(0, "SceneLighting:: missing sceneobject on light start");
            Con::errorf(ConsoleLogEntry::General, "      SceneLighting:: missing sceneobject on light start");
            continue;
        }

#ifdef TORQUE_TERRAIN
        // don't do this to interiors, need to collect the shadow volume info...
        if (dynamic_cast<AtlasInstance2*>((*proxyItr)->getObject()))
            continue;

        // if prelight works add to the list
        if (dynamic_cast<TerrainProxy*>(*proxyItr))
        {
            if ((*proxyItr)->preLight(lightobj))
                mLitObjects.push_back(*proxyItr);
        }
        else
#endif
            if (dynamic_cast<InteriorProxy*>(*proxyItr))
        {
            // need this for interior shadows on the terrain...
            // but these cannot preLight...
            mLitObjects.push_back(*proxyItr);
        }
    }

    // kick off lighting
    Canvas->paint();
    sgNewEvent(light, 0, sgSceneLightingProcessEvent::sgTGELightProcessEventType);
}

/*
* Called once for each TGE light and object - used for calling light on an object
*/
void SceneLighting::sgTGELightProcessEvent(U32 light, S32 object)
{
    // catch bad light or object index
    if ((light >= mLights.size()) || (object >= mLitObjects.size()))
    {
        sgNewEvent(light, 0, sgSceneLightingProcessEvent::sgTGELightCompleteEventType);
        return;
    }

    // light object
    InteriorInstance* interior = dynamic_cast<InteriorInstance*>(mLitObjects[object]->getObject());
    if ((interior) && (interior->mInteriorFileName))
        Con::printf("      Lighting interior object %d of %d (%s)...", (object + 1), mLitObjects.size(), interior->mInteriorFileName);
    else
        Con::printf("      Lighting object %d of %d...", (object + 1), mLitObjects.size());
    Canvas->paint();

    //process object and light
    S32 time = Platform::getRealMilliseconds();
#ifdef TORQUE_TERRAIN
    if (dynamic_cast<TerrainProxy*>(mLitObjects[object]))
        mLitObjects[object]->light(mLights[light]);
#endif

    sgTGESetProgress(light, object);
    Con::printf("      Object lighting complete (%3.3f seconds)", (Platform::getRealMilliseconds() - time) / 1000.f);

    // kick off next object event
    Canvas->paint();
    sgNewEvent(light, (object + 1), sgSceneLightingProcessEvent::sgTGELightProcessEventType);
}

/*
* Called once per TGE light - used for calling postLight on all objects
*/
void SceneLighting::sgTGELightCompleteEvent(U32 light)
{
    // catch bad light index and move to the next pass event
    if (light >= mLights.size())
    {
        sgTGESetProgress(mLights.size(), mLitObjects.size());
        Con::printf("  TGE based scene lighting complete (%3.3f seconds)", (Platform::getRealMilliseconds() - sgTimeTemp2) / 1000.f);
        Canvas->paint();
        sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgSGPassSetupEventType);
        //sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgLightingCompleteEventType);
        return;
    }

    // process post-lighting
    // don't do this, SG lighting events will copy terrain light map...
    /*bool islast = (light == (mLights.size() - 1));
    for(U32 o=0; o<mLitObjects.size(); o++)
    {
        if(dynamic_cast<TerrainProxy *>(mLitObjects[o]))
            mLitObjects[o]->postLight(islast);
    }*/

    // kick off next light event
    Canvas->paint();
    sgNewEvent((light + 1), 0, sgSceneLightingProcessEvent::sgTGELightStartEventType);
}

void SceneLighting::sgTGESetProgress(U32 light, S32 object)
{
    // TGE is light based...
    F32 val = (light * mLitObjects.size()) + object;
    F32 total = mLights.size() * mLitObjects.size();

    if (total == 0.0f)
        return;

    val = getMin(val, total);

    // two passes...
    total *= 2.0f;

    gLightingProgress = val / total;
}

//-----------------------------------------------
/*
* Called once per scenelighting - used for prepping the
* event system for SG style scenelighting
*/
void SceneLighting::sgSGPassSetupEvent()
{
    Con::printf("  Starting Synapse Gaming Lighting Pack scene lighting...");

    mLitObjects.clear();
    for (ObjectProxy** proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
    {
        // is there an object?
        if (!(*proxyItr)->getObject())
        {
            AssertFatal(0, "SceneLighting:: missing sceneobject on light start");
            Con::errorf(ConsoleLogEntry::General, "   SceneLighting:: missing sceneobject on light start");
            continue;
        }

        // add all lights
        mLitObjects.push_back(*proxyItr);
    }


    // stats...
    sgStatistics::sgClear();
    sgStatistics::sgInteriorObjectCount += mLitObjects.size();


    Canvas->paint();
    sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgSGObjectStartEventType);
}

/*
* Called once per object - used for calling preLight on all SG lights
*/
void SceneLighting::sgSGObjectStartEvent(S32 object)
{
    // catch bad light index and jump to complete event
    if (object >= mLitObjects.size())
    {
        sgNewEvent(0, object, sgSceneLightingProcessEvent::sgSGObjectCompleteEventType);
        return;
    }

    // process pre-lighting
    InteriorInstance* interior = dynamic_cast<InteriorInstance*>(mLitObjects[object]->getObject());
    if ((interior) && (interior->mInteriorFileName))
        Con::printf("    Lighting interior object %d of %d (%s)...", (object + 1), mLitObjects.size(), interior->mInteriorFileName);
    else
    {
#ifdef TORQUE_TERRAIN
        AtlasInstance2* atlas = dynamic_cast<AtlasInstance2*>(mLitObjects[object]->getObject());
        if (atlas)
        {
            Con::printf("    Lighting Atlas chunk object %d of %d...", (object + 1), mLitObjects.size());
            // stop rendering when Atlas starts lighting...
            GFX->setAllowRender(false);
        }
        else
#endif
            Con::printf("    Lighting object %d of %d...", (object + 1), mLitObjects.size());
    }

    ObjectProxy* obj = mLitObjects[object];
    for (U32 i = 0; i < mLights.size(); i++)
    {
        // can we use the light?
        LightInfo* lightobj = mLights[i];
        //if((lightobj->mType == LightInfo::SGStaticPoint) || (lightobj->mType == LightInfo::SGStaticSpot))
        obj->preLight(lightobj);
    }

    sgTimeTemp = Platform::getRealMilliseconds();

    // kick off lighting
    Canvas->paint();

    // this is slow with multiple objects...
    //sgNewEvent(0, object, sgSceneLightingProcessEvent::sgSGObjectProcessEventType);
    // jump right to the method...
    sgSGObjectProcessEvent(0, object);
}

/*
* Called once per object and SG light - used for calling light on an object
*/
void SceneLighting::sgSGObjectProcessEvent(U32 light, S32 object)
{
    // catch bad light or object index
    if ((light >= mLights.size()) || (object >= mLitObjects.size()))
    {
        // this is slow with multiple objects...
        //sgNewEvent(0, object, sgSceneLightingProcessEvent::sgSGObjectCompleteEventType);
        // jump right to the method...
        sgSGObjectCompleteEvent(object);
        return;
    }

    // avoid the event overhead...
    // 80 lights == 0.6 seconds an interior without ANY lighting (events only)...
    U32 time = Platform::getRealMilliseconds();
    while ((light < mLights.size()) && ((Platform::getRealMilliseconds() - time) < 500))
    {
        // can we use the light?
        LightInfo* lightobj = mLights[light];
#ifdef TORQUE_TERRAIN
        if (!((lightobj->mType == LightInfo::Vector) && (dynamic_cast<TerrainProxy*>(mLitObjects[object]))))
        {
            // light part of the object
            mLitObjects[object]->light(lightobj);
        }
#endif

        sgSGSetProgress(light, object);

        light++;
    }

    light--;

    // kick off next light event
    Canvas->paint();
    sgNewEvent((light + 1), object, sgSceneLightingProcessEvent::sgSGObjectProcessEventType);
}

/*
* Called once per object - used for calling postLight on all SG lights
*/
void SceneLighting::sgSGObjectCompleteEvent(S32 object)
{
    // catch bad light index and move to the next pass event
    if (object >= mLitObjects.size())
    {
        sgSGSetProgress(mLights.size(), mLitObjects.size());
        Con::printf("  Synapse Gaming Lighting Pack scene lighting complete (%3.3f seconds)", (Platform::getRealMilliseconds() - sgTimeTemp2) / 1000.f);

        // stats...
        sgStatistics::sgPrint();

        Canvas->paint();
        sgNewEvent(0, 0, sgSceneLightingProcessEvent::sgLightingCompleteEventType);
        return;
    }

    // process post-lighting
    Con::printf("    Object lighting complete (%3.3f seconds)", (Platform::getRealMilliseconds() - sgTimeTemp) / 1000.f);

    // in case Atlas turned off rendering...
    GFX->setAllowRender(true);

    // only the last light does something
    mLitObjects[object]->postLight(true);
    /*
        InteriorInstance *interiorinst = dynamic_cast<InteriorInstance *>(mLitObjects[object]->getObject());
        if(interiorinst)
        {
            Interior *detail = interiorinst->getDetailLevel(0);
            for(U32 i=0; i<detail->mNormalLMapIndices.size(); i++)
            {
                GFXTexHandle normHandle = gInteriorLMManager.duplicateBaseLightmap(detail->getLMHandle(),
                    interiorinst->getLMHandle(), detail->getNormalLMapIndex(i));
                GBitmap *normLightmap = normHandle->getBitmap();

                FileStream output;
                output.open(avar("lightmaps/lm_%d_%d.png", object, i), FileStream::Write);
                normLightmap->writePNG(output);
            }
        }
    */
    /*ObjectProxy *obj = mLitObjects[object];
    for(U32 i=0; i<mLights.size(); i++)
    {
    // can we use the light?
    LightInfo *lightobj = mLights[i];
    if((lightobj->mType == LightInfo::SGStaticPoint) || (lightobj->mType == LightInfo::SGStaticSpot))
    obj->postLight((i == (mLights.size() - 1)));
    }*/

    // kick off next light event
    Canvas->paint();

    // this is slow with multiple objects...
    //sgNewEvent(0, (object+1), sgSceneLightingProcessEvent::sgSGObjectStartEventType);
    // jump right to the method...
    sgSGObjectStartEvent((object + 1));
}

void SceneLighting::sgSGSetProgress(U32 light, S32 object)
{
    // SG is object based...
    F32 val = (object * mLights.size()) + light;
    F32 total = mLights.size() * mLitObjects.size();

    if (total == 0.0f)
        return;

    val = getMin(val, total);

    // two passes...
    total *= 2.0f;

    gLightingProgress = (val / total) + 0.5;
}

//-----------------------------------------------

void SceneLighting::processEvent(U32 light, S32 object)
{
    sgNewEvent(light, object, sgSceneLightingProcessEvent::sgLightingStartEventType);
}


//-----------------------------------------------


SceneLighting::SceneLighting()
{
    mStartTime = 0;
    mFileName[0] = 0;
    smUseVertexLighting = Interior::smUseVertexLighting;

    static bool initialized = false;
    if (!initialized)
    {
        Con::addVariable("SceneLighting::terminateLighting", TypeBool, &gTerminateLighting);
        Con::addVariable("SceneLighting::lightingProgress", TypeF32, &gLightingProgress);
        initialized = true;
    }
}

SceneLighting::~SceneLighting()
{
    gLighting = 0;
    gLightingProgress = 0.f;

    ObjectProxy** proxyItr;
    for (proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
        delete* proxyItr;
}

bool SceneLighting::light(BitSet32 flags)
{
    if (!getCurrentClientSceneGraph())
        return(false);

    mStartTime = Platform::getRealMilliseconds();

    // register static lights
    LightManager* lManager = getCurrentClientSceneGraph()->getLightManager();
    lManager->sgRegisterGlobalLights(true);

    // grab all the lights
    mLights.clear();
    lManager->sgGetAllUnsortedLights(mLights);


    if (!mLights.size())
        return(false);

    // get all the objects and create proxy's for them
    Vector<SceneObject*>   objects;
    //gClientContainer.findObjects(InteriorObjectType | TerrainObjectType | AtlasObjectType, sgFindObjectsCallback, &objects);
    getCurrentClientContainer()->findObjects(InteriorObjectType | TerrainObjectType, sgFindObjectsCallback, &objects);

    for (SceneObject** itr = objects.begin(); itr != objects.end(); itr++)
    {
        ObjectProxy* proxy;
        if (isInterior(*itr))
            proxy = new InteriorProxy(*itr);
#ifdef TORQUE_TERRAIN
        else if (isTerrain(*itr))
            proxy = new TerrainProxy(*itr);
        else if (isAtlas(*itr))
        {
            AtlasInstance2* atlas = dynamic_cast<AtlasInstance2*>(*itr);
            // don't error out this could be atlas1...
            if (!atlas)
                continue;

            // Grab deepest resource stubs.
            Vector<AtlasResourceGeomStub*> stubs;
            atlas->findDeepestStubs(stubs);
            for (U32 s = 0; s < stubs.size(); s++)
            {
                proxy = new AtlasChunkProxy(atlas, stubs[s]);
                proxy->calcValidation();
                proxy->loadResources();
                mSceneObjects.push_back(proxy);
            }
            proxy = new AtlasLightMapProxy(atlas);
            proxy->calcValidation();
            proxy->loadResources();
            mSceneObjects.push_back(proxy);
            // already calc-ed and added...
            continue;
        }
#endif
        else
        {
            //AssertFatal(0, "SceneLighting:: invalid object returned from container search");
            continue;
        }

        if (!proxy->calcValidation())
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to calculate validation info for object.  Skipped.");
            delete proxy;
            continue;
        }

        if (!proxy->loadResources())
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to load resources for object.  Skipped.");
            delete proxy;
            continue;
        }

        mSceneObjects.push_back(proxy);
    }

    if (!mSceneObjects.size())
        return(false);

    // grab the missions crc
    U32 missionCRC = calcMissionCRC();

    // remove the '.mis' extension from the mission name
    char misName[256];
    dSprintf(misName, sizeof(misName), "%s", Con::getVariable("$Client::MissionFile"));
    char* dot = dStrstr((const char*)misName, ".mis");
    if (dot)
        *dot = '\0';

    // get the mission name
    if (LightManager::sgAllowFullLightMaps())
        dSprintf(mFileName, sizeof(mFileName), "%s_%x.ml", misName, missionCRC);
    else
        dSprintf(mFileName, sizeof(mFileName), "%s_%x-raw.ml", misName, missionCRC);
    if (!ResourceManager->isValidWriteFileName(mFileName))
    {
        Con::warnf("Invalid filename '%s'.  Failed to light mission.", mFileName);
        return(false);
    }

    // check for some persisted data, check if being forced..
    if (!flags.test(ForceAlways | ForceWritable))
    {
        if (loadPersistInfo(mFileName))
        {
            Con::printf(" Successfully loaded mission lighting file: '%s'", mFileName);

            // touch this file...
            if (!dFileTouch(mFileName))
                Con::warnf("  Failed to touch file '%s'.  File may be read only.", mFileName);

            return(false);
        }

        // texture manager must have lighting complete now
        if (flags.test(LoadOnly))
        {
            Con::errorf(ConsoleLogEntry::General, "Failed to load mission lighting!");
            return(false);
        }
    }

    // don't light if file is read-only?
    if (!flags.test(ForceAlways))
    {
        Stream* fileStream;
        if (!ResourceManager->openFileForWrite(fileStream, mFileName))
        {
            Con::errorf(ConsoleLogEntry::General, "SceneLighting::Light: Failed to light mission.  File '%s' cannot be written to.", mFileName);
            return(false);
        }
        delete fileStream;
    }

    // initialize the objects for lighting
    for (ObjectProxy** proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
        (*proxyItr)->init();

    // get things started
    Sim::postEvent(this, new sgSceneLightingProcessEvent(0, -1,
        sgSceneLightingProcessEvent::sgLightingStartEventType), Sim::getTargetTime() + 1);
    return(true);
}

void SceneLighting::completed(bool success)
{
    // process the cached lighting files
    processCache();

    if (success)
    {
        AssertFatal(smUseVertexLighting == Interior::smUseVertexLighting, "SceneLighting::completed: vertex lighting state changed during scene light");

        // cannot do anything if vertex state has changed (since we only load in what is needed)
        if (smUseVertexLighting == Interior::smUseVertexLighting)
        {
            if (!smUseVertexLighting)
            {
                gInteriorLMManager.downloadGLTextures();
                gInteriorLMManager.destroyBitmaps();
            }
            else
                gInteriorLMManager.destroyTextures();
        }
    }

    if (gCompleteCallback && gCompleteCallback[0])
        Con::executef(1, gCompleteCallback);
}

//------------------------------------------------------------------------------
// Static access method: there can be only one SceneLighting object
bool SceneLighting::lightScene(const char* callback, BitSet32 flags)
{
    if (gLighting)
    {
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: forcing restart of lighting!");
        gLighting->deleteObject();
        gLighting = 0;
    }

    SceneLighting* lighting = new SceneLighting;

    // register the object
    if (!lighting->registerObject())
    {
        AssertFatal(0, "SceneLighting:: Unable to register SceneLighting object!");
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: Unable to register SceneLighting object!");
        delete lighting;
        return(false);
    }

    // could have interior resources but no instances (hey, got this far didnt we...)
    GameConnection* con = dynamic_cast<GameConnection*>(NetConnection::getConnectionToServer());
    if (!con)
    {
        Con::errorf(ConsoleLogEntry::General, "SceneLighting:: no GameConnection");
        return(false);
    }
    con->addObject(lighting);

    // set the globals
    gLighting = lighting;
    gTerminateLighting = false;
    gLightingProgress = 0.f;
    gCompleteCallback = callback;
    gConnectionMissionCRC = con->getMissionCRC();


    if (!lighting->light(flags))
    {
        lighting->completed(true);
        lighting->deleteObject();
        return(false);
    }
    return(true);
}

bool SceneLighting::isLighting()
{
    return(bool(gLighting));
}

bool SceneLighting::sgIsCorrectStaticObjectType(SceneObject* obj)
{
    if ((dynamic_cast<TSStatic*>(obj)) != NULL)
        return true;
    if ((dynamic_cast<StaticShape*>(obj)) != NULL)
        return true;
    return false;
}

/// adds TSStatic objects as shadow casters.
void SceneLighting::addStatic(void* terrainproxy, ShadowVolumeBSP* shadowVolume,
    SceneObject* sceneobject, LightInfo* light, S32 level)
{
    SceneLighting::TerrainProxy* terrain = (SceneLighting::TerrainProxy*)terrainproxy;
    if ((!terrain) || (!sceneobject))
        return;

    if (light->mType != LightInfo::Vector)
        return;

    ConcretePolyList polylist;
    const Box3F box;
    const SphereF sphere;
    sceneobject->buildPolyList(&polylist, box, sphere);

    // retrieve the poly list (uses the collision mesh)...
    //sobj->sgAdvancedStaticOptionsData.sgBuildPolyList(sobj, &polylist);

    S32 i, count, vertind[3];
    ConcretePolyList::Poly* poly;

    count = polylist.mPolyList.size();

    // add the polys to the shadow volume...
    for (i = 0; i < count; i++)
    {
        poly = (ConcretePolyList::Poly*)&polylist.mPolyList[i];
        AssertFatal((poly->vertexCount == 3), "Hmmm... vert count is greater than 3.");

        vertind[0] = polylist.mIndexList[poly->vertexStart];
        vertind[1] = polylist.mIndexList[poly->vertexStart + 1];
        vertind[2] = polylist.mIndexList[poly->vertexStart + 2];

        if (mDot(PlaneF(polylist.mVertexList[vertind[0]], polylist.mVertexList[vertind[1]],
            polylist.mVertexList[vertind[2]]), light->mDirection) < gParellelVectorThresh)
        {
            ShadowVolumeBSP::SVPoly* svpoly = shadowVolume->createPoly();
            svpoly->mWindingCount = 3;

            svpoly->mWinding[0].set(polylist.mVertexList[vertind[0]]);
            svpoly->mWinding[1].set(polylist.mVertexList[vertind[1]]);
            svpoly->mWinding[2].set(polylist.mVertexList[vertind[2]]);
            svpoly->mPlane = PlaneF(svpoly->mWinding[0], svpoly->mWinding[1], svpoly->mWinding[2]);
            svpoly->mPlane.neg();

            shadowVolume->buildPolyVolume(svpoly, light);
            shadowVolume->insertPoly(svpoly);
        }
    }
}

//------------------------------------------------------------------------------
bool SceneLighting::verifyMissionInfo(PersistInfo::PersistChunk* chunk)
{
    PersistInfo::MissionChunk* info = dynamic_cast<PersistInfo::MissionChunk*>(chunk);
    if (!info)
        return(false);

    PersistInfo::MissionChunk curInfo;
    if (!getMissionInfo(&curInfo))
        return(false);

    return(curInfo.mChunkCRC == info->mChunkCRC);
}

bool SceneLighting::getMissionInfo(PersistInfo::PersistChunk* chunk)
{
    PersistInfo::MissionChunk* info = dynamic_cast<PersistInfo::MissionChunk*>(chunk);
    if (!info)
        return(false);

    info->mChunkCRC = gConnectionMissionCRC ^ PersistInfo::smFileVersion;
    return(true);
}

//------------------------------------------------------------------------------
bool SceneLighting::loadPersistInfo(const char* fileName)
{
    // open the file
    Stream* stream = 0;
    stream = ResourceManager->openStream(fileName);
    if (!stream)
        return(false);

    PersistInfo persistInfo;
    bool success = persistInfo.read(*stream);
    ResourceManager->closeStream(stream);
    if (!success)
        return(false);

    // verify the mission chunk
    if (!verifyMissionInfo(persistInfo.mChunks[0]))
        return(false);

#ifdef TORQUE_TERRAIN
    // remove atlas proxy info, these do not persist here...
    for (U32 i = 0; i < mSceneObjects.size(); i++)
    {
        AtlasChunkProxy* atlas = dynamic_cast<AtlasChunkProxy*>(mSceneObjects[i]);
        if (atlas)
        {
            mSceneObjects.erase(i);
            i--;
        }
    }
#endif

    if (mSceneObjects.size() != (persistInfo.mChunks.size() - 1))
        return(false);

    Vector<PersistInfo::PersistChunk*> chunks;

    // ensure that the scene objects are in the same order as the chunks
    //  - different instances will depend on this
    U32 i;
    for (i = 0; i < mSceneObjects.size(); i++)
    {
        // 0th chunk is the mission chunk
        U32 chunkIdx = i + 1;
        if (chunkIdx >= persistInfo.mChunks.size())
            return(false);

        if (!mSceneObjects[i]->isValidChunk(persistInfo.mChunks[chunkIdx]))
            return(false);
        chunks.push_back(persistInfo.mChunks[chunkIdx]);
    }

    // get the objects to load in the persisted chunks
    for (i = 0; i < mSceneObjects.size(); i++)
        if (!mSceneObjects[i]->setPersistInfo(chunks[i]))
            return(false);

    return(true);
}

bool SceneLighting::savePersistInfo(const char* fileName)
{
    // open the file
    Stream* file;
    if (!ResourceManager->openFileForWrite(file, fileName))
        return(false);

    PersistInfo persistInfo;

    // add in the mission chunk
    persistInfo.mChunks.push_back(new PersistInfo::MissionChunk);

    // get the mission info, will return false when there are 0 lights
    if (!getMissionInfo(persistInfo.mChunks[0]))
        return(false);

    // get all the persist chunks
    for (U32 i = 0; i < mSceneObjects.size(); i++)
    {
        if (isInterior(mSceneObjects[i]->mObj))
            persistInfo.mChunks.push_back(new PersistInfo::InteriorChunk);
#ifdef TORQUE_TERRAIN
        else if (isTerrain(mSceneObjects[i]->mObj))
            persistInfo.mChunks.push_back(new PersistInfo::TerrainChunk);
        else if (dynamic_cast<AtlasLightMapProxy*>(mSceneObjects[i]))
            persistInfo.mChunks.push_back(new PersistInfo::AtlasLightMapChunk);
        else if (isAtlas(mSceneObjects[i]->mObj))
            continue;
#endif
        else
            return(false);

        if (!mSceneObjects[i]->getPersistInfo(persistInfo.mChunks.last()))
            return(false);
    }

    if (!persistInfo.write(*file))
        return(false);

    delete file;

    // open/close the stream to get the fileSize calculated on the resource object
    ResourceManager->closeStream(ResourceManager->openStream(fileName));
    return(true);
}

struct CacheEntry {
    ResourceObject* mFileObject;
    const char* mFileName;

    CacheEntry() {
        mFileObject = 0;
        mFileName = 0;
    };
};

// object list sort methods: want list in reverse
static int QSORT_CALLBACK minSizeSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    return(entry2->mFileObject->fileSize - entry1->mFileObject->fileSize);
}

static int QSORT_CALLBACK maxSizeSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    return(entry1->mFileObject->fileSize - entry2->mFileObject->fileSize);
}

static int QSORT_CALLBACK lastCreatedSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    FileTime create[2];
    FileTime modify;

    bool ret[2];

    ret[0] = Platform::getFileTimes(entry1->mFileName, &create[0], &modify);
    ret[1] = Platform::getFileTimes(entry2->mFileName, &create[1], &modify);

    // check return values
    if (!ret[0] && !ret[1])
        return(0);
    if (!ret[0])
        return(1);
    if (!ret[1])
        return(-1);

    return(Platform::compareFileTimes(create[1], create[0]));
}

static int QSORT_CALLBACK lastModifiedSort(const void* p1, const void* p2)
{
    const CacheEntry* entry1 = (const CacheEntry*)p1;
    const CacheEntry* entry2 = (const CacheEntry*)p2;

    FileTime create;
    FileTime modify[2];

    bool ret[2];

    ret[0] = Platform::getFileTimes(entry1->mFileName, &create, &modify[0]);
    ret[1] = Platform::getFileTimes(entry2->mFileName, &create, &modify[1]);

    // check return values
    if (!ret[0] && !ret[1])
        return(0);
    if (!ret[0])
        return(1);
    if (!ret[1])
        return(-1);

    return(Platform::compareFileTimes(modify[1], modify[0]));
}

void SceneLighting::processCache()
{
    // get size in kb
    S32 quota = Con::getIntVariable("$pref::sceneLighting::cacheSize", -1);

    Vector<CacheEntry> files;

    ResourceObject* match = 0;
    const char* name;

    S32 curCacheSize = 0;
    match = ResourceManager->findMatch("*.ml", &name, 0);
    while (match)
    {
        if (match->flags & ResourceObject::File)
        {
            // dont allow the current file to be removed...
            if (!dStrstr(name, mFileName))
            {
                CacheEntry entry;
                entry.mFileObject = match;

                // get out of vfs...
                char fileName[1024];
                dSprintf(fileName, sizeof(fileName), "%s/%s", match->path, match->name);

                entry.mFileName = StringTable->insert(fileName);
                files.push_back(entry);
            }
            else
                curCacheSize += match->fileSize;
        }

        match = ResourceManager->findMatch("*.ml", &name, match);
    }

    // remove old files
    for (S32 i = files.size() - 1; i >= 0; i--)
    {
        char buf[1024];
        dSprintf(buf, sizeof(buf), "%s/%s", files[i].mFileObject->path, files[i].mFileObject->name);

        Stream* stream = ResourceManager->openStream(buf);
        if (!stream)
            continue;

        // read in the version
        U32 version;
        bool ok = (stream->read(&version) && (version == PersistInfo::smFileVersion));
        ResourceManager->closeStream(stream);

        // ok?
        if (ok)
            continue;

        // delete the file
        ResourceManager->freeResource(files[i].mFileObject);

        // no sneaky names
        if (!dStrstr(files[i].mFileName, ".."))
        {
            Con::warnf("Removing old lighting file '%s'.", files[i].mFileName);
            dFileDelete(files[i].mFileName);
        }

        files.pop_back();
    }

    // no size restriction?
    if (quota == -1 || !files.size())
        return;

    for (U32 i = 0; i < files.size(); i++)
        curCacheSize += files[i].mFileObject->fileSize;

    // need to remove?
    if (quota > (curCacheSize >> 10))
        return;

    // sort the entries by the correct method
    const char* purgeMethod = Con::getVariable("$pref::sceneLighting::purgeMethod");
    if (!purgeMethod)
        purgeMethod = "";

    // determine the method (default to least recently used)
    if (!dStricmp(purgeMethod, "minSize"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), minSizeSort);
    else if (!dStricmp(purgeMethod, "maxSize"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), maxSizeSort);
    else if (!dStricmp(purgeMethod, "lastCreated"))
        dQsort(files.address(), files.size(), sizeof(CacheEntry), lastCreatedSort);
    else
        dQsort(files.address(), files.size(), sizeof(CacheEntry), lastModifiedSort);

    // go through and remove the best candidate first (sorted reverse)
    while (((curCacheSize >> 10) > quota) && files.size())
    {
        curCacheSize -= files.last().mFileObject->fileSize;
        ResourceManager->freeResource(files.last().mFileObject);

        // no sneaky names
        if (!dStrstr(files.last().mFileName, ".."))
        {
            Con::warnf("Removing lighting file '%s'.", files.last().mFileName);
            dFileDelete(files.last().mFileName);
        }

        files.pop_back();
    }
}

static S32 QSORT_CALLBACK compareS32(const void* a, const void* b)
{
    return(*((S32*)a) - *((S32*)b));
}

U32 SceneLighting::calcMissionCRC()
{
    // all the objects + mission chunk
    Vector<U32> crc;

    // grab the object crcs
    for (U32 i = 0; i < mSceneObjects.size(); i++)
        crc.push_back(mSceneObjects[i]->mChunkCRC);

    // grab the missions crc
    PersistInfo::MissionChunk curInfo;
    getMissionInfo(&curInfo);
    crc.push_back(curInfo.mChunkCRC);

    // sort them (order may not have been preserved)
    dQsort(crc.address(), crc.size(), sizeof(U32), compareS32);

    return(calculateCRC(crc.address(), sizeof(U32) * crc.size(), 0xffffffff));
}

bool SceneLighting::ObjectProxy::calcValidation()
{
    mChunkCRC = getResourceCRC();
    if (!mChunkCRC)
        return(false);

    return(true);
}

bool SceneLighting::ObjectProxy::isValidChunk(PersistInfo::PersistChunk* chunk)
{
    return(chunk->mChunkCRC == mChunkCRC);
}

bool SceneLighting::ObjectProxy::getPersistInfo(PersistInfo::PersistChunk* chunk)
{
    chunk->mChunkCRC = mChunkCRC;
    return(true);
}

bool SceneLighting::ObjectProxy::setPersistInfo(PersistInfo::PersistChunk* chunk)
{
    mChunkCRC = chunk->mChunkCRC;
    return(true);
}

#ifdef TORQUE_TERRAIN
bool SceneLighting::AtlasLightMapProxy::setPersistInfo(PersistInfo::PersistChunk* info)
{
    if (!Parent::setPersistInfo(info))
        return(false);

    PersistInfo::AtlasLightMapChunk* chunk = dynamic_cast<PersistInfo::AtlasLightMapChunk*>(info);
    AssertFatal(chunk, "SceneLighting::AtlasLightMapChunk::setPersistInfo: invalid info chunk!");

    if (!sgAtlas)
        return false;

    GBitmap* bitmap = new GBitmap(*chunk->mLightmap);
    bitmap->sgAlreadyConverted = true;
    sgAtlas->lightMapTex = GFXTexHandle(bitmap, &GFXDefaultPersistentProfile, true);

    /*TerrainBlock *terrain = getObject();
    if(!terrain || !terrain->lightMap)
        return(false);

    if(terrain->lightMap) delete terrain->lightMap;

    terrain->lightMap = new GBitmap( *chunk->mLightmap);

    terrain->buildMaterialMap();*/
    return(true);
}

bool SceneLighting::AtlasLightMapProxy::getPersistInfo(PersistInfo::PersistChunk* info)
{
    if (!Parent::getPersistInfo(info))
        return(false);

    PersistInfo::AtlasLightMapChunk* chunk = dynamic_cast<PersistInfo::AtlasLightMapChunk*>(info);
    AssertFatal(chunk, "SceneLighting::AtlasLightMapChunk::getPersistInfo: invalid info chunk!");

    if (!sgAtlas)
        return false;

    GBitmap* bitmap = sgAtlas->lightMapTex.getBitmap();
    if (!bitmap)
        return false;
    chunk->mLightmap = new GBitmap(*bitmap);

    /*	TerrainBlock *terrain = getObject();
        if(!terrain || !terrain->lightMap)
            return(false);

        if(chunk->mLightmap) delete chunk->mLightmap;

        chunk->mLightmap = new GBitmap(*terrain->lightMap);*/

    return(true);
}
#endif
