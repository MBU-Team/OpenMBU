//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGOBJECTBASEDPROJECTOR_H_
#define _SGOBJECTBASEDPROJECTOR_H_

#include "collision/depthSortList.h"
#include "materials/shaderData.h"
#include "sim/sceneObject.h"
#include "ts/tsShapeInstance.h"


class sgObjectBasedProjector : public ShadowBase
{
protected:
    Box3F sgBoundingBox;
    SceneObject* sgParentObject;
    virtual void sgCalculateBoundingBox() = 0;

public:
    sgObjectBasedProjector(SceneObject* parentobject);
    virtual ~sgObjectBasedProjector();
};

class sgShadowSharedZBuffer
{
private:
    static Point2I sgSize;
    static GFXTexHandle sgZBuffer;
public:
    static GFXTexHandle& sgGetZBuffer();
    static void sgPrepZBuffer(const Point2I& size);
    static void sgClear();
};

class sgShadowProjector : public sgObjectBasedProjector
{
protected:
    class sgShadow
    {
    private:
        S32 sgCurrentLOD;
    public:
        GFXTexHandle sgShadowTexture;
        GFXFormat sgShadowTextureFormat;
        sgShadow()
        {
            sgCurrentLOD = -1;
            //sgShadowTextureFormat = 0;
            sgShadowTexture = NULL;
        }
        ~sgShadow() { sgRelease(); }
        bool sgSetLOD(S32 lod, Point2I size);
        S32 sgGetLOD() { return sgCurrentLOD; }
        void sgRelease();
    };

    bool sgEnable;
    bool sgCanMove;
    bool sgCanRTT;
    bool sgCanSelfShadow;
    U32 sgRequestedShadowSize;
    U32 sgFrameSkip;
    F32 sgMaxVisibleDistance;
    F32 sgProjectionDistance;
    F32 sgSphereAdjust;
    F32 sgBias;
    LightInfo* sgLight;
    Point3F sgLightVector;
    MatrixF sgLightSpaceY;
    MatrixF sgLightToWorldY;
    MatrixF sgWorldToLightY;
    //MatrixF sgLightToWorldProjY;
    MatrixF sgLightProjToLightY;
    MatrixF sgWorldToLightProjY;
    //MatrixF sgWorldToLightZ;
    Point4F sgProjectionInfo;

    enum
    {
        sgspLastSelfShadowLOD = 3,
        sgspMaxLOD = 5
    };

    // shadow lod textures...
    //GFXTexHandle sgShadowLOD[sgspMaxLOD];
    Point2I sgShadowLODSize[sgspMaxLOD];
    // current texture, cached to avoid texture switch and re-RTT on lod switch...
    sgShadow sgShadowLODObject;
    //U32 sgLastLOD;
    //GFXTexHandle sgCurrentShadow;
    ShaderData* sgShadowBuilderShader;
    ShaderData* sgShadowShader;
    ShaderData* sgShadowBuilderShader_3_0;
    ShaderData* sgShadowShader_3_0;
    ShaderData* sgShadowBuilderShader_2_0;
    ShaderData* sgShadowShader_2_0;
    ShaderData* sgShadowBuilderShader_1_1;
    ShaderData* sgShadowShader_1_1;
    TSShapeInstance* sgShapeInstance;

    Vector<DepthSortList::Poly> sgShadowPolys;
    Vector<Point3F> sgShadowPoints;
    GFXVertexBufferHandle<GFXVertexPN> sgShadowBuffer;
    GFXTextureTargetRef mShadowBufferTarget;

    Point3F testRenderPoints[8];
    Point3F testRenderPointsWorld[8];

    enum sgShaderModel
    {
        sgsm_1_1,
        sgsm_2_0,
        sgsm_3_0,
    };
    sgShaderModel sgCalculateShaderModel();

    bool sgFirstMove;
    bool sgFirstRTT;
    sgShaderModel sgCachedShaderModel;
    Point3F sgCachedParentPos;
    U32 sgLastFrame;
    U32 sgCachedTextureDetailSize;
    U32 sgCachedParentTransformHash;
    U32 sgPreviousShadowTime;
    VectorF sgPreviousShadowLightingVector;
    bool sgShadowTypeDirty;
    bool sgAllowSelfShadowing() { return sgCachedShaderModel != sgsm_1_1; }
    void sgSetupShadowType();
    U32 sgGetShadowSize()
    {
        U32 size = sgRequestedShadowSize >> sgCachedTextureDetailSize;
        size = getMax(size, U32(64));
        return size;
    }
    void sgGetVariables();
    void sgGetLightSpaceBasedOnY();
    virtual void sgCalculateBoundingBox();
    void sgDebugRenderProjectionVolume();
    void sgRenderShape(TSShapeInstance* shapeinst,
        const MatrixF& trans1, S32 vertconstindex1,
        const MatrixF& trans2, S32 vertconstindex2);
    void sgRenderShadowBuffer();
    Point3F sgGetCompositeShadowLightDirection();
    void sgClear();

    template<class T> void sgDirtySync(T& dst, T src)
    {
        if (dst == src)
            return;
        dst = src;
        sgShadowTypeDirty = true;
    }

public:
    F32 sgProjectionScale;
    Box3F sgShadowBox;
    SphereF sgShadowSphere;
    Point3F sgShadowPoly[4];
    DepthSortList sgPolyGrinder;

public:
    U32 sgLastRenderTime;

    sgShadowProjector(SceneObject* parentobject,
        LightInfo* light, TSShapeInstance* shapeinstance);
    ~sgShadowProjector() { sgClear(); }
    virtual void sgRender(F32 camdist);
    static void collisionCallback(SceneObject* obj, void* shadow);
    bool shouldRender(F32 camDist);
    void render(F32 camDist);
    U32 getLastRenderTime() { return sgLastRenderTime; }
    /*void sgSetEnable(bool enable) {sgEnable = enable;}
    void sgSetCanMove(bool enable) {sgCanMove = enable;}
    void sgSetCanRTT(bool enable) {sgCanRTT = enable;}
    void sgSetCanSelfShadow(bool enable)
    {
        if(sgCanSelfShadow == enable)
            return;
        sgCanSelfShadow = enable;
        sgShadowTypeDirty = true;
    }
    void sgSetShadowSize(U32 size)
    {
        if(sgRequestedShadowSize == size)
            return;
        sgRequestedShadowSize = size;
        sgShadowTypeDirty = true;
    }
    void sgSetFrameSkip(U32 frameskip) {sgFrameSkip = frameskip;}
    void sgSetMaxVisibleDistance(F32 dist) {sgMaxVisibleDistance = dist;}
    void sgSetProjectionDistance(F32 dist) {sgProjectionDistance = dist;}
    void sgSetSphereAdjust(F32 adjust) {sgSphereAdjust = adjust;}*/
};


#endif//_SGOBJECTBASEDPROJECTOR_H_
