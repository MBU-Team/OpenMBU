//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderInstMgr.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"
#include "materials/customMaterial.h"
#include "sceneGraph/sceneGraph.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "terrain/environment/sky.h"
#include "renderElemMgr.h"
#include "renderObjectMgr.h"
#include "renderInteriorMgr.h"
#include "renderMeshMgr.h"
#include "renderRefractMgr.h"
#include "renderTranslucentMgr.h"
#include "renderGlowMgr.h"
#include "renderZOnlyMgr.h"
#include "renderMarbleShadowMgr.h"

#define POOL_GROW_SIZE 2048
#define HIGH_NUM ((U32(-1)/2) - 1)

RenderInstManager gRenderInstManager;

//*****************************************************************************
// RenderInstance
//*****************************************************************************

//-----------------------------------------------------------------------------
// calcSortPoint
//-----------------------------------------------------------------------------
void RenderInst::calcSortPoint(SceneObject* obj, const Point3F& camPosition)
{
    if (!obj) return;

    const Box3F& rBox = obj->getObjBox();
    Point3F objSpaceCamPosition = camPosition;
    obj->getRenderWorldTransform().mulP(objSpaceCamPosition);
    objSpaceCamPosition.convolveInverse(obj->getScale());
    sortPoint = rBox.getClosestPoint(objSpaceCamPosition);
    sortPoint.convolve(obj->getScale());
    obj->getRenderTransform().mulP(sortPoint);

}

//*****************************************************************************
// Render Instance Manager
//*****************************************************************************
RenderInstManager::RenderInstManager()
{
    mBlankShader = NULL;
    mWarningMat = NULL;
    mInitialized = false;
    GFXDevice::getDeviceEventSignal().notify(this, &RenderInstManager::handleGFXEvent);
}

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
RenderInstManager::~RenderInstManager()
{
    uninit();
}

//-----------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------
void RenderInstManager::handleGFXEvent(GFXDevice::GFXDeviceEventType event)
{
    switch (event)
    {
        case GFXDevice::deInit :
            init();
            break;
        case GFXDevice::deDestroy :
            uninit();
            break;
    }
}

void RenderInstManager::init()
{
    if (!mInitialized)
    {
        initBins();
        initWarnMat();
        mInitialized = true;
    }
}

void RenderInstManager::uninit()
{
    if (mInitialized)
    {
        uninitBins();
        if (mWarningMat)
        {
            SAFE_DELETE(mWarningMat);
        }
        mInitialized = false;
    }
}

//-----------------------------------------------------------------------------
// initBins
//-----------------------------------------------------------------------------
void RenderInstManager::initBins()
{
    mRenderBins.setSize(NumRenderBins);
    dMemset(mRenderBins.address(), 0, mRenderBins.size() * sizeof(RenderElemMgr*));

    //mRenderBins[Begin] = new RenderObjectMgr;
    mRenderBins[Sky] = new RenderObjectMgr;
    mRenderBins[SkyShape] = new RenderTranslucentMgr;
    mRenderBins[Interior] = new RenderInteriorMgr;
    mRenderBins[InteriorDynamicLighting] = new RenderInteriorMgr;
    mRenderBins[Mesh] = new RenderMeshMgr;
    mRenderBins[MarbleShadow] = new RenderMarbleShadowMgr;
    mRenderBins[Marble] = new RenderMeshMgr;
    mRenderBins[Shadow] = new RenderObjectMgr;
    mRenderBins[MiscObject] = new RenderObjectMgr;
    mRenderBins[Decal] = new RenderObjectMgr;
    mRenderBins[Refraction] = new RenderRefractMgr;
    mRenderBins[Water] = new RenderObjectMgr;
    mRenderBins[Foliage] = new RenderObjectMgr;
    mRenderBins[Translucent] = new RenderTranslucentMgr;
    mRenderBins[TranslucentPreGlow] = new RenderTranslucentMgr;
    mRenderBins[Glow] = new RenderGlowMgr;

    mZOnlyBin = new RenderZOnlyMgr;

    mRenderRenderBin.setSize(NumRenderBins);
    for (S32 i = 0; i < NumRenderBins; i++)
    {
        mRenderRenderBin[i] = true;
        char varName[256];
        dSprintf(varName, 256, "RIMrender%d", i);
        if (Con::getBoolVariable(varName, true))
            Con::addVariable(varName, TypeBool, &mRenderRenderBin[i]);
    }
}

//-----------------------------------------------------------------------------
// uninitBins
//-----------------------------------------------------------------------------
void RenderInstManager::uninitBins()
{
    for (U32 i = 0; i < mRenderBins.size(); i++)
    {
        if (mRenderBins[i])
        {
            delete mRenderBins[i];
            mRenderBins[i] = NULL;
        }
    }

    if (mZOnlyBin)
    {
        delete mZOnlyBin;
        mZOnlyBin = NULL;
    }
}

//-----------------------------------------------------------------------------
// add instance
//-----------------------------------------------------------------------------
void RenderInstManager::addInst(RenderInst* inst)
{
    AssertISV(mInitialized, "RenderInstManager not initialized - call console function 'initRenderInstManager()'");
    AssertFatal(inst != NULL, "doh, null instance");

    if (inst->type == RIT_MarbleShadow) {
        mRenderBins[MarbleShadow]->addElement(inst);
        return;
    }

    PROFILE_START(RenderInstManager_addInst);

    PROFILE_START(RenderInstManager_selectBin0);

    Material* instMat = NULL;
    if (inst->matInst != NULL)
        instMat = inst->matInst->getMaterial();

    bool hasGlow = true;
    if (!inst->matInst || !inst->matInst->hasGlow() || getCurrentClientSceneGraph()->isReflectPass() || inst->obj)
        hasGlow = false;

    // handle special cases that don't require insertion into multiple bins
    if (inst->matInst && instMat && instMat->renderBin)
    {
        PROFILE_END();
        AssertFatal(instMat->renderBin >= 0 && instMat->renderBin < NumRenderBins, "doh, invalid bin, check the renderBin property for this material");
        mRenderBins[instMat->renderBin]->addElement(inst);
    }
    else if (inst->translucent || (instMat && instMat->isTranslucent()) || (inst->visibility < 1.0f))
    {
        PROFILE_END();
        if (!hasGlow)
            mRenderBins[Translucent]->addElement(inst);
    }
    else
    {
        PROFILE_END();

        PROFILE_START(RenderInstManager_selectBin1);

        // handle standard cases
        switch (inst->type)
        {
        case RIT_Interior:
            mRenderBins[Interior]->addElement(inst);
            mZOnlyBin->addElement(inst);
            break;

        case RIT_InteriorDynamicLighting:
            mRenderBins[InteriorDynamicLighting]->addElement(inst);
            break;

        case RIT_Shadow:
            mRenderBins[Shadow]->addElement(inst);
            break;

        case RIT_Decal:
            mRenderBins[Decal]->addElement(inst);
            break;

        case RIT_Sky:
            mRenderBins[Sky]->addElement(inst);
            break;

        case RIT_Water:
            mRenderBins[Water]->addElement(inst);
            break;

        case RIT_Mesh:
            mRenderBins[Mesh]->addElement(inst);
            mZOnlyBin->addElement(inst);
            break;

        case RIT_Foliage:
            mRenderBins[Foliage]->addElement(inst);
            break;

        case RIT_Begin:
            mRenderBins[Begin]->addElement(inst);
            break;

        default:
            mRenderBins[MiscObject]->addElement(inst);
            break;
        }
        PROFILE_END();
    }

    PROFILE_START(RenderInstMgr_specialBin);

    // handle extra insertions
    if (inst->matInst)
    {
        PROFILE_START(RenderInstMgr_custDynCast); // wonder if this will actually be non zero on the profiler...
        CustomMaterial* custMat = dynamic_cast<CustomMaterial*>(inst->matInst->getMaterial());
        PROFILE_END();

        if (custMat && custMat->refract)
        {
            mRenderBins[Refraction]->addElement(inst);
        }

        if (inst->matInst->hasGlow() &&
            !getCurrentClientSceneGraph()->isReflectPass() &&
            !inst->obj)
        {
            mRenderBins[Glow]->addElement(inst);
        }
    }

    PROFILE_END();

    PROFILE_END();
}


//-----------------------------------------------------------------------------
// QSort callback function
//-----------------------------------------------------------------------------
S32 FN_CDECL cmpKeyFunc(const void* p1, const void* p2)
{
    const RenderElemMgr::MainSortElem* mse1 = (const RenderElemMgr::MainSortElem*)p1;
    const RenderElemMgr::MainSortElem* mse2 = (const RenderElemMgr::MainSortElem*)p2;

    S32 test1 = S32(mse1->key) - S32(mse2->key);

    return (test1 == 0) ? S32(mse1->key2) - S32(mse2->key2) : test1;
}

//-----------------------------------------------------------------------------
// sort
//-----------------------------------------------------------------------------
void RenderInstManager::sort()
{
    PROFILE_START(RIM_sort);

    if (mRenderBins.size())
    {
        for (U32 i = 0; i < NumRenderBins; i++)
        {
            if (mRenderBins[i])
            {
                mRenderBins[i]->sort();
            }
        }
    }

    mZOnlyBin->sort();

    PROFILE_END();
}

//-----------------------------------------------------------------------------
// clear
//-----------------------------------------------------------------------------
void RenderInstManager::clear()
{
    mRIAllocator.clear();
    mXformAllocator.clear();
    mPrimitiveFirstPassAllocator.clear();

    if (mRenderBins.size())
    {
        for (U32 i = 0; i < NumRenderBins; i++)
        {
            if (mRenderBins[i])
            {
                mRenderBins[i]->clear();
            }
        }
    }

    mZOnlyBin->clear();
}

//-----------------------------------------------------------------------------
// initialize warning material instance
//-----------------------------------------------------------------------------
void RenderInstManager::initWarnMat()
{
    Material* warnMat = static_cast<Material*>(Sim::findObject("WarningMaterial"));
    if (!warnMat)
    {
        Con::errorf("Can't find WarningMaterial");
    }
    else
    {
        SceneGraphData sgData;
        GFXVertexPNTTBN* vertDef = NULL; // the variable itself is the parameter to the template function
        mWarningMat = new MatInstance(*warnMat);
        mWarningMat->init(sgData, (GFXVertexFlags)getGFXVertFlags(vertDef));
    }
}

MatInstance *RenderInstManager::getWarningMat()
{
    if (!mWarningMat)
        initWarnMat();

    return mWarningMat;
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderInstManager::render()
{
    GFX->pushWorldMatrix();
    MatrixF proj = GFX->getProjectionMatrix();

    if (!mRenderBins.empty())
    {
        for (U32 i = 0; i < NumRenderBins; i++)
        {
            if (mRenderBins[i] && mRenderRenderBin[i])
            {
                mRenderBins[i]->render();
            }
        }
    }

    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
}

void RenderInstManager::renderToZBuff(GFXTarget* target)
{
    GFX->pushActiveRenderTarget();
    GFX->setActiveRenderTarget(target);
    mZOnlyBin->render();
    GFX->popActiveRenderTarget();
}


//*****************************************************************************
// Console functions
//*****************************************************************************

//-----------------------------------------------------------------------------
// initRenderInstManager - do this through script because there's no good place to
// init after device creation in code.
//-----------------------------------------------------------------------------
/*ConsoleFunction( initRenderInstManager, void, 1, 1, "initRenderInstManager")
{
   gRenderInstManager.init();
}*/
