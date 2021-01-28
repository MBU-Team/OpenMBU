/**********************************************************************
 *<
	FILE: triobj.h

	DESCRIPTION:  Defines Triangle Mesh Object

	CREATED BY: Dan Silva

	HISTORY: created 9 September 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __TRIOBJ__ 

#define __TRIOBJ__

#include "meshlib.h"
#include "snap.h"
#include "maxtess.h"

#define TRI_MULTI_PROCESSING TRUE

extern CoreExport Class_ID triObjectClassID;

class TriObject: public GeomObject {
	protected:
		Interval geomValid;
		Interval topoValid;
		Interval texmapValid;
		Interval selectValid;
		Interval vcolorValid;
		Interval gfxdataValid;
		DWORD validBits; // for the remaining constant channels
		CoreExport void CopyValidity(TriObject *fromOb, ChannelMask channels);
#if TRI_MULTI_PROCESSING
		static int		refCount;
		static HANDLE	defThread;
		static HANDLE	defMutex;
		static HANDLE	defStartEvent;
		static HANDLE	defEndEvent;
		friend DWORD WINAPI defFunc(LPVOID ptr);
#endif	
		//  inherited virtual methods for Reference-management
		CoreExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );
	public:
		Mesh  mesh;
		TessApprox mDispApprox;
		bool mSubDivideDisplacement;
		bool mDisableDisplacement;
		bool mSplitMesh;

		CoreExport TriObject();
		CoreExport ~TriObject();

		//  inherited virtual methods:

		//from animatable
		CoreExport void* GetInterface(ULONG id);
		CoreExport void ReleaseInterface(ULONG id,void *i);

		// From BaseObject
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		CoreExport CreateMouseCallBack* GetCreateMouseCallBack();
		CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());

		// From Object			 
		CoreExport ObjectState Eval(TimeValue time);
		CoreExport Interval ObjectValidity(TimeValue t);
		CoreExport BOOL HasUVW();
		CoreExport BOOL HasUVW (int mapChannel);

		// get and set the validity interval for the nth channel
	   	CoreExport Interval ChannelValidity(TimeValue t, int nchan);
		CoreExport void SetChannelValidity(int i, Interval v);
		CoreExport void InvalidateChannels(ChannelMask channels);

		// Convert-to-type validity
		CoreExport Interval ConvertValidity(TimeValue t);

		// Deformable object procs	
		int IsDeformable() { return 1; }  
		int NumPoints() { return mesh.getNumVerts(); }
		Point3 GetPoint(int i) { return mesh.getVert(i); }
		void SetPoint(int i, const Point3& p) { mesh.setVert(i,p); }

		CoreExport BOOL IsPointSelected (int i);
		CoreExport float PointSelection (int i);

		// Mappable object procs
		int IsMappable() { return 1; }
		int NumMapChannels () { return MAX_MESHMAPS; }
		int NumMapsUsed () { return mesh.getNumMaps(); }
		void ApplyUVWMap(int type, float utile, float vtile, float wtile,
			int uflip, int vflip, int wflip, int cap,const Matrix3 &tm,int channel=1) {
				mesh.ApplyUVWMap(type,utile,vtile,wtile,uflip,vflip,wflip,cap,tm,channel); }
				
        CoreExport BOOL PolygonCount(TimeValue t, int& numFaces, int& numVerts);
		void PointsWereChanged(){ mesh.InvalidateGeomCache(); }
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL,BOOL useSel=FALSE );
		CoreExport void Deform(Deformer *defProc, int useSel);

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
		
		// for displacement mapping
		CoreExport BOOL CanDoDisplacementMapping();
		CoreExport TessApprox& DisplacmentApprox() { return mDispApprox; }
		CoreExport bool& DoSubdivisionDisplacment() { return mSubDivideDisplacement; }
		CoreExport bool& SplitMeshForDisplacement() { return mSplitMesh; }
		CoreExport void SetDisplacmentApproxToPreset(int preset);
		CoreExport void DisableDisplacementMapping(BOOL disable);

		CoreExport void TopologyChanged();

		Mesh& GetMesh() { return mesh; }

		// Animatable methods

		void DeleteThis() { delete this; }
		void FreeCaches() {mesh.InvalidateGeomCache(); }
		Class_ID ClassID() { return Class_ID(TRIOBJ_CLASS_ID,0); }
		void GetClassName(TSTR& s) { s = TSTR(_T("TriObject")); }
		void NotifyMe(Animatable *subAnim, int message) {}
		int IsKeyable() { return 0;}
		int Update(TimeValue t) { return 0; }
		//BOOL BypassTreeView() { return TRUE; }
		// This is the name that will appear in the history browser.
		TCHAR *GetObjectName() { return _T("Mesh"); }

		CoreExport void RescaleWorldUnits(float f);

		// IO
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		// TriObject-specific methods


	};

CoreExport void SetDisplacmentPreset(int preset, TessApprox approx);

// Regular TriObject
CoreExport ClassDesc* GetTriObjDescriptor();

// A new decsriptor can be registered to replace the default
// tri object descriptor. This new descriptor will then
// be used to create tri objects.

CoreExport void RegisterEditTriObjDesc(ClassDesc* desc);
CoreExport ClassDesc* GetEditTriObjDesc(); // Returns default of none have been registered

// Use this instead of new TriObject. It will use the registered descriptor
// if one is registered, otherwise you'll get a default tri-object.
CoreExport TriObject *CreateNewTriObject();


#endif
