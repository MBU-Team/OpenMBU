//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/platformAssert.h"
#include "platform/platformVideo.h"
#include "math/mMath.h"
#include "console/console.h"
#include "gfx/gBitmap.h"
#include "core/tVector.h"
#include "core/fileStream.h"
//#include "dgl/gTexManager.h"
#include "console/consoleTypes.h"
#include "math/mathTypes.h"
#include "core/tokenizer.h"
#include "map2dif plus/editGeometry.h"
#include "interior/interior.h"
#include "map2dif plus/editInteriorRes.h"
#include "interior/floorPlanRes.h"
#include "map2dif plus/morianGame.h"
#include "core/frameAllocator.h"
#include "gui/core/guiCanvas.h"
#include "map2dif plus/lmapPacker.h"
#include "map2dif plus/convert.h"

#include <stdlib.h>

MorianGame GameObject;


// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;

#if defined(TORQUE_DEBUG)
const char* const gProgramVersion = "1.0d";
#else
const char* const gProgramVersion = "1.0r";
#endif

//bool gRenderPreview       = false;
bool gSpecifiedDetailOnly = false;
bool gBuildAsLowDetail    = false;
bool gVerbose             = false;
const char* gWadPath = "base/textures/";
bool gTextureSearch = true;
int gQuakeVersion = 2;

U32 gMaxPlanesConsidered  = 32;

EditInteriorResource* gWorkingResource = NULL;

//#if defined(TORQUE_OS_WIN32) // huger hack
// huge hack
//GuiCanvas *Canvas;
//void GuiCanvas::paint() {}
//#endif

//
static bool initLibraries()
{
   // asserts should be created FIRST
   //PlatformAssert::create();
   FrameAllocator::init(2 << 20);

   _StringTable::create();

   ResManager::create();

   // Register known file types here
   ResourceManager->registerExtension(".jpg", constructBitmapJPEG);
   ResourceManager->registerExtension(".png", constructBitmapPNG);
   ResourceManager->registerExtension(".gif", constructBitmapGIF);
   ResourceManager->registerExtension(".dbm", constructBitmapDBM);
   ResourceManager->registerExtension(".bmp", constructBitmapBMP);
   //ResourceManager->registerExtension(".gft", constructFont);
   ResourceManager->registerExtension(".dif", constructInteriorDIF);
   ResourceManager->registerExtension(".map", constructInteriorMAP);

//   RegisterCoreTypes();
//   RegisterMathTypes();

   Con::init();

   Math::init();
   Platform::init();    // platform specific initialization

   // Create a log file
   Con::setLogMode(6);

   return(true);
}

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

   _StringTable::destroy();

   // asserts should be destroyed LAST
   FrameAllocator::destroy();
   PlatformAssert::destroy();
}

void cleanSlashes(char* path)
{
   // Clean up path char.
   for (char* ptr = path; *ptr != '\0'; ptr++)
      if (*ptr == '\\')
         *ptr = '/';
}

void terminate(char* path)
{
   // Check termination
   char* end = &path[dStrlen(path) - 1];
   if (*end != '/')
   {
      end[1] = '/';
      end[2] = 0;      
   }
}

char* cleanPath(const char* _path)
{
   char* path = new char[dStrlen(_path) + 2];
   dStrcpy(path, _path);

   cleanSlashes(path);

   terminate(path);

   return path;
}   

char* getPath(const char* file)
{
   char* path = "\0";

   if (!dStrchr(file, '/') && !dStrchr(file, '\\'))
      return path;
   else
   {
      path = new char[dStrlen(file) + 2];
      dStrcpy(path, file);
   }

   // Strip back to first path char.
   char* slash = dStrrchr(path, '/');
   if (!slash)
      slash = dStrrchr(path, '\\');
   if (slash)
      *slash = 0;

   cleanSlashes(path);

   terminate(path);

   return path;
}

char* getBaseName(const char* file)
{
   // Get rid of path
   const char* slash = dStrrchr(file, '/');
   if (!slash)
      slash = dStrrchr(file, '\\');
   if (!slash)
      slash = file;
   else
      slash++;
   char* name = new char[dStrlen(slash) + 1];
   dStrcpy(name, slash);

   // Strip extension & trailing _N
   char* dot = dStrrchr(name, '.') - 2;
   if (dot[0] == '_' && (dot[1] >= '0' && dot[1] <= '9'))
      dot[0] = '\0';
   else
      dot[2] = '\0';

   return name;
}   

S32 MorianGame::main(int argc, const char** argv)
{
   // Set the memory manager page size to 64 megs...
   setMinimumAllocUnit(64 << 20);

   if(!initLibraries())
      return 0;

   // Set up the command line args for the console scripts...
   Con::setIntVariable("Game::argc", argc);
   for (S32 i = 0; i < argc; i++)
      Con::setVariable(avar("Game::argv%d", i), argv[i]);

   // Parse command line args...
   bool extrusionTest = false;
   const char* wadPath = 0;
   const char* difPath = 0;
   S32 i = 1;
   for (; i < argc; i++) {
      if (argv[i][0] != '-')
         break;
      switch(dToupper(argv[i][1])) {
         case 'D':
            gSpecifiedDetailOnly = true;
            break;
         case 'L':
            gSpecifiedDetailOnly = true;
            gBuildAsLowDetail = true;
            break;
         case 'H':
            gMaxPlanesConsidered = U32(1 << 30);
            break;
         case 'S':
            gTextureSearch = false;
            break;
         case 'T':
            wadPath = cleanPath(argv[++i]);
            break;
         case 'O':
            difPath = cleanPath(argv[++i]);
            break;
         case 'P':
            gcPlaneDistanceEpsilon = 0.00001;
//            gPolyListDeviance = 0.00001;
            break;
      }
   }
   U32 args = argc - i;
   if (args != 1) {
      dPrintf("\nmap2dif - Torque .MAP file converter\n"
              "  Copyright (C) GarageGames.com, Inc.\n"
              "  Program version: %s\n"
              "  Programmers: John Folliard, Dave Moore, and Matthew Fairfax\n"
              "  Built: %s at %s\n\n"
              "Usage: map2dif [-s] [-l] [-h] [-e] [-o outputDirectory] [-t textureDirectory] <file>.map\n"
              "        -d : Process only the detail specified on the command line\n"
              "        -l : Process as a low detail shape (implies -s)\n"
              "        -h : Process for final build (exhaustive BSP search)\n"
              "        -s : Don't search for textures in parent dir.\n"
              "        -o dir: Directory in which to place the .dif file\n"
              "        -t dir: Location of textures\n", gProgramVersion, __DATE__, __TIME__);
      shutdownLibraries();
      return -1;
   }
   else
      dPrintf("\nmap2dif - Torque .MAP file converter\n"
              "  Copyright (C) GarageGames.com, Inc.\n"
              "  Program version: %s\n"
              "  Programmers: John Folliard, Dave Moore, and Matthew Fairfax\n"
              "  Built: %s at %s\n\n", gProgramVersion, __DATE__, __TIME__);

   // Check map file extension
   const char* mapFile = argv[i];
   const char* pDot = dStrrchr(mapFile, '.');
   AssertISV(pDot && ((dStricmp(pDot, ".map") == 0)),
         "Error, the map file must have a .MAP extension.");

   // Get path and file name arguments
   const char* mapPath = getPath(mapFile);
   const char* baseName = getBaseName(mapFile);

   if (!wadPath)
      wadPath = mapPath;
   if (!difPath)
      difPath = mapPath;

   // Wad path
   gWadPath = wadPath;

   // Print out which file we are loading and what texture search path has been set
   Con::printf("Loading %s", mapFile);
   dPrintf("\nLoading %s\n", mapFile);
   Con::printf("Initial texture search path is set to \"%s\"\n", gWadPath);
   dPrintf("Initial texture search path is set to \"%s\"\n\n", gWadPath);

   dFflushStdout();

   // Dif file name
   char* pOutputName = new char[dStrlen(difPath) + dStrlen(baseName) + 5];
   dStrcpy(pOutputName, difPath);
   dStrcat(pOutputName, baseName);
   dStrcat(pOutputName, ".dif");

   Vector<char*> mapFileNames;
   if (gSpecifiedDetailOnly == false)
   {
      const char* pDot = dStrrchr(mapFile, '.');

      if (pDot && *(pDot - 2) == '_')
      {
         // This is a detail based interior
         char buffer[1024];
         dStrcpy(buffer, mapFile);
         char* pBufDot = dStrrchr(buffer, '.');
         AssertFatal(pBufDot, "Error, why isn't it in this buffer too?");
         *(pBufDot-1) = '\0';

         for (U32 i = 0; i <= 9; i++)
         {
            mapFileNames.push_back(new char[1024]);
            dSprintf(mapFileNames.last(), 1023, "%s%d%s", buffer, i, pDot);
         }

         // Now, eliminate all mapFileNames that aren't actually map files
         for (S32 i = S32(mapFileNames.size() - 1); i >= 0; i--)
         {
            Tokenizer* pTokenizer = new Tokenizer();
            if (pTokenizer->openFile(mapFileNames[i]) == false)
            {
               delete [] mapFileNames[i];
               mapFileNames.erase(i);
            }
            delete pTokenizer;
         }
      }
      else
      {
         // normal interior
         mapFileNames.push_back(new char[dStrlen(mapFile) + 1]);
         dStrcpy(mapFileNames.last(), mapFile);
      }
   }
   else
   {
      mapFileNames.push_back(new char[dStrlen(mapFile) + 1]);
      dStrcpy(mapFileNames.last(), mapFile);
   }

   // Fix the slashes
   for (U32 i = 0; i < mapFileNames.size(); i++)
      cleanSlashes(mapFileNames[i]);

   gWorkingResource = new EditInteriorResource;
   
   for (U32 i = 0; i < mapFileNames.size(); i++)
   {
      // setup the tokenizer
      Tokenizer* pTokenizer = new Tokenizer();
      if (pTokenizer->openFile(mapFileNames[i]) == false)
      {
         dPrintf("Error opening map file: %s", mapFileNames[i]);
         delete pTokenizer;
         shutdownLibraries();
         return -1;
      }

      // Create a geometry object
      AssertFatal(gWorkingGeometry == NULL, "Already working?");
      gWorkingGeometry = new EditGeometry;

      // Parse and create the geometry
      // First we need our object to load the file into
      InteriorMap map;

      // Set the texture path
      map.mTexPath = StringTable->insert(wadPath);

      // Set our path info
      map.setPath(mapFileNames[i]);

      // Load resource
      map.mInteriorRes = ResourceManager->load(mapFileNames[i], true);
      if (map.mInteriorRes)
      {
         Con::printf("Successfully opened map file: %s", mapFileNames[i]);
         dPrintf("Successfully opened map file: %s\n", mapFileNames[i]);
         dFflushStdout();
      }
      else
      {
         Con::printf("   Unable to load map file: %s", mapFileNames[i]);
         dPrintf("   Unable to load map file: %s\n", mapFileNames[i]);
         delete pTokenizer;
         delete gWorkingGeometry;
         delete gWorkingResource;
         shutdownLibraries();
         return -1;
      }

      // Little late for this but it looks nice
      dPrintf("  Parsing mapfile...");
      dFflushStdout();

      loadTextures(&map);

      // Reserve enough room for all the brushes
      // Not truly "dynamic" but it will do for now
      gBrushArena.setSize(map.mInteriorRes->mBrushes.size());

      convertInteriorMap(&map);

      delete pTokenizer;
      dPrintf("   done.\n");

      gWorkingGeometry->setGraphGeneration(false,extrusionTest);

      dPrintf("  Creating BSP...");
      dFflushStdout();
      if (gWorkingGeometry->createBSP() == false)
      {
         Con::printf("Error creating BSP for %s!", mapFileNames[i]);
         dPrintf("Error creating BSP for %s!\n", mapFileNames[i]);
         // delete pTokenizer;  (already)
         delete gWorkingGeometry;
         delete gWorkingResource;
         shutdownLibraries();
         return -1;
      }

      dPrintf("done.\n  Marking active zones...");
      gWorkingGeometry->markEmptyZones();
      dPrintf("done\n  Creating surfaces..."); dFflushStdout();
      gWorkingGeometry->createSurfaces();
      dPrintf("done.\n  Lightmaps: Normal...");
      dFflushStdout();
      gWorkingGeometry->computeLightmaps(false);
      dPrintf("Alarm...");
      dFflushStdout();
      gWorkingGeometry->computeLightmaps(true);
      dPrintf("done.\n  Resorting and Packing LightMaps..."); dFflushStdout();
      gWorkingGeometry->preprocessLighting();
      gWorkingGeometry->sortLitSurfaces();
      gWorkingGeometry->packLMaps();
      dPrintf("done.\n");
      dFflushStdout();

      // Process any special entitys...
      for (U32 i = 0; i < gWorkingGeometry->mEntities.size(); i++)
      {
         if (dynamic_cast<DoorEntity*>(gWorkingGeometry->mEntities[i]) != NULL) {
             DoorEntity* pDoor = static_cast<DoorEntity*>(gWorkingGeometry->mEntities[i]);
             pDoor->process();
         }
      }

      // Give status
      Con::printf("\n  STATISTICS\n"
              "   - Total brushes:      %d\n"
              "     + structural:       %d\n"
              "     + detail:           %d\n"
              "     + portal:           %d\n"
              "   - Number of zones:    %d\n"
              "   - Number of surfaces: %d\n", gWorkingGeometry->getTotalNumBrushes(),
              gWorkingGeometry->getNumStructuralBrushes(),
              gWorkingGeometry->getNumDetailBrushes(),
              gWorkingGeometry->getNumPortalBrushes(),
              gWorkingGeometry->getNumZones(),
              gWorkingGeometry->getNumSurfaces());
      dPrintf("\n  STATISTICS\n"
              "   - Total brushes:      %d\n"
              "     + structural:       %d\n"
              "     + detail:           %d\n"
              "     + portal:           %d\n"
              "   - Number of zones:    %d\n"
              "   - Number of surfaces: %d\n", gWorkingGeometry->getTotalNumBrushes(),
              gWorkingGeometry->getNumStructuralBrushes(),
              gWorkingGeometry->getNumDetailBrushes(),
              gWorkingGeometry->getNumPortalBrushes(),
              gWorkingGeometry->getNumZones(),
              gWorkingGeometry->getNumSurfaces());

      // DMMTODO: store new geometry in the correct instance location
      Interior* pRuntime = new Interior;

      // Support for interior light map border sizes.
      //pRuntime->setLightMapBorderSize(SG_LIGHTMAP_BORDER_SIZE);

      dPrintf("\n  Exporting to runtime..."); dFflushStdout();
      gWorkingGeometry->exportToRuntime(pRuntime, gWorkingResource);
      dPrintf("done.\n\n");
      dFflushStdout();
      gWorkingResource->addDetailLevel(pRuntime);

      delete gWorkingGeometry;
      gWorkingGeometry = NULL;
   }

   if (gWorkingResource->getNumDetailLevels() > 0)
   {
      dPrintf(" Writing Resource: "); dFflushStdout();

      dPrintf("persist..(%s) ", pOutputName); dFflushStdout();
      gWorkingResource->sortDetailLevels();

      gWorkingResource->getDetailLevel(0)->processHullPolyLists();
      gWorkingResource->getDetailLevel(0)->processVehicleHullPolyLists();
      for (U32 i = 1; i < gWorkingResource->getNumDetailLevels(); i++)
         gWorkingResource->getDetailLevel(i)->purgeLODData();

      FileStream fws;
      fws.open(pOutputName, FileStream::Write);
      gWorkingResource->write(fws);
      fws.close();

      dPrintf("Done.\n\n");
      dFflushStdout();

      delete gWorkingResource;
   }

   delete [] pOutputName;
   for (U32 i = 0; i < mapFileNames.size(); i++)
      delete [] mapFileNames[i];

   shutdownLibraries();
   return 0;
}

void GameReactivate()
{

}

void GameDeactivate( bool )
{

}
