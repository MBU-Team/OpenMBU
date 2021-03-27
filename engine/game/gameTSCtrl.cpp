//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/gameTSCtrl.h"
#include "console/consoleTypes.h"
#include "game/projectile.h"
#include "game/gameBase.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "game/player.h"

//---------------------------------------------------------------------------
// Debug stuff:
Point3F lineTestStart = Point3F(0, 0, 0);
Point3F lineTestEnd = Point3F(0, 1000, 0);
Point3F lineTestIntersect = Point3F(0, 0, 0);
bool gSnapLine = false;

//----------------------------------------------------------------------------
// Class: GameTSCtrl
//----------------------------------------------------------------------------
IMPLEMENT_CONOBJECT(GameTSCtrl);

GameTSCtrl::GameTSCtrl()
{
}

void GameTSCtrl::consoleInit()
{
    Con::addVariable("Game::renderPreview", TypeBool, &gRenderPreview);
}

//---------------------------------------------------------------------------
bool GameTSCtrl::processCameraQuery(CameraQuery* camq)
{
    if (gRenderPreview)
        return true;
    GameUpdateCameraFov();
    return GameProcessCameraQuery(camq);
}

//---------------------------------------------------------------------------
void GameTSCtrl::renderWorld(const RectI& updateRect)
{
    GameRenderWorld();
    GFX->setClipRect(updateRect);
}

//---------------------------------------------------------------------------
void GameTSCtrl::onMouseMove(const GuiEvent& evt)
{
    if (gSnapLine)
        return;

    MatrixF mat;
    Point3F vel;
    if (GameGetCameraTransform(&mat, &vel))
    {
        Point3F pos;
        mat.getColumn(3, &pos);
        Point3F screenPoint(evt.mousePoint.x, evt.mousePoint.y, -1);
        Point3F worldPoint;
        if (unproject(screenPoint, &worldPoint)) {
            Point3F vec = worldPoint - pos;
            lineTestStart = pos;
            vec.normalizeSafe();
            lineTestEnd = pos + vec * 1000;
        }
    }
}

void GameTSCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    bool disableSPMode = false;
    if (gRenderPreview && !gSPMode)
        disableSPMode = true;

    if (gRenderPreview)
        gSPMode = true;

    // check if should bother with a render
    GameConnection* con = GameConnection::getConnectionToServer();
    bool skipRender = !con || (con->getWhiteOut() >= 1.f) || (con->getDamageFlash() >= 1.f) || (con->getBlackOut() >= 1.f);

    if (gRenderPreview)
        skipRender = false;

    if (!skipRender)
        Parent::onRender(offset, updateRect);

    GFX->setViewport(updateRect);
    CameraQuery camq;
    if (GameProcessCameraQuery(&camq))
        GameRenderFilters(camq);

    if (disableSPMode)
        gSPMode = false;
}

//--------------------------------------------------------------------------
ConsoleFunction(snapToggle, void, 1, 1, "()")
{
    gSnapLine = !gSnapLine;
}
