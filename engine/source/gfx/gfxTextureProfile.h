//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef _GFXTEXTUREPROFILE_H_
#define _GFXTEXTUREPROFILE_H_

#include "platform/types.h"
#include "math/mPoint.h"
#include "gfx/gBitmap.h"
#include "gfx/gfxEnums.h"
#include "core/refBase.h"

class GFXTextureObject;

class GFXTextureProfile
{
private:
    StringTableEntry mName; ///< Name of this profile...
    U32 mDownscale;         ///< Amount to shift textures of this type down, if any.
    U32 mProfile;           ///< Stores a munged version of the profile data.
    U32 mExtantCount;       ///< Count of textures of this profile type allocated.
    U32 mExtantTexels;      ///< Amount of texelspace currently allocated under this profile.
    U32 mExtantBytes;       ///< Amount of storage currently allocated under this profile.
    U32 mAllocatedTextures; ///< Total number of textures allocated under this profile.
    U32 mAllocatedTexels;   ///< Total number of texels allocated under this profile.
    U32 mAllocatedBytes;    ///< Total number of bytes allocated under this profile.

    /// Keep a list of all the profiles.
    GFXTextureProfile* mNext;
    static GFXTextureProfile* mHead;

    /// These constants control the packing for the profile; if you add flags, types, or
    /// compression info then make sure these are giving enough bits!
    enum Constants
    {
        TypeBits = 2,
        FlagBits = 9,
        CompressionBits = 3,
    };

public:

    enum Types
    {
        DiffuseMap,
        NormalMap,
        AlphaMap,
        LuminanceMap
    };

    enum Flags
    {
        PreserveSize = BIT(0),  ///< Never shrink this bitmap in low VRAM situations.
        NoMipmap = BIT(1),  ///< Do not generate mipmap chain for this texture.
        SystemMemory = BIT(2),  ///< System memory texture - isn't uploaded to card - useful as target for copying surface data out of video ram
        RenderTarget = BIT(3),  ///< This texture will be used as a render target.
        Dynamic = BIT(4),  ///< This texture may be refreshed. (Precludes Static)
        Static = BIT(5),  ///< This texture will never be modified once loaded. (Precludes Dynamic)
        NoPadding = BIT(6),  ///< Do not pad this texture if it's non pow2.
        KeepBitmap = BIT(7),  ///< Always keep a copy of this texture's bitmap. (Potentially in addition to the API managed copy?)
        ZTarget = BIT(8),  ///< This texture will be used as a Z target.
        RenderTargetZBuffer = BIT(9), ///< This render target that needs a zbuffer as well, this is in addition to RenderTarget flag above (zbuffer comes separately, used for CreateRenderTarget path in D3D)
    };

    enum Compression
    {
        None,
        DXT1,
        DXT2,
        DXT3,
        DXT4,
        DXT5,
    };

    GFXTextureProfile(const char* name, Types type, U32 flags, Compression compression = None);

    // Accessors
    StringTableEntry getName() { return mName; };
    Types getType() { return (Types)(mProfile & (BIT(TypeBits + 1) - 1)); }
    const Compression getCompression() const { return (Compression)((mProfile >> (FlagBits + TypeBits)) & (BIT(CompressionBits + 1) - 1)); };

    bool testFlag(Flags flag)
    {
        return (mProfile & (flag << TypeBits)) != 0;
    }

    // Mutators
    const U32 getDownscale() const { return mDownscale; }
    void setDownscale(const U32 shift) { mDownscale = shift; }
    void incExtantCopies() { mExtantCount++; }
    void decExtantCopies() { AssertFatal(mExtantCount, "Ran out of extant copies!"); mExtantCount--; }

    // And static interface...
    static void init();
    static GFXTextureProfile* find(StringTableEntry name);
    static void updateStatsForCreation(GFXTextureObject* t);
    static void updateStatsForDeletion(GFXTextureObject* t);

    // Helper functions...
    inline bool doStoreBitmap()
    {
        return testFlag(KeepBitmap);
    }

    inline bool canDownscale()
    {
        return !testFlag(PreserveSize);
    }

    inline bool isDynamic()
    {
        return testFlag(Dynamic);
    }

    inline bool isRenderTarget()
    {
        return testFlag(RenderTarget);
    }

    inline bool isRenderTargetZBuffer()
    {
        return (testFlag(RenderTargetZBuffer) && testFlag(RenderTarget));
    }

    inline bool isZTarget()
    {
        return testFlag(ZTarget);
    }

    inline bool isSystemMemory()
    {
        return testFlag(SystemMemory);
    }

    inline bool noMip()
    {
        return testFlag(NoMipmap);
    }

};

#define GFX_DeclareTextureProfile(name)  extern GFXTextureProfile name;
#define GFX_ImplementTextureProfile(name, type,  flags, compression) GFXTextureProfile name(#name, type, flags, compression);

// Set up some defaults..

// Texture we can render to.
GFX_DeclareTextureProfile(GFXDefaultRenderTargetProfile);
// Texture we can render to (with z buffer support)
GFX_DeclareTextureProfile(GFXDefaultRenderTargetZBufferProfile);
// Standard diffuse texture that stays in system memory.
GFX_DeclareTextureProfile(GFXDefaultPersistentProfile);
// Generic diffusemap. This works in most cases.
GFX_DeclareTextureProfile(GFXDefaultStaticDiffuseProfile);
// Texture that resides in system memory - used to copy data to


GFX_DeclareTextureProfile(GFXFFNoiseMapProfile);

GFX_DeclareTextureProfile(GFXSystemMemProfile);
// Depth buffer texture
GFX_DeclareTextureProfile(GFXDefaultZTargetProfile);


#endif