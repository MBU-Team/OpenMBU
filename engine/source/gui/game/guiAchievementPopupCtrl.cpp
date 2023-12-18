#include "gui/game/guiAchievementPopupCtrl.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiAchievementPopupCtrl);

//------------------------------------------------------------------------------

GuiAchievementPopupCtrl::GuiAchievementPopupCtrl()
{
    mIconBitmapName = StringTable->insert("");
    mIconTextureObject = NULL;
    mBackgroundBitmapName = StringTable->insert("marble/client/ui/achievement/background");
    mBackgroundTextureObject = NULL;
    mTitle = StringTable->insert("");
    mHeader = StringTable->insert("Achievement Unlocked");
}

//------------------------------------------------------------------------------

void GuiAchievementPopupCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("icon", TypeFilename, Offset(mIconBitmapName, GuiAchievementPopupCtrl));
    addField("background", TypeFilename, Offset(mBackgroundBitmapName, GuiAchievementPopupCtrl));
    addField("title", TypeString, Offset(mTitle, GuiAchievementPopupCtrl));
    addField("header", TypeString, Offset(mHeader, GuiAchievementPopupCtrl));
}

//------------------------------------------------------------------------------

bool GuiAchievementPopupCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    setIcon(mIconBitmapName);
    setBackground(mBackgroundBitmapName);
    setHeader(mHeader);

    mProfile->constructBitmapArray();

    return true;
}

void GuiAchievementPopupCtrl::setIcon(const char *name)
{
    mIconBitmapName = StringTable->insert(name);
    if (*mIconBitmapName)
        mIconTextureObject.set(mIconBitmapName, &GFXDefaultGUIProfile);
    else
        mIconTextureObject = NULL;

    setUpdate();
}

void GuiAchievementPopupCtrl::setBackground(const char *name)
{
    mBackgroundBitmapName = StringTable->insert(name);
    if (*mBackgroundBitmapName)
        mBackgroundTextureObject.set(mBackgroundBitmapName, &GFXDefaultGUIProfile);
    else
        mBackgroundTextureObject = NULL;

    setUpdate();
}

void GuiAchievementPopupCtrl::setTitle(const char *title)
{
    mTitle = StringTable->insert(title);
    setUpdate();
}

void GuiAchievementPopupCtrl::setHeader(const char *header)
{
    mHeader = StringTable->insert(header);
    setUpdate();
}

//------------------------------------------------------------------------------

void GuiAchievementPopupCtrl::onRender(Point2I offset, const RectI &updateRect)
{
    if (mBackgroundTextureObject)
    {
        GFX->clearBitmapModulation();

        RectI rect(offset, mBounds.extent);
        GFX->drawBitmapStretch(mBackgroundTextureObject, rect);
    }

    if (mProfile->mBorder || !mBackgroundTextureObject)
    {
        RectI rect(offset, mBounds.extent);
        GFX->drawRect(rect, mProfile->mBorderColor);
    }

    if (mIconTextureObject)
    {
        GFX->clearBitmapModulation();

        RectI rect(offset.x + ACHIEVEMENT_ICON_OFFSET_X, offset.y + ACHIEVEMENT_ICON_OFFSET_Y, 64, 64);
        GFX->drawBitmapStretch(mIconTextureObject, rect);
    }

    // I can't quite find an easy way to change the font size within this method, so both the achievement unlocked and
    // actual achievement text are the same size.

    GFX->setBitmapModulation(mProfile->mFontColor);
    Point2I textExtent(mBounds.extent.x + ACHIEVEMENT_HEADER_OFFSET_X, mBounds.extent.y - ACHIEVEMENT_HEADER_OFFSET_Y);
    Point2I textExtent2(mBounds.extent.x, mBounds.extent.y - ACHIEVEMENT_HEADER_OFFSET_Y);
    renderJustifiedText(mProfile->mFonts[0].mFont, offset, textExtent, mHeader);

    Point2I titleOffset(offset.x - ACHIEVEMENT_TITLE_OFFSET_X, offset.y + (mBounds.extent.y - textExtent2.y) - ACHIEVEMENT_TITLE_OFFSET_Y);
    renderJustifiedText(mProfile->mFonts[1].mFont, titleOffset, textExtent2, mTitle);

    renderChildControls(offset, updateRect);
}

ConsoleMethod(GuiAchievementPopupCtrl, setIcon, void, 3, 3, "(string filename)"
    "Set the icon bitmap displayed in the control. Note that it is limited in size, to 256x256.")
{
    char fileName[1024];
    Con::expandScriptFilename(fileName, sizeof(fileName), argv[2]);
    object->setIcon(fileName);
}

ConsoleMethod(GuiAchievementPopupCtrl, setBackground, void, 3, 3, "(string filename)"
    "Set the background bitmap displayed in the control.")
{
    char fileName[1024];
    Con::expandScriptFilename(fileName, sizeof(fileName), argv[2]);
    object->setBackground(fileName);
}

ConsoleMethod(GuiAchievementPopupCtrl, setTitle, void, 3, 3, "(string title)"
    "Set the title of the achievement.")
{
    object->setTitle(argv[2]);
}

ConsoleMethod(GuiAchievementPopupCtrl, setHeader, void, 3, 3, "(string title)"
    "Set the title of the achievement.")
{
    object->setHeader(argv[2]);
}
