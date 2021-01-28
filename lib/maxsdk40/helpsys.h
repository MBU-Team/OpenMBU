/**********************************************************************
*<

	FILE: helpsys.h

	DESCRIPTION: Help Class include file.

	CREATED BY: greg finch

	HISTORY: Clean-up and added code to support individual html 
			 files (GG:09/26/00)

*>
**********************************************************************/

#ifndef _HELPSYS_H_
#define _HELPSYS_H_

#include "contextids.h"
#include "export.h"

#include "udmIA64.h"

// WIN64 Cleanup: Shuler
// These defines will be in a future basetsd.h, probably
// in VC 7.0. Until then, they must be defined here.

#define F1Focus(cmd,data)	getHelpSys().setHelpFocus(cmd,data)
#define F1Help()			getHelpSys().doHelpFocus()
#define DoHelp(cmd,data)	getHelpSys().help(cmd, data)
#define GetClickHelp()		getHelpSys().getClickHelp()

class DllExport HelpSys {

public:
	
				HelpSys						( );
				~HelpSys					( );

    void		setAppHInst					(HINSTANCE h);
	void		setClickHelp				(int onOff);
	int			getClickHelp				( ) { return clickHelp; }
	void		setHelpHWnd					(HWND h) { helpHWnd = h; }
	HWND		getHelpHWnd					( ) { return helpHWnd; }
	void		setHelpFocus				(UINT uCommand, DWORD dwData);
	int			doHelpFocus					( );
	int			help						(UINT uCommand, ULONG_PTR dwData);
	void		setExportedFunctionPointers	(void (*enableAcc)(), void (*disableAcc)(), BOOL (*accEnabled)());

private:

	int			clickHelp;
	HWND		helpHWnd;
	HCURSOR		helpCursor;
	HCURSOR		savedCursor;
	UINT		focusCmd;
	DWORD		focusData;

};

struct IDPair {
	DWORD CID;
	DWORD HID;
};

DllExport DWORD     CIDtoHID(int CID, IDPair *array);
DllExport void      SetDialogHelpIDs(HWND hDlg, IDPair *array);
DllExport HelpSys & getHelpSys(void);
DllExport HWND		GetHTMLHelpHWnd();

#endif // _HELPSYS_H_
