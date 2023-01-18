//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef GFX_Texture_Manager_H_
#define GFX_Texture_Manager_H_

#include "gfx/gfxTextureObject.h"
#include "gBitmap.h"
#include "console/console.h"
#include "gfx/ddsFile.h"

typedef void (*GFXTexEventCallback)(GFXTexCallbackCode code, void* userData);

class GFXTextureManager
{
private:
    U32 mHandleCount;

    struct CallbackData
    {
        GFXTexEventCallback callback;
        void* userData;
        S32 handle;
    };

    Vector <CallbackData> mEventCallbackList;

protected:
    //-----------------------------------------------------------------------
    // General texture management data
    //-----------------------------------------------------------------------
    bool mAboveTextureThreshold;
    bool mValidTextureQualityInfo;

    GFXTextureObject* mListHead;
    GFXTextureObject* mListTail;

    // We have a hash table for fast texture lookups
    GFXTextureObject** mHashTable;
    U32                mHashCount;
    GFXTextureObject* hashFind(StringTableEntry name);
    void              hashInsert(GFXTextureObject* object);
    void              hashRemove(GFXTextureObject* object);

    enum TextureManagerState
    {
        Living,
        Zombie,
        Dead
    } mTextureManagerState;

    //-----------------------------------------------------------------------
    // Protected methods
    //-----------------------------------------------------------------------

    /// Frees the API handles to the texture, for D3D this is a release call
    ///
    /// @note freeTexture MUST NOT DELETE THE TEXTURE OBJECT
    virtual void freeTexture(GFXTextureObject* texture, bool zombify = false);

    virtual void refreshTexture(GFXTextureObject* texture);

    /// @group Internal Texture Manager Interface
    ///
    /// These pure virtual functions are overloaded by each API-specific
    /// subclass.
    ///
    /// The order of calls is:
    /// @code
    /// _createTexture()
    /// _loadTexture
    /// _refreshTexture()
    /// _refreshTexture()
    /// _refreshTexture()
    /// ...
    /// _freeTexture()
    /// @endcode
    ///
    /// @{

    /// Allocate a texture with the internal API.
    ///
    /// @param  height   Height of the texture.
    /// @param  width    Width of the texture.
    /// @param  depth    Depth of the texture. (Will normally be 1 unless
    ///                  we are doing a cubemap or volumetexture.)
    /// @param  format   Pixel format of the texture.
    /// @param  profile  Profile for the texture.
    /// @param  numMipLevels   If not-NULL, then use that many mips.
    virtual GFXTextureObject* _createTexture(U32 height, U32 width, U32 depth, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels, bool forceMips = false) = 0;

    /// hack to load .dds files using DirectX
    //virtual GFXTextureObject* _loadDDSHack(const char* filename, GFXTextureProfile* profile) = 0;

    /// Load a texture from a proper DDSFile instance.
    virtual bool _loadTexture(GFXTextureObject* texture, DDSFile* dds) = 0;

    /// Load data into a texture from a GBitmap using the internal API.
    virtual bool _loadTexture(GFXTextureObject* texture, GBitmap* bmp) = 0;

    /// Load data into a texture from a raw buffer using the internal API.
    ///
    /// Note that the size of the buffer is assumed from the parameters used
    /// for this GFXTextureObject's _createTexture call.
    virtual bool _loadTexture(GFXTextureObject* texture, void* raw) = 0;

    /// Refresh a texture using the internal API.
    virtual bool _refreshTexture(GFXTextureObject* texture) = 0;

    /// Free a texture (but do not delete the GFXTextureObject) using the internal
    /// API.
    ///
    /// This is only called during zombification for textures which need it, so you
    /// don't need to do any internal safety checks.
    virtual bool _freeTexture(GFXTextureObject* texture, bool zombify = false) = 0;

    /// Returns available VRAM in bytes.
    virtual U32 _getTotalVideoMemory() = 0;

    /// @}

    void validateTextureMemory();
    void linkTexture(GFXTextureObject* obj);
    void validateTexParams(U32 width, U32 height, GFXTextureProfile* profile, U32& numMips);

public:
    GFXTextureManager();
    virtual ~GFXTextureManager();

    /// Set up some global script interface stuff.
    static void init();

    U32 getTotalVideoMemory()
    {
        return _getTotalVideoMemory();
    }

    /// Update width and height based on available resources.
    ///
    /// We provide a simple interface for managing texture memory usage. Specifically,
    /// if the total video memory is below a certain threshold, we scale all texture
    /// resolutions down by a specific factor (you can specify different scale factors
    /// for different types of textures).
    ///
    /// @note The base GFXTextureManager class provides all the logic to do this scaling. 
    ///       Subclasses need only implement getTotalVideoMemory().
    ///
    /// @param  type     Type of the requested texture. This is used to determine scaling factors.
    /// @param  width    Requested width - is changed to the actual width that should be used.
    /// @param  height   Requested height - is changed to the actual height that should be used.
    /// @return True if the texture request should be granted, false otherwise.
    virtual bool validateTextureQuality(GFXTextureProfile* profile, U32& width, U32& height);

    U32 getBitmapScalePower(GFXTextureProfile* profile);

    virtual GFXTextureObject* createTexture(GBitmap* bmp,
        GFXTextureProfile* profile,
        bool deleteBmp);

    virtual GFXTextureObject* createTexture(DDSFile* dds,
        GFXTextureProfile* profile,
        bool deleteDDS);

    virtual GFXTextureObject* createTexture(const char* filename,
        GFXTextureProfile* profile);


    virtual GFXTextureObject* createTexture(U32 width,
        U32 height,
        void* pixels,
        GFXFormat format,
        GFXTextureProfile* profile);

    virtual GFXTextureObject* createTexture(U32 width,
        U32 height,
        U32 depth,
        void* pixels,
        GFXFormat format,
        GFXTextureProfile* profile);

    virtual GFXTextureObject* createTexture(U32 width,
        U32 height,
        GFXFormat format,
        GFXTextureProfile* profile,
        U32 numMipLevels = 0);

    void deleteTexture(GFXTextureObject* texture);
    void reloadTexture(GFXTextureObject* texture);
    void reloadTextureResource(const char* filename);

    /// @name Texture Necromancy
    /// 
    /// Texture necromancy in three easy steps:
    /// - If you want to destroy the texture manager, call kill().
    /// - If you want to switch resolutions, or otherwise reset the device, call zombify().
    /// - When you want to bring the manager back from zombie state, call resurrect().
    ///
    /// @{

    ///
    void kill();
    void zombify();
    void resurrect();

    void registerTexCallback(GFXTexEventCallback, void* userData, S32& handle);
    void unregisterTexCallback(S32 handle);

    /// @}
};

//-----------------------------------------------------------------------------

inline void GFXTextureManager::deleteTexture(GFXTextureObject* texture)
{
    if (mTextureManagerState == GFXTextureManager::Dead)
    {
        return;
    }

    if (mListHead == texture)
        mListHead = texture->mNext;
    if (mListTail == texture)
        mListTail = texture->mPrev;

    hashRemove(texture);

    GFXTextureProfile::updateStatsForDeletion(texture);

    freeTexture(texture);
}

inline void GFXTextureManager::reloadTexture(GFXTextureObject* texture)
{
    refreshTexture(texture);
}

#endif
