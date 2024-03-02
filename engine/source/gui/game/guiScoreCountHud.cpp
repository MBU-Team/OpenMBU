//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gui/core/guiDefaultControlRender.h"

#include "game/gameConnection.h"
#include "gui/core/guiTSControl.h"

#include "gui/game/guiScoreCountHud.h"

//--------------------------------------------------------
IMPLEMENT_CONOBJECT(GuiScoreCountHud);

GuiScoreCountHud::GuiScoreCountHud()
{
    mVelocity = 100.0f;
    mTimeTotal = 3000;
    mAcceleration = -33.0f;
    mVerticalOffset = 0.0f;
}

void GuiScoreCountHud::initPersistFields()
{
    Parent::initPersistFields();

    addField("velocity", TypeF32, Offset(mVelocity, GuiScoreCountHud));
    addField("acceleration", TypeF32, Offset(mAcceleration, GuiScoreCountHud));
    addField("time", TypeS32, Offset(mTimeTotal, GuiScoreCountHud));
    addField("verticalOffset", TypeF32, Offset(mVerticalOffset, GuiScoreCountHud));
}

//--------------------------------------------------------

void GuiScoreCountHud::onRender(Point2I offset, const RectI& updateRect)
{
    U32 curTime = Sim::getCurrentTime();

    for (S32 i = 0; i < scores.size(); i++)
    {
        if (curTime - scores[i].startTime <= mTimeTotal)
            continue;

        scores.erase(i);
    }

    GuiTSCtrl* parent = dynamic_cast<GuiTSCtrl*>(getParent());
    if (!parent)
        return;

    GameConnection* con = GameConnection::getConnectionToServer();
    if (!con)
        return;

    ShapeBase* obj = dynamic_cast<ShapeBase*>(con->getControlObject());
    if (!obj)
        return;

    MatrixF srtMat = obj->getRenderTransform();
    Point3F shapePos = srtMat.getPosition();
    shapePos.z += mVerticalOffset;

    Point3F projPnt;
    if (parent->project(shapePos, &projPnt))
    {
        GFX->setAlphaBlendEnable(true);
        GFX->setSrcBlend(GFXBlendSrcAlpha);
        GFX->setDestBlend(GFXBlendInvSrcAlpha);

        for (S32 i = 0; i < scores.size(); i++)
        {
            ScoreRec score = scores[i];

            Point2I renderPt(projPnt.x, projPnt.y);

            U32 time = curTime - score.startTime;
            F32 t = time * 0.001f;

            F32 a = 1.0f - (F32)time / (F32)mTimeTotal;

            renderPt.y -= (mAcceleration * 0.5f * t * t + t * mVelocity);

            // original gg code
            //char scoreBuffer[3];
            //scoreBuffer[0] = '+';
            //scoreBuffer[1] = score.score + '0';
            //scoreBuffer[2] = '\0';

            // if we ever want to support anything besides single-digit positive numbers, use this instead of the above
            char scoreBuffer[256];
            dSprintf(scoreBuffer, 256, "%c%d", (score.score < 0 ? ' ' : '+'), score.score);

            renderPt.x -= mProfile->mFonts[0].mFont->getStrWidth(scoreBuffer) >> 1;
            renderPt.y -= mProfile->mFonts[0].mFont->getHeight();

            ColorF foreColor = score.foreColor;
            foreColor.alpha *= a;
            ColorF backColor = score.backColor;
            backColor.alpha *= a;

            GFX->setBitmapModulation(backColor);
            GFX->drawText(mProfile->mFonts[0].mFont, renderPt, scoreBuffer);
            --renderPt.x;
            --renderPt.y;

            GFX->setBitmapModulation(foreColor);
            GFX->drawText(mProfile->mFonts[0].mFont, renderPt, scoreBuffer);
        }

        GFX->clearBitmapModulation();
        GFX->setAlphaBlendEnable(false);
    }
}

void GuiScoreCountHud::addScore(S32 score, ColorF foreColor, ColorF backColor)
{
    ScoreRec theScore;
    theScore.score = score;
    theScore.foreColor = foreColor;
    theScore.backColor = backColor;
    theScore.startTime = Sim::getCurrentTime();

    scores.push_back(theScore);
}

//--------------------------------------------------------

ConsoleMethod(GuiScoreCountHud, addScore, void, 5, 5, "(score, foreColor, backColor)")
{
    ColorF foreColor;
    ColorF backColor;

    dSscanf(argv[3], "%f %f %f %f", &foreColor.red, &foreColor.green, &foreColor.blue, &foreColor.alpha);
    dSscanf(argv[4], "%f %f %f %f", &backColor.red, &backColor.green, &backColor.blue, &backColor.alpha);

    object->addScore(dAtoi(argv[2]), foreColor, backColor);
}
