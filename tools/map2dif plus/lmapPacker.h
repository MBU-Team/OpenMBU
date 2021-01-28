//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ITRSHEETMANAGER_H_
#define _ITRSHEETMANAGER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GBITMAP_H_
#include "gfx/gBitmap.h"
#endif

//
// Defines the interior light map border size.
//
// Light map borders prevent visual artifacts
// caused by colors bleeding from adjacent light maps
// or dead texture space.
//
#define SG_LIGHTMAP_BORDER_SIZE 10

class SheetManager
{
  public:
   struct LightMapEntry 
   {
      U32   sheetId;

      U16   x, y;
      U16   width, height;
   };

   struct SheetEntry 
   {
      GBitmap* pData;
   };

  public:
   static const U32     csm_sheetSize;

  public:
   Vector<LightMapEntry>   m_lightMaps;
   Vector<SheetEntry>      m_sheets;

   S32                   m_currSheet;

   U32                  m_currX;
   U32                  m_currY;
   U32                  m_lowestY;

   void setupNewSheet();

   void repackSection(const S32, const S32, const U32 in_sizeX, const U32 in_sizeY, const bool = false);
   void repackBlock();
   bool doesFit(const S32, const S32, const U32 sizeX, const U32 sizeY) const;
   LightMapEntry& getLightmapNC(const S32 in_lightMapIndex);

  public:
   SheetManager();
   ~SheetManager();

   U32 numPixels;
   U32 numSheetPixels;

   void   begin();
   void   end();
   U32    enterLightMap(const GBitmap* in_pData);

   const LightMapEntry& getLightmap(const S32 in_lightMapIndex) const;
};

#endif // _ITRSHEETMANAGER_H_
