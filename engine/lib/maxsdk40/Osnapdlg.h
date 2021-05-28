/**********************************************************************
 *<
	FILE: OsnapDlg.h

	DESCRIPTION: Declares class for the Osnap Dialog

	CREATED BY:	John Hutchinson

	HISTORY: January 11  '97

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef __OSNAPDLG__
#define __OSNAPDLG__
#include "tabdlg.h"

// The dimensions of the vertical toolbar that gets constructed for the UI
#define CHECKBAR_WIDTH 225//230
#define CHECKBAR_HEIGHT 130//150//113
#define CHECKBAR_HOFFSET 5
#define CHECKBAR_VOFFSET 30

class OsnapDlg : public TabbedDialog {
	public:
		HWND	hSnapCat;
		static int curCat;
		OsnapManager *theMan;
		BOOL valid, spinDown, block;
		HWND hWnd;
		IVertToolbar *iCheckbar;
		int	DoDialog(int page);

		ISpinnerControl *iAbs[3], *iRel[3], *iDolly, *iRoll;
		
		static int winX, winY;

		OsnapDlg(HWND appWnd,HINSTANCE hInst);
		~OsnapDlg();

		void Invalidate();
		void Update();
		void Init(HWND hWnd);
		void ChangeCat(int cat);


		void WMCommand(int id, int notify, HWND hCtrl);
		void WMSize(int how);

	};


void ShowOsnapDlg(HWND hWnd,HINSTANCE hInst,int page=0);
void HideOsnapDlg();

class OsnapOffset {
public:
	OsnapOffset(HWND HWnd, HINSTANCE hInst);
	~OsnapOffset();

	void Init(HWND dWnd);
	void Update();
	void OnCommand(int id);
	void OnSpinnerChange(int id);
	
private:
	ISpinnerControl *iAbs[3], *iRel[3];
	Point3 refpoint, pAbs, pRel;
	OsnapManager *theman;
	HWND hWnd;

};

extern void OffsetOsnap(HWND hWnd, HINSTANCE hInst);
#endif //__OSNAPDLG__
