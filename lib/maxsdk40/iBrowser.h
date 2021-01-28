/*	
 *	IBrowser.h - Browser Manager public interface
 *
 *		contains public interface defintions for the global manager 
 *		and individual browsers
 *
 *			Copyright © Autodesk, Inc, 2000.  John Wainwright.
 *
 */

#ifndef _H_IBROWSER
#define _H_IBROWSER

#include "iFnPub.h"

#ifdef BLD_BROWSER
#	define BrowserExport __declspec( dllexport )
#else
#	define BrowserExport __declspec( dllimport )
#endif

class IBrowserMgr; 
class IBrowser;

// Core interface to the new MAX browser manager

// Browser Manager interface ID
#define BROWSER_MGR_INTERFACE   Interface_ID(0x65342378, 0x4c540d3b)
inline IBrowserMgr* GetBrowserMgr() { return (IBrowserMgr*)GetCOREInterface(BROWSER_MGR_INTERFACE); }

// class IBrowserMgr
// manager public Core interface 
class IBrowserMgr : public FPStaticInterface 
{
public:
	// new browser windows
	virtual IBrowser*	NewBrowser(TCHAR* rootURL, 
							BOOL showDirectory=TRUE, BOOL showContent=TRUE,
							BOOL showToolbar=TRUE, BOOL showMenu=TRUE, 
							HWND parentWindow=NULL)=0;
	// iterate browsers
	virtual int			NumBrowsers()=0;
	virtual IBrowser*	GetBrowser(int i)=0;

	// interface function IDs 
	enum { 
		newBrowser, 
		numBrowsers, 
		getBrowser, 
	}; 

}; 

// Browser interface ID
#define BROWSER_INTERFACE_DESC   Interface_ID(0x74a22be8, 0x18e117c9)
inline FPInterfaceDesc* GetBrowserInterfaceDesc() { return (FPInterfaceDesc*)GetCOREInterface(BROWSER_INTERFACE_DESC); }

// class IBrowser
// individiual browser interface 
class IBrowser : public FPMixinInterface
{
public:
	// browser control
	virtual void	Close()=0;
	virtual void	ShowDirectory(BOOL onOff)=0;
	virtual void	ShowContent(BOOL onOff)=0;
	virtual void	ShowMenu(BOOL onOff)=0;
	virtual void	ShowToolbar(BOOL onOff)=0;

	// navigation 
	virtual BOOL	SetRootURL(TCHAR* rootURL)=0;
	virtual BOOL	BrowseTo(TCHAR* browseURL)=0;

	// interface function IDs 
	enum { 
		close, 
		showDirectory, 
		showContent, 
		showMenu, 
		showToolbar, 
		setRootURL, 
		browseTo, 
	}; 

	// dispatch map
	BEGIN_FUNCTION_MAP
		VFN_0(close,		Close); 
		VFN_1(showDirectory,ShowDirectory,	TYPE_BOOL); 
		VFN_1(showContent,	ShowContent,	TYPE_BOOL); 
		VFN_1(showMenu,		ShowMenu,		TYPE_BOOL); 
		VFN_1(showToolbar,	ShowToolbar,	TYPE_BOOL); 
		FN_1(setRootURL,	TYPE_BOOL, SetRootURL, TYPE_STRING); 
		FN_1(browseTo,		TYPE_BOOL, BrowseTo,   TYPE_STRING); 
	END_FUNCTION_MAP 

	FPInterfaceDesc*	GetDesc() { return GetBrowserInterfaceDesc(); }

};

#endif


