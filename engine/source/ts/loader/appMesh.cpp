//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/loader/appMesh.h"
#include "ts/loader/tsShapeLoader.h"

Vector<AppMaterial*> AppMesh::appMaterials;

AppMesh::AppMesh()
   : flags(0), numFrames(0), numMatFrames(0), vertsPerFrame(0)
{
}

AppMesh::~AppMesh()
{
   for (int iBone = 0; iBone < bones.size(); iBone++)
      delete bones[iBone];
   bones.clear();
}

TSMesh* AppMesh::constructTSMesh()
{
   TSMesh* tsmesh;
   if (isSkin())
   {
      TSSkinMesh* tsskin = new TSSkinMesh();
      tsmesh = tsskin;

      // Copy skin elements
      tsskin->weight = weight;
      tsskin->boneIndex = boneIndex;
      tsskin->vertexIndex = vertexIndex;
      tsskin->nodeIndex = nodeIndex;
      tsskin->initialTransforms = initialTransforms;
      tsskin->initialVerts = initialVerts;
      tsskin->initialNorms = initialNorms;
   }
   else
   {
      tsmesh = new TSMesh();
   }

   // Copy mesh elements
   tsmesh->verts = points;
   tsmesh->norms = normals;
   tsmesh->tverts = uvs;
   tsmesh->primitives = primitives;
   tsmesh->indices = indices;
  // tsmesh->colors = colors;
   //tsmesh->tverts2 = uv2s;

   // Finish initializing the shape
   tsmesh->setFlags(flags);
   tsmesh->computeBounds();
   tsmesh->numFrames = numFrames;
   tsmesh->numMatFrames = numMatFrames;
   tsmesh->vertsPerFrame = vertsPerFrame;
   tsmesh->createTangents(tsmesh->verts, tsmesh->norms);
   tsmesh->encodedNorms.set(NULL,0);

   return tsmesh;
}

bool AppMesh::isBillboard()
{
   return !dStrnicmp(getName(),"BB::",4) || !dStrnicmp(getName(),"BB_",3) || isBillboardZAxis();
}

bool AppMesh::isBillboardZAxis()
{
   return !dStrnicmp(getName(),"BBZ::",5) || !dStrnicmp(getName(),"BBZ_",4);
}
