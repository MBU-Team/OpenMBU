//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXEFFECT_H_
#define _GFXEFFECT_H_

#include "gfx/gfxResource.h"
#include "gfx/gfxTextureHandle.h"
#include "math/mPoint.h"
#include "math/mMatrix.h"

//**************************************************************************
// Effect
//**************************************************************************
class GFXEffect : public GFXResource
{
public:
   virtual ~GFXEffect() {}

   virtual void zombify() {}
   virtual void resurrect() {}
   virtual void describeSelf( char* buffer, U32 sizeOfBuffer )
   {
      buffer[ 0 ] = NULL;
   }

   /// @name Rendering
   /// @{

   virtual U32 begin( bool dontSaveState = false ) = 0;
   virtual void end() = 0;
   virtual void beginPass( U32 n ) = 0;
   virtual void endPass() = 0;
   virtual void commit() = 0;

   /// @}

   /// @name Techniques
   /// @{

   /// A technique descriptor.
   struct Technique
   {
      const char* mName;
   };

   virtual void setTechnique( const char* name ) = 0;

#if 0
   virtual U32 getNumTechniques() = 0;

   virtual Technique* getTechnique( U32 index ) = 0;
#endif

   /// @}

   /// @name Parameters
   /// @{

   /// A parameter descriptor.
   struct Parameter
   {
      const char* mName;
      U32 mType;
   };

   typedef const void* HANDLE;

   virtual void setBool( HANDLE parameter, bool value ) = 0;
   virtual void setFloat( HANDLE parameter, F32 value ) = 0;
   virtual void setFloatArray( HANDLE parameter, F32* values, U32 count ) = 0;
   virtual void setInt( HANDLE parameter, int value ) = 0;
   virtual void setVector( HANDLE parameter, const Point4F& vector ) = 0;
   virtual void setVectorArray( HANDLE parameter, const Point4F* vectors, U32 count ) = 0;
   virtual void setTexture( HANDLE parameter, GFXTexHandle texture ) = 0;
   virtual void setMatrix( HANDLE parameter, const MatrixF& matrix ) = 0;

   virtual HANDLE getParameterElement( HANDLE parameter, U32 index ) = 0;

#if 0
   virtual void setParameter( U32 index, void* data ) = 0;

   virtual U32 getNumParameters() = 0;
#endif

   /// @}

   /// @name Passes
   /// @{

   /// A pass descriptor.
   struct Pass
   {
      const char* mName;
   };

   /// @}
};

#endif // _GFXEFFECT_H_
