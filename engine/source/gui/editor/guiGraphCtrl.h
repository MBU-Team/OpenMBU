//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIGRAPHCTRL_H_
#define _GUIGRAPHCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

class GuiGraphCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;

public:
    enum Constants {
        MaxPlots = 6,
        MaxDataPoints = 200
    };

    enum GraphType {
        Point,
        Polyline,
        Filled,
        Bar
    };

    struct PlotInfo
    {
        const char* mAutoPlot;
        U32			mAutoPlotDelay;
        SimTime		mAutoPlotLastDisplay;
        ColorF		mGraphColor;
        Vector<F32>	mGraphData;
        F32			mGraphMax;
        GraphType		mGraphType;
    };

    PlotInfo		mPlots[MaxPlots];

    //creation methods
    DECLARE_CONOBJECT(GuiGraphCtrl);
    GuiGraphCtrl();

    //Parental methods
    bool onWake();

    void onRender(Point2I offset, const RectI& updateRect);

    // Graph interface
    void addDatum(S32 plotID, F32 v);
    F32  getDatum(S32 plotID, S32 samples);
    void addAutoPlot(S32 plotID, const char* variable, S32 update);
    void removeAutoPlot(S32 plotID);
    void setGraphType(S32 plotID, const char* graphType);
};

#endif
