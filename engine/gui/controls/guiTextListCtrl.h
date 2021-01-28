//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTLISTCTRL_H_
#define _GUITEXTLISTCTRL_H_

#ifndef _GUIARRAYCTRL_H_
#include "gui/core/guiArrayCtrl.h"
#endif

class GuiTextListCtrl : public GuiArrayCtrl
{
  private:
   typedef GuiArrayCtrl Parent;

  public:
   struct Entry
   {
      char *text;
      U32 id;
      bool active;
   };

   Vector<Entry> mList;

   bool mEnumerate;
   bool mResizeCell;

  protected:
   enum ScrollConst
   {
      UP = 0,
      DOWN = 1
   };
   enum {
      InvalidId = 0xFFFFFFFF
   };
   Vector<S32> mColumnOffsets;

   bool  mFitParentWidth;
   bool  mClipColumnText;

   U32 getRowWidth(Entry *row);
   void onCellSelected(Point2I cell);

  public:
   GuiTextListCtrl();

   DECLARE_CONOBJECT(GuiTextListCtrl);
   static void initPersistFields();

   virtual void setCellSize( const Point2I &size ){ mCellSize = size; }
   virtual void getCellSize(       Point2I &size ){ size = mCellSize; }

   const char *getScriptValue();
   void setScriptValue(const char *value);

   U32 getNumEntries();

   void clear();
   virtual void addEntry(U32 id, const char *text);
   virtual void insertEntry(U32 id, const char *text, S32 index);
   void setEntry(U32 id, const char *text);
   void setEntryActive(U32 id, bool active);
   S32 findEntryById(U32 id);
   S32 findEntryByText(const char *text);
   bool isEntryActive(U32 id);

   U32 getEntryId(U32 index);

   bool onWake();
   void removeEntry(U32 id);
   virtual void removeEntryByIndex(S32 id);
   virtual void sort(U32 column, bool increasing = true);
   virtual void sortNumerical(U32 column, bool increasing = true);

   U32 getSelectedId();
   const char *getSelectedText();

   bool onKeyDown(const GuiEvent &event);

   virtual void onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver);

   void setSize(Point2I newSize);
   void onRemove();
   void addColumnOffset(S32 offset) { mColumnOffsets.push_back(offset); }
   void clearColumnOffsets() { mColumnOffsets.clear(); }
};

#endif //_GUI_TEXTLIST_CTRL_H
