//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGFORMATMANAGER_H_
#define _SGFORMATMANAGER_H_

GFX_DeclareTextureProfile(ShadowTargetTextureProfile);
GFX_DeclareTextureProfile(ShadowZTargetTextureProfile);
GFX_DeclareTextureProfile(DRLTargetTextureProfile);


class sgFormatManager
{
public:
    static GFXFormat sgShadowTextureFormat_2_0;
    static GFXFormat sgShadowTextureFormat_1_1;
    static GFXFormat sgShadowZTextureFormat;
    static GFXFormat sgDRLTextureFormat;
    static GFXFormat sgHDRTextureFormat;
    static void sgInit();
};


#endif//_SGFORMATMANAGER_H_
