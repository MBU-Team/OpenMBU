//////////////////////////////////////////////////////////////////////////////
//
//		Shader plug-ins
//
//		Created: 8/18/98 Kells Elmquist
//
#ifndef	SHADERS_H
#define SHADERS_H

#include "iparamb2.h"
#include "stdmat.h"

#define STD2_NMAX_TEXMAPS	24
#define		N_ID_CHANNELS	12		// number of ids in stdMat

class Shader;

#define OPACITY_PARAM	0

#define DEFAULT_SOFTEN	0.1f

/////////////////////////////////////////////////////////////////////////////
//
//	Shader param dialog
//
// Returned by a shader when it is asked to put up its rollup page.
class ShaderParamDlg : public ParamDlg {
	public:
		virtual Class_ID ClassID()=0;
		virtual void SetThing(ReferenceTarget *m)=0;
		virtual void SetThings( StdMat2* pMtl, Shader* pShader )=0;
		virtual ReferenceTarget* GetThing()=0;
		virtual Shader* GetShader()=0;
		virtual void SetTime(TimeValue t) {}		
		virtual void DeleteThis()=0;		
		virtual BOOL PanelProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )=0; 
		virtual void LoadDialog( int draw )=0;
		virtual void UpdateDialog( ParamID paramId )=0;
		virtual HWND GetHWnd()=0;
		virtual int  FindSubTexFromHWND(HWND hw)=0;
		virtual void UpdateOpacity()=0;
		virtual void UpdateMapButtons()=0;
};
 
///////////////////////////////////sh flags //////////////////////////////////////
#define SELFILLUM_CLR_ON	(1<<16) // can be or'd w/ mtl, not sure it's necessary

// post mapping params for shader
class IllumParams {
public:
	ULONG mtlFlags;
	ULONG shFlags;
	Point3 N, V;
	Color channels[ STD2_NMAX_TEXMAPS ];

	float falloffOpac;		// textured opacity w/ stdfalloff (reg opac in channels)
	float kR;				// combined reflection.a * amt 
	ULONG hasComponents;	// bits for active components(e.g.currently has active refl map)
	ULONG stdParams;
	int*  stdIDToChannel;	// combined shader & mtl

	Color ambIllumOut, diffIllumOut, transIllumOut, selfIllumOut; // the diffuse clrs
	Color specIllumOut, reflIllumOut;	// the specular colors
	float diffIllumIntens; // used only by reflection dimming, intensity of diffIllum prior to color multiply
	float finalOpac; // for combining components
	Color finalC;	// final clr: combiner composites into this value.
	Color finalT;	// shader transp clr out

public:
	void ClearOutputs() { 
		finalC = finalT = ambIllumOut=diffIllumOut=transIllumOut=selfIllumOut=
		specIllumOut=reflIllumOut= Color( 0.0f, 0.0f, 0.0f ); 
		finalOpac = diffIllumIntens = 0.0f;
	}

};

/////////// Components defines
#define HAS_BUMPS				0x01L
#define HAS_REFLECT				0x02L
#define HAS_REFRACT				0x04L
#define HAS_OPACITY				0x08L
#define HAS_REFLECT_MAP			0x10L
#define HAS_REFRACT_MAP			0x20L


////////// Texture channel type flags
#define UNSUPPORTED_CHANNEL		0x01L
#define CLR_CHANNEL				0x02L
#define MONO_CHANNEL			0x04L
#define BUMP_CHANNEL			0x08L
#define REFL_CHANNEL			0x10L
#define REFR_CHANNEL			0x20L
#define DISP_CHANNEL			0x40L
#define ELIMINATE_CHANNEL		0x8000L

#define SKIP_CHANNELS	(UNSUPPORTED_CHANNEL+BUMP_CHANNEL+REFL_CHANNEL+REFR_CHANNEL)

/////////// Class Id upper half for loading the Pre 3.0 shaders
#define  DEFAULT_SHADER_CLASS_ID BLINNClassID 

#define  PHONGClassID (STDSHADERS_CLASS_ID+2)
#define  BLINNClassID (STDSHADERS_CLASS_ID+3)
#define  METALClassID (STDSHADERS_CLASS_ID+4)

class ParamBlockDescID;
class IParamBlock;

class BaseShader : public SpecialFX {
	public:
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message) {return REF_SUCCEED;}

		SClass_ID SuperClassID() {return SHADER_CLASS_ID;}
		BOOL BypassPropertyLevel() { return TRUE; }  // want to promote shader props to material level

		virtual ULONG GetRequirements(int subMtlNum)=0;

		// Put up a dialog that lets the user edit the plug-ins parameters.
		virtual ShaderParamDlg* CreateParamDialog(	HWND hOldRollup, HWND hwMtlEdit, 
													IMtlParams *imp, 
													StdMat2* theMtl, 
													int rollupOpen )=0;

		virtual ShaderParamDlg* GetParamDlg()=0;
		virtual void SetParamDlg( ShaderParamDlg* newDlg )=0;

		// Saves and loads name. These should be called at the start of
		// a plug-in's save and load methods.
		IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
		IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

		// std parameter support
		virtual ULONG SupportStdParams()=0;

		// this method only req'd for R2.5 shaders, to convert stdmtl1 paramblks to current
		virtual void ConvertParamBlk( ParamBlockDescID *descOld, int oldCount, IParamBlock *oldPB ){};

		// LOCAL vars of mtl for possible mapping prior to being given to back to illum
		virtual void GetIllumParams( ShadeContext &sc, IllumParams& ip )=0;

		// actual shader
		virtual void Illum(ShadeContext &sc, IllumParams &ip)=0;
		virtual void AffectReflection(ShadeContext &sc, IllumParams &ip, Color &rcol)=0;
		virtual void CombineComponents( ShadeContext &sc, IllumParams& ip )=0;

		// texture maps
		virtual long nTexChannelsSupported()=0;
		virtual TSTR GetTexChannelName( long nTextureChan )=0;
		virtual TSTR GetTexChannelInternalName( long nTextureChan ) { return GetTexChannelName(nTextureChan); }
		virtual long ChannelType( long nTextureChan )=0;
		// map StdMat Channel ID's to the channel number
		virtual long StdIDToChannel( long stdID )=0;

		virtual void Reset()=0;	//reset to default values

	};

// Chunk IDs saved by base class
#define SHADERBASE_CHUNK	0x39bf
#define SHADERNAME_CHUNK	0x0100


/////////////////////////////////////////////////////////////////////////////
//
//	Standard params for shaders
//
// combination of these is returned by Shader.SupportStdParams()
#define STD_PARAM_NONE			(0)
#define STD_PARAM_ALL			(0xffffffffL)
#define STD_PARAM_METAL			(1)
#define STD_PARAM_LOCKDS		(1<<1)
#define STD_PARAM_LOCKAD		(1<<2)
#define STD_PARAM_LOCKADTEX		(1<<3)
#define STD_PARAM_SELFILLUM		(1<<4)
#define STD_PARAM_SELFILLUM_CLR	(1<<5)
#define STD_PARAM_AMBIENT_CLR	(1<<6)
#define STD_PARAM_DIFFUSE_CLR	(1<<7)
#define STD_PARAM_SPECULAR_CLR	(1<<8)
#define STD_PARAM_FILTER_CLR	(1<<9)
#define STD_PARAM_GLOSSINESS	(1<<10)
#define STD_PARAM_SOFTEN_LEV	(1<<11)
#define STD_PARAM_SPECULAR_LEV	(1<<12)
#define STD_PARAM_DIFFUSE_LEV	(1<<13)
#define STD_PARAM_DIFFUSE_RHO	(1<<14)
#define STD_PARAM_ANISO			(1<<15)
#define STD_PARAM_ORIENTATION	(1<<16)
#define STD_PARAM_REFL_LEV		(1<<17)
#define STD_PARAM_SELFILLUM_CLR_ON		(1<<18)

#define STD_BASIC2_DLG			(1<<20)
#define STD_EXTRA_DLG			(1<<21)

// not including these 3 in yr param string disables the relevant params 
// in extra params dialog
#define STD_EXTRA_REFLECTION	(1<<22)
#define STD_EXTRA_REFRACTION	(1<<23)
#define STD_EXTRA_OPACITY		(1<<24)

#define STD_EXTRA	(STD_EXTRA_DLG \
					+STD_EXTRA_REFLECTION+STD_EXTRA_REFRACTION \
					+STD_EXTRA_OPACITY )

#define STD_BASIC	(0x00020ffeL | STD_BASIC2_DLG)
#define STD_BASIC_METAL	(0x00020fffL | STD_BASIC2_DLG)

#define STD_ANISO	(0x0002cffe)
#define STD_MULTILAYER	(0x0002fffe)
#define STD_ONB		(0x00023ffe)
#define STD_WARD	(0x00000bce)


///////////////////////////////////////////////////////////////////////////////

class Shader : public BaseShader {
	public:
	virtual void CopyStdParams( Shader* pFrom )=0;
	// these are the standard shader params
	virtual void SetLockDS(BOOL lock)=0;
	virtual BOOL GetLockDS()=0;
	virtual void SetLockAD(BOOL lock)=0;
	virtual BOOL GetLockAD()=0;
	virtual void SetLockADTex(BOOL lock)=0;
	virtual BOOL GetLockADTex()=0;

	virtual void SetSelfIllum(float v, TimeValue t)=0;		
	virtual void SetSelfIllumClrOn( BOOL on )=0;
	virtual void SetSelfIllumClr(Color c, TimeValue t)=0;		

	virtual void SetAmbientClr(Color c, TimeValue t)=0;		
	virtual void SetDiffuseClr(Color c, TimeValue t)=0;		
	virtual void SetSpecularClr(Color c, TimeValue t)=0;
	virtual void SetGlossiness(float v, TimeValue t)=0;		
	virtual void SetSpecularLevel(float v, TimeValue t)=0;		
	virtual void SetSoftenLevel(float v, TimeValue t)=0;
		
	virtual BOOL IsSelfIllumClrOn(int mtlNum, BOOL backFace)=0;
	virtual Color GetAmbientClr(int mtlNum, BOOL backFace)=0;		
    virtual Color GetDiffuseClr(int mtlNum, BOOL backFace)=0;		
	virtual Color GetSpecularClr(int mtlNum, BOOL backFace)=0;
	virtual Color GetSelfIllumClr(int mtlNum, BOOL backFace)=0;
	virtual float GetSelfIllum(int mtlNum, BOOL backFace)=0;
	virtual float GetGlossiness(int mtlNum, BOOL backFace)=0;	
	virtual float GetSpecularLevel(int mtlNum, BOOL backFace)=0;
	virtual float GetSoftenLevel(int mtlNum, BOOL backFace)=0;

	virtual BOOL IsSelfIllumClrOn()=0;
	virtual Color GetAmbientClr(TimeValue t)=0;		
	virtual Color GetDiffuseClr(TimeValue t)=0;		
	virtual Color GetSpecularClr(TimeValue t)=0;
	virtual float GetGlossiness( TimeValue t)=0;		
	virtual float GetSpecularLevel(TimeValue t)=0;
	virtual float GetSoftenLevel(TimeValue t)=0;
	virtual float GetSelfIllum(TimeValue t)=0;		
	virtual Color GetSelfIllumClr(TimeValue t)=0;		

	virtual float EvalHiliteCurve(float x){ return 0.0f; }
	virtual float EvalHiliteCurve2(float x, float y, int level = 0 ){ return 0.0f; }

};



#endif