/**********************************************************************
 *<
	FILE: iurvctl.h

	DESCRIPTION:	CurveControl Interface

	CREATED BY:		Nikolai Sander, Kinetix

	HISTORY:		Created 10/15/98

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/


#ifndef __ICURVECOTL__H
#define __ICURVECOTL__H

#define CURVE_CONTROL_CLASS_ID Class_ID(0x14585773, 0x483a7dcf)

#define I_RESMAKER_INTERFACE 0x2474334a

#define WM_CC_SEL_CURVEPT         WM_USER+0x2b70 // lParam = ICurve * , LOWORD(wParam) = number of points selected
#define WM_CC_CHANGE_CURVEPT      WM_USER+0x2b71 // lParam = ICurve * , LOWORD(wParam) = point index
#define WM_CC_CHANGE_CURVETANGENT WM_USER+0x2b72 // lParam = ICurve * , LOWORD(wParam) = point index, HIWORD(wParam) & IN_CURVETANGENT_CHANGED->InTangent, HIWORD(wParam)&OUT_CURVETANGENT_CHANGED->OutTangent
#define WM_CC_DEL_CURVEPT         WM_USER+0x2b73 // lParam = ICurve * , LOWORD(wParam) = point index
#define WM_CC_INSERT_CURVEPT      WM_USER+0x2b74 // lParam = ICurve * , LOWORD(wParam) = point index  (added by AF (6/26/2000) )
#define WM_CC_LBUTTONDOWN         WM_USER+0x2b75 // lParam = ICurve * , LOWORD(wParam) = point index  (added by AF (10/31/2000) )
#define WM_CC_RBUTTONDOWN         WM_USER+0x2b76 // lParam = ICurve * , LOWORD(wParam) = point index  (added by AF (10/31/2000) )
#define WM_CC_LBUTTONUP	          WM_USER+0x2b77 // lParam = ICurve * , LOWORD(wParam) = point index  (added by AF (10/31/2000) )

#define IN_CURVETANGENT_CHANGED  (1<<0)
#define OUT_CURVETANGENT_CHANGED (1<<1)

#define IN_CURVETANGENT_CHANGED  (1<<0)
#define OUT_CURVETANGENT_CHANGED (1<<1)

#define CC_DRAWBG				(1<<0)
#define CC_DRAWGRID				(1<<1)
#define CC_DRAWUTOOLBAR			(1<<2)
#define CC_SHOWRESET			(1<<3)
#define CC_DRAWLTOOLBAR			(1<<4)
#define CC_DRAWSCROLLBARS		(1<<5)
#define CC_AUTOSCROLL			(1<<6)
#define CC_DRAWRULER			(1<<7)
#define CC_ASPOPUP				(1<<8)
#define CC_CONSTRAIN_Y			(1<<9)
#define CC_HIDE_DISABLED_CURVES (1<<10)

// Rightclick menu
#define CC_RCMENU_MOVE_XY	    (1<<11)
#define CC_RCMENU_MOVE_X	    (1<<12)
#define CC_RCMENU_MOVE_Y	    (1<<13)
#define CC_RCMENU_SCALE			(1<<14)
#define CC_RCMENU_INSERT_CORNER	(1<<15)
#define CC_RCMENU_INSERT_BEZIER	(1<<16)
#define CC_RCMENU_DELETE		(1<<17)
//watje
#define CC_SHOW_CURRENTXVAL		(1<<18)
//this flag allows the user to single select a point 
//normally if a bunch of points are stacked in area if you click on the area you get all of them
//with this flag you get the first one
#define CC_SINGLESELECT			(1<<19)
//this flag turns off the curve visible/editable toggle in the top of the menu bar
//useful for when you have lots of curves and want to do the display management yourself
#define CC_NOFILTERBUTTONS		(1<<20)

#define CC_ALL_RCMENU (CC_RCMENU_MOVE_XY|CC_RCMENU_MOVE_X|CC_RCMENU_MOVE_Y|CC_RCMENU_SCALE|CC_RCMENU_INSERT_CORNER|CC_RCMENU_INSERT_BEZIER|CC_RCMENU_DELETE)

#define CC_ALL (CC_DRAWBG|CC_DRAWGRID|CC_DRAWUTOOLBAR|CC_SHOWRESET|CC_DRAWLTOOLBAR|CC_DRAWSCROLLBARS|CC_AUTOSCROLL| \
		CC_DRAWRULER|CC_ASPOPUP|CC_CONSTRAIN_Y|CC_HIDE_DISABLED_CURVES| CC_ALL_RCMENU )

#define CC_NONE 0

#define CID_CC_MOVE_XY				0
#define CID_CC_MOVE_X				1
#define CID_CC_MOVE_Y				2
#define CID_CC_SCALE				3
#define CID_CC_INSERT_CORNER		4
#define CID_CC_INSERT_BEZIER		5

//Curve out of range types
#define CURVE_EXTRAPOLATE_LINEAR	0
#define CURVE_EXTRAPOLATE_CONSTANT	1

// IPoint flags
#define CURVEP_BEZIER		(1<<0)
#define CURVEP_CORNER		(1<<1)
#define CURVEP_LOCKED_Y		(1<<2)
#define CURVEP_LOCKED_X		(1<<3)
#define CURVEP_SELECTED		(1<<4)
#define CURVEP_ENDPOINT		(1<<8)	// It's a constrained endpoint on the curve
#define CURVEP_NO_X_CONSTRAINT (1<<9)  //Added by AF (6/26/2000)

// Flags passed to SelectPts
#define SELPTS_SELECT			(1<<0)	
#define SELPTS_DESELECT			(1<<1)
#define SELPTS_CLEARPTS			(1<<2)		 

#define IS_CORNER(flags)        ( ( (flags) & CURVEP_CORNER) && !((flags) & CURVEP_BEZIER) )
#define IS_BEZIERSMOOTH(flags)  ( ( (flags) & CURVEP_BEZIER) && !((flags) & CURVEP_CORNER) )
#define IS_BEZIERCORNER(flags)  ( (flags) & (CURVEP_BEZIER | CURVEP_CORNER) )



class ICurve;

class ICurveCtl : public ReferenceTarget {
public:
	
	virtual BOOL  IsActive()=0;										// This reflects if the dialog box is up or not
	virtual void  SetActive(BOOL sw)=0;								// This brings up or closes the dialog box
	virtual HWND  GetHWND()=0;
	virtual void  SetNumCurves(int iNum, BOOL doUndo=FALSE)=0;		// Sets the number of curves for this CurveControl.
																	// Note, that this method actually create the curves, which means,
																	// that the NewCurveCreatedCallback method will be called. However, this happens only 
																	// if a ResourceMakerCallback is registered already.
																	// Thus it is important,that the ResourceMakerCallback is registered BEFORE this method
																	// is called.
																	// Optional BOOL argument will cause the function to register an Restore Object
																	// if set to TRUE.  Added by AF (10/25/00)

	virtual int   GetNumCurves()=0;									// Returns the numbers of curves
	virtual void  SetXRange(float min, float max, BOOL rescaleKeys = TRUE)=0;	// Determines the first and last key for all curves
	virtual void  SetYRange(float min, float max)=0;				// Determines the upper and lower limits, if the Flag CC_CONSTRAIN_Y is set
	virtual Point2 GetXRange()=0;									//Returns the X range as a Point2 (added by AF (6/26/2000) )
	virtual Point2 GetYRange()=0;									//Returns the Y range as a Point2 (added by AF (6/26/2000) )							
	
	virtual void  RegisterResourceMaker(ReferenceMaker *rmak)=0;	// This registers a rmaker, which has to implement GetInterface 
																	// for I_RESMAKER_INTERFACE by returning an object derived from 
																	// ResourceMakerCallback
	
	virtual BOOL  GetZoomValues(float *h, float *v)=0;				// Returns the current zoom values
	virtual void  SetZoomValues(float h, float v)=0;				// Sets the zoom values
	virtual BOOL  GetScrollValues(int *h, int *v)=0;				// Returns the current Scroll Values
	virtual void  SetScrollValues(int h, int v)=0;					// Sets the scroll values
	virtual void  ZoomExtents()=0;
	virtual void  SetTitle(TCHAR *str)=0;							// Sets the title of the dialog box
	virtual ICurve *GetControlCurve(int numCurve)=0;				// Returns and interface to the numCurve'th curve
	virtual void  SetDisplayMode(BitArray &mode)=0;					// Determines which curves are toggled on
	virtual BitArray GetDisplayMode()=0;							// Returns which curves are toggled on
	virtual void  SetCCFlags(DWORD flags)=0;
	virtual DWORD GetCCFlags()=0;
	virtual void  SetCustomParentWnd(HWND hParent)=0;				// Parent Window, if CurveControl is no popup window
	virtual void  SetMessageSink(HWND hWnd)=0;						// WM_CC_CHANGE_CURVEPT, WM_CC_CHANGE_CURVETANGENT and WM_CC_DEL_CURVEPT will be sent to this window
	virtual void  SetCommandMode(int ID)=0;
	virtual int   GetCommandMode()=0;
	virtual void  Redraw()=0;
	virtual Interval	GetValidity(TimeValue t)=0;
	virtual void Update(TimeValue t, Interval& valid)=0;
//watje
//this draws a vertical bar at the current value
	virtual void SetCurrentXValue(float val)=0;
//this turns on/off the display code.  It is useful when you are doing lots of changes and don't
//want the window to continually redraw
	virtual void EnableDraw(BOOL enable)=0;

	virtual void DeleteCurve(int index)=0;							//Added by AF (08/17/00) (used by reactor)

};

class CurvePoint
{
public:
	Point2 p;
	Point2 in;
	Point2 out;
	int flags;
	CurvePoint& operator=(const CurvePoint& rhs) // RK: 08/18/2000
	{
		p		= rhs.p;
		in		= rhs.in;
		out		= rhs.out;
		flags	= rhs.flags;
		return *this;
	}
};


class ICurve : public ReferenceTarget 
{
public:
	virtual void  SetPenProperty(COLORREF color, int width = 0, int style = PS_SOLID)=0; // Sets the pen properties of a curve 
	virtual void  GetPenProperty(COLORREF &color, int &width, int &style)=0;				 // Gets the color of a curve
	virtual void  SetDisabledPenProperty(COLORREF color, int width = 0, int style = PS_SOLID)=0; // Sets the pen properties of a curve if it is disabled
	virtual void  GetDisabledPenProperty(COLORREF &color, int &width, int &style)=0;				 // Gets the color of a curve if it is disabled
	virtual float GetValue(TimeValue t, float fX, Interval &ivalid = FOREVER, BOOL UseLookupTable = FALSE)=0;			 // Returns the Y-value for a given X-Value
	virtual void  SetCanBeAnimated(BOOL Animated)=0;
	virtual BOOL  GetCanBeAnimated()=0;

	virtual int   IsAnimated(int index)=0;
	
	virtual int   GetNumPts()=0;
	virtual void  SetNumPts(int count)=0;
	virtual	BitArray GetSelectedPts()=0;
	virtual	void  SetSelectedPts(BitArray &sel, int flags)=0;

	virtual	void  SetPoint(TimeValue t, int index, CurvePoint *point, BOOL CheckConstraints = TRUE, BOOL notify = TRUE)=0;
	virtual CurvePoint	GetPoint(TimeValue t, int index, Interval &valid = FOREVER)=0;

	//Added by AF (10/26/00)
	//Currently only supports CURVE_EXTRAPOLATE_LINEAR and CURVE_EXTRAPOLATE_CONSTANT
	virtual void  SetOutOfRangeType(int type)=0;
	virtual int   GetOutOfRangeType()=0;
	
	virtual int	  Insert(int where, CurvePoint & p)=0;
//watje
//this is identical to the Insert above but allows you to turn off/on the hold that occurs.
//this is useful when you are doing interactive inserts and moves from code, the original Insert hold
//would often get in the way
	virtual int	  Insert(int where, CurvePoint& p, BOOL do_not_hold)=0;


	virtual void  Delete(int index)=0;

	virtual void  SetLookupTableSize(int size)=0;
	virtual int   GetLookupTableSize()=0;
};

class ResourceMakerCallback
{
public:
	
	// this callback has to set the HIMAGELIST to implement custom bitmaps on the display buttons
	// The imagelist has to have NumCurves bitmaps in the format 16x15 set of images are 
	// for Out&In Enabled.
	// If the Imagelist was assigned the callback has to return TRUE. If it returns FALSE, the default bitmaps will 
	// be used. The pCCtl pointer can be used to determine which ICurveCtl calls the callback, in
	// case the plugin uses many CurveControls and want to set different bitmaps for different CurveControls

	virtual BOOL SetCustomImageList(HIMAGELIST &hCTools,ICurveCtl *pCCtl){return FALSE;} 
	
	// This callback allows the developer to assign custom ToolTips to the display buttons. He simply has to assing a 
	// TSTR to the ToolTip parameter in regards to the button number. The pCCtl pointer can be used to determine 
	// which ICurveCtl calls the callback, in case the plugin uses many CurveControls and want to set different 
	// Tooltips for different CurveControls

	virtual BOOL GetToolTip(int iButton, TSTR &ToolTip,ICurveCtl *pCCtl){return FALSE;}

	virtual void ResetCallback(int curvenum, ICurveCtl *pCCtl){}
	virtual void NewCurveCreatedCallback(int curvenum, ICurveCtl *pCCtl){}
	
	virtual void* GetInterface(ULONG id) {return NULL;}
};

#endif __ICURVECOTL__H