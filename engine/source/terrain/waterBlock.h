//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WATERBLOCK_H_
#define _WATERBLOCK_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "game/moveManager.h"
#endif
#ifndef _SCENEDATA_H_
#include "materials/sceneData.h"
#endif
#ifndef _REFLECTPLANE_H_
#include "gfx/reflectPlane.h"
#endif
#ifndef _CUSTOMMATERIAL_H_
#include "materials/customMaterial.h"
#endif

class AudioEnvironment;

//*****************************************************************************
// WaterBlock
//*****************************************************************************
class WaterBlock : public SceneObject
{
    typedef SceneObject Parent;

public:

    // LEGACY support
    enum EWaterType
    {
        eWater = 0,
        eOceanWater = 1,
        eRiverWater = 2,
        eStagnantWater = 3,
        eLava = 4,
        eHotLava = 5,
        eCrustyLava = 6,
        eQuicksand = 7,
    };

    enum MaterialType
    {
        BASE_PASS = 0,
        UNDERWATER_PASS = 1,
        FOG_PASS = 2,
        BLEND = 3,
        NO_REFLECT = 4,
        NUM_MAT_TYPES
    };

private:

    enum MaskBits {
        InitialUpdateMask = Parent::NextFreeMask,
        UpdateMask = InitialUpdateMask << 1,
        NextFreeMask = UpdateMask << 1
    };

    enum consts
    {
        MAX_WAVES = 4,
        NUM_ANIM_FRAMES = 32,
    };

    // wave parameters   
    Point2F  mWaveDir[MAX_WAVES];
    F32      mWaveSpeed[MAX_WAVES];
    Point2F  mWaveTexScale[MAX_WAVES];

    // vertex / index buffers
    Vector< GFXVertexBufferHandle<GFXVertexP>* > mVertBuffList;
    Vector< GFXPrimitiveBufferHandle* >          mPrimBuffList;
    GFXVertexBufferHandle<GFXVertexP>  mRadialVertBuff;
    GFXPrimitiveBufferHandle           mRadialPrimBuff;

    // misc
    bool           mFullReflect;
    F32            mGridElementSize;
    U32            mWidth;
    U32            mHeight;
    F32            mElapsedTime;
    ColorI         mBaseColor;
    ColorI         mUnderwaterColor;
    F32            mClarity;
    F32            mFresnelBias;
    F32            mFresnelPower;
    bool           mRenderFogMesh;
    Point3F        mPrevScale;
    GFXTexHandle   mBumpTex;
    EWaterType     mLiquidType;            ///< Type of liquid: Water?  Lava?  What?

    // reflect plane
    ReflectPlane   mReflectPlane;
    U32            mReflectTexSize;

    // materials
    const char* mSurfMatName[NUM_MAT_TYPES];
    MatInstance* mMaterial[NUM_MAT_TYPES];

    // for reflection update interval
    U32 mRenderUpdateCount;
    U32 mReflectUpdateCount;
    U32 mReflectUpdateTicks;



    SceneGraphData setupSceneGraphInfo(SceneState* state);
    void setShaderParams();
    void setupVBIB();
    void setupVertexBlock(U32 width, U32 height, U32 rowOffset);
    void setupPrimitiveBlock(U32 width, U32 height);
    void drawUnderwaterFilter();
    void render1_1(SceneGraphData& sgData, const Point3F& camPosition);
    void render2_0(SceneGraphData& sgData, const Point3F& camPosition);
    void animBumpTex(SceneState* state);
    void setupRadialVBIB();
    void setMultiPassProjection();
    void clearVertBuffers();


protected:

    //-------------------------------------------------------
    // Standard engine functions
    //-------------------------------------------------------
    bool onAdd();
    void onRemove();
    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);
    void renderObject(SceneState* state, RenderInst* ri);
    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);

public:
    WaterBlock();

    bool isPointSubmerged(const Point3F& pos, bool worldSpace = true) const { return true; }
    AudioEnvironment* getAudioEnvironment() { return NULL; }

    static void initPersistFields();

    EWaterType getLiquidType() const { return mLiquidType; }
    bool isUnderwater(const Point3F& pnt);

    virtual void updateReflection();
    virtual void inspectPostApply();


    DECLARE_CONOBJECT(WaterBlock);

};







#endif
