//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_INST_MGR_H_
#define _RENDER_INST_MGR_H_

#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif

#ifndef _SCENESTATE_H_
#include "sceneGraph/sceneState.h"
#endif

#ifndef _SHADERTDATA_H_
#include "materials/shaderData.h"
#endif

#include "sceneGraph/lightInfo.h"

class MatInstance;
class SceneGraphData;
class ShaderData;
class Sky;
class RenderElemMgr;
class RenderZOnlyMgr;

//**************************************************************************
// Render Instance
//**************************************************************************
struct RenderInst
{
    GFXVertexBufferHandleBase* vertBuff;
    GFXPrimitiveBufferHandle* primBuff;

    GFXPrimitive* prim;

    U32 primBuffIndex;
    //U32 primStartIndex;
    //U32 primCount;
    MatInstance* matInst;

    // transforms
    MatrixF* worldXform;     // world/projection transform
    MatrixF* objXform;       // obj space transform

    SceneState* state;               // need this for straight up object renders
    SceneObject* obj;                // need this for straight up object renders

    // sorting
    U8 type;                   // sort overrides (draw sky last, etc.)
    Point3F sortPoint;         // just points for now - nothing usese plane


    // misc 
    bool  translucent;  // this could be passed into addInst()?
    bool  particles;
    U8    transFlags;
    bool  reflective;
    F32   visibility;

    // mesh related
    S32   mountedObjIdx;  // for debug rendering on ShapeBase objects
    U32   texWrapFlags;


    // lighting...
    bool* primitiveFirstPass;
    LightInfo light;
    LightInfo lightSecondary;
    GFXTexHandle dynamicLight;
    GFXTexHandle dynamicLightSecondary;


    // textures
    GFXTexHandle* lightmap;
    GFXTexHandle* normLightmap;
    GFXTexHandle* fogTex;
    GFXTexHandle* backBuffTex;
    GFXTexHandle* reflectTex;
    GFXTextureObject* miscTex;
    GFXCubemap* cubemap;


    // methods
    void clear()
    {
        dMemset(this, 0, sizeof(RenderInst));
        visibility = 1.0f;
    }

    void calcSortPoint(SceneObject* obj, const Point3F& camPosition);
};


//**************************************************************************
// Render Instance Manager
//**************************************************************************
class RenderInstManager
{
    //-------------------------------------
    // structs / enum
    //-------------------------------------
public:

    enum RenderInstType
    {
        RIT_Interior = 0,
        RIT_InteriorDynamicLighting,
        RIT_Mesh,
        RIT_Shadow,
        RIT_Sky,
        RIT_Astrolabe,
        RIT_Glass,
        RIT_Refraction,
        RIT_MarbleShadow,
        RIT_Marble,
        RIT_Glow,
        RIT_Object,      // terrain, water, etc. objects that do their own rendering
        RIT_ObjectTranslucent,// self rendering, but sorted with RIT_Translucent
        RIT_Decal,
        RIT_Water,
        RIT_Foliage,
        RIT_Translucent,
        RIT_Begin,
        RIT_NumTypess
    };

    // bins are rendered in this order
    enum RenderBinTypes
    {
        Begin = 0,
#ifndef MB_FLIP_SKY
        Sky,
        SkyShape,
#endif
        Interior,
        InteriorDynamicLighting,
        Mesh,
        MarbleShadow,
        Marble,
#ifdef MB_FLIP_SKY
        Sky,
        SkyShape,
#endif
        MiscObject,
        Shadow,
        Decal,
        Water,
        TranslucentPreGlow,
        Glow,
        Refraction,
        Foliage,
        Translucent,
        NumRenderBins
    };

private:

    //-------------------------------------
    // data
    //-------------------------------------
    Chunker< RenderInst >      mRIAllocator;
    Chunker< MatrixF >         mXformAllocator;
    Vector< RenderElemMgr* >  mRenderBins;
    Vector<bool> mRenderRenderBin;

    // for lighting passes...
    Chunker<bool>         mPrimitiveFirstPassAllocator;

    bool mInitialized;
    ShaderData* mBlankShader;
    MatInstance* mWarningMat;
    Point3F mCamPos;
    RenderZOnlyMgr* mZOnlyBin;

    void handleGFXEvent(GFXDevice::GFXDeviceEventType event);
    void initBins();
    void uninitBins();
    void initWarnMat();

    void init();

public:

    //-------------------------------------
    // main interface
    //-------------------------------------
    RenderInstManager();
    ~RenderInstManager();

    RenderInst* allocInst()
    {
        RenderInst* inst = mRIAllocator.alloc();
        inst->clear();
        return inst;
    }
    void addInst(RenderInst* inst);
    MatrixF* allocXform() { return mXformAllocator.alloc(); }

    // for lighting...
    bool* allocPrimitiveFirstPass() { return mPrimitiveFirstPassAllocator.alloc(); }

    void uninit();
    void clear();  // clear instances, matrices
    void sort();
    void render();
    void renderToZBuff(GFXTarget* target);
    void renderGlow();

    void setCamPos(Point3F& camPos) { mCamPos = camPos; }
    Point3F getCamPos() { return mCamPos; }
    MatInstance* getWarningMat();
};

extern RenderInstManager gRenderInstManager;



#endif
