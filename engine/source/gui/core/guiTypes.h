//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITYPES_H_
#define _GUITYPES_H_

#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif


#include "gfx/gfxDevice.h"

class GBitmap;
class SFXProfile;

/// Represents a single GUI event.
///
/// This is passed around to all the relevant controls so they know what's going on.
struct GuiEvent
{
    U16      ascii;             ///< ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) - possibly a uchar or something
    U8       modifier;          ///< SI_LSHIFT, etc
    U8       keyCode;           ///< for unprintables, 'tab', 'return', ...
    Point2I  mousePoint;        ///< for mouse events
    U8       mouseClickCount;   ///< to determine double clicks, etc...
};

GFX_DeclareTextureProfile(GFXGuiCursorProfile);
GFX_DeclareTextureProfile(GFXDefaultGUIProfile);

class GuiCursor : public SimObject
{
private:
    typedef SimObject Parent;
    StringTableEntry mBitmapName;

    Point2I mHotSpot;
    Point2F mRenderOffset;
    Point2I mExtent;
    GFXTexHandle mTextureObject;

public:
    Point2I getHotSpot() { return mHotSpot; }
    Point2I getExtent() { return mExtent; }

    DECLARE_CONOBJECT(GuiCursor);
    GuiCursor(void);
    ~GuiCursor(void);
    static void initPersistFields();

    bool onAdd(void);
    void onRemove();
    void render(const Point2I& pos);
};

/// A GuiControlProfile is used by every GuiObject and is akin to a
/// datablock. It is used to control information that does not change
/// or is unlikely to change during execution of a program. It is also
/// a level of abstraction between script and GUI control so that you can
/// use the same control, say a button, and have it look completly different
/// just with a different profile.
class GuiControlProfile : public SimObject
{
private:
    typedef SimObject Parent;

public:
    S32  mRefCount;                                 ///< Used to determine if any controls are using this profile
    bool mTabable;                                  ///< True if this object is accessable from using the tab key

    static StringTableEntry  sFontCacheDirectory;
    bool mCanKeyFocus;                              ///< True if the object can be given keyboard focus (in other words, made a first responder @see GuiControl)
    bool mModal;                                    ///< True if this is a Modeless dialog meaning it will pass input through instead of taking it all

    bool mOpaque;                                   ///< True if this object is not translucent
    ColorI mFillColor;                              ///< Fill color, this is used to fill the bounds of the control if it is opaque
    ColorI mFillColorHL;                            ///< This is used insetead of mFillColor if the object is highlited
    ColorI mFillColorNA;                            ///< This is used to instead of mFillColor if the object is not active or disabled

    S32 mBorder;                                    ///< For most controls, if mBorder is > 0 a border will be drawn, some controls use this to draw different types of borders however @see guiDefaultControlRender.cc
    S32 mBorderThickness;                           ///< Border thickness
    ColorI mBorderColor;                            ///< Border color, used to draw a border around the bounds if border is enabled
    ColorI mBorderColorHL;                          ///< Used instead of mBorderColor when the object is highlited
    ColorI mBorderColorNA;                          ///< Used instead of mBorderColor when the object is not active or disabled

    ColorI mBevelColorHL;                          ///< Used for the high-light part of the bevel
    ColorI mBevelColorLL;                          ///< Used for the low-light part of the bevel

    // font members
    struct FontData
    {
        StringTableEntry mFontType;                    ///< Font face name for the control
        S32 mFontSize;                    ///< Font size for the control
        U32 mFontCharset;                              ///< Font character set

        Resource<GFont> mFont;                        ///< Font resource
    };

    FontData mFonts[2];                             ///< Array of fonts used for the control

    enum
    {
        BaseColor = 0,
        ColorHL,
        ColorNA,
        ColorSEL,
        ColorUser0,
        ColorUser1,
        ColorUser2,
        ColorUser3,
        ColorUser4,
        ColorUser5,
    };
    ColorI mFontColors[10];                        ///< Array of font colors used for drawText with escape characters for changing color mid-string
    ColorI &mFontColor;                             ///< Main font color
    ColorI &mFontColorHL;                           ///< Highlited font color
    ColorI &mFontColorNA;                           ///< Font color when object is not active/disabled
    ColorI &mFontColorSEL;                          ///< Font color when object/text is selected

    enum AlignmentType
    {
        LeftJustify,
        RightJustify,
        CenterJustify
    };

    AlignmentType mAlignment;                       ///< Horizontal text alignment
    bool mAutoSizeWidth;                            ///< Auto-size the width-bounds of the control to fit it's contents
    bool mAutoSizeHeight;                           ///< Auto-size the height-bounds of the control to fit it's contents
    bool mReturnTab;                                ///< Used in GuiTextEditCtrl to specify if a tab-event should be simulated when return is pressed.
    bool mNumbersOnly;                              ///< For text controls, true if this should only accept numerical data
    bool mMouseOverSelected;                        ///< True if this object should be "selected" while the mouse is over it
    ColorI mCursorColor;                            ///< Color for the blinking cursor in text fields (for example)

    StringTableEntry mBitmapBase;
    Point2I mTextOffset;                            ///< Text offset for the control

    // bitmap members
    StringTableEntry mBitmapName;                   ///< Bitmap file name for the bitmap of the control
    GFXTexHandle mTextureObject;
    Vector<RectI> mBitmapArrayRects;                ///< Used for controls which use an array of bitmaps such as checkboxes

    S32 mShadow;

    // game menu members
    U32 mRowHeight;
    Vector<int> mHitArea;
    Point2I mIconPosition;

    // sound members
    SFXProfile* mSoundButtonDown;
    SFXProfile* mSoundButtonOver;
    SFXProfile* mSoundOptionChanged;

public:
    DECLARE_CONOBJECT(GuiControlProfile);
    GuiControlProfile();
    ~GuiControlProfile();
    static void initPersistFields();
    bool onAdd();

    /// This method creates an array of bitmaps from one single bitmap with
    /// seperator color. The seperator color is whatever color is in pixel 0,0
    /// of the bitmap. For an example see darkWindow.png and some of the other
    /// UI textures. It returns the number of bitmaps in the array it created
    /// It also stores the sizes in the mBitmapArrayRects vector.
    S32 constructBitmapArray();

    void incRefCount();
    void decRefCount();
};
DefineConsoleType(TypeGuiProfile)
void RegisterGuiTypes(void);

#endif //_GUITYPES_H
