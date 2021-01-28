//-----------------------------------------------------------------------------
// Torque Engine
//
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBUTTONBASECTRL_H_
#define _GUIBUTTONBASECTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

// all the button functionality is moving into buttonBase
// thus, all subclasses will just be rendering classes,
// and radios and check boxes can be done using pushbuttons
// or bitmap buttons.

class GuiButtonBaseCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   StringTableEntry mButtonText;
   StringTableEntry mButtonTextID;
   bool mDepressed;
   bool mMouseOver;
   bool mStateOn;
   S32 mButtonType;
   S32 mRadioGroup;
public:
   enum {
      ButtonTypePush,
      ButtonTypeCheck,
      ButtonTypeRadio,
   };

   GuiButtonBaseCtrl();
   bool onWake();

   DECLARE_CONOBJECT(GuiButtonBaseCtrl);
   static void initPersistFields();

   void setText(const char *text);
   void setTextID(S32 id);
   void setTextID(const char *id);
   const char *getText();

   void acceleratorKeyPress(U32 index);
   void acceleratorKeyRelease(U32 index);

   void onMouseDown(const GuiEvent &);
   void onMouseUp(const GuiEvent &);

   void onMouseEnter(const GuiEvent &);
   void onMouseLeave(const GuiEvent &);

   bool onKeyDown(const GuiEvent &event);
   bool onKeyUp(const GuiEvent &event);

   void setScriptValue(const char *value);
   const char *getScriptValue();

   void onMessage(GuiControl *,S32 msg);
   void onAction();

};

#endif
