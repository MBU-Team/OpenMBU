//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderInstMgr.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "../../example/shaders/shdrConsts.h"
#include "materials/customMaterial.h"
#include "sceneGraph/sceneGraph.h"
#include "gfx/primbuilder.h"
#include "platform/profiler.h"
#include "terrain/sky.h"
#include "renderElemMgr.h"
#include "renderObjectMgr.h"
#include "renderInteriorMgr.h"
#include "renderMeshMgr.h"
#include "renderRefractMgr.h"
#include "renderTranslucentMgr.h"
#include "renderGlowMgr.h"

#define POOL_GROW_SIZE 2048
#define HIGH_NUM ((U32(-1)/2) - 1)

RenderInstManager gRenderInstManager;

//*****************************************************************************
// RenderInstance
//*****************************************************************************

//-----------------------------------------------------------------------------
// calcSortPoint
//-----------------------------------------------------------------------------
void RenderInst::calcSortPoint( SceneObject *obj, const Point3F &camPosition )
{
   if( !obj ) return;

   const Box3F& rBox = obj->getObjBox();
   Point3F objSpaceCamPosition = camPosition;
   obj->getRenderWorldTransform().mulP( objSpaceCamPosition );
   objSpaceCamPosition.convolveInverse( obj->getScale() );
   sortPoint = rBox.getClosestPoint( objSpaceCamPosition );
   sortPoint.convolve( obj->getScale() );
   obj->getRenderTransform().mulP( sortPoint );

}

//*****************************************************************************
// Render Instance Manager
//*****************************************************************************
RenderInstManager::RenderInstManager()
{
   mBlankShader = NULL;
   mWarningMat = NULL;
   mInitialized = false;
}

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
RenderInstManager::~RenderInstManager()
{
   uninitBins();
   if( mWarningMat )
   {
      delete mWarningMat;
      mWarningMat = NULL;
   }
}

//-----------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------
void RenderInstManager::init()
{
   initBins();
   initWarnMat();
   mInitialized = true;
}

//-----------------------------------------------------------------------------
// initBins
//-----------------------------------------------------------------------------
void RenderInstManager::initBins()
{
   mRenderBins.setSize( NumRenderBins );
   dMemset( mRenderBins.address(), 0, mRenderBins.size() * sizeof(RenderElemMgr*) );

   mRenderBins[Sky]           = new RenderObjectMgr;
   mRenderBins[Begin]         = new RenderObjectMgr;
   mRenderBins[Interior]      = new RenderInteriorMgr;
   mRenderBins[InteriorDynamicLighting] = new RenderInteriorMgr;
   mRenderBins[Mesh]          = new RenderMeshMgr;
   mRenderBins[Shadow]        = new RenderObjectMgr;
   mRenderBins[MiscObject]    = new RenderObjectMgr;
   mRenderBins[Decal]         = new RenderObjectMgr;
   mRenderBins[Refraction]    = new RenderRefractMgr;
   mRenderBins[Water]         = new RenderObjectMgr;
   mRenderBins[Foliage]       = new RenderObjectMgr;
   mRenderBins[Translucent]   = new RenderTranslucentMgr;
   mRenderBins[Glow]          = new RenderGlowMgr;
}

//-----------------------------------------------------------------------------
// uninitBins
//-----------------------------------------------------------------------------
void RenderInstManager::uninitBins()
{
   for( U32 i=0; i<mRenderBins.size(); i++ )
   {
      if( mRenderBins[i] )
      {
         delete mRenderBins[i];
         mRenderBins[i] = NULL;
      }
   }
}

//-----------------------------------------------------------------------------
// add instance
//-----------------------------------------------------------------------------
void RenderInstManager::addInst( RenderInst *inst )
{
    if (!mInitialized)
        gRenderInstManager.init();

   AssertISV( mInitialized, "RenderInstManager not initialized - call console function 'initRenderInstManager()'" );

   // handle special cases that don't require insertion into multiple bins
   if( inst->translucent || (inst->matInst && inst->matInst->getMaterial()->translucent ) )
   {
      mRenderBins[ Translucent ]->addElement( inst );
      return;
   }

   // handle standard cases
   switch( inst->type )
   {
      case RIT_Interior:
         mRenderBins[ Interior ]->addElement( inst );
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
         mRenderBins[ Sky ]->addElement( inst );
         break;

      case RIT_Water:
         mRenderBins[ Water ]->addElement( inst );
         break;

      case RIT_Mesh:
         mRenderBins[ Mesh ]->addElement( inst );
         break;

      case RIT_Foliage:
         mRenderBins[ Foliage ]->addElement( inst );
         break;

      case RIT_Begin:
         mRenderBins[ Begin ]->addElement( inst );
         break;

      default:
         mRenderBins[ MiscObject ]->addElement( inst );
         break;

   }

   // handle extra insertions
   if( inst->matInst )
   {
      CustomMaterial *custMat = dynamic_cast<CustomMaterial*>( inst->matInst->getMaterial() );
      if( custMat && custMat->refract )
      {
         mRenderBins[ Refraction ]->addElement( inst );
      }

      if( inst->matInst->hasGlow() && 
         !gClientSceneGraph->isReflectPass() &&
         !inst->obj )
      {
         mRenderBins[ Glow ]->addElement( inst );
      }
   }
}


//-----------------------------------------------------------------------------
// QSort callback function
//-----------------------------------------------------------------------------
S32 FN_CDECL cmpKeyFunc(const void* p1, const void* p2)
{
   const RenderElemMgr::MainSortElem* mse1 = (const RenderElemMgr::MainSortElem*) p1;
   const RenderElemMgr::MainSortElem* mse2 = (const RenderElemMgr::MainSortElem*) p2;

   S32 test1 = S32(mse1->key) - S32(mse2->key);

   return ( test1 == 0 ) ? S32(mse1->key2) - S32(mse2->key2) : test1;
}

//-----------------------------------------------------------------------------
// sort
//-----------------------------------------------------------------------------
void RenderInstManager::sort()
{
   PROFILE_START(RIM_sort);

   if( mRenderBins.size() )
   {
      for( U32 i=0; i<NumRenderBins; i++ )
      {
         if( mRenderBins[i] )
         {
            mRenderBins[i]->sort();
         }
      }
   }

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
   
   if( mRenderBins.size() )
   {
      for( U32 i=0; i<NumRenderBins; i++ )
      {
         if( mRenderBins[i] )
         {
            mRenderBins[i]->clear();
         }
      }
   }
}

//-----------------------------------------------------------------------------
// initialize warning material instance
//-----------------------------------------------------------------------------
void RenderInstManager::initWarnMat()
{
   Material *warnMat = static_cast<Material*>(Sim::findObject( "WarningMaterial" ) );
   if( !warnMat )
   {
      Con::errorf( "Can't find WarningMaterial" );
   }
   else
   {
      SceneGraphData sgData;
      GFXVertexPNTTBN *vertDef = NULL; // the variable itself is the parameter to the template function
      mWarningMat = new MatInstance( *warnMat );
      mWarningMat->init( sgData, (GFXVertexFlags)getGFXVertFlags( vertDef ) );
   }
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderInstManager::render()
{
   GFX->pushWorldMatrix();
   MatrixF proj = GFX->getProjectionMatrix();

   if( mRenderBins.size() )
   {
      for( U32 i=0; i<NumRenderBins; i++ )
      {
         if( mRenderBins[i] )
         {
            mRenderBins[i]->render();
         }
      }
   }

   GFX->popWorldMatrix();
   GFX->setProjectionMatrix( proj );
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
