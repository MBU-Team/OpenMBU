//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

/*
#include "sceneGraph/sceneLighting.h"
#include "interior/interiorInstance.h"
#include "interior/interiorRes.h"
#include "interior/interior.h"
#include "gfx/gBitmap.h"
#include "math/mPlane.h"
#include "sceneGraph/sceneGraph.h"
#include "core/fileStream.h"
#include "console/consoleTypes.h"
#include "gui/core/guiCanvas.h"
#include "core/zipSubStream.h"
#include "game/gameConnection.h"

namespace {

   static void findObjectsCallback(SceneObject* obj, void *val)
   {
      Vector<SceneObject*> * list = (Vector<SceneObject*>*)val;
      list->push_back(obj);
   }

   static const Point3F BoxNormals[] =
   {
      Point3F( 1, 0, 0),
      Point3F(-1, 0, 0),
      Point3F( 0, 1, 0),
      Point3F( 0,-1, 0),
      Point3F( 0, 0, 1),
      Point3F( 0, 0,-1)
   };

   static U32 BoxVerts[][4] = {
      {7,6,4,5},     // +x
      {0,2,3,1},     // -x
      {7,3,2,6},     // +y
      {0,1,5,4},     // -y
      {7,5,1,3},     // +z
      {0,4,6,2}      // -z
   };

   static U32 BoxSharedEdgeMask[][6] = {
      {0, 0, 1, 4, 8, 2},
      {0, 0, 2, 8, 4, 1},
      {8, 2, 0, 0, 1, 4},
      {4, 1, 0, 0, 2, 8},
      {1, 4, 8, 2, 0, 0},
      {2, 8, 4, 1, 0, 0}
   };

   static U32 TerrainSquareIndices[][3] = {
      {2, 1, 0},  // 45
      {3, 2, 0},
      {3, 1, 0},  // 135
      {3, 2, 1}
   };

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

   SceneLighting *   gLighting = 0;
   F32               gPlaneNormThresh = 0.999;
   F32               gPlaneDistThresh = 0.001;
   F32               gParellelVectorThresh = 0.01;
   bool              gTerminateLighting = false;
   F32               gLightingProgress = 0.f;
   const char *      gCompleteCallback = 0;
   U32               gConnectionMissionCRC = 0xffffffff;
}


//------------------------------------------------------------------------------
class SceneLightingProcessEvent : public SimEvent
{
   private:
      U32      mLightIndex;
      S32      mObjectIndex;

   public:
      SceneLightingProcessEvent(U32 lightIndex, S32 objectIndex)
      {
         mLightIndex = lightIndex;        // size(): end of lighting
         mObjectIndex = objectIndex;      // -1: preLight, size(): next light
      }

      void process(SimObject * object)
      {
         AssertFatal(object, "SceneLightingProcessEvent:: null event object!");
         if(object)
            static_cast<SceneLighting*>(object)->processEvent(mLightIndex, mObjectIndex);
      };
};

//------------------------------------------------------------------------------
bool SceneLighting::smUseVertexLighting   = false;

SceneLighting::SceneLighting()
{
   mStartTime = 0;
   mFileName[0] = 0;
   smUseVertexLighting = Interior::smUseVertexLighting;

   static bool initialized = false;
   if(!initialized)
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

   ObjectProxy ** proxyItr;
   for(proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
      delete *proxyItr;
}

void SceneLighting::completed(bool success)
{
   // process the cached lighting files
   processCache();

   if(success)
   {
      AssertFatal(smUseVertexLighting == Interior::smUseVertexLighting, "SceneLighting::completed: vertex lighting state changed during scene light");

      // cannot do anything if vertex state has changed (since we only load in what is needed)
      if(smUseVertexLighting == Interior::smUseVertexLighting)
      {
         if(!smUseVertexLighting)
         {
            gInteriorLMManager.downloadGLTextures();
            gInteriorLMManager.destroyBitmaps();
         }
         else
            gInteriorLMManager.destroyTextures();
      }
   }

   if(gCompleteCallback && gCompleteCallback[0])
      Con::executef(1, gCompleteCallback);
}

//------------------------------------------------------------------------------
// Static access method: there can be only one SceneLighting object
bool SceneLighting::lightScene(const char * callback, BitSet32 flags)
{
   if(gLighting)
   {
      Con::errorf(ConsoleLogEntry::General, "SceneLighting:: forcing restart of lighting!");
      gLighting->deleteObject();
      gLighting = 0;
   }

   SceneLighting * lighting = new SceneLighting;

   // register the object
   if(!lighting->registerObject())
   {
      AssertFatal(0, "SceneLighting:: Unable to register SceneLighting object!");
      Con::errorf(ConsoleLogEntry::General, "SceneLighting:: Unable to register SceneLighting object!");
      delete lighting;
      return(false);
   }

   // could have interior resources but no instances (hey, got this far didnt we...)
   GameConnection * con = dynamic_cast<GameConnection*>(NetConnection::getConnectionToServer());
   if(!con)
   {
      Con::errorf(ConsoleLogEntry::General, "SceneLighting:: no game connection");
      return(false);
   }
   con->addObject(lighting);

   // set the globals
   gLighting = lighting;
   gTerminateLighting = false;
   gLightingProgress = 0.f;
   gCompleteCallback = callback;
   gConnectionMissionCRC = con->getMissionCRC();


   if(!lighting->light(flags))
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





//--------------------------------------------------------------------------




//------------------------------------------------------------------------------
void SceneLighting::processEvent(U32 light, S32 object)
{
   // cancel lighting?
   if(gTerminateLighting)
   {
      completed(false);
      deleteObject();
      return;
   }

   ObjectProxy ** proxyItr;

   // last object?
   if(object == mLitObjects.size())
   {
      for(proxyItr = mLitObjects.begin(); proxyItr != mLitObjects.end(); proxyItr++)
      {
         if(!(*proxyItr)->getObject())
         {
            AssertFatal(0, "SceneLighting:: missing sceneobject on light end");
            continue;
         }
         (*proxyItr)->postLight(light == (mLights.size() - 1));
      }
      mLitObjects.clear();

      Canvas->paint();
      Sim::postEvent(this, new SceneLightingProcessEvent(light + 1, -1), Sim::getTargetTime() + 1);
   }
   else
   {
      // done lighting?
      if(light == mLights.size())
      {
         Con::printf(" Scene lit in %3.3f seconds", (Platform::getRealMilliseconds()-mStartTime)/1000.f);

         // save out the lighting?
         if(Con::getBoolVariable("$pref::sceneLighting::cacheLighting", true))
         {
            if(!savePersistInfo(mFileName))
               Con::errorf(ConsoleLogEntry::General, "SceneLighting::light: unable to persist lighting!");
            else
               Con::printf(" Successfully saved mission lighting file: '%s'", mFileName);
         }

         // wrap things up...
         completed(true);
         deleteObject();
      }
      else
      {
         // start of this light?
         if(object == -1)
         {
            for(proxyItr = mSceneObjects.begin(); proxyItr != mSceneObjects.end(); proxyItr++)
            {
               if(!(*proxyItr)->getObject())
               {
                  AssertFatal(0, "SceneLighting:: missing sceneobject on light start");
                  Con::errorf(ConsoleLogEntry::General, "SceneLighting:: missing sceneobject on light start");
                  continue;
               }
               if((*proxyItr)->preLight(mLights[light]))
                  mLitObjects.push_back(*proxyItr);
            }
         }
         else
         {
            if(mLitObjects[object]->getObject())
            {
               gLightingProgress = (F32(light) / F32(mLights.size())) + ((F32(object + 1) / F32(mLitObjects.size())) / F32(mLights.size()));
               mLitObjects[object]->light(mLights[light]);
            }
            else
            {
               AssertFatal(0, "SceneLighting:: missing sceneobject on light update");
               Con::errorf(ConsoleLogEntry::General, "SceneLighting:: missing sceneobject on light update");
            }
         }

         Canvas->paint();
         Sim::postEvent(this, new SceneLightingProcessEvent(light, object + 1), Sim::getTargetTime() + 1);
      }
   }
}

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Class SceneLighting::ObjectProxy:
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Class SceneLighting::InteriorProxy:
//------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
// Class SceneLighting::TerrainProxy:
//-------------------------------------------------------------------------------
*/
