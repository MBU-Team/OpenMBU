//-----------------------------------------------------------------------------
// Torque Game Engine Advanced 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXPCD3D9CARDPROFILER_H_
#define _GFXPCD3D9CARDPROFILER_H_

#include "gfx/gfxCardProfile.h"

class GFXPCD3D9CardProfiler : public GFXCardProfiler
{
private:
   typedef GFXCardProfiler Parent;

   LPDIRECT3DDEVICE9 mD3DDevice;
   UINT mAdapterOrdinal;

   char *mVersionString;
   const char *mVendorString;
   char *mCardString;

public:
   GFXPCD3D9CardProfiler();
   ~GFXPCD3D9CardProfiler();
   void init();

protected:
   const char* getVersionString();
   const char* getCardString();
   const char* getVendorString();
   const char* getRendererString();
   void setupCardCapabilities();
   bool _queryCardCap(const char *query, U32 &foundResult);
};

#endif
