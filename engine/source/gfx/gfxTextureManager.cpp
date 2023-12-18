//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gfx/gfxTextureManager.h"
#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gui/core/guiCanvas.h"
#include "math/mathUtils.h"

/// Threshold of total VRAM under which we start scaling textures down...
///
/// Textures are scaled by powers of 2, so each integer increase in the scale values
/// results in textures of half the width and half the height, or a quarter of the 
/// source art's size, to be allocated.
///
/// @note We set this a little bit above the actual value we want to run at low quality
///       mode in, so that we are sure of actually being in low-quality mode on that hardware.
U32 gTextureScaleThreshold = 67108864;  // 64 MB

// 0 == auto, 1 == low, 2 == high
S32 gTextureQualityMode = 0;

// 0 == none, 1 == 1/(4^1), 2 == 1/(4^2), 3 = 1/(4^3)
S32 gTextureReductionLevel = 1;

//-----------------------------------------------------------------------------

void GFXTextureManager::init()
{
    Con::addVariable("pref::TextureManager::scaleThreshold", TypeS32, &gTextureScaleThreshold);
    Con::addVariable("pref::TextureManager::qualityMode", TypeS32, &gTextureQualityMode);
    Con::addVariable("pref::TextureManager::reductionLevel", TypeS32, &gTextureReductionLevel);
}

GFXTextureManager::GFXTextureManager()
{
    mListHead = mListTail = NULL;
    mTextureManagerState = GFXTextureManager::Living;

    // Set up the hash table
    mHashCount = 1023;
    mHashTable = new GFXTextureObject * [mHashCount];
    for (U32 i = 0; i < mHashCount; i++)
        mHashTable[i] = NULL;

    mValidTextureQualityInfo = false;
    mHandleCount = 0;
}

//-----------------------------------------------------------------------------

GFXTextureManager::~GFXTextureManager()
{
    delete[] mHashTable;
}

//-----------------------------------------------------------------------------
void GFXTextureManager::validateTextureMemory()
{
    PROFILE_START(GFXTextureManager_validateTextureMemory);

    if (mValidTextureQualityInfo)
    {
        PROFILE_END();
        return;
    }

    if (getTotalVideoMemory() == 0)
    {
        PROFILE_END();
        return;
    }

    // Let the user know what texture strategy we're using...
    Con::printf("Texture Manager");
    Con::printf("   - Approx. Available VRAM:  %d", getTotalVideoMemory());
    Con::printf("   - Threshold VRAM:  %d", gTextureScaleThreshold);

    const char* qualityMode;

    // Use different heuristics based on the global...
    bool force = false;

    switch (gTextureQualityMode)
    {
    case 0:
        mAboveTextureThreshold = (getTotalVideoMemory() > gTextureScaleThreshold);
        break;

    case 1:
        force = true;
        mAboveTextureThreshold = false;
        break;

    case 2:
        force = true;
        mAboveTextureThreshold = true;
        break;
    }

    if (mAboveTextureThreshold)
        qualityMode = "high";
    else
        qualityMode = "low";

    Con::printf("   - Quality mode: %s%s", qualityMode, (force ? " (forced)" : ""));

    mValidTextureQualityInfo = true;

    PROFILE_END();
}

U32 GFXTextureManager::getBitmapScalePower(GFXTextureProfile* profile)
{
    validateTextureMemory();

    if (mAboveTextureThreshold)
    {
        return 0;
    }

    if (profile->canDownscale())
    {
        return gTextureReductionLevel;
    }

    return 0;
}

bool GFXTextureManager::validateTextureQuality(GFXTextureProfile* profile, U32& width, U32& height)
{
    U32 scaleFactor;
    if ((scaleFactor = getBitmapScalePower(profile)) == 0)
        return true;

    // Otherwise apply the appropriate scale...
    width <<= scaleFactor;
    height <<= scaleFactor;

    return true;

}
//-----------------------------------------------------------------------------
void GFXTextureManager::kill()
{
    AssertFatal(mTextureManagerState != GFXTextureManager::Dead, "Don't beat a dead texture manager!");

    GFXTextureObject* curr = mListHead;
    GFXTextureObject* temp;

    // Actually delete all the textures we know about.
    while (curr != NULL)
    {
        temp = curr->mNext;
        curr->kill();
        curr = temp;
    }

    mTextureManagerState = GFXTextureManager::Dead;
}

//-----------------------------------------------------------------------------
void GFXTextureManager::zombify()
{
    AssertFatal(mTextureManagerState != GFXTextureManager::Zombie, "Texture Manager already a zombie! Get the holy water!");

    GFXTextureObject* temp = mListHead;

    // Free all the device copies of the textures.
    while (temp != NULL)
    {
        freeTexture(temp, true);
        temp = temp->mNext;
    }

    // Notify everyone that cares about the zombification!
    for (U32 i = 0; i < mEventCallbackList.size(); i++)
    {
        mEventCallbackList[i].callback(GFXZombify, mEventCallbackList[i].userData);
    }

    // Finally, note our state.
    mTextureManagerState = GFXTextureManager::Zombie;
}

//-----------------------------------------------------------------------------
void GFXTextureManager::resurrect()
{
    // Reupload all the device copies of the textures.
    GFXTextureObject* temp = mListHead;

    while (temp != NULL)
    {
        refreshTexture(temp);

        temp = temp->mNext;
    }

    // Notify callback registries.
    for (U32 i = 0; i < mEventCallbackList.size(); i++)
    {
        mEventCallbackList[i].callback(GFXResurrect, mEventCallbackList[i].userData);
    }

    // Update our state.
    mTextureManagerState = GFXTextureManager::Living;
}

//-----------------------------------------------------------------------------

GFXTextureObject* GFXTextureManager::createTexture(GBitmap* bmp, GFXTextureProfile* profile, bool deleteBmp)
{
    PROFILE_START(GFXTextureManager_createTexture2);
    AssertWarn(bmp, "NULL GBitmap passed to GFXTextureManager::createTexture.");

    // Check the cache first...
    GFXTextureObject* cacheHit;
    StringTableEntry fileName = bmp->mSourceResource ? bmp->mSourceResource->getFullPath() : NULL;

    if (cacheHit = hashFind(fileName))
    {
        // Con::errorf("Cached texture '%s'", (fileName ? fileName : "unknown"));
        if (deleteBmp)
            delete bmp;
        PROFILE_END();
        return cacheHit;
    }

    // Massage the bitmap based on any resize rules.
    U32 scalePower = getBitmapScalePower(profile);

    GBitmap* realBmp = bmp;
    U32 realWidth = bmp->getWidth();
    U32 realHeight = bmp->getHeight();

    if (scalePower && isPow2(bmp->getWidth()) && isPow2(bmp->getHeight()) && profile->canDownscale())
    {
        // We only work with power of 2 textures for now, so we don't have
        // to worry about padding...

        // If createPaddedBitmap is added back in, be sure to comment back in the
        // call to delete padBmp below.
        GBitmap* padBmp = bmp; //->createPaddedBitmap(); // createPaddedBitmap(bmp);
        realWidth = padBmp->getWidth() >> scalePower;
        realHeight = padBmp->getHeight() >> scalePower;

        if (realHeight == 0)   realHeight = 1;
        if (realWidth == 0)    realWidth = 1;

        realBmp = new GBitmap(realWidth, realHeight, false, bmp->getFormat());

        padBmp->extrudeMipLevels();

        // Copy to the new bitmap...
        dMemcpy(
            realBmp->getWritableBits(), padBmp->getBits(scalePower),
            padBmp->bytesPerPixel * realWidth * realHeight
        );

        // This line is commented out because createPaddedBitmap is commented out.
        // If that line is added back in, this line should be added back in.
        // delete padBmp;
    }

    // Call the internal create... (use the real* variables now, as they
    // reflect the reality of the texture we are creating.)
    U32 numMips = 0;
    validateTexParams(realWidth, realHeight, profile, numMips);

    GFXTextureObject* ret = _createTexture(realHeight, realWidth, 0, realBmp->getFormat(), profile, numMips);

    if (!ret)
    {
        Con::errorf("GFXTextureManager - failed to create texture (1) for '%s'", (fileName ? fileName : "unknown"));
        PROFILE_END();
        return NULL;
    }

    // Call the internal load...
    if (!_loadTexture(ret, realBmp))
    {
        Con::errorf("GFXTextureManager - failed to load GBitmap for '%s'", (fileName ? fileName : "unknown"));
        PROFILE_END();
        return NULL;
    }

    // Do statistics and book-keeping...

    //    - info for the texture...
    ret->mTextureFileName = fileName;
    ret->mBitmapSize.set(realWidth, realHeight, 0);

    if (profile->doStoreBitmap())
    {
        // NOTE: may store a downscaled copy!
        ret->mBitmap = new GBitmap(*realBmp);
    }

    linkTexture(ret);

    //    - output debug info?
    // Save texture for debug purpose
    //   static int texId = 0;
    //   char buff[256];
    //   dSprintf(buff, sizeof(buff), "tex_%d", texId++);
    //   bmp->writePNGDebug(buff);
    //   texId++;


    // Some final cleanup...
    if (realBmp != bmp)
        delete realBmp;
    if (deleteBmp)
        delete bmp;

    Canvas->RefreshAndRepaint();

    PROFILE_END();

    // Return the new texture!
    return ret;
}


GFXTextureObject* GFXTextureManager::createTexture(DDSFile* dds, GFXTextureProfile* profile, bool deleteDDS)
{
    AssertWarn(dds, "GFXTextureManager::createTexture - NULL DDS passed to GFXTextureManager::createTexture.");

    // Check the cache first...
 /*   GFXTextureObject *cacheHit;
    StringTableEntry fileName = bmp->mSourceResource ? bmp->mSourceResource->getFullPath() : NULL;

    if(cacheHit = hashFind(fileName))
    {
       //      Con::errorf("Cached texture '%s'", (fileName ? fileName : "unknown"));
       return cacheHit;
    }*/ // For now we assume that all DDS loads are actually necessary.

    // Ignore padding from the profile.

    char* fileName = NULL;

    U32 numMips = dds->mMipMapCount;
    validateTexParams(dds->getHeight(), dds->getWidth(), profile, numMips);

    // Call the internal create... (use the real* variables now, as they
    // reflect the reality of the texture we are creating.)
    GFXTextureObject* ret =
        _createTexture(dds->getHeight(), dds->getWidth(), 0, dds->mFormat,
            profile, numMips, true);

    if (!ret)
    {
        Con::errorf("GFXTextureManager - failed to create texture (1) for '%s' DDSFile.", (fileName ? fileName : "unknown"));
        return NULL;
    }

    // Call the internal load...
    if (!_loadTexture(ret, dds))
    {
        Con::errorf("GFXTextureManager - failed to load DDS for '%s'", (fileName ? fileName : "unknown"));
        return NULL;
    }

    // Do statistics and book-keeping...

    //    - info for the texture...
    ret->mTextureFileName = NULL;
    ret->mBitmapSize.set(dds->mHeight, dds->mWidth, 0);

    if (profile->doStoreBitmap())
    {
        // NOTE: may store a downscaled copy!
        ret->mDDS = new DDSFile(*dds);
    }

    linkTexture(ret);

    //    - output debug info?
    // Save texture for debug purpose
    //   static int texId = 0;
    //   char buff[256];
    //   dSprintf(buff, sizeof(buff), "tex_%d", texId++);
    //   bmp->writePNGDebug(buff);
    //   texId++;

    if (deleteDDS)
        delete dds;

    // Return the new texture!
    return ret;
}

GFXTextureObject* GFXTextureManager::createTexture(const char* filename, GFXTextureProfile* profile)
{
    PROFILE_START(GFXTextureManager_createTexture);
    // hack to load .dds files until proper support is in
//    if (dStrstr(filename, ".dds"))
//    {
//        GFXTextureObject* obj = _loadDDSHack(filename, profile);
//        if (!obj) return NULL;
//
//        linkTexture(obj);
//
//        // Return the new texture!
//        PROFILE_END();
//        return obj;
//
//    }

    // Check the cache first...
    ResourceObject* ro = GBitmap::findBmpResource(filename);
    if (ro)
    {
        StringTableEntry fileName = ro->getFullPath();
        GFXTextureObject* cacheHit = hashFind(fileName);
        if (cacheHit)
        {
            PROFILE_END();
            return cacheHit;
        }
    }

    // Find and load the texture.
    GBitmap* bmp = GBitmap::load(filename);

    if (!bmp)
    {
        //      Con::errorf("GFXTextureManager::createTexture - failed to load bitmap '%s'", filename);

        PROFILE_END();
        return NULL;
    }

    GFXTextureObject* ret = createTexture(bmp, profile, true);
    
    PROFILE_END();
    return ret;
}

GFXTextureObject* GFXTextureManager::createTexture(U32 width, U32 height, void* pixels, GFXFormat format, GFXTextureProfile* profile)
{
    // For now, stuff everything into a GBitmap and pass it off... This may need to be revisited -- BJG
    GBitmap* bmp = new GBitmap(width, height, 0, format);
    dMemcpy(bmp->getWritableBits(), pixels, width * height * bmp->bytesPerPixel);
    return createTexture(bmp, profile, true);
}

GFXTextureObject* GFXTextureManager::createTexture(U32 width, U32 height, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels)
{
    // Deal with sizing issues...
    U32 localWidth = width;
    U32 localHeight = height;

    validateTextureQuality(profile, localWidth, localHeight);

    U32 numMips = numMipLevels;
    validateTexParams(localWidth, localHeight, profile, numMips);

    // Create texture...
    GFXTextureObject* ret = _createTexture(localHeight, localWidth, 0, format, profile, numMips);

    if (!ret)
    {
        Con::errorf("GFXTextureManager - failed to create anonymous texture.");
        return NULL;
    }

    // And do book-keeping...
    //    - texture info
    ret->mBitmapSize.set(localWidth, localHeight, 0);

    linkTexture(ret);

    return ret;
}

//-----------------------------------------------------------------------------
// createTexture - 3D volume
//-----------------------------------------------------------------------------
GFXTextureObject* GFXTextureManager::createTexture(U32 width,
    U32 height,
    U32 depth,
    void* pixels,
    GFXFormat format,
    GFXTextureProfile* profile)
{
    // Create texture...
    GFXTextureObject* ret = _createTexture(height, width, depth, format, profile, 1);

    if (!ret)
    {
        Con::errorf("GFXTextureManager - failed to create anonymous texture.");
        return NULL;
    }

    // Call the internal load...
    if (!_loadTexture(ret, pixels))
    {
        Con::errorf("GFXTextureManager - failed to load volume texture");
        return NULL;
    }


    // And do book-keeping...
    //    - texture info
    ret->mBitmapSize.set(width, height, depth);

    linkTexture(ret);


    // Return the new texture!
    return ret;
}

//-----------------------------------------------------------------------------

void GFXTextureManager::hashInsert(GFXTextureObject* object)
{
    if (object->mTextureFileName)
    {
        U32 key = StringTable->hashString(object->mTextureFileName) % mHashCount;

        object->mHashNext = mHashTable[key];
        mHashTable[key] = object;
    }
}

void GFXTextureManager::hashRemove(GFXTextureObject* object)
{
    // Don't hash stuff with no name.
    if (!object->mTextureFileName) return;

    U32 key = StringTable->hashString(object->mTextureFileName) % mHashCount;
    GFXTextureObject** walk = &mHashTable[key];
    while (*walk)
    {
        if (*walk == object)
        {
            *walk = object->mHashNext;
            break;
        }
        walk = &((*walk)->mHashNext);
    }
}

GFXTextureObject* GFXTextureManager::hashFind(StringTableEntry name)
{
    PROFILE_START(GFXTextureManager_hashFind);
    U32 key = StringTable->hashString(name) % mHashCount;
    GFXTextureObject* walk = mHashTable[key];
    for (; walk; walk = walk->mHashNext)
    {
        if (!dStrcmp(walk->mTextureFileName, name))
        {
            break;
        }
    }
    PROFILE_END();
    return walk;
}


//-----------------------------------------------------------------------------
// Register texture event callback
//-----------------------------------------------------------------------------
void GFXTextureManager::registerTexCallback(GFXTexEventCallback callback,
    void* userData,
    S32& handle)
{
    U32 size = mEventCallbackList.size();

    // Check to see if the callback already exists!
    bool exists = false;
    for (U32 i = 0; i < size; i++)
    {
        if (mEventCallbackList[i].callback == callback &&
            mEventCallbackList[i].userData == userData)
        {
            exists = true;
            break;
        }
    }

    if (exists)
    {
        // It did, so just return a garbage handle.
        handle = -1;
        return;
    }

    // Otherwise, register the callback.
    CallbackData data;
    data.callback = callback;
    data.userData = userData;
    handle = data.handle = mHandleCount;

    mEventCallbackList.push_back(data);
    mHandleCount++;
}

//-----------------------------------------------------------------------------
// Unregister texture event callback.  Pass in the same pointer that was
// registered.
//-----------------------------------------------------------------------------
void GFXTextureManager::unregisterTexCallback(S32 handle)
{
    // Check for bad handles.
    if (handle == -1) return;

    // Go through list and remove callback.
    U32 size = mEventCallbackList.size();
    for (U32 i = 0; i < size; i++)
    {
        if (mEventCallbackList[i].handle == handle)
        {
            mEventCallbackList[i] = mEventCallbackList.last();
            mEventCallbackList.decrement();
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void GFXTextureManager::freeTexture(GFXTextureObject* texture, bool zombify)
{
    // Ok, let the backend deal with it.
    _freeTexture(texture, zombify);
}

void GFXTextureManager::refreshTexture(GFXTextureObject* texture)
{
    _refreshTexture(texture);
}

//-----------------------------------------------------------------------------
// store texture (in Texture Manager) - add to linked list
//-----------------------------------------------------------------------------
void GFXTextureManager::linkTexture(GFXTextureObject* obj)
{
    PROFILE_START(GFXTextureManager_linkTexture);

    //    - info for the profile...
    GFXTextureProfile::updateStatsForCreation(obj);

    //    - info for the cache...
    hashInsert(obj);

    //    - info for the master list...
    if (mListHead == NULL)
        mListHead = obj;

    if (mListTail != NULL)
    {
        mListTail->mNext = obj;
    }

    obj->mPrev = mListTail;
    mListTail = obj;

    PROFILE_END();
}

//-----------------------------------------------------------------------------
// Validate the parameters for creating a texture
//-----------------------------------------------------------------------------
void GFXTextureManager::validateTexParams(U32 width, U32 height,
    GFXTextureProfile* profile,
    U32& numMips)
{
    if (profile->noMip())
    {
        numMips = 1;  // no mipmap - just the one at the top level
    }

    // If a texture is not power-of-2 in size for both dimensions, it must
    // have only 1 mip level.
    if (!MathUtils::isPow2(width) || !MathUtils::isPow2(height))
    {
        numMips = 1;
    }

}

//-----------------------------------------------------------------------------
// Reloads texture resource from disk
//-----------------------------------------------------------------------------
void GFXTextureManager::reloadTextureResource(const char* filename)
{
    // Find and load the texture.
    GBitmap* bmp = GBitmap::load(filename);

    if (bmp)
    {
        GFXTextureObject* obj = hashFind(filename);
        if (obj)
        {
            _loadTexture(obj, bmp);
        }
    }
}

//------------------------------------------------------------------------------
// Console functions
//------------------------------------------------------------------------------
ConsoleFunction(reloadTextures, void, 1, 1, "")
{
    if (ResourceManager)
    {
        ResourceManager->startResourceTraverse();
        ResourceObject* obj;

        // loop through all resources
        while (obj = ResourceManager->getNextResource())
        {
            if (obj->crc != InvalidCRC)
            {
                Stream* stream = ResourceManager->openStream(obj);
                if (stream)
                {
                    U32 crc = calculateCRCStream(stream, InvalidCRC);

                    // file has changed, reload it
                    if (crc != obj->crc)
                    {
                        Con::errorf("Changed file: %s/%s, reloading", obj->path, obj->name);
                        char filename[256];
                        dStrcpy(filename, obj->path);
                        dStrcat(filename, "/");
                        dStrcat(filename, obj->name);
                        ResourceManager->reload(filename, true);
                        GFX->reloadTextureResource(filename);
                    }
                    ResourceManager->closeStream(stream);
                }
            }

        }
    }
}

ConsoleFunction(preloadTexture, void, 2, 2, "preloadTexture(filename)")
{
    static Vector<GFXTexHandle*> sPreloadGuiTextureVector;

    static char buffer[1024];
    Con::expandScriptFilename(buffer, 1024, argv[1]);

    GFXTexHandle* tex = new GFXTexHandle(buffer, &GFXDefaultGUIProfile);
    sPreloadGuiTextureVector.push_back(tex);
}





