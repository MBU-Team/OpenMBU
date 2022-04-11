//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxEffectMgr.h"
#include "gfx/gfxEffect.h"
#include "util/safeDelete.h"

void GFXEffectMgr::shutdown()
{
   for( U32 i = 0; i < mEffects.size(); ++ i )
      SAFE_DELETE( mEffects[ i ] );
   mEffects.clear();
}

void GFXEffectMgr::destroyEffect( GFXEffect *effect )
{
   for( U32 i = 0; i < mEffects.size(); ++ i )
      if( mEffects[ i ] == effect )
      {
         mEffects.erase( i );
         break;
      }

   SAFE_DELETE( effect );
}
