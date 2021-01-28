//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_EDITFLOORPLANRES_
#define _H_EDITFLOORPLANRES_

#ifndef _FLOORPLANRES_H_
#include "interior/floorPlanRes.h"
#endif

class EditFloorPlanResource : public FloorPlanResource
{
   protected:
      Vector<Winding>      mWindings;
      Vector<U32>          mPlaneEQs;

      void  clearFloorPlan();

   public:
      // In Nav graph generation mode, windings are saved during the BSP process.
      void  pushArea(const Winding & winding, U32 planeEQ);

      // When done, this assembles the FloorPlanResource data.
      //    Uses gWorkingGeometry...
      void  constructFloorPlan();
};

#endif  // _H_EDITFLOORPLANRES_
