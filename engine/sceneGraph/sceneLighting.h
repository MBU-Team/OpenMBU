//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SceneLighting_H_
#define _SceneLighting_H_

#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#ifndef _INTERIORINSTANCE_H_
#include "interior/interiorInstance.h"
#endif
#ifndef _SHADOWVOLUMEBSP_H_
#include "sceneGraph/shadowVolumeBSP.h"
#endif


#include "lightingSystem/sgSceneLighting.h"


/*
#define TERRAIN_OVERRANGE 2.0f

//------------------------------------------------------------------------------
class SceneLighting : public SimObject
{
   typedef SimObject Parent;
   public:

      struct PersistInfo
      {
         struct PersistChunk
         {
            enum {
               MissionChunkType = 0,
               InteriorChunkType,
               TerrainChunkType
            };

            U32            mChunkType;
            U32            mChunkCRC;

            virtual ~PersistChunk() {}

            virtual bool read(Stream &);
            virtual bool write(Stream &);
         };

         struct MissionChunk : public PersistChunk
         {
            typedef PersistChunk Parent;
            MissionChunk();
         };

         struct InteriorChunk : public PersistChunk
         {
            typedef PersistChunk Parent;

            InteriorChunk();
            ~InteriorChunk();

            Vector<U32>          mDetailLightmapCount;
            Vector<U32>          mDetailLightmapIndices;
            Vector<GBitmap*>     mLightmaps;

            bool                 mHasAlarmState;
            Vector<U32>          mDetailVertexCount;
            Vector<ColorI>       mVertexColorsNormal;
            Vector<ColorI>       mVertexColorsAlarm;

            bool read(Stream &);
            bool write(Stream &);
         };

         struct TerrainChunk : public PersistChunk
         {
            typedef PersistChunk Parent;

            TerrainChunk();
            ~TerrainChunk();

            GBitmap *mLightmap;

            bool read(Stream &);
            bool write(Stream &);
         };

         ~PersistInfo();

         Vector<PersistChunk*>      mChunks;
         static U32                 smFileVersion;

         bool read(Stream &);
         bool write(Stream &);
      };

      U32 calcMissionCRC();

      bool verifyMissionInfo(PersistInfo::PersistChunk *);
      bool getMissionInfo(PersistInfo::PersistChunk *);

      bool loadPersistInfo(const char *);
      bool savePersistInfo(const char *);

      class ObjectProxy;
      class TerrainProxy;
      class InteriorProxy;

      enum {
         SHADOW_DETAIL = -1
      };

      void addInterior(ShadowVolumeBSP *, InteriorProxy &, LightInfo *, S32);
      ShadowVolumeBSP::SVPoly * buildInteriorPoly(ShadowVolumeBSP *, InteriorProxy &, Interior *, U32, LightInfo *, bool);

      //------------------------------------------------------------------------------
      /// Create a proxy for each object to store data.
      class ObjectProxy
      {
         public:
         SimObjectPtr<SceneObject>     mObj;
         U32                           mChunkCRC;

         ObjectProxy(SceneObject * obj) : mObj(obj){mChunkCRC = 0;}
         virtual ~ObjectProxy(){}
         SceneObject * operator->() {return(mObj);}
         SceneObject * getObject() {return(mObj);}

         /// @name Lighting Interface
         /// @{
         virtual bool loadResources() {return(true);}
         virtual void init() {}
         virtual bool preLight(LightInfo *) {return(false);}
         virtual void light(LightInfo *) {}
         virtual void postLight(bool lastLight) {}
         /// @}

         /// @name Persistence
         ///
         /// We cache lighting information to cut down on load times.
         ///
         /// There are flags such as ForceAlways and LoadOnly which allow you
         /// to control this behaviour.
         /// @{
         bool calcValidation();
         bool isValidChunk(PersistInfo::PersistChunk *);

         virtual U32 getResourceCRC() = 0;
         virtual bool setPersistInfo(PersistInfo::PersistChunk *);
         virtual bool getPersistInfo(PersistInfo::PersistChunk *);
         /// @}
      };

      class InteriorProxy : public ObjectProxy
      {
         private:
            typedef  ObjectProxy       Parent;
            bool isShadowedBy(InteriorProxy *);

         public:

            InteriorProxy(SceneObject * obj);
            ~InteriorProxy();
            InteriorInstance * operator->() {return(static_cast<InteriorInstance*>(static_cast<SceneObject*>(mObj)));}
            InteriorInstance * getObject() {return(static_cast<InteriorInstance*>(static_cast<SceneObject*>(mObj)));}

            // current light info
            ShadowVolumeBSP *                   mBoxShadowBSP;
            Vector<ShadowVolumeBSP::SVPoly*>    mLitBoxSurfaces;
            Vector<PlaneF>                      mOppositeBoxPlanes;
            Vector<PlaneF>                      mTerrainTestPlanes;

            // lighting interface
            bool loadResources();
            void init();
            bool preLight(LightInfo *);
            void light(LightInfo *);
            void postLight(bool lastLight);

            // persist
            U32 getResourceCRC();
            bool setPersistInfo(PersistInfo::PersistChunk *);
            bool getPersistInfo(PersistInfo::PersistChunk *);
      };

      class TerrainProxy : public ObjectProxy
      {
         private:
            typedef  ObjectProxy    Parent;

            BitVector               mShadowMask;
            ShadowVolumeBSP *       mShadowVolume;
            ColorF *                mLightmap;

            void lightVector(LightInfo *);

            struct SquareStackNode
            {
               U8          mLevel;
               U16         mClipFlags;
               Point2I     mPos;
            };

            S32 testSquare(const Point3F &, const Point3F &, S32, F32, const Vector<PlaneF> &);
            bool markInteriorShadow(InteriorProxy *);

         public:

            TerrainProxy(SceneObject * obj);
            ~TerrainProxy();
            TerrainBlock * operator->() {return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj)));}
            TerrainBlock * getObject() {return(static_cast<TerrainBlock*>(static_cast<SceneObject*>(mObj)));}

            bool getShadowedSquares(const Vector<PlaneF> &, Vector<U16> &);

            // lighting
            void init();
            bool preLight(LightInfo *);
            void light(LightInfo *);

            // persist
            U32 getResourceCRC();
            bool setPersistInfo(PersistInfo::PersistChunk *);
            bool getPersistInfo(PersistInfo::PersistChunk *);
      };

      typedef Vector<ObjectProxy*>  ObjectProxyList;

      ObjectProxyList            mSceneObjects;
      ObjectProxyList            mLitObjects;

      LightInfoList              mLights;

      SceneLighting();
      ~SceneLighting();

      enum Flags {
         ForceAlways    = BIT(0),   ///< Regenerate the scene lighting no matter what.
         ForceWritable  = BIT(1),   ///< Regenerate the scene lighting only if we can write to the lighting cache files.
         LoadOnly       = BIT(2),   ///< Just load cached lighting data.
      };
      static bool lightScene(const char *, BitSet32 flags = 0);
      static bool isLighting();

      S32                        mStartTime;
      char                       mFileName[1024];
      static bool                smUseVertexLighting;

      bool light(BitSet32);
      void completed(bool success);
      void processEvent(U32 light, S32 object);
      void processCache();

      // inlined
      bool isTerrain(SceneObject *);
      bool isInterior(SceneObject *);
};



//------------------------------------------------------------------------------

inline bool SceneLighting::isTerrain(SceneObject * obj)
{
   return obj && ((obj->getTypeMask() & TerrainObjectType) != 0);
}

inline bool SceneLighting::isInterior(SceneObject * obj)
{
   return obj && ((obj->getTypeMask() & InteriorObjectType) != 0);
}
*/
#endif
