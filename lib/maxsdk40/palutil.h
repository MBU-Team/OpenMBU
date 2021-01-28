/*******************************************************************
 *
 *    DESCRIPTION: PALUTIL.H
 *
 *    AUTHOR: D.Silva
 *
 *    HISTORY:    
 *
 *******************************************************************/

#ifndef PALUTIL_H_DEFINED
#define PALUTIL_H_DEFINED

//-- 256 color dithering-----------------------------------------------------
// For packing colors into 256 color paletted representation.
// Create one with BMMNewColorPacker
class ColorPacker {
   public:
      virtual void EnableDither(BOOL onoff)=0;  // default: MAX default
      virtual void PropogateErrorBetweenLines(BOOL onoff)=0;  // default ON; 
      virtual void PackLine( BMM_Color_64* in, BYTE *out, int w)=0;
      virtual void PackLine( BMM_Color_48* in, BYTE *out, int w)=0;
      virtual void DeleteThis()=0;
   };

// Get a color packer.  When done, be sure to call its DeleteThis();
UtilExport ColorPacker *NewColorPacker(
   int w,            // width of bitmap to be packed
   BMM_Color_48 *pal,   // palette to use
   int npal,         // number of entries in the palette
   BYTE* remap=NULL, // optional remap done at last stage.
   BOOL dither = FALSE
   );

//---------------------------------------------------------------------
// Color quantizer, for doing true-color to paletted conversion
//
class Quantizer {
   public:
      virtual int AllocHistogram(void)=0;
      virtual int Partition(BMM_Color_48 *pal, int palsize, BMM_Color_64 *forceCol)=0;
      virtual void AddToHistogram(BMM_Color_64 *image, int npix)=0;  
      virtual void AddToHistogram(BMM_Color_48 *image, int npix)=0;  
      virtual void AddToHistogram(BMM_Color_24 *image, int npix)=0;  
      virtual void DeleteThis()=0;
   };

UtilExport Quantizer *NewQuantizer();

#endif // PALUTIL_H_DEFINED
