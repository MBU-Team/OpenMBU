//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif/lmapPacker.h"
#include "platform/platformAssert.h"
#include "math/mPoint.h"
/* for FN_CDECL defs */
#include "platform/types.h"

const U32 SheetManager::csm_sheetSize = 256;

SheetManager::SheetManager()
{
   m_currSheet = -1;
   m_currX     = 0;
   m_currY     = 0;
   m_lowestY   = 0;
}

SheetManager::~SheetManager()
{
   for (U32 i = 0; i < m_sheets.size(); i++) {
      delete m_sheets[i].pData;
      m_sheets[i].pData = NULL;
   }

   m_currSheet = -1;
   m_currX     = 0;
   m_currY     = 0;
   m_lowestY   = 0;
}

void
SheetManager::begin()
{
   numPixels = 0;
   numSheetPixels = 0;
}

void
SheetManager::end()
{
   repackBlock();

//   dPrintf("\n\n"
//           "  Total Pixels: %d\n"
//           "  Total SheetP: %d\n"
//           "  Efficiency:   %g\n\n", numPixels, numSheetPixels, F32(numPixels)/F32(numSheetPixels));

   m_currY      =
      m_lowestY = csm_sheetSize + 1;
   m_currSheet  = -1;
}

U32 SheetManager::enterLightMap(const GBitmap* lm)
{
   U32 width  = lm->getWidth() + (SG_LIGHTMAP_BORDER_SIZE * 2);
   U32 height = lm->getHeight() + (SG_LIGHTMAP_BORDER_SIZE * 2);

   numPixels += width * height;

   if (m_currSheet < 0) 
   {
      // Must initialize the sheets...
      setupNewSheet();
   }

   if (m_currX + width >= csm_sheetSize) 
   {
      // Carriage return...
      m_currX    = 0;
      m_currY    = m_lowestY;
   }

   if (m_currY + height >= csm_sheetSize) 
   {
      // new sheet needed
      setupNewSheet();
   }

   m_lightMaps.increment();
   m_lightMaps.last().sheetId = m_currSheet;
   m_lightMaps.last().x       = m_currX;
   m_lightMaps.last().y       = m_currY;
   m_lightMaps.last().width   = width;
   m_lightMaps.last().height  = height;

   // And place the lightmap...
   //
   AssertFatal(lm->bytesPerPixel == m_sheets[m_currSheet].pData->bytesPerPixel, "Um, bad mismatch of bitmap types...");


   U32 y, b;
   U32 pixoffset = lm->bytesPerPixel;
   U32 borderwidth = SG_LIGHTMAP_BORDER_SIZE * 2;
   U32 nonborderpixlen = (width - borderwidth) * pixoffset;

   U32 lmheightindex = lm->getHeight() - 1;
   U32 lmborderheightindex = lmheightindex + SG_LIGHTMAP_BORDER_SIZE;
   U32 borderpixlen = pixoffset * SG_LIGHTMAP_BORDER_SIZE;

   for(y=0; y<height; y++)
   {
      U8 *srun, *drun;

      if(y < SG_LIGHTMAP_BORDER_SIZE)
         srun = (U8 *)lm->getAddress(0, 0);
      else if(y > lmborderheightindex)
         srun = (U8 *)lm->getAddress(0, lmheightindex);
      else
         srun = (U8 *)lm->getAddress(0, (y - SG_LIGHTMAP_BORDER_SIZE));

      drun = (U8 *)m_sheets[m_currSheet].pData->getAddress(m_currX, (m_currY + y));

      dMemcpy(&drun[borderpixlen], srun, nonborderpixlen);

      U8 *ss, *se;
      ss = srun;
      se = &srun[(nonborderpixlen - pixoffset)];

      for(b=0; b<SG_LIGHTMAP_BORDER_SIZE; b++)
      {
         U32 i = b * pixoffset;
         drun[i] = ss[0];
         drun[i+1] = ss[1];
         drun[i+2] = ss[2];

         i = (lm->getWidth() + SG_LIGHTMAP_BORDER_SIZE + b) * pixoffset;
         drun[i] = se[0];
         drun[i+1] = se[1];
         drun[i+2] = se[2];
      }
   }

   m_currX += width;
   if (m_currY + height > m_lowestY)
      m_lowestY = m_currY + height;

   return m_lightMaps.size() - 1;
}


const SheetManager::LightMapEntry&
SheetManager::getLightmap(const S32 in_lightMapIndex) const
{
   AssertFatal(U32(in_lightMapIndex) < m_lightMaps.size(), "Out of bounds lightmap");

   return m_lightMaps[in_lightMapIndex];
}

SheetManager::LightMapEntry&
SheetManager::getLightmapNC(const S32 in_lightMapIndex)
{
   AssertFatal(U32(in_lightMapIndex) < m_lightMaps.size(), "Out of bounds lightmap");

   return m_lightMaps[in_lightMapIndex];
}


void
SheetManager::setupNewSheet()
{
   m_sheets.increment();
   m_currSheet = m_sheets.size() - 1;

   m_sheets.last().pData = new GBitmap(csm_sheetSize, csm_sheetSize);
   for (U32 y = 0; y < m_sheets.last().pData->getHeight(); y++) {
      for (U32 x = 0; x < m_sheets.last().pData->getWidth(); x++) {
         U8* pDst = m_sheets.last().pData->getAddress(x, y);
         pDst[0] = 0xFF;
         pDst[1] = 0x00;
         pDst[2] = 0xFF;
      }
   }


   m_currX   = 0;
   m_currY   = 0;
   m_lowestY = 0;
}

void
SheetManager::repackBlock()
{
   if (m_lightMaps.size() == 0)
      return;

   U32 currLightMapStart = 0;
   U32 currLightMapEnd   = m_lightMaps.size() - 1;

   S32 first = 0;
   U32 num  = m_sheets.size();

   // OK, so at this point, we have a block of sheets, and a block of lightmaps on
   //  that sheet.  What we want to do is loop on the lightmaps until we have none
   //  left, and pack them into new sheets that are as small as possible.
   //
   do {
      const U32 numSizes = 16;
      U32 sizes[numSizes][2] = {
         {32, 32},         // 1024
         {32, 64},         // 2048
         {64, 32},         // 2048
         {64, 64},         // 4096
         {32, 128},        // 4096
         {128, 32},        // 4096
         {64, 128},        // 8192
         {128, 64},        // 8192
         {32, 256},        // 8192
         {256, 32},        // 8192
         {128, 128},       // 16384
         {64, 256},        // 16384
         {256, 64},        // 16384
         {128, 256},       // 32768
         {256, 128},       // 32768
         {256, 256}        // 65536
      };
//      const U32 numSizes = 4;
//      U32 sizes[numSizes][2] = {
//         {32, 32},         // 1024
//         {64, 64},         // 4096
//         {128, 128},       // 16384
//         {256, 256}        // 65536
//      };

      bool success = false;
      for (U32 i = 0; i < numSizes; i++) {
         if (doesFit(currLightMapStart, currLightMapEnd, sizes[i][0], sizes[i][1]) == true) {
            // Everything left fits into a 32
            //
            numSheetPixels += sizes[i][0] * sizes[i][1];
            repackSection(currLightMapStart, currLightMapEnd, sizes[i][0], sizes[i][1]);
            currLightMapStart = currLightMapEnd;
            success = true;
            break;
         }
      }
      if (success == false) {
         // BSearch for the max we can get into a 256, then keep going...
         //
         U32 searchStart = currLightMapStart;
         U32 searchEnd   = currLightMapEnd - 1;

         while (searchStart != searchEnd) {
            U32 probe = (searchStart + searchEnd) >> 1;

            if (doesFit(currLightMapStart, probe, 256, 256) == true) {
               if (searchStart != probe)
                  searchStart = probe;
               else
                  searchEnd = searchStart;
            } else {
               if (searchEnd != probe)
                  searchEnd = probe;
               else
                  searchEnd = searchStart;
            }
         }

         numSheetPixels += 256*256;
         repackSection(currLightMapStart, searchStart, 256, 256, true);
         currLightMapStart = searchStart + 1;
      }
   } while (currLightMapStart < currLightMapEnd);

   // All the sheets from the same block id are contigous, so all we have to do is
   //  decrement the sheetIds of the affected lightmaps...
   //
   for (U32 i = 0; i < m_lightMaps.size(); i++) {
      if (m_lightMaps[i].sheetId >= (first + num)) {
         m_lightMaps[i].sheetId -= num;
      }
   }

   for (U32 i = num; i != 0; i--) {
      SheetEntry& rEntry = m_sheets[first];

      delete rEntry.pData;
      rEntry.pData = NULL;

      m_sheets.erase(first);
   }
}


S32 FN_CDECL
compY(const void* p1, const void* p2)
{
   const Point2I* point1 = (const Point2I*)p1;
   const Point2I* point2 = (const Point2I*)p2;

   if (point1->y != point2->y)
      return point2->y - point1->y;
   else
      return point2->x - point1->x;
}


SheetManager* g_pManager = NULL;

S32 FN_CDECL
compMapY(const void* in_p1, const void* in_p2)
{
   const S32* p1 = (const S32*)in_p1;
   const S32* p2 = (const S32*)in_p2;

   const SheetManager::LightMapEntry& rEntry1 = g_pManager->getLightmap(*p1);
   const SheetManager::LightMapEntry& rEntry2 = g_pManager->getLightmap(*p2);

   if (rEntry1.height != rEntry2.height)
      return rEntry2.height - rEntry1.height;
   else
      return rEntry2.width - rEntry1.width;
}

void
SheetManager::repackSection(const S32 in_start,
                            const S32 in_end,
                            const U32 in_sizeX,
                            const U32 in_sizeY,
                            const bool overflow)
{
   Vector<S32> sheetIndices;
   for (S32 i = in_start; i <= in_end; i++)
      sheetIndices.push_back(i);

   g_pManager = this;
   dQsort(sheetIndices.address(), sheetIndices.size(), sizeof(S32), compMapY);
   g_pManager = NULL;

   // Ok, we now have the list of entries that we'll be entering, in the correct
   //  order.  Go for it!  Create the "right-sized" sheet, and pack.
   //
   m_sheets.increment();

   m_sheets.last().pData = new GBitmap(in_sizeX, in_sizeY);
   for (U32 y = 0; y < m_sheets.last().pData->getHeight(); y++) {
      for (U32 x = 0; x < m_sheets.last().pData->getWidth(); x++) {
         U8* pDst = m_sheets.last().pData->getAddress(x, y);

         if (overflow == false) {
            pDst[0] = 0xFF;
            pDst[1] = 0x00;
            pDst[2] = 0xFF;
         } else {
            pDst[0] = 0x00;
            pDst[1] = 0xFF;
            pDst[2] = 0x00;
         }
      }
   }

   U32 currX   = 0;
   U32 currY   = 0;
   U32 lowestY = 0;

   while (sheetIndices.size() != 0) {
      LightMapEntry& rEntry = getLightmapNC(sheetIndices[0]);

      S32 lastOfHeight = 0;
      for (U32 j = 1; j < sheetIndices.size(); j++) {
         LightMapEntry& rEntryTest = getLightmapNC(sheetIndices[j]);
         if (rEntryTest.height != rEntry.height)
            break;

         lastOfHeight = j;
      }

      S32 insert = -1;
      for (S32 k = 0; k <= lastOfHeight; k++) {
         LightMapEntry& rEntryTest = getLightmapNC(sheetIndices[k]);
         if (currX + rEntryTest.width < in_sizeX) {
            insert = k;
            break;
         }
      }

      if (insert == -1) {
         // CR
         currY  = lowestY;
         currX  = 0;
         insert = 0;
      }
      LightMapEntry* pInsert = &getLightmapNC(sheetIndices[insert]);
      AssertFatal(currY + pInsert->height < in_sizeY, "Error, too many lmaps for this size");

      for (S32 y = 0; y < pInsert->height; y++) {
         const U8* pSrc = m_sheets[pInsert->sheetId].pData->getAddress(pInsert->x, (pInsert->y + y));
         U8* pDst       = m_sheets.last().pData->getAddress(currX, (currY + y));

         dMemcpy(pDst, pSrc, pInsert->width * m_sheets[pInsert->sheetId].pData->bytesPerPixel);
      }

      pInsert->sheetId = m_sheets.size() - 1;
      pInsert->x       = currX;
      pInsert->y       = currY;
      AssertFatal(pInsert->x + pInsert->width <= m_sheets.last().pData->getWidth(), "very bad");
      AssertFatal(pInsert->y + pInsert->height <= m_sheets.last().pData->getHeight(), "also bad");

      currX += pInsert->width;
      if (currY + pInsert->height > lowestY)
         lowestY = currY + pInsert->height;

      sheetIndices.erase(insert);
   }
}


bool SheetManager::doesFit(const S32 startMap,
                           const S32 endMap,
                           const U32 sizeX,
                           const U32 sizeY) const
{
   Vector<Point2I> mapSizes;
   mapSizes.setSize(endMap - startMap + 1);

   for (S32 i = startMap; i <= endMap; i++) {
      mapSizes[i - startMap].x = m_lightMaps[i].width;
      mapSizes[i - startMap].y = m_lightMaps[i].height;

      if (m_lightMaps[i].width > sizeX ||
          m_lightMaps[i].height > sizeY)
         return false;
   }

   dQsort(mapSizes.address(), mapSizes.size(), sizeof(Point2I), compY);

   U32 currX   = 0;
   U32 currY   = 0;
   U32 lowestY = 0;

   while (mapSizes.size() != 0) {
      S32 lastOfHeight = 0;
      for (U32 j = 1; j < mapSizes.size(); j++) {
         if (mapSizes[j].y != mapSizes[0].y)
            break;

         lastOfHeight = j;
      }

      S32 insert = -1;
      for (S32 k = 0; k <= lastOfHeight; k++) {
         if (currX + mapSizes[k].x < sizeX) {
            insert = k;
            break;
         }
      }

      if (insert == -1) {
         // CR
         currX  = 0;
         currY  = lowestY;
         insert = 0;
      }

      if (currY + mapSizes[insert].y >= sizeY) {
         // Failure.
         return false;
      }

      currX += mapSizes[insert].x;
      if (currY + mapSizes[insert].y > lowestY)
         lowestY = currY + mapSizes[insert].y;

      mapSizes.erase(insert);
   }

   return true;
}
