/**********************************************************************
 *<
	FILE: gport.h

	DESCRIPTION: Palette management.

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __GPORT__H
#define __GPORT__H


class GPort {
	public:
		// get the palette index associated with the ith slot
		virtual int AnimPalIndex(int i)=0;
	
		// returns a slot number if available, -1 if not:
		// typically called in WM_INITDIALOG processing for as
		// may slots as you need (total availible is 8)
		virtual int GetAnimPalSlot()=0;

		// Release an animated palete slot slot 
		// Typically called in WM_DESTROY for each slot
		// obtained with GetAnimPalSlot
		virtual void ReleaseAnimPalSlot(int i)=0;

		// set the color associated with the ith animated slot
		virtual void SetAnimPalEntry(int i, COLORREF cr)=0;

		// Stuff the standard MAX palette the palette for the HDC,
		// handing back a handle to the old palette.
		virtual HPALETTE PlugPalette(HDC hdc)=0;

		// Create a brush for drawing with the ith animated palette slot color
		virtual HBRUSH MakeAnimBrush(int slotNum, COLORREF col )=0;

		// Update colors calls the Windows UpdateColors on the hdc.
		// Returns 1 iff it changed screen pixel values .
	 	// Call this when get WM_PALETTECHANGED Msg
		virtual int UpdateColors(HDC hdc)=0;

		// After several SetAnimPalEntry calls, call this to affect the
		// HDC's palette
		virtual void AnimPalette(HDC hdc)=0;

		// The companion function to PlugPalette.
		virtual void RestorePalette(HDC hDC,HPALETTE hOldPal)=0;

		// Map an single row of pixels 24 bit color to indices into 
		// the current GPort palette, applying a dither pattern.
		// This routine does NOT do gamma correction.
		// inp points to an array of width RGB triples.
		// outp is an array of width bytes.  x and y are necessary to 
		// establish dither pattern alignment.
		virtual void MapPixels(UBYTE* inp, UBYTE *outp, int x, int y, int width)=0;

		 
		// Display an array of 24bit colors in the HDC: if the current display is 8 bit
		//  it will display it (with dither) using in the GPort palette, otherwise it 
		//  will just blit to the screen. Does NOT do gamma correction.
		//   "drect" is the destination rectangle in the hdc.
		//   "map" points to an array of RGB triples, with bytesPerRow bytes on each scanline.
		//   "xsrc" and "ysrc" are the position within this source raster of the upper left
		//    corner of the rectangle to be copied..
		virtual void DisplayMap(HDC hdc, Rect& drect,int xsrc, int ysrc, UBYTE *map, int bytesPerRow)=0;
		
		// This version stretches the image (if src!=dest).
		//  "dest" is the destination rectangle in the hdc;
		//  "src" is the source rectangle in map.
		virtual void DisplayMap(HDC hdc, Rect& dest, Rect& src, UBYTE *map, int bytesPerRow)=0; 

		// DitherColorSwatch first gamma corrects Color c using the current
		// display gamma. In paletted modes, it will fill rectangle "r" with 
		// a dithered pattern  approximating Color c.  In 24 bit modes it just 
		// fills the rectange with c.
		virtual void DitherColorSwatch(HDC hdc, Rect& r, Color c)=0;

		// This attempts to use the animated color slot indicated by "slot"
		// to paint a rectangular color swatch. 
		// If slot is -1, it will uses DitherColorSwatch.  It does gamma correction.
		virtual void PaintAnimPalSwatch(HDC hdc, DWORD col, int slot, int left, int top, int right, int bottom)=0;

		// get the current GPort palette.
		virtual HPALETTE GetPalette()=0;
	};

// Normally this is the only one of these, and this gets you a pointer to it.
extern CoreExport GPort* GetGPort();


#endif // __GPORT__H
