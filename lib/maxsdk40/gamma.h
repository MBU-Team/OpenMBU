/**********************************************************************
 *<
	FILE:  gamma.h

	DESCRIPTION:  Gamma utilities

	CREATED BY: Dan Silva

	HISTORY: created 26 December 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/


#ifndef __GAMMA__H
#define __GAMMA__H

#define WRDMAX 65535
#define FWRDMAX 65535.0f

#define RCBITS 13   // number of bits used to represent colors before gamma correction.
					// this keeps the lookup table a reasonable size
#define RCOLN (1<<RCBITS)    
#define RCMAX (RCOLN-1)	
#define FRCMAX ((float)RCMAX) 
#define RCHALF (RCOLN>>1)
#define RCSH (RCBITS-8)		  /* shift amount from 8 bit to RCBITS */
#define RCSH16 (16-RCBITS)	  /* shift amount from 16 bit to RCBITS */
#define RCFRACMASK ((ulong)((1<<RCSH)-1))	  
#define RC_SCL (1<<RCSH)
#define RC_SCLHALF (1<<(RCSH-1))
#define FRC_SCL ((float)RC_SCL)
#define RCSHMASK (0xffffffffL<<RCSH)
#define RCSHMAX (0xffL<<RCSH)


#define GAMMA_NTSC		2.2f
#define	GAMMA_PAL		2.8f

class GammaMgr {
	public:
		BOOL enable;
		BOOL dithTrue;
		BOOL dithPaletted;
		float dispGamma;
		float fileInGamma;
		float fileOutGamma;
		UBYTE disp_gamtab[256];    	  // (8->8) display gamma for drawing color swatches (8->8) 
		UBYTE disp_gamtabw[RCOLN];    // (RCBITS->8) display gamma 
		UBYTE file_in_gamtab[256];    // (8->8) 
		UWORD file_in_degamtab[256];  // (8->16)  for de-gamifying bitmaps on input
		UWORD file_out_gamtab[RCOLN]; // (RCBITS->16) gamma correct for file output, before dither 

		inline COLORREF DisplayGammaCorrect(COLORREF col) {
			return RGB(disp_gamtab[GetRValue(col)],	disp_gamtab[GetGValue(col)], disp_gamtab[GetBValue(col)]);		
			}

		CoreExport Color DisplayGammaCorrect(Color c);
			
		CoreExport void Enable(BOOL onOff);
		BOOL IsEnabled() { return enable;}

		CoreExport void  SetDisplayGamma(float gam);
		float GetDisplayGamma() { return dispGamma; }

		CoreExport void SetFileInGamma(float gam);
		float GetFileInGamma() { return fileInGamma; }

		CoreExport void SetFileOutGamma(float gam);
		float GetFileOutGamma() { return fileOutGamma; }

		GammaMgr();


	};

CoreExport extern GammaMgr gammaMgr;


inline COLORREF gammaCorrect(DWORD c) { return gammaMgr.DisplayGammaCorrect(c); }
inline UBYTE gammaCorrect(UBYTE b) { return gammaMgr.disp_gamtab[b]; }


#define GAMMA16to8(b)  gammaMgr.disp_gamtabw[b>>RCSH16]

// Build Gamma table that maps 8->8  
CoreExport void BuildGammaTab8(UBYTE gamtab[256], float gamma, int onoff=TRUE);

// Build a Gamma table that maps 8->16
CoreExport void BuildGammaTab8(UWORD gamtab[256], float gamma, int onoff=TRUE);

// Build Gamma table that maps RCBITS->8
CoreExport void BuildGammaTab(UBYTE gamtab[RCOLN], float gamma, int onoff=TRUE);

// Build Gamma table that  maps RCBITS->16
CoreExport void BuildGammaTab(UWORD gamtab[RCOLN], float gamma, int onoff=TRUE);

CoreExport float gammaCorrect(float v, float gamma);
CoreExport float deGammaCorrect(float v, float gamma);
CoreExport UBYTE gammaCorrect(UBYTE v, float gamma);
CoreExport UBYTE deGammaCorrect(UBYTE v, float gamma);
CoreExport UWORD gammaCorrect(UWORD c, float gamma);
CoreExport UWORD deGammaCorrect(UWORD c, float gamma);


// Temporary table for converting 16->16.
class GamConvert16 {
	float gamma;
	UWORD* gtab;
	public:	
		GamConvert16(float gam=1.0f);  
		~ GamConvert16();  
		void SetGamma(float gam);
		UWORD Convert(UWORD v) { return gtab[v>>RCSH16]; }

	};

// Temporary table for converting 8->16.
class GamConvert8 {
	float gamma;
	UWORD gtab[256];
	public:	
		GamConvert8(float gam=1.0f);  
		void SetGamma(float gam);
		UWORD Convert(UBYTE v) { return gtab[v]; }

	};

#endif
