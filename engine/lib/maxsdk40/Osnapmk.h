/**********************************************************************
 *<
	FILE: osnapmk.h

	DESCRIPTION:  A Class for an osnapmarker

	CREATED BY: John Hutchinson

	HISTORY: Feb 12, 1996
 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _OSNAP_MARK_H_
#define _OSNAP_MARK_H_

class OsnapMarker 
{
private:
	int m_numpoints;
	IPoint3 *m_ppt;
	IPoint3 *m_pcache;
	IPoint3 m_cache_trans;
	int m_cache_size;
	int *m_edgevis;
	boolean IsCacheValid(IPoint3 trans, int size);
	void UpdateCache(IPoint3 trans, int size);

public:
	CoreExport OsnapMarker();
	CoreExport ~OsnapMarker();
	CoreExport OsnapMarker(int n, IPoint3 *ppt, int *pes);
	CoreExport OsnapMarker(const OsnapMarker& om);
	CoreExport OsnapMarker& operator=(const OsnapMarker& om);
	void display(IPoint3 xyz, int markersize, GraphicsWindow *gw);
};

#endif