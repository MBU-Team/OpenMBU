/**********************************************************************
 *<
	FILE: polyobj.h

	DESCRIPTION:  Defines Polygon Mesh Object

	CREATED BY: Steve Anderson

	HISTORY: created June 1998

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

#ifndef __POLYOBJ__ 

#define __POLYOBJ__

#include "meshlib.h"
#include "mnmath.h"
#include "snap.h"

#define POLY_MULTI_PROCESSING TRUE

extern CoreExport Class_ID polyObjectClassID;

class PolyObject: public GeomObject {
protected:
	Interval geomValid;
	Interval topoValid;
	Interval texmapValid;
	Interval selectValid;
	Interval vcolorValid;
	DWORD validBits; // for the remaining constant channels
	CoreExport void CopyValidity(PolyObject *fromOb, ChannelMask channels);
	//  inherited virtual methods for Reference-management
	CoreExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );
public:
	MNMesh mm;

	CoreExport PolyObject();
	CoreExport ~PolyObject();

	//  inherited virtual methods:

	// From BaseObject
	CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	CoreExport CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
	CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());

	// From Object			 
	CoreExport ObjectState Eval(TimeValue time);
	CoreExport Interval ObjectValidity(TimeValue t);
	CoreExport BOOL HasUVW();
	CoreExport BOOL HasUVW(int mapChannel);

	// get and set the validity interval for the nth channel
	CoreExport Interval ChannelValidity (int nchan);
	CoreExport Interval PartValidity (DWORD partIDs);
	CoreExport void SetChannelValidity (int i, Interval v);
	CoreExport void SetPartValidity (DWORD partIDs, Interval v);
	CoreExport void InvalidateChannels (ChannelMask channels);

	// Convert-to-type validity
	CoreExport Interval ConvertValidity(TimeValue t);

	// Deformable object procs	
	int IsDeformable() { return 1; }  
	CoreExport int NumPoints();
	CoreExport Point3 GetPoint(int i);
	CoreExport void SetPoint(int i, const Point3& p);
	CoreExport void PointsWereChanged();
	CoreExport void Deform (Deformer *defProc, int useSel=0);

	// Mappable object procs
	int IsMappable() { return 1; }
	CoreExport void ApplyUVWMap(int type, float utile, float vtile, float wtile,
		int uflip, int vflip, int wflip, int cap,const Matrix3 &tm,int channel=1);

	CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL,BOOL useSel=FALSE );

	CoreExport int CanConvertToType(Class_ID obtype);
	CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);
	CoreExport void FreeChannels(ChannelMask chan);
	CoreExport Object *MakeShallowCopy(ChannelMask channels);
	CoreExport void ShallowCopy(Object* fromOb, ChannelMask channels);
	CoreExport void NewAndCopyChannels(ChannelMask channels);

	CoreExport DWORD GetSubselState();
	CoreExport void SetSubSelState(DWORD s);

	CoreExport BOOL CheckObjectIntegrity();

	// From GeomObject
	CoreExport int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	CoreExport ObjectHandle CreateTriObjRep(TimeValue t);  // for rendering, also for deformation		
	CoreExport void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box );
	CoreExport void GetLocalBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box );
	CoreExport Mesh* GetRenderMesh(TimeValue t, INode *inode, View &view,  BOOL& needDelete);

	CoreExport void TopologyChanged();

	MNMesh& GetMesh() { return mm; }

	// Animatable methods

	void DeleteThis() { delete this; }
	void FreeCaches() {mm.InvalidateGeomCache(); }
	Class_ID ClassID() { return Class_ID(POLYOBJ_CLASS_ID,0); }
	void GetClassName(TSTR& s) { s = TSTR(_T("PolyMeshObject")); }
	void NotifyMe(Animatable *subAnim, int message) {}
	int IsKeyable() { return 0;}
	int Update(TimeValue t) { return 0; }
	//BOOL BypassTreeView() { return TRUE; }
	// This is the name that will appear in the history browser.
	TCHAR *GetObjectName() { return _T("PolyMesh"); }
	CoreExport void RescaleWorldUnits(float f);

	// IO
	CoreExport IOResult Save(ISave *isave);
	CoreExport IOResult Load(ILoad *iload);
};

// Regular PolyObject
CoreExport ClassDesc* GetPolyObjDescriptor();

// A new descriptor can be registered to replace the default
// tri object descriptor. This new descriptor will then
// be used to create tri objects.

CoreExport void RegisterEditPolyObjDesc(ClassDesc* desc);
CoreExport ClassDesc* GetEditPolyObjDesc(); // Returns default of none have been registered

// Use this instead of new PolyObject. It will use the registered descriptor
// if one is registered, otherwise you'll get a default poly-object.
CoreExport PolyObject *CreateNewPolyObject();


#endif
