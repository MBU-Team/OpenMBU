//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AtlasOldHeightfield_H_
#define _AtlasOldHeightfield_H_

#include "console/console.h"
#include "core/stream.h"
#include "core/tVector.h"
#include "math/mPoint.h"
#include "math/mBox.h"

/// Returns the bit position of the lowest 1 bit in the given value.
/// If x == 0, returns the number of bits in an integer.
///
/// E.g. lowest_one(1) == 0; lowest_one(16) == 4; lowest_one(5) == 0;
///
/// @note This needs to get moved to the math library.
///
/// @ingroup AtlasEditorOld
static inline S32	lowestOne(const S32 x)
{
#ifdef TORQUE_CPU_X86
    __asm {
        mov eax, 0
        bsf eax, x
    }
#else
    return x & -x;
#endif
}

/// An unsigned 16bit heightfield, used in situations where we need to
/// manipulate heightfield information directly (ie, mesh generation, collision,
/// and so forth).
///
/// @ingroup AtlasEditorOld
class AtlasOldHeightfield
{
public:

    typedef S16 HeightType;

    /// Heightfield buffer.
    HeightType* mHeight;

    /// Sometimes we want a normal buffer.
    Point3F* mNormal;

    /// Log 2 of the buffer size.
    U32 mFieldShift;
    U32 mFieldShiftMinusOne;

    U32 mRootChunkLevel;

    /// Horizontal spacing. The distance from one point to the next on the grid.
    F32 mSampleSpacing;

    /// The scale factor for vertical samples.
    F32 mVerticalScale;

    /// Create a new heightfield.
    ///
    /// Specify the size as a logarithm of 2. So, 8 = 256x256, 9 = 512x512, etc.
    AtlasOldHeightfield(const U32 sizeLog2, const F32 sampleSpacing, const F32 heightScale);
    virtual ~AtlasOldHeightfield();

    /// Given the coordinates of the center of a quadtree node, this
    /// function returns its node index.  The node index is essentially
    /// the node's rank in a breadth-first quadtree traversal.  Assumes
    /// a [nw, ne, sw, se] traversal order.
    ///
    /// If the coordinates don't specify a valid node (e.g. if the coords
    /// are outside the heightfield) then returns -1.
    inline const U32 nodeIndex(const Point2I pos) const
    {
        if (pos.x < 0 || pos.x >= size() || pos.y < 0 || pos.y >= size())
        {
            return -1;
        }

        const S32	l1 = lowestOne(pos.x | pos.y);
        const S32	depth = mFieldShift - l1 - 1;

        const S32	base = 0x55555555 & ((1 << depth * 2) - 1);	// total node count in all levels above ours.
        const S32	shift = l1 + 1;

        // Effective coords within this node's level.
        const S32	col = pos.x >> shift;
        const S32	row = pos.y >> shift;

        return base + (row << depth) + col;
    }

    /// Determine the index for a given location.
    inline const U32 indexOf(const Point2I pos) const
    {
        return pos.x * realSize() + pos.y;
    }

    inline const U32 size() const
    {
        return BIT(mFieldShift);
    }

    inline const U32 realSize() const
    {
        return BIT(mFieldShift) + 1;
    }

    inline HeightType& sample(const Point2I pos) const
    {
        return mHeight[indexOf(pos)];
    }

    inline const HeightType& sampleRead(const Point2I pos) const
    {
        return mHeight[indexOf(pos)];
    }

    inline const F32 sampleScaled(const Point2I pos) const
    {
        return F32(mHeight[indexOf(pos)]) * mVerticalScale;
    }

    bool loadRawU16(Stream& s);
    bool saveJpeg(const char* filename);
    bool saveJpeg(Stream& s);
    bool write(Stream& s);
    bool read(Stream& s);

    /// Given an x or z coordinate, along which an edge runs, this
    /// function returns the lowest LOD level that the edge divides.
    ///
    /// (This is useful for determining how conservative we need to be
    /// with edge skirts.)
    inline S32 minimumEdgeLod(S32 coord)
    {
        const S32 depth = (mFieldShiftMinusOne - lowestOne(coord));
        return mClamp(mRootChunkLevel - depth, 0, mRootChunkLevel);
    }

    /// Returns the normal for the given coordinate
    Point3F getNormal(const Point2I pos) const
    {
        AssertFatal(mNormal, "AtlasOldHeightfield::getNormal - no normal array!");
        return mNormal[pos.x * size() + pos.y];
    }

    void generateNormals();

};

#endif