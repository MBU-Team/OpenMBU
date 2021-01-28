//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXInit_H_
#define _GFXInit_H_

#include "core/tVector.h"
#include "gfx/gfxDevice.h"

#define MAX_ADAPTER_NAME_LEN 512 // This seems pretty generous


struct GFXAdapter 
{
   char name[MAX_ADAPTER_NAME_LEN];

   GFXAdapterType type;
   U32 index;
};

// Implement this class per platform (This is just a class so it can be friend with GFXDevice
class GFXInit 
{
private:
   ///Hold the adapters, primarily for device switching
   static Vector<GFXAdapter> smAdapters;

public:
   /// Enumerate all the graphics adapters on the system
   /// @param   out   Vector to store results in
   static void enumerateAdapters();

   /// Creates a GFXDevice based on an adapter from the
   /// enumerateAdapters method and it can be retrieved
   /// by calling GFXDevice::get(). This method will fail
   /// if a GFXDevice exists already.
   /// @param   adapter   Graphics adapter to create device
   static void createDevice( GFXAdapter adapter );

   /// Get the enumerated adapters.  Should only call this after
   /// a call to enumerateAdapters.
   static void getAdapters( Vector<GFXAdapter> *adapters );
};

#endif
