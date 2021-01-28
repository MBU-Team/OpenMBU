//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "Platform/platform.h"
#include "console/console.h"
#include "Math/mMath.h"
#include "terrain/blender.h"

extern void mInstallLibrary_C();
extern void mInstallLibrary_Vec();


//--------------------------------------
ConsoleFunction( MathInit, void, 1, 10, "(DETECT|C|VEC)")
{
   U32 properties = CPU_PROP_C;  // C entensions are always used
   
   if (argc == 1)
   {
         Math::init(0);
         return;
   }
   for (argc--, argv++; argc; argc--, argv++)
   {
      if (dStricmp(*argv, "DETECT") == 0) { 
         Math::init(0);
         return;
      }
      if (dStricmp(*argv, "C") == 0) { 
         properties |= CPU_PROP_C; 
         continue; 
      }
      if (dStricmp(*argv, "VEC") == 0) { 
         properties |= CPU_PROP_ALTIVEC; 
         continue; 
      }
      Con::printf("Error: MathInit(): ignoring unknown math extension '%s'", *argv);
   }
   Math::init(properties);
}



//------------------------------------------------------------------------------
void Math::init(U32 properties)
{
   if (!properties)
      // detect what's available
      properties = Platform::SystemInfo.processor.properties;  
   else
      // Make sure we're not asking for anything that's not supported
      properties &= Platform::SystemInfo.processor.properties;  

   Con::printf("Math Init:");
   Con::printf("   Installing Standard C extensions");
   mInstallLibrary_C();
   
   #if defined(__VEC__)
   Blender::smUseVecBlender = false;
   if (properties & CPU_PROP_ALTIVEC)
   {
      Con::printf("   Installing Altivec extensions");
      mInstallLibrary_Vec();
      Blender::smUseVecBlender = true;
   }
   #endif
   Con::printf(" ");
}   


