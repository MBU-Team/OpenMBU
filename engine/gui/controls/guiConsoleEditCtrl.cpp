//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "gui/core/guiCanvas.h"
#include "gui/controls/guiConsoleEditCtrl.h"
#include "core/frameAllocator.h"

IMPLEMENT_CONOBJECT(GuiConsoleEditCtrl);

GuiConsoleEditCtrl::GuiConsoleEditCtrl()
{
    mSinkAllKeyEvents = true;
    mSiblingScroller = NULL;
    mUseSiblingScroller = true;
}

void GuiConsoleEditCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Misc");
    addField("useSiblingScroller", TypeBool, Offset(mUseSiblingScroller, GuiConsoleEditCtrl));
    endGroup("Misc");
}

bool GuiConsoleEditCtrl::onKeyDown(const GuiEvent& event)
{
    S32 stringLen = dStrlen(mText);
    setUpdate();

    if (event.keyCode == KEY_TAB)
    {
        if (event.modifier & SI_SHIFT)
        {
            // Do some skullduggery so that we're working against the mTextBuffer.
            FrameTemp<UTF8> tmpBuff(mTextBuffer.length() * 3 + 1);
            mTextBuffer.get(tmpBuff, mTextBuffer.length() * 3 + 1);
            mCursorPos = Con::tabComplete(tmpBuff, mCursorPos, GuiTextCtrl::MAX_STRING_LENGTH, false);
            mTextBuffer.set(tmpBuff);
            return true;
        }
        else
        {
            FrameTemp<UTF8> tmpBuff(GuiTextCtrl::MAX_STRING_LENGTH + 1);
            mTextBuffer.get(tmpBuff, GuiTextCtrl::MAX_STRING_LENGTH + 1);
            mCursorPos = Con::tabComplete(tmpBuff, mCursorPos, GuiTextCtrl::MAX_STRING_LENGTH, true);
            mTextBuffer.set(tmpBuff);
            return true;
        }
    }
    else if ((event.keyCode == KEY_PAGE_UP) || (event.keyCode == KEY_PAGE_DOWN))
    {
        // See if there's some other widget that can scroll the console history.
        if (mUseSiblingScroller)
        {
            if (mSiblingScroller)
            {
                return mSiblingScroller->onKeyDown(event);
            }
            else
            {
                // Let's see if we can find it...
                SimGroup* pGroup = getGroup();
                if (pGroup)
                {
                    // Find the first scroll control in the same group as us.
                    for (SimSetIterator itr(pGroup); *itr; ++itr)
                    {
                        if (mSiblingScroller = dynamic_cast<GuiScrollCtrl*>(*itr))
                        {
                            return mSiblingScroller->onKeyDown(event);
                        }
                    }
                }

                // No luck... so don't try, next time.
                mUseSiblingScroller = false;
            }
        }
    }

    return Parent::onKeyDown(event);
}

