/**********************************************************************
 *<
	FILE: helpsys.h

	DESCRIPTION: Help Class include file.

	CREATED BY: greg finch

	HISTORY:

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

#ifndef _HELPSYS_H_
#define _HELPSYS_H_

#include "contextids.h"
#include "export.h"

#define F1Focus(cmd,data)	getHelpSys().setHelpFocus(cmd,data)
#define F1Help()			getHelpSys().doHelpFocus()
#define Help(cmd,data)		getHelpSys().help(cmd, data)
#define GetClickHelp()		getHelpSys().getClickHelp()

class DllExport HelpSys {
public:
	HelpSys();
	~HelpSys();

    void		setAppHInst(HINSTANCE h);

	void		setClickHelp(int onOff);
	int			getClickHelp()		{ return clickHelp; }
	void		setHelpHWnd(HWND h)	{ helpHWnd = h; }
	HWND		getHelpHWnd()		{ return helpHWnd; }
	void		setHelpFocus(UINT uCommand, DWORD dwData);
	int			doHelpFocus();
	int			help(UINT uCommand, DWORD dwData);
	//int         getKeyID(int which);
	void		setExportedFunctionPointers(void (*enableAcc)(), void (*disableAcc)(), BOOL (*accEnabled)());

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
