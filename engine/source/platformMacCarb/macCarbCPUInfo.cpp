//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "PlatformMacCarb/platformMacCarb.h"
#include "console/console.h"
#include "Core/stringTable.h"
#include <math.h>


/* at some future time...
#if (UNIVERSAL_INTERFACES_VERSION<0x0341) || (UNIVERSAL_INTERFACES_VERSION==0x0341 && UNIVERSAL_INTERFACES_SEED_VERSION<2) || (UNIVERSAL_INTERFACES_VERSION==0x0400) // since OSX base headers are earlier than 3.4.1
// missing the new cpu type until carbonlib 1.5...
#define  gestaltCPUG47450              0x0110
#endif
*/

// possibly useful gestalts I haven't used yet:

//    gestaltDisplayMgrVers       = FOUR_CHAR_CODE('dplv')        /* Display Manager version */
//    gestaltDisplayMgrAttr       = FOUR_CHAR_CODE('dply'),       /* Display Manager attributes */

//    gestaltOSAttr               = FOUR_CHAR_CODE('os  '),       /* o/s attributes */
//    gestaltSysZoneGrowable      = 0,                            /* system heap is growable */
//    gestaltLaunchCanReturn      = 1,                            /* can return from launch */
//    gestaltLaunchFullFileSpec   = 2,                            /* can launch from full file spec */
//    gestaltLaunchControl        = 3,                            /* launch control support available */
//    gestaltTempMemSupport       = 4,                            /* temp memory support */
//    gestaltRealTempMemory       = 5,                            /* temp memory handles are real */
//    gestaltTempMemTracked       = 6,                            /* temporary memory handles are tracked */
//    gestaltIPCSupport           = 7,                            /* IPC support is present */
//    gestaltSysDebuggerSupport   = 8                             /* system debugger support is present */

//    gestaltPowerMgrAttr         = FOUR_CHAR_CODE('powr'),       /* power manager attributes */
//    gestaltPMgrExists           = 0,
//    gestaltPMgrCPUIdle          = 1,
//    gestaltPMgrSCC              = 2,
//    gestaltPMgrSound            = 3,
//    gestaltPMgrDispatchExists   = 4,
//    gestaltPMgrSupportsAVPowerStateAtSleepWake = 5

// should add multiprocessing check
// should add threadmgr check -- and we'll need optional code to do something diff.

Platform::SystemInfo_struct Platform::SystemInfo;

#define BASE_MHZ_SPEED      200

void Processor::init()
{
   OSErr err;
   long raw, mhzSpeed = BASE_MHZ_SPEED;

   Con::printf("System & Processor Information:");

   err = Gestalt(gestaltSystemVersion, &raw);
   
   Con::printf("   MacOS version: %x.%x.%x", (raw>>8), (raw&0xFF)>>4, (raw&0x0F));

   err = Gestalt(gestaltCarbonVersion, &raw);
   if (err) raw = 0;
   if (raw==0)
      Con::printf("   No CarbonLib support.");
   else
      Con::printf("   CarbonLib version: %x.%x.%x", (raw>>8), (raw&0xFF)>>4, (raw&0x0F));
   
   err = Gestalt(gestaltPhysicalRAMSize, &raw);
   raw /= (1024*1024); // MB
   Con::printf("   Physical RAM: %dMB", raw);
   err = Gestalt(gestaltLogicalRAMSize, &raw);
   raw /= (1024*1024); // MB
   Con::printf("   Logical RAM: %dMB", raw);

   // this is known to have issues with some Processor Upgrade cards
   err = Gestalt(gestaltProcClkSpeed, &raw);
   if (err==noErr && raw)
   {
      mhzSpeed = (float)raw / 1000000.0f;
      if (mhzSpeed < BASE_MHZ_SPEED)
      { // something drastically wrong.
         mhzSpeed = BASE_MHZ_SPEED;
      }
   }
   Platform::SystemInfo.processor.mhz = mhzSpeed;

   Platform::SystemInfo.processor.type = CPU_PowerPC_Unknown;
   err = Gestalt(gestaltNativeCPUtype, &raw);
   switch(raw)
   {
      case gestaltCPU601:
         Platform::SystemInfo.processor.type = CPU_PowerPC_601;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC 601");
         break;
      case gestaltCPU603e:
      case gestaltCPU603ev:
         Platform::SystemInfo.processor.type = CPU_PowerPC_603e;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC 603e");
         break;
      case gestaltCPU603:
         Platform::SystemInfo.processor.type = CPU_PowerPC_603;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC 603");
         break;
      case gestaltCPU604e:
      case gestaltCPU604ev:
         Platform::SystemInfo.processor.type = CPU_PowerPC_604e;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC 604e");
         break;
      case gestaltCPU604:
         Platform::SystemInfo.processor.type = CPU_PowerPC_604;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC 604");
         break;
      case gestaltCPU750: //G3
         Platform::SystemInfo.processor.type = CPU_PowerPC_G3;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC G3");
         break;
      case gestaltCPUG4:
         Platform::SystemInfo.processor.type = CPU_PowerPC_G4;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC G4");
         break;
/* at some point -- I these are either speedbump or the TiBooks or the new TiMacs 
//  gestaltCPUVger                = 0x0110, // Vger , Altivec
//  gestaltCPUApollo              = 0x0111 // Apollo , Altivec
       case gestaltCPUVger:
       case gestaltCPUVger:
         Platform::SystemInfo.processor.type = CPU_PowerPC_G4_FUTURE;
         Platform::SystemInfo.processor.name = StringTable->insert("PowerPC G4 FUTURE");
         break;
*/
      default:
         if (raw>gestaltCPUG4)
         { // for now, ID it as a G4u
            Platform::SystemInfo.processor.type = CPU_PowerPC_G4u;
            Platform::SystemInfo.processor.name = StringTable->insert("Unknown/PowerPC G4u ++");
         }
         else
         {
            Platform::SystemInfo.processor.type = CPU_PowerPC_Unknown;
            Platform::SystemInfo.processor.name = StringTable->insert("Unknown PowerPC");
         }
         break;
   }

   Platform::SystemInfo.processor.properties = CPU_PROP_PPCMIN;
   err = Gestalt(gestaltPowerPCProcessorFeatures, &raw);
#if defined(__VEC__)
   if ((1 << gestaltPowerPCHasVectorInstructions) & (raw)) {
      Platform::SystemInfo.processor.properties |= CPU_PROP_ALTIVEC; // OR it in as they are flags...
   }
#endif
   Con::printf("   %s, %d Mhz", Platform::SystemInfo.processor.name, Platform::SystemInfo.processor.mhz);
   if (Platform::SystemInfo.processor.properties & CPU_PROP_PPCMIN)
      Con::printf("   FPU detected");
   if (Platform::SystemInfo.processor.properties & CPU_PROP_ALTIVEC)
      Con::printf("   AltiVec detected");

   Con::printf(" ");
}
