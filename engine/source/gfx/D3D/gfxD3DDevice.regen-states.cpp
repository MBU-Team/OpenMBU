//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include <d3d9.h>
#include "gfx/D3D/gfxD3DDevice.h"
#include "console/console.h"
#include "gfx/D3D/gfxD3DEnumTranslate.h"

// Cut and paste from console.log into GFXD3DDevice::initStates()
void GFXD3DDevice::regenStates()
{
    DWORD temp;
    Con::printf("   //-------------------------------------");
    Con::printf("   // Auto-generated default states, see regenStates() for details");
    Con::printf("   //");
    Con::printf("");
    Con::printf("   // Render states");

    for (U32 state = GFXRenderState_FIRST; state < GFXRenderState_COUNT; state++)
    {
        temp = 0;
        mD3DDevice->GetRenderState(GFXD3DRenderState[state], &temp);

        // Now we need to do a reverse-lookup here to turn 'temp' into a GFX enum
        switch (state)
        {
        case GFXRSSrcBlend:
        case GFXRSDestBlend:
        case GFXRSSrcBlendAlpha:
        case GFXRSDestBlendAlpha:
            GFXREVERSE_LOOKUP(GFXD3DBlend, GFXBlend, temp);
            break;

        case GFXRSBlendOp:
        case GFXRSBlendOpAlpha:
            GFXREVERSE_LOOKUP(GFXD3DBlendOp, GFXBlendOp, temp);
            break;

        case GFXRSStencilFail:
        case GFXRSStencilZFail:
        case GFXRSStencilPass:
        case GFXRSCCWStencilFail:
        case GFXRSCCWStencilZFail:
        case GFXRSCCWStencilPass:
            GFXREVERSE_LOOKUP(GFXD3DStencilOp, GFXStencilOp, temp);
            break;

        case GFXRSZFunc:
        case GFXRSAlphaFunc:
        case GFXRSStencilFunc:
        case GFXRSCCWStencilFunc:
            GFXREVERSE_LOOKUP(GFXD3DCmpFunc, GFXCmp, temp);
            break;

        case GFXRSCullMode:
            GFXREVERSE_LOOKUP(GFXD3DCullMode, GFXCull, temp);
            break;
        }

        Con::printf("   initRenderState( %d, %d );", state, temp);
    }

    Con::printf("");
    Con::printf("   // Texture Stage states");

    for (U32 stage = 0; stage < TEXTURE_STAGE_COUNT; stage++)
    {
        for (U32 state = GFXTSS_FIRST; state < GFXTSS_COUNT; state++)
        {
            temp = 0;
            mD3DDevice->GetTextureStageState(stage, GFXD3DTextureStageState[state], &temp);

            switch (state)
            {
            case GFXTSSColorOp:
            case GFXTSSAlphaOp:
                GFXREVERSE_LOOKUP(GFXD3DTextureOp, GFXTOP, temp);
                break;
            }

            Con::printf("   initTextureState( %d, %d, %d );", stage, state, temp);
        }
    }

    Con::printf("");
    Con::printf("   // Sampler states");
    for (U32 stage = 0; stage < TEXTURE_STAGE_COUNT; stage++)
    {
        for (U32 state = GFXSAMP_FIRST; state < GFXSAMP_COUNT; state++)
        {
            temp = 0;
            mD3DDevice->GetSamplerState(stage, GFXD3DSamplerState[state], &temp);

            switch (state)
            {
            case GFXSAMPMagFilter:
            case GFXSAMPMinFilter:
            case GFXSAMPMipFilter:
                GFXREVERSE_LOOKUP(GFXD3DTextureFilter, GFXTextureFilter, temp);
                break;

            case GFXSAMPAddressU:
            case GFXSAMPAddressV:
            case GFXSAMPAddressW:
                GFXREVERSE_LOOKUP(GFXD3DTextureAddress, GFXAddress, temp);
                break;
            }

            Con::printf("   initSamplerState( %d, %d, %d );", stage, state, temp);
        }
    }
}
