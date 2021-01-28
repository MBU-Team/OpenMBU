/**********************************************************************
 *<
	FILE: simpshp.h

	DESCRIPTION:  Defines a simple shape object class to make
		procedural shape primitives easier to create

	CREATED BY: Tom Hudson

	HISTORY: created 30 October 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __SIMPSHP_H__ 

#define __SIMPSHP_H__

// Parameter block reference indices
// IMPORTANT: Reference #0 is ShapeObject's parameter block!  (Starting with MAXr4)
#define SHAPEOBJPBLOCK 0	// ShapeObject's parameter block
#define USERPBLOCK SHAPE_OBJ_NUM_SUBS	// User's parameter block

#define DEF_RENDERABLE_THICKNESS 1.0f
#define DEF_RENDERABLE_SIDES 12
#define DEF_RENDERABLE_ANGLE 0.0f

class SimpleShape: public ShapeObject {			   
	public:
		IParamBlock *pblock;

		static IObjParam *ip;
		static HWND hGenParams;
		static BOOL dlgRenderable;
		static float dlgThickness;
		static int dlgSides;
		static float dlgAngle;
		static BOOL dlgGenUVs;
		static ISpinnerControl *thickSpin;

		// Shape cache
		PolyShape shape;
		Interval ivalid;

		// Flag to suspend snapping -- Used during creation
		BOOL suspendSnap;

		CoreExport void UpdateShape(TimeValue t);

		static SimpleShape *editOb;

		CoreExport SimpleShape();
		CoreExport ~SimpleShape();

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
		CoreExport void BuildMesh(TimeValue t, Mesh &mesh);
				
		// From ShapeObject
		CoreExport ObjectHandle CreateTriObjRep(TimeValue t);  // for rendering, also for deformation		
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vxt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
		CoreExport int NumberOfVertices(TimeValue t, int curve);
		CoreExport int NumberOfCurves();
		CoreExport BOOL CurveClosed(TimeValue t, int curve);
		CoreExport ShapeHierarchy &OrganizeCurves(TimeValue t, ShapeHierarchy *hier=NULL);	// Ready for lofting, extrusion, etc.
		CoreExport void MakePolyShape(TimeValue t, PolyShape &shape, int steps = PSHAPE_BUILTIN_STEPS, BOOL optimize = FALSE);
		CoreExport int MakeCap(TimeValue t, MeshCapInfo &capInfo, int capType);	// Makes a cap out of the shape
		CoreExport int MakeCap(TimeValue t, PatchCapInfo &capInfo);

		int NumRefs() { return 1 + ShapeObject::NumRefs(); }
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);

		CoreExport void ReadyGeneralParameters();
		CoreExport void SimpleShapeClone( SimpleShape *sshpSource );

		int NumSubs() { return 1 + ShapeObject::NumSubs(); }  
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);		

		// Animatable methods
		void DeleteThis() { delete this; }
		CoreExport void FreeCaches(); 

		// IO
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		CoreExport void SetGenUVs(BOOL sw);
		CoreExport void SetRenderable(BOOL sw);
//		CoreExport void SetThickness(float t);

		LRESULT CALLBACK TrackViewWinProc( HWND hwnd,  UINT message, 
	            WPARAM wParam,   LPARAM lParam ){return(0);}

		void GetClassName(TSTR& s) {s = GetObjectName();}
		void InitNodeName(TSTR& s) {s = GetObjectName();}

		// Clients of SimpleShape need to implement these methods:
	
		virtual Class_ID ClassID() = 0;
		virtual void BuildShape(TimeValue t,PolyShape& ashape) = 0;
		virtual RefTargetHandle Clone(RemapDir& remap = NoRemap()) = 0;
		virtual CreateMouseCallBack* GetCreateMouseCallBack() = 0;
		virtual BOOL ValidForDisplay(TimeValue t) = 0;
		virtual void InvalidateUI() {}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}

		// Unlike SimpleSplines, you're probably procedural, so implementing these
		// is a must!
		virtual Point3 InterpCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE) = 0;
		virtual Point3 TangentCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE) = 0;
		virtual float LengthOfCurve(TimeValue t, int curve) = 0;
		
		// Here are some optional methods.
		// You should _really_ implement these, because they just do the bare-minimum job
		// (Chopping your curve up into manageable pieces makes things look better)
		virtual int NumberOfPieces(TimeValue t, int curve) { return 1; }
		virtual Point3 InterpPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE) { return InterpCurve3D(t, curve, param, ptype); }
		virtual Point3 TangentPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE) { return TangentCurve3D(t, curve, param, ptype); }

		CoreExport virtual MtlID GetMatID(TimeValue t, int curve, int piece);
	};				


#endif // __SIMPSHP_H__
