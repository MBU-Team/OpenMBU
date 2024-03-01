//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#pragma once
#include "ts/loader/appMaterial.h"
#include "ts/loader/appSequence.h"
#include "core/tVector.h"
#ifndef _APPMESH_H_
#define _APPMESH_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

class AppNode;
class AppMaterial;

class AppMesh
{
public:
   // Mesh and skin elements
   Vector<Point3F> points;
   Vector<Point3F> normals;
   Vector<Point2F> uvs;
   Vector<Point2F> uv2s;
   Vector<ColorI> colors;
   Vector<TSDrawPrimitive> primitives;
   Vector<U16> indices;

   // Skin elements
   Vector<F32> weight;
   Vector<S32> boneIndex;
   Vector<S32> vertexIndex;
   Vector<S32> nodeIndex;
   Vector<MatrixF> initialTransforms;
   Vector<Point3F> initialVerts;
   Vector<Point3F> initialNorms;

   U32 flags;
   U32 vertsPerFrame;
   S32 numFrames;
   S32 numMatFrames;

   // Loader elements (can be discarded after loading)
   S32                           detailSize;
   MatrixF                       objectOffset;
   Vector<AppNode*>              bones;
   static Vector<AppMaterial*>   appMaterials;

public:
   AppMesh();
   virtual ~AppMesh();

   // Create a TSMesh object
   TSMesh* constructTSMesh();

   virtual const char * getName(bool allowFixed=true) = 0;

   virtual MatrixF getMeshTransform(F32 time) = 0;
   virtual F32 getVisValue(F32 time) = 0;

   virtual bool getFloat(const char* propName, F32& defaultVal) = 0;
   virtual bool   getInt(const char* propName, S32& defaultVal) = 0;
   virtual bool  getBool(const char* propName, bool& defaultVal) = 0;

   virtual bool animatesVis(const AppSequence* appSeq) { return false; }
   virtual bool animatesMatFrame(const AppSequence* appSeq) { return false; }
   virtual bool animatesFrame(const AppSequence* appSeq) { return false; }

   virtual bool isBillboard();
   virtual bool isBillboardZAxis();

   virtual bool isSkin() { return false; }
   virtual void lookupSkinData() = 0;

   virtual void lockMesh(F32 t, const MatrixF& objectOffset) { }
};

#endif // _APPMESH_H_
