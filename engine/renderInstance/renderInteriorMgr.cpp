//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "renderInteriorMgr.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"
#include "materials/matInstance.h"
#include "../../example/shaders/shdrConsts.h"


//**************************************************************************
// RenderInteriorMgr
//**************************************************************************

//-----------------------------------------------------------------------------
// setup scenegraph data
//-----------------------------------------------------------------------------
void RenderInteriorMgr::setupSGData( RenderInst *ri, SceneGraphData &data )
{
   dMemset( &data, 0, sizeof( SceneGraphData ) );

   // do this here for the init, but also in setupLights...
   dMemcpy(&data.light, &ri->light, sizeof(ri->light));
   dMemcpy(&data.lightSecondary, &ri->lightSecondary, sizeof(ri->lightSecondary));
   data.dynamicLight = ri->dynamicLight;
   data.dynamicLightSecondary = ri->dynamicLightSecondary;

   data.camPos = gRenderInstManager.getCamPos();
   data.objTrans = *ri->objXform;
   data.backBuffTex = *ri->backBuffTex;
   data.cubemap = ri->cubemap;

   data.useFog = true;
   data.fogTex = getCurrentClientSceneGraph()->getFogTexture();
   data.fogHeightOffset = getCurrentClientSceneGraph()->getFogHeightOffset();
   data.fogInvHeightRange = getCurrentClientSceneGraph()->getFogInvHeightRange();
   data.visDist = getCurrentClientSceneGraph()->getVisibleDistanceMod();

   if( ri->lightmap )
   {
      data.lightmap     = *ri->lightmap;
   }
   if( ri->normLightmap )
   {
      data.normLightmap = *ri->normLightmap;
   }

}

//-----------------------------------------------------------------------------
// addElement
//-----------------------------------------------------------------------------
void RenderInteriorMgr::addElement( RenderInst *inst )
{
   mElementList.increment();
   MainSortElem &elem = mElementList.last();
   elem.inst = inst;
   elem.key = elem.key2 = 0;

   // sort by material and matInst
   if( inst->matInst )
   {
      elem.key = (U32) inst->matInst;
   }

   // sort by vertex buffer
   if( inst->vertBuff )
   {
      elem.key2 = (U32) inst->vertBuff->getPointer();
   }

}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderInteriorMgr::render()
{
   GFX->pushWorldMatrix();


   SceneGraphData sgData;

   if(getCurrentClientSceneGraph()->isReflectPass() )
   {
      GFX->setCullMode( GFXCullCW );
   }
   else
   {
      GFX->setCullMode( GFXCullCCW );
   }

   for( U32 i=0; i<TEXTURE_STAGE_COUNT; i++ )
   {
      GFX->setTextureStageAddressModeU( i, GFXAddressWrap );
      GFX->setTextureStageAddressModeV( i, GFXAddressWrap );

      GFX->setTextureStageMagFilter( i, GFXTextureFilterLinear );
      GFX->setTextureStageMinFilter( i, GFXTextureFilterLinear );
      GFX->setTextureStageMipFilter( i, GFXTextureFilterLinear );
   }

   // turn on anisotropic only on base tex stage
   //   GFX->setTextureStageMaxAnisotropy( 0, 2 );
   //   GFX->setTextureStageMagFilter( 0, GFXTextureFilterAnisotropic );
   //   GFX->setTextureStageMinFilter( 0, GFXTextureFilterAnisotropic );


   GFX->setZWriteEnable( true );
   GFX->setZEnable( true );


   U32 binSize = mElementList.size();


   GFXVertexBuffer * lastVB = NULL;
   GFXPrimitiveBuffer * lastPB = NULL;

   GFXTextureObject *lastLM = NULL;
   GFXTextureObject *lastLNM = NULL;

   U32 changeCount = 0, changeVB = 0, changePB = 0;

   for( U32 j=0; j<mElementList.size(); )
   {
      RenderInst *ri = mElementList[j].inst;

      setupSGData( ri, sgData );
      MatInstance *mat = ri->matInst;

      U32 matListEnd = j;


      while( mat->setupPass( sgData ) )
      {
         changeCount++;
         U32 a;
         for( a=j; a<binSize; a++ )
         {
            RenderInst *passRI = mElementList[a].inst;

            if( mat != passRI->matInst || 
                (a != j && passRI->reflective) )  // if reflective, we want to reset textures in case this piece of geometry uses different reflect texture
            {
               lastLM = NULL;  // pointer no longer valid after setupPass() call
               lastLNM = NULL;
               break;
            }

			   if(passRI->type == RenderInstManager::RIT_InteriorDynamicLighting)
			   {
				   // don't break the material multipass rendering...
				   GFX->setAlphaBlendEnable(true);
				   GFX->setSrcBlend(GFXBlendSrcAlpha);
				   GFX->setDestBlend(GFXBlendOne);

				   setupLights(passRI, sgData);
			   }

            // fill in shader constants that change per draw
            //-----------------------------------------------
            GFX->setVertexShaderConstF( 0, (float*)passRI->worldXform, 4 );

            // set object transform
            MatrixF objTrans = *passRI->objXform;
            objTrans.transpose();
            GFX->setVertexShaderConstF( VC_OBJ_TRANS, (float*)&objTrans, 4 );
            objTrans.transpose();
            objTrans.inverse();

            // fill in eye data
            Point3F eyePos = gRenderInstManager.getCamPos();
            objTrans.mulP( eyePos );
            GFX->setVertexShaderConstF( VC_EYE_POS, (float*)&eyePos, 1 );

            // fill in cubemap data
            if( mat->hasCubemap() )
            {
               Point3F cubeEyePos = gRenderInstManager.getCamPos() - passRI->objXform->getPosition();
               GFX->setVertexShaderConstF( VC_CUBE_EYE_POS, (float*)&cubeEyePos, 1 );

               MatrixF cubeTrans = *passRI->objXform;
               cubeTrans.setPosition( Point3F( 0.0, 0.0, 0.0 ) );
               cubeTrans.transpose();
               GFX->setVertexShaderConstF( VC_CUBE_TRANS, (float*)&cubeTrans, 3 );
            }

            // set buffers
			   if(passRI->primBuff)
			   {
				   if( lastVB != passRI->vertBuff->getPointer() )
				   {
					   changeVB++;
					   GFX->setVertexBuffer( *passRI->vertBuff );
					   lastVB = passRI->vertBuff->getPointer();
				   }
				   if( lastPB != passRI->primBuff->getPointer() )
				   {
					   changePB++;
					   GFX->setPrimitiveBuffer( *passRI->primBuff );
					   lastPB = passRI->primBuff->getPointer();
				   }
			   }

            // This section of code is dangerous, it overwrites the
            // lightmap values in sgData.  This could be a problem when multiple
            // render instances use the same multi-pass material.  When
            // the first pass is done, setupPass() is called again on
            // the material, but the lightmap data has been changed in
            // sgData to the lightmaps in the last renderInstance rendered.

            // This section sets the lightmap data for the current batch.
            // For the first iteration, it sets the same lightmap data,
            // however the redundancy will be caught by GFXDevice and not
            // actually sent to the card.  This is done for simplicity given
            // the possible condition mentioned above.  Better to set always
            // than to get bogged down into special case detection.
            //-------------------------------------
            bool dirty = false;

            // set the lightmaps if different
            if( passRI->lightmap && passRI->lightmap->getPointer() != lastLM )
            {
               sgData.lightmap = *passRI->lightmap;
               lastLM = passRI->lightmap->getPointer();
               dirty = true;
            }
            if( passRI->normLightmap && passRI->normLightmap->getPointer() != lastLNM )
            {
               sgData.normLightmap = *passRI->normLightmap;
               lastLNM = passRI->normLightmap->getPointer();
               dirty = true;
            }

            if(dirty && (passRI->type != RenderInstManager::RIT_InteriorDynamicLighting))
            {
               mat->setLightmaps( sgData );
            }
            //-------------------------------------
      

               // draw it
			   if(passRI->prim)
			   {
				   GFXPrimitive *prim = passRI->prim;
				   GFX->drawIndexedPrimitive(prim->type, prim->minIndex, prim->numVertices,
					   prim->startIndex, prim->numPrimitives);
   					
				   lastVB = NULL;
				   lastPB = NULL;
			   }
			   else
            {
               GFX->drawPrimitive( passRI->primBuffIndex );
            }

         }

         matListEnd = a;
      }

      // force increment if none happened, otherwise go to end of batch
      j = ( j == matListEnd ) ? j+1 : matListEnd;

   }

   GFX->popWorldMatrix();
}
