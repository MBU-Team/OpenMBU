/**********************************************************************
 *<
	FILE: arcdlg.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __ARCDLG__H
#define __ARCDLG__H


class ArcballDialog {
	public:
	virtual void DeleteThis()=0;  
	};

class ArcballCallback {
	public:
	virtual void StartDrag()=0;   // called when drag begins (may want to save state at this point)
	virtual void EndDrag()=0;   // called when drag ends
	virtual void Drag(Quat q, BOOL buttonUp)=0;  // called during drag, with q=relative rotation from start
	virtual void CancelDrag()=0;  // called when right button clicked during drag
	virtual	void BeingDestroyed()=0;  // called if the window was closed
	};

CoreExport ArcballDialog *CreateArcballDialog(ArcballCallback *cb, HWND hwndOwner, TCHAR* title=NULL);


#endif
