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
#include "lightingSystem/sgFormatManager.h"


GFX_ImplementTextureProfile(ShadowTargetTextureProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::RenderTarget | GFXTextureProfile::PreserveSize,
    GFXTextureProfile::None);

GFX_ImplementTextureProfile(ShadowZTargetTextureProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::ZTarget | GFXTextureProfile::PreserveSize,
    GFXTextureProfile::None);

GFX_ImplementTextureProfile(DRLTargetTextureProfile,
    GFXTextureProfile::DiffuseMap,
    GFXTextureProfile::RenderTarget | GFXTextureProfile::PreserveSize,
    GFXTextureProfile::None);


GFXFormat sgFormatManager::sgShadowTextureFormat_2_0 = GFXFormatR16F;
GFXFormat sgFormatManager::sgShadowTextureFormat_1_1 = GFXFormatR5G5B5X1;
GFXFormat sgFormatManager::sgShadowZTextureFormat = GFXFormatD16;
GFXFormat sgFormatManager::sgDRLTextureFormat = GFXFormatR8G8B8A8;
GFXFormat sgFormatManager::sgHDRTextureFormat = GFXFormatR8G8B8A8;


void sgFormatManager::sgInit()
{
    Vector<GFXFormat> formats;

    // invalid format...
    //formats.push_back(GFXFormatL16);
    formats.push_back(GFXFormatR16F);
    formats.push_back(GFXFormatR16G16);
    formats.push_back(GFXFormatR16G16F);
    // added for intel DX9 cards...
    formats.push_back(GFXFormatR10G10B10A2);
    // worst case...
    // do not use this - too large!!!
    //formats.push_back(GFXFormatR16G16B16A16);
    formats.push_back(GFXFormatR8G8B8);
    formats.push_back(GFXFormatR8G8B8X8);
    formats.push_back(GFXFormatR8G8B8A8);
    sgShadowTextureFormat_2_0 = GFX->selectSupportedFormat(
        &ShadowTargetTextureProfile, formats, true, false);

    formats.clear();
    // invalid format...
    //formats.push_back(GFXFormatL8);
    // these are way too large for what should be 8bit targets...
    formats.push_back(GFXFormatR5G5B5X1);
    formats.push_back(GFXFormatR5G5B5A1);
    // worst case...
    formats.push_back(GFXFormatR8G8B8);
    formats.push_back(GFXFormatR8G8B8X8);
    formats.push_back(GFXFormatR8G8B8A8);
    sgShadowTextureFormat_1_1 = GFX->selectSupportedFormat(
        &ShadowTargetTextureProfile, formats, true, false);

    formats.clear();
    formats.push_back(GFXFormatD16);
    formats.push_back(GFXFormatD32);
    formats.push_back(GFXFormatD24X8);
    sgShadowZTextureFormat = GFX->selectSupportedFormat(
        &ShadowZTargetTextureProfile, formats, false, false);

    formats.clear();
    // must be this format - need alpha!!!
    formats.push_back(GFXFormatR8G8B8A8);
    sgDRLTextureFormat = GFX->selectSupportedFormat(
        &DRLTargetTextureProfile, formats, true, true);

    formats.clear();
    // must be this format - need alpha!!!
    formats.push_back(GFXFormatR16G16B16A16F);
    formats.push_back(GFXFormatR8G8B8A8);
    sgHDRTextureFormat = GFX->selectSupportedFormat(
        &DRLTargetTextureProfile, formats, true, true);
}

