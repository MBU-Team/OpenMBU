//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#include "d3d9.h"
#include "gfx/D3D/gfxD3DTextureObject.h"
#include "gfxD3DDevice.h"

#include "platform/profiler.h"

U32 GFXD3DTextureObject::mTexCount = 0;

//*****************************************************************************
// GFX D3D Texture Object
//*****************************************************************************
GFXD3DTextureObject::GFXD3DTextureObject(GFXDevice* d, GFXTextureProfile* profile)
    : GFXTextureObject(d, profile)
{
#ifdef D3D_TEXTURE_SPEW
    mTexCount++;
    Con::printf("+ texMake %d %x", mTexCount, this);
#endif

    isManaged = false;
    mD3DTexture = NULL;
    mLocked = false;

    mD3DSurface = NULL;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GFXD3DTextureObject::~GFXD3DTextureObject()
{
    kill();
#ifdef D3D_TEXTURE_SPEW
    mTexCount--;
    Con::printf("+ texkill %d %x", mTexCount, this);
#endif
}

//-----------------------------------------------------------------------------
// lock
//-----------------------------------------------------------------------------
GFXLockedRect* GFXD3DTextureObject::lock(U32 mipLevel /*= 0*/, RectI* inRect /*= NULL*/)
{
    if (mProfile->isRenderTarget())
    {
        if (!mLockTex ||
            mLockTex->getWidth() != getWidth() ||
            mLockTex->getHeight() != getHeight())
        {
            mLockTex.set(getWidth(), getHeight(), mFormat, &GFXSystemMemProfile);
        }

        PROFILE_START(GFXD3DTextureObject_lockRT);

        IDirect3DSurface9* source;
        D3DAssert(get2DTex()->GetSurfaceLevel(0, &source), NULL);

        IDirect3DSurface9* dest;
        GFXD3DTextureObject* to = (GFXD3DTextureObject*)&(*mLockTex);
        D3DAssert(to->get2DTex()->GetSurfaceLevel(0, &dest), NULL);

        LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3DDevice*>(GFX)->getDevice();
        D3DAssert(D3DDevice->GetRenderTargetData(source, dest), NULL);

        source->Release();

        D3DAssert(dest->LockRect(&mLockRect, NULL, D3DLOCK_READONLY), NULL);
        dest->Release();
        mLocked = true;

        PROFILE_END();

        // GFXLockedRect is set up to correspond to D3DLOCKED_RECT, so this is ok.
        return (GFXLockedRect*)&mLockRect;
    }
    else
    {
        RECT r;

        if (inRect)
        {
            r.top = inRect->point.x;
            r.left = inRect->point.y;
            r.bottom = inRect->point.x + inRect->extent.x;
            r.right = inRect->point.y + inRect->extent.y;
        }

        D3DAssert(get2DTex()->LockRect(mipLevel, &mLockRect, inRect ? &r : NULL, 0),
            "GFXD3DTextureObject::lock - could not lock non-RT texture!");
        mLocked = true;

        // GFXLockedRect is set up to correspond to D3DLOCKED_RECT, so this is ok.
        return (GFXLockedRect*)&mLockRect;
    }

    return NULL;
}

//-----------------------------------------------------------------------------
// unLock
//-----------------------------------------------------------------------------
void GFXD3DTextureObject::unlock(U32 mipLevel)
{
    AssertFatal(mLocked, "Attempting to unlock a surface that has not been locked");

    if (mProfile->isRenderTarget())
    {
        IDirect3DSurface9* dest;
        GFXD3DTextureObject* to = (GFXD3DTextureObject*)&(*mLockTex);
        D3DAssert(to->get2DTex()->GetSurfaceLevel(0, &dest), NULL);

        dest->UnlockRect();
        dest->Release();

        mLocked = false;
    }
    else
    {
        D3DAssert(get2DTex()->UnlockRect(mipLevel),
            "GFXD3DTextureObject::unlock - could not unlock non-RT texture.");

        mLocked = false;
    }

}

//-----------------------------------------------------------------------------
// release
//-----------------------------------------------------------------------------
void GFXD3DTextureObject::release()
{
    if (mD3DTexture != NULL)
    {
        mD3DTexture->Release();
        mD3DTexture = NULL;
    }

    if (mD3DSurface != NULL)
    {
        mD3DSurface->Release();
        mD3DSurface = NULL;
    }
}

//-----------------------------------------------------------------------------
// copyToBmp
//-----------------------------------------------------------------------------
bool GFXD3DTextureObject::copyToBmp(GBitmap* bmp)
{
    // profiler?
    AssertFatal(bmp, "copyToBmp: null bitmap specified");
    if (!bmp)
        return false;

    AssertFatal(mProfile->isRenderTarget(), "copyToBmp: this texture is not a render target");
    if (!mProfile->isRenderTarget())
        return false;

    // check format limitations
    // at the moment we only support RGBA for the source (other 4 byte formats should
    // be easy to add though)
    AssertFatal(mFormat == GFXFormatR8G8B8A8, "copyToBmp: invalid format");
    if (mFormat != GFXFormatR8G8B8A8)
        return false;

    PROFILE_START(GFXD3DTextureObject_copyToBmp);

    AssertFatal(bmp->getWidth() == getWidth(), "doh");
    AssertFatal(bmp->getHeight() == getHeight(), "doh");
    U32 width = getWidth();
    U32 height = getHeight();

    // set some constants
    const U32 sourceBytesPerPixel = 4;
    U32 destBytesPerPixel;
    if (bmp->getFormat() == GFXFormatR8G8B8A8)
        destBytesPerPixel = 4;
    else if (bmp->getFormat() == GFXFormatR8G8B8)
        destBytesPerPixel = 3;
    else
        // unsupported
        AssertFatal(false, "unsupported bitmap format");

    // lock the texture
    D3DLOCKED_RECT* lockRect = (D3DLOCKED_RECT*)lock();

    // set pointers
    U8* srcPtr = (U8*)lockRect->pBits;
    U8* destPtr = bmp->getWritableBits();

    // we will want to skip over any D3D cache data in the source texture
    const S32 sourceCacheSize = lockRect->Pitch - width * sourceBytesPerPixel;
    AssertFatal(sourceCacheSize >= 0, "copyToBmp: cache size is less than zero?");

    PROFILE_START(GFXD3DTextureObject_copyToBmp_pixCopy);
    // copy data into bitmap
    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            destPtr[0] = srcPtr[2]; // red
            destPtr[1] = srcPtr[1]; // green
            destPtr[2] = srcPtr[0]; // blue 
            if (destBytesPerPixel == 4)
                destPtr[3] = srcPtr[3]; // alpha

             // go to next pixel in src
            srcPtr += sourceBytesPerPixel;

            // go to next pixel in dest
            destPtr += destBytesPerPixel;
        }
        // skip past the cache data for this row (if any)
        srcPtr += sourceCacheSize;
    }
    PROFILE_END();

    // assert if we stomped or underran memory
    AssertFatal(U32(destPtr - bmp->getWritableBits()) == width * height * destBytesPerPixel, "copyToBmp: doh, memory error");
    AssertFatal(U32(srcPtr - lockRect->pBits) == height * lockRect->Pitch, "copyToBmp: doh, memory error");

    // unlock
    unlock();

    PROFILE_END();

    return true;
}
