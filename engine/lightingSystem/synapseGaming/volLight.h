//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Portions of Volume Light by Parashar Krishnamachari
//-----------------------------------------------------------------------------

#ifndef _VOLLIGHT_H_
#define _VOLLIGHT_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

#include "gfx/gfxVertexBuffer.h"
#include "lightingSystem/sgLightObject.h"


class VolumeLight : public sgLightObject
{
private:
    typedef sgLightObject Parent;

    sgLightObjectData* mDataBlock;
public:
    void buildObjectBox()
    {
        mObjBox.min.set(-mXExtent, -mYExtent, -mShootDistance);
        mObjBox.max.set(mXExtent, mYExtent, mShootDistance);
        resetWorldBox();
    }
    void sgRenderX(const Point3F& near1, const Point3F& far1, const Point3F& far2,
        const ColorF& nearcol, const ColorF& farcol, F32 offset);
    void sgRenderY(const Point3F& near1, const Point3F& far1, const Point3F& far2,
        const ColorF& nearcol, const ColorF& farcol, F32 offset);
    //-----------------------------------------------
    // Lighting Pack code block
    //-----------------------------------------------


protected:
    enum
    {
        VolLightMask = BIT(0),
        VolLightOther = BIT(1),
    };

    GFXTexHandle   mLightHandle; ///< Light beam texture used.

public:
    VolumeLight();
    ~VolumeLight();

    StringTableEntry mLightTexture;      // Filename for light texture

    F32  mLPDistance;      // Distance to hypothetical lightsource point -- affects fov angle
    F32  mShootDistance;   // Distance of shooting -- Length of beams
    F32  mXExtent;         // xExtent and yExtent determine the size/dimension of the plane
    F32  mYExtent;

    U32  mSubdivideU;      // Number of subdivisions in U and V space.
    U32  mSubdivideV;      // Controls the number of "slices" in the volume.
    // NOTE : Total number of polygons = 2 + ((mSubdiveU + 1) + (mSubdivideV + 1)) * 2
    // Each slice being a quad plus the rectangular plane at the bottom.

    ColorF mFootColor;      // Color at the source
    ColorF mTailColor;      // Color at the end.

    GFXVertexBufferHandle<GFXVertexPCT> mVerts;
    GFXPrimitiveBufferHandle            mPrims;

    U32 mVertCount;
    U32 mPrimCount;

    void initBuffers();
    void makeDIPCall();

    // SceneObject functions
    virtual void renderObject(SceneState*, RenderInst* ri);
    virtual bool prepRenderImage(SceneState*, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState = false);

    // SimObject functions
    bool onAdd();
    void onRemove();
    void inspectPostApply();

    // NetObject functions
    U32 packUpdate(NetConnection*, U32, BitStream*);
    void unpackUpdate(NetConnection*, BitStream*);


    // ConObject functions
    static void initPersistFields();

    DECLARE_CONOBJECT(VolumeLight);
    bool onNewDataBlock(GameBaseData* dptr);
};

#endif   // _VOLLIGHT_H_