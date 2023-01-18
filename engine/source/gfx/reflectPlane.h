//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _REFLECTPLANE_H_
#define _REFLECTPLANE_H_

#include "math/mMath.h"
#include "gfx/gfxTextureHandle.h"

//**************************************************************************
// Reflection plane
//**************************************************************************
class ReflectPlane
{
    PlaneF         mPlane;
    GFXTexHandle   mReflectTex;
    GFXTexHandle   mDepthTex;
    U32            mTexSize;


public:

    ReflectPlane();
    void setupTex(U32 size);
    void setPlane(PlaneF& plane) { mPlane = plane; }
    const PlaneF& getPlane() { return mPlane; }

    MatrixF getCameraReflection(MatrixF& camTrans);
    MatrixF getFrustumClipProj(MatrixF& modelview);
    GFXTexHandle& getTex() { return mReflectTex; }
    GFXTexHandle & getDepth() { return mDepthTex; }

};

#endif  // _REFLECTPLANE_H_
