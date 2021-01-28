//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShapeInstance.h"
#include "sim/sceneObject.h"

//-------------------------------------------------------------------------------------
// Collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::buildPolyList(AbstractPolyList * polyList, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::buildPolyList");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // set up static data
   setStatics(dl);

   // nothing emitted yet...
   bool emitted = false;
   U32 surfaceKey = 0;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF initialMat;
      Point3F initialScale;
      polyList->getTransform(&initialMat,&initialScale);

      // set up for first object's node
      MatrixF mat;
      MatrixF scaleMat(true);
      F32* p = scaleMat;
      p[0]  = initialScale.x;
      p[5]  = initialScale.y;
      p[10] = initialScale.z;
      MatrixF * previousMat = mMeshObjects[start].getTransform();
      mat.mul(initialMat,scaleMat);
      mat.mul(*previousMat);
      polyList->setTransform(&mat,Point3F(1, 1, 1));

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (mesh->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = mesh->getTransform();

            if (previousMat != NULL)
            {
               mat.mul(initialMat,scaleMat);
               mat.mul(*previousMat);
               polyList->setTransform(&mat,Point3F(1, 1, 1));
            }
         }
         // collide...
         emitted |= mesh->buildPolyList(od,polyList,surfaceKey);
      }

      // restore original transform...
      polyList->setTransform(&initialMat,initialScale);
   }

   clearStatics();

   return emitted;
}

bool TSShapeInstance::getFeatures(const MatrixF& mat, const Point3F& n, ConvexFeature* cf, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::buildPolyList");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // set up static data
   setStatics(dl);

   // nothing emitted yet...
   bool emitted = false;
   U32 surfaceKey = 0;

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF final;
      MatrixF* previousMat = mMeshObjects[start].getTransform();
      final.mul(mat, *previousMat);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (mesh->getTransform() != previousMat)
         {
            previousMat = mesh->getTransform();
            final.mul(mat, *previousMat);
         }
         emitted |= mesh->getFeatures(od, final, n, cf, surfaceKey);
      }
   }

   clearStatics();

   return emitted;
}

bool TSShapeInstance::castRay(const Point3F & a, const Point3F & b, RayInfo * rayInfo, S32 dl)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return false;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::buildPolyList");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // set up static data
   setStatics(dl);

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   RayInfo saveRay;
   saveRay.t = 1.0f;
   const MatrixF * saveMat = NULL;
   bool found = false;
   if (start<end)
   {
      Point3F ta, tb;

      // set up for first object's node
      MatrixF mat;
      MatrixF * previousMat = mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();
      mat.mulP(a,&ta);
      mat.mulP(b,&tb);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         if (mesh->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = mesh->getTransform();

            if (previousMat != NULL)
            {
               mat = *previousMat;
               mat.inverse();
               mat.mulP(a,&ta);
               mat.mulP(b,&tb);
            }
         }
         // collide...
         if (mesh->castRay(od,ta,tb,rayInfo))
         {
            if (!rayInfo)
            {
               clearStatics();
               return true;
            }
            if (rayInfo->t <= saveRay.t)
            {
               saveRay = *rayInfo;
               saveMat = previousMat;
            }
            found = true;
         }
      }
   }

   // collide with any skins for this detail level...
   // TODO: if ever...

   // finalize the deal...
   if (found)
   {
      *rayInfo = saveRay;
      saveMat->mulV(rayInfo->normal);
      rayInfo->point  = b-a;
      rayInfo->point *= rayInfo->t;
      rayInfo->point += a;
   }
   clearStatics();
   return found;
}

Point3F TSShapeInstance::support(const Point3F & v, S32 dl)
{
   // if dl==-1, nothing to do
   AssertFatal(dl != -1, "Error, should never try to collide with a non-existant detail level!");
   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::support");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // set up static data
   setStatics(dl);

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;
   const MatrixF * saveMat = NULL;
   bool found = false;

   F32     currMaxDP   = -1e9f;
   Point3F currSupport = Point3F(0, 0, 0);
   MatrixF * previousMat = NULL;
   MatrixF mat;
   if (start<end)
   {
      Point3F va;

      // set up for first object's node
      previousMat = mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         MeshObjectInstance * mesh = &mMeshObjects[i];

         if (od >= mesh->object->numMeshes)
            continue;

         TSMesh* physMesh = mesh->getMesh(od);
         if (physMesh && mesh->visible > 0.01f)
         {
            // collide...
            F32 saveMDP = currMaxDP;

            if (mesh->getTransform() != previousMat)
            {
               // different node from before, set up for this node
               previousMat = mesh->getTransform();
               mat = *previousMat;
               mat.inverse();
            }
            mat.mulV(v, &va);
            physMesh->support(mesh->frame, va, &currMaxDP, &currSupport);
         }
      }
   }

   clearStatics();

   if (currMaxDP != -1e9f)
   {
      previousMat->mulP(currSupport);
      return currSupport;
   }
   else
   {
      return Point3F(0, 0, 0);
   }
}

void TSShapeInstance::computeBounds(S32 dl, Box3F & bounds)
{
   // if dl==-1, nothing to do
   if (dl==-1)
      return;

   AssertFatal(dl>=0 && dl<mShape->details.size(),"TSShapeInstance::computeBounds");

   // get subshape and object detail
   const TSDetail * detail = &mShape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // set up static data
   setStatics(dl);

   S32 start = mShape->subShapeFirstObject[ss];
   S32 end   = mShape->subShapeNumObjects[ss] + start;

   // run through objects and updating bounds as we go
   bounds.min.set( 10E30f, 10E30f, 10E30f);
   bounds.max.set(-10E30f,-10E30f,-10E30f);
   Box3F box;
   for (S32 i=start; i<end; i++)
   {
      MeshObjectInstance * mesh = &mMeshObjects[i];

      if (od >= mesh->object->numMeshes)
         continue;

      if (mesh->getMesh(od))
      {
         mesh->getMesh(od)->computeBounds(*mesh->getTransform(),box);
         bounds.min.setMin(box.min);
         bounds.max.setMax(box.max);
      }
   }

   clearStatics();
}

//-------------------------------------------------------------------------------------
// Object (MeshObjectInstance & PluginObjectInstance) collision methods
//-------------------------------------------------------------------------------------

bool TSShapeInstance::ObjectInstance::buildPolyList(S32 objectDetail, AbstractPolyList * polyList, U32 & surfaceKey)
{
   objectDetail,polyList,surfaceKey;
   AssertFatal(0,"TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
   return false;
}

bool TSShapeInstance::ObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
   objectDetail,mat, n, cf, surfaceKey;
   AssertFatal(0,"TSShapeInstance::ObjectInstance::buildPolyList:  no default method.");
   return false;
}

void TSShapeInstance::ObjectInstance::support(S32, const Point3F&, F32*, Point3F*)
{
   AssertFatal(0,"TSShapeInstance::ObjectInstance::supprt:  no default method.");
}

bool TSShapeInstance::ObjectInstance::castRay(S32 objectDetail, const Point3F & start, const Point3F & end, RayInfo * rayInfo)
{
   objectDetail,start,end,rayInfo;
   AssertFatal(0,"TSShapeInstance::ObjectInstance::castRay:  no default method.");
   return false;
}

bool TSShapeInstance::MeshObjectInstance::buildPolyList(S32 objectDetail, AbstractPolyList * polyList, U32 & surfaceKey)
{
   TSMesh * mesh = getMesh(objectDetail);
   if (mesh && visible>0.01f)
      return mesh->buildPolyList(frame,polyList,surfaceKey);
   return false;
}

bool TSShapeInstance::MeshObjectInstance::getFeatures(S32 objectDetail, const MatrixF& mat, const Point3F& n, ConvexFeature* cf, U32& surfaceKey)
{
   TSMesh* mesh = getMesh(objectDetail);
   if (mesh && visible > 0.01f)
      return mesh->getFeatures(frame, mat, n, cf, surfaceKey);
   return false;
}

void TSShapeInstance::MeshObjectInstance::support(S32 objectDetail, const Point3F& v, F32* currMaxDP, Point3F* currSupport)
{
   TSMesh* mesh = getMesh(objectDetail);
   if (mesh && visible > 0.01f)
      mesh->support(frame, v, currMaxDP, currSupport);
}


bool TSShapeInstance::MeshObjectInstance::castRay(S32 objectDetail, const Point3F & start, const Point3F & end, RayInfo * rayInfo)
{
   TSMesh * mesh = getMesh(objectDetail);
   if (mesh && visible>0.01f)
      return mesh->castRay(frame,start,end,rayInfo);
   return false;
}
