/**********************************************************************
 *<
	FILE: gizmoimp.h

	DESCRIPTION: General atmoshperic gizmo objects

	CREATED BY: Rolf Berteig

	HISTORY: 4-15-96
	         11-13-96 Moved into core

 *>	Copyright (c) 1996 Rolf Berteig, All Rights Reserved.
 **********************************************************************/

#ifndef __GIZMOIMP_H__
#define __GIZMOIMP_H__

#define SPHEREGIZMO_CLASSID	Class_ID(0x3bc31904, 0x67d74ec7)
#define CYLGIZMO_CLASSID	Class_ID(0x3bc31904, 0x67d74ec8)
#define BOXGIZMO_CLASSID	Class_ID(0x3bc31904, 0x67d74ec9)

class SphereGizmoObject : public GizmoObject {
	public:		
		CoreExport SphereGizmoObject();
		CoreExport ~SphereGizmoObject();

		// From BaseObject
		CoreExport CreateMouseCallBack* GetCreateMouseCallBack();
		CoreExport void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport TCHAR *GetObjectName();
		CoreExport void InitNodeName(TSTR& s);

		// Animatable methods
		CoreExport void GetClassName(TSTR& s);
		void DeleteThis() {delete this;}
		Class_ID ClassID() {return SPHEREGIZMO_CLASSID;}
		
		// From ref
		CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());		

		// From GizmoObject		
		Interval ObjectValidity(TimeValue t); // mjm - 1.27.99	
		CoreExport void InvalidateUI();
		CoreExport ParamDimension *GetParameterDim(int pbIndex);
		CoreExport TSTR GetParameterName(int pbIndex);
		CoreExport void DrawGizmo(TimeValue t,GraphicsWindow *gw);		
		CoreExport void GetBoundBox(Matrix3 &mat,TimeValue t,Box3 &box);
	};

#define PB_GIZMO_RADIUS	0
#define PB_GIZMO_HEMI	1
#define PB_GIZMO_SEED	2


class CylGizmoObject : public GizmoObject {
	public:		
		CoreExport CylGizmoObject();
		CoreExport ~CylGizmoObject();

		// From BaseObject
		CoreExport CreateMouseCallBack* GetCreateMouseCallBack();
		CoreExport void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport TCHAR *GetObjectName();
		CoreExport void InitNodeName(TSTR& s);

		// Animatable methods
		CoreExport void GetClassName(TSTR& s);
		void DeleteThis() {delete this;}
		Class_ID ClassID() {return CYLGIZMO_CLASSID;}
		
		// From ref
		CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());		

		// From GizmoObject		
		Interval ObjectValidity(TimeValue t); // mjm - 1.27.99	
		CoreExport void InvalidateUI();
		CoreExport ParamDimension *GetParameterDim(int pbIndex);
		CoreExport TSTR GetParameterName(int pbIndex);
		CoreExport void DrawGizmo(TimeValue t,GraphicsWindow *gw);		
		CoreExport void GetBoundBox(Matrix3 &mat,TimeValue t,Box3 &box);
	};

#define PB_CYLGIZMO_RADIUS	0
#define PB_CYLGIZMO_HEIGHT	1
#define PB_CYLGIZMO_SEED	2

class BoxGizmoObject : public GizmoObject {
	public:
		CoreExport BoxGizmoObject();
		CoreExport ~BoxGizmoObject();

		// From BaseObject
		CoreExport CreateMouseCallBack* GetCreateMouseCallBack();
		CoreExport void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport TCHAR *GetObjectName();
		CoreExport void InitNodeName(TSTR& s);

		// Animatable methods
		CoreExport void GetClassName(TSTR& s);
		void DeleteThis() {delete this;}
		Class_ID ClassID() {return BOXGIZMO_CLASSID;}
		
		// From ref
		CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());		

		// From GizmoObject		
		Interval ObjectValidity(TimeValue t); // mjm - 1.27.99	
		CoreExport void InvalidateUI();
		CoreExport ParamDimension *GetParameterDim(int pbIndex);
		CoreExport TSTR GetParameterName(int pbIndex);
		CoreExport void DrawGizmo(TimeValue t,GraphicsWindow *gw);		
		CoreExport void GetBoundBox(Matrix3 &mat,TimeValue t,Box3 &box);
	};

#define PB_BOXGIZMO_LENGTH	0
#define PB_BOXGIZMO_WIDTH	1
#define PB_BOXGIZMO_HEIGHT	2
#define PB_BOXGIZMO_SEED	3


#endif //__GIZMOIMP_H__
