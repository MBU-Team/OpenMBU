//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIGAMELISTMENUCTRL_H_
#define _GUIGAMELISTMENUCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiGameListMenuCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

protected:
    Vector<StringTableEntry> mRowText;
    Vector<StringTableEntry> mRowData;
    S32 mRowHeight;
    S32 mRowsPerPage;
    S32 mSelected;
    S32 mTopRow;

public:
    //DECLARE_CONOBJECT(GuiGameListMenuCtrl);
    GuiGameListMenuCtrl();
    //static void initPersistFields();

    virtual bool onWake();
    virtual void onSleep();

    void move(int increment);
    void moveUp();
    void moveDown();
    void setSelectedIndex(int selected);
    void clear();
    int getSelectedIndex();
};

#endif // _GUIGAMELISTMENUCTRL_H_