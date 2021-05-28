/**********************************************************************
 *<
	FILE: mcapdev.h

	DESCRIPTION: Motion capture device plug-in interface

	CREATED BY: Rolf Berteig

	HISTORY: May 01, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef _MCAPDEV_
#define _MCAPDEV_


class IMCParamDlg;
class MCDeviceBinding;

// Motion capture controller class IDs
#define POS_MOTION_CLASS_ID			0xff8826de
#define ROT_MOTION_CLASS_ID			0xff7826df
#define SCALE_MOTION_CLASS_ID		0xff6826da
#define FLOAT_MOTION_CLASS_ID		0xff5826db
#define POINT3_MOTION_CLASS_ID		0xff4826dc

// If a controller has one of the above class IDs, then it
// can be cast into this class.
class IMCControl : public Control {
	public:
		virtual BOOL IsLiveOn()=0;
		virtual BOOL IsRecordOn()=0;
		virtual int NumDeviceBindings()=0;
		virtual MCDeviceBinding *GetDeviceBinding(int i)=0;
		virtual void SetDeviceBinding(int i,MCDeviceBinding *b)=0;		
		virtual void GetValueLive(TimeValue t,void *val, GetSetMethod method)=0;
	};

class IMCapManager {
	public:
		virtual void MidiNote(int channel, int note)=0;
		virtual TimeValue GetTime()=0;
	};

// Base class for an input device
class MCInputDevice {
	public:
		virtual TSTR DeviceName()=0;
		virtual MCDeviceBinding *CreateBinding()=0;
		virtual void UtilityStarted(IMCapManager *im) {}
		virtual	void UtilityStopped(IMCapManager *im) {}
		virtual void Cycle(UINT tick) {}		

	};

// An instance of this class is created when a motion caprture controller
// binds one of its parameters to a device. The main purpose of this
// class is to store any parameters that describe the binding.
class MCDeviceBinding : public ReferenceTarget {
	public:
		virtual MCInputDevice *GetDevice()=0;
		virtual TSTR BindingName()=0;
		virtual float Eval(TimeValue t)=0;
		virtual void DeleteThis()=0;		
		virtual void AddRollup(IMCParamDlg *dlg)=0;
		virtual void UpdateRollup(IRollupWindow *iRoll)=0;
		virtual void BeginActivate(BOOL reset=TRUE) {}
		virtual void EndActivate() {}
		virtual void Accumulate(TimeValue t) {}

		SClass_ID SuperClassID() {return MOT_CAP_DEVBINDING_CLASS_ID;}		
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message) {return REF_SUCCEED;}

	};

class IMCParamDlg : public ReferenceMaker {
	public:
		MCDeviceBinding *binding;
		IRollupWindow *iRoll;
	};



#endif //_MCAPDEV_

