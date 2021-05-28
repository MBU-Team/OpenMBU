/*******************************************************************
 *
 *    DESCRIPTION: Name table.
 *
 *    AUTHOR:	Dan Silva
 *
 *    HISTORY:    
 *
 *******************************************************************/

#ifndef __NAMETAB__H
#define __NAMETAB__H

#include <ioapi.h>

#define NT_INCLUDE			1
#define NT_AFFECT_ILLUM		 2
#define NT_AFFECT_SHADOWCAST  4

class NameTab: public Tab<TCHAR *> {
	ULONG flags;
	public:
		NameTab() { flags = NT_AFFECT_ILLUM|NT_AFFECT_SHADOWCAST; }
		UtilExport ~NameTab();
		UtilExport NameTab& operator=(const NameTab&	 n);
		void SetFlag(ULONG f, BOOL b=1){ if (b) flags|=f; else flags &= ~f; }
		BOOL TestFlag(ULONG f){ return (flags&f)?1:0; }
		UtilExport int AddName(TCHAR *n);
		UtilExport void SetName(int i, TCHAR *n);
		UtilExport void SetSize(int num);
		UtilExport void RemoveName(int i);
		UtilExport int FindName(TCHAR* n);
		UtilExport IOResult Load(ILoad *iload);
		UtilExport IOResult Save(ISave *isave);
	};


#endif
