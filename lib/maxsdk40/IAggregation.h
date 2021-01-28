/**********************************************************************
 *<
	FILE: IAggregation.h

	DESCRIPTION: Interface to object aggregation manager

	CREATED BY:	John Hutchinson

	HISTORY: Created January 9, 1999

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#pragma once

// DESCRIPTION:
// The IAggregation class is the interface class for representing
// the process of object aggregation

//some constants passed to association class descriptors when editing parameters
#define BEGIN_EDIT	1
#define END_EDIT	2
#define IMAGE_LOAD	3

#define DISPLAY_XRAY  1

typedef enum {eUncommitted,
				ePending,
					ePartial,
					eFully,
					eRejecting,
					eAborting}
commitlevels;

typedef enum {eNoAffinity,
				eSuperClassAffinity,
					ePseudoSuperClassAffinity,
					eClassAffinity,
					eInterfaceAffinity,
					eInstanceAffinity}
aggregationaffinity;

class IAggregation 
{
public:
	virtual void SetActiveComplex(INode* node) = 0;
	virtual INode* GetActiveComplex() = 0;
	virtual void Reset() = 0;
	virtual bool SetProductFactory(SClass_ID superID, Class_ID classID) = 0;
	virtual bool RegisterIntermediateFactory(ClassDesc* cd, int affinity) = 0;
	virtual void ResetFactories() = 0;
	virtual IRollupWindow* GetParamRollup() = 0;
	virtual ClassDesc* GetSelClassDesc() = 0;
	virtual Animatable* GetSelClassTemplate() = 0;
	virtual int ToolbarIndex(ClassDesc* cd) = 0;
	virtual HIMAGELIST ToolbarImagelist(int which) = 0;
	virtual bool RegisterAssocClass(ClassDesc* cd, int whichbar, int ioe_idx, int iod_idx, int iie_idx, int iid_idx) = 0; 
	virtual int CommitAggregation(int action, int flag) = 0;
	virtual void Suspend() = 0;
	virtual void Resume() = 0;
	virtual bool isSuspended() = 0;
	virtual INode* ReactToNode(INode* newnode) = 0;
	virtual commitlevels Status() = 0;
	virtual DWORD DisplayFlags() = 0;
};

class ValenceData
{
public:
	ValenceData():m_type(0){};
	int m_type;
};