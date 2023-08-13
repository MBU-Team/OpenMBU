//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#pragma once
#include "core/path.h"
#include "ts/loader/appMesh.h"
#include "ts/loader/appNode.h"
#include "ts/loader/appSequence.h"
#include "core/tVector.h"
#include "ts/tsShape.h"
#ifndef _TSSHAPE_LOADER_H_
#define _TSSHAPE_LOADER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#endif
#ifndef _APPSEQUENCE_H_
#endif

class AppNode;
class AppMesh;

class TSShapeLoader
{

public:
   enum eLoadPhases
   {
      Load_ReadFile = 0,
      Load_ParseFile,
      Load_EnumerateScene,
      Load_GenerateSubshapes,
      Load_GenerateObjects,
      Load_GenerateDefaultStates,
      Load_GenerateSkins,
      Load_GenerateMaterials,
      Load_GenerateSequences,
      Load_InitShape,
      NumLoadPhases,
      Load_Complete = NumLoadPhases
   };

   static void updateProgress(int major, const char* msg, int numMinor=0, int minor=0);

protected:
   struct Subshape
   {
      Vector<AppNode*>           branches;         ///< Shape branches
      Vector<AppMesh*>           objMeshes;        ///< Object meshes for this subshape
      Vector<S32>                objNodes;         ///< AppNode indices with objects attached

      ~Subshape()
      {
         // Delete children
         for (S32 i = 0; i < branches.size(); i++)
            delete branches[i];
      }
   };

public:
   static const F32 DefaultTime;
   static const double AppFrameRate;
   static const double AppGroundFrameRate;

protected:
   // Variables used during loading that must be held until the shape is deleted
   TSShape*                      shape;
   Vector<AppMesh*>              appMeshes;

   // Variables used during loading, but that can be discarded afterwards
   static Torque::Path           shapePath;

   AppNode*                      boundsNode;
   Vector<AppNode*>              appNodes;            ///< Nodes in the loaded shape
   Vector<AppSequence*>          appSequences;

   Vector<Subshape*>             subshapes;

   Vector<QuatF*>                nodeRotCache;
   Vector<Point3F*>              nodeTransCache;
   Vector<QuatF*>                nodeScaleRotCache;
   Vector<Point3F*>              nodeScaleCache;

   Point3F                       shapeOffset;         ///< Offset used to translate the shape origin

   //--------------------------------------------------------------------------

   // Collect the nodes, objects and sequences for the scene
   virtual void enumerateScene() = 0;
   bool processNode(AppNode* node);
   virtual bool ignore(const String& name) { return false; }

   void addSkin(AppMesh* mesh);
   void addDetailMesh(AppMesh* mesh);
   void addSubshape(AppNode* node);
   void addObject(AppMesh* mesh, S32 nodeIndex, S32 subShapeNum);

   String getUniqueName(const char* name);

   // Node transform methods
   MatrixF getLocalNodeMatrix(AppNode* node, F32 t);
   void generateNodeTransform(AppNode* node, F32 t, bool blend, F32 referenceTime,
                              QuatF& rot, Point3F& trans, QuatF& srot, Point3F& scale);

   void computeBounds(Box3F& bounds);
   virtual void computeShapeOffset() { shapeOffset = Point3F(0, 0, 0); }

   // Create objects, materials and sequences
   void recurseSubshape(AppNode* appNode, S32 parentIndex, bool recurseChildren);

   void generateSubshapes();
   void generateObjects();
   void generateSkins();
   void generateDefaultStates();
   void generateObjectState(TSShape::Object& obj, F32 t, bool addFrame, bool addMatFrame);
   void generateFrame(TSShape::Object& obj, F32 t, bool addFrame, bool addMatFrame);

   virtual void generateIflMaterials();
   void generateMaterialList();

   void generateSequences();

   // Determine what is actually animated in the sequence
   void setNodeMembership(TSShape::Sequence& seq, const AppSequence* appSeq);
   void setRotationMembership(TSShape::Sequence& seq);
   void setTranslationMembership(TSShape::Sequence& seq);
   void setScaleMembership(TSShape::Sequence& seq);
   void setObjectMembership(TSShape::Sequence& seq, const AppSequence* appSeq);
   void setIflMembership(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Manage a cache of all node transform elements for the sequence
   void clearNodeTransformCache();
   void fillNodeTransformCache(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Add node transform elements
   void addNodeRotation(QuatF& rot, bool defaultVal);
   void addNodeTranslation(Point3F& trans, bool defaultVal);
   void addNodeUniformScale(F32 scale);
   void addNodeAlignedScale(Point3F& scale);
   void addNodeArbitraryScale(QuatF& qrot, Point3F& scale);

   // Generate animation data
   void generateNodeAnimation(TSShape::Sequence& seq);
   void generateObjectAnimation(TSShape::Sequence& seq, const AppSequence* appSeq);
   void generateGroundAnimation(TSShape::Sequence& seq, const AppSequence* appSeq);
   void generateFrameTriggers(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Shape construction
   void sortDetails();
   void install();

public:
   TSShapeLoader() : boundsNode(0) { }
   virtual ~TSShapeLoader();

   static const Torque::Path& getShapePath() { return shapePath; }

   TSShape* generateShape(const Torque::Path& path);
};

#endif // _TSSHAPE_LOADER_H_
