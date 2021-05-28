//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUISCORECOUNTHUD_H_
#define _GUISCORECOUNTHUD_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiScoreCountHud : public GuiControl
{
private:
    typedef GuiControl Parent;

protected:
    struct ScoreRec
    {
        S32 score;
        U32 startTime;
        ColorF foreColor;
        ColorF backColor;
    };

    ColorF mForeColors[5];
    ColorF mBackColors[5];
    F32 mVelocity;
    F32 mAcceleration;
    U32 mTimeTotal;
    F32 mVerticalOffset;
    Vector<ScoreRec> scores;
    
public:
    DECLARE_CONOBJECT(GuiScoreCountHud);
    GuiScoreCountHud();
    static void initPersistFields();

    void onRender(Point2I offset, const RectI& updateRect);

    void addScore(S32 score, ColorF foreColor, ColorF backColor);
};

#endif // _GUISCORECOUNTHUD_H_
