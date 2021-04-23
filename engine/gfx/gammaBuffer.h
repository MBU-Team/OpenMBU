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

struct GammaRamp {
    struct NormalEntry {
        union {
            struct {
                U32 b : 10;
                U32 g : 10;
                U32 r : 10;
                U32 : 2;
            };
            U32 value;
        };
    };

    struct PWLValue {
        union {
            struct {
                U16 base;
                U16 delta;
            };
            U32 value;
        };
    };

    struct PWLEntry {
        union {
            struct {
                PWLValue r;
                PWLValue g;
                PWLValue b;
            };
            PWLValue values[3];
        };
    };

    NormalEntry normal[256];
    PWLEntry pwl[128];
};

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
    const char* mGammaShaderName;
    GFXTexHandle        mSurface;
    GammaRamp        mGammaRamp;
    U32           mMapping[256];
    U32           mMappingPWL[128];
    GFXTexHandle    mGamma;
    GFXTexHandle    mGammaPWL;
    S32                 mCallbackHandle;
    bool                mDisabled;

    GFXVertexBufferHandle<GFXVertexPT> mVertBuff;

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
