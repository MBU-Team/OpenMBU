/**********************************************************************
 *<
	FILE: tvutil.h

	DESCRIPTION: Track view utility plug-in class

	CREATED BY:	Rolf Berteig

	HISTORY: 12/18/96

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef __TVUTIL_H__
#define __TVUTIL_H__

class TrackViewUtility;

// The five track view major modes
#define TVMODE_EDITKEYS			0
#define TVMODE_EDITTIME			1
#define TVMODE_EDITRANGES		2
#define TVMODE_POSRANGES		3
#define TVMODE_EDITFCURVE		4

// This is an interface that is given to track view utilities
// that allows them to access the track view they were launched from.
class ITVUtility {
	public:
		virtual int GetNumTracks()=0;
		virtual Animatable *GetAnim(int i)=0;
		virtual Animatable *GetClient(int i)=0;
		virtual int GetSubNum(int i)=0;
		virtual TSTR GetTrackName(int i)=0;
		virtual BOOL IsSelected(int i)=0;
		virtual void SetSelect(int i,BOOL sel)=0;
		virtual HWND GetTrackViewHWnd()=0;
		virtual int GetMajorMode()=0;
		virtual Interval GetTimeSelection()=0;
		virtual BOOL SubTreeMode()=0;
		virtual Animatable *GetTVRoot()=0;

		// This must be called when a track view utility is closing
		// so that it can be unregistered from notifications
		virtual void TVUtilClosing(TrackViewUtility *util)=0;
	};

// This is the base class for track view utilities. Plug-ins will
// derive their classes from this class.
class TrackViewUtility {
	public:
		virtual void DeleteThis()=0;		
		virtual void BeginEditParams(Interface *ip,ITVUtility *iu) {}
		virtual void EndEditParams(Interface *ip,ITVUtility *iu) {}

		virtual void TrackSelectionChanged() {}
		virtual void NodeSelectionChanged() {}
		virtual void KeySelectionChanged() {}
		virtual void TimeSelectionChanged() {}
		virtual void MajorModeChanged() {}
		virtual void TrackListChanged() {}
	};



#endif //__TVUTIL_H__
