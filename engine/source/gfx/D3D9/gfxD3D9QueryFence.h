//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFX_D3D9_QUERYFENCE_H_
#define _GFX_D3D9_QUERYFENCE_H_

#include "gfx/gfxFence.h"
#include "gfx/gfxResource.h"

struct IDirect3DQuery9;

class GFXD3D9QueryFence : public GFXFence
{
private:
   mutable IDirect3DQuery9 *mQuery;

public:
   GFXD3D9QueryFence( GFXDevice *device ) : GFXFence( device ), mQuery( NULL ) {};
   virtual ~GFXD3D9QueryFence();

   virtual void issue();
   virtual FenceStatus getStatus() const;
   virtual void block();

   void describeSelf( char* buffer, U32 sizeOfBuffer);

   // GFXResource interface
   virtual void zombify();
   virtual void resurrect();
};

#endif