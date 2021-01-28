//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MISSIONMARKER_H_
#define _MISSIONMARKER_H_

#ifndef _BITSTREAM_H_
#include "core/bitStream.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif
#ifndef _MATHIO_H_
#include "math/mathIO.h"
#endif
#ifndef _SPHERE_H_
#include "game/sphere.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class MissionMarkerData : public ShapeBaseData
{
   private:
      typedef ShapeBaseData      Parent;
   public:
      DECLARE_CONOBJECT(MissionMarkerData);
};

//------------------------------------------------------------------------------
// Class: MissionMarker
//------------------------------------------------------------------------------
class MissionMarker : public ShapeBase
{
   private:
      typedef ShapeBase          Parent;

   protected:
      enum MaskBits {
         PositionMask = Parent::NextFreeMask,
         NextFreeMask = Parent::NextFreeMask << 1
      };
      MissionMarkerData *        mDataBlock;
      bool                       mAddedToScene;

   public:
      MissionMarker();

      // GameBase
      bool onNewDataBlock(GameBaseData *dptr);

      // SceneObject
      void setTransform(const MatrixF &mat);

      // SimObject
      bool onAdd();
      void onRemove();
      void onEditorEnable();
      void onEditorDisable();

      void inspectPostApply();

      // NetObject
      U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
      void unpackUpdate(NetConnection *conn,           BitStream *stream);

      DECLARE_CONOBJECT(MissionMarker);
      static void initPersistFields();
};

//------------------------------------------------------------------------------
// Class: WayPoint
//------------------------------------------------------------------------------
class WayPoint;
class WayPointTeam
{
   public:
      WayPointTeam();

      S32         mTeamId;
      WayPoint *  mWayPoint;
};
DefineConsoleType( TypeWayPointTeam )

class WayPoint : public MissionMarker
{
   private:
      typedef MissionMarker         Parent;

   public:
      enum WayPointMasks {
         UpdateNameMask    = Parent::NextFreeMask,
         UpdateTeamMask    = Parent::NextFreeMask << 1,
         UpdateHiddenMask  = Parent::NextFreeMask << 2,
         NextFreeMask      = Parent::NextFreeMask << 3
      };

      WayPoint();

      // ShapeBase: only ever added to scene if in the editor
      void setHidden(bool hidden);

      // SimObject
      bool onAdd();
      void inspectPostApply();

      // NetObject
      U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
      void unpackUpdate(NetConnection *conn, BitStream *stream);

      // field data
      StringTableEntry              mName;
      WayPointTeam                  mTeam;

      static void initPersistFields();

      DECLARE_CONOBJECT(WayPoint);
};

//------------------------------------------------------------------------------
// Class: SpawnSphere
//------------------------------------------------------------------------------

class SpawnSphere : public MissionMarker
{
   private:
      typedef MissionMarker         Parent;
      static Sphere                 smSphere;

   public:
      SpawnSphere();

      // SimObject
      bool onAdd();
      void inspectPostApply();

      // NetObject
      enum SpawnSphereMasks {
         UpdateSphereMask = Parent::NextFreeMask,
         NextFreeMask = Parent::NextFreeMask << 1
      };

      U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
      void unpackUpdate(NetConnection *conn,           BitStream *stream);

      // field data
      F32      mRadius;
      F32      mSphereWeight;
      F32      mIndoorWeight;
      F32      mOutdoorWeight;

      static void initPersistFields();

      DECLARE_CONOBJECT(SpawnSphere);
};

#endif
