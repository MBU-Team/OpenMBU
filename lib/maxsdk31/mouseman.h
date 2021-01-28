/**********************************************************************
 *<
	FILE: mouseman.h

	DESCRIPTION:  Manages mouse input

	CREATED BY: Rolf Berteig
	
	HISTORY: created 18 October 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MOUSEMAN__
#define __MOUSEMAN__

#include "coreexp.h"

/* To create a mouse call back, derive a sub class of this
 * class and redefine 'proc'
 */

class MouseManager;

class MouseCallBack {
		MouseManager * mouseMan;
	public:
		virtual ~MouseCallBack() {}

		CoreExport virtual int proc( 
			HWND hwnd, 
			int msg, 
			int point, 
			int flags, 
			IPoint2 m );
		virtual void pan(IPoint2 offset) {}
		virtual int override(int mode) { return mode; }		// Return given mouse mode by default
		void setMouseManager(MouseManager *mm)  { mouseMan = mm; }
		MouseManager *getMouseManager()  { return mouseMan; }

		// Transform Gizmo Interface
		virtual BOOL SupportTransformGizmo() { return FALSE; }
		virtual void DeactivateTransformGizmo() {}
		// End of Transform Gizmo Interface

		virtual BOOL SupportAutoGrid(){return FALSE;}
		virtual BOOL TolerateOrthoMode(){return FALSE; }
	};


// Messages to a mouse procedure and mouseModes.
//
#define MOUSE_ABORT 		0
#define MOUSE_IDLE			0
#define MOUSE_POINT			1
#define MOUSE_MOVE			2
#define MOUSE_DBLCLICK		3
#define MOUSE_INIT			4
#define MOUSE_UNINIT		5
#define MOUSE_FREEMOVE		6
#define MOUSE_KEYBOARD		7
#define MOUSE_PROPCLICK		8
#define MOUSE_SNAPCLICK		9

// Drag modes.
#define CLICK_MODE_DEFAULT	0	// Returned by CreateMouseCallBack to indicate use of system mouse mode
#define CLICK_DRAG_CLICK	1
#define CLICK_MOVE_CLICK	2
#define CLICK_DOWN_POINT	3	// Point messages on mouse-down only

// Buttons
#define LEFT_BUTTON			0
#define MIDDLE_BUTTON		1
#define RIGHT_BUTTON		2

// Flags to mouse callback.
#define MOUSE_SHIFT			(1<<0)
#define MOUSE_CTRL			(1<<1)
#define MOUSE_ALT			(1<<2)
#define MOUSE_LBUTTON		(1<<3)	// Left button is down
#define MOUSE_MBUTTON		(1<<4)	// Middle button is down
#define MOUSE_RBUTTON		(1<<5)	// Right button is down

class MouseManager {
	private:
		// This is shared by all instances.
		static int clickDragMode;
		
		// These are local to each instance.
		int 			mouseMode;
		int 			curPoint;
		int         	curButton;
		MouseCallBack 	*TheMouseProc[3];
		int 			numPoints[3];
		int				buttonState[3];
		int				mouseProcReplaced;
		int 			inMouseProc;
#ifdef _OSNAP
		UINT			m_msg;
#endif
		HWND			captureWnd;

	public:
		// Constructor/Destructor
		CoreExport MouseManager();
		CoreExport ~MouseManager();

		CoreExport int SetMouseProc( MouseCallBack *mproc, int button, int numPoints=2 );
		CoreExport int SetDragMode( int mode );
		CoreExport int GetDragMode( );
		CoreExport int SetNumPoints( int numPoints, int button );
		CoreExport int ButtonFlags();
		CoreExport void Pan(IPoint2 p);
		CoreExport LRESULT CALLBACK MouseWinProc( 
			HWND hwnd, 
			UINT message, 
			WPARAM wParam, 
			LPARAM lParam );
		
		// RB 4-3-96: Resets back to the MOUSE_IDLE state
		CoreExport void Reset();
		int GetMouseMode() {return mouseMode;}
#ifdef _OSNAP
		UINT GetMouseMsg() {return m_msg;}
		int GetMousePoint() {return curPoint;}
#endif

		CoreExport void SetCapture(HWND hWnd);
		CoreExport HWND HasCapture();
		CoreExport void ReleaseCapture();
		CoreExport void RestoreCapture();
	};


#define WM_MOUSEABORT	(WM_USER + 7834)

// Indicates if ANY mouse proc is currently in the process of
// aborting a mouse proc.
CoreExport BOOL GetInMouseAbort();


#endif // __MOUSEMAN__
