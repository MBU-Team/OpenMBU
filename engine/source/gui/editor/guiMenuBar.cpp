//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiDefaultControlRender.h"
#include "gui/controls/guiTextListCtrl.h"
#include "sim/actionMap.h"
#include "gui/editor/guiMenuBar.h"
#include "gfx/gfxDevice.h"

// menu bar:
// basic idea - fixed height control bar at the top of a window, placed and sized in gui editor
// menu text for menus or menu items should not begin with a digit
// all menus can be removed via the clearMenus() console command
// each menu is added via the addMenu(menuText, menuId) console command
// each menu is added with a menu id
// menu items are added to menus via that addMenuItem(menu, menuItemText, menuItemId, accelerator, checkGroup) console command
// each menu item is added with a menu item id and an optional accelerator
// menu items are initially enabled, but can be disabled/re-enabled via the setMenuItemEnable(menu,menuItem,bool)
// menu text can be set via the setMenuText(menu, newMenuText) console method
// menu item text can be set via the setMenuItemText console method
// menu items can be removed via the removeMenuItem(menu, menuItem) console command
// menu items can be cleared via the clearMenuItems(menu) console command
// menus can be hidden or shown via the setMenuVisible(menu, bool) console command
// menu items can be hidden or shown via the setMenuItemVisible(menu, menuItem, bool) console command
// menu items can be check'd via the setMenuItemChecked(menu, menuItem, bool) console command
//    if the bool is true, any other items in that menu item's check group become unchecked.
//
// menu items can have a bitmap set on them via the setMenuItemBitmap(menu, menuItem, bitmapIndex)
//    passing -1 for the bitmap index will result in no bitmap being shown
//    the index paramater is an index into the bitmap array of the associated profile
//    this can be used, for example, to display a check next to a selected menu item
//    bitmap indices are actually multiplied by 3 when indexing into the bitmap
//    since bitmaps have normal, selected and disabled states.
//
// menus can be removed via the removeMenu console command
// specification arguments for menus and menu items can be either the id or the text of the menu or menu item
// adding the menu item "-" will add an un-selectable seperator to the menu
// callbacks:
// when a menu is clicked, before it is displayed, the menu calls its onMenuSelect(menuId, menuText) method -
//    this allows the callback to enable/disable menu items, or add menu items in a context-sensitive way
// when a menu item is clicked, the menu removes itself from display, then calls onMenuItemSelect(menuId, menuText, menuItemId, menuItemText)

// the initial implementation does not support:
//    hierarchal menus
//    keyboard accelerators on menu text (i.e. via alt-key combos)

//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiMenuBar);

//------------------------------------------------------------------------------
// console methods
//------------------------------------------------------------------------------

ConsoleMethod(GuiMenuBar, clearMenus, void, 2, 2, "() - clears all the menus from the menu bar.")
{
    object->clearMenus();
}

ConsoleMethod(GuiMenuBar, addMenu, void, 4, 4, "(string menuText, int menuId) - adds a new menu to the menu bar.")
{
    if (dIsdigit(argv[2][0]))
    {
        Con::errorf("Cannot add menu %s (id = %s).  First character of a menu's text cannot be a digit.", argv[2], argv[3]);
        return;
    }
    object->addMenu(argv[2], dAtoi(argv[3]));
}

ConsoleMethod(GuiMenuBar, addMenuItem, void, 5, 7, "(string menu, string menuItemText, int menuItemId, string accelerator = NULL, int checkGroup = -1) - adds a menu item to the specified menu.  The menu argument can be either the text of a menu or its id.")
{
    if (dIsdigit(argv[3][0]))
    {
        Con::errorf("Cannot add menu item %s (id = %s).  First character of a menu item's text cannot be a digit.", argv[3], argv[4]);
        return;
    }
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for addMenuItem.", argv[2]);
        return;
    }
    object->addMenuItem(menu, argv[3], dAtoi(argv[4]), argc == 5 ? "" : argv[5], argc < 7 ? -1 : dAtoi(argv[6]));
}

ConsoleMethod(GuiMenuBar, setMenuItemEnable, void, 5, 5, "(string menu, string menuItem, bool enabled) - sets the menu item to enabled or disabled based on the enable parameter.  The specified menu and menu item can either be text or ids.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuItemEnable.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for setMenuItemEnable.", argv[3]);
        return;
    }
    menuItem->enabled = dAtob(argv[4]);
}

ConsoleMethod(GuiMenuBar, setMenuItemChecked, void, 5, 5, "(string menu, string menuItem, bool checked) - sets the menu item bitmap to a check mark, which must be the first element in the bitmap array.  Any other menu items in the menu with the same check group become unchecked if they are checked.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuItemChecked.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for setMenuItemChecked.", argv[3]);
        return;
    }
    bool checked = dAtob(argv[4]);
    if (checked && menuItem->checkGroup != -1)
    {
        // first, uncheck everything in the group:
        for (GuiMenuBar::MenuItem* itemWalk = menu->firstMenuItem; itemWalk; itemWalk = itemWalk->nextMenuItem)
            if (itemWalk->checkGroup == menuItem->checkGroup && itemWalk->bitmapIndex == 0)
                itemWalk->bitmapIndex = -1;
    }
    menuItem->bitmapIndex = checked ? 0 : -1;
}

ConsoleMethod(GuiMenuBar, setMenuText, void, 4, 4, "(string menu, string newMenuText) - sets the text of the specified menu to the new string.")
{
    if (dIsdigit(argv[3][0]))
    {
        Con::errorf("Cannot name menu %s to %s.  First character of a menu's text cannot be a digit.", argv[2], argv[3]);
        return;
    }
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuText.", argv[2]);
        return;
    }
    dFree(menu->text);
    menu->text = dStrdup(argv[3]);
    object->menuBarDirty = true;
}

ConsoleMethod(GuiMenuBar, setMenuVisible, void, 4, 4, "(string menu, bool visible) - sets the whether or not to display the specified menu.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuVisible.", argv[2]);
        return;
    }
    menu->visible = dAtob(argv[3]);
    object->menuBarDirty = true;
    object->setUpdate();
}

ConsoleMethod(GuiMenuBar, setMenuItemText, void, 5, 5, "(string menu, string menuItem, string newMenuItemText) - sets the text of the specified menu item to the new string.")
{
    if (dIsdigit(argv[4][0]))
    {
        Con::errorf("Cannot name menu item %s to %s.  First character of a menu item's text cannot be a digit.", argv[3], argv[4]);
        return;
    }
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuItemText.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for setMenuItemText.", argv[3]);
        return;
    }
    dFree(menuItem->text);
    menuItem->text = dStrdup(argv[4]);
}

ConsoleMethod(GuiMenuBar, setMenuItemVisible, void, 5, 5, "(string menu, string menuItem, bool isVisible) - sets the specified menu item to be either visible or not.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuItemVisible.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for setMenuItemVisible.", argv[3]);
        return;
    }
    menuItem->visible = dAtob(argv[4]);
}

ConsoleMethod(GuiMenuBar, setMenuItemBitmap, void, 5, 5, "(string menu, string menuItem, int bitmapIndex) - sets the specified menu item bitmap index in the bitmap array.  Setting the item's index to -1 will remove any bitmap.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for setMenuItemBitmap.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for setMenuItemBitmap.", argv[3]);
        return;
    }
    menuItem->bitmapIndex = dAtoi(argv[4]);
}

ConsoleMethod(GuiMenuBar, removeMenuItem, void, 4, 4, "(string menu, string menuItem) - removes the specified menu item from the menu.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for removeMenuItem.", argv[2]);
        return;
    }
    GuiMenuBar::MenuItem* menuItem = object->findMenuItem(menu, argv[3]);
    if (!menuItem)
    {
        Con::errorf("Cannot find menu item %s for removeMenuItem.", argv[3]);
        return;
    }
    object->removeMenuItem(menu, menuItem);
}

ConsoleMethod(GuiMenuBar, clearMenuItems, void, 3, 3, "(string menu) - removes all the menu items from the specified menu.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for clearMenuItems.", argv[2]);
        return;
    }
    object->clearMenuItems(menu);
}

ConsoleMethod(GuiMenuBar, removeMenu, void, 3, 3, "(string menu) - removes the specified menu from the menu bar.")
{
    GuiMenuBar::Menu* menu = object->findMenu(argv[2]);
    if (!menu)
    {
        Con::errorf("Cannot find menu %s for removeMenu.", argv[2]);
        return;
    }
    object->clearMenuItems(menu);
    object->menuBarDirty = true;
}

//------------------------------------------------------------------------------
// menu management methods
//------------------------------------------------------------------------------

void GuiMenuBar::addMenu(const char* menuText, U32 menuId)
{
    // allocate the menu
    Menu* newMenu = new Menu;
    newMenu->text = dStrdup(menuText);
    newMenu->id = menuId;
    newMenu->nextMenu = NULL;
    newMenu->firstMenuItem = NULL;
    newMenu->visible = true;

    // add it to the menu list
    menuBarDirty = true;
    Menu** walk;
    for (walk = &menuList; *walk; walk = &(*walk)->nextMenu)
        ;
    *walk = newMenu;
}

GuiMenuBar::Menu* GuiMenuBar::findMenu(const char* menu)
{
    if (dIsdigit(menu[0]))
    {
        U32 id = dAtoi(menu);
        for (Menu* walk = menuList; walk; walk = walk->nextMenu)
            if (id == walk->id)
                return walk;
        return NULL;
    }
    else
    {
        for (Menu* walk = menuList; walk; walk = walk->nextMenu)
            if (!dStricmp(menu, walk->text))
                return walk;
        return NULL;
    }
}

GuiMenuBar::MenuItem* GuiMenuBar::findMenuItem(Menu* menu, const char* menuItem)
{
    if (dIsdigit(menuItem[0]))
    {
        U32 id = dAtoi(menuItem);
        for (MenuItem* walk = menu->firstMenuItem; walk; walk = walk->nextMenuItem)
            if (id == walk->id)
                return walk;
        return NULL;
    }
    else
    {
        for (MenuItem* walk = menu->firstMenuItem; walk; walk = walk->nextMenuItem)
            if (!dStricmp(menuItem, walk->text))
                return walk;
        return NULL;
    }
}

void GuiMenuBar::removeMenu(Menu* menu)
{
    menuBarDirty = true;
    clearMenuItems(menu);
    for (Menu** walk = &menuList; *walk; walk = &(*walk)->nextMenu)
    {
        if (*walk == menu)
        {
            *walk = menu->nextMenu;
            break;
        }
    }
    dFree(menu->text);
    delete menu;
}

void GuiMenuBar::removeMenuItem(Menu* menu, MenuItem* menuItem)
{
    for (MenuItem** walk = &menu->firstMenuItem; *walk; walk = &(*walk)->nextMenuItem)
    {
        if (*walk == menuItem)
        {
            *walk = menuItem->nextMenuItem;
            break;
        }
    }
    dFree(menuItem->text);
    dFree(menuItem->accelerator);
    delete menuItem;
}

void GuiMenuBar::addMenuItem(Menu* menu, const char* text, U32 id, const char* accelerator, S32 checkGroup)
{
    // allocate the new menu item
    MenuItem* newMenuItem = new MenuItem;
    newMenuItem->text = dStrdup(text);
    if (accelerator[0])
        newMenuItem->accelerator = dStrdup(accelerator);
    else
        newMenuItem->accelerator = NULL;
    newMenuItem->id = id;
    newMenuItem->checkGroup = checkGroup;
    newMenuItem->nextMenuItem = NULL;
    newMenuItem->acceleratorIndex = 0;
    newMenuItem->enabled = text[0] != '-';
    newMenuItem->visible = true;
    newMenuItem->bitmapIndex = -1;

    // link it into the menu's menu item list
    MenuItem** walk = &menu->firstMenuItem;
    while (*walk)
        walk = &(*walk)->nextMenuItem;
    *walk = newMenuItem;

}

void GuiMenuBar::clearMenuItems(Menu* menu)
{
    while (menu->firstMenuItem)
        removeMenuItem(menu, menu->firstMenuItem);
}

void GuiMenuBar::clearMenus()
{
    while (menuList)
        removeMenu(menuList);
}

//------------------------------------------------------------------------------
// initialization, input and render methods
//------------------------------------------------------------------------------

GuiMenuBar::GuiMenuBar()
{
    menuList = NULL;
    menuBarDirty = true;
    mouseDownMenu = NULL;
    mouseOverMenu = NULL;
    mCurAcceleratorIndex = 0;
    mBackground = NULL;
}

bool GuiMenuBar::onWake()
{
    if (!Parent::onWake())
        return false;
    mProfile->constructBitmapArray();  // if a bitmap was specified...
    maxBitmapSize.set(0, 0);
    S32 numBitmaps = mProfile->mBitmapArrayRects.size();
    if (numBitmaps)
    {
        RectI* bitmapBounds = mProfile->mBitmapArrayRects.address();
        for (S32 i = 0; i < numBitmaps; i++)
        {
            if (bitmapBounds[i].extent.x > maxBitmapSize.x)
                maxBitmapSize.x = bitmapBounds[i].extent.x;
            if (bitmapBounds[i].extent.y > maxBitmapSize.y)
                maxBitmapSize.y = bitmapBounds[i].extent.y;
        }
    }
    return true;
}

GuiMenuBar::Menu* GuiMenuBar::findHitMenu(Point2I mousePoint)
{
    Point2I pos = globalToLocalCoord(mousePoint);

    for (Menu* walk = menuList; walk; walk = walk->nextMenu)
        if (walk->visible && walk->bounds.pointInRect(pos))
            return walk;
    return NULL;
}

void GuiMenuBar::onPreRender()
{
    Parent::onPreRender();
    if (menuBarDirty)
    {
        menuBarDirty = false;
        U32 curX = 0;
        for (Menu* walk = menuList; walk; walk = walk->nextMenu)
        {
            if (!walk->visible)
                continue;
            walk->bounds.set(curX, 1, mProfile->mFonts[0].mFont->getStrWidth((const UTF8*)walk->text) + 12, mBounds.extent.y - 2);
            curX += walk->bounds.extent.x;
        }
        mouseOverMenu = NULL;
        mouseDownMenu = NULL;
    }
}

void GuiMenuBar::checkMenuMouseMove(const GuiEvent& event)
{
    Menu* hit = findHitMenu(event.mousePoint);
    if (hit && hit != mouseDownMenu)
    {
        // gotta close out the current menu...
        mTextList->setSelectedCell(Point2I(-1, -1));
        closeMenu();
        mouseOverMenu = mouseDownMenu = hit;
        setUpdate();
        onAction();
    }
}

void GuiMenuBar::onMouseMove(const GuiEvent& event)
{
    Menu* hit = findHitMenu(event.mousePoint);
    if (hit != mouseOverMenu)
    {
        mouseOverMenu = hit;
        setUpdate();
    }
}

void GuiMenuBar::onMouseLeave(const GuiEvent& event)
{
    if (mouseOverMenu)
        setUpdate();
    mouseOverMenu = NULL;
}

void GuiMenuBar::onMouseDragged(const GuiEvent& event)
{
    Menu* hit = findHitMenu(event.mousePoint);

    if (hit != mouseOverMenu)
    {
        mouseOverMenu = hit;
        mouseDownMenu = hit;
        setUpdate();
        onAction();
    }
}

void GuiMenuBar::onMouseDown(const GuiEvent& event)
{
    mouseDownMenu = mouseOverMenu = findHitMenu(event.mousePoint);
    setUpdate();
    onAction();
}

void GuiMenuBar::onMouseUp(const GuiEvent& event)
{
    mouseDownMenu = NULL;
    setUpdate();
}

void GuiMenuBar::onRender(Point2I offset, const RectI& updateRect)
{
    //if opaque, fill the update rect with the fill color
    if (mProfile->mOpaque)
        GFX->drawRectFill(RectI(offset, mBounds.extent), mProfile->mFillColor);

    for (Menu* walk = menuList; walk; walk = walk->nextMenu)
    {
        if (!walk->visible)
            continue;
        ColorI fontColor = mProfile->mFontColor;
        RectI bounds = walk->bounds;
        bounds.point += offset;

        Point2I start;

        start.x = walk->bounds.point.x + 6;
        start.y = walk->bounds.point.y + (walk->bounds.extent.y - (mProfile->mFonts[0].mFont->getHeight() - 2)) / 2;

        if (walk == mouseDownMenu)
            renderSlightlyLoweredBox(bounds, mProfile);
        else if (walk == mouseOverMenu && mouseDownMenu == NULL)
            renderSlightlyRaisedBox(bounds, mProfile);

        GFX->setBitmapModulation(fontColor);

        GFX->drawText(mProfile->mFonts[0].mFont, start + offset, walk->text, mProfile->mFontColors);
    }
}

void GuiMenuBar::buildAcceleratorMap()
{
    Parent::buildAcceleratorMap();
    // ok, accelerator map is cleared...
    // add all our keys:
    mCurAcceleratorIndex = 1;

    for (Menu* menu = menuList; menu; menu = menu->nextMenu)
    {
        for (MenuItem* item = menu->firstMenuItem; item; item = item->nextMenuItem)
        {
            if (!item->accelerator)
            {
                item->accelerator = 0;
                continue;
            }
            EventDescriptor accelEvent;
            ActionMap::createEventDescriptor(item->accelerator, &accelEvent);

            //now we have a modifier, and a key, add them to the canvas
            GuiCanvas* root = getRoot();
            if (root)
                root->addAcceleratorKey(this, mCurAcceleratorIndex, accelEvent.eventCode, accelEvent.flags);
            item->acceleratorIndex = mCurAcceleratorIndex;
            mCurAcceleratorIndex++;
        }
    }
}

void GuiMenuBar::acceleratorKeyPress(U32 index)
{
    // loop through all the menus
    // and find the item that corresponds to the accelerator index
    for (Menu* menu = menuList; menu; menu = menu->nextMenu)
    {
        if (!menu->visible)
            continue;

        for (MenuItem* item = menu->firstMenuItem; item; item = item->nextMenuItem)
        {
            if (item->acceleratorIndex == index)
            {
                // first, call the script callback for menu selection:
                Con::executef(this, 4, "onMenuSelect", Con::getIntArg(menu->id),
                    menu->text);
                if (item->visible)
                    menuItemSelected(menu, item);
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Menu display class methods
//------------------------------------------------------------------------------

GuiMenuBackgroundCtrl::GuiMenuBackgroundCtrl(GuiMenuBar* ctrl, GuiMenuTextListCtrl* textList)
{
    mMenuBarCtrl = ctrl;
    mTextList = textList;
}

void GuiMenuBackgroundCtrl::onMouseDown(const GuiEvent& event)
{
    mTextList->setSelectedCell(Point2I(-1, -1));
    mMenuBarCtrl->closeMenu();
}

void GuiMenuBackgroundCtrl::onMouseMove(const GuiEvent& event)
{
    GuiCanvas* root = getRoot();
    GuiControl* ctrlHit = root->findHitControl(event.mousePoint, mLayer - 1);
    if (ctrlHit == mMenuBarCtrl)  // see if the current mouse over menu is right...
        mMenuBarCtrl->checkMenuMouseMove(event);
}

void GuiMenuBackgroundCtrl::onMouseDragged(const GuiEvent& event)
{
    GuiCanvas* root = getRoot();
    GuiControl* ctrlHit = root->findHitControl(event.mousePoint, mLayer - 1);
    if (ctrlHit == mMenuBarCtrl)  // see if the current mouse over menu is right...
        mMenuBarCtrl->checkMenuMouseMove(event);
}

GuiMenuTextListCtrl::GuiMenuTextListCtrl(GuiMenuBar* ctrl)
{
    mMenuBarCtrl = ctrl;
    mAlternatingRows = false;
}

void GuiMenuTextListCtrl::onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver)
{
    if (dStrcmp(mList[cell.y].text + 2, "-\t"))
        Parent::onRenderCell(offset, cell, selected, mouseOver);
    else
    {
        S32 yp = offset.y + mCellSize.y / 2;
        GFX->drawLine(offset.x, yp, offset.x + mCellSize.x, yp, ColorI(128, 128, 128));
        GFX->drawLine(offset.x, yp + 1, offset.x + mCellSize.x, yp + 1, ColorI(255, 255, 255));
    }
    // now see if there's a bitmap...
    U8 idx = mList[cell.y].text[0];
    if (idx != 1)
    {
        // there's a bitmap...
        U32 index = U32(idx - 2) * 3;
        if (!mList[cell.y].active)
            index += 2;
        else if (selected || mouseOver)
            index++;

        RectI rect = mProfile->mBitmapArrayRects[index];
        Point2I off = mMenuBarCtrl->maxBitmapSize - rect.extent;
        off /= 2;

        GFX->clearBitmapModulation();
        //dglDrawBitmapSR(mProfile->mTextureObject, offset + off, rect);
        GFX->drawBitmapSR(mProfile->mTextureObject, offset + off, rect);
    }
}

bool GuiMenuTextListCtrl::onKeyDown(const GuiEvent& event)
{
    //if the control is a dead end, don't process the input:
    if (!mVisible || !mActive || !mAwake)
        return false;

    //see if the key down is a <return> or not
    if (event.modifier == 0)
    {
        if (event.keyCode == KEY_RETURN)
        {
            mMenuBarCtrl->closeMenu();
            return true;
        }
        else if (event.keyCode == KEY_ESCAPE)
        {
            mSelectedCell.set(-1, -1);
            mMenuBarCtrl->closeMenu();
            return true;
        }
    }

    //otherwise, pass the event to it's parent
    return Parent::onKeyDown(event);
}

void GuiMenuTextListCtrl::onMouseDown(const GuiEvent& event)
{
    Parent::onMouseDown(event);
    mMenuBarCtrl->closeMenu();
}

void GuiMenuTextListCtrl::onMouseUp(const GuiEvent& event)
{
    // ok, this is kind of strange... but!
    // here's the deal: if we get a mouse up in this control
    // it means the mouse was dragged from the initial menu mouse click
    // so: activate the menu result as though this event were,
    // instead, a mouse down.

    onMouseDown(event);
}

//------------------------------------------------------------------------------

void GuiMenuBar::menuItemSelected(GuiMenuBar::Menu* menu, GuiMenuBar::MenuItem* item)
{
    if (item->enabled)
        Con::executef(this, 6, "onMenuItemSelect", Con::getIntArg(menu->id),
            menu->text, Con::getIntArg(item->id), item->text);
}

void GuiMenuBar::onSleep()
{
    if (mBackground) // a menu is up?
    {
        mTextList->setSelectedCell(Point2I(-1, -1));
        closeMenu();
    }
    Parent::onSleep();
}

void GuiMenuBar::closeMenu()
{
    // Get the selection from the text list:
    S32 selectionIndex = mTextList->getSelectedCell().y;

    // Pop the background:
    getRoot()->popDialogControl(mBackground);

    // Kill the popup:
    mBackground->deleteObject();
    mBackground = NULL;

    // Now perform the popup action:
    if (selectionIndex != -1)
    {
        MenuItem* list = mouseDownMenu->firstMenuItem;

        while (selectionIndex && list)
        {
            list = list->nextMenuItem;
            selectionIndex--;
        }
        if (list)
            menuItemSelected(mouseDownMenu, list);
    }
    mouseDownMenu = NULL;
}

//------------------------------------------------------------------------------
void GuiMenuBar::onAction()
{
    if (!mouseDownMenu)
        return;

    // first, call the script callback for menu selection:
    Con::executef(this, 4, "onMenuSelect", Con::getIntArg(mouseDownMenu->id),
        mouseDownMenu->text);

    MenuItem* visWalk = mouseDownMenu->firstMenuItem;
    while (visWalk)
    {
        if (visWalk->visible)
            break;
        visWalk = visWalk->nextMenuItem;
    }
    if (!visWalk)
    {
        mouseDownMenu = NULL;
        return;
    }

    mTextList = new GuiMenuTextListCtrl(this);
    mTextList->mProfile = mProfile;

    mBackground = new GuiMenuBackgroundCtrl(this, mTextList);

    GuiCanvas* root = getRoot();
    Point2I windowExt = root->mBounds.extent;

    mBackground->mBounds.point.set(0, 0);
    mBackground->mBounds.extent = root->mBounds.extent;

    S32 textWidth = 0, width = 0;
    S32 acceleratorWidth = 0;

    GFont* font = mProfile->mFonts[0].mFont;

    for (MenuItem* walk = mouseDownMenu->firstMenuItem; walk; walk = walk->nextMenuItem)
    {
        if (!walk->visible)
            continue;

        S32 iTextWidth = font->getStrWidth((const UTF8*)walk->text);
        S32 iAcceleratorWidth = walk->accelerator ? font->getStrWidth((const UTF8*)walk->accelerator) : 0;

        if (iTextWidth > textWidth)
            textWidth = iTextWidth;
        if (iAcceleratorWidth > acceleratorWidth)
            acceleratorWidth = iAcceleratorWidth;
    }
    width = textWidth + acceleratorWidth + maxBitmapSize.x * 2 + 2 + 4;

    mTextList->setCellSize(Point2I(width, font->getHeight() + 3));
    mTextList->clearColumnOffsets();
    mTextList->addColumnOffset(-1); // add an empty column in for the bitmap index.
    mTextList->addColumnOffset(maxBitmapSize.x + 1);
    mTextList->addColumnOffset(maxBitmapSize.x + 1 + textWidth + 4);

    U32 entryCount = 0;

    for (MenuItem* walk = mouseDownMenu->firstMenuItem; walk; walk = walk->nextMenuItem)
    {
        if (!walk->visible)
            continue;

        char buf[512];
        char bitmapIndex = 1;
        if (walk->bitmapIndex >= 0 && (walk->bitmapIndex * 3 <= mProfile->mBitmapArrayRects.size()))
            bitmapIndex = walk->bitmapIndex + 2;
        dSprintf(buf, sizeof(buf), "%c\t%s\t%s", bitmapIndex, walk->text, walk->accelerator ? walk->accelerator : "");
        mTextList->addEntry(entryCount, buf);

        if (!walk->enabled)
            mTextList->setEntryActive(entryCount, false);

        entryCount++;
    }
    Point2I menuPoint = localToGlobalCoord(mouseDownMenu->bounds.point);
    menuPoint.y += mouseDownMenu->bounds.extent.y + 2;

    GuiControl* ctrl = new GuiControl;
    ctrl->mBounds.point = menuPoint;
    ctrl->mBounds.extent = mTextList->mBounds.extent + Point2I(6, 6);
    ctrl->mProfile = mProfile;
    mTextList->mBounds.point += Point2I(3, 3);

    //mTextList->mBounds.point = Point2I(3,3);

    mTextList->registerObject();
    mBackground->registerObject();
    ctrl->registerObject();

    mBackground->addObject(ctrl);
    ctrl->addObject(mTextList);

    root->pushDialogControl(mBackground, mLayer + 1);
    mTextList->setFirstResponder();
}


