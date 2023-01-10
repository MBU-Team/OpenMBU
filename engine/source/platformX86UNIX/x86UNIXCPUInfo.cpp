//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platform/platform.h"
#include "platformX86UNIX/platformX86UNIX.h"
#include "console/console.h"
#include "core/stringTable.h"
#include <math.h>

Platform::SystemInfo_struct Platform::SystemInfo;

extern void PlatformBlitInit();
extern void SetProcessorInfo(Platform::SystemInfo_struct::Processor& pInfo,
   char* vendor, U32 processor, U32 properties); // platform/platformCPU.cc

// asm cpu detection routine from platform code
extern "C"
{
void detectX86CPUInfo(char *vendor, U32 *processor, U32 *properties);
}

/* used in the asm */
static U32 time[2];
static U32 clockticks = 0;
static char vendor[13] = {0,};
static U32 properties = 0;
static U32 processor  = 0;

void Processor::init()
{
   // Reference:
   //    www.cyrix.com
   //    www.amd.com
   //    www.intel.com
   //       http://developer.intel.com/design/PentiumII/manuals/24512701.pdf
   Platform::SystemInfo.processor.type = CPU_X86Compatible;
   Platform::SystemInfo.processor.name = StringTable->insert("Unknown x86 Compatible");
   Platform::SystemInfo.processor.mhz  = 0;
   Platform::SystemInfo.processor.properties = CPU_PROP_C;

   clockticks = properties = processor = 0;
   dStrcpy(vendor, "");

   //detectX86CPUInfo(vendor, &processor, &properties);
   SetProcessorInfo(Platform::SystemInfo.processor,
      vendor, processor, properties);

   //--------------------------------------
   // if RDTSC support calculate the aproximate Mhz of the CPU
//   if (Platform::SystemInfo.processor.properties & CPU_PROP_RDTSC &&
//       Platform::SystemInfo.processor.properties & CPU_PROP_FPU)
//   {
//      const U32 MS_INTERVAL = 750;
//      __asm__(
//         "pushl  %eax\n"
//         "pushl  %edx\n"
//         "rdtsc\n"
//         "movl   %eax, (time)\n"
//         "movl   %edx, (time+4)\n"
//         "popl   %edx\n"
//         "popl   %eax\n"
//         );
//      U32 ms = Platform::getRealMilliseconds();
//      while ( Platform::getRealMilliseconds() < ms+MS_INTERVAL )
//      { /* empty */ }
//      ms = Platform::getRealMilliseconds()-ms;
//      asm(
//         "pushl  %eax\n"
//         "pushl  %edx\n"
//         "rdtsc\n"
//         "sub    (time+4), %edx\n"
//         "sbb    (time), %eax\n"
//         "mov    %eax, (clockticks)\n"
//         "popl   %edx\n"
//         "popl   %eax\n"
//         );
//      U32 mhz = static_cast<U32>(F32(clockticks) / F32(ms) / 1000.0f);
//
//      // catch-22 the timing method used above to calc Mhz is generally
//      // wrong by a few percent so we want to round to the nearest clock
//      // multiple but we also want to be careful to not touch overclocked
//      // results
//
//      // measure how close the Raw Mhz number is to the center of each clock
//      // bucket
//      U32 bucket25 = mhz % 25;
//      U32 bucket33 = mhz % 33;
//      U32 bucket50 = mhz % 50;
//
//      if (bucket50 < 8 || bucket50 > 42)
//         Platform::SystemInfo.processor.mhz =
//            U32((mhz+(50.0f/2.0f))/50.0f) * 50;
//      else if (bucket25 < 5 || bucket25 > 20)
//         Platform::SystemInfo.processor.mhz =
//            U32((mhz+(25.0f/2.0f))/25.0f) * 25;
//      else if (bucket33 < 5 || bucket33 > 28)
//         Platform::SystemInfo.processor.mhz =
//            U32((mhz+(33.0f/2.0f))/33.0f) * 33;
//      else
//         Platform::SystemInfo.processor.mhz = U32(mhz);
//   }

   Con::printf("Processor Init:");
   Con::printf("   %s, %d Mhz", Platform::SystemInfo.processor.name, Platform::SystemInfo.processor.mhz);
   if (Platform::SystemInfo.processor.properties & CPU_PROP_FPU)
      Con::printf("   FPU detected");
   if (Platform::SystemInfo.processor.properties & CPU_PROP_MMX)
      Con::printf("   MMX detected");
   if (Platform::SystemInfo.processor.properties & CPU_PROP_3DNOW)
      Con::printf("   3DNow detected");
   if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE)
      Con::printf("   SSE detected");
   Con::printf(" ");

   PlatformBlitInit();
}
