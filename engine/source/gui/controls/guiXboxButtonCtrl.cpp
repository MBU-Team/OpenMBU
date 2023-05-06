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

    mProfile->constructBitmapArray();

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

ConsoleMethod(GuiXboxButtonCtrl, setHover, void, 3, 3, "(hover) - sets the button hover state.")
{
    argc; argv;
    object->setButtonHover(atoi(argv[2]));
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

void GuiXboxButtonCtrl::setButtonHover(bool hover)
{
    if (hover)
    {
        mButtonState = Hover;
        mHovering = true;
    }
    else
    {
        mButtonState = Normal;
        mHovering = false;
    }
}

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
    bool hasBitmap = numBitmaps > 4;

    S32 unselectedBitmapLeftIndex = 0;
    S32 unselectedBitmapMiddleIndex = 1;
    S32 unselectedBitmapRightIndex = 2;
    S32 selectedBitmapLeftIndex = 3;
    S32 selectedBitmapMiddleIndex = 4;
    S32 selectedBitmapRightIndex = 5;

    S32 bitmapHeight = 0;

    if (hasBitmap)
        bitmapHeight = mProfile->mBitmapArrayRects[0].extent.y;

    Point2I clickOffset(0, 0);

    S32 leftBitmapIndex = unselectedBitmapLeftIndex;
    S32 middleBitmapIndex = unselectedBitmapMiddleIndex;
    S32 rightBitmapIndex = unselectedBitmapRightIndex;

    GFX->clearBitmapModulation();
    ColorI color(128, 128, 128);

    if (mButtonState == Down)
    {
        if (hasBitmap)
        {
            leftBitmapIndex = selectedBitmapLeftIndex;
            middleBitmapIndex = selectedBitmapMiddleIndex;
            rightBitmapIndex = selectedBitmapRightIndex;
            GFX->setBitmapModulation(ColorI(128, 128, 128, 255));
            clickOffset.set(1, 1);
        } else {
            color = ColorI(64, 64, 64, 255);
        }
    } else if (mButtonState == Hover)
    {
        if (hasBitmap)
        {
            leftBitmapIndex = selectedBitmapLeftIndex;
            middleBitmapIndex = selectedBitmapMiddleIndex;
            rightBitmapIndex = selectedBitmapRightIndex;
        } else
        {
            color = ColorI(200, 200, 200);
        }
    }

    Point2I buttonPos = offset + clickOffset;

    if (hasBitmap)
    {
        RectI leftImg = mProfile->mBitmapArrayRects[leftBitmapIndex];
        //RectI leftCtrlRect(buttonPos, Point2I(leftImg.extent.x, leftImg.extent.y));
        GFX->drawBitmapSR(mProfile->mTextureObject, buttonPos, leftImg);

        RectI rightImg = mProfile->mBitmapArrayRects[rightBitmapIndex];
        S32 ctrlWidthLeft = mBounds.extent.x - rightImg.extent.x;

        RectI middleImg = mProfile->mBitmapArrayRects[middleBitmapIndex];
        //RectI middleCtrlRect(buttonPos + Point2I(leftImg.extent.x, 0), Point2I(ctrlWidthLeft - leftImg.extent.x, middleImg.extent.y));
        //GFX->drawBitmapStretchSR(mProfile->mTextureObject, middleCtrlRect, middleImg);

        S32 midSrcWidth = middleImg.extent.x;
        S32 midDestWidth = ctrlWidthLeft - leftImg.extent.x;

        F32 pieceSize = 1.0f;//(F32)midDestWidth / (F32)midSrcWidth;

        // Hack to deal with drawBitmapStretchSR giving visual bugs
        for (S32 i = 0; i < midDestWidth; i++)//midSrcWidth; i++)
        {
            F32 srcIndex = ((F32)i * (F32)midSrcWidth / (F32)midDestWidth);//i;

            RectF midPart(Point2F(middleImg.point.x + srcIndex, middleImg.point.y), Point2F(pieceSize, middleImg.extent.y));
            Point2F midPos(buttonPos.x + leftImg.extent.x + i * pieceSize, buttonPos.y);
            GFX->drawBitmapSR(mProfile->mTextureObject, midPos, midPart);
        }

        //RectI rightCtrlRect(buttonPos + Point2I(ctrlWidthLeft, 0), Point2I(rightImg.extent.x, rightImg.extent.y));
        GFX->drawBitmapSR(mProfile->mTextureObject, buttonPos + Point2I(ctrlWidthLeft, 0), rightImg);
    } else {
        RectI ctrlRect(buttonPos, mBounds.extent);
        GFX->drawRectFill(ctrlRect, color);
    }

    GFX->clearBitmapModulation();

    ColorI fontColor;
    if (mButtonState == Hover)
        fontColor = mProfile->mFontColors[1];
    else
        fontColor = mProfile->mFontColors[0];

    fontColor.alpha = 255;

    if (mButtonState == Down)
        GFX->setBitmapModulation(fontColor * 0.5f);
    else
        GFX->setBitmapModulation(fontColor);

    Point2I point(0, 0);
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
