//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _GLOWBUFFER_H_
#define _GLOWBUFFER_H_

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
// Glow Buffer
//**************************************************************************
class GlowBuffer : public SimObject
{
    typedef SimObject Parent;

private:
    //--------------------------------------------------------------
    // Data
    //--------------------------------------------------------------
    ShaderData*         mBlurShader;
    const char*         mBlurShaderName;
    GFXTexHandle        mSurface[3];
    S32                 mCallbackHandle;
    bool                mDisabled;

    GFXVertexBufferHandle<GFXVertexPT> mVertBuff;
    GFXTextureTargetRef mTarget;

    void setupOrthoGeometry();
    MatrixF setupOrthoProjection();
    void setupRenderStates();
    void blur();
    static void texManagerCallback(GFXTexCallbackCode code, void* userData);

public:
    //--------------------------------------------------------------
    // Procedures
    //--------------------------------------------------------------
    GlowBuffer();

    static void initPersistFields();

    bool onAdd();
    void onRemove();

    void init();
    void copyToScreen(RectI& viewport);
    void setAsRenderTarget();
    bool isDisabled() { return mDisabled; }

    DECLARE_CONOBJECT(GlowBuffer);
};




#endif // GLOWBUFFER
