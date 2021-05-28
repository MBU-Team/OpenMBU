/**********************************************************************
 *<
	FILE: fbwin.h

	DESCRIPTION: framebuffer window include file.

	CREATED BY: Don Brittain

	HISTORY:

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#if !defined(_FBWIN_H_)

#define _FBWIN_H_

#ifdef WIN95STUFF
#include <vfw.h>
#endif

#define FBW_MSG_OFFSET (WM_USER + 1000)

#define FBW_LBUTTONDOWN		(WM_LBUTTONDOWN + FBW_MSG_OFFSET)
#define FBW_LBUTTONUP		(WM_LBUTTONUP + FBW_MSG_OFFSET)
#define FBW_LBUTTONDBLCLK	(WM_LBUTTONDBLCLK + FBW_MSG_OFFSET)
#define FBW_RBUTTONDOWN		(WM_RBUTTONDOWN + FBW_MSG_OFFSET)
#define FBW_RBUTTONUP		(WM_RBUTTONUP + FBW_MSG_OFFSET)
#define FBW_RBUTTONDBLCLK	(WM_RBUTTONDBLCLK + FBW_MSG_OFFSET)
#define FBW_MOUSEMOVE		(WM_MOUSEMOVE + FBW_MSG_OFFSET)


class FB_RGBA_Pixel {
	union {
		struct {
			BYTE red;
			BYTE green;
			BYTE blue;
			BYTE alpha;
		} rgba;
		DWORD pix;
	};
};

// framebuffer window setup structure
class FBWinSetup {
public:
    DllExport FBWinSetup();
	DWORD			winStyle;
	POINT			winSize;
	POINT			winPlace;
	POINT			fbSize;
};

class FrameBufferWindow {
public:
	DllExport	FrameBufferWindow(HWND hParent, FBWinSetup &fbw);
	DllExport	~FrameBufferWindow();

				HWND	getHWnd()			{ return hWnd; }
				int		getBitsPerPixel()	{ return bpp; }

				void	setNotify(int n)	{ notify = n; }
				int		getNotify()			{ return notify; }

				int		getOriginX()	{ return origin.x; }
				int		getOriginY()	{ return origin.y; }
				void	setOrigin(int x, int y)	{ origin.x = x; origin.y = y; }

				int		getFbSizeX()	{ return fbSize.x; }
				int		getFbSizeY()	{ return fbSize.y; }
#if 0
	DllExport	void	setFbSize(int x, int y);

	DllExport	void	loadDIB(LPBITMAPINFOHEADER dib);
#endif
	
	DllExport	void	startScanlineLoad();
	DllExport	void	scanline(int line, int start, int count, FB_RGBA_Pixel *pixels);
	DllExport	void	scanline(int line, int start, int count, BYTE *red, BYTE *green, BYTE *blue);
	DllExport	void	endScanlineLoad();

	friend LRESULT CALLBACK FBWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void 				onPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void				onSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void				setupPalette();
	void				setupDIB();

	static int			refCount;
	HWND				hWnd;
	HWND				hParent;

	int					notify;

#ifdef WIN95STUFF
	HDRAWDIB			hDrawDC;
#endif
	int					bpp;
	int					paletted;

	POINT				winSize;
	POINT				fbSize;
	POINT				origin;

	int					dibLoaded;
	LOGPALETTE *		logPal;
	LPBITMAPINFOHEADER	pbih;
	BYTE *				pixBuf;
	int					pixBufSize;
};


#endif // _FBWIN_H_
