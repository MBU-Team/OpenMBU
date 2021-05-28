/**********************************************************************
 *<
	FILE: gizmo.h

	DESCRIPTION: An apparatus object

	CREATED BY: Rolf Berteig

	HISTORY: 4-15-96

 *>	Copyright (c) 1996 Rolf Berteig, All Rights Reserved.
 **********************************************************************/

#ifndef __GIZMO_H__
#define __GIZMO_H__

class IParamMap;

class GizmoObject : public HelperObject {
	public:
		IParamBlock *pblock;		
		static IParamMap *pmapParam;
		static IObjParam *ip;

		CoreExport GizmoObject();
		CoreExport ~GizmoObject();

		CoreExport static GizmoObject *editOb;

		// From BaseObject
		CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);		
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);		

		// From Object
		ObjectState Eval(TimeValue time) {return ObjectState(this);}
		void InitNodeName(TSTR& s) {s = GetObjectName();}		
		CoreExport int CanConvertToType(Class_ID obtype);
		CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);		
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );		

		// Animatable methods		
		//void GetClassName(TSTR& s) {s = GetObjectName();}		
		int NumSubs() { return 1; }  
		Animatable* SubAnim(int i) { return pblock; }
		TSTR SubAnimName(int i) {return _T("Parameters");}

		// From ref
		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
		void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);
		
		// Must implement...
		Interval ObjectValidity(TimeValue t) {return FOREVER;}		
		virtual void InvalidateUI() {}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}
		virtual void DrawGizmo(TimeValue t,GraphicsWindow *gw) {}
		virtual Point3 WireColor() { return GetUIColor(COLOR_ATMOS_APPARATUS); } // mjm - 4.20.99
		virtual void GetBoundBox(Matrix3 &mat,TimeValue t,Box3 &box) {}
	};

#endif //__GIZMO_H__
