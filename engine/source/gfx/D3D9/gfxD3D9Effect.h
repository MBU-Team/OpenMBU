//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXD3D9EFFECT_H_
#define _GFXD3D9EFFECT_H_

#include "gfx/D3D9/platformD3D.h"
#include "gfx/gfxEffect.h"
#include "core/tVector.h"

class GFXD3D9Effect : public GFXEffect
{
public:
   GFXD3D9Effect();
   virtual ~GFXD3D9Effect();

   struct D3D9Parameter : public Parameter
   {
   };

   struct D3D9Technique : public Technique
   {
   };

   struct D3D9Pass : public Pass
   {
   };

   virtual U32 getNumParameters() { return mParameters.size(); }
   virtual U32 getNumTechniques() { return mTechniques.size(); }
   virtual U32 getNumPasses() { return mPasses.size(); }

   virtual U32 begin( bool dontSaveState = false );
   virtual void end();
   virtual void beginPass( U32 n );
   virtual void endPass();
   virtual void commit();

   virtual void setTechnique( const char* name );

   virtual void setBool( HANDLE parameter, bool value );
   virtual void setFloat( HANDLE parameter, F32 value );
   virtual void setFloatArray( HANDLE parameter, F32* values, U32 count );
   virtual void setInt( HANDLE parameter, S32 value );
   virtual void setVector( HANDLE parameter, const Point4F& vector );
   virtual void setVectorArray( HANDLE parameter, const Point4F* vectors, U32 count );
   virtual void setTexture( HANDLE parameter, GFXTexHandle texture );
   virtual void setMatrix( HANDLE parameter, const MatrixF& matrix );

   virtual HANDLE getParameterElement( HANDLE parameter, U32 index );

   bool init( const char* file, LPD3DXEFFECTPOOL pool );

private:
   LPD3DXEFFECT mEffect;

   Vector< D3D9Parameter > mParameters;
   Vector< D3D9Technique > mTechniques;
   Vector< D3D9Pass > mPasses;
};

#endif // _GFXD3D9EFFECT_H_
