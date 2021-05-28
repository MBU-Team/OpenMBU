//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASINSTANCE2_H_
#define _ATLASINSTANCE2_H_

#include "platform/platformThread.h"
#include "platform/platformMutex.h"
#include "core/stream.h"
#include "core/resManager.h"
#include "util/safeDelete.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxStructs.h"
#include "sim/sceneObject.h"
#include "materials/matInstance.h"
#include "atlas/core/atlasFile.h"
#include "atlas/runtime/atlasInstanceGeomTOC.h"
#include "atlas/runtime/atlasInstanceTexTOC.h"
#include "atlas/runtime/atlasClipMapBatcher.h"

class AtlasClipMap;

/// @defgroup AtlasRuntime Atlas Runtime
///
/// Functionality that relates to rendering and interacting with an Atlas
/// terrain in a running game instance.
///
/// @ingroup Atlas

/// An instance of an Atlas terrain.
///
/// Provides glue code to bring an Atlas terrain into the game world!
///
/// @ingroup AtlasRuntime
class AtlasInstance2 : public SceneObject
{
    typedef SceneObject Parent;

    Resource<AtlasFile> mAtlasFile;
    AtlasInstanceGeomTOC* mGeomTOC;

    GFXTexHandle mDetailTex;

    StringTableEntry mDetailTexFileName;
    StringTableEntry mAtlasFileName;

    AtlasClipMap* mClipMap;
    AtlasClipMapBatcher mBatcher;

public:
    AtlasInstance2();
    ~AtlasInstance2();
    DECLARE_CONOBJECT(AtlasInstance2);

    AtlasInstanceGeomTOC* getGeomTOC()
    {
        return mGeomTOC;
    }

    bool lightMapDirty;
    S32 lightMapSize;
    // just until the TOC texture rendering is in...
    GFXTexHandle lightMapTex;
    void initLightMap()
    {
        // Not needed currently as we do no lighting...
        return;

        if (isClientObject() && ((lightMapTex.getWidth() != lightMapSize) || (lightMapTex.getHeight() != lightMapSize)))
        {
            GBitmap* bitmap = new GBitmap(lightMapSize, lightMapSize, false, GFXFormatR8G8B8A8);
            U8* bits = bitmap->getAddress(0, 0);
            for (U32 i = 0; i < (lightMapSize * lightMapSize); i++)
            {
                bits[0] = 127;
                bits[1] = 127;
                bits[2] = 127;
                bits[3] = 255;
                bits += 4;
            }
            lightMapTex = GFXTexHandle(bitmap, &GFXDefaultPersistentProfile, true);
        }
    }
    void findDeepestStubs(Vector<AtlasResourceGeomTOC::StubType*>& stubs);

    bool onAdd();
    void onRemove();
    virtual void inspectPostApply();

    static void consoleInit();
    static void initPersistFields();

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state, RenderInst* ri);

    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
    void buildConvex(const Box3F& box, Convex* convex);
    bool buildPolyList(AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere);

    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    void refillClipMap();

    enum RayCollisionDebugLevel
    {
        RayCollisionDebugToTriangles = 0,
        RayCollisionDebugToColTree = 1,
        RayCollisionDebugToChunkBox = 2,
        RayCollisionDebugToObjectBox = 3,
    };

    enum ChunkBoundsDebugMode
    {
        ChunkBoundsDebugNone = 0,
        ChunkBoundsDebugLODThreshold = 1,
        ChunkBoundsDebugLODHolistic = 2,
        ChunkBoundsDebugHeat = 3,
        ChunkBoundsDebugMorph = 4,
    };

    static S32  smRayCollisionDebugLevel;
    static S32  smRenderChunkBoundsDebugLevel;
    static bool smLockFrustrum;
    static bool smRenderWireframe;
};

#endif