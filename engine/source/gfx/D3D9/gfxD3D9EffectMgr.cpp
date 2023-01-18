//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9EffectMgr.h"
#include "gfx/D3D9/gfxD3D9Device.h"
#include "util/safeDelete.h"

GFXD3D9EffectMgr::~GFXD3D9EffectMgr()
{
   for( U32 i = 0; i < mPools.size(); ++ i )
      SAFE_RELEASE( mPools[ i ].mPool );
}

GFXD3D9Effect* GFXD3D9EffectMgr::createEffect( const char* file, const char* poolName )
{
   GFXD3D9Effect* effect = new GFXD3D9Effect;
   effect->registerResourceWithDevice( GFX );

   LPD3DXEFFECTPOOL pool = NULL;
   if( poolName )
   {
      U32 numPools = mPools.size();
      for( U32 i = 0; i < numPools; ++ i )
         if( dStrcmp( mPools[ i ].mName, poolName ) == 0 )
         {
            pool = mPools[ i ].mPool;
            break;
         }

      if( !pool )
      {
         HRESULT res = GFXD3DX.D3DXCreateEffectPool( &pool );
         if( res != S_OK )
         {
            Con::errorf( "GFXD3DXEffectMgr::createEffect - could not create effect pool '%s'", poolName );
            return NULL;
         }

         Pool poolInfo;
         poolInfo.mName = poolName;
         poolInfo.mPool = pool;

         mPools.push_back( poolInfo );
      }
   }

   if( effect->init( file, pool ) )
   {
      mEffects.push_back( effect );
      return effect;
   }
   else
   {
      SAFE_DELETE( effect );
      return NULL;
   }
}
