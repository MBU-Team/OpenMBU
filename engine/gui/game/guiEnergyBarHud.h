//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIENERGYBARCTRL_H_
#define _GUIENERGYBARCTRL_H_

#ifndef _GUIBARCTRL_H_
#include "gui/game/guiBarCtrl.h"
#endif

class GuiEnergyBarCtrl : public GuiBarCtrl
{
private:
    typedef GuiBarCtrl Parent;

protected:
    ColorI mStateColors[3];
    F32 minActiveValue;

public:
    DECLARE_CONOBJECT(GuiEnergyBarCtrl);
    GuiEnergyBarCtrl();
    static void initPersistFields();

    void onRender(Point2I offset, const RectI& updateRect);

private:
    virtual F32 getValue();
};

#endif
