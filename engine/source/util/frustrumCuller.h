//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FRUSTRUMCULLER_H_
#define _FRUSTRUMCULLER_H_

#include "math/mBox.h"
#include "sceneGraph/sceneState.h"

/// Helper class to perform efficient frustrum culling on bounding boxes.
class FrustrumCuller
{
public:
    enum
    {
        /// We need to store at least 4 clip planes (sides of view frustrum).
        MaxClipPlanes = 5,

        // Clipping related flags...
        ClipPlaneMask = BIT(MaxClipPlanes) - 1,
        FarSphereMask = BIT(MaxClipPlanes + 1),
        FogPlaneBoxMask = BIT(MaxClipPlanes + 2),

        AllClipPlanes = ClipPlaneMask | FarSphereMask,

    };

    SceneState* mSceneState;
    Point3F     mCamPos;
    F32         mFarDistance;
    U32         mNumClipPlanes;
    PlaneF      mClipPlane[MaxClipPlanes];

    FrustrumCuller()
    {
        mSceneState = NULL;
    }

    void init(SceneState* state);
    const S32  testBoxVisibility(const Box3F& bounds, const S32 mask, const F32 expand) const;
    const F32  getBoxDistance(const Box3F& box) const;
};

#endif