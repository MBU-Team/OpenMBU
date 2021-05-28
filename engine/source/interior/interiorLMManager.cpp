//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interiorLMManager.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/gBitmap.h"
#include "interior/interiorRes.h"
#include "interior/interiorInstance.h"
#include "interior/interior.h"
#include "sceneGraph/sceneLighting.h"

//------------------------------------------------------------------------------
// Globals
InteriorLMManager   gInteriorLMManager;

//------------------------------------------------------------------------------
namespace {
    void interiorLMTextureCallback(const U32 eventCode, void* userData)
    {
        AssertFatal(&gInteriorLMManager == reinterpret_cast<InteriorLMManager*>(userData), "Bunk ptr!");
        gInteriorLMManager.processTextureEvent(eventCode);
    }

    // '<interior>_<instance index>_lm_<lightmap index>.png'
    const char* getTextureName(Interior* interior, U32 instance, U32 lightmap)
    {
        static char buffer[256];
        dSprintf(buffer, sizeof(buffer), "%p_%d_lm_%d.png", interior, instance, lightmap);
        return(buffer);
    }
}

//------------------------------------------------------------------------------
U32 InteriorLMManager::smTextureCallbackKey = U32(-1);

// D3D vertex buffers for Interiors are in here so they get dumped/reallocated
// along with the lightmaps on the texture events
S32 InteriorLMManager::smMTVertexBuffer = -1;
S32 InteriorLMManager::smFTVertexBuffer = -1;
S32 InteriorLMManager::smFMTVertexBuffer = -1;

InteriorLMManager::InteriorLMManager()
{
}

InteriorLMManager::~InteriorLMManager()
{
    for (U32 i = 0; i < mInteriors.size(); i++)
        removeInterior(LM_HANDLE(i));
}

//------------------------------------------------------------------------------
void InteriorLMManager::init()
{
    //   smTextureCallbackKey = TextureManager::registerEventCallback(interiorLMTextureCallback, &gInteriorLMManager);
}

void InteriorLMManager::destroy()
{
    /*
       if(smTextureCallbackKey != U32(-1))
       {
          TextureManager::unregisterEventCallback(smTextureCallbackKey);
          smTextureCallbackKey = U32(-1);
       }

        if (smMTVertexBuffer != -1)
        {
            if (dglDoesSupportVertexBuffer())
                glFreeVertexBufferEXT(smMTVertexBuffer);
            else
                AssertFatal(false,"Vertex buffer should have already been freed!");
            smMTVertexBuffer = -1;
        }
        if (smFTVertexBuffer != -1)
        {
            if (dglDoesSupportVertexBuffer())
                glFreeVertexBufferEXT(smFTVertexBuffer);
            else
                AssertFatal(false,"Vertex buffer should have already been freed!");
            smFTVertexBuffer = -1;
        }
        if (smFMTVertexBuffer != -1)
        {
            if (dglDoesSupportVertexBuffer())
                glFreeVertexBufferEXT(smFMTVertexBuffer);
            else
                AssertFatal(false,"Vertex buffer should have already been freed!");
            smFMTVertexBuffer = -1;
        }
    */
}

void InteriorLMManager::processTextureEvent(U32 eventCode)
{
    /*
       switch(eventCode)
       {
          case TextureManager::BeginZombification:
             purgeGLTextures();

                if (smMTVertexBuffer != -1)
                {
                    if (dglDoesSupportVertexBuffer())
                        glFreeVertexBufferEXT(smMTVertexBuffer);
                    else
                        AssertFatal(false,"Vertex buffer should have already been freed!");
                    smMTVertexBuffer = -1;
                }
                if (smFTVertexBuffer != -1)
                {
                    if (dglDoesSupportVertexBuffer())
                        glFreeVertexBufferEXT(smFTVertexBuffer);
                    else
                        AssertFatal(false,"Vertex buffer should have already been freed!");
                    smFTVertexBuffer = -1;
                }
                if (smFMTVertexBuffer != -1)
                {
                    if (dglDoesSupportVertexBuffer())
                        glFreeVertexBufferEXT(smFMTVertexBuffer);
                    else
                        AssertFatal(false,"Vertex buffer should have already been freed!");
                    smFMTVertexBuffer = -1;
                }
             break;

          case TextureManager::CacheResurrected:
             // relighting the scene will take care of things for us
             if(mInteriors.size())
                SceneLighting::lightScene(0, SceneLighting::LoadOnly);
             break;
       }
    */
}

//------------------------------------------------------------------------------
void InteriorLMManager::addInterior(LM_HANDLE& interiorHandle, U32 numLightmaps, Interior* interior)
{
    interiorHandle = mInteriors.size();
    mInteriors.increment();
    mInteriors.last() = new InteriorLMInfo;

    mInteriors.last()->mInterior = interior;
    mInteriors.last()->mHandlePtr = &interiorHandle;
    mInteriors.last()->mNumLightmaps = numLightmaps;

    // create base instance
    addInstance(interiorHandle, mInteriors.last()->mBaseInstanceHandle, 0);
    AssertFatal(mInteriors.last()->mBaseInstanceHandle == LM_HANDLE(0), "InteriorLMManager::addInterior: invalid base instance handle");

    // steal the lightmaps from the interior
    Vector<GFXTexHandle>& texHandles = getHandles(interiorHandle, 0);
    Vector<GFXTexHandle>& normalHandles = getNormalHandles(interiorHandle, 0);
    for (U32 i = 0; i < interior->mLightmaps.size(); i++)
    {
        AssertFatal(interior->mLightmaps[i], "InteriorLMManager::addInterior: interior missing lightmap");
        texHandles[i].set(interior->mLightmaps[i], &GFXDefaultPersistentProfile, true);

        if (interior->mLightDirMaps[i] != NULL)
            normalHandles[i].set(interior->mLightDirMaps[i], &GFXDefaultPersistentProfile, true);
    }

    interior->mLightmaps.clear();
    interior->mLightDirMaps.clear();
}

void InteriorLMManager::removeInterior(LM_HANDLE interiorHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::removeInterior: invalid interior handle");
    AssertFatal(mInteriors[interiorHandle]->mInstances.size() == 1, "InteriorLMManager::removeInterior: cannot remove base interior");

    // remove base instance
    removeInstance(interiorHandle, 0);

    *mInteriors[interiorHandle]->mHandlePtr = LM_HANDLE(-1);

    delete mInteriors[interiorHandle];

    // last one? otherwise move it
    if ((mInteriors.size() - 1) != interiorHandle)
    {
        mInteriors[interiorHandle] = mInteriors.last();
        *(mInteriors[interiorHandle]->mHandlePtr) = interiorHandle;
    }

    mInteriors.decrement();
}

//------------------------------------------------------------------------------
void InteriorLMManager::addInstance(LM_HANDLE interiorHandle, LM_HANDLE& instanceHandle, InteriorInstance* instance)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::addInstance: invalid interior handle");
    AssertFatal(interiorHandle == *(mInteriors[interiorHandle]->mHandlePtr), "InteriorLMManager::addInstance: invalid handle value");

    InteriorLMInfo* interiorInfo = mInteriors[interiorHandle];

    // create the instance info and fill
    InstanceLMInfo* instanceInfo = new InstanceLMInfo;

    instanceInfo->mInstance = instance;
    instanceInfo->mHandlePtr = &instanceHandle;
    instanceHandle = interiorInfo->mInstances.size();

    interiorInfo->mInstances.push_back(instanceInfo);

    // create/clear list
    instanceInfo->mLightmapHandles.setSize(interiorInfo->mNumLightmaps);
    dMemset(instanceInfo->mLightmapHandles.address(), 0, sizeof(GFXTexHandle*) * instanceInfo->mLightmapHandles.size());


    instanceInfo->mNormalVectorHandles.setSize(interiorInfo->mNumLightmaps);
    dMemset(instanceInfo->mNormalVectorHandles.address(), 0, sizeof(GFXTexHandle*) * instanceInfo->mNormalVectorHandles.size());
}

void InteriorLMManager::removeInstance(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::removeInstance: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::removeInstance: invalid instance handle");
    AssertFatal(!(instanceHandle == mInteriors[interiorHandle]->mBaseInstanceHandle &&
        mInteriors[interiorHandle]->mInstances.size() > 1), "InteriorLMManager::removeInstance: invalid base instance");

    InteriorLMInfo* itrInfo = mInteriors[interiorHandle];

    // kill it
    InstanceLMInfo* instInfo = itrInfo->mInstances[instanceHandle];
    for (U32 i = 0; i < instInfo->mLightmapHandles.size(); i++)
    {
        delete instInfo->mLightmapHandles[i];
        delete instInfo->mNormalVectorHandles[i];
    }

    // reset on last instance removal only (multi detailed shapes share the same instance handle)
    if (itrInfo->mInstances.size() == 1)
        *instInfo->mHandlePtr = LM_HANDLE(-1);

    delete instInfo;

    // last one? otherwise move it
    if ((itrInfo->mInstances.size() - 1) != instanceHandle)
    {
        itrInfo->mInstances[instanceHandle] = itrInfo->mInstances.last();
        *(itrInfo->mInstances[instanceHandle]->mHandlePtr) = instanceHandle;
    }

    itrInfo->mInstances.decrement();
}

void InteriorLMManager::useBaseTextures(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::useBaseTextures: invalid interior handle");
    AssertFatal(interiorHandle == *(mInteriors[interiorHandle]->mHandlePtr), "InteriorLMManager::useBaseTextures: invalid handle value");

    // Make sure the base light maps are loaded
    loadBaseLightmaps(interiorHandle, instanceHandle);

    // Install base lightmaps for this instance...
    Vector<GFXTexHandle>& baseHandles = getHandles(interiorHandle, 0);
    Vector<GFXTexHandle>& texHandles = getHandles(interiorHandle, instanceHandle);
    for (U32 i = 0; i < baseHandles.size(); i++)
        texHandles[i] = baseHandles[i];

    Vector<GFXTexHandle>& baseNormalHandles = getNormalHandles(interiorHandle, 0);
    Vector<GFXTexHandle>& texNormalHandles = getNormalHandles(interiorHandle, instanceHandle);
    for (U32 i = 0; i < baseNormalHandles.size(); i++)
        texNormalHandles[i] = baseNormalHandles[i];
}

//------------------------------------------------------------------------------
void InteriorLMManager::destroyBitmaps()
{
    for (S32 i = mInteriors.size() - 1; i >= 0; i--)
    {
        InteriorLMInfo* interiorInfo = mInteriors[i];
        for (S32 j = interiorInfo->mInstances.size() - 1; j >= 0; j--)
        {
            InstanceLMInfo* instanceInfo = interiorInfo->mInstances[j];
            for (S32 k = instanceInfo->mLightmapHandles.size() - 1; k >= 0; k--)
            {
                if (!instanceInfo->mLightmapHandles[k])
                    continue;

                GFXTextureObject* texObj = instanceInfo->mLightmapHandles[k];
                if (!texObj || !texObj->mBitmap)
                    continue;

                // don't remove 'keep' bitmaps
                if (!interiorInfo->mInterior->mLightmapKeep[k])
                {
                    //               delete texObj->bitmap;
                    //               texObj->bitmap = 0;
                }
            }
        }
    }
}

void InteriorLMManager::destroyTextures()
{
    for (S32 i = mInteriors.size() - 1; i >= 0; i--)
    {
        InteriorLMInfo* interiorInfo = mInteriors[i];
        for (S32 j = interiorInfo->mInstances.size() - 1; j >= 0; j--)
        {
            InstanceLMInfo* instanceInfo = interiorInfo->mInstances[j];
            for (S32 k = interiorInfo->mNumLightmaps - 1; k >= 0; k--)
            {
                // will want to remove the vector here eventually... so dont clear
                delete instanceInfo->mLightmapHandles[k];
                instanceInfo->mLightmapHandles[k] = 0;

                delete instanceInfo->mNormalVectorHandles[k];
                instanceInfo->mNormalVectorHandles[k] = 0;
            }
        }
    }
}

void InteriorLMManager::purgeGLTextures()
{
    /*
       Vector<GLuint> purgeList(4096);

       for(S32 i = mInteriors.size() - 1; i >= 0; i--)
       {
          InteriorLMInfo * interiorInfo = mInteriors[i];
          for(S32 j = interiorInfo->mInstances.size() - 1; j >= 0; j--)
          {
             InstanceLMInfo * instanceInfo = interiorInfo->mInstances[j];
             for(S32 k = instanceInfo->mLightmapHandles.size() - 1; k >= 0; k--)
             {
                if(!instanceInfo->mLightmapHandles[k])
                   continue;

                GFXTextureObject * texObj = *instanceInfo->mLightmapHandles[k];
                if(!texObj || !texObj->texGLName)
                   continue;

    #ifdef TORQUE_GATHER_METRICS
                AssertFatal(texObj->textureSpace <= TextureManager::smTextureSpaceLoaded, "Doh!");
                TextureManager::smTextureSpaceLoaded -= texObj->textureSpace;
                texObj->textureSpace   = 0;
    #endif

                purgeList.push_back(texObj->texGLName);
                texObj->texGLName = 0;
             }
          }
       }

       glDeleteTextures(purgeList.size(), purgeList.address());
    */
}

void InteriorLMManager::downloadGLTextures()
{
    for (S32 i = mInteriors.size() - 1; i >= 0; i--)
        downloadGLTextures(i);
}

void InteriorLMManager::downloadGLTextures(LM_HANDLE interiorHandle)
{

    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::downloadGLTextures: invalid interior handle");
    InteriorLMInfo* interiorInfo = mInteriors[interiorHandle];

    // The bit vector is used to keep track of which lightmap sets need
    // to be loaded from the shared "base" instance.  Every instance
    // can have it's own lightmap set due to mission lighting.
    BitVector needTexture;
    needTexture.setSize(interiorInfo->mNumLightmaps);
    needTexture.clear();

    for (S32 j = interiorInfo->mInstances.size() - 1; j >= 0; j--)
    {
        InstanceLMInfo* instanceInfo = interiorInfo->mInstances[j];
        for (S32 k = instanceInfo->mLightmapHandles.size() - 1; k >= 0; k--)
        {
            // All instances can share the base instances static lightmaps.
            // Test here to see if we need to load those lightmaps.
            if ((j == 0) && !needTexture.test(k))
                continue;
            if (!instanceInfo->mLightmapHandles[k])
            {
                needTexture.set(k);
                continue;
            }
            GFXTexHandle texObj = instanceInfo->mLightmapHandles[k];
            if (!texObj || !texObj->mBitmap)
            {
                needTexture.set(k);
                continue;
            }


            //         delete instanceInfo->mLightmapHandles[k];
            instanceInfo->mLightmapHandles[k].set(texObj->mBitmap, &GFXDefaultPersistentProfile, false);


            GFXTexHandle normalObject = instanceInfo->mNormalVectorHandles[k];
            if (normalObject && normalObject->mBitmap)
                instanceInfo->mNormalVectorHandles[k].set(normalObject->mBitmap, &GFXDefaultPersistentProfile, false);
        }
    }

}

bool InteriorLMManager::loadBaseLightmaps(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::loadBaseLightmaps: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::loadBaseLightmaps: invalid instance handle");

    // must use a valid instance handle
    if (!instanceHandle)
        return(false);

    InteriorLMInfo* interiorInfo = mInteriors[interiorHandle];
    if (!interiorInfo->mNumLightmaps)
        return(false);

    InstanceLMInfo* baseInstanceInfo = interiorInfo->mInstances[0];

    // already loaded? (if any bitmap is present, then assumed that all will be)
    GFXTexHandle texture(baseInstanceInfo->mLightmapHandles[0]);
    if (texture.isValid() && texture.getBitmap())
        return(true);

    InstanceLMInfo* instanceInfo = interiorInfo->mInstances[instanceHandle];

    Resource<InteriorResource>& interiorRes = instanceInfo->mInstance->getResource();
    if (!bool(interiorRes))
        return(false);

    GBitmap*** pBitmaps = 0;
    if (!instanceInfo->mInstance->readLightmaps(&pBitmaps))
        return(false);

    for (U32 i = 0; i < interiorRes->getNumDetailLevels(); i++)
    {
        Interior* interior = interiorRes->getDetailLevel(i);
        AssertFatal(interior, "InteriorLMManager::loadBaseLightmaps: invalid detail level in resource");
        AssertFatal(interior->getLMHandle() != LM_HANDLE(-1), "InteriorLMManager::loadBaseLightmaps: interior not added to manager");
        AssertFatal(interior->getLMHandle() < mInteriors.size(), "InteriorLMManager::loadBaseLightmaps: invalid interior");

        InteriorLMInfo* interiorInfo = mInteriors[interior->getLMHandle()];
        InstanceLMInfo* baseInstanceInfo = interiorInfo->mInstances[0];

        for (U32 j = 0; j < interiorInfo->mNumLightmaps; j++)
        {
            AssertFatal(pBitmaps[i][j], "InteriorLMManager::loadBaseLightmaps: invalid bitmap");

            if (baseInstanceInfo->mLightmapHandles[j])
            {
                GFXTextureObject* texObj = baseInstanceInfo->mLightmapHandles[j];
                texObj->mBitmap = pBitmaps[i][j];
            }
            else
                baseInstanceInfo->mLightmapHandles[j].set(pBitmaps[i][j], &GFXDefaultPersistentProfile, false);
        }
    }

    delete[] pBitmaps;
    return(true);
}

//------------------------------------------------------------------------------
GFXTexHandle& InteriorLMManager::getHandle(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getHandle: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::getHandle: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::getHandle: invalid texture index");

    // valid? if not, then get base lightmap handle
    if (!mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index])
    {
        AssertFatal(mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index], "InteriorLMManager::getHandle: invalid base texture handle");
        return(mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index]);
    }
    return(mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index]);
}

GBitmap* InteriorLMManager::getBitmap(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getBitmap: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::getBitmap: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::getBitmap: invalid texture index");

    if (!mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index])
    {
        AssertFatal(mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index], "InteriorLMManager::getBitmap: invalid base texture handle");
        return(mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index]->getBitmap());
    }

    return(mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index]->getBitmap());
}

Vector<GFXTexHandle>& InteriorLMManager::getHandles(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getHandles: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::getHandles: invalid instance handle");
    return(mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles);
}

//------------------------------------------------------------------------------
U32 InteriorLMManager::getNumLightmaps(LM_HANDLE interiorHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getNumLightmaps: invalid interior handle");
    return(mInteriors[interiorHandle]->mNumLightmaps);
}

//------------------------------------------------------------------------------
void InteriorLMManager::deleteLightmap(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::deleteLightmap: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::deleteLightmap: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::deleteLightmap: invalid texture index");

    delete mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index];
    mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index] = 0;

    delete mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index];
    mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index] = 0;
}

void InteriorLMManager::clearLightmaps(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::clearLightmaps: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::clearLightmaps: invalid instance handle");

    for (U32 i = 0; i < mInteriors[interiorHandle]->mNumLightmaps; i++)
    {
        //      if(mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[i])
        //         delete mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[i];
        mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[i] = 0;
        mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[i] = 0;
    }
}

//------------------------------------------------------------------------------
GFXTexHandle& InteriorLMManager::duplicateBaseLightmap(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::duplicateBaseLightmap: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::duplicateBaseLightmap: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::duplicateBaseLightmap: invalid texture index");

    // already exists?
    GFXTexHandle texHandle = mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index];
    if (texHandle && texHandle->getBitmap())
        return mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index];

    AssertFatal(mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index], "InteriorLMManager::duplicateBaseLightmap: invalid base handle");

    // copy it
    GBitmap* src = mInteriors[interiorHandle]->mInstances[0]->mLightmapHandles[index]->getBitmap();
    GBitmap* dest = new GBitmap(*src);

    // don't want this texture to be downloaded yet (SceneLighting will take care of that)
    GFXTexHandle tHandle(dest, &GFXDefaultPersistentProfile, true);
    mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index] = tHandle;
    return mInteriors[interiorHandle]->mInstances[instanceHandle]->mLightmapHandles[index];
}

GFXTexHandle& InteriorLMManager::getNormalHandle(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getHandle: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::getHandle: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::getHandle: invalid texture index");

    // valid? if not, then get base lightmap handle
    if (!mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index])
    {
        AssertFatal(mInteriors[interiorHandle]->mInstances[0]->mNormalVectorHandles[index], "InteriorLMManager::getHandle: invalid base texture handle");
        return(mInteriors[interiorHandle]->mInstances[0]->mNormalVectorHandles[index]);
    }
    return(mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index]);
}

Vector<GFXTexHandle>& InteriorLMManager::getNormalHandles(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::getHandles: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::getHandles: invalid instance handle");
    return(mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles);
}

GFXTexHandle& InteriorLMManager::duplicateBaseNormalmap(LM_HANDLE interiorHandle, LM_HANDLE instanceHandle, U32 index)
{
    AssertFatal(interiorHandle < mInteriors.size(), "InteriorLMManager::duplicateBaseLightmap: invalid interior handle");
    AssertFatal(instanceHandle < mInteriors[interiorHandle]->mInstances.size(), "InteriorLMManager::duplicateBaseLightmap: invalid instance handle");
    AssertFatal(index < mInteriors[interiorHandle]->mNumLightmaps, "InteriorLMManager::duplicateBaseLightmap: invalid texture index");

    // already exists?
    GFXTexHandle texHandle = mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index];
    if (texHandle && texHandle->getBitmap())
        return mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index];

    AssertFatal(mInteriors[interiorHandle]->mInstances[0]->mNormalVectorHandles[index], "InteriorLMManager::duplicateBaseLightmap: invalid base handle");

    // copy it
    GBitmap* src = mInteriors[interiorHandle]->mInstances[0]->mNormalVectorHandles[index]->getBitmap();
    GBitmap* dest = new GBitmap(*src);

    // don't want this texture to be downloaded yet (SceneLighting will take care of that)
    GFXTexHandle tHandle(dest, &GFXDefaultPersistentProfile, true);
    mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index] = tHandle;
    return mInteriors[interiorHandle]->mInstances[instanceHandle]->mNormalVectorHandles[index];
}

/*
S32 InteriorLMManager::getVertexBuffer(S32 format)
{
    switch (format)
    {
        case GL_V12MTVFMT_EXT:
            {
                if (smMTVertexBuffer != -1)
                    return smMTVertexBuffer;

                smMTVertexBuffer = glAllocateVertexBufferEXT(512,GL_V12MTVFMT_EXT,false);

                return smMTVertexBuffer;
            }
        case GL_V12FTVFMT_EXT:
            {
                if (smFTVertexBuffer != -1)
                    return smFTVertexBuffer;

                smFTVertexBuffer = glAllocateVertexBufferEXT(512,GL_V12FTVFMT_EXT,false);

                return smFTVertexBuffer;
            }
        case GL_V12FMTVFMT_EXT:
            {
                if (smFMTVertexBuffer != -1)
                    return smFMTVertexBuffer;

                smFMTVertexBuffer = glAllocateVertexBufferEXT(512,GL_V12FMTVFMT_EXT,false);

                return smFMTVertexBuffer;
            }
    }

   AssertFatal(false,"InteriorLMManager::getVertexBuffer() What?  We should never get here!!!");

    return -1;
}
*/
