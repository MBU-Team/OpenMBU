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
#include "map2dif/tokenizer.h"
#include "map2dif/editGeometry.h"
#include "interior/interior.h"
#include "map2dif/editInteriorRes.h"
#include "map2dif/entityTypes.h"
#include "interior/floorPlanRes.h"
#include "map2dif/morianGame.h"
#include "core/frameAllocator.h"
#include "gui/core/guiCanvas.h"
#include "map2dif/lmapPacker.h"

#include <stdlib.h>

MorianGame GameObject;


// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;

#if defined(TORQUE_DEBUG)
const char* const gProgramVersion = "0.900r-beta";
#else
const char* const gProgramVersion = "0.900d-beta";
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

//
static bool initLibraries()
{
   // asserts should be created FIRST
   PlatformAssert::create();
   FrameAllocator::init(2 << 20);


   _StringTable::create();
//   TextureManager::create();

//   RegisterCoreTypes();
  // RegisterMathTypes();

   Con::init();

   Math::init();
   Platform::init();    // platform specific initialization
   return(true);
}

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

//   TextureManager::destroy();
   _StringTable::destroy();

   // asserts should be destroyed LAST
   FrameAllocator::destroy();
   PlatformAssert::destroy();
}

S32 getGraphNodes( char * mapFileName )
{
   // setup the tokenizer
   Tokenizer* pTokenizer = new Tokenizer();
   if (pTokenizer->openFile(mapFileName) == false) {
      dPrintf("getGraphNodes(): Error opening map file: %s", mapFileName);
      delete pTokenizer;
      shutdownLibraries();
      return -1;
   }

   // Create a geometry object
   AssertFatal(gWorkingGeometry == NULL, "EditGeometry already exists");
   gWorkingGeometry = new EditGeometry;

   // configure graph for generation.  not doing extrusion approach now.
   gWorkingGeometry->setGraphGeneration(true, false);

   // parse and create the geometry
   dPrintf("Map file opened for graph work: %s\n"
           "  Parsing mapfile...", mapFileName);  dFflushStdout();
   if (gWorkingGeometry->parseMapFile(pTokenizer) == false) {
      dPrintf("getGraphNodes(): Error parsing map file: %s\n", mapFileName);
      delete pTokenizer;
      delete gWorkingGeometry;
      shutdownLibraries();
      return -1;
   }
   delete pTokenizer;
   dPrintf("done.\n");

   // The code that gives us the node list is down in the createBSP()
   // call tree.  Kind of klunky but simpler for now.
   dPrintf("  Creating graph node list");    dFflushStdout();
   gWorkingGeometry->xferDetailToStructural();
   bool  result = gWorkingGeometry->createBSP();

   if( result )
      gWorkingGeometry->writeGraphInfo();

   delete gWorkingGeometry;      gWorkingGeometry = NULL;
   delete gWorkingResource;      gWorkingResource = NULL;
   shutdownLibraries();

   if( result == false ){
      dPrintf( "getGraphNodes(): Error in BSP processing (%s)!\n", mapFileName);
      return -1;
   }
   else{
      dPrintf( "getGraphNodes(): Seemed to work... \n" );
      return 0;
   }
}

char* cleanPath(const char* _path)
{
   char* path = new char[dStrlen(_path) + 2];
   dStrcpy(path, _path);

   // Clean up path char.
   for (char* ptr = path; *ptr != '\0'; ptr++)
      if (*ptr == '\\')
         *ptr = '/';

   // Check termination
   char* end = &path[dStrlen(path) - 1];
   if (*end != '/')  {
      end[1] = '/';
      end[2] = 0;
   }

   return path;
}

char* getPath(const char* file)
{
   char* path = new char[dStrlen(file) + 2];
   dStrcpy(path, file);

   // Strip back to first path char.
   char* slash = dStrrchr(path, '/');
   if (!slash)
      slash = dStrrchr(path, '\\');
   if (slash)
      *slash = 0;

   // Clean up path char.
   char* ptr = path;
   for (; *ptr != '\0'; ptr++)
      if (*ptr == '\\')
         *ptr = '/';

   // Check termination
   ptr--;
   if (*ptr != '/') {
      ptr[1] = '/';
      ptr[2] = 0;
   }

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

char* mergePath(const char* path1, const char* path2)
{
   // Will merge and strip off leading ".." from path2
   char* base = new char[dStrlen(path1) + dStrlen(path2) + 2];
   dStrcpy(base,path1);

   // Strip off ending path char.
   char* end = &base[dStrlen(base) - 1];
   if (*end == '/' || *end == '\\')
      *end = 0;

   // Deal with lead ./ and ../
   while (path2[0] == '.')
      if (path2[1] == '.') {
         // Chop off ../ and remove the trailing dir from base
         path2 += 2;
         if (*path2 == '/' || *path2 == '\\')
            path2++;
         char *ptr = dStrrchr(base, '/');
         if (!ptr)
            ptr = dStrrchr(base, '\\');
         AssertISV(ptr,"Error, could not merge relative path past root");
         *ptr = 0;
      }
      else {
         // Simply swallow the ./
         path2 += 1;
         if (*path2 == '/' || *path2 == '\\')
            path2++;
      }

   dStrcat(base,"/");
   dStrcat(base,path2);
   return base;
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
   bool isForNavigation = false, extrusionTest = false;
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
         case 'N':
            gVerbose = true;
            break;
         case 'G':
            isForNavigation = true;
            extrusionTest = true;
            break;
         case 'E':
            extrusionTest = true;
            break;
         case 'S':
            gTextureSearch = false;
            break;

         case 'Q':
            gQuakeVersion = atoi (argv[++i]);
            break;

         case 'T':
            wadPath = cleanPath(argv[++i]);
            break;
         case 'O':
            difPath = cleanPath(argv[++i]);
            break;
      }
   }
   U32 args = argc - i;
   if (args != 1) {
      dPrintf("\nmap2dif - Torque .MAP file converter\n"
              "  Copyright (C) GarageGames.com, Inc.\n"
              "  Program version: %s\n"
              "  Programmers: John Folliard & Dave Moore\n"
              "  Built: %s at %s\n\n"
              "Usage: map2dif [-v] [-p] [-s] [-l] [-h] [-g] [-e] [-n] [-q ver] [-o outputDirectory] [-t textureDirectory] <file>.map\n"
              "        -p : Include a preview bitmap in the interior file\n"
              "        -d : Process only the detail specified on the command line\n"
              "        -l : Process as a low detail shape (implies -s)\n"
              "        -h : Process for final build (exhaustive BSP search)\n"
              "        -g : Generate navigation graph info\n"
              "        -e : Do extrusion test\n"
              "        -s : Don't search for textures in parent dir.\n"
              "        -n : Noisy error/statistic reporting\n"
              "        -q ver: Quake map file version (2, 3)\n"
              "        -o dir: Directory in which to place the .dif file\n"
              "        -t dir: Location of textures\n", gProgramVersion, __DATE__, __TIME__);
      shutdownLibraries();
      return -1;
   }

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

   // Old relative path merge, should think about what to do with it.
   // wadPath = mergePath(mapPath,wadPath);
   // difPath = mergePath(mapPath,difPath);

   // Dif file name
   char* pOutputName = new char[dStrlen(difPath) + dStrlen(baseName) + 5];
   dStrcpy(pOutputName,difPath);
   dStrcat(pOutputName,baseName);
   dStrcat(pOutputName,".dif");

   // Wad path
   gWadPath = wadPath;

   //
   Vector<char*> mapFileNames;
   if (gSpecifiedDetailOnly == false) {
      const char* pDot = dStrrchr(mapFile, '.');

      if (pDot && *(pDot - 2) == '_') {
         // This is a detail based interior
         char buffer[1024];
         dStrcpy(buffer, mapFile);
         char* pBufDot = dStrrchr(buffer, '.');
         AssertFatal(pBufDot, "Error, why isn't it in this buffer too?");
         *(pBufDot-1) = '\0';

         for (U32 i = 0; i <= 9; i++) {
            mapFileNames.push_back(new char[1024]);
            dSprintf(mapFileNames.last(), 1023, "%s%d%s", buffer, i, pDot);
         }

         // Now, eliminate all mapFileNames that aren't actually map files
         for (S32 i = S32(mapFileNames.size() - 1); i >= 0; i--) {
            Tokenizer* pTokenizer = new Tokenizer();
            if (pTokenizer->openFile(mapFileNames[i]) == false) {
               delete [] mapFileNames[i];
               mapFileNames.erase(i);
            }
            delete pTokenizer;
         }
      } else {
         // normal interior
         mapFileNames.push_back(new char[dStrlen(mapFile) + 1]);
         dStrcpy(mapFileNames.last(), mapFile);
      }
   } else {
      mapFileNames.push_back(new char[dStrlen(mapFile) + 1]);
      dStrcpy(mapFileNames.last(), mapFile);
   }

   gWorkingResource = new EditInteriorResource;

   if( isForNavigation ){
      S32   retCode = getGraphNodes( mapFileNames[0] );
      delete [] pOutputName;
      for (U32 i = 0; i < mapFileNames.size(); i++)
         delete [] mapFileNames[i];
      return retCode;
   }

   for (U32 i = 0; i < mapFileNames.size(); i++) {
      // setup the tokenizer
      Tokenizer* pTokenizer = new Tokenizer();
      if (pTokenizer->openFile(mapFileNames[i]) == false) {
         dPrintf("Error opening map file: %s", mapFileNames[i]);
         delete pTokenizer;
         shutdownLibraries();
         return -1;
      }

      // Create a geometry object
      AssertFatal(gWorkingGeometry == NULL, "Already working?");
      gWorkingGeometry = new EditGeometry;

      // parse and create the geometry
      dPrintf("Successfully opened map file: %s\n"
              "  Parsing mapfile...", mapFileNames[i]);
      dFflushStdout();
      if (gWorkingGeometry->parseMapFile(pTokenizer) == false) {
         dPrintf("Error parsing map file: %s\n", mapFileNames[i]);
         delete pTokenizer;
         delete gWorkingGeometry;
         delete gWorkingResource;
         shutdownLibraries();
         return -1;
      }
      delete pTokenizer;
      dPrintf("done.\n");

      gWorkingGeometry->setGraphGeneration(false,extrusionTest);

      dPrintf("  Creating BSP...");
      dFflushStdout();
      if (gWorkingGeometry->createBSP() == false) {
         dPrintf("Error creating BSP!\n", mapFileNames[i]);
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
//          else if (dynamic_cast<ForceFieldEntity*>(gWorkingGeometry->mEntities[i]) != NULL) {
//             ForceFieldEntity* pForceField = static_cast<ForceFieldEntity*>(gWorkingGeometry->mEntities[i]);
//             pForceField->process();
//          }
      }

      // Give status
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

      if (gWorkingGeometry->getNumAmbiguousBrushes() != 0 ||
          gWorkingGeometry->getNumOrphanPolys() != 0) {
         dPrintf(
            "\n  ** ***  WARNING WARNING WARNING  *** **\n"
            "  *** **  WARNING WARNING WARNING  ** ***\n"
            "\n   Errors exists in this interior.  Please use the debug rendering modes\n"
            "    to find and correct the following problems:\n"
            "\n   * Ambiguous brushes: %d"
            "\n   * Orphaned Polygons: %d\n"
            "\n  *** **  WARNING WARNING WARNING  ** ***\n"
            "  ** ***  WARNING WARNING WARNING  *** **\n",
            gWorkingGeometry->getNumAmbiguousBrushes(),
            gWorkingGeometry->getNumOrphanPolys());
      }

      // DMMTODO: store new geometry in the correct instance location
      Interior* pRuntime = new Interior;

      //
      // Support for interior light map border sizes.
      //
      pRuntime->setLightMapBorderSize(SG_LIGHTMAP_BORDER_SIZE);

      dPrintf("\n  Exporting to runtime..."); dFflushStdout();
      gWorkingGeometry->exportToRuntime(pRuntime, gWorkingResource);
      dPrintf("done.\n\n");
      dFflushStdout();
      gWorkingResource->addDetailLevel(pRuntime);

      delete gWorkingGeometry;
      gWorkingGeometry = NULL;
   }

   if (gWorkingResource->getNumDetailLevels() > 0) {
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
