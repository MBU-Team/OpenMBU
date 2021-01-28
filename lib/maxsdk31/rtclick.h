/*********************************************************************
 *<
	FILE: rtclick.h

	DESCRIPTION: Right-click menu functionality

	CREATED BY:	Tom Hudson

	HISTORY: Created 14 June 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __RTCLICK__
#define __RTCLICK__

const int kMaxMenuEntries = 1000;

class RightClickMenuManager;

class RightClickMenu {
	public:
		virtual void Init(RightClickMenuManager* manager, HWND hWnd, IPoint2 m)=0;
		virtual void Selected(UINT id)=0;
	};

typedef RightClickMenu* PRightClickMenu;
typedef Tab<PRightClickMenu> PRightClickMenuTab;

class RCMData {
	public:
		RightClickMenu *menu;
		UINT menuId;
		UINT managerId;
		RCMData() { menu = NULL; menuId = managerId = 0; };
		RCMData(RightClickMenu *menu, UINT menuId, UINT managerID);
	};

typedef Tab<RCMData> RCMDataTab;

class RightClickMenuManager {
	private:
        Stack<HMENU> menuStack;
        Tab<HMENU>   allSubMenus;
		HMENU theMenu;
		PRightClickMenuTab menuTab;
		RCMDataTab dataTab;
		int index;
		int limit;
		int startId;
	public:
		CoreExport void Register(RightClickMenu *menu);
		CoreExport void Unregister(RightClickMenu *menu);
		CoreExport void Init(HMENU menu, int startId, int limit, HWND hWnd, IPoint2 m);
        CoreExport void CleanUp();

		CoreExport int AddMenu(RightClickMenu *menu, UINT flags, UINT id, LPCTSTR data);
        CoreExport int BeginSubMenu(LPCTSTR name);
        CoreExport int EndSubMenu();

		CoreExport void Process(UINT id);
	};

#endif // __RTCLICK__
