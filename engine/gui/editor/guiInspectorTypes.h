//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GUI_INSPECTOR_TYPES_H_
#define _GUI_INSPECTOR_TYPES_H_

#ifndef _GUI_INSPECTOR_H_
#include "gui/editor/guiInspector.h"
#endif

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#ifndef _H_GUIDEFAULTCONTROLRENDER_
#include "gui/core/guiDefaultControlRender.h"
#endif

#ifndef _GUISCROLLCTRL_H_
#include "gui/containers/guiScrollCtrl.h"
#endif

#ifndef _GUITEXTEDITCTRL_H_
#include "gui/controls/guiTextEditCtrl.h"
#endif

#ifndef _GUIPOPUPCTRL_H_
#include "gui/controls/guiPopUpCtrl.h"
#endif

#ifndef _GUICHECKBOXCTRL_H_
#include "gui/controls/guiCheckBoxCtrl.h"
#endif


//////////////////////////////////////////////////////////////////////////
// TypeEnum GuiInspectorField Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeEnum : public GuiInspectorField
{
private:
   typedef GuiInspectorField Parent;
public:
   DECLARE_CONOBJECT(GuiInspectorTypeEnum);
   static void consoleInit();

   //////////////////////////////////////////////////////////////////////////
   // Override able methods for custom edit fields
   //////////////////////////////////////////////////////////////////////////
   virtual GuiControl*        constructEditControl();
   virtual void               setData( StringTableEntry data );
   virtual StringTableEntry   getData();
   virtual void               updateValue( StringTableEntry newValue );
};


//////////////////////////////////////////////////////////////////////////
// GuiInspectorTypeCheckBox Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeCheckBox : public GuiInspectorField
{
private:
   typedef GuiInspectorField Parent;
public:
   DECLARE_CONOBJECT(GuiInspectorTypeCheckBox);
   static void consoleInit();

   //////////////////////////////////////////////////////////////////////////
   // Override able methods for custom edit fields (Both are REQUIRED)
   //////////////////////////////////////////////////////////////////////////
   virtual GuiControl* constructEditControl();
};

//////////////////////////////////////////////////////////////////////////
// GuiInspectorTypeGuiProfile Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeGuiProfile : public GuiInspectorTypeEnum
{
private:
   typedef GuiInspectorTypeEnum Parent;
public:
   DECLARE_CONOBJECT(GuiInspectorTypeGuiProfile);
   static void consoleInit();

   //////////////////////////////////////////////////////////////////////////
   // Override able methods for custom edit fields (Both are REQUIRED)
   //////////////////////////////////////////////////////////////////////////
   virtual GuiControl* constructEditControl();
};


//////////////////////////////////////////////////////////////////////////
// TypeFileName GuiInspectorField Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeFileName : public GuiInspectorField
{
private:
   typedef GuiInspectorField Parent;
public:
   DECLARE_CONOBJECT(GuiInspectorTypeFileName);
   static void consoleInit();

   SimObjectPtr<GuiButtonCtrl>   mBrowseButton;

   //////////////////////////////////////////////////////////////////////////
   // Override able methods for custom edit fields
   //////////////////////////////////////////////////////////////////////////
   virtual GuiControl*        constructEditControl();
   virtual void               resize(const Point2I &newPosition, const Point2I &newExtent);
};


//////////////////////////////////////////////////////////////////////////
// TypeColor GuiInspectorField Class (Base for ColorI/ColorF)
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeColor : public GuiInspectorField
{
private:
   typedef GuiInspectorField Parent;
public:
   DECLARE_CONOBJECT(GuiInspectorTypeColor);

   StringTableEntry  mColorFunction;
   SimObjectPtr<GuiButtonCtrl>   mBrowseButton;

   //////////////////////////////////////////////////////////////////////////
   // Override able methods for custom edit fields
   //////////////////////////////////////////////////////////////////////////
   virtual GuiControl*        constructEditControl();
   virtual void               resize(const Point2I &newPosition, const Point2I &newExtent);
};

//////////////////////////////////////////////////////////////////////////
// TypeColorI GuiInspectorField Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeColorI : public GuiInspectorTypeColor
{
private:
   typedef GuiInspectorTypeColor Parent;
public:
   GuiInspectorTypeColorI();

   DECLARE_CONOBJECT(GuiInspectorTypeColorI);
   static void consoleInit();
};

//////////////////////////////////////////////////////////////////////////
// TypeColorF GuiInspectorField Class
//////////////////////////////////////////////////////////////////////////
class GuiInspectorTypeColorF : public GuiInspectorTypeColor
{
private:
   typedef GuiInspectorTypeColor Parent;
public:
   GuiInspectorTypeColorF();

   DECLARE_CONOBJECT(GuiInspectorTypeColorF);
   static void consoleInit();
};

#endif