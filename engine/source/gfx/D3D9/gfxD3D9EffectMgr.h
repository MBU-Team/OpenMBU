//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GFXD3D9EFFECTMGR_H_
#define _GFXD3D9EFFECTMGR_H_

#include "gfx/gfxEffectMgr.h"
#include "gfx/D3D9/gfxD3D9Effect.h"

//**************************************************************************
// Effect Manager
//**************************************************************************
class GFXD3D9EffectMgr : public GFXEffectMgr
{
public:
   virtual ~GFXD3D9EffectMgr();

   virtual GFXD3D9Effect* createEffect( const char* file, const char* poolName = NULL );

private:
   struct Pool
   {
      const char*          mName;
      LPD3DXEFFECTPOOL     mPool;
   };

   Vector< Pool > mPools;
};

#endif // _GFXD3D9EFFECTMGR_H_
