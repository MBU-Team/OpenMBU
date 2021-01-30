#include "gui/game/guiBitmapAnimation.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiBitmapAnimation);

//------------------------------------------------------------------------------

GuiBitmapAnimation::GuiBitmapAnimation() : mLastFrameSwitchTime(0),
mAnimationFrameDelay(300),
mFrameNum(0),
mBoundsCorrect(false)
{
    // Nothing
}

//------------------------------------------------------------------------------

void GuiBitmapAnimation::initPersistFields()
{
    Parent::initPersistFields();

    addField("animationFrameDelay", TypeS32, Offset(mAnimationFrameDelay, GuiBitmapAnimation));
    addField("boundsCorrect", TypeBool, Offset(mBoundsCorrect, GuiBitmapAnimation));
}

//------------------------------------------------------------------------------

bool GuiBitmapAnimation::onWake()
{
    if (!Parent::onWake())
        return false;

    mProfile->constructBitmapArray();

    return true;
}

//------------------------------------------------------------------------------

void GuiBitmapAnimation::onRender(Point2I offset, const RectI& updateRect)
{
    if (Platform::getRealMilliseconds() > mLastFrameSwitchTime + mAnimationFrameDelay)
    {
        mFrameNum++;

        if (mFrameNum >= mProfile->mBitmapArrayRects.size())
            mFrameNum = 0;

        mLastFrameSwitchTime = Platform::getRealMilliseconds();
    }

    if (mProfile->mBitmapArrayRects.size() > 0)
    {
        // Note: we should really be using rect rather than "bounds" in the draw routine...
        // but don't want to screw it up for marble blast just now...
        // so only do this if "mBoundsCorrect" is true.
        RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
        GFX->drawBitmapStretchSR(mProfile->mTextureObject, mBoundsCorrect ? rect : mBounds, mProfile->mBitmapArrayRects[mFrameNum]);
    }

    renderChildControls(offset, updateRect);
}