//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMETSCTRL_H_
#define _GAMETSCTRL_H_

#ifndef _GAME_H_
#include "game/game.h"
#endif
#ifndef _GUITSCONTROL_H_
#include "gui/core/guiTSControl.h"
#endif

class ProjectileData;
class GameBase;

//----------------------------------------------------------------------------
class GameTSCtrl : public GuiTSCtrl
{
private:
    typedef GuiTSCtrl Parent;

public:
    GameTSCtrl();

    bool processCameraQuery(CameraQuery* query);
    void renderWorld(const RectI& updateRect);

    void onMouseMove(const GuiEvent& evt);
    void onRender(Point2I offset, const RectI& updateRect);

    static void consoleInit();

    DECLARE_CONOBJECT(GameTSCtrl);
};

#endif
