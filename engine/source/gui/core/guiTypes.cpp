//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "sfx/sfxSystem.h"

// We need to get some kind of abstraction here with the character
// sheet stuff -pw
#if defined(TORQUE_OS_WIN32) || defined(TORQUE_OS_WIN64)
#  include "platformWin32/platformWin32.h"
#else
#  pragma error("Woah, other OS's not supported yet!")
#endif

#include "gui/core/guiTypes.h"
#include "gfx/gNewFont.h"
#include "gfx/gBitmap.h"
#include "gfx/gfxDevice.h"


IMPLEMENT_CONOBJECT(GuiCursor);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

GFX_ImplementTextureProfile(GFXGuiCursorProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::PreserveSize |
    GFXTextureProfile::Static,
    GFXTextureProfile::None);
GFX_ImplementTextureProfile(GFXDefaultGUIProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::PreserveSize |
    GFXTextureProfile::Static |
    GFXTextureProfile::NoPadding,
    GFXTextureProfile::None);
GuiCursor::GuiCursor()
{
    mHotSpot.set(0, 0);
    mRenderOffset.set(0.0f, 0.0f);
    mExtent.set(1, 1);
    mTextureObject = NULL;
}

GuiCursor::~GuiCursor()
{
}

void GuiCursor::initPersistFields()
{
    Parent::initPersistFields();
    addField("hotSpot", TypePoint2I, Offset(mHotSpot, GuiCursor));
    addField("renderOffset", TypePoint2F, Offset(mRenderOffset, GuiCursor));
    addField("bitmapName", TypeFilename, Offset(mBitmapName, GuiCursor));
}

bool GuiCursor::onAdd()
{
    if (!Parent::onAdd())
        return false;

    Sim::getGuiDataGroup()->addObject(this);

    return true;
}

void GuiCursor::onRemove()
{
    Parent::onRemove();
}

void GuiCursor::render(const Point2I& pos)
{
    if (!mTextureObject && mBitmapName && mBitmapName[0])
    {
        mTextureObject.set(mBitmapName, &GFXGuiCursorProfile);
        if (!mTextureObject)
            return;
        mExtent.set(mTextureObject->getWidth(), mTextureObject->getHeight());
    }

    // Render the cursor centered according to dimensions of texture
    S32 texWidth = mTextureObject.getWidth();
    S32 texHeight = mTextureObject.getHeight();

    Point2I renderPos = pos;
    renderPos.x -= (texWidth * mRenderOffset.x);
    renderPos.y -= (texHeight * mRenderOffset.y);

    GFX->clearBitmapModulation();
    GFX->drawBitmap(mTextureObject, renderPos);
}


IMPLEMENT_CONOBJECT(GuiControlProfile);

static EnumTable::Enums alignEnums[] =
{
   { GuiControlProfile::LeftJustify,          "left"      },
   { GuiControlProfile::CenterJustify,        "center"    },
   { GuiControlProfile::RightJustify,         "right"     }
};
static EnumTable gAlignTable(3, &alignEnums[0]);

static EnumTable::Enums charsetEnums[] =
{
    { TGE_ANSI_CHARSET,         "ANSI" },
    { TGE_SYMBOL_CHARSET,       "SYMBOL" },
    { TGE_SHIFTJIS_CHARSET,     "SHIFTJIS" },
    { TGE_HANGEUL_CHARSET,      "HANGEUL" },
    { TGE_HANGUL_CHARSET,       "HANGUL" },
    { TGE_GB2312_CHARSET,       "GB2312" },
    { TGE_CHINESEBIG5_CHARSET,  "CHINESEBIG5" },
    { TGE_OEM_CHARSET,          "OEM" },
    { TGE_JOHAB_CHARSET,        "JOHAB" },
    { TGE_HEBREW_CHARSET,       "HEBREW" },
    { TGE_ARABIC_CHARSET,       "ARABIC" },
    { TGE_GREEK_CHARSET,        "GREEK" },
    { TGE_TURKISH_CHARSET,      "TURKISH" },
    { TGE_VIETNAMESE_CHARSET,   "VIETNAMESE" },
    { TGE_THAI_CHARSET,         "THAI" },
    { TGE_EASTEUROPE_CHARSET,   "EASTEUROPE" },
    { TGE_RUSSIAN_CHARSET,      "RUSSIAN" },
    { TGE_MAC_CHARSET,          "MAC" },
    { TGE_BALTIC_CHARSET,       "BALTIC" },
};

#define NUMCHARSETENUMS     (sizeof(charsetEnums) / sizeof(EnumTable::Enums))

static EnumTable gCharsetTable(NUMCHARSETENUMS, &charsetEnums[0]);

StringTableEntry GuiControlProfile::sFontCacheDirectory = "";

GuiControlProfile::GuiControlProfile(void) :
    mFontColor(mFontColors[BaseColor]),
    mFontColorHL(mFontColors[ColorHL]),
    mFontColorNA(mFontColors[ColorNA]),
    mFontColorSEL(mFontColors[ColorSEL])
{
    mRefCount = 0;
    mBitmapArrayRects.clear();
    mBorderThickness = 1;
    mMouseOverSelected = false;
    mBitmapName = NULL;
    mFonts[0].mFontType = "";
    mFonts[1].mFontType = "";
    mFonts[0].mFontSize = 0;
    mFonts[1].mFontSize = 0;
    mFonts[0].mFontCharset = TGE_ANSI_CHARSET;
    mFonts[1].mFontCharset = TGE_ANSI_CHARSET;

    mSoundButtonDown = NULL;
    mSoundButtonOver = NULL;
    mSoundOptionChanged = NULL;
    mRowHeight = 0;

    mShadow = 0;

    GuiControlProfile* def = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiDefaultProfile"));
    if (def)
    {
        mTabable = def->mTabable;
        mCanKeyFocus = def->mCanKeyFocus;

        mOpaque = def->mOpaque;
        mFillColor = def->mFillColor;
        mFillColorHL = def->mFillColorHL;
        mFillColorNA = def->mFillColorNA;

        mBorder = def->mBorder;
        mBorderThickness = def->mBorderThickness;
        mBorderColor = def->mBorderColor;
        mBorderColorHL = def->mBorderColorHL;
        mBorderColorNA = def->mBorderColorNA;

        mBevelColorHL = def->mBevelColorHL;
        mBevelColorLL = def->mBevelColorLL;

        // default font
        mFonts[0].mFontType = def->mFonts[0].mFontType;
        mFonts[0].mFontSize = def->mFonts[0].mFontSize;
        mFonts[0].mFontCharset = def->mFonts[0].mFontCharset;
        mFonts[1].mFontType = def->mFonts[1].mFontType;
        mFonts[1].mFontSize = def->mFonts[1].mFontSize;
        mFonts[1].mFontCharset = def->mFonts[1].mFontCharset;

        for (U32 i = 0; i < 10; i++)
            mFontColors[i] = def->mFontColors[i];

        // default bitmap
        mBitmapName = def->mBitmapName;
        mTextOffset = def->mTextOffset;

        // default sound
        mSoundButtonDown = def->mSoundButtonDown;
        mSoundButtonOver = def->mSoundButtonOver;
        mSoundOptionChanged = def->mSoundOptionChanged;

        //used by GuiTextCtrl
        mModal = def->mModal;
        mAlignment = def->mAlignment;
        mAutoSizeWidth = def->mAutoSizeWidth;
        mAutoSizeHeight = def->mAutoSizeHeight;
        mReturnTab = def->mReturnTab;
        mNumbersOnly = def->mNumbersOnly;
        mCursorColor = def->mCursorColor;

        mShadow = def->mShadow;

        mRowHeight = def->mRowHeight;
        mHitArea = def->mHitArea;
        mIconPosition = def->mIconPosition;
    }
}

GuiControlProfile::~GuiControlProfile()
{
}


void GuiControlProfile::initPersistFields()
{
    Parent::initPersistFields();

    addField("tab", TypeBool, Offset(mTabable, GuiControlProfile));
    addField("canKeyFocus", TypeBool, Offset(mCanKeyFocus, GuiControlProfile));
    addField("mouseOverSelected", TypeBool, Offset(mMouseOverSelected, GuiControlProfile));

    addField("modal", TypeBool, Offset(mModal, GuiControlProfile));
    addField("opaque", TypeBool, Offset(mOpaque, GuiControlProfile));
    addField("fillColor", TypeColorI, Offset(mFillColor, GuiControlProfile));
    addField("fillColorHL", TypeColorI, Offset(mFillColorHL, GuiControlProfile));
    addField("fillColorNA", TypeColorI, Offset(mFillColorNA, GuiControlProfile));
    addField("border", TypeS32, Offset(mBorder, GuiControlProfile));
    addField("borderThickness", TypeS32, Offset(mBorderThickness, GuiControlProfile));
    addField("borderColor", TypeColorI, Offset(mBorderColor, GuiControlProfile));
    addField("borderColorHL", TypeColorI, Offset(mBorderColorHL, GuiControlProfile));
    addField("borderColorNA", TypeColorI, Offset(mBorderColorNA, GuiControlProfile));

    addField("bevelColorHL", TypeColorI, Offset(mBevelColorHL, GuiControlProfile));
    addField("bevelColorLL", TypeColorI, Offset(mBevelColorLL, GuiControlProfile));

    {
        addField("fontType", TypeString, Offset(mFonts[0].mFontType, GuiControlProfile));
        addField("fontSize", TypeS32, Offset(mFonts[0].mFontSize, GuiControlProfile));
        addField("fontCharset", TypeEnum, Offset(mFonts[0].mFontCharset, GuiControlProfile), 1, &gCharsetTable);

        addField("fontType2", TypeString, Offset(mFonts[1].mFontType, GuiControlProfile));
        addField("fontSize2", TypeS32, Offset(mFonts[1].mFontSize, GuiControlProfile));
        addField("fontCharset2", TypeEnum, Offset(mFonts[1].mFontCharset, GuiControlProfile), 1, &gCharsetTable);
    }

    addField("fontColors", TypeColorI, Offset(mFontColors, GuiControlProfile), 10);
    addField("fontColor", TypeColorI, Offset(mFontColors[BaseColor], GuiControlProfile));
    addField("fontColorHL", TypeColorI, Offset(mFontColors[ColorHL], GuiControlProfile));
    addField("fontColorNA", TypeColorI, Offset(mFontColors[ColorNA], GuiControlProfile));
    addField("fontColorSEL", TypeColorI, Offset(mFontColors[ColorSEL], GuiControlProfile));
    addField("fontColorLink", TypeColorI, Offset(mFontColors[ColorUser0], GuiControlProfile));
    addField("fontColorLinkHL", TypeColorI, Offset(mFontColors[ColorUser1], GuiControlProfile));

    addField("justify", TypeEnum, Offset(mAlignment, GuiControlProfile), 1, &gAlignTable);
    addField("textOffset", TypePoint2I, Offset(mTextOffset, GuiControlProfile));
    addField("hitArea", TypeS32Vector, Offset(mHitArea, GuiControlProfile));
    addField("autoSizeWidth", TypeBool, Offset(mAutoSizeWidth, GuiControlProfile));
    addField("autoSizeHeight", TypeBool, Offset(mAutoSizeHeight, GuiControlProfile));
    addField("returnTab", TypeBool, Offset(mReturnTab, GuiControlProfile));
    addField("numbersOnly", TypeBool, Offset(mNumbersOnly, GuiControlProfile));
    addField("cursorColor", TypeColorI, Offset(mCursorColor, GuiControlProfile));

    addField("bitmap", TypeFilename, Offset(mBitmapName, GuiControlProfile));

    addField("shadow", TypeS32, Offset(mShadow, GuiControlProfile));

    addField("soundButtonDown", TypeSFXProfilePtr, Offset(mSoundButtonDown, GuiControlProfile));
    addField("soundButtonOver", TypeSFXProfilePtr, Offset(mSoundButtonOver, GuiControlProfile));
    addField("soundOptionChanged", TypeSFXProfilePtr, Offset(mSoundOptionChanged, GuiControlProfile));

    addField("rowHeight", TypeS32, Offset(mRowHeight, GuiControlProfile));
    addField("iconPosition", TypePoint2I, Offset(mIconPosition, GuiControlProfile));
}

bool GuiControlProfile::onAdd()
{
    if (!Parent::onAdd())
        return false;

    Sim::getGuiDataGroup()->addObject(this);

    return true;
}

S32 GuiControlProfile::constructBitmapArray()
{
    if (mBitmapArrayRects.size())
        return mBitmapArrayRects.size();

    if (mTextureObject.isNull())
        return 0;

    GBitmap* bmp = mTextureObject->getBitmap();

    //get the separator color
    ColorI sepColor;
    if (!bmp || !bmp->getColor(0, 0, sepColor))
    {
        Con::errorf("Failed to create bitmap array from %s for profile %s - couldn't ascertain seperator color!", mBitmapName, getName());
        AssertFatal(false, avar("Failed to create bitmap array from %s for profile %s - couldn't ascertain seperator color!", mBitmapName, getName()));
        return 0;
    }

    //now loop through all the scroll pieces, and find the bounding rectangle for each piece in each state
    S32 curY = 0;

    // ascertain the height of this row...
    ColorI color;
    mBitmapArrayRects.clear();
    while (curY < bmp->getHeight())
    {
        // skip any sep colors
        bmp->getColor(0, curY, color);
        if (color == sepColor)
        {
            curY++;
            continue;
        }
        // ok, process left to right, grabbing bitmaps as we go...
        S32 curX = 0;
        while (curX < bmp->getWidth())
        {
            bmp->getColor(curX, curY, color);
            if (color == sepColor)
            {
                curX++;
                continue;
            }
            S32 startX = curX;
            while (curX < bmp->getWidth())
            {
                bmp->getColor(curX, curY, color);
                if (color == sepColor)
                    break;
                curX++;
            }
            S32 stepY = curY;
            while (stepY < bmp->getHeight())
            {
                bmp->getColor(startX, stepY, color);
                if (color == sepColor)
                    break;
                stepY++;
            }
            mBitmapArrayRects.push_back(RectI(startX, curY, curX - startX, stepY - curY));
        }
        // ok, now skip to the next seperation color on column 0
        while (curY < bmp->getHeight())
        {
            bmp->getColor(0, curY, color);
            if (color == sepColor)
                break;
            curY++;
        }
    }
    return mBitmapArrayRects.size();
}

void GuiControlProfile::incRefCount()
{
    if (!mRefCount++)
    {
        sFontCacheDirectory = Con::getVariable("$GUI::fontCacheDirectory");

        //verify the font
        mFonts[0].mFont = GFont::create(mFonts[0].mFontType, mFonts[0].mFontSize, sFontCacheDirectory, mFonts[0].mFontCharset);
        if (mFonts[0].mFont.isNull())
            Con::errorf("Failed to load/create profile font (%s/%d)", mFonts[0].mFontType, mFonts[0].mFontSize);

        mFonts[1].mFont = GFont::create(mFonts[1].mFontType, mFonts[1].mFontSize, sFontCacheDirectory, mFonts[1].mFontCharset);
        if (mFonts[1].mFont.isNull())
            Con::errorf("Failed to load/create profile font2 (%s/%d)", mFonts[0].mFontType, mFonts[0].mFontSize);

        //verify the bitmap
        if (!mTextureObject.set(mBitmapName, &GFXDefaultPersistentProfile))
            Con::errorf("Failed to load profile bitmap (%s)", mBitmapName);
    }
}

void GuiControlProfile::decRefCount()
{
    AssertFatal(mRefCount, "GuiControlProfile::decRefCount: zero ref count");
    if (!mRefCount)
        return;

    if (!--mRefCount)
    {
        mFonts[0].mFont = NULL;
        mFonts[1].mFont = NULL;
        mTextureObject = NULL;
    }
}

ConsoleType(GuiProfile, TypeGuiProfile, sizeof(GuiControlProfile*))

ConsoleSetType(TypeGuiProfile)
{
    GuiControlProfile* profile = NULL;
    if (argc == 1)
        Sim::findObject(argv[0], profile);

    AssertWarn(profile != NULL, avar("GuiControlProfile: requested gui profile (%s) does not exist.", argv[0]));
    if (!profile)
        profile = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiDefaultProfile"));

    AssertFatal(profile != NULL, avar("GuiControlProfile: unable to find specified profile (%s) and GuiDefaultProfile does not exist!", argv[0]));

    GuiControlProfile** obj = (GuiControlProfile**)dptr;
    if ((*obj) == profile)
        return;

    if (*obj)
        (*obj)->decRefCount();

    *obj = profile;
    (*obj)->incRefCount();
}

ConsoleGetType(TypeGuiProfile)
{
    static char returnBuffer[256];

    GuiControlProfile** obj = (GuiControlProfile**)dptr;
    dSprintf(returnBuffer, sizeof(returnBuffer), "%s", *obj ? (*obj)->getName() : "");
    return returnBuffer;
}
