//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXD3DCARDPROFILER_H_
#define _GFXD3DCARDPROFILER_H_

#include "gfx/gfxCardProfile.h"

class GFXD3DCardProfiler : public GFXCardProfiler
{
private:
   typedef GFXCardProfiler Parent;

   GFXD3DDevice *mDevice;

   LPDIRECT3DDEVICE9 mD3DDevice;
   UINT mAdapterOrdinal;

   char * mVersionString;
   const char * mVendorString;
   char * mCardString;

public:
   GFXD3DCardProfiler();
   ~GFXD3DCardProfiler();
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
