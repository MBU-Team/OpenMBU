//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXEFFECTMGR_H_
#define _GFXEFFECTMGR_H_

#include "core/tVector.h"

class GFXEffect;

//**************************************************************************
// Effect Manager
//**************************************************************************
class GFXEffectMgr
{
public:
   virtual ~GFXEffectMgr() {}

   /// Create a new effect instance from the given effect source file.
   virtual GFXEffect* createEffect( const char* file, const char* poolName = NULL ) = 0;

   virtual void shutdown();

   virtual void destroyEffect( GFXEffect* effect );

protected:
   Vector< GFXEffect* > mEffects;
};

#endif // _GFXEFFECTMGR_H_
