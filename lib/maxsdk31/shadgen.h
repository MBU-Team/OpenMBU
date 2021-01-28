/**********************************************************************
 *<
FILE: shadgen.h : pluggable shadow generators.

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY: Created 10/27/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __SHADGEN__H

#define __SHADGEN__H

#define SHAD_PARALLEL       2
#define SHAD_OMNI           4

class ShadowGenerator;
class ParamBlockDescID;
class IParamBlock;

class ShadowParamDlg {
public:
	virtual void DeleteThis()=0;
	};

// This class carries the parameters for the shadow type, and puts up the parameter rollup.
class ShadowType: public ReferenceTarget {
	public:
		SClass_ID SuperClassID() { return SHADOW_TYPE_CLASS_ID;}
		virtual ShadowParamDlg *CreateShadowParamDlg(Interface *ip) { return NULL; }
		virtual ShadowGenerator* CreateShadowGenerator(LightObject *l,  ObjLightDesc *ld, ULONG flags)=0;
		virtual BOOL SupportStdMapInterface() { return FALSE; }

		BOOL BypassPropertyLevel() { return TRUE; }  // want to promote shadowtype props to light level

		// If the shadow generator can handle omni's directly, this should return true. If it does,
		// then when doing an Omni light, the SHAD_OMNI flag will be passed in to 
		// the CreateShadowGenerator call, and only one ShadowGenerator will be created
		// instead of the normal 6 (up,down,right,left,front,back).
		virtual BOOL CanDoOmni() { return FALSE; }

		// This method used for converting old files: only needs to be supported by default 
		// shadow map and ray trace shadows.
		virtual void ConvertParamBlk( ParamBlockDescID *descOld, int oldCount, IParamBlock *oldPB ) { }

		// This method valid iff SupportStdMapInterface returns TRUE
		virtual int MapSize(TimeValue t) { return 512; } 

		// This interface is solely for the default shadow map type ( Class_ID(STD_SHADOW_MAP_CLASS_ID,0) )
		virtual void SetMapRange(TimeValue t, float f) {}
		virtual float GetMapRange(TimeValue t, Interval& valid = Interval(0,0)) { return 0.0f; }
		virtual void SetMapSize(TimeValue t, int f) {}
		virtual int GetMapSize(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
		virtual void SetMapBias(TimeValue t, float f) {} 
		virtual float GetMapBias(TimeValue t, Interval& valid = Interval(0,0)) { return 0.0f; }
		virtual void SetAbsMapBias(TimeValue t, int a) {}
		virtual int GetAbsMapBias(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }

		// This interface is solely for the default raytrace shadow type ( Class_ID(STD_RAYTRACE_SHADOW_CLASS_ID,0) )
		virtual float GetRayBias(TimeValue t, Interval &valid = Interval(0,0)) { return 0.0f; }
		virtual	void SetRayBias(TimeValue t, float f) {}
		virtual int GetMaxDepth(TimeValue t, Interval &valid = Interval(0,0)) { return 1; } 
		virtual void SetMaxDepth(TimeValue t, int f) {}

	};

// This class generates the shadows. It only exists during render, one per instance of the light.
class ShadowGenerator {
public:
	virtual int Update(
		TimeValue t,
		const RendContext& rendCntxt,   // Mostly for progress bar.
		RenderGlobalContext *rgc,       // Need to get at instance list.
		Matrix3& lightToWorld, // light to world space: not necessarly same as that of light
		float aspect,      // aspect
		float param,   	   // persp:field-of-view (radians) -- parallel : width in world coords
		float clipDist = DONT_CLIP  
		)=0;

	virtual int UpdateViewDepParams(const Matrix3& worldToCam)=0;

	virtual void FreeBuffer()=0;
	virtual void DeleteThis()=0; // call this to destroy the ShadowGenerator

	// Generic shadow sampling function
	// Implement this when ShadowType::SupportStdMapInterface() returns FALSE. 
	virtual float Sample(ShadeContext &sc, Point3 &norm, Color& color) { return 1.0f; }

	// Implement these methods when ShadowType::SupportStdMapInterface() returns TRUE. 
	// This interface allows illuminated atmospherics
	// Note: Sample should return a small NEGATIVE number ehen the sample falls outside of the shadow buffer, so
	//    the caller can know to take appropriate action.
	virtual	float Sample(ShadeContext &sc, float x, float y, float z, float xslope, float yslope) { return 1.0f; }
	virtual BOOL QuickSample(int x, int y, float z) { return 1; }
	virtual float FiltSample(int x, int y, float z, int level) { return 1.0f; }
	virtual float LineSample(int x1, int y1, float z1, int x2, int y2, float z2) { return 1.0f; }

	};


// This returns a new default shadow-map shadow generator
CoreExport ShadowType *NewDefaultShadowMapType();

// This returns a new default ray-trace shadow generator
CoreExport ShadowType *NewDefaultRayShadowType();


#endif __SHADGEN__H
