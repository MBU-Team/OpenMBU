//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"


#include "gui/controls/guiBitmapCtrl.h"

class GuiFadeinBitmapCtrl : public GuiBitmapCtrl
{
    typedef GuiBitmapCtrl Parent;
public:
    U32 wakeTime;
    bool done;
    U32 fadeinTime;
    U32 waitTime;
    U32 fadeoutTime;

    GuiFadeinBitmapCtrl()
    {
        wakeTime = 0;
        fadeinTime = 1000;
        waitTime = 2000;
        fadeoutTime = 1000;
        done = false;
    }
    void onPreRender()
    {
        Parent::onPreRender();
        setUpdate();
    }
    void onMouseDown(const GuiEvent&)
    {
        Con::executef(this, 1, "click");
    }
    bool onKeyDown(const GuiEvent&)
    {
        Con::executef(this, 1, "click");
        return true;
    }
    DECLARE_CONOBJECT(GuiFadeinBitmapCtrl);
    bool onWake()
    {
        if (!Parent::onWake())
            return false;
        wakeTime = Platform::getRealMilliseconds();
        return true;
    }
    void onRender(Point2I offset, const RectI& updateRect)
    {
        GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);

        Parent::onRender(offset, updateRect);
        U32 elapsed = Platform::getRealMilliseconds() - wakeTime;

        U32 alpha;
        if (elapsed < fadeinTime)
        {
            // fade-in
            alpha = 255 - (255 * (F32(elapsed) / F32(fadeinTime)));
        }
        else if (elapsed < (fadeinTime + waitTime))
        {
            // wait
            alpha = 0;
        }
        else if (elapsed < (fadeinTime + waitTime + fadeoutTime))
        {
            // fade out
            elapsed -= (fadeinTime + waitTime);
            alpha = 255 * F32(elapsed) / F32(fadeoutTime);
        }
        else
        {
            // done state
            alpha = fadeoutTime ? 255 : 0;
            done = true;
        }
        ColorI color(0, 0, 0, alpha);
        GFX->drawRectFill(offset, mBounds.extent + offset, color);
    }
    static void initPersistFields()
    {
        Parent::initPersistFields();
        addField("fadeinTime", TypeS32, Offset(fadeinTime, GuiFadeinBitmapCtrl));
        addField("waitTime", TypeS32, Offset(waitTime, GuiFadeinBitmapCtrl));
        addField("fadeoutTime", TypeS32, Offset(fadeoutTime, GuiFadeinBitmapCtrl));
        addField("done", TypeBool, Offset(done, GuiFadeinBitmapCtrl));
    }
};

IMPLEMENT_CONOBJECT(GuiFadeinBitmapCtrl);

























