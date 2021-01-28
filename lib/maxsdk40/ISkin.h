 /**********************************************************************
 
	FILE: ISkin.h

	DESCRIPTION:  Skin Bone Deformer API

	CREATED BY: Nikolai Sander, Discreet

	HISTORY: 7/12/99


 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

#ifndef __ISKIN__H
#define __ISKIN__H

#include "ISkinCodes.h"

#define I_SKIN 0x00010000
#define I_GIZMO 9815854

#define SKIN_INVALID_NODE_PTR 0
#define SKIN_OK				  1

//#define SKIN_CLASSID Class_ID(0x68477bb4, 0x28cf6b86)
#define SKIN_CLASSID Class_ID(9815843,87654)

class ISkinContextData
{
public:
	virtual int GetNumPoints()=0;
	virtual int GetNumAssignedBones(int vertexIdx)=0;
	virtual int GetAssignedBone(int vertexIdx, int boneIdx)=0;
	virtual float GetBoneWeight(int vertexIdx, int boneIdx)=0;
	
	// These are only used for Spline animation
	virtual int GetSubCurveIndex(int vertexIdx, int boneIdx)=0;
	virtual int GetSubSegmentIndex(int vertexIdx, int boneIdx)=0;
	virtual float GetSubSegmentDistance(int vertexIdx, int boneIdx)=0;
	virtual Point3 GetTangent(int vertexIdx, int boneIdx)=0;
	virtual Point3 GetOPoint(int vertexIdx, int boneIdx)=0;

};

class ISkin 
{
public:
	virtual int GetBoneInitTM(INode *pNode, Matrix3 &InitTM, bool bObjOffset = false)=0;
	virtual int GetSkinInitTM(INode *pNode, Matrix3 &InitTM, bool bObjOffset = false)=0;
	virtual int GetNumBones()=0;
	virtual INode *GetBone(int idx)=0;
	virtual DWORD GetBoneProperty(int idx)=0;
	virtual ISkinContextData *GetContextInterface(INode *pNode)=0;
//new stuff
	virtual TCHAR *GetBoneName(int index) = 0;
	virtual int GetSelectedBone() = 0;
	virtual void UpdateGizmoList() = 0;
	virtual void GetEndPoints(int id, Point3 &l1, Point3 &l2) = 0;
	virtual Matrix3 GetBoneTm(int id) = 0;
	virtual INode *GetBoneFlat(int idx)=0;
	virtual int GetNumBonesFlat()=0;
	virtual int GetRefFrame()=0;

};


class IGizmoBuffer
{
public:
Class_ID cid;
void DeleteThis() { delete this; 	}

};


class GizmoClass : public ReferenceTarget
	{
// all the refernce stuff and paramblock stuff here
public:

	ISkin *bonesMod;
    IParamBlock2 *pblock_gizmo_data;

	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i)
				{
				if (i == 0) return pblock_gizmo_data;
				else return NULL;
				}
	IParamBlock2* GetParamBlockByID(BlockID id)
				{
				if (pblock_gizmo_data->ID() == id) return pblock_gizmo_data ;
				 else return  NULL; 
				 }

	int NumRefs() {return 1;}
	RefTargetHandle GetReference(int i)
		{
		if (i==0)
			{
			return (RefTargetHandle)pblock_gizmo_data;
			}
		return NULL;
		}

	void SetReference(int i, RefTargetHandle rtarg)
		{
		if (i==0)
			{
			pblock_gizmo_data = (IParamBlock2*)rtarg;
			}
		}

    void DeleteThis() { 
					delete this; 
					}


	int NumSubs() {return 1;}
    Animatable* SubAnim(int i) { return GetReference(i);}
 

	TSTR SubAnimName(int i)	{return _T("");	}

	int SubNumToRefNum(int subNum) {return -1;}

	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message)	
		{
		return REF_SUCCEED;
		}

    virtual void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev) {}
    virtual void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next) {}         
    virtual IOResult Load(ILoad *iload) {return IO_OK;}
    virtual IOResult Save(ISave *isave) {return IO_OK;}

//	void* GetInterface(ULONG id);  

//this retrieves the boudng bx of the gizmo in world space
	virtual void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc){}               
// this called in the bonesdef display code to show the gizmo
	virtual int Display(TimeValue t, GraphicsWindow *gw, Matrix3 tm ) { return 1;}
	virtual Interval LocalValidity(TimeValue t) {return FOREVER;}
	virtual BOOL IsEnabled() { return TRUE; }
	virtual BOOL IsVolumeBased() {return FALSE;}
	virtual BOOL IsInVolume(Point3 p, Matrix3 tm) { return FALSE;}

//this is what deforms the point
// this is passed in from the Map call in bones def
	virtual  Point3 DeformPoint(TimeValue t, int index, Point3 initialP, Point3 p, Matrix3 tm)
		{return p;}
//this is the suggested name that the gizmo should be called in the list
	virtual void SetInitialName() {}
//this is the final name of the gizmo in th list
	virtual TCHAR *GetName(){return NULL;}
//this sets the final name of the gizmo in the list
	virtual void SetName(TCHAR *name) {}
// this is called when the gizmo is initially created
// it is passed to the current selected verts in the world space
	//count is the number of vertice in *p
	//*p is the list of point being affected in world space
	//numberOfInstances is the number of times this modifier has been instanced
	//mapTable is an index list into the original vertex table for *p
	virtual BOOL InitialCreation(int count, Point3 *p, int numbeOfInstances, int *mapTable) { return TRUE;}
//this is called before the deformation on a frame to allow the gizmo to do some
//initial setupo
	virtual void PreDeformSetup(TimeValue t) {}
	virtual void PostDeformSetup(TimeValue t) {}

	virtual IGizmoBuffer *CopyToBuffer() { return NULL;}
	virtual void PasteFromBuffer(IGizmoBuffer *buffer) {}

	virtual void Enable(BOOL enable) {}
	virtual BOOL IsEditing() { return FALSE;}
	virtual void EndEditing() {}
	virtual void EnableEditing(BOOL enable) {}

// From BaseObject
    virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc, Matrix3 tm) {return 0;}
    virtual void SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE) {}
    virtual void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, Matrix3 tm, BOOL localOrigin=FALSE ) {}
    virtual void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node, Matrix3 tm) {}
    virtual void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node, Matrix3 tm) {}
    virtual void ClearSelection(int selLevel) {}
	virtual void SelectAll(int selLevel) {}
    virtual void InvertSelection(int selLevel) {}



		
	};

/*
Modifier* FindSkinModifier (INode* node)
{
	// Get object from node. Abort if no object.
	Object* pObj = node->GetObjectRef();
	

	if (!pObj) return NULL;

	// Is derived object ?
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* pDerObj = static_cast<IDerivedObject*>(pObj);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < pDerObj->NumModifiers())
		{
			// Get current modifier.
			Modifier* mod = pDerObj->GetModifier(ModStackIndex);

			// Is this Skin ?
			if (mod->ClassID() == SKIN_CLASSID )
			{
				// Yes -> Exit.
				return mod;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		pObj = pDerObj->GetObjRef();
	}

	// Not found.
	return NULL;
}
*/
#endif 