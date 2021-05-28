//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/fx/underLava.h"

#include "dgl/dgl.h"
#include "math/mRect.h"
#include "dgl/gTexManager.h"
#include "terrain/waterBlock.h"
#include "math/mConstants.h"

UnderLavaFX gLavaFX;


//**************************************************************************
// Under lava FX  - "Lava - With pumice!"
//**************************************************************************
UnderLavaFX::UnderLavaFX()
{

}

//--------------------------------------------------------------------------
// Init
//--------------------------------------------------------------------------
void UnderLavaFX::init()
{
   RectI viewport;
   dglGetViewport( &viewport );

   mViewSize = viewport.extent;

   mTexFrequency.x = F32(viewport.extent.x / viewport.extent.y);
   mTexFrequency.y = 1.0;

   mNumPoints.x = (S32)(50 * F32(viewport.extent.x / viewport.extent.y));
   mNumPoints.y = 50;

   mWave[0].amplitude = 0.02;
   mWave[0].frequency = 2.0;
   mWave[0].velocity = Sim::getCurrentTime() / 1000.0 * 2.0;

   mMoveSpeed = Sim::getCurrentTime() / 1000.0 * 0.025;
}

//--------------------------------------------------------------------------
// Render
//--------------------------------------------------------------------------
void UnderLavaFX::render()
{
   init();

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();

   glEnable(GL_TEXTURE_2D);
//   TextureHandle lavaTex = WaterBlock::getSubmergeTexture(0);
   glBindTexture(GL_TEXTURE_2D, WaterBlock::getSubmergeTexture(0).getGLName());

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_FALSE);
   glColor4f(1.0, 1.0, 1.0, 0.5);

   if( WaterBlock::getSubmergeTexture(0) )
   {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      // render layer 1
      for( U32 i=0; i<mNumPoints.y; i++ )
      {
         renderRow( i, mNumPoints.y-1, mNumPoints.x-1 );
      }
   }

   // give second layer a different phase
   mWave[0].velocity += 10.0;

//   lavaTex = WaterBlock::getSubmergeTexture(1);

   if( WaterBlock::getSubmergeTexture(1) )
   {
      glBindTexture(GL_TEXTURE_2D, WaterBlock::getSubmergeTexture(1).getGLName());

      // render layer 2
      for( U32 i=0; i<mNumPoints.y; i++ )
      {
         renderRow( i, mNumPoints.y-1, mNumPoints.x-1 );
      }
   }

   glDepthMask(GL_TRUE);
   glDisable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glDisable(GL_BLEND);

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}


//--------------------------------------------------------------------------
// Render row (triangle strip)
//--------------------------------------------------------------------------
void UnderLavaFX::renderRow( U32 row, U32 numRows, U32 numColumns )
{

   F32 xMult = F32(2.0) / F32(numColumns);
   F32 yMult = F32(2.0) / F32(numRows);

   glBegin( GL_TRIANGLE_STRIP );

      for( U32 i=0; i<numColumns+1; i++ )
      {
         F32 u = F32(i) / F32(numColumns) * mTexFrequency.x;
         F32 v = F32(row) / F32(numRows) * mTexFrequency.y;

         u += mMoveSpeed + mWave[0].amplitude * mSin( u * M_2PI * mWave[0].frequency + mWave[0].velocity );
         v += mMoveSpeed + mWave[0].amplitude * mSin( v * M_2PI * mWave[0].frequency + mWave[0].velocity );

         glTexCoord2f( u, v );
         glVertex2f( -1.0 + i * xMult, -1.0 + row * yMult );

         v = F32(row+1) / F32(numRows) * mTexFrequency.y;
         v += mMoveSpeed + mWave[0].amplitude * mSin( v * M_2PI * mWave[0].frequency + mWave[0].velocity );

         glTexCoord2f( u, v );
         glVertex2f( -1.0 + i * xMult, -1.0 + (row+1) * yMult );

      }

   glEnd();

}
