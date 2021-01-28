/**********************************************************************
 *<
	FILE: simpspl.h

	DESCRIPTION:  Defines a simple spline object class to make spline
		primitives easier to create

	CREATED BY: Tom Hudson

	HISTORY: created 3 October 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __SIMPSPL_H__ 

#define __SIMPSPL_H__

// Interpolation parameter block indices
#define IPB_STEPS		0
#define IPB_OPTIMIZE	1
#define IPB_ADAPTIVE	2

// Parameter block reference indices
// IMPORTANT: Reference #0 is ShapeObject's parameter block!  (Starting with MAXr4)
#define SHAPEOBJPBLOCK 0	// ShapeObject's parameter block
#define USERPBLOCK SHAPE_OBJ_NUM_REFS	// User's parameter block
#define IPBLOCK (SHAPE_OBJ_NUM_REFS + 1)		// Interpolations parameter block

// Default interpolation settings
#define DEF_STEPS 6
#define DEF_OPTIMIZE TRUE
#define DEF_ADAPTIVE FALSE
#define DEF_RENDERABLE FALSE
#define DEF_DISPRENDERMESH FALSE
//#define DEF_USEVIEWPORT FALSE
#define DEF_RENDERABLE_THICKNESS 1.0f
#define DEF_RENDERABLE_SIDES 12
#define DEF_RENDERABLE_ANGLE 0.0f
#define DEF_GENUVS FALSE

// Special dialog handling
class SimpleSpline;

class SimpleSplineDlgProc : public ParamMapUserDlgProc {
	private:
		SimpleSpline *spl;
	public:
		SimpleSplineDlgProc(SimpleSpline *s) { spl = s; }
		CoreExport BOOL DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
		void DeleteThis() { delete this; }
	};
						 
class SimpleSpline: public ShapeObject {			   
	private:
	public:
		IParamBlock *ipblock;	// Interpolation parameter block (handled by SimpleSpline)
		IParamBlock *pblock;	// User's parameter block

		static IParamMap *ipmapParam;
		static int dlgSteps;
		static BOOL dlgOptimize;
		static BOOL dlgAdaptive;

		// Spline cache
		BezierShape shape;
		Interval ivalid;

		// Flag to suspend snapping -- Used during creation
		BOOL suspendSnap;

		CoreExport void UpdateShape(TimeValue t);

		static SimpleSpline *editOb;

		CoreExport SimpleSpline();
		CoreExport ~SimpleSpline();

		void ShapeInvalid() { ivalid.SetEmpty(); }

		//  inherited virtual methods:

		// From BaseObject
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		CoreExport virtual void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport virtual void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		IParamArray *GetParamBlock() {return pblock;}
		CoreExport int GetParamBlockIndex(int id);

		// From Object
		CoreExport ObjectState Eval(TimeValue time);
		CoreExport Interval ObjectValidity(TimeValue t);
		CoreExport int CanConvertToType(Class_ID obtype);
		CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);
		CoreExport void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

		CoreExport void BuildMesh(TimeValue t, Mesh &mesh);
		
		// From ShapeObject
		CoreExport ObjectHandle CreateTriObjRep(TimeValue t);  // for rendering, also for deformation		
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vxt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
		CoreExport int NumberOfVertices(TimeValue t, int curve);
		CoreExport int NumberOfCurves();
		CoreExport BOOL CurveClosed(TimeValue t, int curve);
		CoreExport Point3 InterpCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE);
		CoreExport Point3 TangentCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE);
		CoreExport float LengthOfCurve(TimeValue t, int curve);
		CoreExport int NumberOfPieces(TimeValue t, int curve);
		CoreExport Point3 InterpPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE);
		CoreExport Point3 TangentPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE);
		CoreExport MtlID GetMatID(TimeValue t, int curve, int piece);
		BOOL CanMakeBezier() { return TRUE; }			// Return TRUE if can turn into a bezier representation
		CoreExport void MakeBezier(TimeValue t, BezierShape &shape);	// Create the bezier representation
		CoreExport ShapeHierarchy &OrganizeCurves(TimeValue t, ShapeHierarchy *hier=NULL);	// Ready for lofting, extrusion, etc.
		CoreExport void MakePolyShape(TimeValue t, PolyShape &shape, int steps = PSHAPE_BUILTIN_STEPS, BOOL optimize = FALSE);
		CoreExport int MakeCap(TimeValue t, MeshCapInfo &capInfo, int capType);	// Makes a cap out of the shape
		CoreExport int MakeCap(TimeValue t, PatchCapInfo &capInfo);

		int NumRefs() { return 2 + ShapeObject::NumRefs();}
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);

		CoreExport void ReadyInterpParameterBlock();
		void UnReadyInterpParameterBlock() { ipblock = NULL; }

		CoreExport void SimpleSplineClone( SimpleSpline *ssplSource );

		int NumSubs() { return 2 + ShapeObject::NumSubs(); }  
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);		

		// Animatable methods
		void DeleteThis() { delete this; }
		CoreExport void FreeCaches(); 

		// IO
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		LRESULT CALLBACK TrackViewWinProc( HWND hwnd,  UINT message, 
	            WPARAM wParam,   LPARAM lParam ){return(0);}

		void GetClassName(TSTR& s) {s = GetObjectName();}
		void InitNodeName(TSTR& s) {s = GetObjectName();}

		// Clients of SimpleSpline need to implement these methods:
	
		virtual Class_ID ClassID() = 0;
		virtual void BuildShape(TimeValue t,BezierShape& ashape) = 0;
		virtual RefTargetHandle Clone(RemapDir& remap = NoRemap()) = 0;
		virtual CreateMouseCallBack* GetCreateMouseCallBack() = 0;
		virtual BOOL ValidForDisplay(TimeValue t) = 0;
		virtual void InvalidateUI() {}
		virtual ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}
		virtual BOOL DisplayVertTicksDuringCreation() { return TRUE; }
	};				

#endif // __SIMPSPL_H__
