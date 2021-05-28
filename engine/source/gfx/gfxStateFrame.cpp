//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfxStateFrame.h"
#include "gfxDevice.h"
#include "platform/profiler.h"

FreeListChunker<GFXStateFrame::GlobalFragment> GFXStateFrame::mGlobalChunker;
FreeListChunker<GFXStateFrame::Fragment>       GFXStateFrame::mFragmentChunker;

//-----------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------
void GFXStateFrame::init()
{
    mGlobals = mTexture = mSampler = NULL;
}

//-----------------------------------------------------------------------------
// rollback
//-----------------------------------------------------------------------------
void GFXStateFrame::rollback()
{
    PROFILE_START(StateFrameRollback);

    mEnable = false;

    // Restore everything to its original state and delete it.
    // We traverse from newest to oldest, so if multiple sets happen
    // we'll end up with the last value.
    //
    // Presumably a somewhat longer iteration at the end pays off for
    // not having to do a bunch of O(n) traversals or maintain a hash.
    GlobalFragment* walkG = mGlobals;
    while (walkG)
    {
        GlobalFragment* del = walkG;
        //            Con::printf(" setting state %d=%d", walkG->state, walkG->oldVal);
        GFX->trackRenderState(walkG->state, walkG->oldVal);
        walkG = walkG->next;
        mGlobalChunker.free(del);
    }

    Fragment* walk = mTexture;
    while (walk)
    {
        Fragment* del = (Fragment*)walk;
        //            Con::printf(" setting mTexture %d state %d=%d", walk->stage, walk->state, walk->oldVal);
        GFX->trackTextureStageState(walk->stage, walk->state, walk->oldVal);
        walk = (Fragment*)walk->next;
        mFragmentChunker.free(del);
    }

    walk = mSampler;
    while (walk)
    {
        Fragment* del = (Fragment*)walk;
        //            Con::printf(" setting mSampler %d state %d=%d", walk->stage, walk->state, walk->oldVal);
        GFX->trackSamplerState(walk->stage, walk->state, walk->oldVal);
        walk = (Fragment*)walk->next;
        mFragmentChunker.free(del);
    }

    mEnable = true;

    // And reset list pointers.
    init();

    PROFILE_END();

    //         Con::printf("--- rollback");
}

//-----------------------------------------------------------------------------
// trackRenderState - Called by GFX to update our state.
//-----------------------------------------------------------------------------
void GFXStateFrame::trackRenderState(U32 state, U32 value)
{
    if (!mEnable) return;

    // Check to see if we already know about this.
    GlobalFragment* walk = mGlobals;
    while (walk)
    {
        if (walk->state == state)
            return;

        walk = walk->next;
    }

    //         Con::printf(" storing state %d=%d", state, value);

    GlobalFragment* gf = mGlobalChunker.alloc();

    gf->state = state;
    gf->oldVal = value;

    gf->next = mGlobals;
    mGlobals = gf;
}

//-----------------------------------------------------------------------------
// trackSamplerState
//-----------------------------------------------------------------------------
void GFXStateFrame::trackSamplerState(U32 stage, U32 type, U32 value)
{
    if (!mEnable) return;

    // Check to see if we already know about this.
    Fragment* walk = mSampler;
    while (walk)
    {
        if (walk->stage == stage && walk->state == type)
            return;

        walk = (Fragment*)walk->next;
    }

    //         Con::printf(" storing mSampler %d state %d=%d", stage, type, value);

    Fragment* f = mFragmentChunker.alloc();

    f->stage = stage;
    f->state = type;
    f->oldVal = value;

    f->next = mSampler;
    mSampler = f;
}

//-----------------------------------------------------------------------------
// trackTextureStageState
//-----------------------------------------------------------------------------
void GFXStateFrame::trackTextureStageState(U32 stage, U32 type, U32 value)
{
    if (!mEnable) return;

    // Check to see if we already know about this.
    Fragment* walk = mTexture;
    while (walk)
    {
        if (walk->stage == stage && walk->state == type)
            return;

        walk = (Fragment*)walk->next;
    }

    //         Con::printf(" storing mTexture %d state %d=%d", stage, type, value);

    Fragment* f = mFragmentChunker.alloc();

    f->stage = stage;
    f->state = type;
    f->oldVal = value;

    f->next = mTexture;
    mTexture = f;
}

