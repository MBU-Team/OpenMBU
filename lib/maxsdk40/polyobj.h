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

#ifdef POLY_LIB_EXPORTING
#define PolyLibExport __declspec( dllexport )
#else
#define PolyLibExport __declspec( dllimport )
#endif

#define POLY_MULTI_PROCESSING TRUE

extern PolyLibExport Class_ID polyObjectClassID;

class PolyObject: public GeomObject {
private:
	// Displacement approximation stuff:
	TessApprox mDispApprox;
	bool mSubDivideDisplacement;
	bool mDisableDisplacement;
	bool mSplitMesh;

protected:
	Interval geomValid;
	Interval topoValid;
	Interval texmapValid;
	Interval selectValid;
	Interval vcolorValid;
	DWORD validBits; // for the remaining constant channels
	PolyLibExport void CopyValidity(PolyObject *fromOb, ChannelMask channels);
	//  inherited virtual methods for Reference-management
	PolyLibExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );
public:
	MNMesh mm;

	PolyLibExport PolyObject();
	PolyLibExport ~PolyObject();

	//  inherited virtual methods:

	// From BaseObject
	PolyLibExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	PolyLibExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	PolyLibExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	PolyLibExport CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }
	PolyLibExport RefTargetHandle Clone(RemapDir& remap = NoRemap());

	// From Object			 
	PolyLibExport ObjectState Eval(TimeValue time);
	PolyLibExport Interval ObjectValidity(TimeValue t);
	PolyLibExport BOOL HasUVW();
	PolyLibExport BOOL HasUVW(int mapChannel);
	PolyLibExport Object *CollapseObject();
	// get and set the validity interval for the nth channel
	PolyLibExport Interval ChannelValidity (TimeValue t, int nchan);
	PolyLibExport Interval PartValidity (DWORD partIDs);
	PolyLibExport void SetChannelValidity (int i, Interval v);
	PolyLibExport void SetPartValidity (DWORD partIDs, Interval v);
	PolyLibExport void InvalidateChannels (ChannelMask channels);

	// Convert-to-type validity
	PolyLibExport Interval ConvertValidity(TimeValue t);

	// Deformable object procs	
	int IsDeformable() { return 1; }  
	PolyLibExport int NumPoints();
	PolyLibExport Point3 GetPoint(int i);
	PolyLibExport void SetPoint(int i, const Point3& p);
	PolyLibExport void PointsWereChanged();
	PolyLibExport void Deform (Deformer *defProc, int useSel=0);

	// Mappable object procs
	int IsMappable() { return 1; }
	int NumMapChannels () { return MAX_MESHMAPS; }
	int NumMapsUsed () { return mm.numm; }
	PolyLibExport void ApplyUVWMap(int type, float utile, float vtile, float wtile,
		int uflip, int vflip, int wflip, int cap,const Matrix3 &tm,int channel=1);

	PolyLibExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL,BOOL useSel=FALSE );

	PolyLibExport int CanConvertToType(Class_ID obtype);
	PolyLibExport Object* ConvertToType(TimeValue t, Class_ID obtype);
	PolyLibExport void FreeChannels(ChannelMask chan);
	PolyLibExport Object *MakeShallowCopy(ChannelMask channels);
	PolyLibExport void ShallowCopy(Object* fromOb, ChannelMask channels);
	PolyLibExport void NewAndCopyChannels(ChannelMask channels);

	PolyLibExport DWORD GetSubselState();
	PolyLibExport void SetSubSelState(DWORD s);

	PolyLibExport BOOL CheckObjectIntegrity();

	// From GeomObject
	PolyLibExport int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	PolyLibExport ObjectHandle CreateTriObjRep(TimeValue t);  // obselete, do not use!!!  (Returns NULL.)
	PolyLibExport void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box );
	PolyLibExport void GetLocalBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box );
	PolyLibExport Mesh* GetRenderMesh(TimeValue t, INode *inode, View &view,  BOOL& needDelete);

	PolyLibExport void TopologyChanged();

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
	PolyLibExport void RescaleWorldUnits(float f);

	// IO
	PolyLibExport IOResult Save(ISave *isave);
	PolyLibExport IOResult Load(ILoad *iload);

	// Displacement mapping parameter access.
	// (PolyObjects don't do displacement mapping directly, but they
	// pass their settings on to TriObjects.)
	PolyLibExport BOOL CanDoDisplacementMapping();
	PolyLibExport void SetDisplacementApproxToPreset(int preset);

	// Accessor methods:
	void SetDisplacementDisable (bool disable) { mDisableDisplacement = disable; }
	void SetDisplacementParameters (TessApprox & params) { mDispApprox = params; }
	void SetDisplacementSplit (bool split) { mSplitMesh = split; }
	void SetDisplacement (bool displace) { mSubDivideDisplacement = displace; }

	bool GetDisplacementDisable () const { return mDisableDisplacement; }
	TessApprox GetDisplacementParameters () const { return mDispApprox; }
	bool GetDisplacementSplit () const { return mSplitMesh; }
	bool GetDisplacement () const { return mSubDivideDisplacement; }

	TessApprox &DispParams() { return mDispApprox; }
};

// Regular PolyObject
PolyLibExport ClassDesc* GetPolyObjDescriptor();

// A new descriptor can be registered to replace the default
// tri object descriptor. This new descriptor will then
// be used to create tri objects.

PolyLibExport void RegisterEditPolyObjDesc(ClassDesc* desc);
PolyLibExport ClassDesc* GetEditPolyObjDesc(); // Returns default of none have been registered

// Use this instead of new PolyObject. It will use the registered descriptor
// if one is registered, otherwise you'll get a default poly-object.
PolyLibExport PolyObject *CreateEditablePolyObject();

// Inter-object conversions:
PolyLibExport void ConvertPolyToPatch (MNMesh & from, PatchMesh & to, DWORD flags=0);
PolyLibExport void ConvertPatchToPoly (PatchMesh & from, MNMesh & to, DWORD flags=0);

#endif
