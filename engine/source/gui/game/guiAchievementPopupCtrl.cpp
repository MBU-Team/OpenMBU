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
}

//------------------------------------------------------------------------------

void GuiAchievementPopupCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("icon", TypeFilename, Offset(mIconBitmapName, GuiAchievementPopupCtrl));
    addField("background", TypeFilename, Offset(mBackgroundBitmapName, GuiAchievementPopupCtrl));
    addField("title", TypeString, Offset(mTitle, GuiAchievementPopupCtrl));
}

//------------------------------------------------------------------------------

bool GuiAchievementPopupCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    setIcon(mIconBitmapName);
    setBackground(mBackgroundBitmapName);

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

    GFX->setBitmapModulation(mProfile->mFontColor);
    renderJustifiedText(offset, mBounds.extent, mTitle);

    //Point2I headerExtent(mBounds.extent.x, mBounds.extent.y / 2);
    //renderJustifiedText(offset, headerExtent, "Achievement Unlocked");

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
