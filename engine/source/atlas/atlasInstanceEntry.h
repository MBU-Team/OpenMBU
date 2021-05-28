//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASINSTANCEENTRY_H_
#define _ATLASINSTANCEENTRY_H_

#include "atlas/atlasResourceEntry.h"

class AtlasInstance;

/// Since each AtlasInstance needs to keep track of its LOD seperately, each one
/// maintains a shadow set of entries which are responsible for solely tracking
/// LOD information, and maintaining pointers to the appropriate 
/// AtlasResourceEntrys. This class implements those entries. It acts to proxy
/// LOD requests into the AtlasResource, as well as making sure that refcounts
/// are correct at all times.
///
/// It also does rendering based on the LOD that it contains.
class AtlasInstanceEntry
{
private:
    static AtlasInstance* smCurrentInstance;

    AtlasResourceEntry* mResourceEntry;

    AtlasInstanceEntry* mNeighbor[4];
    AtlasInstanceEntry* mChildren[4];
    AtlasInstanceEntry* mParent;

    /// Our label.
    S32 mLabel;

    /// Used to enforce a max morph per-frame. We can unload as fast as we
    /// want, but data we're paging in will morph.
    F32 mLastMorph;

    /// Debugging aid. Goes up when we split or combine, does down when we
    /// don't.
    F32 mHeat;

    U16   mLod;
    bool  mSplit;

    bool  mGeometryReference;
    bool  mTextureReference;

public:

    static void setInstance(AtlasInstance* ci);
    static AtlasInstance* getInstance();

    const bool  hasChildren() const { return mChildren[0] != NULL; }

    AtlasInstanceEntry()
    {
        mLastMorph = 1.f;
    }

    ~AtlasInstanceEntry()
    {
    }

    void init(AtlasResourceEntry* cre);

    // Enable this chunk.  Use the given viewpoint to decide what morph
    // level to use.
    void  doSplit();

    void  updateLOD();
    void  updateTexture();

    void  render(const S32 clipMask);

    /// Reset LOD data.
    void clear();

    /// Return true if this chunk can be split.  Also, requests the
    /// necessary data for the chunk children and its dependents.
    ///
    /// A chunk won't be able to be split if it doesn't have vertex data,
    /// or if any of the dependent chunks don't have vertex data.
    bool canSplit();

    const bool isSplit() const;

    /// Schedule this node's data for loading at the given priority.  Also,
    /// schedule our child/descendent nodes for unloading.
    void warmUpData(F32 priority);

    /// Unload geometry under this entry.
    void requestUnloadGeometry(AtlasResource::DeleteReason reason);

    /// Unload textures under this entry.
    void requestUnloadTextures(AtlasResource::DeleteReason reason);
};

#endif