//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gBitmap.h"
#include "gui/core/guiControl.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureHandle.h"

class GuiChunkedBitmapCtrl : public GuiControl
{
private:
    typedef GuiControl Parent;
    void renderRegion(const Point2I& offset, const Point2I& extent);

protected:
    StringTableEntry mBitmapName;
    GFXTexHandle mTexHandle;
    bool  mUseVariable;
    bool  mTile;

public:
    //creation methods
    DECLARE_CONOBJECT(GuiChunkedBitmapCtrl);
    GuiChunkedBitmapCtrl();
    static void initPersistFields();

    //Parental methods
    bool onWake();
    void onSleep();

    void setBitmap(const char* name);

    void onRender(Point2I offset, const RectI& updateRect);
};

IMPLEMENT_CONOBJECT(GuiChunkedBitmapCtrl);

void GuiChunkedBitmapCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");
    addField("bitmap", TypeFilename, Offset(mBitmapName, GuiChunkedBitmapCtrl));
    addField("useVariable", TypeBool, Offset(mUseVariable, GuiChunkedBitmapCtrl));
    addField("tile", TypeBool, Offset(mTile, GuiChunkedBitmapCtrl));
    endGroup("Misc");
}

ConsoleMethod(GuiChunkedBitmapCtrl, setBitmap, void, 3, 3, "(string filename)"
    "Set the bitmap contained in this control.")
{
    AssertFatal(dynamic_cast<GuiChunkedBitmapCtrl*>(object), "Object passed to cChunkBmpSetBitmap is not a GuiChunkedBitmapCtrl!");
    GuiChunkedBitmapCtrl* ctrl = static_cast<GuiChunkedBitmapCtrl*>(object);
    ctrl->setBitmap(argv[2]);
}

GuiChunkedBitmapCtrl::GuiChunkedBitmapCtrl()
{
    mBitmapName = StringTable->insert("");
    mUseVariable = false;
    mTile = false;
}

void GuiChunkedBitmapCtrl::setBitmap(const char* name)
{
    bool awake = mAwake;
    if (awake)
        onSleep();

    mBitmapName = StringTable->insert(name);
    if (awake)
        onWake();
    setUpdate();
}

bool GuiChunkedBitmapCtrl::onWake()
{
    if (!Parent::onWake())
        return false;

    if (!mTexHandle)
    {
        if (mUseVariable)
            mTexHandle.set(Con::getVariable(mConsoleVariable), &GFXDefaultGUIProfile);
        else
            mTexHandle.set(mBitmapName, &GFXDefaultGUIProfile);
    }


    return true;
}

void GuiChunkedBitmapCtrl::onSleep()
{
    mTexHandle = NULL;
    Parent::onSleep();
}

void GuiChunkedBitmapCtrl::renderRegion(const Point2I& offset, const Point2I& extent)
{
    /*
       U32 widthCount = mTexHandle.getTextureCountWidth();
       U32 heightCount = mTexHandle.getTextureCountHeight();
       if(!widthCount || !heightCount)
          return;

       F32 widthScale = F32(extent.x) / F32(mTexHandle.getWidth());
       F32 heightScale = F32(extent.y) / F32(mTexHandle.getHeight());
       dglSetBitmapModulation(ColorF(1,1,1));
       for(U32 i = 0; i < widthCount; i++)
       {
          for(U32 j = 0; j < heightCount; j++)
          {
             TextureHandle t = mTexHandle.getSubTexture(i, j);
             RectI stretchRegion;
             stretchRegion.point.x = (S32)(i * 256 * widthScale  + offset.x);
             stretchRegion.point.y = (S32)(j * 256 * heightScale + offset.y);
             if(i == widthCount - 1)
                stretchRegion.extent.x = extent.x + offset.x - stretchRegion.point.x;
             else
                stretchRegion.extent.x = (S32)((i * 256 + t.getWidth() ) * widthScale  + offset.x - stretchRegion.point.x);
             if(j == heightCount - 1)
                stretchRegion.extent.y = extent.y + offset.y - stretchRegion.point.y;
             else
                stretchRegion.extent.y = (S32)((j * 256 + t.getHeight()) * heightScale + offset.y - stretchRegion.point.y);
             dglDrawBitmapStretch(t, stretchRegion);
          }
       }
    */
}


void GuiChunkedBitmapCtrl::onRender(Point2I offset, const RectI& updateRect)
{

    if (mTexHandle)
    {
        GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);

        RectI rect(offset, mBounds.extent);
        GFX->drawBitmapStretch(mTexHandle, mBounds);
    }


    /*
       if(mTexHandle)
       {
          if (mTile)
          {
             int stepy = 0;
             for(int y = 0; offset.y + stepy < mBounds.extent.y; stepy += mTexHandle.getHeight())
             {
                int stepx = 0;
                for(int x = 0; offset.x + stepx < mBounds.extent.x; stepx += mTexHandle.getWidth())
                   renderRegion(Point2I(offset.x+stepx, offset.y+stepy), Point2I(mTexHandle.getWidth(), mTexHandle.getHeight()) );
             }
          }
          else
             renderRegion(offset, mBounds.extent);
    */
    renderChildControls(offset, updateRect);
    /*
       }
       else
          Parent::onRender(offset, updateRect);
    */
}
