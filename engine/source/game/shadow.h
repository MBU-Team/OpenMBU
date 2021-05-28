//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SHADOW_H_
#define _SHADOW_H_

#ifndef _DEPTHSORTLIST_H_
#include "collision/depthSortList.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif

class GBitmap;

class Shadow
{
    GBitmap* mBitmap;
    GFXTexHandle mShadowTexture;
    F32 mRadius;
    F32 mInvShadowDistance;
    MatrixF mLightToWorld;
    MatrixF mWorldToLight;

    Vector<DepthSortList::Poly> mPartition;
    Vector<Point3F> mPartitionVerts;
    Vector<Point2F> mPartitionTVerts;
    Vector<Point4F> mPartitionColors;

    struct ShadowSettings
    {
        // set by user (probably just once)
        bool alwaysUseGenericBmp;
        bool noAnimate;
        bool noMove;

        // determined frame to frame
        S32 bmpDim;
        S32 blur;
        U32 lastBmpTime;
        bool needBmp;
    } mSettings;

    static U32 smShadowMask;

    static DepthSortList smDepthSortList;
    static GFXTexHandle* smGenericShadowTexture;
    static S32 smGenericShadowDim;
    static S32 smInstanceCount;
    static F32 smShapeDetailScale;
    static S32 smShapeDetailMin;
    static F32 smSmallestVisibleSize;
    static F32 smGenericRadiusSkew;
    static bool smAlwaysUseGenericBmp;
    static F32 smLightDirSkew;
    static F32 smLightLenSkew;

    static F32 smGlobalShadowDetail;

    static void collisionCallback(SceneObject*, void*);

public:

    struct DistanceDetail
    {
        F32 dist;
        F32 directionSkew; /// 0-1, 0 means leave direction alone, 1 means light vector (0,0,-1)
        F32 lengthSkew;    /// 0-1, 0 means leave length alone, 1 means shadow volume has no depth
    };

    struct PixelSizeDetail
    {
        F32 size;
        U32 frameExpiration;
        S32 bmpDim;
        S32 blur;
        bool genericShadowBmp;
    };

    void setDetailTables(const DistanceDetail*, S32 ddCount, const PixelSizeDetail*, S32 psdCount);
    void setDefaultDetailTables();

    // this method changes several shadow detail parameters all at once -- range is 0-1, default value is 1
    static void setGlobalShadowDetailLevel(F32 d);
    static F32 getGlobalShadowDetailLevel() { return smGlobalShadowDetail; }

    void setGeneric(bool b) { mSettings.alwaysUseGenericBmp = b; }
    void setAnimating(bool b) { mSettings.noAnimate = !b; }
    void setMoving(bool b) { mSettings.noMove = !b; }

    static DistanceDetail smDefaultDistanceDetails[];
    static PixelSizeDetail smDefaultPixelSizeDetails[];

private:

    const DistanceDetail* mDistanceDetails;
    const PixelSizeDetail* mPixelSizeDetails;

    S32 mNumDistanceDetails;
    S32 mNumPixelSizeDetails;

    void findDistanceDetail(F32 dist, DistanceDetail*);
    void findPixelSizeDetail(F32 pixelSize, const PixelSizeDetail**);

private:
    void setLightMatrices(const Point3F& lightDir, const Point3F& pos);

    void buildPartition(const Point3F& p, const Point3F& lightDir, F32 radius, F32 shadowLen);
    void updatePartition(F32 fogAmount);

public:

    Shadow();
    ~Shadow();

    void beginRenderToBitmap();
    void endRenderToBitmap();
    void renderToBitmap(TSShapeInstance*, const MatrixF&, const Point3F& center, Point3F scale);
    GBitmap* getBitmap() { return mBitmap; }

    void setRadius(F32 radius);
    void setRadius(TSShapeInstance*, const Point3F& scale);

    bool prepare(const Point3F& pos, Point3F lightDir, F32 shadowLen, const Point3F& scale, F32 dist, F32 fogAmount, TSShapeInstance*);
    bool needBitmap() { return mSettings.needBmp; }
    S32 selectShapeDetail(TSShapeInstance*, F32 dist, F32 scale, S32 detailMin = -1);

    void render();

    static GBitmap* generateGenericShadowBitmap(S32 dim);

#ifdef TORQUE_DEBUG
    void dumpSort();
#endif
};

#endif // _SHADOW_H_


