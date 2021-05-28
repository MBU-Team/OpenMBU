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