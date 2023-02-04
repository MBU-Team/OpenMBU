#include "guiXboxButtonCtrl.h"
#include "console/consoleTypes.h"
#include "sfx/sfxSystem.h"

IMPLEMENT_CONOBJECT(GuiXboxButtonCtrl);

//--------------------------------------------------------
// Initialization
//--------------------------------------------------------

GuiXboxButtonCtrl::GuiXboxButtonCtrl()
{
    mInitialText = StringTable->insert("");
    mInitialTextID = StringTable->insert("");
    mText[0] = '\0';
    mButtonState = Normal;
    mHovering = false;
}

void GuiXboxButtonCtrl::initPersistFields()
{
    Parent::initPersistFields();

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
    //mButtonState = Normal;
    //mHovering = false;
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

//--------------------------------------------------------
// Misc
//--------------------------------------------------------

//void GuiXboxButtonCtrl::setVisible(bool value)
//{
//    Parent ::setVisible(value);
//
//    if (!value)
//    {
//        mButtonState = Normal;
//        mHovering = false;
//    }
//}

void GuiXboxButtonCtrl::setText(const char* text)
{
    // Hacky workaround to make hover not persist between guis
    if (dStrcmp(text, mText) == 0)
    {
        mButtonState = Normal;
        mHovering = false;
    }

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

    S32 numBitmaps = mProfile->mBitmapArrayRects.size();
    bool hasBitmap = numBitmaps > 0;

    S32 bitmapHeight = 0;

    if (numBitmaps > 0)
        bitmapHeight = mProfile->mBitmapArrayRects[0].extent.y;

    Point2I clickOffset(0, 0);

    GFX->clearBitmapModulation();
    ColorI color(128, 128, 128);

    if (mButtonState == Down)
    {
        color = ColorI(64, 64, 64, 255);
        //clickOffset.set(1, 1);
    } else if (mButtonState == Hover)
    {
        color = ColorI(200, 200, 200);
    }

    RectI ctrlRect(offset + clickOffset, mBounds.extent);
    GFX->drawRectFill(ctrlRect, color);

    Point2I point(0, 0);


    GFX->clearBitmapModulation();

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
    //onMouseDown(event);
}

void GuiXboxButtonCtrl::onMouseDown(const GuiEvent &event)
{
    mButtonState = Down;
    //if (mProfile->mSoundButtonDown)
    //    SFX->playOnce(mProfile->mSoundButtonDown);
}

void GuiXboxButtonCtrl::onMouseUp(const GuiEvent &event)
{
    if (mHovering)
        mButtonState = Hover;
    else
        mButtonState = Normal;

    if (mConsoleCommand[0])
        Con::evaluate(mConsoleCommand, false);
}

void GuiXboxButtonCtrl::onMouseMove(const GuiEvent &event)
{
    mButtonState = Hover;
}

void GuiXboxButtonCtrl::onMouseEnter(const GuiEvent &event)
{
    if (mProfile->mSoundButtonOver)
        SFX->playOnce(mProfile->mSoundButtonOver);
    mButtonState = Hover;
    mHovering = true;
}

void GuiXboxButtonCtrl::onMouseLeave(const GuiEvent &event)
{
    mButtonState = Normal;
    mHovering = false;
}
