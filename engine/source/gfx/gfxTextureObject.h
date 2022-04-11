//-----------------------------------------------------------------------------
// Torque Shader Engine
//
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef _GFX_Texture_Object_H_
#define _GFX_Texture_Object_H_

#include "platform/types.h"
#include "math/mPoint.h"
#include "gfx/gBitmap.h"
#include "gfx/gfxEnums.h"
#include "gfx/gfxTextureProfile.h"
#include "core/refBase.h"
#include "gfx/gfxResource.h"

class GFXDevice;
class GFXTextureProfile;
class DDSFile;

/// Contains information on a locked region of a texture.
///
/// In general, to access a given pixel in a locked rectangle, use this
/// equation:
///
/// @code
///     U8 *pixelAtXY = bits + x * pitch + y * pixelSizeInBytes;
/// @endcode
///
/// @note D3DLOCKED_RECT and this structure match up. If you change this
///       assumption, be sure to revisit the D3D GFX implementation.
///
/// @see GFXTextureObject::lock() 
struct GFXLockedRect
{
    /// Pitch of the lock. This is the spacing in bytes of the start
    /// of each row of the locked region.
    int pitch;

    /// Pointer to the start of locked rectangle.
    U8* bits;
};


class GFXTextureObject : public RefBase, public GFXResource
{
public:

#ifdef TORQUE_DEBUG
    // In debug builds we provide a TOC leak tracking system.
    static U32 smExtantTOCount;
    static GFXTextureObject* smHead;
    static void dumpExtantTOs();

    StringTableEntry mDebugCreationPath;
    GFXTextureObject* mDebugNext;
    GFXTextureObject* mDebugPrev;
#endif

    bool mDead;

    U32 cacheId;
    U32 cacheTime;

    // Linked List management
    GFXDevice* mDevice;   ///< Device this texture belongs to.
    GFXTextureObject* mNext;     ///< Next texture in the linked list
    GFXTextureObject* mPrev;     ///< Previous texture in the linked list
    GFXTextureObject* mHashNext; ///< Used for hash table lookups.

    StringTableEntry mTextureFileName;

    Point3I  mBitmapSize;
    Point3I  mTextureSize;
    U32      mMipLevels;

    GBitmap* mBitmap;   ///< GBitmap we are backed by.
    DDSFile* mDDS;      ///< DDSFile we're backed by.

    GFXTextureProfile* mProfile;
    GFXFormat          mFormat;


    GFXTextureObject(GFXDevice* aDevice, GFXTextureProfile* profile);
    virtual ~GFXTextureObject();

    GBitmap* getBitmap();
    DDSFile* getDDS();
    const U32 getWidth() const;
    const U32 getHeight() const;
    const U32 getDepth() const;
    const U32 getBitmapWidth() const;
    const U32 getBitmapHeight() const;
    const U32 getBitmapDepth() const;

    /// Acquire a lock on part of the texture. The GFXLockedRect returned
    /// is managed by the GFXTextureObject and does not need to be freed.
    virtual GFXLockedRect* lock(U32 mipLevel = 0, RectI* inRect = NULL) = 0;

    /// Releases a lock previously acquired. Note that the mipLevel parameter
    /// must match the corresponding lock!
    virtual void unlock(U32 mipLevel = 0) = 0;

    /// Copy the texture data into the specified bitmap.
    ///   - this texture object must be a render target.  the function will assert if this is not the case.
    ///   - you must have called allocateBitmap() on the input bitmap first.  the bitmap should have the 
    ///     same dimensions as this texture.  the bitmap format can be RGB or RGBA (in the latter case
    ///     the alpha values from the texture are copied too)
    ///   - returns true if successful, false otherwise
    ///   - this process is not fast.
    virtual bool copyToBmp(GBitmap* bmp) = 0;

#ifdef TORQUE_DEBUG
    /// It is important for any derived objects to define this method
    /// and also call 'kill' from their destructors.  If you fail to
    /// do either, you will get a pure virtual function call crash
    /// in debug mode.  This is a precaution to make sure you don't
    /// forget to add 'kill' to your destructor.
    virtual void pureVirtualCrash() = 0;
#endif

    virtual void kill();

    // GFXResource interface
    virtual void describeSelf(char* buffer, U32 sizeOfBuffer);
};

//-----------------------------------------------------------------------------

inline GBitmap* GFXTextureObject::getBitmap()
{
    AssertFatal(mProfile->doStoreBitmap(), avar("GFXTextureObject::getBitmap - Cannot access bitmap for a '%s' texture.", mProfile->getName()));

    return mBitmap;
}

inline DDSFile* GFXTextureObject::getDDS()
{
    AssertFatal(mProfile->doStoreBitmap(), avar("GFXTextureObject::getDDS - Cannot access bitmap for a '%s' texture.", mProfile->getName()));

    return mDDS;
}

inline const U32 GFXTextureObject::getWidth() const
{
    return mTextureSize.x;
}

inline const U32 GFXTextureObject::getHeight() const
{
    return mTextureSize.y;
}

inline const U32 GFXTextureObject::getDepth() const
{
    return mTextureSize.z;
}

inline const U32 GFXTextureObject::getBitmapWidth() const
{
    return mBitmapSize.x;
}

inline const U32 GFXTextureObject::getBitmapHeight() const
{
    return mBitmapSize.y;
}

inline const U32 GFXTextureObject::getBitmapDepth() const
{
    return mBitmapSize.z;
}

#endif
