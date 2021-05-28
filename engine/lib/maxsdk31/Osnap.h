/**********************************************************************
 *<
	FILE: osnap.h

	DESCRIPTION:  Classes for Osnaps

	CREATED BY: John Hutchinson

	HISTORY: December 9, 1996
 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _OSNAP_H_
#define _OSNAP_H_

#include "tab.h"
#define RESET 0
#define NEXT 1
#define ICON_WIDTH 32

#define HILITE_NORMAL	(1<<0)
#define HILITE_BOX		(1<<1)
#define HILITE_NODE		(1<<2)//Not implemented
#define HILITE_CROSSHAIR (1<<3)

#define OSNAP_STATE	_T("ObjectSnapPluginState")

//Some types
class Candidate
{
public:
	Candidate(){};
	Candidate(Point3* p, int s, int num=0, ...);
	void SetMeshPoint(int which, const Point3 p);
	~Candidate();
	Point3 *pt;
	int type;
	int count;
	Point3 *meshverts;
};

//typedef Tab<Point3> Point3Tab;
typedef Tab<Candidate *> CandidateTab;
typedef	void (*SnapCallback) (Object* pobj, IPoint2 *p) ;

class OsnapMarker; 
class HitMesh;
class IOsnapManager;

//The osnap class
//===========================================================================

class Osnap {
	friend class OsnapHit;
	friend class OsnapManager;

protected:
	BOOL *m_active;
	DllExport void _Snap(INode* inode, TimeValue t, ViewExp *vpt, IPoint2 *p, SnapInfo *snap);
	DllExport Point3 _ReEvaluate(TimeValue t, OsnapHit *hit);
	DllExport boolean IsActive();
	DllExport void SetActive(int index, boolean state);
	DllExport boolean GetActive(int index);

	GraphicsWindow *m_hitgw;

	//Point management.
	//Some osnaps may want to maintain a list of potential hit points. 
	//Note that the points should be passed in in modeling space
	DllExport void AddCandidate(Point3 *pt, int type = -1, int num = 0,...);
	DllExport Point3 *GetCandidatePoint(int index);
	DllExport void GetCandidateMesh(int index, HitMesh *m);
	DllExport int GetCandidateType(int index);
//	DllExport void AddCandidate(Point3 *pt);
	DllExport void ClearCandidates();
	DllExport int NumCandidates(){return point_candidates.Count();}
	virtual DllExport Point3 ReEvaluate(TimeValue t, OsnapHit *hit, Object* pobj);

	//Note: the following version uses an index into the candidate list
	DllExport BOOL CheckPotentialHit(int ptindex, Point2 cursor);
	//Note: the following version uses a remote matrix of points
	DllExport BOOL CheckPotentialHit(Point3 *p, int ptindex, Point2 cursor);
	int m_baseindex;//an index into the tool array

public:

	DllExport Osnap();//constructor
	DllExport virtual ~Osnap();
	DllExport void Init();

	virtual int numsubs(){return 1;}; //the number of subsnaps this guy has
	virtual DllExport TCHAR *Category();//JH 01/04/98 {return NULL;}
	virtual Class_ID ClassID() { return Class_ID( 0, 0); }
	virtual BOOL UseCallbacks(){return FALSE;}
	virtual int NumCallbacks(){return 0;}
	virtual DllExport BOOL GetSupportedObject(INode *iNode, TimeValue t, ObjectState *os);

	virtual TSTR *snapname(int index)=0; // the snap’s name to be displayed in the UI
	virtual TSTR *tooltip(int index){return NULL;} // the snap’s name to be displayed in the UI
	virtual boolean ValidInput(SClass_ID scid, Class_ID cid)=0;//the objects it supports 
	virtual OsnapMarker *GetMarker(int index)=0; // single object might contain subsnaps
	virtual WORD HiliteMode(){return HILITE_NORMAL;}
	// UI methods
	virtual boolean BeginUI(HWND hwnd){return TRUE;}
	virtual void EndUI(HWND hwnd){};
	virtual HBITMAP getTools()=0;
	virtual HBITMAP getMasks()=0;
	virtual void Snap(Object* pobj, IPoint2 *p, TimeValue t){};
	virtual BOOL HitTest(Object* pobj, IPoint2 *p, TimeValue t){return TRUE;}
	virtual SnapCallback GetSnapCallback( int sub){ return NULL;}
	virtual WORD AccelKey(int index)=0;


protected://data
	IOsnapManager *theman;
//	Point3Tab point_candidates; //this will hold the point candidates
  	CandidateTab point_candidates; //this will hold the point candidates

public://data
//	INode *m_inode;
//	ViewExp *m_vpt;

};






#endif // _OSNAP_H_
