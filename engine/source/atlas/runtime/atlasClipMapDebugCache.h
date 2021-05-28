//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLAS_RUNTIME_ATLASCLIPMAPDEBUGCACHE_H_
#define _ATLAS_RUNTIME_ATLASCLIPMAPDEBUGCACHE_H_

#include "atlas/runtime/atlasInstanceTexTOC.h"
#include "atlas/runtime/atlasClipMap.h"
#include "atlas/runtime/atlasClipMapCache.h"

/// A clipmap "cache" that produces a variety of debug patterns on the fly.
///
/// This is very useful for debugging the clipmap shaders as well as
/// visualizing various parts of the clipmap process (LOD selection,
/// cache centering behavior, etc.).
class AtlasClipMapImageCache_Debug : public IAtlasClipMapImageCache
{
    /// One of DebugModes.
    U32 mCurDebugMode;

    /// Render alternating 4px squares on top of the debug mode.
    bool mDoSquares;

public:
    enum DebugModes
    {
        /// Render flat colors based on the clipmaplevel.
        LOD,

        /// Render a UV gradient based on position in the clipmap level.
        UVGradient,

        /// Render a checkerboard pattern, sized so that it's 1px on the
        /// highest clipmap level.
        Checkers,
    };

    AtlasClipMapImageCache_Debug();
    virtual ~AtlasClipMapImageCache_Debug()
    {
    }

    void setDebugMode(DebugModes dm, bool doSquares)
    {
        mCurDebugMode = dm;
        mDoSquares = doSquares;
    }

    virtual void initialize(U32 clipMapSize, U32 clipMapDepth)
    {
    }

    virtual void setInterestCenter(Point2I origin)
    {
    }

    virtual bool isDataAvailable(U32 mipLevel, RectI region);

    virtual void beginRectUpdates(AtlasClipMap::ClipStackEntry& cse)
    {
    }

    /// This is the only place that does any real work - it copies the debug pattern.
    virtual void doRectUpdate(U32 mipLevel, AtlasClipMap::ClipStackEntry& cse, RectI srcRegion, RectI dstRegion);

    virtual void finishRectUpdates(AtlasClipMap::ClipStackEntry& cse)
    {
    }

    virtual bool isRenderToTargetCache()
    {
        return false;
    }
};

#endif