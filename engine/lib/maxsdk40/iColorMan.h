 /**********************************************************************
 *<
	FILE: iColorMan.h

	DESCRIPTION: Color Manager

	CREATED BY:	Scott Morrison

	HISTORY: Created 19 April, 2000,

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#pragma once

#ifdef CUSTDLG_IMP
#define CustDlgExport __declspec(dllexport)
#else
#define CustDlgExport __declspec(dllimport)
#endif

#include "iFnPub.h"

typedef DWORD ColorId;

// Standard colors the system knows about
const ColorId kBackground    = 0;  // used for all windows backgrounds
const ColorId kText          = 1;  // Used for static and button text
const ColorId kActiveCommand = 4;  // Used for active command mode buttons
const ColorId kHilight       = 5;  // COLOR_BTNHILIGHT
const ColorId kShadow        = 6;  // COLOR_BTNSHADOW
const ColorId kWindow        = 7;  // COLOR_WINDOW
const ColorId kActiveCaption = 8;  // COLOR_ACTIVECAPTION
const ColorId kToolTipBackground = 9; // COLOR_INFOBK
const ColorId kToolTipText   = 10; // COLOR_INFOTEXT
const ColorId kHilightText   = 11; // COLOR_HILIGHTTEXT
const ColorId kWindowText    = 12; // COLOR_WINDOWTEXT
const ColorId kItemHilight   = 13; // COLOR_HILIGHT
const ColorId kSubObjectColor= 14; // blue sub-object color
const ColorId k3dDarkShadow  = 15; // COLOR_3DDKSHADOW
const ColorId k3dLight       = 16; // COLOR_3DLIGHT
const ColorId kAppWorkspace  = 17; // COLOR_APPWORKSPACE
const ColorId kTrackbarBg    = 18;
const ColorId kTrackbarBgSel = 19;
const ColorId kTrackbarText  = 20;
const ColorId kTrackbarTicks = 21;
const ColorId kTrackbarKeys  = 22;
const ColorId kTrackbarSelKeys = 23;
const ColorId kTrackbarCursor  = 24;
const ColorId kPressedButton   = 25;
const ColorId kTimeSliderBg    = 26;
const ColorId kViewportBorder  = 27;
const ColorId kActiveViewportBorder = 28;
const ColorId kRollupTitleFace      = 29;
const ColorId kRollupTitleText      = 30;
const ColorId kRollupTitleHilight   = 31;
const ColorId kRollupTitleShadow    = 32;
const ColorId kSelectionRubberBand  = 33;
const ColorId kStackViewSelection = 34; // yellow sub-object color
const ColorId kImageViewerBackground = 35; // defaults to grey
const ColorId kIRenderProgressHoriz = 36; // interactive rendering - horizontal progress bar
const ColorId kIRenderProgressVert  = 37; // interactive rendering - vertical progress bar
const ColorId kPressedHierarchyButton  = 38;
const ColorId kTrackViewBackground  = 39;
const ColorId kTrackViewInactiveBackground  = 40;
const ColorId kManipulatorsActive  = 41;
const ColorId kManipulatorsSelected  = 42;
const ColorId kManipulatorsInactive  = 43;
const ColorId kFunctionCurveX = 44; 
const ColorId kFunctionCurveY = 45; 
const ColorId kFunctionCurveZ = 46; 
const ColorId kFunctionCurveW = 47; 
const ColorId kFunctionCurveFloat = 48; 
const ColorId kFunctionCurveSelected = 49; 
const ColorId kTrackViewTrackText = 50;
const ColorId kTrackViewSelInstantTime = 51;
const ColorId kTrackViewKeyCursor = 52;
const ColorId kTrackViewSelectedBackground = 53;
const ColorId kTrackViewDisabledTrack1 = 54;
const ColorId kTrackViewDisabledTrack2 = 55;
const ColorId kTrackViewEnabledTrack1 = 56;
const ColorId kTrackViewEnabledTrack2 = 57;
const ColorId kTrackViewTimeCursor = 58;
const ColorId kTrackViewGrid = 59;
const ColorId kTrackViewUnSelectedKeys = 60;
const ColorId kTrackViewSelectedKeys = 61;
const ColorId kTrackViewKeyOutline = 62;
const ColorId kTrackViewKeyOutlineOnTop = 63;
const ColorId kViewportShowDependencies = 64;
const ColorId kTrackViewSoundTrackRight = 65;
const ColorId kTrackViewSoundTrackLeft = 66;

#define COLOR_MGR_INTERFACE  Interface_ID(0x1bf46c90, 0x18bf6199)

class IColorManager: public FPStaticInterface {

public:

    // If true, we don't use custom colors
    virtual bool UseStandardWindowsColors() = 0;
    virtual void SetUseStandardWindowsColors(bool useStandardColors) = 0;

    // Register a new color with the system.
    // Return false if the color is already registered.
    virtual bool RegisterColor(ColorId id, TCHAR* pName, TCHAR* pCategory, COLORREF defaultValue) = 0;

    virtual BOOL LoadColorFile(TCHAR* pFileName) = 0;
    virtual BOOL SaveColorFile(TCHAR* pFileName) = 0;
    virtual TCHAR* GetColorFile() = 0;

    // Access to the customized colors
    virtual bool     SetColor(ColorId id, COLORREF color) = 0;
    virtual COLORREF GetColor(ColorId id) = 0;
    virtual Point3   GetColorAsPoint3(ColorId id) = 0;
    virtual HBRUSH   GetBrush(ColorId id) = 0;
    virtual TCHAR*   GetName(ColorId id) = 0;
    virtual TCHAR*   GetCategory(ColorId id) = 0;


    virtual COLORREF CustSysColor(int which) = 0;
    virtual HBRUSH   CustSysColorBrush(int which) = 0;

    // Interface for old "GetUIColor" call from gfx.
    // The colors are defined in gfx.h
    virtual Point3 GetOldUIColor(int which) = 0;
    virtual void   SetOldUIColor(int which, Point3 *clr) = 0;
    virtual Point3 GetOldDefaultUIColor(int which) = 0;

    // Icon Image processing
    // The following values are used to do image processing on the icons
    // at start-up time.
    enum IconType {
        kDisabledIcon,
        kEnabledIcon
    };

    enum IconColorScale {
        kSaturationScale,
        kValueScale,
        kAlphaScale,
    };

    virtual float GetIconColorScale(IconType type, IconColorScale which) = 0;
    virtual void  SetIconColorScale(IconType type, IconColorScale which, float value) = 0;

    virtual bool GetIconColorInvert(IconType type) = 0;
    virtual void SetIconColorInvert(IconType type, bool value) = 0;

    virtual TCHAR* GetFileName() = 0;

    virtual COLORREF GetDefaultColor(ColorId id) = 0;

    // Get an old UI color as a COLORREF
    virtual COLORREF GetOldUIColorCOLORREF(int which) = 0;

    enum RepaintType {
        kRepaintAll,
        kRepaintTrackBar,
        kRepaintTimeBar,
    };

    virtual void RepaintUI(RepaintType type) = 0;

    // Set the folder that is searched for UI icon files.
    // This must be a folder that lives under the "UI" folder.
    virtual BOOL SetIconFolder(TCHAR* pFolder) = 0;

    // returns the full path to the icon folder.
    virtual TCHAR* GetIconFolder() = 0;

	//Function-published interface
	enum {
		useStandardWindowsColors,
        setUseStandardWindowsColors,
        registerColor,
		loadColorFile,
        saveColorFile,
        getColorFile,
        setColor,
        getColor,
		getName,
        getCategory,
        getIconColorScale,
        setIconColorScale,
		getIconColorInvert,
        setIconColorInvert,
        getFileName,
        getDefaultColor,
        getOldUIColorCOLORREF,
        repaintUI,
        setIconFolder,
        getIconFolder,
	};

    // Function-published enums
    enum {
        iconType,
        iconColorScale,
        repaintType,
    };
};

// defined in CustomizationDialgos/CustColorDlg.cpp
CustDlgExport void DoColorCustDlg();
CustDlgExport void SaveColors();

#ifdef CoreExport
CoreExport void DeleteColorManager(IColorManager* pColorMan);

CoreExport IColorManager* GetColorManager();
#endif

// Easy access macros for the system's color manager
#define ColorMan() (GetColorManager())
#define GetCustSysColor(which) (ColorMan()->CustSysColor(which))
#define GetCustSysColorBrush(which) (ColorMan()->CustSysColorBrush(which))



	
