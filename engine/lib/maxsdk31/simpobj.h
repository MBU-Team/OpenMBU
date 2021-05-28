/**********************************************************************
 *<
	FILE: SimpObj.h

	DESCRIPTION:  A base class for procedural objects that fit into
	              a standard form.

	CREATED BY: Rolf Berteig

	HISTORY: created 10/10/95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __SIMPOBJ__
#define __SIMPOBJ__

// ParticleSys lib
#include "partclib.h"


class SimpleObject : public GeomObject {
	public:
		IParamBlock *pblock;
		Mesh mesh;
		Interval ivalid;

		BOOL suspendSnap;	// If TRUE, no snapping will occur
		
		static SimpleObject *editOb;

		CoreExport SimpleObject();
		CoreExport ~SimpleObject();
		
		CoreExport void UpdateMesh(TimeValue t);				
		CoreExport void GetBBox(TimeValue t, Matrix3& tm, Box3& box );		
		void MeshInvalid() {ivalid.SetEmpty();}
		
		// From BaseObject
		CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);		
		CoreExport IParamArray *GetParamBlock();
		CoreExport int GetParamBlockIndex(int id);

		// From Object
		CoreExport ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) {s = GetObjectName();}
		CoreExport Interval ObjectValidity(TimeValue t);
		CoreExport int CanConvertToType(Class_ID obtype);
		CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);		
        CoreExport BOOL PolygonCount(TimeValue t, int& numFaces, int& numVerts);

		// From GeomObject
		CoreExport int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);		
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
		CoreExport Mesh* GetRenderMesh(TimeValue t, INode *inode, View &view, BOOL& needDelete);

		// Animatable methods
		CoreExport void FreeCaches(); 		
		void GetClassName(TSTR& s) {s = GetObjectName();}		
		int NumSubs() { return 1; }  
		Animatable* SubAnim(int i) { return pblock; }
		CoreExport TSTR SubAnimName(int i);

		// From ref
		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
		void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);
		

		// --- These must be implemented by the derived class ----------------
		virtual void BuildMesh(TimeValue t)=0;
		virtual BOOL OKtoDisplay(TimeValue t) {return TRUE;}
		virtual void InvalidateUI() {}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}		
	};

// ParamBlock2 version added JBW 9/11/98

class IParamBlock2;
class SimpleObject2 : public SimpleObject {
	public:
		IParamBlock2* pblock2;
		// From ref
		RefTargetHandle GetReference(int i) {return pblock;}
		void SetReference(int i, RefTargetHandle rtarg) { pblock2 = (IParamBlock2*)rtarg; SimpleObject::SetReference(i, rtarg); }		
	};

class SimpleWSMObject : public WSMObject {
	public:
		IParamBlock *pblock;
		Mesh mesh;
		Interval ivalid;
		
		static SimpleWSMObject *editOb;

		CoreExport SimpleWSMObject();
		CoreExport ~SimpleWSMObject();
		
		CoreExport void UpdateMesh(TimeValue t);				
		CoreExport void GetBBox(TimeValue t, Matrix3& tm, Box3& box );		
		void MeshInvalid() {ivalid.SetEmpty();}
		
		// From BaseObject
		CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);		
		CoreExport IParamArray *GetParamBlock();
		CoreExport int GetParamBlockIndex(int id);

		// From Object
		int DoOwnSelectHilite() {return TRUE;}
		CoreExport ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) {s = GetObjectName();}
		CoreExport Interval ObjectValidity(TimeValue t);		
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
		int IsRenderable() {return FALSE;}

		// Animatable methods
		CoreExport void FreeCaches(); 		
		void GetClassName(TSTR& s) {s = GetObjectName();}		
		int NumSubs() { return 1; }  
		Animatable* SubAnim(int i) { return pblock; }
		CoreExport TSTR SubAnimName(int i);

		// From ref
		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
		void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);
		

		// --- These must be implemented by the derived class ----------------
		virtual void BuildMesh(TimeValue t)=0;
		virtual BOOL OKtoDisplay(TimeValue t) {return TRUE;}
		virtual void InvalidateUI() {}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}		
	};

class SimpleMod;
class IParamMap;

// Make a WSM out of an OSM
class SimpleOSMToWSMObject : public SimpleWSMObject {
	public:
		SimpleMod *mod;
		static IParamMap *pmapParam;

		CoreExport SimpleOSMToWSMObject();
		CoreExport SimpleOSMToWSMObject(SimpleMod *m);

		int NumRefs() {return 2;}
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport IOResult Load(ILoad *iload);

		int NumSubs() {return 2;}  
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);

		CoreExport void BuildMesh(TimeValue t);
		CoreExport Modifier *CreateWSMMod(INode *node);

		CoreExport CreateMouseCallBack* GetCreateMouseCallBack();
		CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

		CoreExport ParamDimension *GetParameterDim(int pbIndex) {return stdWorldDim;}
		CoreExport TSTR GetParameterName(int pbIndex);

		CoreExport Deformer &GetDecayDeformer(TimeValue t,Deformer &mdef,Point3 origin,Interval &iv);

		CoreExport SimpleOSMToWSMObject* SimpleOSMToWSMClone(SimpleOSMToWSMObject *from,RemapDir& remap);

		CoreExport void InvalidateUI();
	};

#define PB_OSMTOWSM_LENGTH	0
#define PB_OSMTOWSM_WIDTH	1
#define PB_OSMTOWSM_HEIGHT	2
#define PB_OSMTOWSM_DECAY	3


class SimpleParticle : public ParticleObject {
	public:
		IParamBlock *pblock;
		ParticleSys parts;
		TimeValue tvalid;
		BOOL valid;
		Tab<ForceField*> fields;		
		Tab<CollisionObject*> cobjs;
		Mesh mesh;
		Interval mvalid;
		MetaParticle metap;
		
		CoreExport static SimpleParticle *editOb;
		CoreExport static IObjParam *ip;

		CoreExport SimpleParticle();
		CoreExport ~SimpleParticle();
				
		CoreExport void Update(TimeValue t,INode *node=NULL);
		CoreExport void UpdateMesh(TimeValue t);
		CoreExport void GetBBox(TimeValue t, Matrix3& tm, Box3& box);		
		void MeshInvalid() {mvalid.SetEmpty();}
		void ParticleInvalid() {valid=FALSE;}

		// From BaseObject
		CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);		
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);		
		CoreExport IParamArray *GetParamBlock();
		CoreExport int GetParamBlockIndex(int id);

		// From Object
		int DoOwnSelectHilite() {return TRUE;}
		CoreExport ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) {s = GetObjectName();}
		CoreExport Interval ObjectValidity(TimeValue t);
		CoreExport int CanConvertToType(Class_ID obtype);
		CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);		
		CoreExport Object *MakeShallowCopy(ChannelMask channels);
		void WSStateInvalidate() {valid = FALSE;}
		BOOL IsWorldSpaceObject() {return TRUE;}

		// From GeomObject				
		CoreExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );

		// From ParticleObject
		CoreExport void ApplyForceField(ForceField *ff);
		CoreExport BOOL ApplyCollisionObject(CollisionObject *co);
		CoreExport TimeValue ParticleAge(TimeValue t, int i);		
		CoreExport void SetParticlePosition(TimeValue t, int i, Point3 pos);
		CoreExport void SetParticleVelocity(TimeValue t, int i, Point3 vel);
		CoreExport void SetParticleAge(TimeValue t, int i, TimeValue age);

		// Animatable methods		
		void GetClassName(TSTR& s) {s = GetObjectName();}		
		int NumSubs() {return 1;}  
		Animatable* SubAnim(int i) { return pblock; }
		CoreExport TSTR SubAnimName(int i);
		CoreExport void* GetInterface(ULONG id);

		// From ref
		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
		void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}		
		CoreExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);
		

		// --- These must be implemented by the derived class ----------------
		virtual void UpdateParticles(TimeValue t,INode *node)=0;
		virtual void BuildEmitter(TimeValue t, Mesh& amesh)=0;
		virtual Interval GetValidity(TimeValue t)=0;
		virtual MarkerType GetMarkerType() {return POINT_MRKR;}
		virtual BOOL OKtoDisplay(TimeValue t) {return TRUE;}
		virtual BOOL EmitterVisible() {return TRUE;}
		virtual void InvalidateUI() {}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}		
	};

class GenBoxObject: public SimpleObject {	
	public:
	virtual void SetParams(float width, float height, float length, int wsegs=1,int lsegs=1, 
		int hsegs=1, BOOL genUV=TRUE)=0; 
	};

class GenCylinder: public SimpleObject {	
	public:
	virtual void SetParams(float rad, float height, int segs, int sides, int capsegs=1, BOOL smooth=TRUE, 
		BOOL genUV=TRUE, BOOL sliceOn= FALSE, float slice1 = 0.0f, float slice2 = 0.0f)=0;
	};

class GenSphere: public SimpleObject {	
	public:
	virtual void SetParams(float rad, int segs, BOOL smooth=TRUE, BOOL genUV=TRUE,
		 float hemi=0.0f, BOOL squash=FALSE, BOOL recenter=FALSE)=0;
	};


#endif // __SIMPOBJ__
