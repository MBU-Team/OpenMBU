//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "gui/editor/guiGraphCtrl.h"

IMPLEMENT_CONOBJECT(GuiGraphCtrl);

GuiGraphCtrl::GuiGraphCtrl()
{

    for (int i = 0; i < MaxPlots; i++)
    {
        mPlots[i].mAutoPlot = NULL;
        mPlots[i].mAutoPlotDelay = 0;
        mPlots[i].mGraphColor = ColorF(1.0, 1.0, 1.0);
        VECTOR_SET_ASSOCIATION(mPlots[i].mGraphData);
        mPlots[i].mGraphMax = 1;
        mPlots[i].mGraphType = Polyline;
        for (int j = 0; j < MaxDataPoints; j++)
            mPlots[i].mGraphData.push_front(0.0);
    }

    AssertWarn(MaxPlots == 6, "Only 6 plot colors initialized.  Update following code if you change MaxPlots.");

    mPlots[0].mGraphColor = ColorF(1.0, 1.0, 1.0);
    mPlots[1].mGraphColor = ColorF(1.0, 0.0, 0.0);
    mPlots[2].mGraphColor = ColorF(0.0, 1.0, 0.0);
    mPlots[3].mGraphColor = ColorF(0.0, 0.0, 1.0);
    mPlots[4].mGraphColor = ColorF(0.0, 1.0, 1.0);
    mPlots[5].mGraphColor = ColorF(0.0, 0.0, 0.0);
}

static EnumTable::Enums enumGraphTypes[] = {
   { GuiGraphCtrl::Bar,        "bar"      },
   { GuiGraphCtrl::Filled,     "filled"   },
   { GuiGraphCtrl::Point,      "point"    },
   { GuiGraphCtrl::Polyline ,  "polyline" },
};

static EnumTable gGraphTypeTable(4, &enumGraphTypes[0]);

bool GuiGraphCtrl::onWake()
{
    if (!Parent::onWake())
        return false;
    setActive(true);
    return true;
}

void GuiGraphCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    if (mProfile->mBorder)
    {
        RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
        GFX->drawRect(rect, mProfile->mBorderColor);
    }

    GFX->setSrcBlend(GFXBlendSrcColor);
    GFX->setDestBlend(GFXBlendInvSrcColor);

    GFX->setAlphaBlendEnable(true);

    ColorF color(1.f, 1.f, 1.f, 0.5f);
    GFX->drawRectFill(updateRect, color);

    GFX->setAlphaBlendEnable(false);

    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendZero);

    for (int k = 0; k < MaxPlots; k++)
    {
        // Check if there is an autoplot and the proper amount of time has passed, if so add datum.
        if ((mPlots[k].mAutoPlot != NULL) &&
            (mPlots[k].mAutoPlotDelay < (Sim::getCurrentTime() - mPlots[k].mAutoPlotLastDisplay)))
        {
            mPlots[k].mAutoPlotLastDisplay = Sim::getCurrentTime();
            addDatum(k, Con::getFloatVariable(mPlots[k].mAutoPlot));
            Con::setIntVariable(mPlots[k].mAutoPlot, 0);
        }

        // Adjust scale to max value + 5% so we can see high values
        F32 Scale = (F32(getExtent().y) / (F32(mPlots[k].mGraphMax * 1.05)));

        // Nothing to graph
        if (mPlots[k].mGraphData.size() == 0)
            continue;

        // Bar graph
        if (mPlots[k].mGraphType == Bar)
        {
            PrimBuild::begin(GFXTriangleFan, getExtent().x * 4); // Max size correct? -pw
            PrimBuild::color3f(mPlots[k].mGraphColor.red, mPlots[k].mGraphColor.green, mPlots[k].mGraphColor.blue);

            S32 temp1, temp2;
            temp1 = 0;

            for (S32 sample = 0; sample < getExtent().x; sample++)
            {
                if (mPlots[k].mGraphData.size() >= getExtent().x)
                    temp2 = sample;
                else
                    temp2 = (S32)(((F32)getExtent().x / (F32)mPlots[k].mGraphData.size()) * (F32)sample);

                PrimBuild::vertex2f(getPosition().x + temp1,
                    (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));

                PrimBuild::vertex2f(getPosition().x + temp2,
                    (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));

                PrimBuild::vertex2f(getPosition().x + temp2,
                    getPosition().y + getExtent().y);

                PrimBuild::vertex2f(getPosition().x + temp1,
                    getPosition().y + getExtent().y);

                temp1 = temp2;
            }

            PrimBuild::end();
        }

        // Filled graph
        else if (mPlots[k].mGraphType == Filled)
        {
            // Not getExtent().x - 1, because we are adding another 4 verts after this loop. -pw
            PrimBuild::begin(GFXTriangleFan, getExtent().x * 4); // Max size correct? -pw

            PrimBuild::color3f(mPlots[k].mGraphColor.red, mPlots[k].mGraphColor.green, mPlots[k].mGraphColor.blue);

            S32 temp1, temp2;
            temp1 = 0;

            for (S32 sample = 0; sample < (getExtent().x - 1); sample++)
            {
                if (mPlots[k].mGraphData.size() >= getExtent().x)
                    temp2 = sample;
                else
                    temp2 = (S32)(((F32)getExtent().x / (F32)mPlots[k].mGraphData.size()) * (F32)sample);

                PrimBuild::vertex2f(getPosition().x + temp1,
                    (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));

                PrimBuild::vertex2f(getPosition().x + temp2,
                    (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample + 1) * Scale));

                PrimBuild::vertex2f(getPosition().x + temp2,
                    getPosition().y + getExtent().y);

                PrimBuild::vertex2f(getPosition().x + temp1,
                    getPosition().y + getExtent().y);

                temp1 = temp2;
            }


            // last point
            S32 sample = getExtent().x;

            if (mPlots[k].mGraphData.size() >= getExtent().x)
                temp2 = sample;
            else
                temp2 = (S32)(((F32)getExtent().x / (F32)mPlots[k].mGraphData.size()) * (F32)sample);

            PrimBuild::vertex2f(getPosition().x + temp1,
                (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));

            PrimBuild::vertex2f(getPosition().x + temp2,
                (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));

            PrimBuild::vertex2f(getPosition().x + temp2,
                getPosition().y + getExtent().y);

            PrimBuild::vertex2f(getPosition().x + temp1,
                getPosition().y + getExtent().y);

            PrimBuild::end();
        }


        // Point or Polyline graph
        else if ((mPlots[k].mGraphType == Point) || (mPlots[k].mGraphType == Polyline))
        {
            if (mPlots[k].mGraphType == Point)
                PrimBuild::begin(GFXPointList, getExtent().x); // Max size correct? -pw
            else
                PrimBuild::begin(GFXLineStrip, getExtent().x);

            PrimBuild::color3f(mPlots[k].mGraphColor.red, mPlots[k].mGraphColor.green, mPlots[k].mGraphColor.blue);

            for (S32 sample = 0; sample < getExtent().x; sample++)
            {
                S32 temp;
                if (mPlots[k].mGraphData.size() >= getExtent().x)
                    temp = sample;
                else
                    temp = (S32)(((F32)getExtent().x / (F32)mPlots[k].mGraphData.size()) * (F32)sample);

                PrimBuild::vertex2f(getPosition().x + temp,
                    (getPosition().y + getExtent().y) - (S32)(getDatum(k, sample) * Scale));
            }

            PrimBuild::end();
        }

    }

}

void GuiGraphCtrl::addDatum(S32 plotID, F32 v)
{
    AssertFatal(plotID > -1 && plotID < MaxPlots, "Invalid plot specified!");

    // Add the data and trim the vector...
    mPlots[plotID].mGraphData.push_front(v);

    if (mPlots[plotID].mGraphData.size() > MaxDataPoints)
        mPlots[plotID].mGraphData.pop_back();

    // Keep record of maximum data value for scaling purposes.
    if (v > mPlots[plotID].mGraphMax)
        mPlots[plotID].mGraphMax = v;
}

float GuiGraphCtrl::getDatum(int plotID, int sample)
{
    AssertFatal(plotID > -1 && plotID < MaxPlots, "Invalid plot specified!");
    AssertFatal(sample > -1 && sample < MaxDataPoints, "Invalid sample specified!");
    return mPlots[plotID].mGraphData[sample];
}

void GuiGraphCtrl::addAutoPlot(S32 plotID, const char* variable, S32 update)
{
    mPlots[plotID].mAutoPlot = StringTable->insert(variable);
    mPlots[plotID].mAutoPlotDelay = update;
    Con::setIntVariable(mPlots[plotID].mAutoPlot, 0);
}

void GuiGraphCtrl::removeAutoPlot(S32 plotID)
{
    mPlots[plotID].mAutoPlot = NULL;
}

void GuiGraphCtrl::setGraphType(S32 plotID, const char* graphType)
{
    AssertFatal(plotID > -1 && plotID < MaxPlots, "Invalid plot specified!");
    if (!dStricmp(graphType, "Bar"))
        mPlots[plotID].mGraphType = Bar;
    else if (!dStricmp(graphType, "Filled"))
        mPlots[plotID].mGraphType = Filled;
    else if (!dStricmp(graphType, "Point"))
        mPlots[plotID].mGraphType = Point;
    else if (!dStricmp(graphType, "Polyline"))
        mPlots[plotID].mGraphType = Polyline;
    else AssertWarn(true, "Invalid graph type!");
}

ConsoleMethod(GuiGraphCtrl, addDatum, void, 4, 4, "(int plotID, float v)"
    "Add a data point to the given plot.")
{
    S32 plotID = dAtoi(argv[2]);
    if (plotID > object->MaxPlots)
    {
        Con::errorf("Invalid plotID.");
        return;
    }
    object->addDatum(plotID, dAtof(argv[3]));
}

ConsoleMethod(GuiGraphCtrl, getDatum, F32, 4, 4, "(int plotID, int samples)"
    "Get a data point from the plot specified, samples from the start of the graph.")
{
    S32 plotID = dAtoi(argv[2]);
    S32 samples = dAtoi(argv[3]);

    if (plotID > object->MaxPlots)
    {
        Con::errorf("Invalid plotID.");
        return -1;
    }
    if (samples > object->MaxDataPoints)
    {
        Con::errorf("Invalid sample.");
        return -1;
    }

    return object->getDatum(plotID, samples);
}

ConsoleMethod(GuiGraphCtrl, addAutoPlot, void, 5, 5, "(int plotID, string variable, int update)"
    "Adds a data point with value variable, every update ms.")
{
    S32 plotID = dAtoi(argv[2]);

    if (plotID > object->MaxPlots)
    {
        Con::errorf("Invalid plotID.");
        return;
    }

    object->addAutoPlot(plotID, argv[3], dAtoi(argv[4]));
}

ConsoleMethod(GuiGraphCtrl, removeAutoPlot, void, 3, 3, "(int plotID)"
    "Stops automatic pointing over set interval.")
{
    S32 plotID = dAtoi(argv[2]);

    if (plotID > object->MaxPlots)
    {
        Con::errorf("Invalid plotID.");
        return;
    }

    object->removeAutoPlot(plotID);
}

ConsoleMethod(GuiGraphCtrl, setGraphType, void, 4, 4, "(int plotID, string graphType)"
    "Change GraphType of plot plotID.")
{
    S32 plotID = dAtoi(argv[2]);

    if (plotID > object->MaxPlots)
    {
        Con::errorf("Invalid plotID.");
        return;
    }

    object->setGraphType(dAtoi(argv[2]), argv[3]);
}

ConsoleMethod(GuiGraphCtrl, matchScale, void, 3, GuiGraphCtrl::MaxPlots + 2, "(int plotID, int plotID, ...)"
    "Sets the scale of all specified plots to the maximum scale among them.")
{
    F32 Max = 0;
    for (int i = 0; i < (argc - 2); i++)
    {
        if (dAtoi(argv[2 + i]) > object->MaxPlots)
        {
            Con::errorf("Invalid plotID.");
            return;
        }
        if (object->mPlots[dAtoi(argv[2 + i])].mGraphMax > Max)
            Max = object->mPlots[dAtoi(argv[2 + i])].mGraphMax;
    }
    for (int i = 0; i < (argc - 2); i++)
        object->mPlots[dAtoi(argv[2 + i])].mGraphMax = Max;
}