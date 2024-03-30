//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"

#include "gui/controls/guiBitmapCtrl.h"

IMPLEMENT_CONOBJECT(GuiBitmapCtrl);

GuiBitmapCtrl::GuiBitmapCtrl(void)
{
    mBitmapName = StringTable->insert("");
    startPoint.set(0, 0);
    mWrap = false;
    flipY = false;
    mTextureObject = NULL;
    mOnMouseUpCommand = StringTable->insert("");
}


void GuiBitmapCtrl::initPersistFields()
{
    Parent::initPersistFields();
    addGroup("Misc");
    addField("bitmap", TypeFilename, Offset(mBitmapName, GuiBitmapCtrl));
    addField("wrap", TypeBool, Offset(mWrap, GuiBitmapCtrl));
    addField("onMouseUp", TypeString, Offset(mOnMouseUpCommand, GuiBitmapCtrl));
    addField("flipY", TypeBool, Offset(flipY, GuiBitmapCtrl));
    endGroup("Misc");
}

ConsoleMethod(GuiBitmapCtrl, setValue, void, 4, 4, "(int xAxis, int yAxis)"
    "Set the offset of the bitmap.")
{
    object->setValue(dAtoi(argv[2]), dAtoi(argv[3]));
}

ConsoleMethod(GuiBitmapCtrl, setBitmap, void, 3, 4, "(string filename, bool resize=false)"
    "Set the bitmap displayed in the control. Note that it is limited in size, to 256x256.")
{
    char fileName[1024];
    Con::expandScriptFilename(fileName, sizeof(fileName), argv[2]);
    object->setBitmap(fileName, argc > 3 ? dAtob(argv[3]) : false);
}

bool GuiBitmapCtrl::onWake()
{
    if (!Parent::onWake())
        return false;
    setActive(true);
    setBitmap(mBitmapName);
    return true;
}

void GuiBitmapCtrl::onSleep()
{
    mTextureObject = NULL;
    Parent::onSleep();
}

//-------------------------------------
void GuiBitmapCtrl::inspectPostApply()
{
    // if the extent is set to (0,0) in the gui editor and appy hit, this control will
    // set it's extent to be exactly the size of the bitmap (if present)
    Parent::inspectPostApply();

    if (!mWrap && (mBounds.extent.x == 0) && (mBounds.extent.y == 0) && mTextureObject)
    {
        mBounds.extent.x = mTextureObject->getWidth();
        mBounds.extent.y = mTextureObject->getHeight();
    }
}

void GuiBitmapCtrl::setBitmap(const char* name, bool resize)
{
    PROFILE_START(GuiBitmapCtrl_SetBitmap);

    mBitmapName = StringTable->insert(name);
    if (*mBitmapName)
    {
        mTextureObject.set(mBitmapName, &GFXDefaultGUIProfile);

        // Resize the control to fit the bitmap
        if (mTextureObject && resize)
        {
            mBounds.extent.x = mTextureObject->getWidth();
            mBounds.extent.y = mTextureObject->getHeight();
            Point2I extent = getParent()->getExtent();
            parentResized(extent, extent);
        }
    }
    else
        mTextureObject = NULL;

    setUpdate();

    PROFILE_END();
}


void GuiBitmapCtrl::setBitmap(GFXTexHandle handle, bool resize)
{
    mTextureObject = handle;

    // Resize the control to fit the bitmap
    if (resize)
    {
        mBounds.extent.x = mTextureObject->getWidth();
        mBounds.extent.y = mTextureObject->getHeight();
        Point2I extent = getParent()->getExtent();
        parentResized(extent, extent);
    }
}


void GuiBitmapCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    if (mTextureObject)
    {
        GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);

        GFX->clearBitmapModulation();
        if (mWrap)
        {

            GFXTextureObject* texture = mTextureObject;
            RectI srcRegion;
            RectI dstRegion;
            float xdone = ((float)mBounds.extent.x / (float)texture->mBitmapSize.x) + 1;
            float ydone = ((float)mBounds.extent.y / (float)texture->mBitmapSize.y) + 1;

            int xshift = startPoint.x % texture->mBitmapSize.x;
            int yshift = startPoint.y % texture->mBitmapSize.y;
            for (int y = 0; y < ydone; ++y)
                for (int x = 0; x < xdone; ++x)
                {
                    srcRegion.set(0, 0, texture->mBitmapSize.x, texture->mBitmapSize.y);
                    dstRegion.set(((texture->mBitmapSize.x * x) + offset.x) - xshift,
                        ((texture->mBitmapSize.y * y) + offset.y) - yshift,
                        texture->mBitmapSize.x,
                        texture->mBitmapSize.y);
                    GFX->drawBitmapStretchSR(texture, dstRegion, srcRegion, flipY ? GFXBitmapFlip_Y : GFXBitmapFlip_None);
                }

        }
        else
        {
            RectI rect(offset, mBounds.extent);
            GFX->drawBitmapStretch(mTextureObject, rect, flipY ? GFXBitmapFlip_Y : GFXBitmapFlip_None);
        }
    }

    if (mProfile->mBorder || !mTextureObject)
    {
        RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
        GFX->drawRect(rect, mProfile->mBorderColor);
    }

    renderChildControls(offset, updateRect);
}

void GuiBitmapCtrl::setValue(S32 x, S32 y)
{
    if (mTextureObject)
    {
        x += mTextureObject->getWidth() / 2;
        y += mTextureObject->getHeight() / 2;
    }
    while (x < 0)
        x += 256;
    startPoint.x = x % 256;

    while (y < 0)
        y += 256;
    startPoint.y = y % 256;
}

void GuiBitmapCtrl::onMouseUp(const GuiEvent &event)
{
    if (mOnMouseUpCommand[0])
        Con::evaluate(mOnMouseUpCommand, false);
}
