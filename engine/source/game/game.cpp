//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/platformVideo.h"
#include "sfx/sfxSystem.h"
#include "platform/platformInput.h"
#include "core/findMatch.h"

#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#ifdef TORQUE_TERRAIN
#include "terrain/terrData.h"
#include "terrain/terrRender.h"
#include "terrain/waterBlock.h"
#endif
#include "game/collisionTest.h"
#include "game/showTSShape.h"
#include "sceneGraph/sceneGraph.h"
#include "gui/core/guiTSControl.h"
#include "game/moveManager.h"
#include "console/consoleTypes.h"
#include "game/shapeBase.h"
#include "core/dnet.h"
#include "game/gameConnection.h"
#include "game/gameProcess.h"
#include "core/fileStream.h"
#include "gui/core/guiCanvas.h"
#include "sceneGraph/sceneLighting.h"
#include "terrain/environment/sky.h"
#include "game/ambientAudioManager.h"
#include "core/frameAllocator.h"
#include "sceneGraph/detailManager.h"
#include "gui/controls/guiMLTextCtrl.h"
#include "platform/profiler.h"
#include "gfx/gfxCubemap.h"
#include "core/iTickable.h"
#include "core/tDictionary.h"
#include <game/missionMarker.h>

#include "core/tokenizer.h"
#include "interior/interiorResObjects.h"

static void cPanoramaScreenShot(SimObject*, S32, const char** argv);

void wireCube(F32 size, Point3F pos);

CollisionTest collisionTest;

F32 gMovementSpeed = 1;

bool gSPMode = false;
bool gRenderPreview = false;
SimSet* gSPModeSet;

//--------------------------------------------------------------------------
ConsoleFunctionGroupBegin(GameFunctions, "General game functionality.");

//--------------------------------------------------------------------------

#ifdef MB_ULTRA_PREVIEWS
ConsoleFunction(setSinglePlayerMode, void, 2, 2, "( flag ) - Enable or disable singleplayer only mode.")
{
    argc;

    for (NetConnection* walk = NetConnection::getConnectionList();
        walk; walk = walk->getNext())
    {
        if (!walk->isConnectionToServer())
            walk->resetGhosting();
    }

    bool oldSPMode = gSPMode;
    gSPMode = dAtob(argv[1]);
    if (gSPMode && !oldSPMode)
        gSPModeProcessList.setDirty(gServerProcessList.isDirty());
    else if (!gSPMode && oldSPMode)
        gServerProcessList.setDirty(gSPModeProcessList.isDirty());
}

ConsoleFunction(isSinglePlayerMode, bool, 1, 1, "() - Checks if singleplayer only mode is enabled.")
{
    argc;

    return gSPMode;
}

ConsoleFunction(loadObjectsFromMission, const char*, 2, 5, "(mission, className, dataBlock, missionGroup) - Loads Objects from a Mission")
{
    const char* missionFile = argv[1];

    char buf1[1024];
    buf1[0] = 0;
    dStrcat(buf1, missionFile);
    if (!dStrrchr(missionFile, '.'))
        dStrcat(buf1, ".mis");

    char srcPath[1024];
    srcPath[0] = 0;
    dStrcat(srcPath, "*/");
    dStrcat(srcPath, buf1);

    char absPath[1152];
    ResourceObject* resourceObject;
    const char* fileName;
    if (!Con::expandScriptFilename(absPath, 1024, srcPath) ||
        (resourceObject = ResourceManager->findMatch(absPath, &fileName)) == 0)
    {
        Con::errorf("Unable to find %s", buf1);
        return "";
    }

    Tokenizer tokenizer;
    if (!tokenizer.openFile(fileName))
    {
        Con::errorf("Unable to open %s", fileName);
        return "";
    }

    StringTableEntry className = StringTable->insert("InteriorInstance");
    if (argc > 2 && dStrlen((argv[2])) > 0)
        className = StringTable->insert(argv[2]);

    StringTableEntry dataBlockName = StringTable->insert("");
    if (argc > 3 && dStrlen((argv[3])) > 0)
        dataBlockName = StringTable->insert(argv[3]);

    Vector<ItrGameEntity*> gameEntities;

    while (tokenizer.advanceToken(true))
    {
        const char* token = tokenizer.getToken();
        if (dStrstr(token, className))
        {
            bool isGameEntity = argc <= 3;

            ItrGameEntity* entity = new ItrGameEntity;
            entity->mGameClass = className;

            tokenizer.advanceToken(true);
            tokenizer.advanceToken(true);

            if (tokenizer.getToken()[0] != '}')
            {
                do
                {
                    entity->mDictionary.increment();
                    const char *token2 = tokenizer.getToken();
                    dSprintf(entity->mDictionary[entity->mDictionary.size() - 1].name, 256, "%s", token2);

                    tokenizer.advanceToken(false);
                    tokenizer.advanceToken(false);

                    char buf4[1024];
                    if (tokenizer.getToken()[0] == '.')
                    {
                        const char* token3 = tokenizer.getToken();
                        dSprintf(buf4, 256, "%s/%s", resourceObject->path, token3 + 2); // why + 2?
                    } else
                    {
                        const char* token3 = tokenizer.getToken();
                        dSprintf(buf4, 1024, "%s", token3);
                    }
                    dSprintf(entity->mDictionary[entity->mDictionary.size() - 1].value, 256, "%s", buf4);

                    if (argc == 4
                        && !dStricmp(entity->mDictionary[entity->mDictionary.size() - 1].name, "datablock")
                        && !dStricmp(entity->mDictionary[entity->mDictionary.size() - 1].value, dataBlockName))
                    {
                        isGameEntity = 1;
                    }

                    tokenizer.advanceToken(false);
                    tokenizer.advanceToken(true);
                } while (tokenizer.getToken()[0] != '}');
            }

            if (isGameEntity)
                gameEntities.push_back(entity);
        }
    }

    buf1[dStrlen(buf1) - 4] = 0;

    char buf5[256];
    dSprintf(buf5, 256, "%sGroup", buf1);
    if (buf5[0] < '0' || buf5[0] > '9')
    {
        SimGroup *group = static_cast<SimGroup*>(Sim::findObject(buf5));
        if (!group)
        {
            group = static_cast<SimGroup*>(ConsoleObject::create("SimGroup"));
            group->registerObject(buf5);
        }

        StringTableEntry missionGroupName = StringTable->insert("MissionGroup");
        if (argc > 4 && dStrlen((argv[4])) > 0)
            missionGroupName = StringTable->insert(argv[4]);

        SimSet* missionGroup = static_cast<SimSet*>(Sim::findObject(missionGroupName));
        if (missionGroup)
            missionGroup->addObject(group);
        else
            Con::errorf("********* Unable to find MissionGroup!");

        for (int m = 0; m < gameEntities.size(); m++)
        {
            
            ItrGameEntity* gameObj = gameEntities[m];
            ConsoleObject*  conObj = ConsoleObject::create(gameObj->mGameClass);
            SceneObject* sceneObject = dynamic_cast<SceneObject*>(conObj);
            if (!sceneObject)
                Con::errorf("Invalid game class for entity: %s", gameEntities[m]->mGameClass);
            else
            {
                sceneObject->setModStaticFields(true);

                for (int i = 0; i < gameObj->mDictionary.size(); i++)
                    sceneObject->setDataField(StringTable->insert(gameObj->mDictionary[i].name, 0), 0, gameObj->mDictionary[i].value);
                
                sceneObject->setModStaticFields(false);
                sceneObject->setHidden(true);

                if (sceneObject->registerObject())
                    group->addObject(sceneObject);
            }
        }
        
        for (int i = 0; i < gameEntities.size(); i++)
        {
            if (gameEntities[i])
                delete gameEntities[i];
        }

        gameEntities.clear();
        char* ret = Con::getReturnBuffer(256);
        dSprintf(ret, 256, "%s", buf5);
        return ret;
    }
    else
    {
        Con::errorf("Don't use mission names that begin with numbers like %s!", buf1);

        for (int i = 0; i < gameEntities.size(); i++)
        {
            if (gameEntities[i])
                delete gameEntities[i];
        }

        gameEntities.clear();

        return "";
    }
}
#endif

#ifdef MB_ULTRA
ConsoleFunction(SetupGemSpawnGroups, void, 3, 3, "(gemGroupRadius, maxGemsPerGroup) - Sets up gem spawn groups")
{
    U32 start = Platform::getRealMilliseconds();
    S32 gemGroupRadius = dAtoi(argv[1]);
    S32 maxGemsPerGroup = dAtoi(argv[2]);

    SimObject* oldGroup = Sim::findObject("GemSpawnGroups");
    if (oldGroup)
        oldGroup->deleteObject();

    SimGroup* gemSpawnGroups = new SimGroup;
    gemSpawnGroups->assignName("GemSpawnGroups");
    gemSpawnGroups->registerObject();

    SimGroup* gemSpawns = dynamic_cast<SimGroup*>(Sim::findObject("GemSpawns"));

    Map<U32, bool, HashTable<U32, bool>> spawnInScene;
    if (gemSpawns)
    {
        for (S32 i = 0; i < gemSpawns->size(); i++)
        {
            SceneObject* obj = dynamic_cast<SceneObject*>((*gemSpawns)[i]);
            if (obj)
            {
                spawnInScene[obj->getId()] = obj->getSceneManager() != 0;
                if (!obj->getSceneManager())
                    obj->addToScene();
            }
        }

        for (S32 i = 0; i < gemSpawns->size(); i++)
        {
            SceneObject* obj = dynamic_cast<SceneObject*>((*gemSpawns)[i]);
            if (obj)
            {
                SimSet* spawnGroup = new SimSet;
                spawnGroup->registerObject();
                gemSpawnGroups->addObject(spawnGroup);
                spawnGroup->addObject(obj);

                getCurrentServerContainer()->initRadiusSearch(obj->getPosition(), gemGroupRadius, 1);

                while (true)
                {
                    U32 search = getCurrentServerContainer()->containerSearchNext();
                    if (!search || spawnGroup->size() >= maxGemsPerGroup)
                        break;

                    SpawnSphere* spawnSphere = dynamic_cast<SpawnSphere*>(Sim::findObject(search));
                    if (spawnSphere && spawnSphere->getId() != obj->getId())
                        spawnGroup->addObject(spawnSphere);
                }
            }
        }

        for (S32 i = 0; i < gemSpawns->size(); i++)
        {
            SceneObject* obj = dynamic_cast<SceneObject*>((*gemSpawns)[i]);
            if (obj)
            {
                if (!spawnInScene[obj->getId()])
                    obj->removeFromScene();
            }
        }
    }

    U32 end = Platform::getRealMilliseconds();
    Con::printf("Created %d gem spawn groups using radius %d and max gems %d in %ums",
                gemSpawnGroups->size(),
                gemGroupRadius,
                maxGemsPerGroup,
                end - start);
}
#endif

//--------------------------------------------------------------------------
ConsoleFunction(gotoWebPage, void, 2, 2, "( address ) - Open a web page in the user's favorite web browser.")
{
    argc;
    Platform::openWebBrowser(argv[1]);
}

//--------------------------------------------------------------------------
ConsoleFunction(deactivateDirectInput, void, 1, 1, "Deactivate input. (ie, ungrab the mouse so the user can do other things.")
{
    argc; argv;
    if (Input::isActive())
        Input::deactivate();
}

//--------------------------------------------------------------------------
ConsoleFunction(activateDirectInput, void, 1, 1, "Activate input. (ie, grab the mouse again so the user can play our game.")
{
    argc; argv;
    if (!Input::isActive())
        Input::activate();
}


//--------------------------------------------------------------------------
ConsoleFunction(purgeResources, void, 1, 1, "Purge resources from the resource manager.")
{
    ResourceManager->purge();
}


//--------------------------------------------------------------------------
ConsoleFunction(lightScene, bool, 1, 3, "(script_function completeCallback=NULL, string mode=\"\")"
    "Relight the scene.\n\n"
    "If mode is \"forceAlways\", the lightmaps will be regenerated regardless of whether "
    "lighting cache files can be written to. If mode is \"forceWritable\", then the lightmaps "
    "will be regenerated only if the lighting cache files can be written.")
{
    const char* callback = StringTable->insert(argv[1]);
    BitSet32 flags = 0;

    if (argc > 1)
    {
        if (!dStricmp(argv[2], "forceAlways"))
            flags.set(SceneLighting::ForceAlways);
        else if (!dStricmp(argv[2], "forceWritable"))
            flags.set(SceneLighting::ForceWritable);
    }

    return(SceneLighting::lightScene(callback, flags));
}

//--------------------------------------------------------------------------
ConsoleFunction(registerLights, void, 1, 1, "()")
{
    LightManager* lManager = getCurrentClientSceneGraph()->getLightManager();
    lManager->sgRegisterGlobalLights(false);
}

//--------------------------------------------------------------------------

static const U32 MaxPlayerNameLength = 16;
ConsoleFunction(strToPlayerName, const char*, 2, 2, "strToPlayerName( string )")
{
    argc;

    const char* ptr = argv[1];

    // Strip leading spaces and underscores:
    while (*ptr == ' ' || *ptr == '_')
        ptr++;

    U32 len = dStrlen(ptr);
    if (len)
    {
        char* ret = Con::getReturnBuffer(MaxPlayerNameLength + 1);
        char* rptr = ret;
        ret[MaxPlayerNameLength - 1] = '\0';
        ret[MaxPlayerNameLength] = '\0';
        bool space = false;

        U8 ch;
        while (*ptr && dStrlen(ret) < MaxPlayerNameLength)
        {
            ch = (U8)*ptr;

            // Strip all illegal characters:
            if (ch < 32 || ch == ',' || ch == '.' || ch == '\'' || ch == '`')
            {
                ptr++;
                continue;
            }

            // Don't allow double spaces or space-underline combinations:
            if (ch == ' ' || ch == '_')
            {
                if (space)
                {
                    ptr++;
                    continue;
                }
                else
                    space = true;
            }
            else
                space = false;

            *rptr++ = *ptr;
            ptr++;
        }
        *rptr = '\0';

        //finally, strip out the ML text control chars...
        return GuiMLTextCtrl::stripControlChars(ret);
        return(ret);
    }

    return("");
}

//--------------------------------------------------------------------------
ConsoleFunction(stripTrailingSpaces, const char*, 2, 2, "stripTrailingSpaces( string )")
{
    argc;
    S32 temp = S32(dStrlen(argv[1]));
    if (temp)
    {
        while ((argv[1][temp - 1] == ' ' || argv[1][temp - 1] == '_') && temp >= 1)
            temp--;

        if (temp)
        {
            char* returnString = Con::getReturnBuffer(temp + 1);
            dStrncpy(returnString, argv[1], U32(temp));
            returnString[temp] = '\0';
            return(returnString);
        }
    }

    return("");
}


//--------------------------------------------------------------------------
ConsoleFunction(flushTextureCache, void, 1, 1, "Flush the texture cache.")
{
    //TextureManager::flush();
    GFX->zombifyTextureManager();
    GFX->resurrectTextureManager();
}


#ifdef TORQUE_GATHER_METRICS
ConsoleFunction(dumpTextureStats, void, 1, 1, "Dump texture manager statistics. Debug only!")
{
    //   TextureManager::dumpStats();
}
#endif

#ifdef TORQUE_DEBUG
ConsoleFunction(dumpResourceStats, void, 1, 1, "Dump information about resources. Debug only!")
{
    ResourceManager->dumpLoadedResources();
}
#endif

//------------------------------------------------------------------------------

static U32 moveCount = 0;

bool GameGetCameraTransform(MatrixF* mat, Point3F* velocity)
{
    // Return the position and velocity of the control object
    GameConnection* connection = GameConnection::getConnectionToServer();
    return connection && connection->getControlCameraTransform(0, mat) &&
        connection->getControlCameraVelocity(velocity);
}

//------------------------------------------------------------------------------
/// Camera and FOV info
namespace {

    const  U32 MaxZoomSpeed = 2000;     ///< max number of ms to reach target FOV

    static F32 sConsoleCameraFov = 90.f;     ///< updated to camera FOV each frame
    static F32 sDefaultFov = 90.f;     ///< normal FOV
    static F32 sCameraFov = 90.f;     ///< current camera FOV
    static F32 sTargetFov = 90.f;     ///< the desired FOV
    static F32 sLastCameraUpdateTime = 0;        ///< last time camera was updated
    static S32 sZoomSpeed = 500;      ///< ms per 90deg fov change

} // namespace {}

//------------------------------------------------------------------------------
ConsoleFunctionGroupBegin(CameraFunctions, "Functions controlling the global camera properties defined in main.cc.");

ConsoleFunction(setDefaultFov, void, 2, 2, "(defaultFov) - Set the default FOV for a camera.")
{
    argc;
    sDefaultFov = mClampF(dAtof(argv[1]), MinCameraFov, MaxCameraFov);
    if (sCameraFov == sTargetFov)
        sTargetFov = sDefaultFov;
}

ConsoleFunction(setZoomSpeed, void, 2, 2, "(speed) - Set the zoom speed of the camera, in ms per 90deg FOV change.")
{
    argc;
    sZoomSpeed = mClamp(dAtoi(argv[1]), 0, MaxZoomSpeed);
}

ConsoleFunction(setFov, void, 2, 2, "(fov) - Set the FOV of the camera.")
{
    argc;
    sTargetFov = mClampF(dAtof(argv[1]), MinCameraFov, MaxCameraFov);
}

ConsoleFunctionGroupEnd(CameraFunctions);

F32 GameGetCameraFov()
{
    return(sCameraFov);
}

void GameSetCameraFov(F32 fov)
{
    sTargetFov = sCameraFov = fov;
}

void GameSetCameraTargetFov(F32 fov)
{
    sTargetFov = fov;
}

void GameUpdateCameraFov()
{
    F32 time = F32(Platform::getVirtualMilliseconds());

    // need to update fov?
    if (sTargetFov != sCameraFov)
    {
        F32 delta = time - sLastCameraUpdateTime;

        // snap zoom?
        if ((sZoomSpeed == 0) || (delta <= 0.f))
            sCameraFov = sTargetFov;
        else
        {
            // gZoomSpeed is time in ms to zoom 90deg
            F32 step = 90.f * (delta / F32(sZoomSpeed));

            if (sCameraFov > sTargetFov)
            {
                sCameraFov -= step;
                if (sCameraFov < sTargetFov)
                    sCameraFov = sTargetFov;
            }
            else
            {
                sCameraFov += step;
                if (sCameraFov > sTargetFov)
                    sCameraFov = sTargetFov;
            }
        }
    }

    // the game connection controls the vertical and the horizontal
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection)
    {
        // check if fov is valid on control object
        if (connection->isValidControlCameraFov(sCameraFov))
            connection->setControlCameraFov(sCameraFov);
        else
        {
            // will set to the closest fov (fails only on invalid control object)
            if (connection->setControlCameraFov(sCameraFov))
            {
                F32 setFov = sCameraFov;
                connection->getControlCameraFov(&setFov);
                sTargetFov = sCameraFov = setFov;
            }
        }
    }

    // update the console variable
    sConsoleCameraFov = sCameraFov;
    sLastCameraUpdateTime = time;
}
//--------------------------------------------------------------------------

#ifdef TORQUE_DEBUG
// ConsoleFunction(dumpTSShapes, void, 1, 1, "dumpTSShapes();")
// {
//    argc, argv;

//    FindMatch match("*.dts", 4096);
//    ResourceManager->findMatches(&match);

//    for (U32 i = 0; i < match.numMatches(); i++)
//    {
//       U32 j;
//       Resource<TSShape> shape = ResourceManager->load(match.matchList[i]);
//       if (bool(shape) == false)
//          Con::errorf(" aaa Couldn't load: %s", match.matchList[i]);

//       U32 numMeshes = 0, numSkins = 0;
//       for (j = 0; j < shape->meshes.size(); j++)
//          if (shape->meshes[j])
//             numMeshes++;
//       for (j = 0; j < shape->skins.size(); j++)
//          if (shape->skins[j])
//             numSkins++;

//      Con::printf(" aaa Shape: %s (%d meshes, %d skins)", match.matchList[i], numMeshes, numSkins);
//       Con::printf(" aaa   Meshes");
//       for (j = 0; j < shape->meshes.size(); j++)
//       {
//          if (shape->meshes[j])
//             Con::printf(" aaa     %d -> nf: %d, nmf: %d, nvpf: %d (%d, %d, %d, %d, %d)",
//                         shape->meshes[j]->meshType & TSMesh::TypeMask,
//                         shape->meshes[j]->numFrames,
//                         shape->meshes[j]->numMatFrames,
//                         shape->meshes[j]->vertsPerFrame,
//                         shape->meshes[j]->verts.size(),
//                         shape->meshes[j]->norms.size(),
//                         shape->meshes[j]->tverts.size(),
//                         shape->meshes[j]->primitives.size(),
//                         shape->meshes[j]->indices.size());
//       }
//       Con::printf(" aaa   Skins");
//       for (j = 0; j < shape->skins.size(); j++)
//       {
//          if (shape->skins[j])
//             Con::printf(" aaa     %d -> nf: %d, nmf: %d, nvpf: %d (%d, %d, %d, %d, %d)",
//                         shape->skins[j]->meshType & TSMesh::TypeMask,
//                         shape->skins[j]->numFrames,
//                         shape->skins[j]->numMatFrames,
//                         shape->skins[j]->vertsPerFrame,
//                         shape->skins[j]->verts.size(),
//                         shape->skins[j]->norms.size(),
//                         shape->skins[j]->tverts.size(),
//                         shape->skins[j]->primitives.size(),
//                         shape->skins[j]->indices.size());
//       }
//    }
// }
#endif

ConsoleFunction(getControlObjectAltitude, const char*, 1, 1, "Get distance from bottom of controlled object to terrain.")
{
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection) {
        ShapeBase* pSB = connection->getControlObject();
        if (pSB != NULL && pSB->isClientObject())
        {
            Point3F pos(0.f, 0.f, 0.f);

            // if this object is mounted, then get the bottom position of the mount's bbox
            if (pSB->getObjectMount())
            {
                static Point3F BoxPnts[] = {
                   Point3F(0,0,0),
                   Point3F(0,0,1),
                   Point3F(0,1,0),
                   Point3F(0,1,1),
                   Point3F(1,0,0),
                   Point3F(1,0,1),
                   Point3F(1,1,0),
                   Point3F(1,1,1)
                };

                ShapeBase* mount = pSB->getObjectMount();
                Box3F box = mount->getObjBox();
                MatrixF mat = mount->getTransform();
                VectorF scale = mount->getScale();

                Point3F projPnts[8];
                F32 minZ = 1e30;

                for (U32 i = 0; i < 8; i++)
                {
                    Point3F pnt(BoxPnts[i].x ? box.max.x : box.min.x,
                        BoxPnts[i].y ? box.max.y : box.min.y,
                        BoxPnts[i].z ? box.max.z : box.min.z);

                    pnt.convolve(scale);
                    mat.mulP(pnt, &projPnts[i]);

                    if (projPnts[i].z < minZ)
                        pos = projPnts[i];
                }
            }
            else
                pSB->getTransform().getColumn(3, &pos);

#ifdef TORQUE_TERRAIN
            TerrainBlock* pBlock = getCurrentClientSceneGraph()->getCurrentTerrain();
            if (pBlock != NULL) {
                Point3F terrPos = pos;
                pBlock->getWorldTransform().mulP(terrPos);
                terrPos.convolveInverse(pBlock->getScale());

                F32 height;
                if (pBlock->getHeight(Point2F(terrPos.x, terrPos.y), &height) == true) {
                    terrPos.z = height;
                    terrPos.convolve(pBlock->getScale());
                    pBlock->getTransform().mulP(terrPos);

                    pos.z -= terrPos.z;
                }
            }
#endif

            char* retBuf = Con::getReturnBuffer(128);
            dSprintf(retBuf, 128, "%g", mFloor(getMax(pos.z, 0.f)));
            return retBuf;
        }
    }

    return "0";
}

ConsoleFunction(getControlObjectSpeed, const char*, 1, 1, "Get speed (but not velocity) of controlled object.")
{
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection)
    {
        ShapeBase* pSB = connection->getControlObject();
        if (pSB != NULL && pSB->isClientObject()) {
            Point3F vel = pSB->getVelocity();
            F32 speed = vel.len();

            // We're going to force the formating to be what we want...
            F32 intPart = mFloor(speed);
            speed -= intPart;
            speed *= 10;
            speed = mFloor(speed);

            char* retBuf = Con::getReturnBuffer(128);
            dSprintf(retBuf, 128, "%g.%g", intPart, speed);
            return retBuf;
        }
    }

    return "0";
}

//--------------------------------------------------------------------------
ConsoleFunction(panoramaScreenShot, void, 3, 3, "(string file, string format)"
    "Take a panoramic screenshot.\n\n"
    "@param format This is either JPEG or PNG.")
{
    /*
       S32 numShots = 3;
       if (argc == 3)
          numShots = dAtoi(argv[2]);

       CameraQuery query;
       if (!GameProcessCameraQuery( &query ))
          return;

       SceneObject *object = dynamic_cast<SceneObject*>(query.object);
       if (!object)
          return;

       F32 rotInc = query.fov * 0.75f;

       FileStream fStream;
       GBitmap bitmap;
       Point2I extent = Canvas->getExtent();
       bitmap.allocateBitmap(U32(extent.x), U32(extent.y));
       U8 * pixels = new U8[extent.x * extent.y * 3];


       S32 start = -(numShots/2);
       for (S32 i=0; i<numShots; i++, start++)
       {
          char buffer[256];

          MatrixF rot( EulerF(0.0f, 0.0f, rotInc * F32(start)) );
          MatrixF result;
          result.mul(query.cameraMatrix, rot);

          object->setTransform( result );
          Canvas->renderFrame(false);
          dSprintf(buffer, sizeof(buffer), "%s-%d.png", argv[1], i);

          glReadBuffer(GL_FRONT);
          glReadPixels(0, 0, extent.x, extent.y, GL_RGB, GL_UNSIGNED_BYTE, pixels);

          if(!fStream.open(buffer, FileStream::Write))
          {
             Con::printf("Failed to open file '%s'.", buffer);
             break;
          }

          // flip the rows
          for(U32 y = 0; y < extent.y; y++)
             dMemcpy(bitmap.getAddress(0, extent.y - y - 1), pixels + y * extent.x * 3, U32(extent.x * 3));

          if ( dStrcmp( argv[2], "JPEG" ) == 0 )
              bitmap.writeJPEG(fStream);
          else if( dStrcmp( argv[2], "PNG" ) == 0)
              bitmap.writePNG(fStream);
          else
              bitmap.writePNG(fStream);

          fStream.close();
       }

       delete [] pixels;
    */
}

ConsoleFunctionGroupEnd(GameFunctions);

//------------------------------------------------------------------------------
void GameInit()
{
    // Make sure the exporter draws from the correct directories...
    //
    Con::addVariable("movementSpeed", TypeF32, &gMovementSpeed);
    /*
       Con::addVariable("$pref::OpenGL::disableEXTPalettedTexture",     TypeBool, &gOpenGLDisablePT);
       Con::addVariable("$pref::OpenGL::disableEXTCompiledVertexArray", TypeBool, &gOpenGLDisableCVA);
       Con::addVariable("$pref::OpenGL::disableARBMultitexture",        TypeBool, &gOpenGLDisableARBMT);
       Con::addVariable("$pref::OpenGL::disableEXTFogCoord",            TypeBool, &gOpenGLDisableFC);
       Con::addVariable("$pref::OpenGL::disableEXTTexEnvCombine",       TypeBool, &gOpenGLDisableTEC);
       Con::addVariable("$pref::OpenGL::disableARBTextureCompression",  TypeBool, &gOpenGLDisableTCompress);
       Con::addVariable("$pref::OpenGL::noEnvColor",		              TypeBool, &gOpenGLNoEnvColor);
       Con::addVariable("$pref::OpenGL::gammaCorrection",               TypeF32,  &gOpenGLGammaCorrection);
        Con::addVariable("$pref::OpenGL::noDrawArraysAlpha",				  TypeBool, &gOpenGLNoDrawArraysAlpha);
    */
    Con::addVariable("$pref::TS::autoDetail", TypeF32, &DetailManager::smDetailScale);
    Con::addVariable("$pref::visibleDistanceMod", TypeF32, &SceneGraph::smVisibleDistanceMod);

    // updated every frame
    Con::addVariable("cameraFov", TypeF32, &sConsoleCameraFov);

    // Initialize the collision testing script stuff.
    collisionTest.consoleInit();
}

const U32 AudioUpdatePeriod = 125;  ///< milliseconds between audio updates.

bool clientProcess(U32 timeDelta)
{
    if (gSPMode)
        return true;

    ShowTSShape::advanceTime(timeDelta);
    ITickable::advanceTime(timeDelta);

    bool ret = gClientProcessList.advanceClientTime(timeDelta);

    // Run the collision test and update the Audio system
    // by checking the controlObject
    MatrixF mat;
    Point3F velocity;

    if (GameGetCameraTransform(&mat, &velocity))
    {
        SFX->getListener().setTransform(mat);
        SFX->getListener().setVelocity(velocity);
    }

    // determine if were lagging
    GameConnection* connection = GameConnection::getConnectionToServer();
    if (connection)
        connection->detectLag();
    
    // Let SFX process.
    SFX->_update();

    return ret;
}

bool serverProcess(U32 timeDelta)
{
    if (gSPMode)
        return true;
    return gServerProcessList.advanceServerTime(timeDelta);
}

bool spmodeProcess(U32 timeDelta)
{
    if (!gSPMode)
        return false;

    ITickable::advanceTime(timeDelta);
    bool ret = getCurrentServerProcessList()->advanceSPModeTime(timeDelta);

    // Run the collision test and update the Audio system
    // by checking the controlObject
    MatrixF mat;
    Point3F velocity;

    if (GameGetCameraTransform(&mat, &velocity))
    {
        SFX->getListener().setTransform(mat);
        SFX->getListener().setVelocity(velocity);
    }

    return ret;
}

static ColorF cubeColors[8] = {
   ColorF(0, 0, 0),
   ColorF(1, 0, 0),
   ColorF(0, 1, 0),
   ColorF(0, 0, 1),
   ColorF(1, 1, 0),
   ColorF(1, 0, 1),
   ColorF(0, 1, 1),
   ColorF(1, 1, 1)
};

static Point3F cubePoints[8] = {
   Point3F(-1, -1, -1),
   Point3F(-1, -1,  1),
   Point3F(-1,  1, -1),
   Point3F(-1,  1,  1),
   Point3F(1, -1, -1),
   Point3F(1, -1,  1),
   Point3F(1,  1, -1),
   Point3F(1,  1,  1)
};

static U32 cubeFaces[6][4] = {
   { 0, 2, 6, 4 },
   { 0, 2, 3, 1 },
   { 0, 1, 5, 4 },
   { 3, 2, 6, 7 },
   { 7, 6, 4, 5 },
   { 3, 7, 5, 1 }
};

void wireCube(F32 size, Point3F pos)
{
    /*
       glDisable(GL_CULL_FACE);

       for (S32 i = 0; i < 6; i++)
       {
          glBegin(GL_LINE_LOOP);
          for(S32 vert = 0; vert < 4; vert++)
          {
             U32 idx = cubeFaces[i][vert];
             glColor3f(cubeColors[idx].red, cubeColors[idx].green, cubeColors[idx].blue);
             glVertex3f(cubePoints[idx].x * size + pos.x, cubePoints[idx].y * size + pos.y, cubePoints[idx].z * size + pos.z);
          }
          glEnd();
       }
    */
}

bool GameProcessCameraQuery(CameraQuery* query)
{
    GameConnection* connection = GameConnection::getConnectionToServer();

    if (connection && connection->getControlCameraTransform(0.032f, &query->cameraMatrix))
    {
        query->object = connection->getControlObject();

        Sky* pSky = getCurrentClientSceneGraph()->getCurrentSky();

        if (pSky)
            query->farPlane = pSky->getVisibleDistance();
        else
            query->farPlane = 1000.0f;

#ifdef MB_ULTRA
        query->nearPlane = 0.1f;
#else
        query->nearPlane = query->farPlane / 5000.0f;
#endif

        F32 cameraFov;
        if (!connection->getControlCameraFov(&cameraFov))
            return false;

        query->fov = mDegToRad(cameraFov);
        return true;
    }
    return false;
}


struct OutputPoint
{
    Point3F point;
    U8      color[4];
    Point2F texCoord;
    Point2F fogCoord;
};

#define USEOLDFILTERS 1

void GameRenderFilters(const CameraQuery& camq)
{
    /*
    #if USEOLDFILTERS

       GameConnection* connection = GameConnection::getConnectionToServer();

       F32 damageFlash = 0;
       F32 whiteOut = 0;
       F32 blackOut = 0;

       if(connection)
       {
          damageFlash = connection->getDamageFlash();
          whiteOut = connection->getWhiteOut();
          blackOut = connection->getBlackOut();
       }

       ShapeBase* psb = dynamic_cast<ShapeBase*>(camq.object);
       if (psb != NULL) {
          if (damageFlash > 0.0) {
             if (damageFlash > 0.76)
                damageFlash = 0.76f;

             glMatrixMode(GL_MODELVIEW);
             glPushMatrix();
             glLoadIdentity();
             glMatrixMode(GL_PROJECTION);
             glPushMatrix();
             glLoadIdentity();

             glDisable(GL_TEXTURE_2D);
             glEnable(GL_BLEND);
             glDepthMask(GL_FALSE);
             glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
             glColor4f(1, 0, 0, damageFlash);
             glBegin(GL_TRIANGLE_FAN);
             glVertex3f(-1, -1, 0);
             glVertex3f(-1,  1, 0);
             glVertex3f( 1,  1, 0);
             glVertex3f( 1, -1, 0);
             glEnd();
             glDisable(GL_BLEND);
             glBlendFunc(GL_ONE, GL_ZERO);
             glDepthMask(GL_TRUE);

             glMatrixMode(GL_PROJECTION);
             glPopMatrix();
             glMatrixMode(GL_MODELVIEW);
             glPopMatrix();
          }

          if (whiteOut > 0.0) {
             glMatrixMode(GL_MODELVIEW);
             glPushMatrix();
             glLoadIdentity();
             glMatrixMode(GL_PROJECTION);
             glPushMatrix();
             glLoadIdentity();

             glDisable(GL_TEXTURE_2D);
             glEnable(GL_BLEND);
             glDepthMask(GL_FALSE);
             glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
             glColor4f(1.0f, 1.0f, 0.92f, (whiteOut > 1.0f ? 1.0f : whiteOut));
             glBegin(GL_TRIANGLE_FAN);
             glVertex3f(-1, -1, 0);
             glVertex3f(-1,  1, 0);
             glVertex3f( 1,  1, 0);
             glVertex3f( 1, -1, 0);
             glEnd();
             glDisable(GL_BLEND);
             glBlendFunc(GL_ONE, GL_ZERO);
             glDepthMask(GL_TRUE);

             glMatrixMode(GL_PROJECTION);
             glPopMatrix();
             glMatrixMode(GL_MODELVIEW);
             glPopMatrix();
          }

          if (blackOut > 0.0) {
             glMatrixMode(GL_MODELVIEW);
             glPushMatrix();
             glLoadIdentity();
             glMatrixMode(GL_PROJECTION);
             glPushMatrix();
             glLoadIdentity();

             glDisable(GL_TEXTURE_2D);
             glEnable(GL_BLEND);
             glDepthMask(GL_FALSE);
             glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
             glColor4f(0.0f, 0.0f, 0.0f, (blackOut > 1.0f ? 1.0f : blackOut));
             glBegin(GL_TRIANGLE_FAN);
             glVertex3f(-1, -1, 0);
             glVertex3f(-1,  1, 0);
             glVertex3f( 1,  1, 0);
             glVertex3f( 1, -1, 0);
             glEnd();
             glDisable(GL_BLEND);
             glBlendFunc(GL_ONE, GL_ZERO);
             glDepthMask(GL_TRUE);

             glMatrixMode(GL_PROJECTION);
             glPopMatrix();
             glMatrixMode(GL_MODELVIEW);
             glPopMatrix();
          }

          F32 invincible = psb->getInvincibleEffect();
          if (invincible > 0.0) {
             glMatrixMode(GL_MODELVIEW);
             glPushMatrix();
             glLoadIdentity();
             glMatrixMode(GL_PROJECTION);
             glPushMatrix();
             glLoadIdentity();

             glDisable(GL_TEXTURE_2D);
             glEnable(GL_BLEND);
             glDepthMask(GL_FALSE);
             glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
             glColor4f(0, 0, 1, (invincible > 1.0f ? 1.0f : invincible));
             glBegin(GL_TRIANGLE_FAN);
             glVertex3f(-1, -1, 0);
             glVertex3f(-1,  1, 0);
             glVertex3f( 1,  1, 0);
             glVertex3f( 1, -1, 0);
             glEnd();
             glDisable(GL_BLEND);
             glBlendFunc(GL_ONE, GL_ZERO);
             glDepthMask(GL_TRUE);

             glMatrixMode(GL_PROJECTION);
             glPopMatrix();
             glMatrixMode(GL_MODELVIEW);
             glPopMatrix();
          }

          if (WaterBlock::mCameraSubmerged)
          {
             if (WaterBlock::isWater(WaterBlock::mSubmergedType))
             {
                // view filter for camera below the water surface
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadIdentity();

                glDisable(GL_TEXTURE_2D);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(.2, .6, .6, .3);
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(-1, -1, 0);
                glVertex3f(-1,  1, 0);
                glVertex3f( 1,  1, 0);
                glVertex3f( 1, -1, 0);
                glEnd();
                glDisable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ZERO);
                glDepthMask(GL_TRUE);

                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
             }
             else if (WaterBlock::isLava(WaterBlock::mSubmergedType))
             {
                gLavaFX.render();
             }
             else if (WaterBlock::isQuicksand(WaterBlock::mSubmergedType))
             {
             }
          }
          WaterBlock::mCameraSubmerged = false;
          WaterBlock::mSubmergedType   = 0;
       }
    #else

       //
       // Need to build a filter for damage, invincibility, underwater, whiteout... ect
       //
       //    Damage, Whiteout, and invincible effects have constant color with variable
       //    alpha values. The water filter has a constant color and alpha value. This
       //    looks kinda tricky, and it is. See Frohn for more details  Jett-

       // first get the game connection for this player
       GameConnection* connection = GameConnection::getConnectionToServer();

       bool addWaterFilter  = false;
       bool addLavaFilter   = false;
       F32 maxDamageFilter  = 0.77;
       F32 damageFlash      = 0.f; ColorF damageFilterColor(1.f, 0.f, 0.f, 0.f);
       F32 whiteOut         = 0.f; ColorF whiteoutFilterColor(1.f, 1.f, 1.f, 0.f);
       F32 waterFilter      = 0.f; ColorF waterFilterColor(0.2, 0.6, 0.6, 0.6);
       F32 invincible       = 0.f; ColorF invincibleFilterColor(0.f, 0.f, 1.f, 0.f);

       // final color and alpha of filter + an adder
       ColorF Xcolor(0.f, 0.f, 0.f, 1.f);

       if(connection)
       {
          // grab the damage flash alpha value
          damageFlash = connection->getDamageFlash();
          if( damageFlash > maxDamageFilter )
             damageFlash = maxDamageFilter;

          damageFilterColor.alpha = damageFlash;

          // grab the whiteout value
          whiteoutFilterColor.alpha = connection->getWhiteOut();

          // need to grab the player obj to get inv. alpha value
          ShapeBase* psb = dynamic_cast<ShapeBase*>(camq.object);
          if(psb != NULL)
             invincibleFilterColor.alpha = psb->getInvincibleEffect();

          // determine if we need to add in our water filter (constant color and alpha)
          if( WaterBlock::mCameraSubmerged )
          {
             if( WaterBlock::isWater( WaterBlock::mSubmergedType ) )
                addWaterFilter = true;
             else if( WaterBlock::isLava( WaterBlock::mSubmergedType))
                addLavaFilter = true;
          }

          // compute the final color and alpha
          Xcolor         = ( Xcolor * ( 1 - damageFilterColor.alpha ) ) + ( damageFilterColor * damageFilterColor.alpha );
          Xcolor.alpha   =   Xcolor.alpha * ( 1 - damageFilterColor.alpha  );

          Xcolor         = ( Xcolor * ( 1 - whiteoutFilterColor.alpha ) ) + ( whiteoutFilterColor * whiteoutFilterColor.alpha );
          Xcolor.alpha   =   Xcolor.alpha * ( 1 - whiteoutFilterColor.alpha  );

          Xcolor         = ( Xcolor * ( 1 - invincibleFilterColor.alpha ) ) + ( invincibleFilterColor * invincibleFilterColor.alpha );
          Xcolor.alpha   =   Xcolor.alpha * ( 1 - invincibleFilterColor.alpha  );

          // if were sitting in water, then add that filter in as well.
          if(addWaterFilter)
          {
             Xcolor         = ( Xcolor * ( 1 - waterFilterColor.alpha ) ) + ( waterFilterColor * waterFilterColor.alpha );
             Xcolor.alpha   =   Xcolor.alpha * ( 1 - waterFilterColor.alpha  );
          }

          // draw our filter with final color
          glMatrixMode(GL_MODELVIEW);
          glPushMatrix();
          glLoadIdentity();
          glMatrixMode(GL_PROJECTION);
          glPushMatrix();
          glLoadIdentity();

          glDisable(GL_TEXTURE_2D);
          glEnable(GL_BLEND);
          glDepthMask(GL_FALSE);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE);
          glColor4f(Xcolor.red, Xcolor.blue, Xcolor.blue, Xcolor.alpha);
          glBegin(GL_TRIANGLE_FAN);
          glVertex3f(-1, -1, 0);
          glVertex3f(-1,  1, 0);
          glVertex3f( 1,  1, 0);
          glVertex3f( 1, -1, 0);
          glEnd();
          glDisable(GL_BLEND);
          glBlendFunc(GL_ONE, GL_ZERO);
          glDepthMask(GL_TRUE);

          glMatrixMode(GL_PROJECTION);
          glPopMatrix();
          glMatrixMode(GL_MODELVIEW);
          glPopMatrix();


          // if were under lava, apply appropriate texture
          if( addLavaFilter )
          {
             gLavaFX.render();
          }

          WaterBlock::mCameraSubmerged = false;
          WaterBlock::mSubmergedType   = 0;
       }

    #endif
    */
}

void GameRenderWorld()
{
    PROFILE_START(GameRenderWorld);
    FrameAllocator::setWaterMark(0);

    GFX->setZEnable(true); //glEnable(GL_DEPTH_TEST);
    GFX->setZFunc(GFXCmpLessEqual); //glDepthFunc(GL_LEQUAL);


#if defined(TORQUE_GATHER_METRICS) && TORQUE_GATHER_METRICS > 1
    TextureManager::smTextureCacheMisses = 0;
#endif

    // Need to consoldate to one clear call // glClear(GL_DEPTH_BUFFER_BIT);
    GFX->setCullMode(GFXCullNone);//glDisable(GL_CULL_FACE);

    getCurrentClientSceneGraph()->renderScene();
    GFX->setZEnable(false); //glDisable(GL_DEPTH_TEST);
    collisionTest.render();

#if defined(TORQUE_GATHER_METRICS) && TORQUE_GATHER_METRICS > 1
    Con::setFloatVariable("Video::texResidentPercentage",
        TextureManager::getResidentFraction());
    Con::setIntVariable("Video::textureCacheMisses",
        TextureManager::smTextureCacheMisses);
#endif

    AssertFatal(FrameAllocator::getWaterMark() == 0, "Error, someone didn't reset the water mark on the frame allocator!");
    FrameAllocator::setWaterMark(0);
    PROFILE_END();
}




