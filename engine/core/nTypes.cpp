//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"


//--------------------------------------
U32 getNextPow2(U32 io_num)
{
   S32 oneCount   = 0;
   S32 shiftCount = -1;
   while (io_num) {
      if(io_num & 1)
         oneCount++;
      shiftCount++;
      io_num >>= 1;
   }
   if(oneCount > 1)
      shiftCount++;

   return U32(1 << shiftCount);
}

//--------------------------------------
U32 getBinLog2(U32 io_num)
{
   AssertFatal(io_num != 0 && isPow2(io_num) == true,
               "Error, this only works on powers of 2 > 0");

   S32 shiftCount = 0;
   while (io_num) {
      shiftCount++;
      io_num >>= 1;
   }

   return U32(shiftCount - 1);
}

