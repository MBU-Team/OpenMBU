/**********************************************************************
 *<
	FILE:  ikctrl.h

	DESCRIPTION:  Inverse Kinematics Controllers

	CREATED BY:  Rolf Berteig

	HISTORY: 3-1-97

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __IKCTRL_H__
#define __IKCTRL_H__


#define IKMASTER_CLASSID		Class_ID(0xa91004be,0x9901fe83)
#define IKSLAVE_CLASSID			Class_ID(0xbe380a31,0x310dc9e4)

class IKMasterControl : public ReferenceTarget {
	public:
		Class_ID			ClassID() {return IKMASTER_CLASSID;}
		SClass_ID			SuperClassID() {return REF_TARGET_CLASS_ID;}

		virtual void		AddSlaveNode(INode *node)=0;
		virtual void		*GetMasterBase()=0;

		virtual void		SetPosThresh(float t)=0;
		virtual void		SetRotThresh(float t)=0;
		virtual void		SetIterations(int i)=0;
		virtual void		SetStartTime(TimeValue s)=0;
		virtual void		SetEndTime(TimeValue e)=0;

		virtual float		GetPosThresh()=0;
		virtual float		GetRotThresh()=0;
		virtual int			GetIterations()=0;
		virtual TimeValue	GetStartTime()=0;
		virtual TimeValue	GetEndTime()=0;

		virtual void RemoveIKChainControllers(TimeValue t)=0;
	};

class IKSlaveControl : public Control {
	public:
		Class_ID ClassID() {return IKSLAVE_CLASSID;}
		SClass_ID SuperClassID() {return CTRL_MATRIX3_CLASS_ID;}

		virtual IKMasterControl *GetMaster()=0;
		virtual void SetDOF(int which,BOOL onOff)=0;
		virtual void SetInitPos(Point3 pos)=0;
		virtual void SetInitRot(Point3 rot)=0;
		virtual void MakeEE(BOOL onOff,DWORD which,Point3 pos,Quat rot)=0;
	};

CoreExport IKMasterControl *CreateIKMasterControl();
CoreExport IKSlaveControl *CreateIKSlaveControl(IKMasterControl *master,INode *slaveNode);


#endif //__IKCTRL_H__
