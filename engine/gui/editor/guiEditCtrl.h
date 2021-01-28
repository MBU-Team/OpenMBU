//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIEDITCTRL_H_
#define _GUIEDITCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiEditCtrl : public GuiControl
{
   typedef GuiControl Parent;

   Vector<GuiControl *> mSelectedControls;
   GuiControl*          mCurrentAddSet;
   GuiControl*          mContentControl;
   Point2I              mLastMousePos;
   Point2I              mSelectionAnchor;
   Point2I              mGridSnap;
   Point2I				mDragBeginPoint;
   Vector<Point2I>		mDragBeginPoints;

   // Sizing Cursors
   GuiCursor*        mDefaultCursor;
   GuiCursor*        mLeftRightCursor;
   GuiCursor*        mUpDownCursor;
   GuiCursor*        mNWSECursor;
   GuiCursor*        mNESWCursor;
   GuiCursor*        mMoveCursor;

   enum mouseModes { Selecting, MovingSelection, SizingSelection, DragSelecting };
   enum sizingModes { sizingNone = 0, sizingLeft = 1, sizingRight = 2, sizingTop = 4, sizingBottom = 8 };

   mouseModes             mMouseDownMode;
   sizingModes             mSizingMode;

  public:
   GuiEditCtrl();
   DECLARE_CONOBJECT(GuiEditCtrl);

   bool onWake();
   void onSleep();

   void select(GuiControl *ctrl);
   void setRoot(GuiControl *ctrl);
   void setEditMode(bool value);
   S32 getSizingHitKnobs(const Point2I &pt, const RectI &box);
   void getDragRect(RectI &b);
   void drawNut(const Point2I &nut, ColorI &outlineColor, ColorI &nutColor);
   void drawNuts(RectI &box, ColorI &outlineColor, ColorI &nutColor);
   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect);
   void addNewControl(GuiControl *ctrl);
   bool selectionContains(GuiControl *ctrl);
   void setCurrentAddSet(GuiControl *ctrl);
   void setSelection(GuiControl *ctrl, bool inclusive = false);

   // Sizing Cursors
   bool initCursors();
   void getCursor(GuiCursor *&cursor, bool &showCursor, const GuiEvent &lastGuiEvent);


   const Vector<GuiControl *> *getSelected() const { return &mSelectedControls; }
   const GuiControl *getAddSet() const { return mCurrentAddSet; }; //JDD

   bool onKeyDown(const GuiEvent &event);
   void onMouseDown(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onRightMouseDown(const GuiEvent &event);

   enum Justification {
      JUSTIFY_LEFT,
      JUSTIFY_CENTER,
      JUSTIFY_RIGHT,
      JUSTIFY_TOP,
      JUSTIFY_BOTTOM,
      SPACING_VERTICAL,
      SPACING_HORIZONTAL
   };

   void justifySelection( Justification j);
   void moveSelection(const Point2I &delta);
   void saveSelection(const char *filename);
   void loadSelection(const char *filename);
   void addSelection(S32 id);
   void removeSelection(S32 id);
   void deleteSelection();
   void clearSelection();
   void selectAll();
   void bringToFront();
   void pushToBack();
};

#endif //_GUI_EDIT_CTRL_H
