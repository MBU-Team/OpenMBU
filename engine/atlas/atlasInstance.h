//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASLOD_H_
#define _ATLASLOD_H_

#include "platform/platformThread.h"
#include "platform/platformMutex.h"
#include "sim/sceneObject.h"
#include "core/stream.h"
#include "core/dataChunker.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStructs.h"
#include "materials/matInstance.h"
#include "atlas/atlasTQTFile.h"
#include "atlas/atlasInstance.h"
#include "util/safeDelete.h"
#include "util/frustrumCuller.h"

class RenderInst;
class AtlasResource;
class AtlasInstanceEntry;

class AtlasInstance : public SceneObject
{
    friend class AtlasInstanceEntry;
    typedef SceneObject Parent;

    MatInstance* mMaterial;
    Resource<AtlasResource> mAtlasResource;
    StringTableEntry        mChunkPath;
    StringTableEntry        mTQTPath;
    StringTableEntry        mMaterialName;
    StringTableEntry        mDetailName;
    GFXTexHandle            mDetailTex;

    void load();

    /// Process LOD based on the current scene state. Please don't call me
    /// for more than one viewpoint, lest things get ugly.
    void processLOD(SceneState* state, const MatrixF& renderTrans);

    /// Actually do rendering (done in filespace).
    void render(SceneGraphData* cgd);

    U16 computeLod(const Box3F& bounds, const Point3F& camPos);
    S32 computeTextureLod(const Box3F& bounds, const Point3F& camPos);

    // Local LOD info...
    F32 mError_LODMax;
    F32 mDistance_LODMax;
    F32 mTextureDistance_LODMax;

    AtlasInstanceEntry* mChunks;
public:

    static bool smShowChunkBounds;
    static bool smRenderWireframe;
    static bool smSynchronousLoad;
    static bool smLockCamera;
    static bool smCollideGeomBoxes;
    static bool smCollideChunkBoxes;
    static FrustrumCuller smCuller;

    void purge();

    AtlasInstance();
    ~AtlasInstance();

    DECLARE_CONOBJECT(AtlasInstance);
    static void consoleInit();
    static void initPersistFields();

    bool onAdd();
    void onRemove();

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState /* = false */);
    void renderObject(SceneState* state, RenderInst* ri);

    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    // Collision...
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
    void buildConvex(const Box3F& box, Convex* convex);
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);

    AtlasResource* getResource() { return mAtlasResource; }
};

#endif