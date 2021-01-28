/**********************************************************************
 *<
	FILE: ICollision.h

	DESCRIPTION: An interface class to our collisions

	CREATED BY: Peter Watje

	HISTORY: 3-15-00

 *>	Copyright (c) 200, All Rights Reserved.
 **********************************************************************/

#ifndef __ICOLLSION__H
#define __ICOLLISION__H

#include "Max.h"
#include "iparamm2.h"
#include "iFnPub.h"

#define PLANAR_COLLISION_ID Class_ID(0x14585111, 0x444a7dcf)
#define SPHERICAL_COLLISION_ID Class_ID(0x14585222, 0x555a7dcf)
#define MESH_COLLISION_ID Class_ID(0x14585333, 0x666a7dcf)


#define COLLISION_FO_INTERFACE Class_ID(0x14585444, 0x777a7dcf)

#define GetCollisionOpsInterface(cd) \
			(CollisionOps *)(cd)->GetInterface(COLLISION_FO_INTERFACE)



#define POINT_COLLISION		1
#define SPHERE_COLLISION	2
#define BOX_COLLISION		4
#define EDGE_COLLISION		8	


class ICollision : public ReferenceTarget {
public:
//return what is supported for collision engine
//right now all we support is point to surface collision
//but in the future the others maybe support by us or 3rd party
//it returns the or'd flags above
	virtual int SuppportedCollisions() = 0; 

//This method is called once before the checkcollision is called for each frame
//which allows you to do some data initializations
	virtual void PreFrame(TimeValue t, TimeValue dt) = 0;
//This method is called at the end f each frame solve to allow 
//you to destroy any data you don't need want
	virtual void PostFrame(TimeValue t, TimeValue dt) = 0;

//point to surface collision
//computes the time at which the particle hit the surface 
//t is the end time of the particle 
//dt is the delta of time that particle travels
//   t-dt = start of time of the particle
//pos the position of the particle in world space
//vel the velocity of the particle in world space
//at is the point in time that the collision occurs with respect to dt
//norm is bounce vector component of the final velocity
//friction is the friction vector component of the final velocity
//inheritVel is the amount of velocity inherited from the motion of the delfector
//		this is a rough apporximate
	virtual BOOL CheckCollision (TimeValue t,Point3 pos, Point3 vel, float dt, 
								 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel) = 0;

//sphere to surface collision
	virtual BOOL CheckCollision (TimeValue t,Point3 pos, float radius, Point3 vel, float dt, 
								 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel) = 0;
//box to surface collision
	virtual BOOL CheckCollision (TimeValue t, Box3 box, Point3 vel, float dt,  
								 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel) = 0;
//edge to surface collision
	virtual BOOL CheckCollision (TimeValue t,Point3 edgeA,Point3 edgeB ,Point3 vel, float dt,  
								 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel) = 0;
};


enum { collision_supportedcollisions, collision_preframe,collision_postframe,
	   collision_point_to_surface,collision_sphere_to_surface ,collision_box_to_surface,
	   collision_edge_to_surface   };


class CollisionOps : public FPInterface
{
	virtual int SuppportedCollisions(ReferenceTarget *r) = 0; 
	virtual void PreFrame(ReferenceTarget *r, TimeValue &t, TimeValue &dt) = 0;
	virtual void PostFrame(ReferenceTarget *r, TimeValue &t, TimeValue &dt) = 0;
	virtual BOOL CheckCollision (ReferenceTarget *r, TimeValue &t,Point3 *pos, Point3 *vel, float &dt, 
								 float &at, Point3 *hitPoint, Point3 *norm, Point3 *friction, Point3 *inheritedVel) = 0;

//sphere to surface collision
	virtual BOOL CheckCollision (ReferenceTarget *r, TimeValue &t,Point3 *pos, float &radius, Point3 *vel, float &dt, 
								 float &at, Point3 *hitPoint, Point3 *norm, Point3 *friction, Point3 *inheritedVel) = 0;
//box to surface collision FIX ME can't publish box3
	virtual BOOL CheckCollision (ReferenceTarget *r, TimeValue &t, 
								 Point3 *boxCenter,float &w, float &h, float &d, Point3 *vel, float &dt,  
								 float &at, Point3 *hitPoint, Point3 *norm, Point3 *friction, Point3 *inheritedVel) = 0;

//edge to surface collision
	virtual BOOL CheckCollision (ReferenceTarget *r, TimeValue &t,Point3 *edgeA,Point3 *edgeB ,Point3 *vel, float &dt,  
								 float &at, Point3 *hitPoint, Point3 *norm, Point3 *friction, Point3 *inheritedVel) = 0;


};


// block IDs
enum { collisionplane_params, };

// geo_param param IDs
enum { collisionplane_width,
	   collisionplane_height, 
	   collisionplane_quality,
	   collisionplane_node,    
	};
class CollisionPlane : public ICollision
{
private:
	INode *node;
public:
	IParamBlock2 *pblock;
	Interval validity;

	CollisionPlane();
	~CollisionPlane();
//determines what type of collisions are supported
	int SuppportedCollisions() 
		{ 
		return POINT_COLLISION; 
		} 

	void PreFrame(TimeValue t, TimeValue dt) ;
	void PostFrame(TimeValue t, TimeValue dt) {}

	BOOL CheckCollision (TimeValue t,Point3 pos, Point3 vel, float dt, 
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel);
//sphere to surface collision
	BOOL CheckCollision (TimeValue t,Point3 pos, float radius, Point3 vel, float dt, 
		                 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//box to surface collision
	BOOL CheckCollision (TimeValue t, Box3 box, Point3 vel, float dt,  
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}
//edge to surface collision
	BOOL CheckCollision (TimeValue t,Point3 edgeA,Point3 edgeB ,Point3 vel,  float dt,  
						float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//access functions to the pblock
	void SetWidth(TimeValue t, float w)  { pblock->SetValue(collisionplane_width,t,w); }
	void SetHeight(TimeValue t, float h) { pblock->SetValue(collisionplane_height,t,h); }
	void SetQuality(TimeValue t, int q)  { pblock->SetValue(collisionplane_quality,t,q); }
//	void SetTM(TimeValue t, Matrix3 tm)	 { pblock->SetValue(collisionplane_tm,t,&tm); }
	void SetNode(TimeValue t, INode *n)	 { pblock->SetValue(collisionplane_node,t,n);
										   node = n; }

	void SetWidth(Control *c) { pblock->SetController(collisionplane_width,0,c); }
	void SetHeight(Control *c) { pblock->SetController(collisionplane_height,0,c); }
	void SetQuality(Control *c) { pblock->SetController(collisionplane_quality,0,c); }
//	void SetTM(Control *c);

	Matrix3 tm, invtm;
	Matrix3 prevInvTm;

	int initialTime;
	Tab<Matrix3> invTmList;
	float  width, height;
	int quality;



	// Methods from Animatable
	void DeleteThis() { delete this; }
	Class_ID ClassID() {return PLANAR_COLLISION_ID;}
	SClass_ID SuperClassID() {return REF_MAKER_CLASS_ID;}

	// Methods from ReferenceTarget :
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
	void SetReference(int i, RefTargetHandle rtarg) {pblock = (IParamBlock2*)rtarg;}
	RefTargetHandle Clone(RemapDir &remap = NoRemap())
		{
		CollisionPlane* newob = new CollisionPlane();	
		newob->ReplaceReference(0,pblock->Clone(remap));
		BaseClone(this, newob, remap);
		return newob;
		}

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,RefMessage message)
		{
		switch (message) {
			case REFMSG_CHANGE:
				if (hTarget == pblock)
					validity.SetEmpty();
				break;
			}
//note this is ref_stop because we don't want the engine updating it references
//may need a flag to turn this off or on
		return( REF_STOP);
		}


};


// block IDs
enum { collisionsphere_params, };

// geo_param param IDs
enum { collisionsphere_radius,
	   collisionsphere_node,    //using a node right now this really needs to be a TM but it does not look like tms are hooked up yet in pb2
	};


class CollisionSphere : public ICollision
{
private:
	INode *node;
public:
	IParamBlock2 *pblock;
	Interval validity;

	CollisionSphere();
	~CollisionSphere();
//determines what type of collisions are supported
	int SuppportedCollisions() 
		{ 
		return POINT_COLLISION; 
		} 

	void PreFrame(TimeValue t, TimeValue dt) ;
	void PostFrame(TimeValue t, TimeValue dt) {}

	BOOL CheckCollision (TimeValue t,Point3 pos, Point3 vel, float dt, 
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel);
//sphere to surface collision
	BOOL CheckCollision (TimeValue t,Point3 pos, float radius, Point3 vel, float dt, 
		                 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//box to surface collision
	BOOL CheckCollision (TimeValue t, Box3 box, Point3 vel, float dt,  
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}
//edge to surface collision
	BOOL CheckCollision (TimeValue t,Point3 edgeA,Point3 edgeB ,Point3 vel,  float dt,  
						float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//access functions to the pblock
	void SetRadius(TimeValue t, float r)  { pblock->SetValue(collisionsphere_radius,t,r); }
	void SetNode(TimeValue t, INode *n)	 { pblock->SetValue(collisionsphere_node,t,n);
										   node = n; }

	void SetRadius(Control *c) { pblock->SetController(collisionsphere_radius,0,c); }

	Matrix3 tm, invtm;
	Matrix3 prevInvTm;
	Point3 Vc;



	float  radius;



	// Methods from Animatable
	void DeleteThis() { delete this; }
	Class_ID ClassID() {return SPHERICAL_COLLISION_ID;}
	SClass_ID SuperClassID() {return REF_MAKER_CLASS_ID;}

	// Methods from ReferenceTarget :
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
	void SetReference(int i, RefTargetHandle rtarg) {pblock = (IParamBlock2*)rtarg;}
	RefTargetHandle Clone(RemapDir &remap = NoRemap())
		{
		CollisionSphere* newob = new CollisionSphere();	
		newob->ReplaceReference(0,pblock->Clone(remap));
		BaseClone(this, newob, remap);
		return newob;
		}

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,RefMessage message)
		{
		switch (message) {
			case REFMSG_CHANGE:
				if (hTarget == pblock)
					validity.SetEmpty();
				break;
			}
//note this is ref_stop because we don't want the engine updating it references
//may need a flag to turn this off or on
		return( REF_STOP);
		}


};



class CollisionVNormal {
	public:
		Point3 norm;
		DWORD smooth;
		CollisionVNormal *next;
		BOOL init;

		CollisionVNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}
		CollisionVNormal(Point3 &n,DWORD s) {next=NULL;init=TRUE;norm=n;smooth=s;}
		~CollisionVNormal() {delete next;}
		void AddNormal(Point3 &n,DWORD s);
		Point3 &GetNormal(DWORD s);
		void Normalize();
	};


// block IDs
enum { collisionmesh_params, };

// geo_param param IDs
enum { 
		collisionmesh_hit_face_index,
		collisionmesh_hit_bary,
		collisionmesh_node    //using a node right now this really needs to be a TM but it does not look like tms are hooked up yet in pb2
	};


class CollisionMesh : public ICollision
{
private:
	INode *node;
public:
	IParamBlock2 *pblock;
	Interval validity;
	DWORD outFi;
	Point3 outBary;

	CollisionMesh();
	~CollisionMesh();
//determines what type of collisions are supported
	int SuppportedCollisions() 
		{ 
		return POINT_COLLISION; 
		} 

	void PreFrame(TimeValue t, TimeValue dt) ;
	void PostFrame(TimeValue t, TimeValue dt) {}

	BOOL CheckCollision (TimeValue t,Point3 pos, Point3 vel, float dt, 
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel);

//sphere to surface collision
	BOOL CheckCollision (TimeValue t,Point3 pos, float radius, Point3 vel, float dt, 
		                 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//box to surface collision
	BOOL CheckCollision (TimeValue t, Box3 box, Point3 vel, float dt,  
						 float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}
//edge to surface collision
	BOOL CheckCollision (TimeValue t,Point3 edgeA,Point3 edgeB ,Point3 vel,  float dt,  
						float &at, Point3 &hitPoint, Point3 &norm, Point3 &friction, Point3 &inheritedVel)
		{
		return FALSE;
		}

//access functions to the pblock
	void SetNode(TimeValue t, INode *n)	 { 
//check for circle loop here

										   pblock->SetValue(collisionmesh_node,t,n);
										   node = n; }


	Matrix3 tm, invtm;
	Matrix3 tmPrev,invtmPrev;

	Mesh *dmesh;
	int nv,nf;
	CollisionVNormal *vnorms;
	Point3 *fnorms;

//	Mesh *dmeshPrev;
//	VNormal *vnormsPrev;
//	Point3 *fnormsPrev;

	// Methods from Animatable
	void DeleteThis() { delete this; }
	Class_ID ClassID() {return MESH_COLLISION_ID;}
	SClass_ID SuperClassID() {return REF_MAKER_CLASS_ID;}

	// Methods from ReferenceTarget :
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
	void SetReference(int i, RefTargetHandle rtarg) {pblock = (IParamBlock2*)rtarg;}
	RefTargetHandle Clone(RemapDir &remap = NoRemap())
		{
		CollisionSphere* newob = new CollisionSphere();	
		newob->ReplaceReference(0,pblock->Clone(remap));
		BaseClone(this, newob, remap);
		return newob;
		}

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,RefMessage message)
		{
		switch (message) {
			case REFMSG_CHANGE:
				if (hTarget == pblock)
					validity.SetEmpty();
				break;
			}
//note this is ref_stop because we don't want the engine updating it references
//may need a flag to turn this off or on
		return( REF_STOP);
		}


};

#endif __ICOLLSION__H