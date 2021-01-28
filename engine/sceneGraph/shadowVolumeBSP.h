//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SHADOWVOLUMEBSP_H_
#define _SHADOWVOLUMEBSP_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _DATACHUNKER_H_
#include "core/dataChunker.h"
#endif
#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _INTERIOR_H_
#include "interior/interior.h"
#endif

/// Used to calculate shadows.
class ShadowVolumeBSP
{
   public:
      ShadowVolumeBSP();
      ~ShadowVolumeBSP();

      struct SVNode;
      struct SurfaceInfo
      {
         U32               mSurfaceIndex;
         U32               mPlaneIndex;
         Vector<U32>       mShadowed;
         SVNode *          mShadowVolume;
      };

      struct SVNode
      {
         enum Side
         {
            Front       = 0,
            Back        = 1,
            On          = 2,
            Split       = 3
         };

         SVNode *       mFront;
         SVNode *       mBack;
         U32            mPlaneIndex;
         U32            mShadowVolume;

         /// Used with shadowed interiors.
         SurfaceInfo *  mSurfaceInfo;
      };

      struct SVPoly
      {
         enum {
            MaxWinding  = 32
         };

         U32               mWindingCount;
         Point3F           mWinding[MaxWinding];

         PlaneF            mPlane;
         SVNode *          mTarget;
         U32               mShadowVolume;
         SVPoly *          mNext;
         SurfaceInfo *     mSurfaceInfo;
      };

      void insertPoly(SVNode **, SVPoly *);
      void insertPolyFront(SVNode **, SVPoly *);
      void insertPolyBack(SVNode **, SVPoly *);

      void splitPoly(SVPoly *, const PlaneF &, SVPoly **, SVPoly **);
      void insertShadowVolume(SVNode **, U32);
      void addUniqueVolume(SurfaceInfo *, U32);

      SVNode::Side whichSide(SVPoly *, const PlaneF &) const;

      //
      bool testPoint(SVNode *, const Point3F &);
      bool testPoly(SVNode *, SVPoly *);
      void addToPolyList(SVPoly **, SVPoly *) const;
      void clipPoly(SVNode *, SVPoly **, SVPoly *);
      void clipToSelf(SVNode *, SVPoly **, SVPoly *);
      F32 getPolySurfaceArea(SVPoly *) const;
      F32 getClippedSurfaceArea(SVNode *, SVPoly *);
      void movePolyList(SVPoly **, SVPoly *) const;
      F32 getLitSurfaceArea(SVPoly *, SurfaceInfo *);

      Vector<SurfaceInfo *>   mSurfaces;

      Chunker<SVNode>         mNodeChunker;
      Chunker<SVPoly>         mPolyChunker;

      SVNode * createNode();
      void recycleNode(SVNode *);

      SVPoly * createPoly();
      void recyclePoly(SVPoly *);

      U32 insertPlane(const PlaneF &);
      const PlaneF & getPlane(U32) const;

      //
      SVNode *          mSVRoot;
      Vector<SVNode*>   mShadowVolumes;
      SVNode * getShadowVolume(U32);

      Vector<PlaneF>    mPlanes;
      SVNode *          mNodeStore;
      SVPoly *          mPolyStore;

      // used to remove the last inserted interior from the tree
      Vector<SVNode*>   mParentNodes;
      SVNode *          mFirstInteriorNode;
      void removeLastInterior();

      /// @name  Access functions
      /// @{
      void insertPoly(SVPoly * poly) {insertPoly(&mSVRoot, poly);}
      bool testPoint(Point3F & pnt) {return(testPoint(mSVRoot, pnt));}
      bool testPoly(SVPoly * poly) {return(testPoly(mSVRoot, poly));}
      F32 getClippedSurfaceArea(SVPoly * poly) {return(getClippedSurfaceArea(mSVRoot, poly));}
      /// @}

      /// @name Helpers
      /// @{
      void buildPolyVolume(SVPoly *, LightInfo *);
      SVPoly * copyPoly(SVPoly *);
      /// @}
};

#endif
