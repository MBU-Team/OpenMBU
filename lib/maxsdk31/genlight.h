/**********************************************************************
 *<
	FILE: genlight.h

	DESCRIPTION:  Defines General-Purpose lights

	CREATED BY: Tom Hudson

	HISTORY: created 5 December 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __GENLIGHT__H__ 

#define __GENLIGHT__H__


#define OMNI_LIGHT		0	// Omnidirectional
#define TSPOT_LIGHT		1	// Targeted
#define DIR_LIGHT		2	// Directional
#define FSPOT_LIGHT		3	// Free
#define TDIR_LIGHT		4   // Targeted directional

#define NUM_LIGHT_TYPES	5

#define DECAY_NONE  0
#define DECAY_INV   1
#define DECAY_INVSQ 2

// SetAtten types
#define ATTEN1_START   	0  // near
#define ATTEN1_END		1  // near
#define ATTEN_START		2  // far
#define ATTEN_END		3  // far

// Shapes
#define RECT_LIGHT		0
#define CIRCLE_LIGHT	1

class ShadowType;

class GenLight: public LightObject {			   
public:
	virtual GenLight *NewLight(int type)=0;
	virtual RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs)=0;
	virtual int Type()=0;  // OMNI_LIGHT, TSPOT_LIGHT, DIR_LIGHT, FSPOT_LIGHT, TDIR_LIGHT
	virtual void SetType(int tp) {} // OMNI_LIGHT, TSPOT_LIGHT, DIR_LIGHT, FSPOT_LIGHT, TDIR_LIGHT      
	virtual BOOL IsSpot()=0;
	virtual BOOL IsDir()=0;
	virtual void SetUseLight(int onOff)=0;
	virtual BOOL GetUseLight(void)=0;
	virtual void SetSpotShape(int s)=0;
	virtual int GetSpotShape(void)=0;
	virtual void SetHotspot(TimeValue time, float f)=0;
	virtual float GetHotspot(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetFallsize(TimeValue time, float f)=0;
	virtual float GetFallsize(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetAtten(TimeValue time, int which, float f)=0;
	virtual float GetAtten(TimeValue t, int which, Interval& valid = Interval(0,0))=0;
	virtual void SetTDist(TimeValue time, float f)=0;
	virtual float GetTDist(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual ObjLightDesc *CreateLightDesc(INode *n)=0;
	virtual void SetRGBColor(TimeValue t, Point3& rgb)=0;
	virtual Point3 GetRGBColor(TimeValue t, Interval &valid = Interval(0,0))=0;
	virtual void SetHSVColor(TimeValue t, Point3& hsv)=0;
	virtual Point3 GetHSVColor(TimeValue t, Interval &valid = Interval(0,0))=0;
	virtual void SetIntensity(TimeValue time, float f)=0;
	virtual float GetIntensity(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetContrast(TimeValue time, float f)=0;
	virtual float GetContrast(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetAspect(TimeValue t, float f)=0;
	virtual float GetAspect(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetConeDisplay(int s, int notify=TRUE)=0;
	virtual BOOL GetConeDisplay(void)=0;
	virtual void SetUseAtten(int s)=0;
	virtual BOOL GetUseAtten(void)=0;
	virtual void SetAttenDisplay(int s)=0;
	virtual BOOL GetAttenDisplay(void)=0;
	virtual void SetUseAttenNear(int s)=0;
	virtual BOOL GetUseAttenNear(void)=0;
	virtual void SetAttenNearDisplay(int s)=0;
	virtual BOOL GetAttenNearDisplay(void)=0;
	virtual void Enable(int enab)=0;
	virtual void SetMapBias(TimeValue t, float f)=0;
	virtual float GetMapBias(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetMapRange(TimeValue t, float f)=0;
	virtual float GetMapRange(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetMapSize(TimeValue t, int f)=0;
	virtual int GetMapSize(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetRayBias(TimeValue t, float f)=0;
	virtual float GetRayBias(TimeValue t, Interval& valid = Interval(0,0))=0;

	virtual int GetUseGlobal()=0;
	virtual void SetUseGlobal(int a)=0;
	virtual int GetShadow()=0;
	virtual void SetShadow(int a)=0;

	virtual int GetShadowType()=0;
	virtual void SetShadowType(int a)=0;

	// Pluggable Shadow generator (11/2/98): 
	virtual	void SetShadowGenerator(ShadowType *s) {};
	virtual ShadowType *GetShadowGenerator() { return NULL; } 

	virtual int GetAbsMapBias()=0;
	virtual void SetAbsMapBias(int a)=0;

	virtual void SetAtmosShadows(TimeValue t, int onOff) {}
	virtual int GetAtmosShadows(TimeValue t) { return 0; }
	virtual void SetAtmosOpacity(TimeValue t, float f) {}
	virtual float GetAtmosOpacity(TimeValue t, Interval& valid=FOREVER) { return 0.0f; }
	virtual void SetAtmosColAmt(TimeValue t, float f) {}
	virtual float GetAtmosColAmt(TimeValue t, Interval& valid=FOREVER) { return 0.0f; }
	virtual void SetUseShadowColorMap(TimeValue t, int onOff) { }
	virtual int GetUseShadowColorMap(TimeValue t) { return FALSE; }
	
	virtual int GetOvershoot()=0;
	virtual void SetOvershoot(int a)=0;

	virtual NameTab& GetExclusionList()=0;
	virtual void SetExclusionList(NameTab &list)=0;

	virtual BOOL SetHotSpotControl(Control *c)=0;
	virtual BOOL SetFalloffControl(Control *c)=0;
	virtual BOOL SetColorControl(Control *c)=0;
	virtual Control* GetHotSpotControl()=0;
	virtual Control* GetFalloffControl()=0;
	virtual Control* GetColorControl()=0;
	
	virtual void SetAffectDiffuse(BOOL onOff) {}
	virtual BOOL GetAffectDiffuse() {return 0;}
	virtual void SetAffectSpecular(BOOL onOff) {}
	virtual BOOL GetAffectSpecular() {return 0;}

	virtual void SetDecayType(BOOL onOff) {}
	virtual BOOL GetDecayType() {return 0;}
	virtual void SetDecayRadius(TimeValue time, float f) {}
	virtual float GetDecayRadius(TimeValue t, Interval& valid = Interval(0,0)) { return 0.0f;}
	virtual void SetDiffuseSoft(TimeValue time, float f) {}
	virtual float GetDiffuseSoft(TimeValue t, Interval& valid = Interval(0,0)) { return 0.0f; }

	virtual void SetShadColor(TimeValue t, Point3& rgb) {}
	virtual Point3 GetShadColor(TimeValue t, Interval &valid = Interval(0,0)) { return Point3(0,0,0); }
	virtual BOOL GetLightAffectsShadow() { return 0; }
	virtual void SetLightAffectsShadow(BOOL b) {  }
	virtual void SetShadMult(TimeValue t, float m) {}
	virtual float GetShadMult(TimeValue t, Interval &valid = Interval(0,0)) { return 1.0f; }

	virtual Texmap* GetProjMap() { return NULL;  }
	virtual void SetProjMap(Texmap* pmap) {}
	virtual Texmap* GetShadowProjMap() { return NULL;  }
	virtual void SetShadowProjMap(Texmap* pmap) {}

	virtual void SetAmbientOnly(BOOL onOff) {  }
	virtual BOOL GetAmbientOnly() { return FALSE; }

	virtual void SetEmitterEnable(TimeValue t, BOOL onOff) {}
	virtual BOOL GetEmitterEnable(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
	virtual void SetEmitterEnergy(TimeValue t, float energy) {}
	virtual float GetEmitterEnergy(TimeValue t, Interval& valid = Interval(0,0)) { return 0.0f; }
	virtual void SetEmitterDecayType(TimeValue t, int decay) {}
	virtual int  GetEmitterDecayType(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
	virtual void SetEmitterCausticPhotons(TimeValue t, int photons) {}
	virtual int  GetEmitterCausticPhotons(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
	virtual void SetEmitterGlobalIllumPhotons(TimeValue t, int photons) {}
	virtual int  GetEmitterGlobalIllumPhotons(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
};


#endif // __GENLIGHT_H__
