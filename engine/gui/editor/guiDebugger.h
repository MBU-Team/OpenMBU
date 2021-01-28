//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIDEBUGGER_H_
#define _GUIDEBUGGER_H_

#ifndef _GUIARRAYCTRL_H_
#include "gui/core/guiArrayCtrl.h"
#endif

class DbgFileView : public GuiArrayCtrl
{
  private:

   typedef GuiArrayCtrl Parent;

   struct FileLine
   {
      bool breakPosition;
      bool breakOnLine;
      char *text;
   };

   Vector<FileLine> mFileView;

   StringTableEntry mFileName;

   void AdjustCellSize();

   //used to display the program counter
   StringTableEntry mPCFileName;
   S32 mPCCurrentLine;

   //vars used to highlight the selected line segment for copying
   bool mbMouseDragging;
   S32 mMouseDownChar;
   S32 mBlockStart;
   S32 mBlockEnd;

   char mMouseOverVariable[256];
   char mMouseOverValue[256];
   S32 findMouseOverChar(const char *text, S32 stringPosition);
   bool findMouseOverVariable();
   S32 mMouseVarStart;
   S32 mMouseVarEnd;

   //find vars
   char mFindString[256];
   S32 mFindLineNumber;

  public:

   DbgFileView();
   ~DbgFileView();
   DECLARE_CONOBJECT(DbgFileView);

   bool onWake();

   void clear();
   void clearBreakPositions();

   void setCurrentLine(S32 lineNumber, bool setCurrentLine);
   const char *getCurrentLine(S32 &lineNumber);
   bool openFile(const char *fileName);
   void scrollToLine(S32 lineNumber);
   void setBreakPointStatus(U32 lineNumber, bool value);
   void setBreakPosition(U32 line);
   void addLine(const char *text, U32 textLen);

   bool findString(const char *text);

   void onMouseDown(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);

   void onPreRender();
   void onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver);
};

#endif //_GUI_DEBUGGER_H
