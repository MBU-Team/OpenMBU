//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "sfx/sfxSystem.h"

#include "gui/controls/guiGameListMenuCtrl.h"

//IMPLEMENT_CONOBJECT(GuiGameListMenuCtrl);

GuiGameListMenuCtrl::GuiGameListMenuCtrl()
{
    mRowHeight = 0;
    mRowsPerPage = 0;
    mSelected =  0;
    mTopRow = 0;
}

//void GuiGameListMenuCtrl::initPersistFields()
//{
//    
//}

//--------------------------------------------------------

bool GuiGameListMenuCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    mRowHeight = mProfile->mFonts[0].mFont->getHeight();
    mRowsPerPage = mBounds.extent.y / mRowHeight;
    mProfile->constructBitmapArray();
    setActive(true);

    return true;
}

void GuiGameListMenuCtrl::onSleep()
{
    Parent::onSleep();
}

//--------------------------------------------------------

void GuiGameListMenuCtrl::move(int increment)
{
    bool inBounds = increment + mSelected < 0;
    mSelected += increment;
    if (inBounds)
    {
        mSelected = mRowText.size() - 1;
    }
    else if (mSelected > mRowText.size() - 1)
    {
        mSelected = 0;
    }

    S32 selected = mSelected;
    S32 top = mTopRow;
    if (selected >= top)
    {
        S32 rows = mRowsPerPage;
        if (selected <= rows + top - 1)
            goto RETURN; // TODO: avoid using goto
        selected -= rows + 1;
    }
    mTopRow = selected;

RETURN:

    SFXProfile* sfx = mProfile->mSoundButtonOver;
    if (sfx)
        SFX->playOnce(sfx);

    Con::executef(this, 1, "onSelChange");
}

void GuiGameListMenuCtrl::moveUp()
{
    move(-1);
}

void GuiGameListMenuCtrl::moveDown()
{
    move(1);
}

void GuiGameListMenuCtrl::setSelectedIndex(int selected)
{
    if (selected < 0)
        return;

    S32 count = mRowText.size();
    if (selected < count)
    {
        S32 rows = mRowsPerPage;
        mSelected = selected;
        if (rows != 0 && count > rows)
        {
            mTopRow = selected;
        }
    }
}

void GuiGameListMenuCtrl::clear()
{
    mRowText.clear();
    mRowData.clear();
}

int GuiGameListMenuCtrl::getSelectedIndex()
{
    return mSelected;
}
