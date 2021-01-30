#include "atlas/runtime/atlasClipMapDebugCache.h"
#include "platform/platform.h"
#include "core/frameAllocator.h"

AtlasClipMapImageCache_Debug::AtlasClipMapImageCache_Debug()
{
    mDoSquares = true;
    mCurDebugMode = 0;
}

bool AtlasClipMapImageCache_Debug::isDataAvailable(U32 mipLevel, RectI region)
{
    // We generate on the fly, so it's always available!
    return true;
}

void AtlasClipMapImageCache_Debug::doRectUpdate(U32 mipLevel,
    AtlasClipMap::ClipStackEntry& cse,
    RectI srcRegion, RectI dstRegion)
{
    // Acquire a lock on our texture.
    GFXLockedRect* lockRect = cse.mTex->lock(0, &dstRegion);
    U32 pitch = lockRect->pitch;
    U8* bits = lockRect->bits;

    static bool updateToggle = false;

    updateToggle = !updateToggle;

    // Let's do a simple gradient + colored checkers test pattern.
    for (S32 x = 0; x < srcRegion.extent.x; x++)
    {
        for (S32 y = 0; y < srcRegion.extent.y; y++)
        {
            U8* pixel = bits + (x * pitch) + 4 * y;

            S32 realX = x + srcRegion.point.x;
            S32 realY = y + srcRegion.point.y;

            S32 xFlag = realX & 4;
            S32 yFlag = realY & 4;

            if (xFlag ^ yFlag)
                pixel[0] = 0xFF;
            else
                pixel[0] = 0;

            pixel[1] = F32(F32(realX) / F32(F32(512) * cse.mScale) * 255.0);
            pixel[2] = updateToggle ? 0xFF : 0;
            pixel[3] = 0; //F32(F32(realY) / F32(F32(mClipMapSize) * cse.mScale) * 255.0);
        }
    }

    // Don't forget to unlock.
    cse.mTex.unlock();
}
