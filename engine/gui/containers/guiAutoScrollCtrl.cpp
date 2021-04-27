//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/containers/guiAutoScrollCtrl.h"
#include "console/consoleTypes.h"

//-----------------------------------------------------------------------------
// GuiAutoScrollCtrl
//-----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiAutoScrollCtrl);

GuiAutoScrollCtrl::GuiAutoScrollCtrl()
{
   mScrolling = false;
   mCurrentTime = 0.0f;
   mStartDelay = 3.0f;
   mResetDelay = 5.0f;
   mChildBorder = 10;
   mScrollInterval = 1.0f;
   mTickCallback = false;
   //mIsContainer = true;

   // Make sure we receive our ticks.
   setProcessTicks();
}

GuiAutoScrollCtrl::~GuiAutoScrollCtrl()
{
}

//-----------------------------------------------------------------------------
// Persistence 
//-----------------------------------------------------------------------------
void GuiAutoScrollCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("startDelay", TypeF32, Offset(mStartDelay, GuiAutoScrollCtrl));
   addField("resetDelay", TypeF32, Offset(mResetDelay, GuiAutoScrollCtrl));
   addField("childBorder", TypeS32, Offset(mChildBorder, GuiAutoScrollCtrl));
   addField("scrollInterval", TypeF32, Offset(mScrollInterval, GuiAutoScrollCtrl));
   addField("tickCallback", TypeBool, Offset(mTickCallback, GuiAutoScrollCtrl));
}

void GuiAutoScrollCtrl::onChildAdded(GuiControl* control)
{
   resetChild(control);
}

void GuiAutoScrollCtrl::onChildRemoved(GuiControl* control)
{
   mScrolling = false;
}

void GuiAutoScrollCtrl::resetChild(GuiControl* control)
{
   Point2I extent = control->getExtent();

   control->setPosition(Point2I(mChildBorder, mChildBorder));
   control->setExtent(Point2I(getExtent().x - (mChildBorder * 2), extent.y));

   mControlPositionY = F32(control->getPosition().y);

   if ((mControlPositionY + extent.y) > getExtent().y)
      mScrolling = true;
   else
      mScrolling = false;

   mCurrentTime = 0.0f;
}

void GuiAutoScrollCtrl::resize( const Point2I &newPosition, const Point2I &newExtent )
{
   Parent::resize( newPosition, newExtent );

   for (iterator i = begin(); i != end(); i++)
   {
      GuiControl* control = static_cast<GuiControl*>(*i);
      if (control)
         resetChild(control);
   }
}

void GuiAutoScrollCtrl::childResized(GuiControl *child)
{
   Parent::childResized( child );

   resetChild(child);
}

void GuiAutoScrollCtrl::processTick()
{
   if (mTickCallback && isMethod("onTick") )
      Con::executef(this, 1, "onTick");
}

void GuiAutoScrollCtrl::advanceTime(F32 timeDelta)
{
   if (!mScrolling)
      return;

   if ((mCurrentTime + timeDelta) < mStartDelay)
   {
      mCurrentTime += timeDelta;
      return;
   }

   GuiControl* control = static_cast<GuiControl*>(objectList.at(0));
   if (!control)
      return;

   Point2I oldPosition = control->getPosition();
   if ((oldPosition.y + control->getExtent().y) < (getExtent().y - mChildBorder))
   {
      mCurrentTime += timeDelta;
      if (mCurrentTime > (mStartDelay + mResetDelay))
      {
         resetChild(control);
         return;
      }
   }

   else
   {
      mControlPositionY -= mScrollInterval * timeDelta;
      control->setPosition(Point2I(oldPosition.x, (S32)mControlPositionY));
   }
}
