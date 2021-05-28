//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTEDITCTRL_H_
#define _GUITEXTEDITCTRL_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif
#ifndef _STRINGBUFFER_H_
#include "core/stringBuffer.h"
#endif

class GuiTextEditCtrl : public GuiTextCtrl
{
private:
    typedef GuiTextCtrl Parent;

    static U32 smNumAwake;

protected:

    StringBuffer mTextBuffer;

    StringTableEntry mValidateCommand;
    StringTableEntry mEscapeCommand;
    SFXProfile* mDeniedSound;

    // for animating the cursor
    S32      mNumFramesElapsed;
    U32      mTimeLastCursorFlipped;
    ColorI   mCursorColor;
    bool     mCursorOn;

    //Edit Cursor
    GuiCursor* mEditCursor;

    bool     mInsertOn;
    S32      mMouseDragStart;
    Point2I  mTextOffset;
    bool     mTextOffsetReset;
    bool     mDragHit;
    bool     mTabComplete;
    S32      mScrollDir;

    //undo members
    StringBuffer mUndoText;
    S32      mUndoBlockStart;
    S32      mUndoBlockEnd;
    S32      mUndoCursorPos;
    void saveUndoState();

    S32      mBlockStart;
    S32      mBlockEnd;
    S32      mCursorPos;
    S32 setCursorPos(const Point2I& offset);

    bool                 mHistoryDirty;
    S32                  mHistoryLast;
    S32                  mHistoryIndex;
    S32                  mHistorySize;
    bool                 mPasswordText;
    StringTableEntry     mPasswordMask;

    bool    mSinkAllKeyEvents;   // any non-ESC key is handled here or not at all
    UTF8** mHistoryBuf;
    void updateHistory(StringBuffer* txt, bool moveIndex);

    void playDeniedSound();
    void execConsoleCallback();

public:
    GuiTextEditCtrl();
    ~GuiTextEditCtrl();
    DECLARE_CONOBJECT(GuiTextEditCtrl);
    static void initPersistFields();

    bool onAdd();
    bool onWake();
    void onSleep();

    void getText(char* dest);  // dest must be of size
                               // StructDes::MAX_STRING_LEN + 1
    bool initCursors();
    void getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent);

    void setText(S32 tag);
    virtual void setText(const char* txt);
    S32   getCursorPos() { return(mCursorPos); }
    void  reallySetCursorPos(const S32 newPos);

    void selectAllText(); //*** DAW: Added
    void forceValidateText(); //*** DAW: Added
    const char* getScriptValue();
    void setScriptValue(const char* value);

    bool onKeyDown(const GuiEvent& event);
    void onMouseDown(const GuiEvent& event);
    void onMouseDragged(const GuiEvent& event);
    void onMouseUp(const GuiEvent& event);

    void onCopy(bool andCut);
    void onPaste();
    void onUndo();

    virtual void setFirstResponder();
    virtual void onLoseFirstResponder();

    void parentResized(const Point2I& oldParentExtent, const Point2I& newParentExtent);
    bool hasText();

    void onStaticModified(const char* slotName);

    void onPreRender();
    void onRender(Point2I offset, const RectI& updateRect);
    virtual void drawText(const RectI& drawRect, bool isFocused);
};

#endif //_GUI_TEXTEDIT_CTRL_H
