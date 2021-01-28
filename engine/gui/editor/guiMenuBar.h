//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIMENUBAR_H_
#define _GUIMENUBAR_H_

#ifndef _GUITEXTLISTCTRL_H_
#include "gui/guiTextListCtrl.h"
#endif

class GuiMenuBar;
class GuiMenuTextListCtrl;

class GuiMenuBackgroundCtrl : public GuiControl
{
protected:
   GuiMenuBar *mMenuBarCtrl;
   GuiMenuTextListCtrl *mTextList;
public:
   GuiMenuBackgroundCtrl(GuiMenuBar *ctrl, GuiMenuTextListCtrl* textList);
   void onMouseDown(const GuiEvent &event);
   void onMouseMove(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
};

//------------------------------------------------------------------------------

class GuiMenuTextListCtrl : public GuiTextListCtrl
{
   private:
      typedef GuiTextListCtrl Parent;

   protected:
      GuiMenuBar *mMenuBarCtrl;

   public:
      GuiMenuTextListCtrl(); // for inheritance
      GuiMenuTextListCtrl(GuiMenuBar *ctrl);

      // GuiControl overloads:
      bool onKeyDown(const GuiEvent &event);
		void onMouseDown(const GuiEvent &event);
      void onMouseUp(const GuiEvent &event);
      void onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver);
};

//------------------------------------------------------------------------------

class GuiMenuBar : public GuiControl
{
   typedef GuiControl Parent;
public:

	struct MenuItem   // an individual item in a pull-down menu
	{
		char *text;    // the text of the menu item
		U32 id;        // a script-assigned identifier
		char *accelerator; // the keyboard accelerator shortcut for the menu item
      U32 acceleratorIndex; // index of this accelerator
		bool enabled;        // true if the menu item is selectable
      bool visible;        // true if the menu item is visible
      S32 bitmapIndex;     // index of the bitmap in the bitmap array
      S32 checkGroup;      // the group index of the item visa vi check marks -
                           // only one item in the group can be checked.
		MenuItem *nextMenuItem; // next menu item in the linked list
	};
	struct Menu
	{
		char *text;
		U32 id;
		RectI bounds;
      bool visible;

		Menu *nextMenu;
		MenuItem *firstMenuItem;
	};
	
	GuiMenuBackgroundCtrl *mBackground;
	GuiMenuTextListCtrl *mTextList;

	Menu *menuList;
   Menu *mouseDownMenu;
   Menu *mouseOverMenu;

   bool menuBarDirty;
   U32 mCurAcceleratorIndex;
   Point2I maxBitmapSize;
	
	GuiMenuBar();
   bool onWake();
   void onSleep();

	// internal menu handling functions
	// these are used by the script manipulation functions to add/remove/change menu items

   void addMenu(const char *menuText, U32 menuId);
	Menu *findMenu(const char *menu);  // takes either a menu text or a string id
	MenuItem *findMenuItem(Menu *menu, const char *menuItem); // takes either a menu text or a string id
	void removeMenu(Menu *menu);
	void removeMenuItem(Menu *menu, MenuItem *menuItem);
	void addMenuItem(Menu *menu, const char *text, U32 id, const char *accelerator, S32 checkGroup);
	void clearMenuItems(Menu *menu);
   void clearMenus();

	// display/mouse functions

	Menu *findHitMenu(Point2I mousePoint);

   void onPreRender();
	void onRender(Point2I offset, const RectI &updateRect);

   void checkMenuMouseMove(const GuiEvent &event);
   void onMouseMove(const GuiEvent &event);
   void onMouseLeave(const GuiEvent &event);
   void onMouseDown(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);

   void onAction();
   void closeMenu();
   void buildAcceleratorMap();
   void acceleratorKeyPress(U32 index);

   void menuItemSelected(Menu *menu, MenuItem *item);

   DECLARE_CONOBJECT(GuiMenuBar);
};

#endif
