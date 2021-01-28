//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformMacCarb/macCarbFont.h"
#include "platformMacCarb/platformMacCarb.h"
#include "dgl/gFont.h"
#include "dgl/gBitmap.h"
#include "Math/mRect.h"
#include "console/console.h"
#include "core/unicode.h"

static GWorldPtr fontgw = NULL;
static Rect fontrect = {0,0,256,256};
static short fontdepth = 32;
static U8 rawmap[256*256];


void createFontInit(void);
void createFontShutdown(void);
S32 CopyCharToRaw(U8 *raw, PixMapHandle pm, const Rect &r);

// !!!!! TBD this should be returning error, or raising exception...
void createFontInit()
{
   OSErr err = NewGWorld(&fontgw, fontdepth, &fontrect, NULL, NULL, keepLocal);
   AssertWarn(err==noErr, "Font system failed to initialize GWorld.");
}
 
void createFontShutdown()
{
   DisposeGWorld(fontgw);
   fontgw = NULL;
}

U8 ColorAverage8(RGBColor &rgb)
{
   return ((rgb.red>>8) + (rgb.green>>8) + (rgb.blue>>8)) / 3;
}

S32 CopyCharToRaw(U8 *raw, PixMapHandle pmh, const Rect &r)
{
   // walk the pixmap, copying into raw buffer.
   // we want bg black, fg white, which is opposite 'sign' from the pixmap (bg white, with black text)
   
//   int off = GetPixRowBytes(pmh);
//   U32 *c = (U32*)GetPixBaseAddr(pmh);
//   U32 *p;

   RGBColor rgb;

   S32 i, j;
   U8 val, ca;
   S32 lastRow = -1;
   
   for (i = r.left; i <= r.right; i++)
   {
      for (j = r.top; j <= r.bottom; j++)
      {
//         p = (U32*)(((U8*)c) + (j*off)); // since rowbytes is bytes not pixels, need to convert to byte * before doing the math...
         val = 0;
//         if (((*p)&0x00FFFFFF)==0) // then black pixel in current port, so want white in new buffer.
         GetCPixel(i, j, &rgb);
         if ((ca = ColorAverage8(rgb))<=250) // get's us some grays with a small slop factor.
         {
            val = 255 - ca; // flip sign, basically.
            lastRow = j; // track the last row in the rect that we actually saw an active pixel, finding descenders basically...
         }
         raw[i + (j<<8)] = val;
//         p++;
      }
   }
   
   return(lastRow);
}

GOldFont* createFont(const char *name, dsize_t size, U32 charset)
{
   if(!name)
      return NULL;
   if(size < 1)
      return NULL;

   bool wantsBold = false;
   short fontid;
   GetFNum(str2p(name), &fontid);
   if (fontid == 0)
   {
      // hmmm... see if it has "Bold" on the end.  if so, remove it and try again.
      int len = dStrlen(name);
      if (len>4 && 0==dStricmp(name+len-4, "bold"))
      {
         char substr[128];
         dStrcpy(substr, name);
         len -= 5;
         substr[len] = 0; // new null termination.
         GetFNum(str2p(substr), &fontid);
         wantsBold = true;
      }

      if (fontid == 0)
      {
         Con::errorf(ConsoleLogEntry::General,"Error creating font [%s (%d)] -- it doesn't exist on this machine.",name, size);
         return(NULL);
      }
   }

   Boolean aaWas;
   S16 aaSize;
   CGrafPtr savePort;
   GDHandle saveGD;
   GetGWorld(&savePort, &saveGD);
   
   aaWas = IsAntiAliasedTextEnabled(&aaSize);
   SetAntiAliasedTextEnabled(true, 6);
   
   RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
   RGBColor black = {0, 0, 0};
   PixMapHandle pmh;

   SetGWorld(fontgw, NULL);
   PenNormal(); // reset pen attribs.
   // shouldn't really need to do this, right?
   RGBBackColor(&white);
   RGBForeColor(&black);

   // set proper font.
   // mac fonts are coming out a bit bigger than PC - think PC is like 80-90dpi comparatively, vs 72dpi.
   // so, let's tweak here.  20=>16. 16=>13. 12=>9. 10=>7.
   TextSize(size - 2 - (int)((float)size * 0.1));
   TextFont(fontid);
   TextMode(srcCopy);
   if (wantsBold)
      TextFace(bold);

   // get font info
   int cx, cy, ry, my;
   FontInfo fi;
   GetFontInfo(&fi); // gets us basic glyphs.
   cy = fi.ascent + fi.descent + fi.leading + 1; // !!!! HACK.  Not per-char-specific.
   
   pmh = GetGWorldPixMap(fontgw);

   GOldFont *retFont = new GOldFont;

   Rect b = {0,0,0,0};
   int drawBase = fi.ascent+1;
   int outBase = fi.ascent+fi.descent-1;
   for(S32 i = 32; i < 256; i++)
   {
      if (!LockPixels(pmh))
      {
         UpdateGWorld(&fontgw, fontdepth, &fontrect, NULL, NULL, keepLocal);
         pmh = GetGWorldPixMap(fontgw);
         // for now, assume we're good to go... TBD!!!!
         LockPixels(pmh);
      }
      
      // clear the port.
      EraseRect(&fontrect);
      // reset position to left edge, bottom of line for this font style.
      MoveTo(0, drawBase);
      // draw char & calc its pixel width.
      DrawChar(i);
      cx = CharWidth(i);      

      b.right = cx+1;
      b.bottom = cy+1; // in case descenders drop too low, we want to catch the chars.
      ry = CopyCharToRaw(rawmap, pmh, b);

      UnlockPixels(pmh);

      if (ry<0) // bitmap was blank.
      {
         Con::printf("Blank character %c", i);
         if (cx)
            retFont->insertBitmap(i, rawmap, 0, 0, 0, 0, 0, cx);
      }
      else
         retFont->insertBitmap(i, rawmap, 256, cx+1, cy+1, 0, outBase, cx);
   }

   retFont->pack(cy, outBase);

//   if (actualChars==0)
//      Con::errorf(ConsoleLogEntry::General,"Error creating font: %s %d",name, size);

   //clean up local vars
   if (aaWas)
      SetAntiAliasedTextEnabled(aaWas, aaSize);
   SetGWorld(savePort, saveGD);
   
   return retFont;
}

//------------------------------------------------------------------------------
// New Unicode capable font class.
PlatformFont *createPlatformFont(const char *name, U32 size, U32 charset /* = TGE_ANSI_CHARSET */)
{
    PlatformFont *retFont = new MacCarbFont;

    if(retFont->create(name, size, charset))
        return retFont;

    delete retFont;
    return NULL;
}

//------------------------------------------------------------------------------
MacCarbFont::MacCarbFont()
{
   mStyle   = NULL;
   mLayout  = NULL;
   mColorSpace = NULL;
}

MacCarbFont::~MacCarbFont()
{
   // apple docs say we should dispose the layout first.
   ATSUDisposeTextLayout(mLayout);
   ATSUDisposeStyle(mStyle);
   CGColorSpaceRelease(mColorSpace);
}

//------------------------------------------------------------------------------
bool MacCarbFont::create( const char *name, U32 size, U32 charset)
{
   // create and cache the style and layout.
   // based on apple sample code at http://developer.apple.com/qa/qa2001/qa1027.html

   // note: charset is ignored on mac. -- we don't need it to get the right chars.
   // But do we need it to translate encodings? hmm...

   CFStringRef       cfsName;
   ATSUFontID        atsuFontID;
   ATSFontRef        atsFontRef;
   Fixed             atsuSize;
   ATSURGBAlphaColor black;
   ATSFontMetrics    fontMetrics;
   U32               scaledSize;

   // Look up the font. We need it in 2 differnt formats, for differnt Apple APIs.
   cfsName = CFStringCreateWithCString( kCFAllocatorDefault, name, kCFStringEncodingUTF8);
   atsFontRef =  ATSFontFindFromName( cfsName, kATSOptionFlagsDefault);
   atsuFontID = FMGetFontFromATSFontRef( atsFontRef);

   // make sure we found it. ATSFontFindFromName() appears to return 0 if it cant find anything. Apple docs contain no info on error returns.
   if( !atsFontRef || !atsuFontID )
   {
      Con::errorf("Error: Could not load font -%s-",name);
      return false;
   }

   // adjust the size. win dpi = 96, mac dpi = 72. 72/96 = .75
   // Interestingly enough, 0.75 is not what makes things the right size.
   scaledSize = size - 2 - (int)((float)size * 0.1);
   mSize = scaledSize;
   
   // Set up the size and color. We send these to ATSUSetAttributes().
   atsuSize = IntToFixed(scaledSize);
   black.red = black.green = black.blue = black.alpha = 1.0;

   // Three parrallel arrays for setting up font, size, and color attributes.
   ATSUAttributeTag theTags[] = { kATSUFontTag, kATSUSizeTag, kATSURGBAlphaColorTag};
   ByteCount theSizes[] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(ATSURGBAlphaColor) };
   ATSUAttributeValuePtr theValues[] = { &atsuFontID, &atsuSize, &black };

   // create and configure the style object.
   ATSUCreateStyle(&mStyle);
   ATSUSetAttributes( mStyle, 3, theTags, theSizes, theValues );
   
   // create the layout object, 
   ATSUCreateTextLayout(&mLayout);  
   // we'll bind the layout to a bitmap context when we actually draw.
   // ATSUSetTextPointerLocation()  - will set the text buffer
   // ATSUSetLayoutControls()       - will set the cg context.
   
   // get font metrics, save our baseline and height
   ATSFontGetHorizontalMetrics(atsFontRef, kATSOptionFlagsDefault, &fontMetrics);
   mBaseline = scaledSize * fontMetrics.ascent;
   mHeight   = scaledSize * ( fontMetrics.ascent - fontMetrics.descent + fontMetrics.leading ) + 1;
   
   // cache our grey color space, so we dont have to re create it every time.
   mColorSpace = CGColorSpaceCreateDeviceGray();
   
   // and finally cache the font's name. We use this to cheat some antialiasing options below.
   mName = StringTable->insert(name);
   
   return true;
}    

//------------------------------------------------------------------------------
bool MacCarbFont::isValidChar(const UTF8 *str) const
{
   return isValidChar(oneUTF32toUTF16(oneUTF8toUTF32(str,NULL)));
   
}

bool MacCarbFont::isValidChar( const UTF16 ch) const
{
   // The expected behavior of this func is not well documented by the windows version, 
   //  and on the win side, is highly dependant on the font and code page.
   // Cutting out all the ASCII control chars seems like the right thing to do...
   //  if you find a bug related to this assumption, please post a report.

   // 0x20 == 32 == space
   if( ch < 0x20 )
      return false;

   return true;
}

PlatformFont::CharInfo& MacCarbFont::getCharInfo(const UTF8 *str) const
{
   return getCharInfo(oneUTF32toUTF16(oneUTF8toUTF32(str,NULL)));
}

PlatformFont::CharInfo& MacCarbFont::getCharInfo(const UTF16 ch) const
{
   // We use some static data here to avoid re allocating the same variable in a loop.
   // this func is primarily called by GFont::loadCharInfo(),
   Rect                 imageRect;
   CGContextRef         imageCtx;
   U32                  bitmapDataSize;
   ATSUTextMeasurement  tbefore, tafter, tascent, tdescent;
   OSStatus             err;

   // 16 bit character buffer for the ATUSI calls.
   // -- hey... could we cache this at the class level, set style and loc *once*, 
   //    then just write to this buffer and clear the layout cache, to speed up drawing?
   static UniChar chUniChar[1];
   chUniChar[0] = ch;

   // Declare and clear out the CharInfo that will be returned.
   static PlatformFont::CharInfo c;
   dMemset(&c, 0, sizeof(c));
   
   // prep values for GFont::addBitmap()
   c.bitmapIndex = 0;
   c.xOffset = 0;
   c.yOffset = 0;

   // put the text in the layout.
   // we've hardcoded a string length of 1 here, but this could work for longer strings... (hint hint)
   // note: ATSUSetTextPointerLocation() also clears the previous cached layout information.
   ATSUSetTextPointerLocation( mLayout, chUniChar, 0, 1, 1);
   ATSUSetRunStyle( mLayout, mStyle, 0,1);
   
   // get the typographic bounds. this tells us how characters are placed relative to other characters.
   ATSUGetUnjustifiedBounds( mLayout, 0, 1, &tbefore, &tafter, &tascent, &tdescent);
   c.xIncrement =  FixedToInt(tafter);
   
   // find out how big of a bitmap we'll need.
   // as a bonus, we also get the origin where we should draw, encoded in the Rect.
   ATSUMeasureTextImage( mLayout, 0, 1, 0, 0, &imageRect);
   c.width  = imageRect.right - imageRect.left + 2; // add 2 because small fonts don't always have enough room
   c.height = imageRect.bottom - imageRect.top;
   c.xOrigin = imageRect.left; // dist x0 -> center line
   c.yOrigin = -imageRect.top; // dist y0 -> base line
   
   // kick out early if the character is undrawable
   if( c.width == 0 || c.height == 0)
      return c;
   
   // allocate a greyscale bitmap and clear it.
   bitmapDataSize = c.width * c.height;
   c.bitmapData = new U8[bitmapDataSize];
   dMemset(c.bitmapData,0x00,bitmapDataSize);
   
   // get a graphics context on the bitmap
   imageCtx = CGBitmapContextCreate( c.bitmapData, c.width, c.height, 8, c.width, mColorSpace, kCGImageAlphaNone);
   if(!imageCtx) {
      Con::errorf("Error: failed to create a graphics context on the CharInfo bitmap! Drawing a blank block.");
      c.xIncrement = c.width;
      dMemset(c.bitmapData,0xa0F,bitmapDataSize);
      return c;
   }

   // Turn off antialiasing for monospaced console fonts. yes, this is cheating.
   if(mSize < 12  && ( dStrstr(mName,"Monaco")!=NULL || dStrstr(mName,"Courier")!=NULL ))
      CGContextSetShouldAntialias(imageCtx, false);

   // Set up drawing options for the context.
   // Since we're not going straight to the screen, we need to adjust accordingly
   CGContextSetShouldSmoothFonts(imageCtx, false);
   CGContextSetRenderingIntent(imageCtx, kCGRenderingIntentAbsoluteColorimetric);
   CGContextSetInterpolationQuality( imageCtx, kCGInterpolationNone);
   CGContextSetGrayFillColor( imageCtx, 1.0, 1.0);
   CGContextSetTextDrawingMode( imageCtx,  kCGTextFill);
   
   // tell ATSUI to substitute fonts as needed for missing glyphs
   ATSUSetTransientFontMatching(mLayout, true); 

   // set up three parrallel arrays for setting up attributes. 
   // this is how most options in ATSUI are set, by passing arrays of options.
   ATSUAttributeTag theTags[] = { kATSUCGContextTag };
   ByteCount theSizes[] = { sizeof(CGContextRef) };
   ATSUAttributeValuePtr theValues[] = { &imageCtx };
   
   // bind the layout to the context.
   ATSUSetLayoutControls( mLayout, 1, theTags, theSizes, theValues );

   // Draw the character!
   err = ATSUDrawText( mLayout, 0, 1, IntToFixed(-imageRect.left+1), IntToFixed( imageRect.bottom ) );
   CGContextRelease(imageCtx);
   
   if(err != noErr) {
      Con::errorf("Error: could not draw the character! Drawing a blank box.");
      dMemset(c.bitmapData,0x0F,bitmapDataSize);
   }


#if TORQUE_DEBUG
   //Con::printf("Font Metrics: Rect = %i %i %i %i Char= %C, 0x%x  Size= %i, Baseline= %i, Height= %i",imageRect.top, imageRect.bottom, imageRect.left, imageRect.right,ch,ch, mSize,mBaseline, mHeight);
   //Con::printf("Font Bounds: left= %i  right= %i  Char= %C, 0x%x  Size= %i",FixedToInt(tbefore), FixedToInt(tafter), ch,ch, mSize);
#endif
      
   return c;
}

//-----------------------------------------------------------------------------
// The following code snippet demonstrates how to get the elusive GlyphIDs, 
// which are needed when you want to do various complex and arcane things
// with ATSUI and CoreGraphics.
//
//  ATSUGlyphInfoArray glyphinfoArr;
//  ATSUGetGlyphInfo( mLayout, kATSUFromTextBeginning, kATSUToTextEnd,sizeof(ATSUGlyphInfoArray), &glyphinfoArr);
//  ATSUGlyphInfo glyphinfo = glyphinfoArr.glyphs[0];
//  Con::printf(" Glyphinfo: screenX= %i, idealX=%f, deltaY=%f", glyphinfo.screenX, glyphinfo.idealX, glyphinfo.deltaY);

