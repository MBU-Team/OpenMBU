//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTEDITSLIDERCTRL_H_
#define _GUITEXTEDITSLIDERCTRL_H_

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUITEXTEDITCTRL_H_
#include "gui/controls/guiTextEditCtrl.h"
#endif

class GuiTextEditSliderCtrl : public GuiTextEditCtrl
{
private:
   typedef GuiTextEditCtrl Parent;
   Point2F mRange;
   F32 mIncAmount;
   F32 mValue;
   F32 mIncCounter;
   F32 mMulInc;
   StringTableEntry mFormat;
   U32 mMouseDownTime;
   // max string len, must be less then or equal to 255
public:
   enum CtrlArea
   {
      None,
      Slider,
      ArrowUp,
      ArrowDown
   };
private:
   CtrlArea mTextAreaHit;
public:
   GuiTextEditSliderCtrl();
   ~GuiTextEditSliderCtrl();
   DECLARE_CONOBJECT(GuiTextEditSliderCtrl);
   static void initPersistFields();

   void getText(char *dest);  // dest must be of size
                                      // StructDes::MAX_STRING_LEN + 1

   void setText(S32 tag);
   void setText(const char *txt);

   void setValue();
   void checkRange();
   void checkIncValue();
   void timeInc(U32 elapseTime);

   bool onKeyDown(const GuiEvent &event);
   void onMouseDown(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);


   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect);


};

#endif //_GUI_TEXTEDIT_CTRL_H
