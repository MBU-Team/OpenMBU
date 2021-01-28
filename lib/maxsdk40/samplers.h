/////////////////////////////////////////////////////////////////////////
//
//
//	Sampler Plug-Ins
//
//	Created 11/30/98	Kells Elmquist
//

#ifndef	SAMPLERS_H
#define SAMPLERS_H

// Default Sampler ClassId
#define R25_SAMPLER_CLASS_ID			0x25773211
#define DEFAULT_SAMPLER_CLASS_ID		R25_SAMPLER_CLASS_ID


typedef	ULONG	MASK[2];

class ShadeContext;

typedef SFXParamDlg SamplerParamDlg;

class SamplingCallback : public InterfaceServer {
	public:
//	virtual BOOL SampleAtOffset( Color &col, Color &trans, Point2& sample, float sampleScale )=0;
	virtual BOOL SampleAtOffset( ShadeOutput* pOut, Point2& sample, float sampleScale )=0;
};

class StdMat2;

class Sampler : public SpecialFX {
	public:
		RefResult NotifyRefChanged( Interval changeInt, 
									RefTargetHandle hTarget, 
							        PartID& partID, 
									RefMessage message ) { return REF_SUCCEED; }

		SClass_ID SuperClassID() { return SAMPLER_CLASS_ID; }
		
		// Saves and loads name. These should be called at the start of
		// a plug-in's save and load methods.
		IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
		IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

		// this samples a set of points over the area
		// this v3 call replaced by the more general v4 call below
//		virtual void DoSamples( Color& c, Color& t, SamplingCallback* cb, 
//			ShadeContext* sc, MASK mask=NULL ){};

		// this is the revised method. use this for all new samplers
		// if method supported, return TRUE;
		virtual void DoSamples( ShadeOutput* pOut, SamplingCallback* cb, ShadeContext* sc, 
								MASK mask=NULL )=0;
	
		// integer number of samples for current quality
		virtual int GetNSamples()=0;	//what return when adaptive? n for max quality.

		// This is the one default parameter
		// Quality is nominal, 0...1, 
		// 0 is one sample, high about .75, 1.0 shd be awesome
		// for adaptive samplers, this sets the maximum quality
		virtual void SetQuality( float value )=0;
		virtual float GetQuality()=0;
		// returns 0 on "unchangeable", otherwise n quality levels
		virtual int SupportsQualityLevels()=0;

		virtual void SetEnable( BOOL samplingOn )=0;
		virtual BOOL GetEnable()=0;

		virtual TCHAR* GetDefaultComment()=0;

		// Adaptive Sampling, non-reqd methods
		// there are various optional params, this defines which ones to show/enable
		virtual ULONG SupportsStdParams(){ return 0; }

		// this determines whether to cut down the texture sample size of each sample, 
		// or whether to always use 1 pixel texture sample size
		virtual void SetTextureSuperSampleOn( BOOL on ){}
		virtual BOOL GetTextureSuperSampleOn(){ return FALSE; }

		virtual void SetAdaptiveOn( BOOL on ){}
		virtual BOOL IsAdaptiveOn(){ return FALSE; }

		virtual void SetAdaptiveThreshold( float value ){}
		virtual float GetAdaptiveThreshold(){ return 0.0f; }

		// there are 2 optional 0.0...max parameters, for whatever
		virtual long GetNOptionalParams(){ return 0; }
		virtual TCHAR * GetOptionalParamName( long nParam ){ return _T(""); }
		virtual float GetOptionalParamMax( long nParam ){ return 1.0f; }
		virtual float GetOptionalParam( long nParam ){ return 0.0f; };
		virtual void SetOptionalParam( long nParam, float val ){};

		// Put up a modal pop-up dialog that allows editing the sampler extended
		virtual void ExecuteParamDialog(HWND hWndParent, StdMat2* mtl ){}

		// Implement this if you are using the ParamMap2 AUTO_UI system and the 
		// effect has secondary dialogs that don't have the sampler as their 'thing'.
		// Called once for each secondary dialog for you to install the correct thing.
		// Return TRUE if you process the dialog, false otherwise.
		virtual BOOL SetDlgThing(EffectParamDlg* dlg) { return FALSE; }
	};

// There are the standard parameters for samplers
#define	IS_ADAPTIVE					0x1		// adaptive in some way
#define	ADAPTIVE_CHECK_BOX			0x2		// enable adaptive check box
#define	ADAPTIVE_THRESHOLD			0x4		// enable adaptive threshold spinner
#define	SUPER_SAMPLE_TEX_CHECK_BOX	0x8		// enable texture subsampling check box
#define	ADVANCED_DLG_BUTTON			0x10	// enable advanced button
#define	OPTIONAL_PARAM_0			0x20	// enable optional spinner
#define	OPTIONAL_PARAM_1			0x40	// enable optional spinner

#define	R3_ADAPTIVE					(IS_ADAPTIVE+ADAPTIVE_CHECK_BOX+ADAPTIVE_THRESHOLD)		

// Chunk IDs saved by base class
#define SAMPLERBASE_CHUNK	0x39bf
#define SAMPLERNAME_CHUNK	0x0100
#define SAMPLER_VERS_CHUNK  0x0200



#endif