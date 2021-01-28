//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIARRAYCTRL_H_
#define _GUIARRAYCTRL_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUITEXTCTRL_H_
#include "gui/controls/guiTextCtrl.h"
#endif

/// Renders a grid of cells.
class GuiArrayCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:

   Point2I mHeaderDim;
   Point2I mSize;
   Point2I mCellSize;
   Point2I mSelectedCell;
   Point2I mMouseOverCell;

   Resource<GFont> mFont;

   bool cellSelected(Point2I cell);
   virtual void onCellSelected(Point2I cell);
public:

   GuiArrayCtrl();
   DECLARE_CONOBJECT(GuiArrayCtrl);

   bool onWake();
   void onSleep();

   /// @name Array attribute methods
   /// @{
   Point2I getSize() { return mSize; }
   virtual void setSize(Point2I size);
   void setHeaderDim(const Point2I &dim) { mHeaderDim = dim; }
   void getScrollDimensions(S32 &cell_size, S32 &num_cells);
   /// @}

   /// @name Selected cell methods
   /// @{
   void setSelectedCell(Point2I cell);
   void deselectCells() { mSelectedCell.set(-1,-1); }
   Point2I getSelectedCell();
   void scrollSelectionVisible();
   void scrollCellVisible(Point2I cell);
   /// @}

   /// @name Rendering methods
   /// @{
   virtual void onRenderColumnHeaders(Point2I offset, Point2I parentOffset, Point2I headerDim);
   virtual void onRenderRowHeader(Point2I offset, Point2I parentOffset, Point2I headerDim, Point2I cell);
   virtual void onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver);
   void onRender(Point2I offset, const RectI &updateRect);
   /// @}

   /// @name Mouse input methods
   /// @{
   void onMouseDown(const GuiEvent &event);
   void onMouseMove(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseEnter(const GuiEvent &event);
   void onMouseLeave(const GuiEvent &event);
   bool onKeyDown(const GuiEvent &event);
   void onRightMouseDown(const GuiEvent &event);
   /// @}
};

#endif //_GUI_ARRAY_CTRL_H
