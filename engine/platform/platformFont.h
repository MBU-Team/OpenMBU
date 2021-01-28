//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _PLATFORMFONT_H_
#define _PLATFORMFONT_H_

class PlatformFont
{
public:
   struct CharInfo 
   {
      S16 bitmapIndex;    // Note: -1 indicates character is NOT to be
                          //  rendered, i.e., \n, \r, etc.
      U8  xOffset;        // x offset into bitmap sheet
      U8  yOffset;        // y offset into bitmap sheet
      U8  width;          // width of character (pixels)
      U8  height;         // height of character (pixels)
      S8  xOrigin;
      S8  yOrigin;
      S8  xIncrement;
      U8  *bitmapData;    // temp storage for bitmap data
   };

   virtual bool isValidChar(const UTF16 ch) const = 0;
   virtual bool isValidChar(const UTF8 *str) const = 0;

   virtual U32 getFontHeight() const = 0;
   virtual U32 getFontBaseLine() const = 0;

   virtual PlatformFont::CharInfo &getCharInfo(const UTF16 ch) const = 0;
   virtual PlatformFont::CharInfo &getCharInfo(const UTF8 *str) const = 0;

   virtual bool create(const char *name, U32 size, U32 charset = TGE_ANSI_CHARSET) = 0;
};

extern PlatformFont *createPlatformFont(const char *name, U32 size, U32 charset = TGE_ANSI_CHARSET);


#endif // _PLATFORMFONT_H_
