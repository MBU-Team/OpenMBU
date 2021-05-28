//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "gui/core/guiCanvas.h"
#include "gui/controls/guiMLTextCtrl.h"
#include "gui/controls/guiTextEditCtrl.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gfx/gfxDevice.h"
#include "core/frameAllocator.h"
#include "sfx/sfxSystem.h"

IMPLEMENT_CONOBJECT(GuiTextEditCtrl);

U32 GuiTextEditCtrl::smNumAwake = 0;

GuiTextEditCtrl::GuiTextEditCtrl()
{
    mInsertOn = true;
    mBlockStart = 0;
    mBlockEnd = 0;
    mCursorPos = 0;
    mCursorOn = false;
    mNumFramesElapsed = 0;

    mDragHit = false;
    mTabComplete = false;
    mScrollDir = 0;

    mUndoBlockStart = 0;
    mUndoBlockEnd = 0;
    mUndoCursorPos = 0;
    mPasswordText = false;

    mSinkAllKeyEvents = false;

    mActive = true;

    mTextOffsetReset = true;

    mHistoryDirty = false;
    mHistorySize = 0;
    mHistoryLast = -1;
    mHistoryIndex = 0;
    mHistoryBuf = NULL;

    mValidateCommand = StringTable->insert("");
    mEscapeCommand = StringTable->insert("");
    mPasswordMask = StringTable->insert("*");
    mDeniedSound = dynamic_cast<SFXProfile*>(Sim::findObject("InputDeniedSound"));

    mEditCursor = NULL;
}

GuiTextEditCtrl::~GuiTextEditCtrl()
{
    //delete the history buffer if it exists
    if (mHistoryBuf)
    {
        for (S32 i = 0; i < mHistorySize; i++)
            delete[] mHistoryBuf[i];

        delete[] mHistoryBuf;
    }
}

void GuiTextEditCtrl::initPersistFields()
{
    Parent::initPersistFields();

    addField("validate", TypeString, Offset(mValidateCommand, GuiTextEditCtrl));
    addField("escapeCommand", TypeString, Offset(mEscapeCommand, GuiTextEditCtrl));
    addField("historySize", TypeS32, Offset(mHistorySize, GuiTextEditCtrl));
    addField("password", TypeBool, Offset(mPasswordText, GuiTextEditCtrl));
    addField("tabComplete", TypeBool, Offset(mTabComplete, GuiTextEditCtrl));
    addField("deniedSound", TypeSFXProfilePtr, Offset(mDeniedSound, GuiTextEditCtrl));
    addField("sinkAllKeyEvents", TypeBool, Offset(mSinkAllKeyEvents, GuiTextEditCtrl));
    addField("password", TypeBool, Offset(mPasswordText, GuiTextEditCtrl));
    addField("passwordMask", TypeString, Offset(mPasswordMask, GuiTextEditCtrl));
}

bool GuiTextEditCtrl::onAdd()
{
    if (!Parent::onAdd())
        return false;

    //create the history buffer
    if (mHistorySize > 0)
    {
        mHistoryBuf = new UTF8 * [mHistorySize];
        for (S32 i = 0; i < mHistorySize; i++)
        {
            mHistoryBuf[i] = new UTF8[GuiTextCtrl::MAX_STRING_LENGTH + 1];
            mHistoryBuf[i][0] = '\0';
        }
    }

    setText(mText);

    return true;
}

void GuiTextEditCtrl::onStaticModified(const char* slotName)
{
    if (!dStricmp(slotName, "text"))
        setText(mText);
}

bool GuiTextEditCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    // If this is the first awake text edit control, enable keyboard translation
    if (smNumAwake == 0)
        Platform::enableKeyboardTranslation();
    ++smNumAwake;

    return true;
}

void GuiTextEditCtrl::onSleep()
{
    Parent::onSleep();

    // If this is the last awake text edit control, disable keyboard translation
    --smNumAwake;
    if (smNumAwake == 0)
        Platform::disableKeyboardTranslation();
}

void GuiTextEditCtrl::execConsoleCallback()
{
    // Execute the console command!
    if (mConsoleCommand[0])
    {
        char buf[16];
        dSprintf(buf, sizeof(buf), "%d", getId());
        Con::setVariable("$ThisControl", buf);
        Con::evaluate(mConsoleCommand, false);
    }

    // Update the console variable:
    UTF8 buffer[1024];
    mTextBuffer.get(buffer, 1024);

    if (mConsoleVariable[0])
        Con::setVariable(mConsoleVariable, (char*)buffer);
}

void GuiTextEditCtrl::updateHistory(StringBuffer* inTxt, bool moveIndex)
{
    FrameTemp<UTF8> textString(inTxt->length() * 3 + 1);
    inTxt->get(textString, inTxt->length() * 3 + 1);

    UTF8* txt = textString;

    if (!txt)
        return;

    if (!mHistorySize)
        return;

    // see if it's already in
    if (mHistoryLast == -1 || dStrcmp((const UTF8*)txt, (const UTF8*)mHistoryBuf[mHistoryLast]))
    {
        if (mHistoryLast == mHistorySize - 1) // we're at the history limit... shuffle the pointers around:
        {
            UTF8* first = mHistoryBuf[0];
            for (U32 i = 0; i < mHistorySize - 1; i++)
                mHistoryBuf[i] = mHistoryBuf[i + 1];
            mHistoryBuf[mHistorySize - 1] = first;
            if (mHistoryIndex > 0)
                mHistoryIndex--;
        }
        else
            mHistoryLast++;

        inTxt->get(mHistoryBuf[mHistoryLast], GuiTextCtrl::MAX_STRING_LENGTH);
        mHistoryBuf[mHistoryLast][GuiTextCtrl::MAX_STRING_LENGTH] = '\0';
    }

    if (moveIndex)
        mHistoryIndex = mHistoryLast + 1;
}

void GuiTextEditCtrl::getText(char* dest)
{
    if (dest)
        mTextBuffer.get((UTF8*)dest, GuiTextCtrl::MAX_STRING_LENGTH + 1);
}

void GuiTextEditCtrl::setText(const char* txt)
{
    if (txt && txt[0] != 0)
    {
        Parent::setText(txt);
        mTextBuffer.set(txt);
    }
    else
        mTextBuffer.set("");

    mCursorPos = mTextBuffer.length();
}
//*** DAW: Select all text
void GuiTextEditCtrl::selectAllText()
{
    mBlockStart = 0;
    mBlockEnd = dStrlen(mText);
    setUpdate();
}

//*** DAW: Force a validate on the current text
void GuiTextEditCtrl::forceValidateText()
{
    if (mValidateCommand[0])
    {
        char buf[16];
        dSprintf(buf, sizeof(buf), "%d", getId());
        Con::setVariable("$ThisControl", buf);
        Con::evaluate(mValidateCommand, false);
    }
}

void GuiTextEditCtrl::reallySetCursorPos(const S32 newPos)
{
    S32 charCount = mTextBuffer.length();
    S32 realPos = newPos > charCount ? charCount : newPos < 0 ? 0 : newPos;
    if (realPos != mCursorPos)
    {
        mCursorPos = realPos;
        setUpdate();
    }
}

S32 GuiTextEditCtrl::setCursorPos(const Point2I& offset)
{
    Point2I ctrlOffset = localToGlobalCoord(Point2I(0, 0));
    S32 charCount = mTextBuffer.length();
    S32 charLength = 0;
    S32 curX;

    curX = offset.x - ctrlOffset.x;
    setUpdate();

    //if the cursor is too far to the left
    if (curX < 0)
        return -1;

    //if the cursor is too far to the right
    if (curX >= ctrlOffset.x + mBounds.extent.x)
        return -2;

    curX = offset.x - mTextOffset.x;
    S32 count = 0;
    if (mTextBuffer.length() == 0)
        return 0;

    for (count = 0; count < mTextBuffer.length(); count++)
    {
        if (mPasswordText)
            charLength += mFont->getCharXIncrement(mPasswordMask[0]);
        else
            charLength += mFont->getCharXIncrement(mTextBuffer.getChar(count));

        if (charLength > curX)
            break;
    }

    return count;
}

void GuiTextEditCtrl::onMouseDown(const GuiEvent& event)
{
    mDragHit = false;

    //*** DAW: If we have a double click, select all text.  Otherwise
    //*** act as before by clearing any selection.
    bool doubleClick = (event.mouseClickCount > 1);
    if (doubleClick)
    {
        selectAllText();

    }
    else
    {
        //undo any block function
        mBlockStart = 0;
        mBlockEnd = 0;
    }

    //find out where the cursor should be
    S32 pos = setCursorPos(event.mousePoint);

    // if the position is to the left
    if (pos == -1)
        mCursorPos = 0;
    else if (pos == -2) //else if the position is to the right
        mCursorPos = mTextBuffer.length();
    else //else set the mCursorPos
        mCursorPos = pos;

    //save the mouseDragPos
    mMouseDragStart = mCursorPos;

    // lock the mouse
    mouseLock();

    //set the drag var
    mDragHit = true;

    //let the parent get the event
    setFirstResponder();
}

void GuiTextEditCtrl::onMouseDragged(const GuiEvent& event)
{
    S32 pos = setCursorPos(event.mousePoint);

    // if the position is to the left
    if (pos == -1)
        mScrollDir = -1;
    else if (pos == -2) // the position is to the right
        mScrollDir = 1;
    else // set the new cursor position
    {
        mScrollDir = 0;
        mCursorPos = pos;
    }

    // update the block:
    mBlockStart = getMin(mCursorPos, mMouseDragStart);
    mBlockEnd = getMax(mCursorPos, mMouseDragStart);
    if (mBlockStart < 0)
        mBlockStart = 0;

    if (mBlockStart == mBlockEnd)
        mBlockStart = mBlockEnd = 0;

    //let the parent get the event
    Parent::onMouseDragged(event);
}

void GuiTextEditCtrl::onMouseUp(const GuiEvent& event)
{
    event;
    mDragHit = false;
    mScrollDir = 0;
    mouseUnlock();
}

void GuiTextEditCtrl::saveUndoState()
{
    //save the current state
    mUndoText.set(&mTextBuffer);
    mUndoBlockStart = mBlockStart;
    mUndoBlockEnd = mBlockEnd;
    mUndoCursorPos = mCursorPos;
}

void GuiTextEditCtrl::onCopy(bool andCut)
{
    // Don't copy/cut password field!
    if (mPasswordText)
        return;

    UTF8 buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
    if (mBlockEnd > 0)
    {
        //save the current state
        saveUndoState();

        //copy the text to the clipboard
        StringBuffer tmp = mTextBuffer.substring(mBlockStart, mBlockEnd - mBlockStart);
        FrameTemp<UTF8> clipBuff((mBlockEnd - mBlockStart) * 3 + 1);
        tmp.get(clipBuff, (mBlockEnd - mBlockStart) * 3 + 1);

        Platform::setClipboard((char*)(UTF8*)clipBuff);

        //if we pressed the cut shortcut, we need to cut the selected text from the control...
        if (andCut)
        {
            mTextBuffer.cut(mBlockStart, mBlockEnd - mBlockStart);
            mCursorPos = mBlockStart;
        }

        mBlockStart = 0;
        mBlockEnd = 0;
    }

}

void GuiTextEditCtrl::onPaste()
{
    S32 stringLen = mTextBuffer.length();
    UTF8 buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];

    //first, make sure there's something in the clipboard to copy...
    const char* temp = (const char*)Platform::getClipboard();
    UTF8* clipBuf = (UTF8*)GuiMLTextCtrl::stripControlChars(temp);
    if (dStrlen(clipBuf) <= 0)
        return;

    //save the current state
    saveUndoState();

    //delete anything hilited
    if (mBlockEnd > 0)
    {
        mTextBuffer.cut(mBlockStart, mBlockEnd - mBlockStart);
        mCursorPos = mBlockStart;
        mBlockStart = 0;
        mBlockEnd = 0;
    }

    StringBuffer tmp(clipBuf);
    S32 pasteLen = tmp.length();
    if ((stringLen + pasteLen) > mMaxStrLen)
    {
        pasteLen = mMaxStrLen - stringLen;

        // Trim down to be pasteLen long.
        tmp.cut(pasteLen, tmp.length() - pasteLen);
    }

    if (mCursorPos == stringLen)
    {
        mTextBuffer.append(tmp);
    }
    else
    {
        mTextBuffer.insert(mCursorPos, tmp);
    }
    mCursorPos += tmp.length();

}

void GuiTextEditCtrl::onUndo()
{
    StringBuffer tempBuffer;
    S32 tempBlockStart;
    S32 tempBlockEnd;
    S32 tempCursorPos;

    //save the current
    tempBuffer.set(&mTextBuffer);
    tempBlockStart = mBlockStart;
    tempBlockEnd = mBlockEnd;
    tempCursorPos = mCursorPos;

    //restore the prev
    mTextBuffer.set(&mUndoText);
    mBlockStart = mUndoBlockStart;
    mBlockEnd = mUndoBlockEnd;
    mCursorPos = mUndoCursorPos;

    //update the undo
    mUndoText.set(&tempBuffer);
    mUndoBlockStart = tempBlockStart;
    mUndoBlockEnd = tempBlockEnd;
    mUndoCursorPos = tempCursorPos;
}

bool GuiTextEditCtrl::onKeyDown(const GuiEvent& event)
{
    if (!isActive())
        return false;

    S32 stringLen = mTextBuffer.length();
    setUpdate();

    // Ugly, but now I'm cool like MarkF.
    if (event.keyCode == KEY_BACKSPACE)
        goto dealWithBackspace;

    if (event.modifier & SI_SHIFT)
    {
        switch (event.keyCode)
        {
        case KEY_TAB:
            if (mTabComplete)
            {
                Con::executef(this, 2, "onTabComplete", "1");
                return(true);
            }
            break; //*** DAW: We don't want to fall through if we don't handle the TAB here.

        case KEY_HOME:
            mBlockStart = 0;
            mBlockEnd = mCursorPos;
            mCursorPos = 0;
            return true;

        case KEY_END:
            mBlockStart = mCursorPos;
            mBlockEnd = stringLen;
            mCursorPos = stringLen;
            return true;

        case KEY_LEFT:
            if ((mCursorPos > 0) & (stringLen > 0))
            {
                //if we already have a selected block
                if (mCursorPos == mBlockEnd)
                {
                    mCursorPos--;
                    mBlockEnd--;
                    if (mBlockEnd == mBlockStart)
                    {
                        mBlockStart = 0;
                        mBlockEnd = 0;
                    }
                }
                else {
                    mCursorPos--;
                    mBlockStart = mCursorPos;

                    if (mBlockEnd == 0)
                    {
                        mBlockEnd = mCursorPos + 1;
                    }
                }
            }
            return true;

        case KEY_RIGHT:
            if (mCursorPos < stringLen)
            {
                if ((mCursorPos == mBlockStart) && (mBlockEnd > 0))
                {
                    mCursorPos++;
                    mBlockStart++;
                    if (mBlockStart == mBlockEnd)
                    {
                        mBlockStart = 0;
                        mBlockEnd = 0;
                    }
                }
                else
                {
                    if (mBlockEnd == 0)
                    {
                        mBlockStart = mCursorPos;
                        mBlockEnd = mCursorPos;
                    }
                    mCursorPos++;
                    mBlockEnd++;
                }
            }
            return true;
        }
    }
    else if (event.modifier & SI_CTRL)
    {
        switch (event.keyCode)
        {
            // Added UNIX emacs key bindings - just a little hack here...

            // BJGTODO: Add vi bindings.

            // Ctrl-B - move one character back
        case KEY_B:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_LEFT;
            return(onKeyDown(new_event));
        }

        // Ctrl-F - move one character forward
        case KEY_F:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_RIGHT;
            return(onKeyDown(new_event));
        }

        // Ctrl-A - move to the beginning of the line
        case KEY_A:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_HOME;
            return(onKeyDown(new_event));
        }

        // Ctrl-E - move to the end of the line
        case KEY_E:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_END;
            return(onKeyDown(new_event));
        }

        // Ctrl-P - move backward in history
        case KEY_P:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_UP;
            return(onKeyDown(new_event));
        }

        // Ctrl-N - move forward in history
        case KEY_N:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_DOWN;
            return(onKeyDown(new_event));
        }

        // Ctrl-D - delete under cursor
        case KEY_D:
        {
            GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_DELETE;
            return(onKeyDown(new_event));
        }

        case KEY_U:
        {
            GuiEvent new_event;
            new_event.modifier = SI_CTRL;
            new_event.keyCode = KEY_DELETE;
            return(onKeyDown(new_event));
        }

        // End added UNIX emacs key bindings

#if !defined(TORQUE_OS_MAC)
         // windows style cut / copy / paste / undo keybinds
        case KEY_C:
        case KEY_X:
        {
            // copy, and cut the text if we hit ctrl-x
            onCopy(event.keyCode == KEY_X);
            return true;
        }
        case KEY_V:
        {
            onPaste();

            // Execute the console command!
            execConsoleCallback();
            return true;
        }

        case KEY_Z:
            if (!mDragHit)
            {
                onUndo();
                return true;
            }
#endif

        case KEY_DELETE:
        case KEY_BACKSPACE:
            //save the current state
            saveUndoState();

            //delete everything in the field
            mTextBuffer.set("");
            mCursorPos = 0;
            mBlockStart = 0;
            mBlockEnd = 0;

            execConsoleCallback();

            return true;
        }
    }
#if defined(TORQUE_OS_MAC)
    // mac style cut / copy / paste / undo keybinds
    else if (event.modifier & SI_ALT)
    {
        // Added Mac cut/copy/paste/undo keys
        // Mac command key maps to alt in torque.
        switch (event.keyCode)
        {
        case KEY_C:
        case KEY_X:
        {
            // copy, and cut the text if we hit cmd-x
            onCopy(event.keyCode == KEY_X);
            return true;
        }
        case KEY_V:
        {
            onPaste();

            // Execute the console command!
            execConsoleCallback();

            return true;
        }

        case KEY_Z:
            if (!mDragHit)
            {
                onUndo();
                return true;
            }
        }
    }
#endif
    else
    {
        switch (event.keyCode)
        {
        case KEY_ESCAPE:
            if (mEscapeCommand[0])
            {
                Con::evaluate(mEscapeCommand);
                return(true);
            }
            return(Parent::onKeyDown(event));

        case KEY_RETURN:
        case KEY_NUMPADENTER:
            //first validate
            if (mProfile->mReturnTab)
            {
                onLoseFirstResponder();
            }

            updateHistory(&mTextBuffer, true);
            mHistoryDirty = false;

            //next exec the alt console command
            if (mAltConsoleCommand[0])
            {
                char buf[16];
                dSprintf(buf, sizeof(buf), "%d", getId());
                Con::setVariable("$ThisControl", buf);
                Con::evaluate(mAltConsoleCommand, false);
            }

            // Notify of Return
            if (isMethod("onReturn"))
                Con::executef(this, 2, "onReturn");

            if (mProfile->mReturnTab)
            {
                GuiCanvas* root = getRoot();
                if (root)
                {
                    root->tabNext();
                    return true;
                }
            }
            return true;

        case KEY_UP:
        {
            if (mHistoryDirty)
            {
                updateHistory(&mTextBuffer, false);
                mHistoryDirty = false;
            }

            mHistoryIndex--;

            if (mHistoryIndex >= 0 && mHistoryIndex <= mHistoryLast)
                setText((char*)mHistoryBuf[mHistoryIndex]);
            else if (mHistoryIndex < 0)
                mHistoryIndex = 0;

            return true;
        }

        case KEY_DOWN:
            if (mHistoryDirty)
            {
                updateHistory(&mTextBuffer, false);
                mHistoryDirty = false;
            }
            mHistoryIndex++;
            if (mHistoryIndex > mHistoryLast)
            {
                mHistoryIndex = mHistoryLast + 1;
                setText("");
            }
            else
                setText((char*)mHistoryBuf[mHistoryIndex]);
            return true;

        case KEY_LEFT:
            mBlockStart = 0;
            mBlockEnd = 0;
            if (mCursorPos > 0)
            {
                mCursorPos--;
            }
            return true;

        case KEY_RIGHT:
            mBlockStart = 0;
            mBlockEnd = 0;
            if (mCursorPos < stringLen)
            {
                mCursorPos++;
            }
            return true;

        case KEY_BACKSPACE:
        dealWithBackspace:
            //save the current state
            saveUndoState();

            if (mBlockEnd > 0)
            {
                mTextBuffer.cut(mBlockStart, mBlockEnd - mBlockStart);
                mCursorPos = mBlockStart;
                mBlockStart = 0;
                mBlockEnd = 0;
                mHistoryDirty = true;

                // Execute the console command!
                execConsoleCallback();

            }
            else if (mCursorPos > 0)
            {
                mTextBuffer.cut(mCursorPos - 1, 1);
                mCursorPos--;
                mHistoryDirty = true;

                // Execute the console command!
                execConsoleCallback();
            }
            return true;

        case KEY_DELETE:
            //save the current state
            saveUndoState();

            if (mBlockEnd > 0)
            {
                mHistoryDirty = true;
                mTextBuffer.cut(mBlockStart, mBlockEnd - mBlockStart);

                mCursorPos = mBlockStart;
                mBlockStart = 0;
                mBlockEnd = 0;

                // Execute the console command!
                execConsoleCallback();
            }
            else if (mCursorPos < stringLen)
            {
                mHistoryDirty = true;
                mTextBuffer.cut(mCursorPos, 1);

                // Execute the console command!
                execConsoleCallback();
            }
            return true;

        case KEY_INSERT:
            mInsertOn = !mInsertOn;
            return true;

        case KEY_HOME:
            mBlockStart = 0;
            mBlockEnd = 0;
            mCursorPos = 0;
            return true;

        case KEY_END:
            mBlockStart = 0;
            mBlockEnd = 0;
            mCursorPos = stringLen;
            return true;

        }
    }

    switch (event.keyCode)
    {
    case KEY_TAB:
        if (mTabComplete)
        {
            Con::executef(this, 2, "onTabComplete", "0");
            return(true);
        }
    case KEY_UP:
    case KEY_DOWN:
    case KEY_ESCAPE:
        return Parent::onKeyDown(event);
    }

    if (mFont->isValidChar(event.ascii))
    {
        // Get the character ready to add to a UTF8 string.
        UTF16 conv[2] = { event.ascii, 0 };
        StringBuffer convertedChar(conv);

        //see if it's a number field
        if (mProfile->mNumbersOnly)
        {
            if (event.ascii == '-')
            {
                //a minus sign only exists at the beginning, and only a single minus sign
                if (mCursorPos != 0)
                {
                    playDeniedSound();
                    return true;
                }

                if (mInsertOn && (mTextBuffer.getChar(0) == '-'))
                {
                    playDeniedSound();
                    return true;
                }
            }
            // BJTODO: This is probably not unicode safe.
            else if (event.ascii < '0' || event.ascii > '9')
            {
                playDeniedSound();
                return true;
            }
        }

        //save the current state
        saveUndoState();

        //delete anything highlighted
        if (mBlockEnd > 0)
        {
            mTextBuffer.cut(mBlockStart, mBlockEnd - mBlockStart);
            mCursorPos = mBlockStart;
            mBlockStart = 0;
            mBlockEnd = 0;
        }

        if ((mInsertOn && (stringLen < mMaxStrLen)) ||
            (!mInsertOn && (mCursorPos < mMaxStrLen)))
        {
            if (mCursorPos == stringLen)
            {
                mTextBuffer.append(convertedChar);
                mCursorPos++;
            }
            else
            {
                if (mInsertOn)
                {
                    mTextBuffer.insert(mCursorPos, convertedChar);
                    mCursorPos++;
                }
                else
                {
                    mTextBuffer.cut(mCursorPos, 1);
                    mTextBuffer.insert(mCursorPos, convertedChar);
                    mCursorPos++;
                }
            }
        }
        else
            playDeniedSound();

        //reset the history index
        mHistoryDirty = true;

        //execute the console command if it exists
        execConsoleCallback();

        return true;
    }

    //not handled - pass the event to it's parent

    // Or eat it if that's appropriate.
    if (mSinkAllKeyEvents)
        return true;

    return Parent::onKeyDown(event);
}

void GuiTextEditCtrl::setFirstResponder()
{
    Parent::setFirstResponder();

    Platform::enableKeyboardTranslation();
}

void GuiTextEditCtrl::onLoseFirstResponder()
{
    Platform::disableKeyboardTranslation();

    //first, update the history
    updateHistory(&mTextBuffer, true);

    //execute the validate command
    if (mValidateCommand[0])
    {
        char buf[16];
        dSprintf(buf, sizeof(buf), "%d", getId());
        Con::setVariable("$ThisControl", buf);
        Con::evaluate(mValidateCommand, false);
    }

    if (isMethod("onValidate"))
        Con::executef(this, 2, "onValidate");

    // Redraw the control:
    setUpdate();
}


void GuiTextEditCtrl::parentResized(const Point2I& oldParentExtent, const Point2I& newParentExtent)
{
    Parent::parentResized(oldParentExtent, newParentExtent);
    mTextOffsetReset = true;
}

void GuiTextEditCtrl::onRender(Point2I offset, const RectI& /*updateRect*/)
{
    RectI ctrlRect(offset, mBounds.extent);

    //if opaque, fill the update rect with the fill color
    if (mProfile->mOpaque)
        GFX->drawRectFill(ctrlRect, mProfile->mFillColor);


    //if there's a border, draw the border
    if (mProfile->mBorder)
        renderBorder(ctrlRect, mProfile);

    drawText(ctrlRect, isFirstResponder());

}

void GuiTextEditCtrl::onPreRender()
{
    if (isFirstResponder())
    {
        U32 timeElapsed = Platform::getVirtualMilliseconds() - mTimeLastCursorFlipped;
        mNumFramesElapsed++;
        if ((timeElapsed > 500) && (mNumFramesElapsed > 3))
        {
            mCursorOn = !mCursorOn;
            mTimeLastCursorFlipped = Platform::getVirtualMilliseconds();
            mNumFramesElapsed = 0;
            setUpdate();
        }

        //update the cursor if the text is scrolling
        if (mDragHit)
        {
            if ((mScrollDir < 0) && (mCursorPos > 0))
                mCursorPos--;
            else if ((mScrollDir > 0) && (mCursorPos < (S32)mTextBuffer.length()))
                mCursorPos++;
        }
    }
}

void GuiTextEditCtrl::drawText(const RectI& drawRect, bool isFocused)
{
    StringBuffer textBuffer;
    Point2I drawPoint = drawRect.point;
    Point2I paddingLeftTop, paddingRightBottom;

    // Just a little sanity.
    if (mCursorPos > mTextBuffer.length())
        mCursorPos = mTextBuffer.length();

    // Apply password masking (make the masking char optional perhaps?)
    if (mPasswordText)
    {
        for (U32 i = 0; i < mTextBuffer.length(); i++)
            textBuffer.append(StringBuffer(mPasswordMask));
    }
    else
    {
        // Or else just copy it over.
        textBuffer.set(&mTextBuffer);
    }

    paddingLeftTop.set((mProfile->mTextOffset.x != 0 ? mProfile->mTextOffset.x : 3), mProfile->mTextOffset.y);
    paddingRightBottom = paddingLeftTop;

    // Center vertically:
    drawPoint.y += ((drawRect.extent.y - paddingLeftTop.y - paddingRightBottom.y - mFont->getHeight()) / 2) + paddingLeftTop.y;

    // Align horizontally:

    S32 textWidth;
    {
        FrameTemp<UTF8> widthTemp(textBuffer.length() * 3 + 1);
        textBuffer.get(widthTemp, textBuffer.length() * 3 + 1);
        textWidth = mFont->getStrWidth(widthTemp);
    }

    if (drawRect.extent.x - paddingLeftTop.x > textWidth)
    {
        switch (mProfile->mAlignment)
        {
        case GuiControlProfile::RightJustify:
            drawPoint.x += (drawRect.extent.x - textWidth - paddingRightBottom.x);
            break;
        case GuiControlProfile::CenterJustify:
            drawPoint.x += ((drawRect.extent.x - textWidth) / 2);
            break;
        default:
        case GuiControlProfile::LeftJustify:
            drawPoint.x += paddingLeftTop.x;
            break;
        }
    }
    else
        drawPoint.x += paddingLeftTop.x;

    ColorI fontColor = mActive ? mProfile->mFontColor : mProfile->mFontColorNA;

    // now draw the text
    Point2I cursorStart, cursorEnd;
    mTextOffset.y = drawPoint.y;
    if (mTextOffsetReset)
    {
        mTextOffset.x = drawPoint.x;
        mTextOffsetReset = false;
    }

    if (drawRect.extent.x - paddingLeftTop.x > textWidth)
        mTextOffset.x = drawPoint.x;
    else
    {
        // Alignment affects large text
        if (mProfile->mAlignment == GuiControlProfile::RightJustify
            || mProfile->mAlignment == GuiControlProfile::CenterJustify)
        {
            if (mTextOffset.x + textWidth < (drawRect.point.x + drawRect.extent.x) - paddingRightBottom.x)
                mTextOffset.x = (drawRect.point.x + drawRect.extent.x) - paddingRightBottom.x - textWidth;
        }
    }

    // calculate the cursor
    if (isFocused)
    {
        // Where in the string are we?
        StringBuffer preCursor = textBuffer.substring(0, mCursorPos);
        S32 cursorOffset = 0, charWidth = 0;
        UTF16 tempChar = mTextBuffer.getChar(mCursorPos);

        // Alright, we want to terminate things momentarily.
        if (mCursorPos > 0)
        {
            FrameTemp<UTF8> cursorTemp(preCursor.length() * 3 + 1);
            preCursor.get(cursorTemp, preCursor.length() * 3 + 1);
            cursorOffset = mFont->getStrWidth(cursorTemp);
        }
        else
            cursorOffset = 0;

        if (tempChar)
            charWidth = mFont->getCharWidth(tempChar);
        else
            charWidth = paddingRightBottom.x;

        if (mTextOffset.x + cursorOffset + charWidth >= (drawRect.point.x + drawRect.extent.x) - paddingLeftTop.x)
        {
            // Cursor somewhere beyond the textcontrol,
            // skip forward roughly 25% of the total width (if possible)
            S32 skipForward = drawRect.extent.x / 4;

            if (cursorOffset + skipForward > textWidth)
                mTextOffset.x = (drawRect.point.x + drawRect.extent.x) - paddingRightBottom.x - textWidth;
            else
                mTextOffset.x -= skipForward;
        }
        else if (mTextOffset.x + cursorOffset < drawRect.point.x + paddingLeftTop.x)
        {
            // Cursor somewhere before the textcontrol
            // skip backward roughly 25% of the total width (if possible)
            S32 skipBackward = drawRect.extent.x / 4;

            if (cursorOffset - skipBackward < 0)
                mTextOffset.x = drawRect.point.x + paddingLeftTop.x;
            else
                mTextOffset.x += skipBackward;
        }
        cursorStart.x = mTextOffset.x + cursorOffset;
        cursorEnd.x = cursorStart.x;

        S32 cursorHeight = mFont->getHeight();
        if (cursorHeight < drawRect.extent.y)
        {
            cursorStart.y = drawPoint.y;
            cursorEnd.y = cursorStart.y + cursorHeight;
        }
        else
        {
            cursorStart.y = drawRect.point.y;
            cursorEnd.y = cursorStart.y + drawRect.extent.y;
        }
    }

    //draw the text
    if (!isFocused)
        mBlockStart = mBlockEnd = 0;

    //also verify the block start/end
    if ((mBlockStart > textBuffer.length() || (mBlockEnd > textBuffer.length()) || (mBlockStart > mBlockEnd)))
        mBlockStart = mBlockEnd = 0;

    UTF16 temp;

    Point2I tempOffset = mTextOffset;

    //draw the portion before the highlight
    if (mBlockStart > 0)
    {
        StringBuffer preBuffer = textBuffer.substring(0, mBlockStart);
        FrameTemp<UTF8> preString(preBuffer.length() * 3 + 1);
        preBuffer.get(preString, preBuffer.length() * 3 + 1);

        GFX->setBitmapModulation(fontColor);
        GFX->drawText(mFont, tempOffset, preString, mProfile->mFontColors);
        tempOffset.x += mFont->getStrWidth((const UTF8*)preString);
    }

    //draw the hilighted portion
    if (mBlockEnd > 0)
    {
        StringBuffer highlightBuff = mTextBuffer.substring(mBlockStart, mBlockEnd - mBlockStart);

        FrameTemp<UTF8> highlightTemp(highlightBuff.length() * 3 + 1);
        highlightBuff.get(highlightTemp, highlightBuff.length() * 3 + 1);

        S32 highlightWidth = mFont->getStrWidth(highlightTemp);

        GFX->drawRectFill(Point2I(tempOffset.x, drawRect.point.y),
            Point2I(tempOffset.x + highlightWidth, drawRect.point.y + drawRect.extent.y - 1),
            mProfile->mFillColorHL);

        GFX->setBitmapModulation(mProfile->mFontColorHL);
        GFX->drawText(mFont, tempOffset, highlightTemp, mProfile->mFontColors);
        tempOffset.x += highlightWidth;
    }

    //draw the portion after the highlite
    if (mBlockEnd < mTextBuffer.length())
    {
        StringBuffer finalBuff = mTextBuffer.substring(mBlockEnd, mTextBuffer.length() - mBlockEnd);
        FrameTemp<UTF8> finalTemp(finalBuff.length() * 3 + 1);
        finalBuff.get(finalTemp, finalBuff.length() * 3 + 1);

        GFX->setBitmapModulation(fontColor);
        GFX->drawText(mFont, tempOffset, finalTemp, mProfile->mFontColors);
    }

    //draw the cursor
    if (isFocused && mCursorOn)
        GFX->drawLine(cursorStart, cursorEnd, mProfile->mCursorColor);
}

bool GuiTextEditCtrl::hasText()
{
    return (mTextBuffer.length());
}

void GuiTextEditCtrl::playDeniedSound()
{
    if (mDeniedSound)
        SFX->playOnce(mDeniedSound);
}

bool GuiTextEditCtrl::initCursors()
{
    if (mEditCursor == NULL)
    {
        SimObject* obj;
        obj = Sim::findObject("TextEditCursor");
        mEditCursor = dynamic_cast<GuiCursor*>(obj);

        return(mEditCursor != NULL);
    }
    else
        return(true);
}


void GuiTextEditCtrl::getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent)
{
    showCursor = true;

    if (initCursors())
    {
        Point2I mousePos = lastGuiEvent.mousePoint;
        RectI winRect = mBounds;
        winRect.point = localToGlobalCoord(winRect.point);

        if (winRect.pointInRect(mousePos))
            cursor = mEditCursor;
        else
            cursor = NULL;
    }
}

const char* GuiTextEditCtrl::getScriptValue()
{
    FrameTemp<UTF8> temp(mTextBuffer.length() * 3 + 1);
    mTextBuffer.get(temp, mTextBuffer.length() * 3 + 1);
    return StringTable->insert((const char*)(UTF8*)temp);
}

void GuiTextEditCtrl::setScriptValue(const char* value)
{
    mTextBuffer.set(value);
    mCursorPos = mTextBuffer.length() - 1;
}

ConsoleMethod(GuiTextEditCtrl, getText, const char*, 2, 2, "textEditCtrl.getText()")
{
    argc; argv;
    if (!object->hasText())
        return NULL;

    char* retBuffer = Con::getReturnBuffer(GuiTextEditCtrl::MAX_STRING_LENGTH);
    object->getText(retBuffer);

    if (retBuffer && dStrstr((const char*)retBuffer, "foo"))
        U32 i = 0;

    return retBuffer;
}


ConsoleMethod(GuiTextEditCtrl, getCursorPos, S32, 2, 2, "textEditCtrl.getCursorPos()")
{
    argc; argv;
    return(object->getCursorPos());
}

ConsoleMethod(GuiTextEditCtrl, setCursorPos, void, 3, 3, "textEditCtrl.setCursorPos( newPos )")
{
    argc;
    object->reallySetCursorPos(dAtoi(argv[2]));
}

ConsoleMethod(GuiTextEditCtrl, selectAllText, void, 2, 2, "textEditCtrl.selectAllText()")
{
    argc; argv;
    object->selectAllText();
}

ConsoleMethod(GuiTextEditCtrl, forceValidateText, void, 2, 2, "textEditCtrl.forceValidateText()")
{
    argc; argv;
    object->forceValidateText();
}
