/**********************************************************************
 *<
	FILE: winutil.h

	DESCRIPTION: Misc. windows related functions

	CREATED BY: Rolf Berteig

	HISTORY: 1-6-95 file created

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __WINUTIL__
#define __WINUTIL__

#include <limits>

float CoreExport GetWindowFloat(HWND hwnd,BOOL *valid=NULL);
int CoreExport GetWindowInt(HWND hwnd,BOOL *valid=NULL);
BOOL CoreExport SetWindowTextInt( HWND hwnd, int i );
BOOL CoreExport SetWindowTextFloat( HWND hwnd, float f, int precision=3 );
BOOL CoreExport SetDlgItemFloat( HWND hwnd, int idControl, float val );
float CoreExport GetDlgItemFloat( HWND hwnd, int idControl, BOOL *valid=NULL );
void CoreExport SetDlgFont( HWND hDlg, HFONT hFont );
void CoreExport SlideWindow( HWND hwnd, int x, int y );
void CoreExport StretchWindow( HWND hwnd, int w, int h );
BOOL CoreExport CenterWindow(HWND hWndChild, HWND hWndParent);
void CoreExport GetClientRectP( HWND hwnd, Rect *rect );
void CoreExport DrawIconButton( HDC hdc, HBITMAP hBitmap, Rect& wrect, Rect& brect, BOOL in );
int CoreExport GetListHieght( HWND hList );
void CoreExport ShadedVertLine( HDC hdc, int x, int y0, int y1, BOOL in );
void CoreExport ShadedHorizLine( HDC hdc, int y, int x0, int x1, BOOL in );
void CoreExport ShadedRect( HDC hdc, RECT& rect );
void CoreExport Rect3D( HDC hdc, RECT& rect, BOOL in );
void CoreExport WhiteRect3D( HDC hdc, RECT rect, BOOL in );
void CoreExport DrawButton( HDC hdc, RECT rect, BOOL in );
void CoreExport XORDottedRect( HWND hwnd, IPoint2 p0, IPoint2 p1, int solidToRight = 0 );
void CoreExport XORDottedCircle( HWND hwnd, IPoint2 p0, IPoint2 p1, int solidToRight = 0 );
void CoreExport XORDottedPolyline( HWND hwnd, int count, IPoint2 *pts, int solid = 0);
void CoreExport XORRect(HDC hdc, RECT& r, int border=1);
void CoreExport MakeButton2State(HWND hCtrl);
void CoreExport MakeButton3State(HWND hCtrl);
int CoreExport GetCheckBox(HWND hw, int id);
				// WIN64 Cleanup: Shuler
void CoreExport SetCheckBox(HWND hw, int id, BOOL b);
BOOL CoreExport DoesFileExist(const TCHAR *file);
int CoreExport GetBitsPerPixel();

// Delete superfluous zeroes from float string: 1.2300000 -> 1.23
void CoreExport StripTrailingZeros(TCHAR* buf);

template<class T> void LimitValue( T& value, T min, T max )
	{
	if ( value < min ) value = min;
	if ( value > max ) value = max;
	}

// mjm - 1.26.99 - safely casts double to float - valid flag will indicate overflow
inline float Dbl2Flt(double val, BOOL *valid = NULL)
{
	if ( val < 0.0f )
	{
		if ( val < -FLT_MAX )
		{
			if (valid) *valid = FALSE;
			return -FLT_MAX;
		}
		if ( val > -FLT_MIN )
		{
			if (valid) *valid = FALSE;
			return -FLT_MIN;
		}
		if (valid) *valid = TRUE;
		return (float)val;
	}

	if ( val > FLT_MAX )
	{
		if (valid) *valid = FALSE;
		return FLT_MAX;
	}
	if ( val < FLT_MIN && val != 0.0 )
	{
		if (valid) *valid = FALSE;
		return FLT_MIN;
	}
	if (valid) *valid = TRUE;
	return (float)val;
}

// mjm - 1.26.99 - safely casts double to int - valid flag will indicate overflow
inline int Dbl2Int(double val, BOOL *valid = NULL)
{
	if ( val > INT_MAX )
	{
		if (valid) *valid = FALSE;
		return INT_MAX;
	}
	if ( val < INT_MIN )
	{
		if (valid) *valid = FALSE;
		return INT_MIN;
	}
	if (valid) *valid = TRUE;
	return (int)val;
}

#define MAKEPOINT( lparam, pt ) { pt.x = (short)LOWORD(lparam); pt.y = (short)HIWORD(lparam); }

// The following two functions extend list boxes. Set the list box to be
// owner draw and then call these two methods in response to the
// WM_MEASUREITEM and WM_DRAWITEM messages.
// 

// Flags to pass to CustListDrawItem
#define CUSTLIST_DISABLED		(1<<0)		// Text is gray
#define CUSTLIST_MED_DISABLED	(1<<1)		// Test is darker gray
#define CUSTLIST_SEPARATOR		(1<<2)		// Draws a separator instead of text
#define CUSTLIST_DBL_SERPARATOR	(1<<3)		// Draw a double line seperator
#define CUSTLIST_RED			(1<<4)      // Text is red

CoreExport void CustListMeasureItem(HWND hList,WPARAM wParam, LPARAM lParam);
CoreExport void CustListDrawItem(HWND hList,WPARAM wParam, LPARAM lParam,DWORD flags);


// MAX extended message box functionality  DB 7/98

#define MAX_MB_HOLD				0x0001		// add "Hold" button
#define MAX_MB_DONTSHOWAGAIN	0x0002		// add "Don't show this dialog again" checkbox

// The first four parameters are just like the Win32 MessageBox routine (but not
// all MessageBox functionality is supported!)
//
// The last two optional args add the functionality listed above -- exType is used
// for adding the additional buttons, and exRet is used for getting the extra
// return info.  For example, if exType includes MAX_MB_DONTSHOWAGAIN, and exRet
// is non-NULL, then exRet will have MAX_MB_DONTSHOWAGAIN set if that checkbox was
// checked by the user.

CoreExport INT_PTR MaxMsgBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT type, UINT exType=0, DWORD *exRet=NULL);
			// WIN64 Cleanup: Shuler

/**********************************************************************
 *
 * alpha blended icon support...
 *
 **********************************************************************/
#include "plugapi.h"

enum LoadMAXIconErrors
	{
	LMI_Ok,
	LMI_ResourceNotFound,
	LMI_ResourceLoadFailed,
	LMI_ImageAndMaskNotCompatible,
	};

CoreExport LoadMAXIconErrors LoadMAXIcon(HINSTANCE hInstance, LPCTSTR resID, LPCTSTR resMaskID, COLORREF bkColor,
		HIMAGELIST imageList, int imageIndex, int preMultAlpha=TRUE);

// returns index of first image into existing imageList
CoreExport int LoadMAXIconFromBMI(LPBITMAPINFOHEADER biImage, LPBITMAPINFOHEADER biMask, COLORREF bkColor, TCHAR* pFilePrefix, int preMultAlpha=TRUE, HIMAGELIST* pImageList=NULL);
CoreExport BITMAPINFOHEADER *LoadBitmapFromFile(TCHAR *filename);

CoreExport void DrawMAXIcon(HDC hDC, Rect &r, HIMAGELIST hList32, HIMAGELIST hList16, int index);

// Compute a good color to use for drawing XOR lines over a particular background color
CoreExport COLORREF ComputeXORDrawColor(COLORREF bkgColor);
// Compute a good color to use for drawing XOR lines over a viewport
CoreExport COLORREF ComputeViewportXORDrawColor();

#endif
