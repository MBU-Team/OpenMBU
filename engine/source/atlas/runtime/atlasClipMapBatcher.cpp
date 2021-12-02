#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/runtime/atlasClipMapBatcher.h"
#include "materials/shaderData.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"

bool AtlasClipMapBatcher::smRenderDebugTextures = false;

AtlasClipMapBatcher::AtlasClipMapBatcher()
{
    mClipMap = NULL;

    // Set up our render lists.
    mRenderList[0] = NULL;
    for (S32 i = 1; i < 4; i++)
        mRenderList[i].clear();
}

void AtlasClipMapBatcher::init(AtlasClipMap* acm, SceneState* state)
{
    // Note our render state.
    mClipMap = acm;
    mState = state;

    // Empty the render lists...
    for (S32 i = 1; i < 4; i++)
        mRenderList[i].clear();
    mDetailList.clear();
    mFogList.clear();

    // And clear the render notes.
    mRenderNoteAlloc.freeBlocks();
}

void AtlasClipMapBatcher::queue(const Point3F& camPos, AtlasResourceGeomStub* args, F32 morph)
{
    PROFILE_START(AtlasClipMapBatcher_queue);

    AtlasGeomChunk* agc = args->mChunk;

    Point3F nearPos, farPos;
    Point2F nearTC, farTC;
    agc->calculatePoints(camPos, nearPos, farPos, nearTC, farTC);
    const F32 nearDistance = (camPos - nearPos).len();
    const F32 farDistance = (camPos - farPos).len();

    const RectF texBounds(nearTC, farTC - nearTC);

    // Now, calculate and store levels into a new RenderNote.
    S32 startLevel, endLevel;
    mClipMap->calculateClipMapLevels(nearDistance, farDistance, texBounds, startLevel, endLevel);

    // Allocate a render note.
    RenderNote* rn = mRenderNoteAlloc.alloc();

    // Check if this chunk will get fogged - consider furthest point, and if
    // it'll be fogged then draw a fog pass.
    if (mState->getHazeAndFog(farDistance, farPos.z - camPos.z) > 0.01)
        mFogList.push_back(rn);

    rn->levelStart = startLevel;
    rn->levelEnd = endLevel;
    rn->levelCount = endLevel - startLevel + 1;
    rn->chunk = agc;
    rn->nearDist = nearDistance;
    rn->morph = morph;

    // Stuff into right list based on shader.
    switch (rn->levelCount)
    {
    case 2:
    case 3:
    case 4:
        mRenderList[rn->levelCount - 1].push_back(rn);
        break;

    default:
        Con::errorf("AtlasClipMapBatcher::queue - got unexpected level count of %d", rn->levelCount);
        break;
    }

    AssertFatal(rn->levelCount >= 2 && rn->levelCount <= 4,
        "AtlasClipMapBatcher::queue - bad level count!");

    PROFILE_END();
}

S32 AtlasClipMapBatcher::cmpRenderNote(const void* a, const void* b)
{
    RenderNote* fa = *((RenderNote**)a);
    RenderNote* fb = *((RenderNote**)b);

    // Sort by distance.
    if (fa->nearDist > fb->nearDist)
        return 1;
    else if (fa->nearDist < fb->nearDist)
        return -1;

    // So this is what equality is like...
    return 0;
}

void AtlasClipMapBatcher::sort()
{
    // Sort our elements. Luckily we have qsort!
    for (S32 i = 1; i < 4; i++)
        dQsort(mRenderList[i].address(), mRenderList[i].size(), sizeof(RenderNote*), cmpRenderNote);
}

void AtlasClipMapBatcher::renderClipMap()
{
    PROFILE_START(AtlasClipMapBatcher_renderClipMap);

    // Change tracking state variables.
    U32 numShaderChanges = 0;
    U32 numTextureChanges = 0;
    U32 numRendered = 0;
    GFXShader* lastShader = NULL;
    S32 lastMax = -1;

    for (S32 i = 0; i < 4; i++)
    {
        GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
        GFX->setTextureStageAddressModeV(i, GFXAddressWrap);
        GFX->setTextureStageMagFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMipFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageMinFilter(i, GFXTextureFilterLinear);
        GFX->setTextureStageColorOp(i, GFXTOPModulate);
    }

    for (S32 curBin = 1; curBin < 4; curBin++)
    {
        // If bin is empty, skip.
        if (mRenderList[curBin].size() == 0)
            continue;

        // Set up shader.
        PROFILE_START(AtlasClipMapBatcher_render_setup_shader);

        numShaderChanges++;
        lastShader = mClipMap->getShader(curBin);
        AssertFatal(lastShader,
            avar("AtlasClipMapBatcher::render - no shader present for %d level rendering!", curBin));
        GFX->setShader(lastShader);

        PROFILE_END();

        for (S32 i = 0; i < mRenderList[curBin].size(); i++)
        {
            // Grab the render note.
            const RenderNote* rn = mRenderList[curBin][i];
            numRendered++;

            // Set up clipmap levels.
            if (rn->levelEnd != lastMax)
            {
                PROFILE_START(AtlasClipMapBatcher_render_setup_texture);

                numTextureChanges += rn->levelCount;

                // And iterate...
                Point4F shaderConsts[4];

                for (S32 j = rn->levelStart; j <= rn->levelEnd; j++)
                {
                    const S32 shaderIdx = rn->levelEnd - j;

                    AssertFatal(shaderIdx >= 0 && shaderIdx < mClipMap->mClipStackDepth,
                        "AtlasClipMapBatcher::render - out of range shaderIdx!");

                    Point4F& pt = shaderConsts[shaderIdx];

                    // Note the offset and center for this level as well.
                    AtlasClipMap::ClipStackEntry& cse = mClipMap->mLevels[j];
                    pt.x = cse.mClipCenter.x * cse.mScale;
                    pt.y = cse.mClipCenter.y * cse.mScale;
                    pt.z = cse.mScale;
                    pt.w = 0.f;

                    if (smRenderDebugTextures)
                        GFX->setTexture(shaderIdx, cse.mDebugTex);
                    else
                        GFX->setTexture(shaderIdx, cse.mTex);
                }

                // Set all the constants in one go.
                GFX->setVertexShaderConstF(50, &shaderConsts[0].x, rn->levelCount);
                //GFX->setPixelShaderConstF(0, &shaderConsts[0].x, 4);
                PROFILE_END();
            }

            // Alright, we're all ready to go - so issue the draw.
            PROFILE_START(AtlasClipMapBatcher_render_chunk);

            // Pass morph constant to the chunk.
            Point4F chunkConsts(rn->morph, 0, 0, 0);
            GFX->setVertexShaderConstF(49, &chunkConsts.x, 1);

            // And render.
            rn->chunk->render();
            PROFILE_END();
        }
    }

    // Don't forget to clean up.
    for (S32 i = 0; i < 4; i++)
        GFX->setTexture(0, NULL);

    PROFILE_END();

    /*Con::printf("AtlasClipMapBatcher::render - %d shader changes, %d texture changes, %d chunks drawn.",
       numShaderChanges, numTextureChanges, numRendered);*/
}

void AtlasClipMapBatcher::renderFog()
{
    PROFILE_START(AtlasClipMapBatcher_renderFog);

    // Grab the shader for this pass - replaceme w/ real code.
    GFXShader* s;
    ShaderData* sd;
    if (!Sim::findObject("AtlasShaderFog", sd))
    {
        Con::errorf("AtlasClipMapBatcher::renderFog - no shader 'AtlasShaderFog' present!");
        return;
    }

    // Set up the fog shader and texture.
    GFX->setShader(sd->getShader());

    Point4F fogConst(
        getCurrentClientSceneGraph()->getFogHeightOffset(),
        getCurrentClientSceneGraph()->getFogInvHeightRange(),
        getCurrentClientSceneGraph()->getVisibleDistanceMod(),
        0);
    GFX->setVertexShaderConstF(50, &fogConst[0], 1);

    GFX->setTexture(0, getCurrentClientSceneGraph()->getFogTexture());
    GFX->setTextureStageAddressModeU(0, GFXAddressClamp);
    GFX->setTextureStageAddressModeV(0, GFXAddressClamp);

    // We need the eye pos but AtlasInstance2 deals with setting this up.

    // Set blend mode and alpha test as well.
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendSrcAlpha);
    GFX->setDestBlend(GFXBlendInvSrcAlpha);

    GFX->setAlphaTestEnable(true);
    GFX->setAlphaFunc(GFXCmpGreaterEqual);
    GFX->setAlphaRef(2);


    // And render all the fogged chunks - all for now.
    for (S32 i = 0; i < mFogList.size(); i++)
    {
        // Grab the render note.
        const RenderNote* rn = mFogList[i];

        // Pass morph constant to the chunk.
        Point4F chunkConsts(rn->morph, 0, 0, 0);
        GFX->setVertexShaderConstF(49, &chunkConsts.x, 1);

        rn->chunk->render();
    }

    GFX->setAlphaBlendEnable(false);
    GFX->setAlphaTestEnable(false);

    // Don't forget to clean up.
    for (S32 i = 0; i < 4; i++)
        GFX->setTexture(0, NULL);

    PROFILE_END();

}

void AtlasClipMapBatcher::renderDetail()
{

}

void AtlasClipMapBatcher::render()
{
    renderClipMap();
    renderDetail();
    renderFog();
}