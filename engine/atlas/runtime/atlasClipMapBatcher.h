#ifndef _ATLAS_RUNTIME_ATLASCLIPMAPBATCHER_H_
#define _ATLAS_RUNTIME_ATLASCLIPMAPBATCHER_H_

#include "atlas/runtime/atlasClipMap.h"
#include "atlas/resource/atlasGeomChunk.h"

class AtlasResourceGeomStub;
class SceneState;

/// Manage optimal rendering of a set of AtlasGeomChunks with a clipmap
/// applied.
class AtlasClipMapBatcher
{
    AtlasClipMap* mClipMap;
    SceneState* mState;

    struct RenderNote
    {
        U8 levelStart, levelEnd, levelCount, pad;
        F32 nearDist;
        F32 morph;
        AtlasGeomChunk* chunk;
    };

    FreeListChunker<RenderNote> mRenderNoteAlloc;

    /// List of chunks to render for each shader.
    Vector<RenderNote*> mRenderList[4];

    /// RenderNotes to be rendered with detail maps.
    ///
    /// Offsets to entries in mRenderList
    Vector<RenderNote*> mDetailList;

    /// RenderNotes to be rendered with fog.
    ///
    /// Offsets to entries in mRenderList
    Vector<RenderNote*> mFogList;

    static S32 cmpRenderNote(const void* a, const void* b);

    void renderClipMap();
    void renderDetail();
    void renderFog();

public:

    AtlasClipMapBatcher();
    ~AtlasClipMapBatcher()
    {

    }

    void init(AtlasClipMap* acm, SceneState* state);
    void queue(const Point3F& camPos, AtlasResourceGeomStub* args, F32 morph);
    void sort();
    void render();

    static bool smRenderDebugTextures;
};

#endif