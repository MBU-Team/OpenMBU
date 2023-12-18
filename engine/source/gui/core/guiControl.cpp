//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "platform/event.h"
#include "gfx/gBitmap.h"
#include "sim/actionMap.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiControl.h"
#include "gui/core/guiDefaultControlRender.h"

Point2I FontShadowLowerRightOffset(2, 2);
Point2I FontShadowUpperLeftOffset(-2, -2);
ColorI FontShadowColor(0, 0, 0);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
IMPLEMENT_CONOBJECT(GuiControl);

bool GuiControl::smLegacyUI = false;

//used to locate the next/prev responder when tab is pressed
GuiControl* GuiControl::smPrevResponder = NULL;
GuiControl* GuiControl::smCurResponder = NULL;

GuiEditCtrl* GuiControl::smEditorHandle = NULL;

bool GuiControl::smDesignTime = false;

GuiControl::GuiControl()
{
    mLayer = 0;
    mBounds.set(0, 0, 64, 64);
    mMinExtent.set(8, 2);			// MM: Reduced to 8x2 so GuiControl can be used as a seperator.

    mProfile = NULL;

    mConsoleVariable = StringTable->insert("");
    mConsoleCommand = StringTable->insert("");
    mAltConsoleCommand = StringTable->insert("");
    mAcceleratorKey = StringTable->insert("");

    mLangTableName = StringTable->insert("");
    mLangTable = NULL;
    mFirstResponder = NULL;

    mVisible = true;
    mActive = false;
    mAwake = false;

    mHorizSizing = horizResizeRight;
    mVertSizing = vertResizeBottom;

    mEventCtrl = NULL;

    mTooltipProfile = NULL;
    mTooltip = StringTable->insert("");

    mUIMode = AlwaysShow;
}

GuiControl::~GuiControl()
{
}

bool GuiControl::onAdd()
{
    if (!Parent::onAdd())
        return false;
    const char* name = getName();
    if (name && name[0] && getClassRep())
    {
        Namespace* parent = getClassRep()->getNameSpace();
        if (Con::linkNamespaces(parent->mName, name))
            mNameSpace = Con::lookupNamespace(name);
    }
    Sim::getGuiGroup()->addObject(this);
    Con::executef(this, 1, "onAdd");

    return true;
}

void GuiControl::onChildAdded(GuiControl* child)
{
    // Base class does not make use of this
}

static EnumTable::Enums horzEnums[] =
{
    { GuiControl::horizResizeRight,      "right"     },
    { GuiControl::horizResizeWidth,      "width"     },
    { GuiControl::horizResizeLeft,       "left"      },
   { GuiControl::horizResizeCenter,     "center"    },
   { GuiControl::horizResizeRelative,       "relative"  },
   { GuiControl::horizResizeParent,         "parent"    },
   { GuiControl::horizResizeRelative,       "relativeWidth"    },
   { GuiControl::horizResizeRelativeHeight, "relativeHeight"    }
};
static EnumTable gHorizSizingTable(sizeof(horzEnums) / sizeof(EnumTable::Enums), &horzEnums[0]);

static EnumTable::Enums vertEnums[] =
{
    { GuiControl::vertResizeBottom,      "bottom"     },
    { GuiControl::vertResizeHeight,      "height"     },
    { GuiControl::vertResizeTop,         "top"        },
   { GuiControl::vertResizeCenter,      "center"     },
   { GuiControl::vertResizeRelative,      "relative"   },
   { GuiControl::vertResizeParent,        "parent"     },
   { GuiControl::vertResizeRelativeWidth, "relativeWidth"    },
   { GuiControl::vertResizeRelative,      "relativeHeight"    }
};
static EnumTable gVertSizingTable(sizeof(vertEnums) / sizeof(EnumTable::Enums), &vertEnums[0]);

static EnumTable::Enums uiModes[] =
    {
        { GuiControl::UIMode::AlwaysShow,      "AlwaysShow"     },
        { GuiControl::UIMode::LegacyOnly,      "LegacyOnly"     },
        { GuiControl::UIMode::NewOnly,      "NewOnly"     }
    };
static EnumTable gUIModeTable(sizeof(uiModes) / sizeof(EnumTable::Enums), &uiModes[0]);

void GuiControl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Parent");
    addField("Profile", TypeGuiProfile, Offset(mProfile, GuiControl));
    addField("HorizSizing", TypeEnum, Offset(mHorizSizing, GuiControl), 1, &gHorizSizingTable);
    addField("VertSizing", TypeEnum, Offset(mVertSizing, GuiControl), 1, &gVertSizingTable);

    addField("Position", TypePoint2I, Offset(mBounds.point, GuiControl));
    addField("Extent", TypePoint2I, Offset(mBounds.extent, GuiControl));
    addField("MinExtent", TypePoint2I, Offset(mMinExtent, GuiControl));

    addField("Visible", TypeBool, Offset(mVisible, GuiControl));

    addField("UIMode", TypeEnum, Offset(mUIMode, GuiControl), 1, &gUIModeTable);
    addDepricatedField("Modal");
    addDepricatedField("SetFirstResponder");

    addField("Variable", TypeString, Offset(mConsoleVariable, GuiControl));
    addField("Command", TypeString, Offset(mConsoleCommand, GuiControl));
    addField("AltCommand", TypeString, Offset(mAltConsoleCommand, GuiControl));
    addField("Accelerator", TypeString, Offset(mAcceleratorKey, GuiControl));
    addField("eventControl", TypeSimObjectPtr, Offset(mEventCtrl, GuiControl));
    addField("tooltipprofile", TypeGuiProfile, Offset(mTooltipProfile, GuiControl));
    addField("tooltip", TypeString, Offset(mTooltip, GuiControl));

    endGroup("Parent");
    addGroup("I18N");
    addField("langTableMod", TypeString, Offset(mLangTableName, GuiControl));
    endGroup("I18N");
}

void GuiControl::consoleInit()
{
    Con::addVariable("$pref::UI::LegacyUI", TypeBool, &smLegacyUI);
}

bool GuiControl::isCurrentUIMode()
{
    GuiControl* parent = getParent();
    if (parent && !parent->isCurrentUIMode())
        return false;

    if ((mUIMode == LegacyOnly && !smLegacyUI) || (mUIMode == NewOnly && smLegacyUI))
        return false;

    return true;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

LangTable* GuiControl::getGUILangTable()
{
    if (mLangTable)
        return mLangTable;

    if (mLangTableName && *mLangTableName)
    {
        mLangTable = (LangTable*)getModLangTable((const UTF8*)mLangTableName);
        return mLangTable;
    }

    GuiControl* parent = getParent();
    if (parent)
        return parent->getGUILangTable();

    return NULL;
}

const UTF8* GuiControl::getGUIString(S32 id)
{
    LangTable* lt = getGUILangTable();
    if (lt)
        return lt->getString(id);

    return NULL;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //


void GuiControl::addObject(SimObject* object)
{
    GuiControl* ctrl = dynamic_cast<GuiControl*>(object);
    if (!ctrl)
    {
        AssertWarn(0, "GuiControl::addObject: attempted to add NON GuiControl to set");
        return;
    }

    if (object->getGroup() == this)
        return;

    Parent::addObject(object);

    AssertFatal(!ctrl->isAwake(), "GuiControl::addObject: object is already awake before add");
    if (mAwake)
        ctrl->awaken();

    // If we are a child, notify our parent that we've been removed
    GuiControl* parent = ctrl->getParent();
    if (parent)
        parent->onChildAdded(ctrl);


}

void GuiControl::removeObject(SimObject* object)
{
    AssertFatal(mAwake == static_cast<GuiControl*>(object)->isAwake(), "GuiControl::removeObject: child control wake state is bad");
    if (mAwake)
        static_cast<GuiControl*>(object)->sleep();
    Parent::removeObject(object);
}

GuiControl* GuiControl::getParent()
{
    SimObject* obj = getGroup();
    if (GuiControl* gui = dynamic_cast<GuiControl*>(obj))
        return gui;
    return 0;
}

GuiCanvas* GuiControl::getRoot()
{
    GuiControl* root = NULL;
    GuiControl* parent = getParent();
    while (parent)
    {
        root = parent;
        parent = parent->getParent();
    }
    if (root)
        return dynamic_cast<GuiCanvas*>(root);
    else
        return NULL;
}

void GuiControl::inspectPreApply()
{
    // The canvas never sleeps
    if (mAwake && dynamic_cast<GuiCanvas*>(this) == NULL)
    {
        onSleep(); // release all our resources.
        mAwake = true;
    }
}

void GuiControl::inspectPostApply()
{
    // Shhhhhhh, you don't want to wake the canvas!
    if (mAwake && dynamic_cast<GuiCanvas*>(this) == NULL)
    {
        mAwake = false;
        onWake();
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

Point2I GuiControl::localToGlobalCoord(const Point2I& src)
{
    Point2I ret = src;
    ret += mBounds.point;
    GuiControl* walk = getParent();
    while (walk)
    {
        ret += walk->getPosition();
        walk = walk->getParent();
    }
    return ret;
}

Point2I GuiControl::globalToLocalCoord(const Point2I& src)
{
    Point2I ret = src;
    ret -= mBounds.point;
    GuiControl* walk = getParent();
    while (walk)
    {
        ret -= walk->getPosition();
        walk = walk->getParent();
    }
    return ret;
}

//----------------------------------------------------------------

void GuiControl::resize(const Point2I& newPosition, const Point2I& newExtent)
{
    Point2I actualNewExtent = Point2I(getMax(mMinExtent.x, newExtent.x),
        getMax(mMinExtent.y, newExtent.y));

    // only do the child control resizing stuff if you really need to.
    // If we didn't size anything, return false to indicate such
    bool extentChanged = (actualNewExtent != mBounds.extent);
    bool positionChanged = (newPosition != mBounds.point);
    if (!extentChanged && !positionChanged)
        return;

    // Update Extent
    if (extentChanged)
    {
        //call set update both before and after
        setUpdate();
        iterator i;
        for (i = begin(); i != end(); i++)
        {
            GuiControl* ctrl = static_cast<GuiControl*>(*i);
            ctrl->parentResized(mBounds.extent, actualNewExtent);
        }
        mBounds.set(newPosition, actualNewExtent);

        GuiControl* parent = getParent();
        if (parent)
            parent->childResized(this);
        setUpdate();
    }

    // Update Position
    if (positionChanged)
        mBounds.point = newPosition;
}

void GuiControl::setPosition(const Point2I& newPosition)
{
    resize(newPosition, mBounds.extent);
}

void GuiControl::setExtent(const Point2I& newExtent)
{
    resize(mBounds.point, newExtent);
}

void GuiControl::setLeft(S32 newLeft)
{
    resize(Point2I(newLeft, mBounds.point.y), mBounds.extent);
}

void GuiControl::setTop(S32 newTop)
{
    resize(Point2I(mBounds.point.x, newTop), mBounds.extent);
}

void GuiControl::setWidth(S32 newWidth)
{
    resize(mBounds.point, Point2I(newWidth, mBounds.extent.y));
}

void GuiControl::setHeight(S32 newHeight)
{
    resize(mBounds.point, Point2I(mBounds.extent.x, newHeight));
}

void GuiControl::childResized(GuiControl* child)
{
    child;
    // default to do nothing...
}

void GuiControl::parentResized(const Point2I& oldParentExtent, const Point2I& newParentExtent)
{
    PROFILE_START(GuiControl_parentResized);
    Point2I newPosition = getPosition();
    Point2I newExtent = getExtent();

    S32 deltaX = newParentExtent.x - oldParentExtent.x;
    S32 deltaY = newParentExtent.y - oldParentExtent.y;

    if (mHorizSizing == horizResizeCenter)
        newPosition.x = (newParentExtent.x - mBounds.extent.x) >> 1;
    else if (mHorizSizing == horizResizeWidth)
        newExtent.x += deltaX;
    else if (mHorizSizing == horizResizeLeft)
        newPosition.x += deltaX;
    else if (mHorizSizing == horizResizeRelative && oldParentExtent.x != 0)
    {
        S32 newLeft = (newPosition.x * newParentExtent.x) / oldParentExtent.x;
        S32 newRight = ((newPosition.x + newExtent.x) * newParentExtent.x) / oldParentExtent.x;

        newPosition.x = newLeft;
        newExtent.x = newRight - newLeft;
    }
    else if (mHorizSizing == horizResizeRelativeHeight && oldParentExtent.y != 0)
    {
        // Were we closer to the left or right edge?  (Confusing since "new" position/extent
        // is actually old position/extent until we change it).
        // This would all be a little simpler if we could just compute a resize scaler, but
        // I'm assuming integer math was used before in order to avoid rounding errors so
        // I'll do the same here.
        S32 newLeft, newRight;
        if (oldParentExtent.x - newExtent.x - newPosition.x < newPosition.x)
        {
            // closer to the right edge, so anchor there (keep it's relative position)
            newRight = newParentExtent.x;
            newRight -= ((oldParentExtent.x - newExtent.x - newPosition.x) * newParentExtent.y) / oldParentExtent.y;
            // now move left edge in order to maintain size of control
            newLeft = newRight - (newExtent.x * newParentExtent.y) / oldParentExtent.y;
        }
        else
        {
            // closer to the left edge, so anchor there (this is easier)
            newLeft = (newPosition.x * newParentExtent.y) / oldParentExtent.y;
            // now move right edge in order to maintain size of control
            newRight = newLeft + (newExtent.x * newParentExtent.y) / oldParentExtent.y;
        }

        newPosition.x = newLeft;
        newExtent.x = newRight - newLeft;
    }
    else if (mHorizSizing == horizResizeParent)
    {
        newPosition.x = getParent()->getPosition().x;
        newExtent.x = newParentExtent.x;
    }

    if (mVertSizing == vertResizeCenter)
        newPosition.y = (newParentExtent.y - mBounds.extent.y) >> 1;
    else if (mVertSizing == vertResizeHeight)
        newExtent.y += deltaY;
    else if (mVertSizing == vertResizeTop)
        newPosition.y += deltaY;
    else if (mVertSizing == vertResizeRelative && oldParentExtent.y != 0)
    {

        S32 newTop = (newPosition.y * newParentExtent.y) / oldParentExtent.y;
        S32 newBottom = ((newPosition.y + newExtent.y) * newParentExtent.y) / oldParentExtent.y;

        newPosition.y = newTop;
        newExtent.y = newBottom - newTop;
    }
    else if (mVertSizing == vertResizeRelativeWidth && oldParentExtent.x != 0)
    {
        // Were we closer to the top or bottom edge?  (Confusing since "new" position/extent
        // is actually old position/extent until we change it).
        // This would all be a little simpler if we could just compute a resize scaler, but
        // I'm assuming integer math was used before in order to avoid rounding errors so
        // I'll do the same here.
        S32 newTop, newBottom;
        if (oldParentExtent.y - newExtent.y - newPosition.y < newPosition.y)
        {
            // closer to the top edge, so anchor there (keep it's relative position)
            newBottom = newParentExtent.y;
            newBottom -= ((oldParentExtent.y - newExtent.y - newPosition.y) * newParentExtent.x) / oldParentExtent.x;
            // now move top edge in order to maintain size of control
            newTop = newBottom - (newExtent.y * newParentExtent.x) / oldParentExtent.x;
        }
        else
        {
            // closer to the top edge, so anchor there (this is easier)
            newTop = (newPosition.y * newParentExtent.x) / oldParentExtent.x;
            // now move bottom edge in order to maintain size of control
            newBottom = newTop + (newExtent.y * newParentExtent.x) / oldParentExtent.x;
        }
        newPosition.y = newTop;
        newExtent.y = newBottom - newTop;
    }
    else if (mVertSizing == vertResizeParent)
    {
        newPosition.y = getParent()->getPosition().y;
        newExtent.y = newParentExtent.y;
    }
    resize(newPosition, newExtent);
    PROFILE_END();
}

//----------------------------------------------------------------

void GuiControl::onRender(Point2I offset, const RectI& updateRect)
{
    if (!isCurrentUIMode())
        return;

    RectI ctrlRect(offset, mBounds.extent);

    // MBU X360 has this extra line
    //GFX->setZEnable(false);

    //if opaque, fill the update rect with the fill color
    if (mProfile->mOpaque)
        GFX->drawRectFill(ctrlRect, mProfile->mFillColor);

    //if there's a border, draw the border
    if (mProfile->mBorder)
        renderBorder(ctrlRect, mProfile);

    renderChildControls(offset, updateRect);
}

bool GuiControl::renderTooltip(Point2I cursorPos)
{
    /*
       //*** Return immediately if we don't need to be here

       if (!mAwake) return false;
       if (dStrlen(mTooltip) == 0) return false;

       GuiCanvas *root = getRoot();
       if (!root) return false;

       if (!mTooltipProfile)
          mTooltipProfile = mProfile;

       GFont *font = mTooltipProfile->mFont;

       //Vars used:
       //Screensize (for position check)
       //Offset to get position of cursor
       //textBounds for text extent.
       Point2I screensize = Platform::getWindowSize();
       Point2I offset = cursorPos;
       Point2I textBounds;
       S32 textWidth = font->getStrWidth(mTooltip);

       //Offset below cursor image
       offset.y += root->getCursorExtent().y;
       //Create text bounds.
       textBounds.x = textWidth+8;
       textBounds.y = font->getHeight() + 4;

       // Check position/width to make sure all of the tooltip will be rendered
       // 5 is given as a buffer against the edge
       if (screensize.x < offset.x + textBounds.x + 5)
          offset.x = screensize.x - textBounds.x - 5;

       //*** And ditto for the height
       if(screensize.y < offset.y + textBounds.y + 5)
          offset.y = cursorPos.y - textBounds.y - 5;

       // Set rectangle for the box, and set the clip rectangle.
       RectI rect(offset, textBounds);
       dglSetClipRect(rect);

       // Draw Filler bit, then border on top of that
       dglDrawRectFill(rect, mTooltipProfile->mFillColor);
       renderBorder(rect, mTooltipProfile);

       //*** Draw the text centered in the tool tip box
       dglSetBitmapModulation( mTooltipProfile->mFontColor );
       Point2I start;
       start.set( ( textBounds.x - textWidth) / 2, ( textBounds.y - font->getHeight() ) / 2 );
       dglDrawText( font, start + offset, mTooltip, mProfile->mFontColors );
    */
    return true;
}

void GuiControl::renderChildControls(Point2I offset, const RectI& updateRect)
{
    // offset is the upper-left corner of this control in screen coordinates
    // updateRect is the intersection rectangle in screen coords of the control
    // hierarchy.  This can be set as the clip rectangle in most cases.
    RectI clipRect = updateRect;

    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        if (ctrl->mVisible)
        {
            Point2I childPosition = offset + ctrl->getPosition();
            RectI childClip(childPosition, ctrl->getExtent());

            if (childClip.intersect(clipRect))
            {
                GFX->setClipRect(childClip);
                GFX->setCullMode(GFXCullNone);
                ctrl->onRender(childPosition, childClip);
            }
        }
    }
}

void GuiControl::setUpdateRegion(Point2I pos, Point2I ext)
{
    Point2I upos = localToGlobalCoord(pos);
    GuiCanvas* root = getRoot();
    if (root)
    {
        root->addUpdateRegion(upos, ext);
    }
}

void GuiControl::setUpdate()
{
    setUpdateRegion(Point2I(0, 0), mBounds.extent);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiControl::awaken()
{
    AssertFatal(!mAwake, "GuiControl::awaken: control is already awake");
    if (mAwake)
        return;

    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);

        AssertFatal(!ctrl->isAwake(), "GuiControl::awaken: child control is already awake");
        if (!ctrl->isAwake())
            ctrl->awaken();
    }

    AssertFatal(!mAwake, "GuiControl::awaken: should not be awake here");
    if (!mAwake)
    {
        if (!onWake())
        {
            Con::errorf(ConsoleLogEntry::General, "GuiControl::awaken: failed onWake for obj: %s", getName());
            AssertFatal(0, "GuiControl::awaken: failed onWake");
            deleteObject();
        }
    }
}

void GuiControl::sleep()
{
    AssertFatal(mAwake, "GuiControl::sleep: control is not awake");
    if (!mAwake)
        return;

    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);

        AssertFatal(ctrl->isAwake(), "GuiControl::sleep: child control is already asleep");
        if (ctrl->isAwake())
            ctrl->sleep();
    }

    AssertFatal(mAwake, "GuiControl::sleep: should not be asleep here");
    if (mAwake)
        onSleep();
}

void GuiControl::preRender()
{
    AssertFatal(mAwake, "GuiControl::preRender: control is not awake");
    if (!mAwake)
        return;

    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        ctrl->preRender();
    }
    onPreRender();
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

bool GuiControl::onWake()
{
    AssertFatal(!mAwake, "GuiControl::onWake: control is already awake");
    if (mAwake)
        return false;

    // [tom, 4/18/2005] Cause mLangTable to be refreshed in case it was changed
    mLangTable = NULL;

    //make sure we have a profile
    if (!mProfile)
    {
        // First lets try to get a profile based on the classname of this object
        const char* cName = getClassName();

        // Ensure this is a valid name...
        if (cName && cName[0])
        {
            S32 pos = 0;

            for (pos = 0; pos <= dStrlen(cName); pos++)
                if (!dStrncmp(cName + pos, "Ctrl", 4))
                    break;

            if (pos != 0) {
                char buff[255];
                dStrncpy(buff, cName, pos);
                buff[pos] = '\0';
                dStrcat(buff, "Profile\0");

                SimObject* obj = Sim::findObject(buff);

                if (obj)
                    mProfile = dynamic_cast<GuiControlProfile*>(obj);
            }
        }

        // Ok lets check to see if that worked
        if (!mProfile) {
            SimObject* obj = Sim::findObject("GuiDefaultProfile");

            if (obj)
                mProfile = dynamic_cast<GuiControlProfile*>(obj);
        }

        AssertFatal(mProfile, avar("GuiControl: %s created with no profile.", getName()));
    }

    //set the flag
    mAwake = true;

    //set the layer
    GuiCanvas* root = getRoot();
    AssertFatal(root, "Unable to get the root Canvas.");
    GuiControl* parent = getParent();
    if (parent && parent != root)
        mLayer = parent->mLayer;

    //make sure the first responder exists
    if (!mFirstResponder)
        mFirstResponder = findFirstTabable();

    //see if we should force this control to be the first responder
    if (mProfile->mTabable && mProfile->mCanKeyFocus)
        setFirstResponder();

    //increment the profile
    mProfile->incRefCount();

    // Only invoke script callbacks if we have a namespace in which to do so
    // This will suppress warnings
    if (getNamespace())
        Con::executef(this, 1, "onWake");

    return true;
}

void GuiControl::onSleep()
{
    AssertFatal(mAwake, "GuiControl::onSleep: control is not awake");
    if (!mAwake)
        return;

    //decrement the profile referrence
    if (mProfile != NULL)
        mProfile->decRefCount();
    clearFirstResponder();
    mouseUnlock();

    // Only invoke script callbacks if we have a namespace in which to do so
    // This will suppress warnings
    if (getNamespace())
        Con::executef(this, 1, "onSleep");

    // Set Flag
    mAwake = false;
}

void GuiControl::setControlProfile(GuiControlProfile* prof)
{
    AssertFatal(prof, "GuiControl::setControlProfile: invalid profile");
    if (prof == mProfile)
        return;
    if (mAwake)
        mProfile->decRefCount();
    mProfile = prof;
    if (mAwake)
        mProfile->incRefCount();

}

void GuiControl::onPreRender()
{
    // do nothing.
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

ConsoleMethod(GuiControl, setValue, void, 3, 3, "(string value)")
{
    object->setScriptValue(argv[2]);
}

ConsoleMethod(GuiControl, getValue, const char*, 2, 2, "")
{
    return object->getScriptValue();
}

ConsoleMethod(GuiControl, setActive, void, 3, 3, "(bool active)")
{
    object->setActive(dAtob(argv[2]));
}

ConsoleMethod(GuiControl, isActive, bool, 2, 2, "")
{
    return object->isActive();
}

ConsoleMethod(GuiControl, setVisible, void, 3, 3, "(bool visible)")
{
    object->setVisible(dAtob(argv[2]));
}

ConsoleMethod(GuiControl, makeFirstResponder, void, 3, 3, "(bool isFirst)")
{
    object->makeFirstResponder(dAtob(argv[2]));
}

ConsoleMethod(GuiControl, isVisible, bool, 2, 2, "")
{
    return object->isVisible();
}

ConsoleMethod(GuiControl, isAwake, bool, 2, 2, "")
{
    return object->isAwake();
}

ConsoleMethod(GuiControl, setProfile, void, 3, 3, "(GuiControlProfile p)")
{
    GuiControlProfile* profile;

    if (Sim::findObject(argv[2], profile))
        object->setControlProfile(profile);
}

ConsoleMethod(GuiControl, resize, void, 6, 6, "(int x, int y, int w, int h)")
{
    Point2I newPos(dAtoi(argv[2]), dAtoi(argv[3]));
    Point2I newExt(dAtoi(argv[4]), dAtoi(argv[5]));
    object->resize(newPos, newExt);
}

ConsoleMethod(GuiControl, getPosition, const char*, 2, 2, "")
{
    char* retBuffer = Con::getReturnBuffer(64);
    const Point2I& pos = object->getPosition();
    dSprintf(retBuffer, 64, "%d %d", pos.x, pos.y);
    return retBuffer;
}

ConsoleMethod(GuiControl, getExtent, const char*, 2, 2, "Get the width and height of the control.")
{
    char* retBuffer = Con::getReturnBuffer(64);
    const Point2I& ext = object->getExtent();
    dSprintf(retBuffer, 64, "%d %d", ext.x, ext.y);
    return retBuffer;
}

ConsoleMethod(GuiControl, getMinExtent, const char*, 2, 2, "Get the minimum allowed size of the control.")
{
    char* retBuffer = Con::getReturnBuffer(64);
    const Point2I& minExt = object->getMinExtent();
    dSprintf(retBuffer, 64, "%d %d", minExt.x, minExt.y);
    return retBuffer;
}

void GuiControl::onRemove()
{
    // Only invoke script callbacks if they can be received
    if (getNamespace())
        Con::executef(this, 1, "onRemove");

    clearFirstResponder();
    Parent::onRemove();

    // If we are a child, notify our parent that we've been removed
    GuiControl* parent = getParent();
    if (parent)
        parent->onChildRemoved(this);
}

void GuiControl::onChildRemoved(GuiControl* child)
{
    // Base does nothing with this
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

const char* GuiControl::getScriptValue()
{
    return NULL;
}

void GuiControl::setScriptValue(const char* value)
{
    value;
}

void GuiControl::setConsoleVariable(const char* variable)
{
    if (variable)
    {
        mConsoleVariable = StringTable->insert(variable);
    }
    else
    {
        mConsoleVariable = StringTable->insert("");
    }
}

void GuiControl::setConsoleCommand(const char* newCmd)
{
    if (newCmd)
        mConsoleCommand = StringTable->insert(newCmd);
    else
        mConsoleCommand = StringTable->insert("");
}

const char* GuiControl::getConsoleCommand()
{
    return mConsoleCommand;
}

void GuiControl::setSizing(S32 horz, S32 vert)
{
    mHorizSizing = horz;
    mVertSizing = vert;
}


void GuiControl::setVariable(const char* value)
{
    if (mConsoleVariable[0])
        Con::setVariable(mConsoleVariable, value);
}

void GuiControl::setIntVariable(S32 value)
{
    if (mConsoleVariable[0])
        Con::setIntVariable(mConsoleVariable, value);
}

void GuiControl::setFloatVariable(F32 value)
{
    if (mConsoleVariable[0])
        Con::setFloatVariable(mConsoleVariable, value);
}

const char* GuiControl::getVariable()
{
    if (mConsoleVariable[0])
        return Con::getVariable(mConsoleVariable);
    else return NULL;
}

S32 GuiControl::getIntVariable()
{
    if (mConsoleVariable[0])
        return Con::getIntVariable(mConsoleVariable);
    else return 0;
}

F32 GuiControl::getFloatVariable()
{
    if (mConsoleVariable[0])
        return Con::getFloatVariable(mConsoleVariable);
    else return 0.0f;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

bool GuiControl::cursorInControl()
{
    GuiCanvas* root = getRoot();
    if (!root) return false;

    Point2I pt = root->getCursorPos();
    Point2I offset = localToGlobalCoord(Point2I(0, 0));
    if (pt.x >= offset.x && pt.y >= offset.y &&
        pt.x < offset.x + mBounds.extent.x && pt.y < offset.y + mBounds.extent.y)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GuiControl::pointInControl(const Point2I& parentCoordPoint)
{
    S32 xt = parentCoordPoint.x - mBounds.point.x;
    S32 yt = parentCoordPoint.y - mBounds.point.y;
    return xt >= 0 && yt >= 0 && xt < mBounds.extent.x&& yt < mBounds.extent.y;
}

GuiControl* GuiControl::findHitControl(const Point2I& pt, S32 initialLayer)
{
    iterator i = end(); // find in z order (last to first)
    while (i != begin())
    {
        i--;
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        if (initialLayer >= 0 && ctrl->mLayer > initialLayer)
        {
            continue;
        }
        else if (ctrl->mVisible && ctrl->isCurrentUIMode() && ctrl->pointInControl(pt))
        {
            Point2I ptemp = pt - ctrl->mBounds.point;
            GuiControl* hitCtrl = ctrl->findHitControl(ptemp);

            if (hitCtrl->mProfile->mModal)
                return hitCtrl;
        }
    }
    return this;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

bool GuiControl::isMouseLocked()
{
    GuiCanvas* root = getRoot();
    return root ? root->getMouseLockedControl() == this : false;
}

void GuiControl::mouseLock(GuiControl* lockingControl)
{
    GuiCanvas* root = getRoot();
    if (root)
        root->mouseLock(lockingControl);
}

void GuiControl::mouseLock()
{
    GuiCanvas* root = getRoot();
    if (root)
        root->mouseLock(this);
}

void GuiControl::mouseUnlock()
{
    GuiCanvas* root = getRoot();
    if (root)
        root->mouseUnlock(this);
}

bool GuiControl::onInputEvent(const InputEvent& event)
{
    // Do nothing by default...
    return(false);
}

void GuiControl::onMouseUp(const GuiEvent& event)
{
}

void GuiControl::onMouseDown(const GuiEvent& event)
{
}

void GuiControl::onMouseMove(const GuiEvent& event)
{
    //if this control is a dead end, make sure the event stops here
    if (!mVisible || !mAwake)
        return;

    //pass the event to the parent
    GuiControl* parent = getParent();
    if (parent)
        parent->onMouseMove(event);
}

void GuiControl::onMouseDragged(const GuiEvent& event)
{
}

void GuiControl::onMouseEnter(const GuiEvent&)
{
}

void GuiControl::onMouseLeave(const GuiEvent&)
{
}

bool GuiControl::onMouseWheelUp(const GuiEvent& event)
{
    //if this control is a dead end, make sure the event stops here
    if (!mVisible || !mAwake)
        return true;

    //pass the event to the parent
    GuiControl* parent = getParent();
    if (parent)
        return parent->onMouseWheelUp(event);
    else
        return false;
}

bool GuiControl::onMouseWheelDown(const GuiEvent& event)
{
    //if this control is a dead end, make sure the event stops here
    if (!mVisible || !mAwake)
        return true;

    //pass the event to the parent
    GuiControl* parent = getParent();
    if (parent)
        return parent->onMouseWheelDown(event);
    else
        return false;
}

void GuiControl::onRightMouseDown(const GuiEvent&)
{
}

void GuiControl::onRightMouseUp(const GuiEvent&)
{
}

void GuiControl::onRightMouseDragged(const GuiEvent&)
{
}

void GuiControl::onMiddleMouseDown(const GuiEvent&)
{
}

void GuiControl::onMiddleMouseUp(const GuiEvent&)
{
}

void GuiControl::onMiddleMouseDragged(const GuiEvent&)
{
}
// JDD : Editor Mouse events
void GuiControl::onMouseDownEditor(const GuiEvent& event, Point2I offset)
{
}
void GuiControl::onRightMouseDownEditor(const GuiEvent& event, Point2I offset)
{
}
GuiControl* GuiControl::findFirstTabable()
{
    GuiControl* tabCtrl = NULL;
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        tabCtrl = ctrl->findFirstTabable();
        if (tabCtrl)
        {
            mFirstResponder = tabCtrl;
            return tabCtrl;
        }
    }

    //nothing was found, therefore, see if this ctrl is tabable
    return (mProfile != NULL) ? ((mProfile->mTabable && mAwake && mVisible) ? this : NULL) : NULL;
}

GuiControl* GuiControl::findLastTabable(bool firstCall)
{
    //if this is the first call, clear the global
    if (firstCall)
        smPrevResponder = NULL;

    //if this control is tabable, set the global
    if (mProfile->mTabable)
        smPrevResponder = this;

    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        ctrl->findLastTabable(false);
    }

    //after the entire tree has been traversed, return the last responder found
    mFirstResponder = smPrevResponder;
    return smPrevResponder;
}

GuiControl* GuiControl::findNextTabable(GuiControl* curResponder, bool firstCall)
{
    //if this is the first call, clear the global
    if (firstCall)
        smCurResponder = NULL;

    //first find the current responder
    if (curResponder == this)
        smCurResponder = this;

    //if the first responder has been found, return the very next *tabable* control
    else if (smCurResponder && mProfile->mTabable && mAwake && mVisible && mActive)
        return(this);

    //loop through, checking each child to see if it is the one that follows the firstResponder
    GuiControl* tabCtrl = NULL;
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        tabCtrl = ctrl->findNextTabable(curResponder, false);
        if (tabCtrl) break;
    }
    mFirstResponder = tabCtrl;
    return tabCtrl;
}

GuiControl* GuiControl::findPrevTabable(GuiControl* curResponder, bool firstCall)
{
    if (firstCall)
        smPrevResponder = NULL;

    //if this is the current reponder, return the previous one
    if (curResponder == this)
        return smPrevResponder;

    //else if this is a responder, store it in case the next found is the current responder
    else if (mProfile->mTabable && mAwake && mVisible && mActive)
        smPrevResponder = this;

    //loop through, checking each child to see if it is the one that follows the firstResponder
    GuiControl* tabCtrl = NULL;
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        tabCtrl = ctrl->findPrevTabable(curResponder, false);
        if (tabCtrl) break;
    }
    mFirstResponder = tabCtrl;
    return tabCtrl;
}

void GuiControl::onLoseFirstResponder()
{
    // Since many controls have visual cues when they are the firstResponder...
    setUpdate();
}

bool GuiControl::ControlIsChild(GuiControl* child)
{
    //function returns true if this control, or one of it's children is the child control
    if (child == this)
        return true;

    //loop through, checking each child to see if it is ,or contains, the firstResponder
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        if (ctrl->ControlIsChild(child)) return true;
    }

    //not found, therefore false
    return false;
}

bool GuiControl::isFirstResponder()
{
    GuiCanvas* root = getRoot();
    return root && root->getFirstResponder() == this;
}

void GuiControl::setFirstResponder(GuiControl* firstResponder)
{
    if (firstResponder && firstResponder->mProfile->mCanKeyFocus)
        mFirstResponder = firstResponder;

    GuiControl* parent = getParent();
    if (parent)
        parent->setFirstResponder(firstResponder);
}

void GuiControl::setFirstResponder()
{
    if (mAwake && mVisible)
    {
        GuiControl* parent = getParent();
        if (mProfile->mCanKeyFocus && parent)
            parent->setFirstResponder(this);
        // Since many controls have visual cues when they are the firstResponder...
        setUpdate();
    }
}

void GuiControl::clearFirstResponder()
{
    GuiControl* parent = this;
    while ((parent = parent->getParent()) != NULL)
    {
        if (parent->mFirstResponder == this)
            parent->mFirstResponder = NULL;
        else
            break;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiControl::buildAcceleratorMap()
{
    //add my own accel key
    addAcceleratorKey();

    //add all my childrens keys
    iterator i;
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        ctrl->buildAcceleratorMap();
    }
}

void GuiControl::addAcceleratorKey()
{
    //see if we have an accelerator
    if (mAcceleratorKey == StringTable->insert(""))
        return;

    EventDescriptor accelEvent;
    ActionMap::createEventDescriptor(mAcceleratorKey, &accelEvent);

    //now we have a modifier, and a key, add them to the canvas
    GuiCanvas* root = getRoot();
    if (root)
        root->addAcceleratorKey(this, 0, accelEvent.eventCode, accelEvent.flags);
}

void GuiControl::acceleratorKeyPress(U32 index)
{
    index;
    onAction();
}

void GuiControl::acceleratorKeyRelease(U32 index)
{
    index;
    //do nothing
}

bool GuiControl::onKeyDown(const GuiEvent& event)
{
    //pass the event to the parent
    GuiControl* parent = getParent();
    if (parent)
        return parent->onKeyDown(event);
    else
        return false;
}

bool GuiControl::onKeyRepeat(const GuiEvent& event)
{
    // default to just another key down.
    return onKeyDown(event);
}

bool GuiControl::onKeyUp(const GuiEvent& event)
{
    //pass the event to the parent
    GuiControl* parent = getParent();
    if (parent)
        return parent->onKeyUp(event);
    else
        return false;
}

bool GuiControl::onGamepadButtonPressed(U32 button)
{
    if (scriptOnGamepadButtonEvent(button, true))
        return true;

    if (button == XI_START)
        return scriptOnGamepadButtonEvent(XI_A, true);

    if (button == XI_BACK)
        return scriptOnGamepadButtonEvent(XI_B, true);

    GuiControl* parent = getParent();
    if (parent)
        return parent->onGamepadButtonPressed(button);

    return false;
}

bool GuiControl::onGamepadButtonReleased(U32 button)
{
    if (scriptOnGamepadButtonEvent(button, false))
        return true;

    if (button == XI_START)
        return scriptOnGamepadButtonEvent(XI_A, false);

    if (button == XI_BACK)
        return scriptOnGamepadButtonEvent(XI_B, false);

    GuiControl* parent = getParent();
    if (parent)
        return parent->onGamepadButtonReleased(button);

    return false;
}

bool GuiControl::scriptOnGamepadButtonEvent(U32 button, bool pressed)
{
    if (!mNameSpace)
        return false;

    const char* buttonCmd;

    switch (button)
    {
        case XI_LEFT_TRIGGER:
            buttonCmd = "onLTrigger";
            break;
        case XI_RIGHT_TRIGGER:
            buttonCmd = "onRTrigger";
            break;
        case XI_DPAD_UP:
            buttonCmd = "onUp";
            break;
        case XI_DPAD_DOWN:
            buttonCmd = "onDown";
            break;
        case XI_DPAD_LEFT:
            buttonCmd = "onLeft";
            break;
        case XI_DPAD_RIGHT:
            buttonCmd = "onRight";
            break;
        case XI_START:
            buttonCmd = "onStart";
            break;
        case XI_BACK:
            buttonCmd = "onBack";
            break;
        case XI_LEFT_THUMB:
            buttonCmd = "onLThumb";
            break;
        case XI_RIGHT_THUMB:
            buttonCmd = "onRThumb";
            break;
        case XI_LEFT_SHOULDER:
            buttonCmd = "onLShoulder";
            break;
        case XI_RIGHT_SHOULDER:
            buttonCmd = "onRShoulder";
            break;
        case XI_A:
            buttonCmd = "onA";
            break;
        case XI_B:
            buttonCmd = "onB";
            break;
        case XI_X:
            buttonCmd = "onX";
            break;
        case XI_Y:
            buttonCmd = "onY";
            break;
        default:
            return false;
    }

    const char* action = "";
    if (!pressed)
        action = "Released";

    char fullname[1024];
    dSprintf(fullname, 1024, "%s%s", buttonCmd, action);
    StringTableEntry entry = StringTable->insert(fullname);
    if (!mNameSpace->lookup(entry))
        return false;

    Con::executef(this, 1, entry);

    return true;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiControl::onAction()
{
    if (!mActive)
        return;

    //execute the console command
    if (mConsoleCommand[0])
    {
        char buf[16];
        dSprintf(buf, sizeof(buf), "%d", getId());
        Con::setVariable("$ThisControl", buf);
        Con::evaluate(mConsoleCommand, false);
    }
    else
        Con::executef(this, 1, "onAction");
}

void GuiControl::onMessage(GuiControl* sender, S32 msg)
{
    sender;
    msg;
}

void GuiControl::messageSiblings(S32 message)
{
    GuiControl* parent = getParent();
    if (!parent) return;
    GuiControl::iterator i;
    for (i = parent->begin(); i != parent->end(); i++)
    {
        GuiControl* ctrl = dynamic_cast<GuiControl*>(*i);
        if (ctrl != this)
            ctrl->onMessage(this, message);
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiControl::onDialogPush()
{
}

void GuiControl::onDialogPop()
{
}

//------------------------------------------------------------------------------
void GuiControl::setVisible(bool value)
{
    mVisible = value;
    iterator i;
    setUpdate();
    for (i = begin(); i != end(); i++)
    {
        GuiControl* ctrl = static_cast<GuiControl*>(*i);
        ctrl->clearFirstResponder();
    }

    GuiControl* parent = getParent();
    if (parent)
        parent->childResized(this);
}


void GuiControl::makeFirstResponder(bool value)
{
    if (value)
        //setFirstResponder(this);
        setFirstResponder();
    else
        clearFirstResponder();
}

void GuiControl::setActive(bool value)
{
    mActive = value;

    if (!mActive)
        clearFirstResponder();

    if (mVisible && mAwake)
        setUpdate();
}

void GuiControl::getScrollLineSizes(U32* rowHeight, U32* columnWidth)
{
    // default to 10 pixels in y, 30 pixels in x
    *columnWidth = 30;
    *rowHeight = 10;
}

void GuiControl::renderJustifiedText(Point2I offset, Point2I extent, const char* text)
{
    GFont* font = mProfile->mFonts[0].mFont;
    this->renderJustifiedText(font, offset, extent, text);
}

void GuiControl::renderJustifiedText(GFont* font, Point2I offset, Point2I extent, const char* text)
{
    S32 textWidth = font->getStrWidth((const UTF8*)text);
    Point2I start;

    // align the horizontal
    switch (mProfile->mAlignment)
    {
        case GuiControlProfile::RightJustify:
            start.set(extent.x - textWidth, 0);
            break;
        case GuiControlProfile::CenterJustify:
            start.set((extent.x - textWidth) / 2, 0);
            break;
        default:
            // GuiControlProfile::LeftJustify
            start.set(0, 0);
            break;
    }

    // If the text is longer then the box size, (it'll get clipped) so
    // force Left Justify

    if (textWidth > extent.x)
        start.set(0, 0);

    // center the vertical
    /*if (font->getHeight() > extent.y)
        start.y = 0 - ((font->getHeight() - extent.y) / 2);
    else
        start.y = (extent.y - font->getHeight()) / 2;*/

    // On MBU on xbox this happens instead of the above
    start.y = (extent.y - font->getHeight() + 2) / 2;

    if (mProfile->mShadow)
    {
        if (mProfile->mShadow == 2)
        {
            start += FontShadowLowerRightOffset;
        }

        ColorI color;
        GFX->getBitmapModulation(&color);

        renderShadowText(mProfile->mShadow, font, color.alpha / 255.0f, start + offset, text, dStrlen(text));
    }

    GFX->drawText(font, start + offset, text, mProfile->mFontColors);
}

void GuiControl::renderShadowText(U32 shadowType, const GFont* font, F32 alpha, const Point2I& drawPoint, const char* text, U32 textLen)
{
    U32 len = 2 * textLen + 2;
    FrameTemp<UTF16> ubuf(len);

    U32 newLen = convertUTF8toUTF16(text, ubuf, len);

    renderShadowText(shadowType, font, alpha, drawPoint, ubuf, newLen);
}

void GuiControl::renderShadowText(U32 shadowType, const GFont* font, F32 alpha, const Point2I& drawPoint, const UTF16* text, U32 textLen)
{
    if (!shadowType || !font)
        return;

    ColorI shadowColor = FontShadowColor;
    shadowColor.alpha = alpha * 255.0f;

    ColorI savedColor;
    GFX->getBitmapModulation(&savedColor);

    GFX->setBitmapModulation(shadowColor);

    if (shadowType == 1 || shadowType == 2)
    {
        GFX->drawTextN((GFont*)font, FontShadowLowerRightOffset + drawPoint, text, textLen);

        if (shadowType == 2)
            GFX->drawTextN((GFont*)font, FontShadowUpperLeftOffset + drawPoint, text, textLen);
    }

    GFX->setBitmapModulation(savedColor);
}

void GuiControl::getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent)
{
    lastGuiEvent;

    cursor = NULL;
    showCursor = true;
}

