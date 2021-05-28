//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLAS_RUNTIME_ATLASCLIPMAPCACHE_H_
#define _ATLAS_RUNTIME_ATLASCLIPMAPCACHE_H_

#include "atlas/runtime/atlasInstanceTexTOC.h"
#include "atlas/runtime/atlasClipMap.h"

/// Interface for a clipmap image cache.
///
/// @see AtlasClipMap
class IAtlasClipMapImageCache
{
public:

    virtual ~IAtlasClipMapImageCache()
    {
    }

    /// Initialize the image cache that can hold at least clipMapSize*clipMapSize
    /// pixels of data. In general the image cache will allocate a bit more
    /// than that so it can be efficient in pre-empting requests.
    ///
    /// The image cache will also store mipmap data for the interest point,
    /// at least clipMapSize px at each miplevel. e.g., for a parameter of 512,
    /// we would have available in the cache at all times 512*512*clipMapDepth
    /// pixels. Ideally, we would ensure at least one page quanta's worth of
    /// extra data is present at all levels so we can fulfill new data
    /// requests quickly.
    ///
    /// For instance, if we have 128px paged tiles, we'd want to have at least
    /// 512/128 = 4 base tiles and 2 border tiles on each axis, so 36 loaded
    /// tiles at all times at the source level of detail. This is about two and
    /// a quarter megabytes of data per clipmap level, so a cache of about 7
    /// levels * 2.25mb = 15.75mb in system memory for a 32k px source texture. 
    /// We don't need any "mip" levels - just pure source data for every 
    /// cliplevel greater than or equal to the clipmap size.
    ///
    /// As we may need to regenerate the full clipmap in the case of 
    /// zombification, we can't discard the top level of data, even though it is
    /// never needed in the course of normal clipmap operation.
    virtual void initialize(U32 clipMapSize, U32 clipMapDepth) = 0;

    /// Indicate our interest area center. The cache is for a fixed size
    /// area, so we disallow changing that area size.
    virtual void setInterestCenter(Point2I origin) = 0;

    /// Indicate that we are going to be updating a series of rectangles on
    /// specified level of the clipstack.
    virtual void beginRectUpdates(AtlasClipMap::ClipStackEntry& cse) = 0;

    /// Indicate that we're updating a specific rectangle on a clipstack.
    ///
    /// @param srcRegion The region on the source mip level we are updating
    ///                  from.
    /// @param dstRegion The region on the clipmap layer we'll be updating into.
    virtual void doRectUpdate(U32 mipLevel, AtlasClipMap::ClipStackEntry& cse, RectI srcRegion, RectI dstRegion) = 0;

    /// And finally, give the cache a chance to finish processing  for a 
    /// specified clipstack level.
    virtual void finishRectUpdates(AtlasClipMap::ClipStackEntry& cse) = 0;

    /// Is cached data available for a given region?
    virtual bool isDataAvailable(U32 mipLevel, RectI region) = 0;

    /// If true, we'll allocate render targets rather than normal diffuse maps.
    virtual bool isRenderToTargetCache() = 0;
};

#endif