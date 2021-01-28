//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIFILTERCTRL_H_
#define _GUIFILTERCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif



//--------------------------------------
// helper class
class Filter: public Vector<F32>
{
public:
   Filter() : Vector<F32>(__FILE__, __LINE__) { }

   void set(S32 argc, const char *argv[]);
   F32  getValue(F32 t) const;
};


//--------------------------------------
class GuiFilterCtrl : public GuiControl
{
  private:
   typedef GuiControl Parent;

   S32 mControlPointRequest;
   S32 mCurKnot;
   Filter mFilter;

  public:
   //creation methods
   DECLARE_CONOBJECT(GuiFilterCtrl);
   GuiFilterCtrl();
   static void initPersistFields();

   //Parental methods
   bool onWake();

   void onMouseDown(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseUp(const GuiEvent &);

   F32  getValue(S32 n);
   const Filter* get() { return &mFilter; }
   void set(const Filter &f);
   S32  getNumControlPoints() {return mFilter.size(); }
   void identity();

   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect );
};


inline F32 GuiFilterCtrl::getValue(S32 n)
{
   S32 index = getMin(getMax(n,0), (S32)mFilter.size()-1);
   return mFilter[n];
}


inline void GuiFilterCtrl::set(const Filter &f)
{
   mControlPointRequest = f.size();
   mFilter = f;
}

#endif
