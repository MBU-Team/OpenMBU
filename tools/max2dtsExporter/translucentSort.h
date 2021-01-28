//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TRANS_SORT
#define _TRANS_SORT

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TSINTEGERSET_H_
#include "ts/tsIntegerSet.h"
#endif
#ifndef _TSSORTEDMESH_H_
#include "ts/tsSortedMesh.h"
#endif

struct TSDrawPrimitive;
class TSIntegerSet;

class TranslucentSort
{
   Vector<TSIntegerSet*> frontClusters;
   Vector<TSIntegerSet*> backClusters;
   Vector<S32> middleCluster;

   Point3F splitNormal;
   F32 splitK;

   S32 mNumBigFaces;
   S32 mMaxDepth;
   bool mZLayerUp;
   bool mZLayerDown;

   S32 currentDepth;

   TranslucentSort * frontSort;
   TranslucentSort * backSort;

   struct FaceInfo
   {
      bool used;
      S32 priority;
      S32 parentFace;
      S32 childFace1;
      S32 childFace2;
      S32 childFace3;
      Point3F normal;
      F32 k;
      TSIntegerSet isInFrontOfMe;
      TSIntegerSet isBehindMe;
      TSIntegerSet isCutByMe;
      TSIntegerSet isCoplanarWithMe;
   };
   Vector<FaceInfo*> faceInfoList;
   Vector<FaceInfo*> saveFaceInfoList;

   Vector<TSDrawPrimitive> & mFaces;
   Vector<U16> & mIndices;
   Vector<Point3F> & mVerts;
   Vector<Point3F> & mNorms;
   Vector<Point2F> & mTVerts;

   void initFaces();
   void initFaceInfo(TSDrawPrimitive & face, FaceInfo & faceInfo, bool setPriority = true);
   void setFaceInfo(TSDrawPrimitive & face, FaceInfo & faceInfo);
   void clearFaces(TSIntegerSet &);
   void saveFaceInfo();
   void restoreFaceInfo();
   void addFaces(TSIntegerSet *, Vector<TSDrawPrimitive> & faces, Vector<U16> & indices, bool continueLast = false);
   void addFaces(Vector<TSIntegerSet *> &, Vector<TSDrawPrimitive> & faces, Vector<U16> & indices, bool continueLast = false);
   void addOrderedFaces(Vector<S32> &, Vector<TSDrawPrimitive> &, Vector<U16> & indices, bool continueLast = false);
   void splitFace(S32 faceIndex, Point3F normal, F32 k);
   void splitFace2(S32 faceIndex, Point3F normal, F32 k);
   void sort();

   // routines for sorting faces when there is no perfect solution for all cases
   void copeSort(Vector<S32> &);
   void layerSort(Vector<S32> &, bool upFirst);

   // these are for debugging
   bool anyInFrontOfPlane(Point3F normal, F32 k);
   bool anyBehindPlane(Point3F normal, F32 k);

   //
   void generateClusters(Vector<TSSortedMesh::Cluster> & clusters, Vector<TSDrawPrimitive> & faces, Vector<U16> & indices, S32 retIndex = -1);

   TranslucentSort(TranslucentSort *);
   TranslucentSort(Vector<TSDrawPrimitive> & faces,
                   Vector<U16> & indices,
                   Vector<Point3F> & verts,
                   Vector<Point3F> & norms,
                   Vector<Point2F> & tverts,
                   S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown);

   ~TranslucentSort();

public:

   static void generateSortedMesh(TSSortedMesh * mesh, S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown);
};



#endif // _TRANS_SORT

