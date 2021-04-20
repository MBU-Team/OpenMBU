//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BLOBSHADOW_H_
#define _BLOBSHADOW_H_

#include "collision/depthSortList.h"
#include "sim/sceneObject.h"
#include "ts/tsShapeInstance.h"
#include "lightingSystem/shadowBase.h"

class ShapeBase;

class BlobShadow : public ShadowBase
{
   F32 mRadius;
   F32 mInvShadowDistance;
   MatrixF mLightToWorld;
   MatrixF mWorldToLight;

   Vector<DepthSortList::Poly> mPartition;
   Vector<Point3F> mPartitionVerts;
   GFXVertexBufferHandle<GFXVertexPT> mShadowBuffer;

   static U32 smShadowMask;

   static DepthSortList smDepthSortList;
   static GFXTexHandle smGenericShadowTexture;
   static F32 smGenericRadiusSkew;
   static S32 smGenericShadowDim;

   U32 mLastRenderTime;

   static void collisionCallback(SceneObject*,void *);

private:

   SceneObject* mParentObject;
   ShapeBase* mShapeBase;
   LightInfo* mParentLight;
   TSShapeInstance* mShapeInstance;
   void setLightMatrices(const Point3F & lightDir, const Point3F & pos);

   void buildPartition(const Point3F & p, const Point3F & lightDir, F32 radius, F32 shadowLen);
   void updatePartition();

public:

   BlobShadow(SceneObject* parentobject, LightInfo* light, TSShapeInstance* shapeinstance);
   ~BlobShadow();

   void setRadius(F32 radius);
   void setRadius(TSShapeInstance *, const Point3F & scale);

   bool prepare(const Point3F & pos, Point3F lightDir, F32 shadowLen);

   bool shouldRender(F32 camDist);
   void render(F32 camDist);
   U32 getLastRenderTime() { return mLastRenderTime; }

   static void generateGenericShadowBitmap(S32 dim);
   static void deleteGenericShadowBitmap();
};

#endif