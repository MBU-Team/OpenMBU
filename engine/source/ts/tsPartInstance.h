//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSPARTINSTANCE_H_
#define _TSPARTINSTANCE_H_

#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif

class TSPartInstance
{
    /// TSPartInstance assumes ownership (or shared ownership) of the source shape.  This means that the source
    /// shape cannot be deleted so long as the part instance is still around.  This also means that any change
    /// to source shapes's transforms or other animation properties will affect how the part instance displays.
    /// It is ok (even expected), however, to have many part instances accessing the same shape.
    TSShapeInstance* mSourceShape;

    /// @name Bounding info
    /// @{

    Box3F mBounds;
    Point3F mCenter;
    F32 mRadius;
    /// @}

    /// detail selection uses the table pointed to by this member
    ///
    /// if this member is blank, then it uses source shape to determine detail...
    ///
    /// detail 0 draws up until size of shape is less than mSizeCutoffs[0], detail 1 until mSizeCutoffs[1], etc.
    F32* mSizeCutoffs;
    S32* mPolyCount;
    S32 mNumDetails;

    /// @name Detail Levels
    /// detail levels on part instance correspond directly
    /// to object details on objects -- this is different
    /// from shape instances where dl corresponds to a
    /// subtree number and object detail.  The reason
    /// for this is that partinstances are derived from
    /// a single subtree of a shape instance, so the subtree
    /// is implied (implied by which objects are in the part instance)...
    /// @{
    S32 mCurrentObjectDetail;
    F32 mCurrentIntraDL;
    /// @}

    Vector<TSShapeInstance::MeshObjectInstance*> mMeshObjects;
    Vector<TSShapeInstance::DecalObjectInstance*> mDecalObjects;

    static MRandomR250 smRandom;

    void addObject(S32 objectIndex);
    void updateBounds();

    void renderDetailMap(S32 od);
    void renderEnvironmentMap(S32 od);
    void renderFog(S32 od);

    void init(TSShapeInstance*);

    static void breakShape(TSShapeInstance*, TSPartInstance*, S32 currentNode,
        Vector<TSPartInstance*>& partList, F32* probShatter,
        F32* probBreak, S32 probDepth);

    /// @name Private Detail Selection Methods
    /// @{
    void selectCurrentDetail(F32* sizeCutoffs, S32 numDetails, bool ignoreScale);
    void selectCurrentDetail(F32 pixelSize, F32* sizeCutoffs, S32 numDetails);
    void computePolyCount();
    /// @}

public:

    TSPartInstance(TSShapeInstance* source);
    TSPartInstance(TSShapeInstance* source, S32 objectIndex);
    ~TSPartInstance();

    const TSShape* getShape() { return mSourceShape->getShape(); }
    TSShapeInstance* getSourceShapeInstance() { return mSourceShape; }

    static void breakShape(TSShapeInstance*, S32 subShape, Vector<TSPartInstance*>& partList, F32* probShatter, F32* probBreak, S32 probDepth);

    Point3F& getCenter() { return mCenter; }
    Box3F& getBounds() { return mBounds; }
    F32& getRadius() { return mRadius; }

    void render(const Point3F* objectScale = NULL) { render(mCurrentObjectDetail, objectScale); }
    void render(S32 dl, const Point3F* objectScale = NULL);

    /// choose detail method -- pass in NULL for first parameter to just use shapes data
    void setDetailData(F32* sizeCutoffs, S32 numDetails);

    /// @name Detail Selection
    /// @{
    void selectCurrentDetail(bool ignoreScale = false);
    void selectCurrentDetail(F32 pixelSize);
    void selectCurrentDetail2(F32 adjustedDist);
    /// @}

    /// @name Detail Information Accessors
    /// @{
    F32 getDetailSize(S32 dl);
    S32 getPolyCount(S32 dl);
    S32 getNumDetails() { return mSizeCutoffs ? mNumDetails : mSourceShape->getShape()->mSmallestVisibleDL + 1; }

    S32 getCurrentObjectDetail() { return mCurrentObjectDetail; }
    void setCurrentObjectDetail(S32 od) { mCurrentObjectDetail = od; }
    F32 getCurrentIntraDetail() { return mCurrentIntraDL; }
    void setCurrentIntraDetail(F32 intra) { mCurrentIntraDL = intra; }
    /// @}

    void* mData; ///< for use by app
};

#endif

