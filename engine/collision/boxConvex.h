//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BOXCONVEX_H_
#define _BOXCONVEX_H_

#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif


//----------------------------------------------------------------------------

class BoxConvex: public Convex
{
   Point3F getVertex(S32 v);
   void emitEdge(S32 v1,S32 v2,const MatrixF& mat,ConvexFeature* cf);
   void emitFace(S32 fi,const MatrixF& mat,ConvexFeature* cf);
public:
   //
   Point3F mCenter;
   VectorF mSize;

   BoxConvex() { mType = BoxConvexType; }
   void init(SceneObject* obj) { mObject = obj; }

   Point3F support(const VectorF& v) const;
   void getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf);
   void getPolyList(AbstractPolyList* list);
};


class OrthoBoxConvex: public BoxConvex
{
   typedef BoxConvex Parent;
   mutable MatrixF mOrthoMatrixCache;

 public:
   OrthoBoxConvex() { mOrthoMatrixCache.identity(); }

   virtual const MatrixF& getTransform() const;
};

#endif
