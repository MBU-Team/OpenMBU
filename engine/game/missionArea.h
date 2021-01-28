//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MISSIONAREA_H_
#define _MISSIONAREA_H_

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif

class MissionArea : public NetObject
{
  private:
   typedef NetObject Parent;
   RectI             mArea;

   F32 mFlightCeiling;
   F32 mFlightCeilingRange;

  public:
   MissionArea();

   static RectI      smMissionArea;

   static const MissionArea * getServerObject();

   F32 getFlightCeiling()      const { return mFlightCeiling;      }
   F32 getFlightCeilingRange() const { return mFlightCeilingRange; }

   //
   const RectI & getArea(){return(mArea);}
   void setArea(const RectI & area);

   /// @name SimObject Inheritance
   /// @{
   bool onAdd();

   static void initPersistFields();
   /// @}

   /// @name NetObject Inheritance
   /// @{
   enum NetMaskBits {
      UpdateMask     = BIT(0)
   };

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);
   /// @}

   DECLARE_CONOBJECT(MissionArea);
};

#endif
