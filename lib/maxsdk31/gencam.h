/**********************************************************************
 *<
	FILE: gencamera.h

	DESCRIPTION:  Defines General-Purpose cameras

	CREATED BY: Tom Hudson

	HISTORY: created 5 December 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __GENCAM__H__ 

#define __GENCAM__H__

// Camera types
#define FREE_CAMERA 0
#define TARGETED_CAMERA 1
#define PARALLEL_CAMERA 2

#define NUM_CAM_TYPES 2

class GenCamera: public CameraObject {			   
	public:
		virtual GenCamera *NewCamera(int type)=0;
		virtual void SetConeState(int s)=0;
		virtual int GetConeState()=0;
		virtual void SetHorzLineState(int s)=0;
		virtual int GetHorzLineState()=0;
		virtual void Enable(int enab)=0;
		virtual BOOL SetFOVControl(Control *c)=0;
		virtual void  SetFOVType(int ft)=0;
		virtual int GetFOVType()=0;
		virtual Control *GetFOVControl()=0;
		virtual	int  Type()=0;
		virtual void SetType(int tp)=0;

		virtual void SetDOFEnable(TimeValue t, BOOL onOff) {}
		virtual BOOL GetDOFEnable(TimeValue t, Interval& valid = Interval(0,0)) { return 0; }
		virtual void SetDOFFStop(TimeValue t, float fs) {}
		virtual float GetDOFFStop(TimeValue t, Interval& valid = Interval(0,0)) { return 1.0f; }
	};



#endif // __GENCAM__H__
