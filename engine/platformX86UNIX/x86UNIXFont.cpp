//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformX86UNIX.h"
#include "dgl/gFont.h"
#include "dgl/gBitmap.h"
#include "math/mRect.h"
#include "console/console.h"

// Needed by createFont
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

// Needed for getenv in createFont
#include <stdlib.h>

S32 mapSize(char* fontname, S32 size)
{
   // warning!  hacking ahead

   S32 newSize = size;

   // if the size is >= 12, do a size - 2 adjustment
   // (except for courier.)
   // fonts seem to be too big otherwise, but this might be
   // system/font specific
   if (newSize >= 12 && !stristr(fontname, "courier"))
      newSize = newSize - 2;

   // adobe helvetica and courier look like crap at 22
   // (probably scaled bitmap problem)
   if (newSize == 22 &&
      (stristr(fontname, "helvetica")==0 ||
      stristr(fontname, "courier")==0))
      newSize = 24;

   return newSize;
}

XFontStruct* loadFont(const char *name, S32 size, Display* display,
   char* pickedFontName, int pickedFontNameSize)
{
   XFontStruct* fontInfo = NULL;

   char* fontname = const_cast<char*>(name);
   if (dStrlen(fontname)==0)
      fontname = "arial";
   else if (stristr(const_cast<char*>(name), "arial") != NULL)
      fontname = "arial";
   else if (stristr(const_cast<char*>(name), "lucida console") != NULL)
      fontname = "lucida console";

   char* weight = "medium";
   char* slant = "r"; // no slant

   int newSize = mapSize(fontname, size);

   // look for bold or italic
   if (stristr(const_cast<char*>(name), "bold") != NULL)
      weight = "bold";
   if (stristr(const_cast<char*>(name), "italic") != NULL)
      slant = "o";

   const int xfontNameSize = 512;
   char xfontName[xfontNameSize];
   char* fontFormat = "-*-%s-%s-%s-*-*-%d-*-*-*-*-*-*-*";
   dSprintf(xfontName, xfontNameSize, fontFormat, fontname, weight, slant, newSize);
   // lowercase the whole thing
   strtolwr(xfontName);

   fontInfo = XLoadQueryFont(display, xfontName);

   if (fontInfo == NULL)
   {
      // Couldn't load the requested font.  This probably will be common
      // since many unix boxes don't have arial or lucida console installed.
      // Attempt to map the font name into a font we're pretty sure exists
      if (stristr(const_cast<char*>(name), "arial") != NULL)
         fontname = "helvetica";
      else if (stristr(const_cast<char*>(name), "lucida console") != NULL)
         fontname = "courier";
      else
         // to helvetica with you!
         fontname = "helvetica";

      newSize = mapSize(fontname, size);

      // change the font format so that we get adobe fonts
      fontFormat = "-adobe-%s-%s-%s-*-*-%d-*-*-*-*-*-*-*";
      dSprintf(xfontName, xfontNameSize, fontFormat, fontname, weight, slant, newSize);
      fontInfo = XLoadQueryFont(display, xfontName);
   }

   if (fontInfo != NULL && pickedFontName && pickedFontNameSize > 0)
      dSprintf(pickedFontName, pickedFontNameSize, "%s", xfontName);

   return fontInfo;
}

GFont *createFont(const char *name, dsize_t size)
{
   char * displayName = getenv("DISPLAY");
   Display* display = NULL;

   display = XOpenDisplay(displayName);
   if (display == NULL)
      AssertFatal(false, "createFont: cannot connect to X server");

   // load the font
   const int pickedFontNameSize = 512;
   char pickedFontName[pickedFontNameSize];
   XFontStruct* fontInfo =
      loadFont(name, size, display, pickedFontName, pickedFontNameSize);
   if (!fontInfo)
      AssertFatal(false, "createFont: cannot load font");
   Con::printf("CreateFont: request for %s %d, using %s", name, size,
      pickedFontName);

   // store some info about the font
   int maxAscent = fontInfo->max_bounds.ascent;
   int maxDescent = fontInfo->max_bounds.descent;
   int maxHeight = maxAscent + maxDescent;
   int maxWidth = fontInfo->max_bounds.width;

//     dPrintf("Font info:\n");
//     dPrintf("maxAscent: %d, maxDescent: %d, maxHeight: %d, maxWidth: %d\n",
//  	   maxAscent, maxDescent, maxHeight, maxWidth);

   // create the pixmap for rendering font characters
   int width = maxWidth;
   int height = maxHeight;
   int depth = 8; // don't need much depth here
   Pixmap pixmap = XCreatePixmap(display, DefaultRootWindow(display),
      width, height, depth);

   // create the gc and set rendering parameters
   GC gc = XCreateGC(display, pixmap, 0, 0);
   XSetFont(display, gc, fontInfo->fid);
   int screenNum = DefaultScreen(display);
   int white = WhitePixel(display, screenNum);
   int black = BlackPixel(display, screenNum);

   // create more stuff
   GFont *retFont = new GFont;
   static U8 scratchPad[65536];
   char textString[2];
   int x = 0;
   int y = 0;

   // insert bitmaps into the font for each character
   for(U16 i = 32; i < 256; i++)
   {
      // if the character is out of range, just insert an empty
      // bitmap with xIncr set to maxWidth
      if (i < fontInfo->min_char_or_byte2 ||
         i > fontInfo->max_char_or_byte2)
      {
         retFont->insertBitmap(i, scratchPad, 0, 0, 0, 0, 0, maxWidth);
         continue;
      }
      XCharStruct fontchar;
      int fontDirection;
      int fontAscent;
      int fontDescent;
      char charstr[2];
      dSprintf(charstr, 2, "%c", i);
      XTextExtents(fontInfo, charstr, 1, &fontDirection, &fontAscent,
		   &fontDescent, &fontchar);

      int charAscent = fontchar.ascent;
      int charDescent = fontchar.descent;
//       int charLBearing = fontchar.lbearing;
//       int charRBearing = fontchar.rbearing;
      int charWidth = fontchar.width; //charRBearing - charLBearing;
      int charHeight = charAscent + charDescent;
      int charXIncr = fontchar.width;

//        dPrintf("Char info: %c\n", (char)i);
//        dPrintf("charAscent: %d, charDescent: %d, charWidth: %d, charHeight: %d, charXIncr: %d\n", charAscent, charDescent, charWidth, charHeight, charXIncr);

      // clear the pixmap
      XSetForeground(display, gc, black);
      XFillRectangle(display, pixmap, gc, 0, 0, width, height);
      // draw the string in the pixmap
      XSetForeground(display, gc, white);
      dSprintf(textString, 2, "%c", static_cast<char>(i));
      XDrawString(display, pixmap, gc, 0, charAscent,
         textString, dStrlen(textString));

      // grab the pixmap image
      XImage *ximage = XGetImage(display, pixmap, 0, 0, width, height,
         AllPlanes, XYPixmap);
      if (ximage == NULL)
         AssertFatal(false, "cannot get x image");

      // grab each pixel and store it in the scratchPad
      for(y = 0; y < charHeight; y++)
      {
         for(x = 0; x < charWidth; x++)
         {
            U8 val = static_cast<U8>(XGetPixel(ximage, x, y));
            scratchPad[y * charWidth + x] = val;
         }
      }

      // we're done with the image
      XDestroyImage(ximage);

      // insert the bitmap
      retFont->insertBitmap(i, scratchPad, charWidth, charWidth, charHeight,
         0, charAscent, charXIncr);
   }

   // pack the font
   retFont->pack(fontInfo->ascent + fontInfo->descent, fontInfo->ascent);

   // free up stuff
   // GC
   XFreeGC(display, gc);
   // PIXMAP
   XFreePixmap(display, pixmap);
   // FONT
   XFreeFont(display, fontInfo);
   // DISPLAY
   XCloseDisplay(display);

   return retFont;
}
