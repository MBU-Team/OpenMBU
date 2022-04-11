//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GAMMABUFFER_H_
#define _GAMMABUFFER_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _SHADERDATA_H_
#include "materials/shaderData.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif


class GFXVertexBuffer;

//**************************************************************************
// Gamma Buffer
//**************************************************************************
class GammaBuffer : public SimObject
{
    typedef SimObject Parent;

private:
    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------
    ShaderData* mGammaShader;
    StringTableEntry mGammaShaderName;
    StringTableEntry mGammaRampBitmapName;
    GFXTexHandle        mSurface;
    GFXTexHandle mGammaRamp;
    S32                 mCallbackHandle;
    bool                mDisabled;

    GFXTextureTargetRef mTarget;

    MatrixF setupOrthoProjection();
    void setupRenderStates();
    static void texManagerCallback(GFXTexCallbackCode code, void* userData);

public:
    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
    GammaBuffer();

    static void initPersistFields();

    bool onAdd();
    void onRemove();

    void init();
    void copyToScreen(RectI& viewport);
    void setAsRenderTarget();
    bool isDisabled() { return mDisabled; }

    DECLARE_CONOBJECT(GammaBuffer);
};


#endif // _GAMMABUFFER_H_
