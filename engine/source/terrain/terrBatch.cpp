//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "terrain/terrRender.h"
#include "terrain/terrBatch.h"
#include "math/mMath.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gBitmap.h"
#include "terrain/terrRender.h"
#include "sceneGraph/sceneState.h"
#include "terrain/waterBlock.h"
#include "terrain/blender.h"
#include "core/frameAllocator.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sgUtil.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "terrain/sky.h"

namespace TerrBatch
{

    // We buffer everything here then send it off to the GPU
    GFXTerrainVert* mVertexStore;
    U16                     mCurVertex;
    U16* mIndexStore;
    U16                     mCurIndex;
    GFXPrimitive* mPrimitiveStore;
    U16                     mCurPrim;
    GFXTextureObject** mTextureStore;
    U16                     mCurTexture;
    U32                     mCurXF;

    // Store previous counts for batching
    U16                     mLastIndex;
    U16                     mLastVertex;

    // Statistics
    S32                     mBiggestVertexBatch;
    S32                     mBiggestIndexBatch;

    const U32               mVertexStoreSize = 16535;
    const U32               mIndexStoreSize = 16535;
    const U32               mPrimitiveStoreSize = 4096;
    const U32               mTextureStoreSize = 4096;

    void init()
    {
        // Allocate space...
        mVertexStore = new GFXTerrainVert[mVertexStoreSize];
        mIndexStore = new U16[mIndexStoreSize];
        mPrimitiveStore = new GFXPrimitive[mPrimitiveStoreSize];
        mTextureStore = new GFXTextureObject * [mTextureStoreSize];

        /*      // Gratuitous mem usage spew
              Con::printf("Allocated terrain buffers...");
              Con::printf("    - Vertices   (bytes): %d", mVertexStore   .memSize());
              Con::printf("    - Indices    (bytes): %d", mIndexStore    .memSize());
              Con::printf("    - Prims      (bytes): %d", mPrimitiveStore.memSize());
              Con::printf("    - TexHandles (bytes): %d", mTextureStore  .memSize());
              Con::printf("  === Total      (bytes): %d", mVertexStore.memSize() + mIndexStore.memSize() + mPrimitiveStore.memSize() + mTextureStore.memSize());
        */
        // Initialize
        mCurVertex = 0;
        mCurIndex = 0;
        mCurTexture = 0;
        mCurXF = 1;

        mLastIndex = 0;
        mLastVertex = 0;

        // Init statistics
        mBiggestIndexBatch = 0;
        mBiggestVertexBatch = 0;

        Con::addVariable("TRender::mBiggestIndexBatch", TypeS32, &mBiggestIndexBatch);
        Con::addVariable("TRender::mBiggestVertexBatch", TypeS32, &mBiggestVertexBatch);
    }

    void begin()
    {
        // Do nothing for now...
        mLastVertex = mCurVertex;
    }

    Point4F texGenS;
    Point4F texGenT;

    void setTexGen(Point4F s, Point4F t)
    {
        // We have to assign texture coords to each square, this lets us set the
        // tex transforms applied to the next batch of geometry we get. (ie,
        // these changes are applied at the next call to end())
        texGenS = s;
        texGenT = t;
    }

    Point4F& getTexGenS()
    {
        return texGenS;
    }

    Point4F& getTexGenT()
    {
        return texGenT;
    }

    void end(MatInstance* m, SceneGraphData& sgData, GFXTexHandle tex, bool final, bool dlighting)
    {
        PROFILE_START(Terrain_batchEnd);

        // Advance the cache - we can't keep old verts because of texture co-ordinates.
        mCurXF++;

        // If we have nothing to draw, quick-out
  /*      if( (mCurVertex== 0 || (mCurIndex-mLastIndex) == 0) && !final)
           return;
  */
        if (!final)
        {
            PROFILE_START(Terrain_TexGen);

            // Apply texgen to the new verts...
            for (U32 i = mLastVertex; i < mCurVertex; i++)
            {
                mVertexStore[i].texCoord0.x = mVertexStore[i].point.x * texGenS.x + mVertexStore[i].point.y * texGenS.y + mVertexStore[i].point.z * texGenS.z + texGenS.w;
                mVertexStore[i].texCoord0.y = mVertexStore[i].point.x * texGenT.x + mVertexStore[i].point.y * texGenT.y + mVertexStore[i].point.z * texGenT.z + texGenT.w;
            };

            PROFILE_END();

            // If we're not up to the threshold don't draw anything, just store the primitive...

            // Store the material
            mTextureStore[mCurTexture++] = tex;

            // Store the primitive
            // GFXTriangleList, 0, mCurVertex, 0, mCurIndex / 3
            GFXPrimitive& tempPrim = mPrimitiveStore[mCurPrim++];
            tempPrim.type = GFXTriangleList;
            tempPrim.minIndex = 0;
            tempPrim.numVertices = mCurVertex;
            tempPrim.startIndex = mLastIndex;
            tempPrim.numPrimitives = (mCurIndex - mLastIndex) / 3;

            if (mCurIndex - mLastIndex > mBiggestIndexBatch)
                mBiggestIndexBatch = mCurIndex - mLastIndex;

            if (mCurVertex - mLastVertex > mBiggestVertexBatch)
                mBiggestVertexBatch = mCurVertex - mLastVertex;


            // Update our last-state
            mLastIndex = mCurIndex;
            mLastVertex = mCurVertex;



            // BJGNOTE - These thresholds were set by careful readthroughs of the code
            //           If you change the terrain code these need to be updated.
            //
            // Some reference values...
            //    - Most indices  commander chunk can generate - 768
            //    - Most vertices commander chunk can generate - 128
            //    - Most indices  normal    chunk can generate - 96
            //    - Most vertices normal    chunk can generate - 25

            // Default, normal chunks...
            U32 idxThresh = 96;
            U32 vertThresh = 25;

            if (TerrainRender::mRenderingCommander)
            {
                // Special case for commander map
                idxThresh = 768;
                vertThresh = 128;
            }

            if (mCurVertex < mVertexStoreSize - vertThresh && mCurIndex < (mIndexStoreSize - idxThresh))
            {
                // And bail if we don't HAVE to draw now...
                PROFILE_END();
                return;
            }
        }

        // Blast everything out to buffers and draw it.

        // If there's nothing to draw... draw nothing
        if (mCurVertex == 0 || mCurIndex == 0)
        {
            PROFILE_END();
            return;
        }

        GFXVertexBufferHandle<GFXTerrainVert> v(GFX, mCurVertex, GFXBufferTypeVolatile);
        GFXPrimitiveBufferHandle              p(GFX, mCurIndex, 1, GFXBufferTypeVolatile);

        PROFILE_START(Terrain_bufferCopy);

        // Do vertices
        PROFILE_START(Terrain_bufferCopy_lockV);
        v.lock();
        PROFILE_END();

        dMemcpy(&v[0], &mVertexStore[0], sizeof(GFXTerrainVert) * mCurVertex);

        PROFILE_START(Terrain_bufferCopy_unlockV);
        v.unlock();
        PROFILE_END();

        // Do primitives/indices
        U16* idxBuff;
        GFXPrimitive* primBuff;

        PROFILE_START(Terrain_bufferCopy_lockP);
        p.lock(&idxBuff, &primBuff);
        PROFILE_END();

        dMemcpy(idxBuff, &mIndexStore[0], sizeof(U16) * mCurIndex);

        PROFILE_START(Terrain_bufferCopy_unlockP);
        p.unlock();
        PROFILE_END();

        PROFILE_END();

        PROFILE_START(TRender_DIP);

        // Now... RENDER!!!
        GFX->setVertexBuffer(v);
        GFX->setPrimitiveBuffer(p);

        // Set up our shader and material...
  //      Atmosphere::initSGData(sgData);

        GFX->setTextureStageMagFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(0, GFXTextureFilterLinear);
        GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(0, GFXAddressClamp);
        GFX->setTextureStageAddressModeU(1, GFXAddressClamp);
        GFX->setTextureStageAddressModeV(1, GFXAddressClamp);

        m->init(sgData, (GFXVertexFlags)getGFXVertFlags((GFXTerrainVert*)NULL));

        U32 texChange = 0, dipCall = 0;

        while (m->setupPass(sgData))
        {
            if (dlighting)
            {
                GFX->setAlphaBlendEnable(true);
                GFX->setSrcBlend(GFXBlendOne);
                GFX->setDestBlend(GFXBlendOne);
                //GFX->setDestBlend(GFXBlendZero);
            }

            if (sgData.useFog)
                GFX->setTexture(1, sgData.fogTex);

            GFXTextureObject* lastTO = NULL;

            U32 curPrim = 0;

            do
            {
                if (!lastTO || mTextureStore[curPrim] != lastTO)
                {
                    GFX->setTexture(0, mTextureStore[curPrim]);
                    lastTO = mTextureStore[curPrim];
                    texChange++;
                }

                // This is all triangle list geometry, so we can batch adjacent stuff. Yay!
                // We do a fixup when we submit so all indices are in "global" number-space.

                // Figure out what span we can do
                U32 startIndex = mPrimitiveStore[curPrim].startIndex;
                U32 numPrims = mPrimitiveStore[curPrim].numPrimitives;
                U32 minIndex = mPrimitiveStore[curPrim].minIndex;
                U32 numVerts = mPrimitiveStore[curPrim].numVertices;

                // Advance right off.
                curPrim++;

                while ((mTextureStore[curPrim] == lastTO) && !dlighting)
                {
                    // Update our range. All of this is consecutive in the IB...
                    AssertFatal(startIndex + numPrims * 3 == mPrimitiveStore[curPrim].startIndex,
                        "TerrBatch::end - non-contiguous IB ranges!");

                    // Increase our prim count.
                    numPrims += mPrimitiveStore[curPrim].numPrimitives;

                    // See if we need to update our index range.
                    minIndex = getMin(mPrimitiveStore[curPrim].minIndex, minIndex);

                    if (mPrimitiveStore[curPrim].minIndex + mPrimitiveStore[curPrim].numVertices > minIndex + numVerts)
                        numVerts = mPrimitiveStore[curPrim].minIndex + mPrimitiveStore[curPrim].numVertices - minIndex;

                    curPrim++;
                }

                // Now issue an aggregate draw.
                GFX->drawIndexedPrimitive(GFXTriangleList, minIndex, numVerts, startIndex, numPrims);
                dipCall++;
            } while (curPrim < mCurPrim);

        }

        // Count unique textures... this is dumb and slow.
        /*
        S32 uniqueCount = 0;
        for(S32 i=0; i<mCurTexture; i++)
        {
           // If it's the first occurrence, it's unique.
           bool isUnique = true;
           for(S32 j=0; j<i; j++)
              if(mTextureStore[i] == mTextureStore[j])
              {
                 isUnique = false;
                 break;
              }

           if(isUnique)
              uniqueCount++;
        }*/

        // Clear texture references...
        for (U32 i = 0; i < mCurTexture; i++)
            mTextureStore[i] = NULL;

        //Con::printf("---------- terrain draw (%d prim, %d tex, %d texChange, %d dips)", mCurPrim, mCurTexture, texChange, dipCall);

        // Clear all our buffers
        mCurIndex = 0;
        mLastIndex = 0;
        mCurVertex = 0;
        mLastVertex = 0;
        mCurTexture = 0;
        mCurPrim = 0;
        mCurXF++;

        PROFILE_END();
        // All done!

        PROFILE_END();
    }
};

