/**********************************************************************
 *<
	FILE: sceneapi.h

	DESCRIPTION: Scene interface

	CREATED BY:	Rolf Berteig

	HISTORY: Created 13 January 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef __SCENEAPI__
#define __SCENEAPI__


class BaseObject;

class IScene {		
	public:
		virtual int EnumTree( ITreeEnumProc *proc )=0;
		virtual void FlagFGSelected( TimeValue t )=0;
		virtual void FlagFGAnimated( TimeValue t )=0;
		virtual void FlagFGDependent( TimeValue t, BaseObject *obj )=0;
	};


// The purpose of this callback is to call FlagForeground() for
// any nodes in the scene that are supposed to be in the foreground.
class ChangeForegroundCallback {
	public:
		virtual BOOL IsValid()=0;
		virtual void Invalidate()=0;
		virtual void Validate()=0;
		virtual void callback(TimeValue t,IScene *scene)=0;
	};

#endif // __SCENEAPI__
