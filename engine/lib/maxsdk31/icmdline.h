/**********************************************************************
 *<
	FILE:			icmdline.h

	DESCRIPTION:	Class definitions for command line panel interface

	CREATED BY:		Christer Janson

	HISTORY:		Created 26 September 1997

 *>	Copyright (c) Autodesk, 1997, All Rights Reserved.
 **********************************************************************/

#if !defined(_ICMDLINE_H_)
#define _ICMDLINE_H_

class CommandLineCallback {
public:
	virtual BOOL	ExecuteCommand(TCHAR* szCmdLine) { return FALSE; };
	virtual void	GotKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {};
};

class ICommandLine {
public:
	virtual BOOL	RegisterCallback(CommandLineCallback* cb) = 0;
	virtual BOOL	UnRegisterCallback(CommandLineCallback* cb) = 0;
	virtual void	SetVisibility(BOOL bShow) = 0;
	virtual BOOL	GetVisibility() = 0;
	virtual	BOOL	Prompt(TCHAR* szCmdLine) = 0;

	// Set the actual string in the command line editor
	virtual	BOOL	SetCommandLineText(TCHAR* szCmdLine) = 0;
};

#endif // _ICMDLINE_H_
