//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSMESH_H_
#define _TSMESH_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STREAM_H_
#include "core/stream.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _ABSTRACTPOLYLIST_H_
#include "collision/abstractPolyList.h"
#endif

#include "sceneGraph/sceneState.h"
#include "gfx/gfxDevice.h"

typedef GFXVertexPNTTBN MeshVertex;

// when working with 3dsmax, we want some things to be vectors that otherwise
// are pointers to non-resizeable blocks of memory
#if defined(TORQUE_MAX_LIB)
#define ToolVector Vector
#else
template<class A> class ToolVector
{
public:
    A* addr;
    U32 sz;
    U32 size() const { return sz; }
    bool empty() const { return sz == 0; }
    A& operator[](U32 idx) { return addr[idx]; }
    A const& operator[](U32 idx) const { return addr[idx]; }
    A* address() { return addr; }
    void set(void* _addr, U32 _sz) { addr = (A*)_addr; sz = _sz; }
};
#endif

namespace Opcode { class Model; class MeshInterface; }
namespace IceMaths { class IndexedTriangle; class Point; }

class TSMaterialList;
class TSShapeInstance;
struct RayInfo;
class ConvexFeature;
class ShapeBase;

struct TSDrawPrimitive
{
    enum
    {
        Triangles = 0 << 30, ///< bits 30 and 31 index element type
        Strip = 1 << 30, ///< bits 30 and 31 index element type
        Fan = 2 << 30, ///< bits 30 and 31 index element type
        Indexed = BIT(29), ///< use glDrawElements if indexed, glDrawArrays o.w.
        NoMaterial = BIT(28), ///< set if no material (i.e., texture missing)
        MaterialMask = ~(Strip | Fan | Triangles | Indexed | NoMaterial),
        TypeMask = Strip | Fan | Triangles
    };

    U16 start;
    U16 numElements;
    S32 matIndex;    ///< holds material index & element type (see above enum)
};

class TSMesh
{
protected:
    U32 meshType;
    Box3F mBounds;
    Point3F mCenter;
    F32 mRadius;
    F32 mVisibility;
    bool mDynamic;

    static F32 overrideFadeVal;
    static MatrixF smCamTrans;
    static SceneState* smSceneState;
    static SceneObject* smObject;
    static GFXCubemap* smCubemap;
    static bool smGlowPass;
    static bool smRefractPass;


    GFXVertexBufferHandle<MeshVertex> mVB;
    GFXPrimitiveBufferHandle mPB;

public:

    enum
    {
        /// types...
        StandardMeshType = 0,
        SkinMeshType = 1,
        DecalMeshType = 2,
        SortedMeshType = 3,
        NullMeshType = 4,
        TypeMask = StandardMeshType | SkinMeshType | DecalMeshType | SortedMeshType | NullMeshType,

        /// flags (stored with meshType)...
        Billboard = BIT(31), HasDetailTexture = BIT(30),
        BillboardZAxis = BIT(29), UseEncodedNormals = BIT(28),
        FlagMask = Billboard | BillboardZAxis | HasDetailTexture | UseEncodedNormals
    };

    U32 getMeshType() { return meshType & TypeMask; }
    void setFlags(U32 flag) { meshType |= flag; }
    void clearFlags(U32 flag) { meshType &= ~flag; }
    U32 getFlags(U32 flag = 0xFFFFFFFF) { return meshType & flag; }

    const Point3F* getNormals(S32 firstVert);

    S32 parentMesh; ///< index into shapes mesh list
    S32 numFrames;
    S32 numMatFrames;
    S32 vertsPerFrame;

    Vector<Point3F> verts;
    Vector<Point3F> norms;
    Vector<Point2F> tverts;
    Vector<TSDrawPrimitive> primitives;
    Vector<U8> encodedNorms;
    Vector<U16> indices;
    Vector<U16> mergeIndices; ///< the last so many verts merge with these
                                  ///< verts to form the next detail level
                                  ///< NOT IMPLEMENTED YET

    Vector<Point3F> initialTangents;
    Vector<Point4F> tangents;

    /// billboard data
    Point3F billboardAxis;

    /// @name Convex Hull Data
    /// Convex hulls are convex (no angles >= 180º) meshes used for collision
    /// @{

    Vector<Point3F> planeNormals;
    Vector<F32>     planeConstants;
    Vector<U32>     planeMaterials;
    S32 planesPerFrame;
    S32 vbOffset;
    U32 mergeBufferStart;
    /// @}

    /// @name Render Methods
    /// @{

    virtual void fillVB(S32 vb, S32 frame, S32 matFrame, TSMaterialList* materials);
    virtual void morphVB(S32 vb, S32 morph, S32 frame, S32 matFrame, TSMaterialList* materials);
    virtual void render();
    virtual void renderVB(S32 frame, S32 matFrame, TSMaterialList* materials);
    virtual void render(S32 frame, S32 matFrame, TSMaterialList*);
    virtual void renderShadow(S32 frame, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList*);
    void renderEnvironmentMap(S32 frame, S32 matFrame, TSMaterialList*);
    void renderDetailMap(S32 frame, S32 matFrame, TSMaterialList*);
    void renderFog(S32 frame, TSMaterialList* materials);
    /// @}


    /// @name Hack methods
    /// @{
    static void setCamTrans(const MatrixF& trans) { smCamTrans = trans; }
    static void setSceneState(SceneState* state) { smSceneState = state; }
    static void setCubemap(GFXCubemap* cm) { smCubemap = cm; }
    static void setObject(SceneObject* obj) { smObject = obj; }
    static void setGlow(bool glow) { smGlowPass = glow; }
    static void setRefract(bool refract) { smRefractPass = refract; }

    static const MatrixF& getCamTrans() { return smCamTrans; }
    /// @}

    /// @name Material Methods
    /// @{

    static void initMaterials();
    static void resetMaterials();
    static void initEnvironmentMapMaterials();
    static void resetEnvironmentMapMaterials();
    static void initDetailMapMaterials();
    static void resetDetailMapMaterials();
    static void setMaterial(S32 matIndex, TSMaterialList*);
    void setFade(F32 fadeValue);
    void clearFade();
    static void setOverrideFade(F32 fadeValue) { overrideFadeVal = fadeValue; }
    static F32  getOverrideFade() { return overrideFadeVal; }
    /// @}

    /// @name Collision Methods
    /// @{

    virtual bool buildPolyList(S32 frame, AbstractPolyList* polyList, U32& surfaceKey);
    virtual bool getFeatures(S32 frame, const MatrixF&, const VectorF&, ConvexFeature*, U32& surfaceKey);
    virtual void support(S32 frame, const Point3F& v, F32* currMaxDP, Point3F* currSupport);
    virtual bool castRay(S32 frame, const Point3F& start, const Point3F& end, RayInfo* rayInfo);
    virtual bool buildConvexHull(); ///< returns false if not convex (still builds planes)
    bool addToHull(U32 idx0, U32 idx1, U32 idx2);
    /// @}

    /// @name Bounding Methods
    /// calculate and get bounding information
    /// @{

    void computeBounds();
    virtual void computeBounds(MatrixF& transform, Box3F& bounds, S32 frame = 0, Point3F* center = NULL, F32* radius = NULL);
    void computeBounds(Point3F*, S32 numVerts, MatrixF& transform, Box3F& bounds, Point3F* center, F32* radius);
    Box3F& getBounds() { return mBounds; }
    Point3F& getCenter() { return mCenter; }
    F32 getRadius() { return mRadius; }
    virtual S32 getNumPolys();

    U8 encodeNormal(const Point3F& normal);
    const Point3F& decodeNormal(U8 ncode);
    /// @}

    void saveMergeVerts();      ///< called by shapeinstance in setStatics
    void restoreMergeVerts();   ///< called by shapeinstance in clearStatics
    void saveMergeNormals();    ///< called by mesh at start of render (decals don't bother)
    void restoreMergeNormals(); ///< called by mesh at end of render

    /// persist methods...
    virtual void assemble(bool skip);
    static TSMesh* assembleMesh(U32 meshType, bool skip);
    virtual void disassemble();

    void createTangents(const Vector<Point3F>& _verts, const Vector<Point3F>& _norms);
    void findTangent(U32 index1,
        U32 index2,
        U32 index3,
        Point3F* tan0,
        Point3F* tan1,
        const Vector<Point3F>& _verts);

    // this function allows subclasses to override where there vertex and primitive buffers come from.
    // note that this function returns a reference, allowing the caller to modify the buffers.
    virtual GFXVertexBufferHandle<MeshVertex>& getVertexBuffer() { return mVB; };

    void createVBIB();
    void createTextureSpaceMatrix(MeshVertex* v0, MeshVertex* v1, MeshVertex* v2);
    void fillTextureSpaceInfo(MeshVertex* vertArray);

    /// on load...optionally convert primitives to other form
    static bool smUseTriangles;
    static bool smUseOneStrip;
    static S32  smMinStripSize;
    static bool smUseEncodedNormals;

    /// convert primitives on load...
    void convertToTris(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn,
        S32 numPrimIn, S32& numPrimOut, S32& numIndicesOut,
        S32* primitivesOut, S16* indicesOut);
    void convertToSingleStrip(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn,
        S32 numPrimIn, S32& numPrimOut, S32& numIndicesOut,
        S32* primitivesOut, S16* indicesOut);
    void leaveAsMultipleStrips(S16* primitiveDataIn, S32* primitiveMatIn, S16* indicesIn,
        S32 numPrimIn, S32& numPrimOut, S32& numIndicesOut,
        S32* primitivesOut, S16* indicesOut);

    /// methods used during assembly to share vertexand other info
    /// between meshes (and for skipping detail levels on load)
    S32* getSharedData32(S32 parentMesh, S32 size, S32** source, bool skip);
    S8* getSharedData8(S32 parentMesh, S32 size, S8** source, bool skip);

    /// @name Assembly Variables
    /// variables used during assembly (for skipping mesh detail levels
    /// on load and for sharing verts between meshes)
    /// @{

    static Vector<Point3F*> smVertsList;
    static Vector<Point3F*> smNormsList;
    static Vector<U8*>      smEncodedNormsList;
    static Vector<Point2F*> smTVertsList;
    static Vector<bool>     smDataCopied;

    static Vector<Point3F>  smSaveVerts;
    static Vector<Point3F>  smSaveNorms;
    static Vector<Point2F>  smSaveTVerts;

    static const Point3F smU8ToNormalTable[];
    /// @}


    TSMesh() : meshType(StandardMeshType) {
        VECTOR_SET_ASSOCIATION(planeNormals);
        VECTOR_SET_ASSOCIATION(planeConstants);
        VECTOR_SET_ASSOCIATION(planeMaterials);
        parentMesh = -1;
        //      mVB = NULL;
        //      mPB = NULL;
        mDynamic = false;
        mVisibility = 1.0f;
    }
    virtual ~TSMesh();

    Opcode::Model* mOptTree = NULL;
    Opcode::MeshInterface* mOpMeshInterface = NULL;
    IceMaths::IndexedTriangle* mOpTris = NULL;
    IceMaths::Point* mOpPoints = NULL;

    void prepOpcodeCollision();
    bool buildPolyListOpcode(const S32 od, AbstractPolyList* polyList, const Box3F& nodeBox, TSMaterialList* materials, U32& surfaceKey);
    bool castRayOpcode(const Point3F& start, const Point3F& end, RayInfo* rayInfo, TSMaterialList* materials);
};

inline const Point3F& TSMesh::decodeNormal(U8 ncode) { return smU8ToNormalTable[ncode]; }

class TSSkinMesh : public TSMesh
{
public:
    typedef TSMesh Parent;

    /// vectors that define the vertex, weight, bone tuples
    Vector<F32> weight;
    Vector<S32> boneIndex;
    Vector<S32> vertexIndex;

    /// vectors indexed by bone number
    Vector<S32> nodeIndex;
    Vector<MatrixF> initialTransforms;

    /// initial values of verts and normals
    /// these get transformed into initial bone space,
    /// from there into world space relative to current bone
    /// pos, and then weighted by bone weights...
    Vector<Point3F> initialVerts;
    Vector<Point3F> initialNorms;

    /// set verts and normals...
    void updateSkin();

    // overrides from TSMesh
    GFXVertexBufferHandle<MeshVertex>& getVertexBuffer();

    // render methods..
    void render(S32 frame, S32 matFrame, TSMaterialList*);
    void renderShadow(S32 frame, const MatrixF& mat, S32 dim, U32* bits, TSMaterialList*);

    // collision methods...
    bool buildPolyList(S32 frame, AbstractPolyList* polyList, U32& surfaceKey);
    bool castRay(S32 frame, const Point3F& start, const Point3F& end, RayInfo* rayInfo);
    bool buildConvexHull(); // does nothing, skins don't use this

    void computeBounds(MatrixF& transform, Box3F& bounds, S32 frame, Point3F* center, F32* radius);

    /// persist methods...
    void assemble(bool skip);
    void disassemble();

    /// variables used during assembly (for skipping mesh detail levels
    /// on load and for sharing verts between meshes)
    static Vector<MatrixF*> smInitTransformList;
    static Vector<S32*>     smVertexIndexList;
    static Vector<S32*>     smBoneIndexList;
    static Vector<F32*>     smWeightList;
    static Vector<S32*>     smNodeIndexList;

    TSSkinMesh()
    {
        meshType = SkinMeshType;
        mDynamic = true;
    }
};

#endif
