//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"
#include "sceneGraph/sceneGraph.h"
#include "lightingSystem/blobShadow.h"
#include "game/shapeBase.h"
#include "interior/interiorInstance.h"
#include "ts/tsMesh.h"
#include "sceneGraph/lightManager.h"

DepthSortList BlobShadow::smDepthSortList;
GFXTexHandle BlobShadow::smGenericShadowTexture = NULL;
#if defined(MB_ULTRA) && !defined(MB_ONLINE_SHADOW)
S32 BlobShadow::smGenericShadowDim = 256;
#else
S32 BlobShadow::smGenericShadowDim = 32;
#endif
U32 BlobShadow::smShadowMask = TerrainObjectType | InteriorObjectType;
#ifdef MB_ULTRA
F32 BlobShadow::smGenericRadiusSkew = 0.6f; // shrink radius of shape when it always uses generic shadow...
#else
F32 BlobShadow::smGenericRadiusSkew = 0.4f; // shrink radius of shape when it always uses generic shadow...
#endif

Box3F gBlobShadowBox;
SphereF gBlobShadowSphere;
Point3F gBlobShadowPoly[4];

//--------------------------------------------------------------

BlobShadow::BlobShadow(SceneObject* parentObject, LightInfo* light, TSShapeInstance* shapeInstance)
{
   mParentObject = parentObject;
   mShapeBase = dynamic_cast<ShapeBase*>(parentObject);
   mParentLight = light;
   mShapeInstance = shapeInstance;
   mRadius = 0.0f;
   mLastRenderTime = 0;

   generateGenericShadowBitmap(smGenericShadowDim);
}

BlobShadow::~BlobShadow()
{
   mShadowBuffer = NULL;
}

bool BlobShadow::shouldRender(F32 camDist)
{
   Point3F lightDir;

   F32 shadowLen = mShapeInstance->getShape()->radius;
   Point3F pos = mShapeInstance->getShape()->center;

   Point3F scale = mParentObject->getScale();
#ifdef MB_ULTRA
   shadowLen *= 200.0f;
#else
   shadowLen *= 10.0f;
#endif
   // this is a bit of a hack...move generic shadows towards feet/base of shape
   pos *= 0.5f;
   pos.convolve(scale);
   mParentObject->getRenderTransform().mulP(pos);
   if (mParentLight->mType == LightInfo::Vector)
   {
       lightDir = mParentLight->mDirection;
#ifdef MB_ULTRA
       mParentObject->getShadowLightVectorHack(lightDir);
#endif
   }
   else
   {
       lightDir = pos - mParentLight->mPos;
       lightDir.normalize();
   }

   // pos is where shadow will be centered (in world space)
   setRadius(mShapeInstance, scale);
   bool render = prepare(pos, lightDir, shadowLen);
   return render;
}

void BlobShadow::generateGenericShadowBitmap(S32 dim)
{
   if(smGenericShadowTexture)
      return;
   GBitmap * bitmap = new GBitmap(dim,dim,false,GFXFormatR8G8B8A8);
   U8 * bits = bitmap->getWritableBits();
   dMemset(bits, 0, dim*dim*4);
   S32 center = dim >> 1;
   F32 invRadiusSq = 1.0f / (F32)(center*center);
   F32 tmpF;
   for (S32 i=0; i<dim; i++)
   {
      for (S32 j=0; j<dim; j++)
      {
         tmpF = (F32)((i-center)*(i-center)+(j-center)*(j-center)) * invRadiusSq;
#if defined(MB_ULTRA) && !defined(MB_ONLINE_SHADOW)
         U8 val = tmpF>0.99f ? 0 : (U8)(75.0f*(1.0f/*-tmpF*/)); // 180 out of 255 max
#else
         U8 val = tmpF > 0.99f ? 0 : (U8)(180.0f * (1.0f-tmpF)); // 180 out of 255 max
#endif
         bits[(i*dim*4)+(j*4)+3] = val;
      }
   }
   smGenericShadowTexture = GFX->getTextureManager()->createTexture(bitmap, &GFXDefaultStaticDiffuseProfile, false);
}

//--------------------------------------------------------------

void BlobShadow::setLightMatrices(const Point3F & lightDir, const Point3F & pos)
{
   AssertFatal(mDot(lightDir,lightDir)>0.0001f,"BlobShadow::setLightDir: light direction must be a non-zero vector.");

   // construct light matrix
   Point3F x,z;
   if (mFabs(lightDir.z)>0.001f)
   {
      // mCross(Point3F(1,0,0),lightDir,&z);
      z.x = 0.0f;
      z.y =  lightDir.z;
      z.z = -lightDir.y;
      z.normalize();
      mCross(lightDir,z,&x);
   }
   else
   {
      mCross(lightDir,Point3F(0,0,1),&x);
      x.normalize();
      mCross(x,lightDir,&z);
   }

   mLightToWorld.identity();
   mLightToWorld.setColumn(0,x);
   mLightToWorld.setColumn(1,lightDir);
   mLightToWorld.setColumn(2,z);
   mLightToWorld.setColumn(3,pos);

   mWorldToLight = mLightToWorld;
   mWorldToLight.inverse();
}

void BlobShadow::setRadius(F32 radius)
{
   mRadius = radius;
}

void BlobShadow::setRadius(TSShapeInstance * shapeInstance, const Point3F & scale)
{
   const Box3F & bounds = shapeInstance->getShape()->bounds;
   F32 dx = 0.5f * (bounds.max.x-bounds.min.x) * scale.x;
   F32 dy = 0.5f * (bounds.max.y-bounds.min.y) * scale.y;
   F32 dz = 0.5f * (bounds.max.z-bounds.min.z) * scale.z;
   mRadius = mSqrt(dx*dx+dy*dy+dz*dz);
}


//--------------------------------------------------------------

bool BlobShadow::prepare(const Point3F & pos, Point3F lightDir, F32 shadowLen)
{
   if (mPartition.empty())
   {
      // --------------------------------------
      // 1.
#ifndef MB_ULTRA
      F32 dirMult = (1.0f) * (1.0f);
      if (dirMult < 0.99f)
      {
         lightDir.z *= dirMult;
         lightDir.z -= 1.0f - dirMult;
      }
      lightDir.normalize();
      shadowLen *= (1.0f) * (1.0f);
#else
       m_point3F_normalize(lightDir);
#endif

      // --------------------------------------
      // 2. get polys
      F32 radius = mRadius;
      radius *= smGenericRadiusSkew;
      buildPartition(pos,lightDir,radius,shadowLen);
   }
   updatePartition();
   if (mPartition.empty())
      // no need to draw shadow if nothing to cast it onto
      return false;


   return true;
}

//--------------------------------------------------------------

void BlobShadow::buildPartition(const Point3F & p, const Point3F & lightDir, F32 radius, F32 shadowLen)
{
   setLightMatrices(lightDir,p);

   Point3F extent(2.0f*radius,shadowLen,2.0f*radius);
   smDepthSortList.clear();
   smDepthSortList.set(mWorldToLight,extent);
   smDepthSortList.setInterestNormal(lightDir);

   if (shadowLen<1.0f)
      // no point in even this short of a shadow...
      shadowLen = 1.0f;
   mInvShadowDistance = 1.0f / shadowLen;

   // build world space box and sphere around shadow

   Point3F x,y,z;
   mLightToWorld.getColumn(0,&x);
   mLightToWorld.getColumn(1,&y);
   mLightToWorld.getColumn(2,&z);
   x *= radius;
   y *= shadowLen;
   z *= radius;
   gBlobShadowBox.max.set(mFabs(x.x)+mFabs(y.x)+mFabs(z.x),
      mFabs(x.y)+mFabs(y.y)+mFabs(z.y),
      mFabs(x.z)+mFabs(y.z)+mFabs(z.z));
   y *= 0.5f;
   gBlobShadowSphere.radius = gBlobShadowBox.max.len();
   gBlobShadowSphere.center = p + y;
   gBlobShadowBox.min  = y + p - gBlobShadowBox.max;
   gBlobShadowBox.max += y + p;

   // get polys

#ifdef MB_ULTRA_PREVIEWS
   getCurrentClientContainer()->findObjects(smShadowMask,BlobShadow::collisionCallback,this);
#else
   gClientContainer.findObjects(smShadowMask, BlobShadow::collisionCallback, this);
#endif

   // setup partition list
   gBlobShadowPoly[0].set(-radius,0,-radius);
   gBlobShadowPoly[1].set(-radius,0, radius);
   gBlobShadowPoly[2].set( radius,0, radius);
   gBlobShadowPoly[3].set( radius,0,-radius);

   mPartition.clear();
   mPartitionVerts.clear();
   smDepthSortList.depthPartition(gBlobShadowPoly,4,mPartition,mPartitionVerts);

   if(mPartitionVerts.empty())
      return;
   // now set up tverts & colors
   mShadowBuffer.set(GFX, mPartitionVerts.size(), GFXBufferTypeVolatile);
   mShadowBuffer.lock();

   //F32 visibleAlpha = 255;
   //if (mShapeBase && mShapeBase->getFadeVal())
   //   visibleAlpha = mClampF(255.0f * mShapeBase->getFadeVal(), 0, 255);
   F32 invRadius = 1.0f / radius;
   for (S32 i=0; i<mPartitionVerts.size(); i++)
   {
      Point3F vert = mPartitionVerts[i];
      mShadowBuffer[i].point.set(vert);
      //mShadowBuffer[i].color.set(255, 255, 255, visibleAlpha);
      mShadowBuffer[i].texCoord.set(0.5f + 0.5f * mPartitionVerts[i].x * invRadius, 0.5f + 0.5f * mPartitionVerts[i].z * invRadius);
   };

   mShadowBuffer.unlock();
}

void BlobShadow::updatePartition()
{
   // CodeReview [bjg 6/4/07] What's this supposed to do? Fade out shadows
   // by distance? Hopefully superceded by a shader...
//    for (S32 i=0; i<mPartitionVerts.size(); i++)
//    {
//       F32 val = 1.0f - mPartitionVerts[i].y * mInvShadowDistance;
//    }
}

//--------------------------------------------------------------

void BlobShadow::collisionCallback(SceneObject * obj, void* thisPtr)
{
   if (!obj->isHidden() && obj->getWorldBox().isOverlapped(gBlobShadowBox))
   {
      // only interiors clip...
      ClippedPolyList::allowClipping = (dynamic_cast<InteriorInstance*>(obj) != NULL);
      obj->buildRenderPolyList(&smDepthSortList,gBlobShadowBox,gBlobShadowSphere);
      ClippedPolyList::allowClipping = true;
   }
}

//--------------------------------------------------------------

void BlobShadow::render(F32 camDist)
{
   mLastRenderTime = Platform::getRealMilliseconds();
   GFX->pushWorldMatrix();
   MatrixF world = GFX->getWorldMatrix();
   world.mul(mLightToWorld);
   GFX->setWorldMatrix(world);

   F32 left;
   F32 right;
   F32 bottom;
   F32 top;
   F32 near;
   F32 far;
   GFX->getFrustum(&left, &right, &bottom, &top, &near, &far);
   GFX->setFrustum(left, right, bottom, top, near, far);

   F32 depthbias = -0.0002f;
   GFX->setZBias(*((U32 *)&depthbias));

   bool zwrite = GFX->getRenderState(GFXRSZWriteEnable);

   GFX->disableShaders();
   GFX->setCullMode(GFXCullNone);
   GFX->setLightingEnable(false);

   GFX->setAlphaBlendEnable(true);
   
   GFX->setZEnable(true);
   GFX->setZWriteEnable(false);
   
   GFX->setSrcBlend(GFXBlendSrcAlpha);
   GFX->setDestBlend(GFXBlendInvSrcAlpha);

   GFX->setAlphaTestEnable(true);
   GFX->setAlphaFunc(GFXCmpGreater);
   GFX->setAlphaRef(0);

   GFX->setTexture(0, smGenericShadowTexture);
   GFX->setTextureStageColorOp(0, GFXTOPModulate);      

   GFX->setVertexBuffer(mShadowBuffer);

   //GFX->setupGenericShaders( GFXDevice::GSModColorTexture );

   for(U32 p=0; p<mPartition.size(); p++)
      GFX->drawPrimitive(GFXTriangleFan, mPartition[p].vertexStart, (mPartition[p].vertexCount - 2));

   // This is a bad nasty hack which forces the shadow to reconstruct itself every frame.
   mPartition.clear();

   GFX->setZWriteEnable(zwrite);
   GFX->setVertexBuffer(NULL);
   GFX->setTexture(0, NULL);

   GFX->setZBias(0);
   GFX->popWorldMatrix();
   GFX->setBaseRenderState();
}

void BlobShadow::deleteGenericShadowBitmap()
{
   smGenericShadowTexture = NULL;
}
