//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gfx/gfxTextureObject.h"
#include "gfx/gfxDevice.h"
#include "platform/profiler.h"

#ifdef TORQUE_DEBUG
GFXTextureObject* GFXTextureObject::smHead = NULL;
U32 GFXTextureObject::smExtantTOCount = 0;

void GFXTextureObject::dumpExtantTOs()
{
    if (!smExtantTOCount)
    {
        Con::printf("GFXTextureObject::dumpExtantTOs - no extant TOs to dump. You are A-OK!");
        return;
    }

    Con::printf("GFXTextureObject Usage Report - %d extant TOs", smExtantTOCount);
    Con::printf("---------------------------------------------------------------");
    Con::printf(" Addr   Dim. GFXTextureProfile  Profiler Path");

    for (GFXTextureObject* walk = smHead; walk; walk = walk->mDebugNext)
        Con::printf(" %x  (%4d, %4d)  %s    %s", walk, walk->getWidth(),
            walk->getHeight(), walk->mProfile->getName(), walk->mDebugCreationPath);

    Con::printf("----- dump complete -------------------------------------------");
}

#endif

//-----------------------------------------------------------------------------
// GFXTextureObject
//-----------------------------------------------------------------------------
GFXTextureObject::GFXTextureObject(GFXDevice* aDevice, GFXTextureProfile* aProfile)
{
    mHashNext = mNext = mPrev = NULL;

    mDevice = aDevice;
    mProfile = aProfile;

    mTextureFileName = NULL;
    mProfile = NULL;
    mBitmap = NULL;
    mMipLevels = 1;

    mTextureSize.set(0, 0, 0);

    mDead = false;

    cacheId = 0;
    cacheTime = 0;

    mBitmap = NULL;
    mDDS = NULL;

#ifdef TORQUE_DEBUG
    // Extant tracking.
    smExtantTOCount++;
    mDebugCreationPath = gProfiler->getProfilePath();
    mDebugNext = smHead;
    mDebugPrev = NULL;

    if (smHead)
    {
        AssertFatal(smHead->mDebugPrev == NULL, "GFXTextureObject::GFXTextureObject - found unexpected previous in current head!");
        smHead->mDebugPrev = this;
    }

    smHead = this;
#endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GFXTextureObject::~GFXTextureObject()
{
    kill();

#ifdef TORQUE_DEBUG
    if (smHead == this)
        smHead = this->mDebugNext;

    if (mDebugNext)
        mDebugNext->mDebugPrev = mDebugPrev;

    if (mDebugPrev)
        mDebugPrev->mDebugNext = mDebugNext;

    mDebugPrev = mDebugNext = NULL;

    smExtantTOCount--;
#endif
}

//-----------------------------------------------------------------------------
// kill - this function clears out the data in texture object.  It's done like
// this because the texture object needs to release its pointers to textures
// before the GFXDevice is shut down.  The texture objects themselves get
// deleted by the refcount structure - which may be after the GFXDevice has
// been destroyed.
//-----------------------------------------------------------------------------
void GFXTextureObject::kill()
{
    if (mDead) return;

#ifdef TORQUE_DEBUG
    // This makes sure that nobody is forgetting to call kill from the derived
    // destructor.  If they are, then we should get a pure virtual function
    // call here.
    pureVirtualCrash();
#endif

    // If we're a dummy, don't do anything...
    if (!mDevice || !mDevice->mTextureManager) return;

    // Remove ourselves from the texture list and hash
    mDevice->mTextureManager->deleteTexture(this);

    // Delete bitmap(s)
    SAFE_DELETE(mBitmap)
        SAFE_DELETE(mDDS);

    // Clean up linked list
    if (mNext) mNext->mPrev = mPrev;
    if (mPrev) mPrev->mNext = mNext;

    mDead = true;
}

void GFXTextureObject::describeSelf( char* buffer, U32 sizeOfBuffer )
{
    dSprintf(buffer, sizeOfBuffer, " (width: %4d, height: %4d)  profile: %s   creation path: %s", getWidth(),
#if defined(TORQUE_DEBUG) && defined(TORQUE_ENABLE_PROFILER)
        getHeight(), mProfile->getName(), mDebugCreationPath);
#else
             getHeight(), mProfile->getName(), "");
#endif
}