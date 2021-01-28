//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UNDERLAVA_H_
#define _UNDERLAVA_H_

#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

//**************************************************************************
//  Data
//**************************************************************************
struct LavaVertex
{
   Point2I pnt;
   Point2F texPnt;

};

struct LavaWave
{
   F32   frequency;
   F32   amplitude;
   F32   velocity;

};


//**************************************************************************
// Under lava FX
//**************************************************************************
class UnderLavaFX
{
  private:
   Point2F     mTexFrequency;
   Point2I     mNumPoints;
   Point2I     mViewSize;
   LavaWave    mWave[2];
   F32         mMoveSpeed;

   void renderRow( U32 row, U32 numRows, U32 numColumns );

  public:
   UnderLavaFX();

   void init();
   void render();

};

extern UnderLavaFX gLavaFX;

#endif
