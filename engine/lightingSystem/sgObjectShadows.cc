//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"
#include "sceneGraph/sceneGraph.h"

#include "lightingSystem/sgLighting.h"
#include "lightingSystem/sgLightingModel.h"
#include "lightingSystem/sgObjectShadows.h"

#include "blobShadow.h"
#include "lightingSystem/sgFormatManager.h"

#define SG_UNUSED_TIMEOUT	2000
#define SG_SHADOW_TIMEOUT	3000
// this can be a long time, only used for cleaning
// up texture memory after an intensive scene...
#define SG_TEXTURE_TIMEOUT	30000

//extern SceneGraph* gClientSceneGraph;


sgShadowTextureMultimap sgShadowTextureCache::sgShadowTextures;
Vector<sgObjectShadows*> sgObjectShadowMonitor::sgAllObjectShadows;


bool sgTimeElapsed(U32 currenttime, U32& lasttime, U32 period)
{
    if (currenttime < lasttime)
    {
        lasttime = 0;
        return false;
    }

    if ((currenttime - lasttime) < period)
        return false;

    lasttime = currenttime;
    return true;
}

void sgShadowTextureCache::sgAcquire(GFXTexHandle& texture, Point2I size, U32 format)
{
    U32 hash = calculateCRC(&size.x, (sizeof(size.x) * 2));
    hash = calculateCRC(&format, sizeof(format), hash);

    sgShadowTextureMultimap* entry = sgShadowTextures.find(hash);
    AssertFatal((entry), "No Entry?");

    if (entry->object.size() > 0)
    {
        // copy...
        texture = entry->object.last();
        texture->cacheId = hash;
        // force dereference...
        entry->object.last() = NULL;
        // remove...
        entry->object.decrement(1);
        return;
    }

    texture = GFXTexHandle(size.x, size.y, GFXFormat(format), &ShadowTargetTextureProfile);
    texture->cacheId = hash;

    entry->info.sgFormat = GFXFormat(format);
    entry->info.sgSize = size;
    entry->info.sgCreateCount++;
}

void sgShadowTextureCache::sgRelease(GFXTexHandle& texture)
{
    if (!(GFXTextureObject*)texture)
        return;

    AssertFatal(((GFXTextureObject*)texture), "Bad texture reference!");

    if (texture->cacheId == 0)
    {
        texture = NULL;
        return;
    }

    AssertFatal((texture->mRefCount == 1), "Non unique texture reference!");

    texture->cacheTime = Platform::getRealMilliseconds();

    sgShadowTextureMultimap* entry = sgShadowTextures.find(texture->cacheId);
    // add to the cache...
    entry->object.increment(1);
    // copy...
    entry->object.last() = texture;
    // remove the external reference...
    texture = NULL;
}

sgShadowTextureMultimap* sgShadowTextureCache::sgGetFirstEntry()
{
    return sgShadowTextures.find(0);
}

void sgShadowTextureCache::sgClear()
{
    sgShadowTextureMultimap* entry = sgGetFirstEntry();
    while (entry)
    {
        for (U32 i = 0; i < entry->object.size(); i++)
        {
            // force delete...
            entry->object[i] = NULL;
        }

        entry->object.clear();
        entry->info.sgCreateCount = 0;
        entry = entry->linkHigh;
    }
}

void sgShadowTextureCache::sgCleanupUnused()
{
    // get rid of old stuff...
    // might need to pull textures
    // from the back of the list to
    // prevent textures from staying
    // warm...

    static U32 lasttime = 0;

    U32 time = Platform::getRealMilliseconds();

    if (!sgTimeElapsed(time, lasttime, SG_UNUSED_TIMEOUT))
        return;

    //Con::warnf("Checking for unused textures...");

    // SG_TEXTURE_TIMEOUT...
    sgShadowTextureMultimap* entry = sgGetFirstEntry();
    while (entry)
    {
        for (U32 i = 0; i < entry->object.size(); i++)
        {
            GFXTextureObject* texture = entry->object[i];
            if (!texture)
                continue;

            if (!sgTimeElapsed(time, texture->cacheTime, SG_TEXTURE_TIMEOUT))
                continue;

            // dereference, this should be the last reference, which will kill the texture...
            entry->object[i] = NULL;
            entry->object.erase_fast(i);
            entry->info.sgCreateCount--;
            i--;
        }

        entry = entry->linkHigh;
    }

    sgPrintStats();

    //Con::warnf("Done.");
}

void sgShadowTextureCache::sgPrintStats()
{
    if (!LightManager::sgShowCacheStats)
        return;

    U32 totalalloc = 0;
    U32 totalcache = 0;
    U32 totalallocbytes = 0;
    U32 totalcachebytes = 0;

    Con::warnf("");
    Con::warnf("-------------------------------------");
    Con::warnf("Lighting System - Texture Cache Stats");

    sgShadowTextureMultimap* entry = sgGetFirstEntry();
    while (entry)
    {
        U32 texturebytes = GFX->formatByteSize(entry->info.sgFormat) *
            F32(entry->info.sgSize.x * entry->info.sgSize.y);
        U32 alloc = texturebytes * entry->info.sgCreateCount;
        U32 cache = texturebytes * entry->object.size();

        totalalloc += entry->info.sgCreateCount;
        totalcache += entry->object.size();
        totalallocbytes += alloc;
        totalcachebytes += cache;

        Con::warnf(" %dx%d - alloc: %d, cached: %d, alloc size: %d, cached size: %d",
            entry->info.sgSize.x, entry->info.sgSize.y,
            entry->info.sgCreateCount, entry->object.size(), alloc, cache);

        entry = entry->linkHigh;
    }

    Con::warnf("");
    Con::warnf(" Total - alloc: %d, cached: %d, alloc size: %d, cached size: %d",
        totalalloc, totalcache, totalallocbytes, totalcachebytes);
    Con::warnf("-------------------------------------");
    Con::warnf("");
}

//-----------------------------------------------

void sgObjectShadowMonitor::sgRegister(sgObjectShadows* shadows)
{
    sgAllObjectShadows.push_back(shadows);
}

void sgObjectShadowMonitor::sgUnregister(sgObjectShadows* shadows)
{
    for (U32 i = 0; i < sgAllObjectShadows.size(); i++)
    {
        if (sgAllObjectShadows[i] == shadows)
        {
            sgAllObjectShadows.erase_fast(i);
            return;
        }
    }
}

void sgObjectShadowMonitor::sgCleanupUnused()
{
    static U32 lasttime = 0;

    U32 time = Platform::getRealMilliseconds();
    if (!sgTimeElapsed(time, lasttime, SG_UNUSED_TIMEOUT))
        return;

    //Con::warnf("Checking for unused shadows...");

    for (U32 i = 0; i < sgAllObjectShadows.size(); i++)
        sgAllObjectShadows[i]->sgCleanupUnused(time);

    //Con::warnf("Done.");
}

//-----------------------------------------------

sgObjectShadows::sgObjectShadows()
{
    sgRegistered = false;
    sgLastRenderTime = 0;

    sgSingleShadowSource.mColor = ColorF(0.5, 0.5, 0.5);

    //sgEnable = false;
    //sgCanMove = false;
    //sgCanRTT = false;
    //sgCanSelfShadow = false;
    //sgRequestedShadowSize = 64;
    //sgFrameSkip = 5;
    //sgMaxVisibleDistance = 15.0f;
    //sgProjectionDistance = 7.0f;

    //sgFirstEntry = sgGetFirstShadowEntry();
}

sgObjectShadows::~sgObjectShadows()
{
    sgClearMap();
}

void sgObjectShadows::sgClearMap()
{
    sgShadowMultimap* entry = sgGetFirstShadowEntry();
    while (entry)
    {
        if (entry->info)
        {
            delete entry->info;
            entry->info = NULL;
        }

        entry = entry->linkHigh;
    }

    // all shadows are deleted, so nothing left to monitor...
    if (sgRegistered)
    {
        sgObjectShadowMonitor::sgUnregister(this);
        sgRegistered = false;
    }
}

void sgObjectShadows::sgRender(SceneObject* parentobject, TSShapeInstance* shapeinstance, F32 camdist)
{
    //if(/*!sgEnable ||*/ (camdist > sgMaxVisibleDistance))
    //	return;

    // prior to this no shadows exist, so no resources are used...
    if (!sgRegistered)
    {
        sgObjectShadowMonitor::sgRegister(this);
        sgRegistered = true;
    }

    sgLastRenderTime = Platform::getRealMilliseconds();

    if (!LightManager::sgMultipleDynamicShadows)
    {
        ShadowBase* shadow = sgFindShadow(parentobject, &sgSingleShadowSource, shapeinstance);
        AssertFatal((shadow), "Shadow not found?");
        if (shadow->shouldRender(camdist))
            shadow->render(camdist);
        return;
    }

    bool gotOne = false;
    LightInfoList lights;
    getCurrentClientSceneGraph()->getLightManager()->sgGetBestLights(lights);
    for (U32 i = 0; i < lights.size(); i++)
    {
        LightInfo* light = lights[i];
        if (!light->sgCastsShadows)
            continue;

        //if(light->mType == LightInfo::Ambient)
        //	continue;

        // testing!!!
        //if(light->mType == LightInfo::Vector)
        //	continue;

        ShadowBase* shadow = sgFindShadow(parentobject, light, shapeinstance);
        AssertFatal((shadow), "Shadow not found?");

        if (shadow->shouldRender(camdist))
        {
            shadow->render(camdist);
            gotOne = true;
        }
    }

#ifdef MB_ULTRA
    if (!gotOne)
    {
        LightInfo* light = getCurrentClientSceneGraph()->getLightManager()->sgGetSpecialLight(LightManager::sgSunLightType);
        ShadowBase* shadow = sgFindShadow(parentobject, light, shapeinstance);
        AssertFatal((shadow), "Shadow not found?");

        if (shadow->shouldRender(camdist))
            shadow->render(camdist);
    }
#endif

    // remove dirty flag...
    shapeinstance->shadowDirty = false;
}

void sgObjectShadows::sgCleanupUnused(U32 time)
{
    // wrapped around?
    if (time < sgLastRenderTime)
    {
        sgLastRenderTime = 0;
        return;
    }

    // try to ditch the whole thing first...
    if ((time - sgLastRenderTime) > SG_SHADOW_TIMEOUT)
    {
        //Con::warnf("Found a whole set...");

        sgClearMap();
        return;
    }

    // no? alright lets try to get rid of some old shadows...
    sgShadowMultimap* entry = sgGetFirstShadowEntry();
    while (entry)
    {
        if (entry->info)
        {
            U32 renderTime = entry->info->getLastRenderTime();
            if (sgTimeElapsed(time, renderTime, SG_SHADOW_TIMEOUT))
            {
                //Con::warnf("Found one...");

                delete entry->info;
                entry->info = NULL;
            }
        }

        entry = entry->linkHigh;
    }
}
/*
void sgObjectShadows::sgSetValues(bool enable, bool canmove,
        bool canrtt, bool selfshadow, U32 shadowsize, U32 frameskip,
        F32 maxvisibledist, F32 projectiondist, F32 adjust)
{
    sgEnable = enable;
    sgCanMove = canmove;
    sgCanRTT = canrtt;
    sgCanSelfShadow = selfshadow;
    sgRequestedShadowSize = shadowsize;
    sgFrameSkip = frameskip;
    sgMaxVisibleDistance = maxvisibledist;
    sgProjectionDistance = projectiondist;
    sgSphereAdjust = adjust;

    sgUpdateShadows();
}
*/

ShadowBase* sgObjectShadows::createNewShadow(SceneObject* parentObject, LightInfo* light, TSShapeInstance* shapeInstance)
{
#ifdef MB_ULTRA
    return new BlobShadow(parentObject, light, shapeInstance);
#else
    // Create a shadow
    ShadowBase* shadow = NULL;
    if (GFX->getPixelShaderVersion() < 0.001)
    {
        // No shaders, use a blob shadow
        shadow = new BlobShadow(parentObject, light, shapeInstance);
    }
    else
    {
        // Shaders, use a real shadow
        shadow = new sgShadowProjector(parentObject, light, shapeInstance);
    }
    return shadow;
#endif
}

ShadowBase* sgObjectShadows::sgFindShadow(SceneObject* parentobject,
    LightInfo* light, TSShapeInstance* shapeinstance)
{
    sgShadowMultimap* entry = sgShadows.find(sgLightToHash(light));

    if (entry->info)
        return entry->info;

    // Ack, no shadow! Create one
    ShadowBase* shadow = createNewShadow(parentobject, light, shapeinstance);
    AssertFatal(shadow, "We didn't create a shadow?");
    
    entry->info = shadow;

    //sgUpdateShadow(entry->info);

    return entry->info;
}
/*
void sgObjectShadows::sgUpdateShadow(sgShadowProjector *shadow)
{
    shadow->sgSetEnable(sgEnable);
    shadow->sgSetCanMove(sgCanMove);
    shadow->sgSetCanRTT(sgCanRTT);
    shadow->sgSetCanSelfShadow(sgCanSelfShadow);
    shadow->sgSetShadowSize(sgRequestedShadowSize);
    shadow->sgSetFrameSkip(sgFrameSkip);
    shadow->sgSetMaxVisibleDistance(sgMaxVisibleDistance);
    shadow->sgSetProjectionDistance(sgProjectionDistance);
    shadow->sgSetSphereAdjust(sgSphereAdjust);
}

void sgObjectShadows::sgUpdateShadows()
{
    sgShadowMultimap *entry = sgGetFirstShadowEntry();
    while(entry)
    {
        if(entry->info)
            sgUpdateShadow(entry->info);

        entry = entry->linkHigh;
    }
}
*/
sgShadowMultimap* sgObjectShadows::sgGetFirstShadowEntry()
{
    return sgShadows.find(sgosFirstEntryHash);
}

