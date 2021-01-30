//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) 2003 GarageGames.Com
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------
#ifndef _ATLASTQTFILE_H_
#define _ATLASTQTFILE_H_

#include "core/tVector.h"
#include "gfx/gBitmap.h"

class DDSFile;

/// Core of texture paging functionality, allows textures to be paged from a
/// .TQT file.
class AtlasTQTFile
{
public:
    /// Stream for file IO.
    Stream* mStream;

    /// Offset of each image chunk in the TQT.
    Vector<U32> mOffsets;

    U32 mFileSize;

    U32 mVersion;
    U32 mTreeDepth;
    U32 mTileSize;

    /// Debug aid. Sets the texture generator to dump every generated texture
    /// as a DDS to the CWD.
    static bool smDumpGeneratedTextures;

    // Return the number of nodes in a fully populated quadtree of the specified depth.
    static const U32 getNodeCount(const S32 depth)
    {
        return 0x55555555 & ((1 << depth * 2) - 1);
    }

    // Given a tree level and the indices of a node within that tree
    // level, this function returns a node index.
    static const U32 getNodeIndex(const S32 level, const S32 col, const S32 row)
    {
        AssertFatal(col >= 0 && col < BIT(level), "Invalid col for node_index!");
        AssertFatal(row >= 0 && row < BIT(level), "Invalid row for node_index!");

        return getNodeCount(level) + (row << level) + col;
    }

    /// Write leaf tile data from an 8-bit RGB mTileSize square raw buffer.
    ///
    /// The level and pos parameters are for debugging.
    U32 writeLeafTile(U8* img, FileStream& s, U32 level, Point2I pos);

    /// Read in child tiles, combine, filter down, and write out.
    void generateInnerTiles(FileStream& s);

    // Copy nodes from one AtlasTQTFile to another. Not working ATM.
    void copy(AtlasTQTFile* tqt00, Stream& s, U32 srcLevel, Point2I srcPos, U32 dstLevel, Point2I dstPos);

public:

    AtlasTQTFile(const char* file);
    AtlasTQTFile()
    {
        mStream = NULL;
    };

    ~AtlasTQTFile();

    void createTQT(const char* sourceImage, const char* outputTQT, U32 treeDepth, U32 tileSize);
    void merge(AtlasTQTFile* tqt00, AtlasTQTFile* tqt01, AtlasTQTFile* tqt10, AtlasTQTFile* tqt11, const char* outFile);

    const bool isDDS() const { return mVersion == 2; }
    const bool isBitmap() const { return mVersion == 1; }
    const S32 getTreeDepth() const { return mTreeDepth; }

    /// Create a new GBitmap containing the specified image in the TQT.
    GBitmap* loadBitmap(const U32 level, const Point2I pos) const;

    /// Create a new DDSFile containing the specified image in the TQT.
    DDSFile* loadDDS(const U32 level, const Point2I pos) const;

    /// Offset into the file where a given tile lives.
    U32 getTileOffset(const U32 level, const Point2I pos) const;

    /// Best guess towards the length of a tile based on next tile/EOF position.
    U32 getTileLengthGuess(const U32 level, const Point2I pos) const;

};

#endif