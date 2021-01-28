//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "engine/platform/platform.h"
#include "engine/platform/platformAssert.h"
#include "engine/dgl/gBitmap.h"
#include "engine/dgl/gPalette.h"
#include "engine/platform/event.h"
#include "engine/core/fileStream.h"
#include "tools/texmunge/svector.h"
#include "engine/sim/frameAllocator.h"
#include "engine/dgl/gTexManager.h"
#include "engine/console/console.h"
#include "engine/core/resManager.h"
#include "engine/core/zipAggregate.h"

//------------------------------------------------------------------------------
#include "engine/platform/gameInterface.h"
class ZipCleanserGame : public GameInterface
{
   public:
      S32 main(S32 argc, const char **argv);
} GameObject;

// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;
void GameHandleNotify(NetConnectionId, bool)
{
}

#if defined(TORQUE_DEBUG)
   const char * const gProgramVersion = "0.1d";
#else
   const char * const gProgramVersion = "0.1r";
#endif


//------------------------------------------------------------------------------

static bool initLibraries()
{
   // asserts should be created FIRST
   PlatformAssert::create();
   FrameAllocator::init(2 << 20);


   _StringTable::create();

   // Register known file types here
   ResManager::create();
   ResourceManager->registerExtension(".png", constructBitmapPNG);

   Con::init();

   Math::init();
   Platform::init();    // platform specific initialization
   return(true);
}

//------------------------------------------------------------------------------

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

   ResManager::destroy();
   _StringTable::destroy();

   FrameAllocator::destroy();
   // asserts should be destroyed LAST
   PlatformAssert::destroy();
}

bool cleanseMem(U8* mem, U32 size)
{
   U32 currPos = 0;

   while (currPos < size)
   {
      U32 sig = *((U32*)&mem[currPos]);
      if (sig == 0x04034b50)
      {
         // Local file header
         //  bitflag located at offset 6
         //  file name length located at offset 26
         //  extra field length located at offset 28
         U16 bitflag = *((U16*)&mem[currPos + 6]);
         U32 compressedsize = *((U32*)&mem[currPos + 18]);
         U16 filenamelength = *((U16*)&mem[currPos + 26]);
         U16 extrafieldlength = *((U16*)&mem[currPos + 28]);
         currPos += 30 + filenamelength;
         dMemset(&mem[currPos], 0, extrafieldlength);
         currPos += extrafieldlength;
         currPos += compressedsize;
         if (bitflag & 1 << 3)
         {
            // get past optional data descriptor
            currPos += 24;
         }
      }
      else if (sig == 0x02014b50)
      {
         U16 filenamelength    = *((U16*)&mem[currPos + 28]);
         U16 extrafieldlength  = *((U16*)&mem[currPos + 30]);
         U16 filecommentlength = *((U16*)&mem[currPos + 32]);
         currPos += 46 + filenamelength;
         dMemset(&mem[currPos], 0, extrafieldlength + filecommentlength);
         currPos += extrafieldlength + filecommentlength;
      }
      else if (sig == 0x06054b50)
      {
         return true;
      }
      else if (sig == 0x05054b50)
      {
         U16 datalength = *((U16*)&mem[currPos + 4]);
         currPos += 6 + datalength;
      }
      else
      {
         AssertFatal(false, avar("bad record!: %x", sig));
         return false;
      }
   }

   AssertFatal(false, "missing eocd record");
   return false;
}










int ZipCleanserGame::main(S32 argc, const char** argv)
{
   if(!initLibraries())
      return 0;

//    const char* argvfake[] = { "asdf", "test.vl2" };
//    argc = 2;
//    argv = argvfake;

   // info
   if(argc < 2)
   {
      dPrintf("\nZipCleanser - Torque zip cleaner-upper\n"
              "  Copyright (C) GarageGames.com, Inc.\n"
              "  Program version: %s\n"
              "  Brought to you by: Dave Moore\n"
              "  Built: %s at %s\n", gProgramVersion, __DATE__, __TIME__);

      dPrintf("\n  Usage: zipCleanser input.<zip|vl2|...>\n");
      shutdownLibraries();
      return 0;
   }

   FileStream frs;
   if (frs.open(argv[1], FileStream::Read) == false)
   {
      dPrintf("Could not open: %s\n", argv[1]);
      return 1;
   }

   U32 size = frs.getStreamSize();
   U8* pMem = new U8[size];

   frs.read(size, pMem);
   frs.close();

   if (cleanseMem(pMem, size) == false)
   {
      dPrintf("Couldn't cleanse %s, not a zipfile?\n", argv[1]);
      return 1;
   }

   FileStream fws;
   fws.open("tmp.tmp", FileStream::Write);
   fws.write(size, pMem);
   fws.close();

   shutdownLibraries();
   return 0;
}


void GameReactivate()
{

}

void GameDeactivate( bool )
{

}

