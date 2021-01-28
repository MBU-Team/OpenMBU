/**********************************************************************
 *<
	FILE: IXref.h

	DESCRIPTION: Object XRef API

	CREATED BY: Nikolai Sander

	HISTORY: created 7/7/00

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IXREF_H__
#define __IXREF_H__

class IObjXRefManager : public FPStaticInterface 
{
	public:
	
	// function IDs 
	enum { 
		   fnIdAddXRefObject,
		   fnIdGetNumXRefObjects,
		   fnIdGetXRefObject,
		   fnIdGetNumFiles,
		   fnIdGetFileName,
		   fnIdReloadFile,
		   fnIdIsFileUnresolved,
		   fnIdIsFileDisabled,
		   fnIdGetAllXRefObjects,
	};
	
	virtual IXRefObject *AddXRefObject(TCHAR *fname, TCHAR *obname, BOOL proxy = false)=0;

	virtual int GetNumXRefObjects(TCHAR *fname)=0;
	virtual IXRefObject *GetXRefObject(TCHAR *fname, int i)=0;
	
	virtual int GetNumFiles()=0;
	virtual TCHAR *GetFileName(int fidx)=0;
	virtual BOOL ReloadFile(TCHAR *fname)=0;
	
	virtual BOOL IsFileUnresolved(TCHAR *fname)=0;
	virtual BOOL IsFileDisabled(TCHAR *fname)=0;
	virtual void GetAllXRefObjects(Tab<IXRefObject*> &objs)=0;
};

#define OBJXREFMANAGER_INTERFACE Interface_ID(0x7ede1c65, 0x353d271f)
inline IObjXRefManager* GetObjXRefManager () { return (IObjXRefManager*)GetCOREInterface(OBJXREFMANAGER_INTERFACE); }

#endif