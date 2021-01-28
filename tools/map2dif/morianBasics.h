//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MORIANBASICS_H_
#define _MORIANBASICS_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

static const F64 gcPlaneDistanceEpsilon = 0.00001;
static const U32 MaxWindingPoints       = 32;
static const U32 MaxWindingNodes        = 96;

enum BrushType {
   StructuralBrush       = 0,
   PortalBrush,
   DetailBrush,
   CollisionBrush,
   VehicleCollisionBrush,
   LightBrush,

};

enum PlaneSide {
   PlaneFront  = 1,
   PlaneOn     = 0,
   PlaneBack   = -1
};

enum SpecialPlaneSide {
   PlaneSpecialFront = 1 << 0,
   PlaneSpecialOn    = 1 << 1,
   PlaneSpecialBack  = 1 << 2
};

class UniqueVector : public Vector<U32>
{
  public:
   UniqueVector(U32 size) : Vector<U32>(size) { }
   UniqueVector() : Vector<U32>(32) { }

   void pushBackUnique(U32);
};

struct PlaneEQ
{
   Point3D normal;
   F64  dist;

   PlaneSide whichSide(const Point3D&) const;
   PlaneSide whichSidePerfect(const Point3D&) const;
   F64    distanceToPlane(const Point3D&) const;
   void neg() { normal.neg(); dist = -dist; }
};

struct Winding {
   U32 numIndices;
   U32 numNodes;
   U32 numZoneIds;
   U32 indices[MaxWindingPoints];
   U32 solidNodes[MaxWindingNodes];
   U32 zoneIds[8];
   U32 brushId;

   Winding* pNext;

   Winding() : numNodes(0), numZoneIds(0) { }
   void insertZone(U32 zone)
   {
      // FIXME: lame sort to make sure the zones stay sorted
      for (U32 i = 0; i < numZoneIds; i++)
         if (zoneIds[i] == zone)
            return;

      AssertFatal(numZoneIds < 8, "Error, too many zones on a surface.  Increase count from 8");
      zoneIds[numZoneIds++] = zone;
      extern int FN_CDECL cmpZoneNum(const void*, const void*);
      dQsort(zoneIds, numZoneIds, sizeof(U32), cmpZoneNum);
   }
};

//------------------------------------------------------------------------------
//-------------------------------------- Inlines
//
inline void UniqueVector::pushBackUnique(U32 newElem)
{
   for (U32 i = 0; i < size(); i++)
      if (operator[](i) == newElem)
         return;

   push_back(newElem);
}

#endif //_MORIANBASICS_H_
