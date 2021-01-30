//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASMESHER_H_
#define _ATLASMESHER_H_

#include "console/console.h"
#include "core/stream.h"
#include "core/tVector.h"
#include "math/mPoint.h"
#include "math/mBox.h"

class AtlasActivationHeightfield;

/// Helper class for generating and outputting chunk geometry for an Atlas 
/// terrain.
class AtlasMesher
{
public:
    typedef U16 VertIndex;

protected:

    /// Represents a vertex in our mesh.
    struct Vert
    {
        Point2I pos; S16 z; bool special;

        /// Default constructor. Init garbage data.
        Vert() : pos(-1, -1), special(false) { };

        /// Initialize a vert on the heightfield.
        Vert(S16 aX, S16 aY) : pos(aX, aY), z(0), special(false) { };

        /// Initialize a special vert, which isn't on the heightfield.
        Vert(S16 aX, S16 aY, S16 aZ) : pos(aX, aY), z(aZ), special(true) { };

        inline const bool isEqual(const Point2I testPos) const
        {
            if (special)
                return false;

            if (testPos == pos)
                return true;

            return false;
        }

        inline const bool isSpecialEqual(const Point2I testPos, S16 testZ) const
        {
            if (!special)
                return false;

            if (testPos != pos)
                return false;

            if (z != testZ)
                return false;

            return true;
        }
    };

    /// Bounds of this mesh.
    Box3F mBounds;

    /// The vertices of this mesh.
    Vector<Vert> mVerts;

    /// The indices of this mesh. We store a trilist and cook into a tristrip.
    ///
    /// @note We're l33t like that.
    ///
    /// @note Actually, this isn't true atm. We just store a trilist. Tristrip
    ///       comes later in the milestone path.
    Vector<VertIndex> mIndices;

    /// Compression information, so we can write fixed point integers out.
    Point3F mCompressionFactor;

    /// Pointer to the heightfield we're generating mesh data for.
    AtlasActivationHeightfield* mHeight;

    S8 mLevel;

    Point3F getVertPos(const Vert& v);

public:
    S16 mMinZ, mMaxZ;


    /// Create an Atlas chunk mesh helper instance.
    ///
    /// @param  hf    Heightfield we're generating data for, used to get heights 
    ///               of vertices and calculate morph info, etc.
    /// @param  level Level we're generating for. Necessary so we know how much
    ///               to morph.
    AtlasMesher(AtlasActivationHeightfield* hf, S8 level);
    ~AtlasMesher();

    U32 getTriCount()
    {
        return mIndices.size() / 3;
    }

    inline void emitTri(VertIndex a, VertIndex b, VertIndex c)
    {
        mIndices.push_back(a);
        mIndices.push_back(b);
        mIndices.push_back(c);
    }

    inline void emitTri(Point2I a, Point2I b, Point2I c)
    {
        emitTri(
            getVertIndex(a),
            getVertIndex(b),
            getVertIndex(c)
        );
    }

    /// Return an index for the given vertex on the heightfield. This will emit
    /// the vertex if necessary, or recycle it if it already exists.
    ///
    /// @todo Make this use some technique for faster lookups.
    VertIndex getVertIndex(Point2I pos);

    /// Emit a special vertex; these are generally used for skirts.
    VertIndex addSpecialVert(Point2I pos, S16 z);

    void updateBounds();
    void optimize();
    void writeVertex(Stream* s, Vert* vert, const S8 level);
    void writeCollision(Stream* s);
    void write(Stream* s, const S8 level, bool writeCollision);
};

/// This is a utility class to generate an optimized triangle buffer for
/// the Atlas terrain collision engine.
///
/// It is fed the size of the grid, then the list of triangles for each bin.
///
/// Then it performs a processing step. This step consists of sorting the
/// binlists from longest to shortest, performing an insert on each one. The
/// insert first does a search in the buffer to see if the list already exists
/// (each list is sorted to maximize matches), then adds to the end of the
/// buffer if not, storing the resulting offset in either case.
///
/// Ends of binlists are marked by 0xFFFF.
///
/// Finally, it writes the results to disk (the offsets and the buffer).
class ChunkTriangleBufferGenerator
{
    U32 mGridSize;

    /// The triangle buffer. This encodes lists of triangles contained in each
    /// bin.
    Vector<U16> mTriangles;

    /// Stores the start of each bin's data.
    Vector<U16> mBinOffsets;

public:

    ChunkTriangleBufferGenerator(U32 gridSize)
    {
        mGridSize = gridSize;

        // Allocate space for the bins.
        mBinOffsets.setSize(gridSize * gridSize);

        // Assume we've got approx. 3 triangles per bin and prep some space.
        mTriangles.reserve(3 * mBinOffsets.size());
    }

    /// Reset the generator.
    void clear()
    {
        mTriangles.clear();
        mBinOffsets.clear();
    }

    /// Add a list to the buffer.
    void insertBinList(Point2I bin, Vector<U16>& binList)
    {
        // Stick the list on the end.
        mBinOffsets[bin.x * mGridSize + bin.y] = mTriangles.size();

        for (S32 i = 0; i < binList.size(); i++)
            mTriangles.push_back(binList[i]);
        mTriangles.push_back(0xFFFF); // Terminate the list.
    }

    bool write(Stream* s)
    {
        // Write this mofo to the disk!
        for (S32 i = 0; i < mBinOffsets.size(); i++)
            s->write(mBinOffsets[i]);

        s->write(U32(mTriangles.size()));
        for (S32 i = 0; i < mTriangles.size(); i++)
            s->write(mTriangles[i]);

        return true;
    }
};

#endif