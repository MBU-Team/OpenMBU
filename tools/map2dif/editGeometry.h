//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EDITGEOMETRY_H_
#define _EDITGEOMETRY_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _MORIANBASICS_H_
#include "map2dif/morianBasics.h"
#endif
#ifndef _MORIANUTIL_H_
#include "map2dif/morianUtil.h"
#endif
#ifndef _BSPNODE_H_
#include "map2dif/bspNode.h"
#endif
#ifndef _MPLANE_H_
#include "math/mPlane.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _EDITFLOORPLANRES_H_
#include "map2dif/editFloorPlanRes.h"
#endif

#include "interior/interiorResObjects.h"

class  Tokenizer;
class  Interior;
class  CSGBrush;
class  GBitmap;
class  CSGPlane;
class  WorldSpawnEntity;
class  MirrorSurfaceEntity;
class  TriggerEntity;
class  DoorEntity;
class  ForceFieldEntity;
class  PathStartEntity;
class  SpecialNodeEntity;
class GameEntity;
class  InteriorResource;
struct TempHullReference;

//------------------------------------------------------------------------------
class EditGeometry
{
   friend class EditBSPNode;
   friend class Lighting;
   friend void parseBrushList(Vector<CSGBrush*>& brushes, Tokenizer* pToker);

   //-------------------------------------- first class structures
  public:
   struct Portal {
      U32  portalId;
      S32  planeEQIndex;
      S32  frontZone;
      S32  backZone;
      bool passAmbientLight;

      Vector<Winding> windings;

      Point3D mMin;
      Point3D mMax;

      Point3D x;
      Point3D y;
   };
   struct Zone {
      U32  zoneId;
      bool active;

      bool ambientLit;

      Vector<U32> referencingPortals;
   };
   struct Surface {
      bool     isMemberOfZone(U32);

      U32      uniqueKey;
      U32      planeIndex;
      U32      textureIndex;

      Winding  winding;
      Winding  originalWinding;

      U32      texGenIndex;

      U32      lMapDimX;
      U32      lMapDimY;
      U32      offsetX;
      U32      offsetY;
      bool     isInside;

      bool     lmapTexGenSwapped;
      F32      lmapTexGenX[4];
      F32      lmapTexGenY[4];
      F32      tempScale[2];

      GBitmap* pLMap;

      GBitmap* pLightDirMap;
      GBitmap* pNormalLMap;
      GBitmap* pAlarmLMap;
      U32      sheetIndex;
      U32      alarmSheetIndex;

      U32      flags;
      U32      fanMask;

      U32      numLights;
      U32      stateDataStart;

      bool     mustPortalExtend;
      U32      temptemptemp;

      MatrixF  texSpace;

	  U32 sgLightingScale;
	  ColorI sgSelfIllumination;
   };
   struct NullSurface {
      U32     planeIndex;
      Winding winding;
      U32     flags;
   };

   class Entity {
     public:
      Entity() { }
      virtual ~Entity() { };
      InteriorDict mDictionary;

      virtual bool      parseEntityDescription(Tokenizer* pToker)  = 0;
      virtual BrushType getBrushType() = 0;

      virtual bool           isPointClass() const = 0;
      virtual const char*    getName()   const    = 0;
      virtual const Point3D& getOrigin() const    = 0;

      virtual void grabSurface(Surface&) { }
   };

   struct PlaneHashEntry {
      U32          planeIndex;
      PlaneHashEntry* pNext;
   };
   struct PointHashEntry {
      U32          pointIndex;
      PointHashEntry* pNext;
   };
   struct TexGenPlanes {
      PlaneF   planeX;
      PlaneF   planeY;
   };


   //-------------------------------------- animated lighting structures
  public:
   struct StateData {
      GBitmap* pLMap;         // 8-Bit intensity map
      U32      surfaceIndex;  // Surface affected by this state

      U32      stateDataIndex;   // index
   };
   struct LightState {
      ColorI            color;
      F32               duration;
      Vector<StateData> stateData;
   };
   struct AnimatedLight {
      char* name;
      U32   type;    // From Interior::LightType
      bool  alarm;   // entity->mAlarmStatus != NormalOnly

      Vector<LightState*> states;

      AnimatedLight() : name(NULL), type(0) { }
   };

   Vector<AnimatedLight*> mAnimatedLights;

   //-------------------------------------- arenas
  public:
   class PlaneHashArena {
      Vector<PlaneHashEntry*> mBuffers;

      U32             arenaSize;
      PlaneHashEntry* currBuffer;
      U32             currPosition;

     public:
      PlaneHashArena(U32 _arenaSize);
      ~PlaneHashArena();

      PlaneHashEntry* allocateEntry();
   };
   class PointHashArena {
      Vector<PointHashEntry*> mBuffers;

      U32          arenaSize;
      PointHashEntry* currBuffer;
      U32          currPosition;

     public:
      PointHashArena(U32 _arenaSize);
      ~PointHashArena();

      PointHashEntry* allocateEntry();
   };

   //------------------------------------------------------------------------------
   //-------------------------------------- DATA
  private:
   //-------------------------------------- Error variables
   U32            mNumAmbiguousBrushes;
   U32            mNumOrphanPolys;

   //-------------------------------------- Brushes and texture vars
  public:
   Point3D           mMinBound;
   Point3D           mMaxBound;

  public:
   U32               mCurrBrushId;
   U32               mSurfaceKey;
   Vector<CSGBrush*> mStructuralBrushes;
   Vector<CSGBrush*> mDetailBrushes;

   Vector<CSGBrush*> mSpecialCollisionBrushes;

   Vector<CSGBrush*> mVehicleCollisionBrushes;
   Vector<NullSurface>        mVehicleNullSurfaces;

   Vector<CSGBrush*> mPortalBrushes;
   Vector<Entity*>   mPortalEntities;

   Vector<char*>     mTextureNames;
   Vector<GBitmap*>  mTextures;

   Vector<Portal*>   mPortals;
   Vector<Zone*>     mZones;
   U32               mOutsideZoneIndex;

   Vector<TexGenPlanes>       mTexGenEQs;
   Vector<Surface>            mSurfaces;
   Vector<NullSurface>        mNullSurfaces;
   Vector<GBitmap*>           mLightmaps;
   Vector<GBitmap*>           mLightDirMaps;

   bool                       mHasAlarmState;

   //-------------------------------------- Nav Graph Generation
   bool                       mPerformExtrusion;
   bool                       mGenerateGraph;
   bool                       mHashPlanes, mHashPoints;
   UniqueVector               mGraphSurfacePts;
   EditFloorPlanResource      mEditFloorPlanRes;

   //-------------------------------------- Planes and Points
   PlaneHashEntry   mPlaneHashTable[1 << 12];
   Vector<PlaneEQ>  mPlaneEQs;

   Vector<PlaneF>   mReflectPlanes;

   Vector<S32>      mPlaneRemaps;
   U16              remapPlaneIndex(S32 inPlaneIndex);
   U16              remapVehiclePlaneIndex(S32 inPlaneIndex, Interior*);

   PointHashEntry   mPointHashTable[1 << 12];
   Vector<Point3D>  mPoints;

   struct ExportPointMapEntry {
      U32 originalIndex;
      U32 runtimeIndex;
   };
   Vector<ExportPointMapEntry> mExportPointMap;
   Vector<ExportPointMapEntry> mVehicleExportPointMap;

  public:
   PlaneHashArena   mPlaneHashArena;
   PointHashArena   mPointHashArena;

   //-------------------------------------- BSP info
  public:
   NodeArena      mNodeArena;
   VisLinkArena   mVisLinkArena;

  private:
   EditBSPNode*  mBSPRoot;

   //-------------------------------------- Entity information
  public:
   WorldSpawnEntity* mWorldEntity;
   Vector<Entity*>   mEntities;

   //------------------------------------------------------------------------------
   //-------------------------------------- FUNCTIONS
  private:
   Entity*       parseEntity(Tokenizer* pToker);
   const Entity* getNamedEntity(const char*) const;

  public:
   const char* insertTexture(const char*);

   void fixSparkles();
   void convertToStrips();
   void markSurfaceOriginalPoints();
   void sortSurfaces();
   void sortLitSurfaces();

  private:
   void createBrushPolys();
   void buildBSP();
   void findOutsideZone();
   void enterPortalZoneRefs();
   void ambientVisitZone(const U32 zone);
   void floodAmbientLight();

  public:
   void preprocessLighting();
  private:
   void postProcessLighting(Interior*);
   U32  exportIntensityMap(Interior* pRuntime, GBitmap*);
   void exportLightsToRuntime(Interior* pRuntime);
   void exportDMLToRuntime(Interior* pRuntime, Vector<char*>&);
   U16  exportBSPToRuntime(Interior* pRuntime, EditBSPNode* pNode);
   void exportWindingToRuntime(Interior* pRuntime, const Winding& rWinding);
   void exportVehicleWindingToRuntime(Interior* pRuntime, const Winding& rWinding);
   U32  exportPointToRuntime(Interior* pRuntime, const U32 pointIndex);
   void exportHullToRuntime(Interior* pRuntime, CSGBrush* pBrush);
   U32  exportVehiclePointToRuntime(Interior* pRuntime, const U32 pointIndex);
   void exportVehicleHullToRuntime(Interior* pRuntime, CSGBrush* pBrush);
   void exportHullBins(Interior* pRuntime);
   void exportPlanes(Interior*);
   U32  exportEmitStringToRuntime(Interior* pRuntime,
                                  const U8* pString,
                                  const U32 stringLen);
   U32  exportVehicleEmitStringToRuntime(Interior* pRuntime,
                                         const U8* pString,
                                         const U32 stringLen);

   bool fixTJuncs();
   void rotateSurfaceToNonColinear(Surface& surface);
   void dumpSurfaceToRuntime(Interior* pRuntime,
                             Surface&  editSurface);
   void dumpNullSurfaceToRuntime(Interior* pRuntime,
                                 NullSurface& editSurface);
   void dumpVehicleNullSurfaceToRuntime(Interior* pRuntime,
                                        NullSurface& editSurface);

   void dumpTriggerToRuntime(Interior*         pRuntime,
                             InteriorResource* pResource,
                             TriggerEntity*    pEntity);
   void dumpPathToRuntime(Interior*         pRuntime,
                          InteriorResource* pResource,
                          PathStartEntity*    pEntity);
   void dumpAISpecialToRuntime(Interior*      pRuntime,
                               InteriorResource* pResource,
                               SpecialNodeEntity*    pEntity);
   void dumpGameEntityToRuntime(Interior*      pRuntime,
                               InteriorResource* pResource,
                               GameEntity*    pEntity);
   void dumpDoorToRuntime(Interior*         /*pRuntime*/,
                          InteriorResource* pResource,
                          DoorEntity*       pEntity);
   void dumpForceFieldToRuntime(Interior*         /*pRuntime*/,
                                InteriorResource* pResource,
                                ForceFieldEntity* pEntity);
   void fillInLightmapInfo(Surface& rSurface);
   void fillInTexSpaceMat(Surface& surface);


   void adjustLMapTexGen(Surface&);
   void createBrushSurfaces(const CSGBrush& brush);
   void createBoundingVolumes(Interior* pRuntime);

  public:
   EditGeometry();
   ~EditGeometry();

   //-------------------------------------- High level functions
  public:
   bool parseMapFile(Tokenizer*);
   bool createBSP();

   void createSurfaces();
   void markEmptyZones();
   void packLMaps();
   void computeLightmaps(const bool alarmMode);

   bool exportToRuntime(Interior*, InteriorResource*);

   U16  getMaterialIndex(const char* texName) const;

   //-------------------------------------- Point/Plane buffer functions
  public:
   U32         insertPlaneEQ(const Point3D& normal, const F64);
   const U32   getPlaneInverse(const U32 planeIndex);
   const PlaneEQ& getPlaneEQ(const U32) const;
   bool           isCoplanar(const U32, const U32) const;
   U32         insertPoint(const Point3D&);
   const Point3D& getPoint(const U32) const;


   void giftWrapPortal(Winding& winding, Portal* portal);

   //-------------------------------------- NavGraph functions
  public:
   void  setGraphGeneration(bool generate=false,bool extrude=false);
   void  gatherLinksForGraph(EditBSPNode * pLeafNode);
   U32   writeGraphInfo();
   void  doGraphExtrusions(Vector<CSGBrush*> & brushList);
   void  xferDetailToStructural();

   //-------------------------------------- Statistics
  public:
   U32 getTotalNumBrushes() const;
   U32 getNumStructuralBrushes() const;
   U32 getNumDetailBrushes() const;
   U32 getNumPortalBrushes() const;
   U32 getNumAmbiguousBrushes() const;
   U32 getNumOrphanPolys() const;
   U32 getNumUniquePlanes() const;
   U32 getNumUniquePoints() const;
   U32 getNumZones() const;
   U32 getNumSurfaces() const;

   const Point3D& getMinBound() const;
   const Point3D& getMaxBound() const;
};

extern EditGeometry* gWorkingGeometry;


inline U32 EditGeometry::getNumStructuralBrushes() const
{
   return mStructuralBrushes.size();
}

inline U32 EditGeometry::getNumDetailBrushes() const
{
   return mDetailBrushes.size();
}

inline U32 EditGeometry::getNumPortalBrushes() const
{
   return mPortalBrushes.size();
}

inline U32 EditGeometry::getNumAmbiguousBrushes() const
{
   return mNumAmbiguousBrushes;
}

inline U32 EditGeometry::getNumOrphanPolys() const
{
   return mNumOrphanPolys;
}

inline U32 EditGeometry::getTotalNumBrushes() const
{
   return getNumStructuralBrushes() + getNumDetailBrushes() + getNumPortalBrushes();
}

inline U32 EditGeometry::getNumUniquePlanes() const
{
   return mPlaneEQs.size() / 2;
}

inline U32 EditGeometry::getNumUniquePoints() const
{
   return mPoints.size();
}

inline U32 EditGeometry::getNumZones() const
{
   return mZones.size();
}

inline U32 EditGeometry::getNumSurfaces() const
{
   return mSurfaces.size();
}

inline const Point3D& EditGeometry::getMinBound() const
{
   return mMinBound;
}

inline const Point3D& EditGeometry::getMaxBound() const
{
   return mMaxBound;
}

//------------------------------------------------------------------------------
inline const PlaneEQ& EditGeometry::getPlaneEQ(const U32 planeIndex) const
{
   AssertFatal(planeIndex < mPlaneEQs.size(), "EditGeometry::getPlaneEQ: planeIndex out of range");

   return mPlaneEQs[planeIndex];
}

inline bool EditGeometry::isCoplanar(const U32 planeIndex1, const U32 planeIndex2) const
{
   AssertFatal(planeIndex1 < mPlaneEQs.size() && planeIndex2 < mPlaneEQs.size(),
               "EditGeometry::isCoplanar: planeIndex out of range");

   return (planeIndex1 & ~1) == (planeIndex2 & ~1);
}

inline const Point3D& EditGeometry::getPoint(const U32 index) const
{
   AssertFatal(index < mPoints.size(), "EditGeometry::getPoint: out of bounds point index");

   return mPoints[index];
}

inline const U32 EditGeometry::getPlaneInverse(const U32 planeIndex)
{
   return (planeIndex ^ 0x1);
}


//------------------------------------------------------------------------------
inline EditGeometry::PlaneHashEntry* EditGeometry::PlaneHashArena::allocateEntry()
{
   if (currPosition < arenaSize)
      return &currBuffer[currPosition++];

   // Need to add another buffer
   currBuffer   = new EditGeometry::PlaneHashEntry[arenaSize];
   currPosition = 1;
   mBuffers.push_back(currBuffer);

   return currBuffer;
}

inline EditGeometry::PointHashEntry* EditGeometry::PointHashArena::allocateEntry()
{
   if (currPosition < arenaSize)
      return &currBuffer[currPosition++];

   // Need to add another buffer
   currBuffer   = new EditGeometry::PointHashEntry[arenaSize];
   currPosition = 1;
   mBuffers.push_back(currBuffer);

   return currBuffer;
}

//------------------------------------------------------------------------------
inline F64 PlaneEQ::distanceToPlane(const Point3D& rPoint) const
{
   return mDot(rPoint, normal) + dist;
}

inline PlaneSide PlaneEQ::whichSide(const Point3D& testPoint) const
{
   F64 distance = distanceToPlane(testPoint);

   if (distance < -gcPlaneDistanceEpsilon)
      return PlaneBack;
   else if (distance > gcPlaneDistanceEpsilon)
      return PlaneFront;
   else
      return PlaneOn;
}

inline PlaneSide PlaneEQ::whichSidePerfect(const Point3D& testPoint) const
{
   F64 distance = distanceToPlane(testPoint);

   if (distance < 0)
      return PlaneBack;
   else if (distance > 0)
      return PlaneFront;
   else
      return PlaneOn;
}

#endif //_EDITGEOMETRY_H_
