//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef SHAPEMIMIC_H_
#define SHAPEMIMIC_H_

#pragma pack(push,8)
#include <max.h>
#include <stdmat.h>
#include <decomp.h>
#pragma pack(pop)

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

struct ExporterSequenceData;

struct SkinMimic
{
   typedef Vector<F32> WeightList;

   INode * skinNode;
   TSSkinMesh * skinMesh;
   S32 detailSize;
   S32 skinNum;
   S32 meshNum;

   F32 multiResPercent;

   Vector<TSDrawPrimitive> faces;
   Vector<Point3> verts;
   Vector<Point3F> normals;
   Vector<Point3> tverts;
   Vector<U16> indices;
   Vector<U16> mergeIndices;
   Vector<U32> smoothingGroups;
   Vector<U32> vertId;

   Vector<INode *> bones;
   Vector<WeightList*> weights;

   ~SkinMimic() { for (S32 i=0;i<weights.size(); i++) delete weights[i]; }
};

struct IflMimic
{
   char * fileName;
   char * fullFileName;
   S32 materialSlot;
   ~IflMimic() { dFree(fileName); dFree(fullFileName); }
};

struct MeshMimic
{
   INode * pNode;
   TSMesh * tsMesh;
   SkinMimic * skinMimic;
   bool billboard; // i.e., face camera
   bool sortedObject;
   S32 numVerts; // number of unique vertices
   S32 meshNum;
   Vector<U32> smoothingGroups;
   Vector<U32> remap;
   Vector<U32> vertId;

   Matrix3 objectOffset; // NOTE: not valid till late in the game

   F32 multiResPercent;

   MeshMimic(INode * meshNode) { pNode = meshNode; skinMimic = NULL; tsMesh = NULL; }
};

struct MultiResMimic
{
   INode * pNode;
   INode * multiResNode;
   S32 totalVerts;
   // record off order of vertex merging (we'll figure out tverts later)
   Vector<Point3F> mergeFrom;
   Vector<Point3F> mergeTo;
   MultiResMimic(INode * pn, INode * mrn) { pNode = pn; multiResNode = mrn; }
};

struct ObjectMimic
{
   enum { MaxDetails=20 };
   struct Detail
   {
      S32 size;
      MeshMimic * mesh;
   };

   // my object has a name...I'm Jimmy.
   const char * name;
   const char * fullName; // name of object in tree

   // each object has several meshes attached
   S32 numDetails;
   Detail details[MaxDetails];

   // we'll check the object against this list in the end
   Vector<S32> * pValidDetails;
   INode * inTreeNode; // this is the node that sits in the shape's node hierrarchy

   // The next two items are used for sorting objects
   // objects are sorted by subTreeNum first, and then
   // priority (smallest to highest in both cases).
   S32 subtreeNum;
   U32 priority;

   // all verts in the meshes get multiplied by this matrix
   // before being added to ts shape
   Matrix3 objectOffset;

   // The max node this object hangs on (i.e., the one in the shape not
   // the loose objects that make up the detail levels).
   INode * maxParent;
   // Similar to above: the max node that corresponds to tsNode that will
   // be our parent.  Starts out the same as maxParent, but is revised
   // as we prune unwanted nodes from the tree structure.
   INode * maxTSParent;

   // ts node index we hang off
   S32 tsNodeIndex;
   S32 tsObjectIndex;

   // This is the eventual payoff
   TSObject * tsObject;

   //
   bool isBone;
   bool isSkin;

   ~ObjectMimic()
   {
      dFree((char*)name);
      dFree((char*)fullName);
      for (S32 i=0;i<numDetails;i++)
         delete details[i].mesh;
   }
};

struct DecalMeshMimic
{
   MeshMimic * targetMesh;
   TSDecalMesh * tsMesh;

   DecalMeshMimic(MeshMimic * target) { targetMesh = target; tsMesh = NULL; }
};

struct DecalObjectMimic
{
   enum { MaxDetails=20 };
   struct Detail
   {
      DecalMeshMimic * decalMesh;
   };

   // each object has several meshes attached
   S32 numDetails;
   Detail details[MaxDetails];

   // The next two items are used for sorting decal objects.
   // See ObjectMimic for more info.
   S32 subtreeNum;
   U32 priority;

   // The object used to map the decal onto the object
   INode * decalNode;

   // This is the object we decal
   ObjectMimic * targetObject;

   // This is the eventual payoff
   TSDecal * tsDecal;

   ~DecalObjectMimic()
   {
      for (S32 i=0;i<numDetails;i++)
         delete details[i].decalMesh;
   }
};

struct NodeMimic
{
   // our twin in the max world:
   INode * maxNode;

   // our neighbors in the mimic world:
   NodeMimic * parent;
   NodeMimic * child;
   NodeMimic * sibling;

   // transforms at default time
   AffineParts child0;
   AffineParts parent0;

   // index of our ts version
   S32 number;

   // our mimic object
   Vector<ObjectMimic*> objects;
};

struct CropInfo
{
   bool hasTVerts;
   Matrix3 uvTransform;

   bool uWrap;
   bool vWrap;

   bool crop;
   bool place;

   F32 uOffset;
   F32 vOffset;
   F32 uWidth;
   F32 vHeight;

   // only valid if crop==true
   bool cropLeft;
   bool cropRight;
   bool cropTop;
   bool cropBottom;

   bool twoSided;
};

class ShapeMimic
{
   friend class SceneEnumProc;

   struct Subtree
   {
      Vector<S32> validDetails;
      Vector<const char*> detailNames;
      Vector<INode*> detailNodes;
      NodeMimic start;
   };

   Vector<Subtree*> subtrees;
   Vector<ObjectMimic*> objectList;
   Vector<DecalObjectMimic*> decalObjectList;
   Vector<SkinMimic*> skins;
   Vector<MultiResMimic*> multiResList;
   Vector<IflMimic*> iflList;
   Vector<INode*> sequences;
   Vector<char*> materials;
   Vector<U32> materialFlags;
   Vector<U32> materialReflectionMaps;
   Vector<U32> materialBumpMaps;
   Vector<U32> materialDetailMaps;
   Vector<F32> materialDetailScales;
   Vector<F32> materialEmapAmounts;
   INode * boundsNode;

   // this gets filled in late in the game
   // it holds the nodes that actually go into the shape
   // in the order they appear in the shape
   Vector<NodeMimic*> nodes;

   // error control
   char * errorStr;
   void setExportError(const char * str);

   // called by generateShape
   void generateBounds(TSShape * pShape);
   void generateDetails(TSShape * pShape);
   void generateSubtrees(TSShape * pShape);
   void generateObjects(TSShape * pShape);
   void generateDecals(TSShape * pShape);
   void generateDefaultStates(TSShape * pShape);
   void generateIflMaterials(TSShape * pShape);
   void generateSequences(TSShape * pShape);
   void generateMaterialList(TSShape * pShape);
   void generateSkins(TSShape * pShape);
   void optimizeMeshes(TSShape * pShape);
   void convertSortObjects(TSShape * pShape);

   void generateObjectState(ObjectMimic*,S32 time,TSShape *, bool addFrame, bool addMatFrame);
   void generateDecalState(DecalObjectMimic*,S32 time,TSShape *, bool multipleFrames);
   void generateFrame(ObjectMimic*,S32 time, bool addFrame, bool addMatFrame);
   void generateFaces(INode * meshNode, Mesh & maxMesh, Matrix3 & objectOffset,
                      Vector<TSDrawPrimitive> & faces, Vector<Point3F> & normals,
                      Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                      Vector<U32> & smooth, Vector<U32> * vertId = NULL);
   S32 getMultiResVerts(INode * meshNode, F32 multiResPercent);
   void generateNodeTransform(NodeMimic*,S32 time,TSShape *, bool blend, S32 blendReferenceTime, Quat16 & rot, Point3F & trans, Quat16 & srot, Point3F & scale);

   void addNodeRotation(NodeMimic*,S32 time,TSShape *, bool blend, Quat16 & rot, bool defaultVal=false);
   void addNodeTranslation(NodeMimic*,S32 time,TSShape *, bool blend, Point3F & trans, bool defaultVal=false);
   void addNodeUniformScale(NodeMimic*,S32 time,TSShape *, bool blend, F32 scale);
   void addNodeAlignedScale(NodeMimic*,S32 time,TSShape *, bool blend, Point3F & scale);
   void addNodeArbitraryScale(NodeMimic*,S32 time,TSShape *, bool blend, Quat16 & srot, Point3F & scale);

   void computeNormals(Vector<TSDrawPrimitive> &, Vector<U16> & indices, Vector<Point3F> & verts, Vector<Point3F> & norms, Vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames, Vector<U16> * mergeIndices);
   void computeNormals(Vector<TSDrawPrimitive> &, Vector<U16> & indices, Vector<Point3> & verts, Vector<Point3F> & norms, Vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames);

   void generateNodeAnimation(TSShape * pShape, Interval & range, ExporterSequenceData &);
   void generateObjectAnimation(TSShape * pShape, Interval & range, ExporterSequenceData &);
   void generateDecalAnimation(TSShape * pShape, Interval & range, ExporterSequenceData &);
   void generateGroundAnimation(TSShape * pShape, Interval & range, ExporterSequenceData &);
   void generateFrameTriggers(TSShape *, Interval &, ExporterSequenceData &, IParamBlock *);
   void addTriggersInOrder(IKeyControl *, F32 duration, TSShape *, U32 & offTriggers);

   void setNodeMembership(TSShape *, Interval & range, ExporterSequenceData &, S32 &, S32 &, S32 &, S32 &, S32 &);
   S32 setObjectMembership(TSShape::Sequence &, Interval & range, ExporterSequenceData &);
   S32 setDecalMembership(TSShape::Sequence &, Interval & range, ExporterSequenceData &);
   S32 setIflMembership(TSShape::Sequence &, Interval & range, ExporterSequenceData &);

   S32 setRotationMembership(TSShape *, TSSequence &, ExporterSequenceData &, S32 numFrames);
   S32 setTranslationMembership(TSShape *, TSSequence &, ExporterSequenceData &, S32 numFrames);
   S32 setUniformScaleMembership(TSShape *, TSSequence &, ExporterSequenceData &, S32 numFrames);
   S32 setAlignedScaleMembership(TSShape *, TSSequence &, ExporterSequenceData &, S32 numFrames);
   S32 setArbitraryScaleMembership(TSShape *, TSSequence &, ExporterSequenceData &, S32 numFrames);

   bool animatesAlignedScale(ExporterSequenceData &, S32 numFrames);
   bool animatesArbitraryScale(ExporterSequenceData &, S32 numFrames);

   void setObjectPriorities(Vector<ObjectMimic*> &);
   void setDecalObjectPriorities(Vector<DecalObjectMimic*> &);

   bool testCutNodes(Interval & range, ExporterSequenceData &);

   // methods used to optimize meshes
   void collapseVertices(TSMesh*, Vector<U32> & smooth, Vector<U32> & remap, Vector<U32> * vertId = NULL);
   void shareVerts(ObjectMimic *, S32 dl, TSShape *);
   void shareVerts(SkinMimic *, Vector<U32> & originalRemap, TSShape *);
   void shareBones(SkinMimic *, TSShape *);
   bool vertexSame(Point3F &, Point3F &, Point2F &, Point2F &, U32 smooth1, U32 smooth2, U32 idx1, U32 idx2, Vector<U32> * vertId);
   void stripify(Vector<TSDrawPrimitive> &, Vector<U16> & indices);

   // add a name to the shape and return the index
   S32 addName(const char *, TSShape *);

   // adds a mesh node to the mesh list and returns the mesh it hangs off
   ObjectMimic * getObject(INode *, char * name, S32 size, S32 * detailNum, F32 multiResPercent = -1.0f, bool matchFullName = true, bool isBone=false, bool isSkin=false);
   ObjectMimic * addObject(INode *, Vector<S32> *pValidDetails);
   ObjectMimic * addObject(INode *, Vector<S32> *pValidDetails, bool multiRes, S32 multiResSize = -1, F32 multiResPercent = -1.0f);
   ObjectMimic * addBoneObject(INode * bone, S32 subtreeNum);
   MeshMimic   * addSkinObject(SkinMimic * skinMimic);
   void addDecalObject(INode *, ObjectMimic *);

   // functions for adding decals
   bool decalOn(DecalObjectMimic*,S32 time);
   void interpolateTVert(Point3F v0, Point3F v1, Point3F v2, Point3F planeNormal, Point3F & p, Point3 tv0, Point3 tv1, Point3 tv2, Point3 & tv);
   S32 generateDecalFrame(DecalObjectMimic*,S32 time);
   void findDecalTexGen(MatrixF &, Point4F &, Point4F &);
   S32  findLastDecalFrameNumber(DecalObjectMimic*);
   bool prepareDecal(INode * decalNode, S32 time);
   void  generateDecalFrame(DecalObjectMimic*, S32 time, S32 dl);
   bool getDecalTVert(U32 decalType, Point3 vert, Point3F normal, Point3 & tv);
   bool getPlaneDecalTVSpecial(Point3 & vert, const Point3F & normal, Point3 & tv);
   void findNonOrthogonalComponents(const Point3F & v1, const Point3F & v2, const Point3F & p, F32 & t1, F32 & t2);
   bool getDecalProjPoint(U32 decalType, Point3 vert, Point3F & normal, U32 decalFaceIdx, Point3F & p);
   bool checkDecalFilter(Point3 v0, Point3 v1, Point3 v2);
   bool checkDecalFace(Point3 tv0, Point3 tv1, Point3 tv2);

   // utility function for traversing tree in depth first order
   // returns NULL when we get back to the top
   NodeMimic * findNextNode(NodeMimic * cur);

   // utility function called by collapseTransform removes
   // the node from the mimic tree structure, taking care
   // to otherwise maintain the integrity of the shape tree
   void snip(NodeMimic *);

   // should we cut this node?
   bool cut(NodeMimic *);

   // is this node on the never animate list?
   bool neverAnimateNode(NodeMimic *);

   void splitFaceX(TSDrawPrimitive & face, TSDrawPrimitive & faceA, TSDrawPrimitive & faceB,
                   Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                   Vector<bool> & flipX, Vector<bool> & flipY,
                   F32 splitAt,
                   Vector<U32> & smooth, Vector<U32> * vertId);
   void splitFaceY(TSDrawPrimitive & face, TSDrawPrimitive & faceA, TSDrawPrimitive & faceB,
                   Vector<Point3> & verts, Vector<Point3> & tverts, Vector<U16> & indices,
                   Vector<bool> & flipX, Vector<bool> & flipY,
                   F32 splitAt,
                   Vector<U32> & smooth, Vector<U32> * vertId);
   void handleCropAndPlace(Point3 &, CropInfo &);

public:
   ShapeMimic();
   ~ShapeMimic();

   // check for errors
   bool isError();
   const char * getError();

   //
   void setGlobals(INode * pNode, S32 polyCount, F32 minVolPerPoly, F32 maxVolPerPoly);

   void recordMergeOrder(IParamBlock *, MultiResMimic *);
   void collapseEdge(Vector<Face> & faces, S32 fromA, S32 toB);
   void getMultiResData(INode * pNode, Vector<S32> & multiResSize, Vector<F32> & multiResPercent);
   void addMultiRes(INode * pNode, INode * multiResNode);
   MultiResMimic * getMultiRes(INode * pNode);
   void remapWeights(Vector<SkinMimic::WeightList*> &, INode * skinNode, INode * multiResNode);
   void copyWeightsToVerts(SkinMimic *);

   void fixupT2AutoDetail();
   void applyAutoDetail(Vector<S32> &, Vector<const char*> &, Vector<INode*> &);

   // find which vertices merge with which vertices
   void findMergeIndices(MeshMimic *, Matrix3 & objectOffset, const Vector<TSDrawPrimitive> &,
           Vector<Point3F> &, Vector<Point3F> &, Vector<Point2F> &,
           const Vector<U16> & indices, Vector<U16> & mergeIndices,
           Vector<U32> & smooth, Vector<U32> & vertId, S32 numChildVerts);
   // very specialized function used by above
   void addMergeTargetAndMerge(Vector<TSDrawPrimitive> & faces, S32 & faceNum, S32 & v,
           Vector<U16> & indices, Vector<U16> & mergeIndices,
           Vector<Point3F> & verts, Vector<Point2F> & tverts, Vector<Point3F> & norms,
           Vector<U32> & smooth, Vector<U32> & vertId,
           Vector<S32> & mergeOrder, Vector<S32> & mergeID, Vector<Point3F> & mergeTarget);
   // called in optimizeMeshes
   void fixupMergeIndices(TSShape *);
   // called after adding first frame to non-skin objects
   void generateMergeIndices(ObjectMimic*);
   void testMerge(TSShape *, ObjectMimic *, S32 dl, S32 numChildVerts);

   // parts added to mimic using these methods...order unimportant
   void addBounds(INode * pNode);
   void addSubtree(INode * pNode);
   void addNode(NodeMimic *,INode *, Vector<S32> &,bool);
   void addMesh(INode * pNode) { addObject(pNode,NULL); }
   void addSkin(INode * pNode);
   void addSkin(INode * pNode, S32 multiResSize, F32 multiResPercent);
   void addSequence(INode * pNode);
   S32 addMaterial(INode * pNode, S32 materialIndex, CropInfo *);
   S32 findMaterial(const char * name, const char * fullName, U32 flags, S32,S32,S32,F32 dscale = 1.0f,F32 emapAmount=1.0f); // finds, and adds if not there
   void appendMaterialList(const char * name, U32 flags, S32,S32,S32,F32 dscale = 1.0f,F32 emapAmount=1.0f); // adds to the end
   const char * getBaseTextureName(const char *);

   // can optionally get rid of nodes with no mesh
   void collapseTransforms();
   void clearCollapseTransforms();

   F32 findMaxDistance(TSMesh * loMesh, TSMesh * hiMesh, F32 & total, S32 & count);

   // the payoff...call after adding all of the above
   TSShape * generateShape();
   void initShape(TSShape*);
   void destroyShape(TSShape*);
};

#endif

