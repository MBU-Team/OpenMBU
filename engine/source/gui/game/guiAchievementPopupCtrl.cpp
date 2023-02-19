#include "gui/game/guiAchievementPopupCtrl.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiAchievementPopupCtrl);

//------------------------------------------------------------------------------

GuiAchievementPopupCtrl::GuiAchievementPopupCtrl()
{
    mBitmapName = StringTable->insert("");
    mBackgroundTextureObject = NULL;
    mIconTextureObject = NULL;
    mTitle = StringTable->insert("");
}

//------------------------------------------------------------------------------

void GuiAchievementPopupCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("bitmap", TypeFilename, Offset(mBitmapName, GuiAchievementPopupCtrl));
}

//------------------------------------------------------------------------------

bool GuiAchievementPopupCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    mBackgroundTextureObject.set("marble/client/ui/achievement/background", &GFXDefaultGUIProfile);
    setBitmap(mBitmapName);

    mProfile->constructBitmapArray();

    return true;
}

void GuiAchievementPopupCtrl::setBitmap(const char *name)
{
    mBitmapName = StringTable->insert(name);
    if (*mBitmapName)
        mIconTextureObject.set(mBitmapName, &GFXDefaultGUIProfile);
    else
        mIconTextureObject = NULL;

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

    renderChildControls(offset, updateRect);
}

ConsoleMethod(GuiAchievementPopupCtrl, setBitmap, void, 3, 3, "(string filename)"
    "Set the bitmap displayed in the control. Note that it is limited in size, to 256x256.")
{
    char fileName[1024];
    Con::expandScriptFilename(fileName, sizeof(fileName), argv[2]);
    object->setBitmap(fileName);
}

ConsoleMethod(GuiAchievementPopupCtrl, setTitle, void, 3, 3, "(string title)"
    "Set the title of the achievement.")
{
    object->setTitle(argv[2]);
}
