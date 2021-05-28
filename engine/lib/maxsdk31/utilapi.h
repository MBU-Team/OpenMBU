/**********************************************************************
*<
	FILE: utilapi.h

	DESCRIPTION: Utility plug-ins interface

	CREATED BY: Rolf Berteig

	HISTORY: 12-23-95 file created

*>	Copyright (c) 1994, All Rights Reserved.
**********************************************************************/

#ifndef __UTILAPI__
#define __UTILAPI__

class IUtil {
	public:
		// Closes the current utility in the command panel
		virtual void CloseUtility()=0;
	};

// A utility plug-in object
class UtilityObj {
	public:
		virtual void BeginEditParams(Interface *ip,IUtil *iu)=0;
		virtual void EndEditParams(Interface *ip,IUtil *iu)=0;
		virtual void SelectionSetChanged(Interface *ip,IUtil *iu) {}
		virtual void DeleteThis()=0;
		virtual void SetStartupParam(TSTR param) {}
	};


#endif //__UTILAPI__

