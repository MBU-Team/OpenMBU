//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
// This is a "frame" or element in the GFX state stack
//-----------------------------------------------------------------------------

#ifndef _GFXSTATEFRAME_H_
#define _GFXSTATEFRAME_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif

#include "core/dataChunker.h"

//-----------------------------------------------------------------------------
// GFXStateFrame
//-----------------------------------------------------------------------------
class GFXStateFrame
{
    struct GlobalFragment
    {
        GlobalFragment* next;

        U32 state;
        U32 oldVal;
    };

    struct Fragment : GlobalFragment
    {
        U32 stage;
    };

    static FreeListChunker<GlobalFragment> mGlobalChunker;
    static FreeListChunker<Fragment>       mFragmentChunker;

    GlobalFragment* mGlobals;
    Fragment* mTexture;
    Fragment* mSampler;
    bool              mEnable;

public:

    void init();
    void rollback();

    void trackRenderState(U32 state, U32 value);
    void trackSamplerState(U32 stage, U32 type, U32 value);
    void trackTextureStageState(U32 stage, U32 type, U32 value);

};



#endif
