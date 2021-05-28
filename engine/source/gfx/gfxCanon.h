#include "gfx/gfxDevice.h"

#define GFX_CANONIZE

#ifdef GFX_CANONIZE

#define GFX_Canonizer(func, file, line)  _GFXCanonizer canon##line(func);

class _GFXCanonizer
{
    bool    mHasChanged;

    const char* mName;
    MatrixF mProj;
    MatrixF mWorld;
    U32     mWorldDepth;

    U32               mStates[GFXRenderState_COUNT];
    U32               mTextureStates[TEXTURE_STAGE_COUNT][GFXTSS_COUNT];
    U32               mSamplerStates[TEXTURE_STAGE_COUNT][GFXSAMP_COUNT];

public:
    _GFXCanonizer(const char* chunkName)
    {
        // Store our name...
        mName = chunkName;
        mHasChanged = false;

        // Store out matrix info
        mProj = GFX->getProjectionMatrix();
        mWorld = GFX->getWorldMatrix();
        mWorldDepth = GFX->mWorldStackSize;

        // Store state info
        for (U32 i = 0; i < GFXRenderState_COUNT; i++)
            mStates[i] = GFX->mStateTracker[i].newValue;

        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
            for (U32 j = 0; j < GFXTSS_COUNT; j++)
                mTextureStates[i][j] = GFX->mTextureStateTracker[i][j].newValue;

        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
            for (U32 j = 0; j < GFXSAMP_COUNT; j++)
                mSamplerStates[i][j] = GFX->mSamplerStateTracker[i][j].newValue;

        // All done!
    }

    inline void matchState(const U32 idx, const U32 stored, const U32 read, const char* type)
    {
        switch (idx)
        {
        case GFXRSCullMode:
            if (read == GFXCullNone)
                return;
            break;

            // If alpha is disabled, then don't worry about blends
        case GFXRSSrcBlend:
        case GFXRSDestBlend:
            if (GFX->mStateTracker[GFXRSAlphaBlendEnable].newValue == false)
                return;
            break;
        }

        if (stored != read)
        {
            Con::errorf("%s: Mismatching %s #%d: %d != %d", mName, type, idx, stored, read);
            mHasChanged = true;
        }
    }

    inline void matchTextureState(const U32 idx, const U32 idx2, const U32 stored, const U32 read, const char* type)
    {
        // Accept certain defaults
        switch (idx2)
        {
        case GFXTSSColorOp:
            if (read == GFXTOPModulate || read == GFXTOPDisable)
                return;
            break;
        }

        if (stored != read)
        {
            Con::errorf("%s: Mismatching %s stage %d #%d: %d != %d", mName, type, idx, idx2, stored, read);
            mHasChanged = true;
        }
    }

    inline void matchSamplerState(const U32 idx, const U32 idx2, const U32 stored, const U32 read, const char* type)
    {
        // Accept certain defaults
        switch (idx2)
        {
        case GFXSAMPMagFilter:
        case GFXSAMPMinFilter:
        case GFXSAMPMipFilter:
            if (read == GFXTextureFilterLinear)
                return;
            break;

        case GFXSAMPAddressU:
        case GFXSAMPAddressV:
        case GFXSAMPAddressW:
            if (read == GFXAddressWrap)
                return;
        }

        if (stored != read)
        {
            Con::errorf("%s: Mismatching %s stage %d #%d: %d != %d", mName, type, idx, idx2, stored, read);
            mHasChanged = true;
        }
    }

    inline void matchMatrix(const MatrixF& a, const MatrixF& b, const char* name)
    {
        const F32* arrA = a;
        const F32* arrB = b;

        // Match us a matrix!
        for (U32 i = 0; i < 16; i++)
        {
            if (arrA[i] != arrB[i])
            {
                Con::errorf("%s: Mismatch in (%d, %d) of %s!", mName, i / 4, i % 4, name);
                mHasChanged = true;
            }
        }

    }

    void validate()
    {
        // Check the state vars first...
        for (U32 i = 0; i < GFXRenderState_COUNT; i++)
            matchState(i, mStates[i], GFX->mStateTracker[i].newValue, "render state");

        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
            for (U32 j = 0; j < GFXTSS_COUNT; j++)
                matchTextureState(i, j, mTextureStates[i][j], GFX->mTextureStateTracker[i][j].newValue, "texture state");

        for (U32 i = 0; i < TEXTURE_STAGE_COUNT; i++)
            for (U32 j = 0; j < GFXSAMP_COUNT; j++)
                matchSamplerState(i, j, mSamplerStates[i][j], GFX->mSamplerStateTracker[i][j].newValue, "sampler state");

        // Now check the transforms
        matchMatrix(mProj, GFX->getProjectionMatrix(), "projection");
        matchMatrix(mWorld, GFX->getWorldMatrix(), "world");

        // And the stack
        if (mWorldDepth != GFX->mWorldStackSize)
        {
            Con::errorf("%s: mismatched world matrix stack!", mName);
            mHasChanged = true;
        }

        AssertFatal(!mHasChanged, avar("GFXCanonizer %s: State was not restored! Check console for specifics.", mName));
    }

    ~_GFXCanonizer()
    {
        validate();
    }
};

#else
#define GFX_Canonizer(func, file, line)
#endif