#include "guiXboxButtonCtrl.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiXboxButtonCtrl);

//--------------------------------------------------------
// Initialization
//--------------------------------------------------------

GuiXboxButtonCtrl::GuiXboxButtonCtrl()
{
    mInitialText = StringTable->insert("");
    mInitialTextID = StringTable->insert("");
    mText[0] = '\0';
    // TODO: init
}

void GuiXboxButtonCtrl::initPersistFields()
{
    Parent::initPersistFields();

    // TODO: add fields
    addField("text", TypeCaseString, Offset(mInitialText, GuiXboxButtonCtrl));
    addField("textID", TypeString, Offset(mInitialTextID, GuiXboxButtonCtrl));
}

//--------------------------------------------------------
// States
//--------------------------------------------------------

bool GuiXboxButtonCtrl::onAdd()
{
    if (!Parent::onAdd())
        return false;
    dStrncpy(mText, (UTF8*)mInitialText, MAX_STRING_LENGTH);
    mText[MAX_STRING_LENGTH] = '\0';
    return true;
}

void GuiXboxButtonCtrl::inspectPostApply()
{
    Parent::inspectPostApply();
    if (mInitialTextID && *mInitialTextID != 0)
        setTextID(mInitialTextID);
    else
        setText(mInitialText);
}

bool GuiXboxButtonCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    if (mInitialTextID && *mInitialTextID != 0)
        setTextID(mInitialTextID);

    return true;
}

void GuiXboxButtonCtrl::onSleep()
{
    Parent::onSleep();
}

//--------------------------------------------------------
// Console Methods
//--------------------------------------------------------

ConsoleMethod(GuiXboxButtonCtrl, setText, void, 3, 3, "(string text) - sets the text of the button to the string.")
{
    argc;
    object->setText(argv[2]);
}

ConsoleMethod(GuiXboxButtonCtrl, setTextID, void, 3, 3, "(string id) - sets the text of the button to the localized string.")
{
    argc;
    object->setTextID(argv[2]);
}

ConsoleMethod(GuiXboxButtonCtrl, getText, const char*, 2, 2, "() - returns the text of the button.")
{
    argc; argv;
    return object->getText();
}

// TODO: Console Methods

//--------------------------------------------------------
// Misc
//--------------------------------------------------------

void GuiXboxButtonCtrl::setText(const char* text)
{
    if (text)
        dStrncpy(mText, (UTF8*)text, MAX_STRING_LENGTH);
    mText[MAX_STRING_LENGTH] = '\0';
}

void GuiXboxButtonCtrl::setTextID(const char* id)
{
    S32 n = Con::getIntVariable(id, -1);
    if (n != -1)
    {
        mInitialTextID = StringTable->insert(id);
        setTextID(n);
    }
}

void GuiXboxButtonCtrl::setTextID(S32 id)
{
    const UTF8* str = getGUIString(id);
    if (str)
        setText((const char*)str);
    //mInitialTextID = id;
}

const char* GuiXboxButtonCtrl::getText()
{
    return mText;
}

//--------------------------------------------------------
// Rendering
//--------------------------------------------------------

void GuiXboxButtonCtrl::onPreRender()
{
    const char* var = getVariable();
    if (var && var[0] && dStricmp((char*)mText, var))
        setText(var);
}

void GuiXboxButtonCtrl::onRender(Point2I offset, const RectI &updateRect)
{
    if (!isCurrentUIMode())
        return;

    // TODO: onRender

    S32 numBitmaps = mProfile->mBitmapArrayRects.size();
    bool hasBitmap = numBitmaps > 0;

    S32 bitmapHeight = 0;

    if (numBitmaps > 0)
        bitmapHeight = mProfile->mBitmapArrayRects[0].extent.y;

    RectI ctrlRect(offset, mBounds.extent);
    GFX->drawRectFill(ctrlRect, ColorI(128, 128, 128));

    Point2I point(0, 0);
    Point2I clickOffset(0, 0);

    point.x = offset.x;
    point.y = offset.y;
    //point += mProfile->mTextOffset;
    renderJustifiedText(point + clickOffset, Point2I(mBounds.extent.x, mBounds.extent.y), mText);
}

//--------------------------------------------------------
// Input
//--------------------------------------------------------

void GuiXboxButtonCtrl::onMouseDragged(const GuiEvent &event)
{
    Parent::onMouseDragged(event);
}

void GuiXboxButtonCtrl::onMouseDown(const GuiEvent &event)
{
    // TODO: onMouseDown
}

void GuiXboxButtonCtrl::onMouseUp(const GuiEvent &event)
{
    // TODO: onMouseUp
}

void GuiXboxButtonCtrl::onMouseMove(const GuiEvent &event)
{
    // TODO: onMouseMove
}

void GuiXboxButtonCtrl::onMouseLeave(const GuiEvent &event)
{
    // TODO: onMouseLeave
}
