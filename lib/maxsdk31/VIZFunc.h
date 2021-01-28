/**********************************************************************
 *<
	FILE: VIZFunc.h

	DESCRIPTION:  General header file for VIZ functionality that
	needs to be exposed across projects.

	CREATED BY: Michael Larson

	HISTORY: created 11/12/98

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/
#pragma once
#ifndef _VIZFUNC_H_
#define _VIZFUNC_H_

//This class is a catch all for functionality that will show up in VIZ
//Note that this guy will be instantiated only once.
class VIZFunc
{
private:
  static bool mIsInstantiated;

public:
//methods
VIZFunc();
~VIZFunc() {;}
CoreExport bool IsSubObjAnimEnabled(SClass_ID sclID, Class_ID clID);   //for checking to see if subobject is enabled for this class ID
CoreExport bool IsPluginPresent(SClass_ID sclID, Class_ID clID);
};

extern CoreExport VIZFunc gVIZFunc;

inline TSTR TwiddleResString(int MAXName, int VIZName)
	{
#ifdef DESIGN_VER
	return GetResString(VIZName);
#else
	return GetResString(MAXName);
#endif
	}

#endif //_VIZFunc_H_



