//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GUIAUTOSCROLLCTRL_H_
#define _GUIAUTOSCROLLCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#ifndef _GUITICKCTRL_H_
#include "gui/shiny/guiTickCtrl.h"
#endif

class GuiAutoScrollCtrl : public GuiTickCtrl
{
private:
   typedef GuiTickCtrl Parent;
   bool mScrolling;
   F32 mCurrentTime;
   F32 mStartDelay;
   F32 mResetDelay;
   S32 mChildBorder;
   F32 mScrollInterval;
   bool mTickCallback;

   F32 mControlPositionY;

   void resetChild(GuiControl* control);

public:
   // Constructor/Destructor/Conobject Declaration
   GuiAutoScrollCtrl();
   ~GuiAutoScrollCtrl();
   DECLARE_CONOBJECT(GuiAutoScrollCtrl);

   // Persistence
   static void initPersistFields();

   // Control Events
   virtual void onChildAdded(GuiControl* control);
   virtual void onChildRemoved(GuiControl* control);
   virtual void resize(const Point2I& newPosition, const Point2I& newExtent);
   virtual void childResized(GuiControl *child);

   virtual void processTick();
   virtual void advanceTime(F32 timeDelta);
};
#endif