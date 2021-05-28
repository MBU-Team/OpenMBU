//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSLASTDETAIL_H_
#define _TSLASTDETAIL_H_

#ifndef _MATHTYPES_H_
#include "math/mathTypes.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

#include "gfx/gfxDevice.h"

class TSShape;
class TSShapeInstance;
class GBitmap;
class TextureHandle;

/// This neat little class renders the object to a texture so that when the object
/// is far away, it can be drawn as a billboard instead of a mesh.  This happens
/// when the model is first loaded as to keep the realtime render as fast as possible.
/// It also renders the model from a few different perspectives so that it would actually
/// pass as a model instead of a silly old billboard.
class TSLastDetail
{
    U32 mNumEquatorSteps; ///< number steps around the equator of the globe
    U32 mNumPolarSteps;   ///< number of steps to go from equator to each polar region (0 means equator only)
    F32 mPolarAngle;      ///< angle in radians of sub-polar regions
    bool mIncludePoles;   ///< include poles in snapshot?

    Point3F mCenter;

    /// remember these values so we can save some work next time we render...
    U32 mBitmapIndex;
    F32 mRotY;

    Vector<GBitmap*> mBitmaps;       ///< Rendered bitmaps
    Vector<GFXTexHandle> mTextures;///< Texture handles for those bitmaps

    Point3F mPoints[4];   ///< always draw poly defined by these points...
    static Point3F smNorms[4];
    static Point2F smTVerts[4];

public:

    /// This indicates that the TSLastDetail need neither clear nor set gl render states.
   ///
   /// If you're doing a more complex renderer this is a useful trick.
    static bool smDirtyMode;

    TSLastDetail(TSShapeInstance* shape, U32 numEquatorSteps, U32 numPolarSteps, F32 polarAngle, bool includePoles, S32 dl, S32 dim);
    ~TSLastDetail();

    void render(F32 alpha, bool drawFog);
    void renderNoFog(F32 alpha);
    void renderFog_Combine(F32 alpha);
    void renderFog_MultiCombine(F32 alpha);
    void chooseView(const MatrixF&, const Point3F& scale);
};


#endif // _TS_LAST_DETAIL


