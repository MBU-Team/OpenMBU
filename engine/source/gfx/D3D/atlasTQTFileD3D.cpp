//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include <d3d9.h>
#include "d3dx9math.h"
#include "gfx/D3D/gfxD3DDevice.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "gfx/D3D/gfxD3DCardProfiler.h"
#include "gfx/D3D/gfxD3DVertexBuffer.h"
#include "gfx/D3D/screenshotD3D.h"
#include "atlas/atlasTQTFile.h"
#include "core/frameAllocator.h"

void genMips(IDirect3DTexture9* tmpTex)
{
    // Assume the first level is filled; copy each surface to the next surface up.
    for (S32 i = 1; i < tmpTex->GetLevelCount(); i++)
    {
        IDirect3DSurface9* prevSurf = NULL, * curSurf = NULL;

        D3DAssert(tmpTex->GetSurfaceLevel(i - 1, &prevSurf), "genMips - A failed.");
        D3DAssert(tmpTex->GetSurfaceLevel(i, &curSurf), "genMips - B failed.");

        D3DAssert(D3DXLoadSurfaceFromSurface(
            curSurf, NULL, NULL,
            prevSurf, NULL, NULL,
            D3DX_FILTER_LINEAR | D3DX_FILTER_MIRROR, 0), "genMips - Copy failed!");

        SAFE_RELEASE(prevSurf);
        SAFE_RELEASE(curSurf);
    }
}

U32 AtlasTQTFile::writeLeafTile(U8* img, FileStream& s, U32 level, Point2I pos)
{
    U32 size = mTileSize;

    // Use D3DX to load the image, DXT compress it, and write it out.
    IDirect3DTexture9* tmpTex;
    IDirect3DSurface9* tmpSurf;

    // Create an offscreen texture to play with.
    /*D3DAssert(D3DXCreateTexture(((GFXD3DDevice*)GFX)->getDevice(), size, size,
       D3DX_DEFAULT, 0, D3DFMT_DXT1, D3DPOOL_SCRATCH, &tmpTex), "Wooga!");*/
    D3DAssert(((GFXD3DDevice*)GFX)->getDevice()->CreateTexture(
        size, size, 0,
        0, D3DFMT_DXT1, D3DPOOL_MANAGED,
        &tmpTex, NULL), "Unable to create temp texture!");

    D3DAssert(tmpTex->GetSurfaceLevel(0, &tmpSurf),
        "AtlasTQTFile::writeLeafTile - Unable to get surface!");

    // Copy our shizzle into it.
    RECT imgSize;
    imgSize.left = imgSize.top = 0; imgSize.bottom = imgSize.right = size;
    D3DAssert(D3DXLoadSurfaceFromMemory(tmpSurf, NULL, NULL, (LPCVOID)img,
        D3DFMT_R8G8B8, size * 3, NULL, &imgSize,
        D3DX_FILTER_LINEAR | D3DX_FILTER_MIRROR, 0),
        "AtlasTQTFile::writeLeafTile - Wonga!");

    SAFE_RELEASE(tmpSurf);

    genMips(tmpTex);

    // Ok, now we need to compress the image into a memory buffer and we're set.
    ID3DXBuffer* d3dxBuff;

    D3DAssert(D3DXSaveTextureToFileInMemory(&d3dxBuff, D3DXIFF_DDS, tmpTex, NULL),
        "AtlasTQTFile::writeLeafTile - Weeba!");

    // also for debug, write it out to disk.
    if (AtlasTQTFile::smDumpGeneratedTextures)
        D3DAssert(D3DXSaveTextureToFileA(avar("tqt%d_%dX%d.dds", level, pos.x, pos.y),
            D3DXIFF_DDS, tmpTex, NULL),
            "AtlasTQTFile::writeLeafTile - Weeba x2!");

    // AWESOMEZ, let's write the buffer out.
    U32 offset = s.getPosition();

    // Write the length, then the file.
    s.write(U32(d3dxBuff->GetBufferSize()));
    s.write(d3dxBuff->GetBufferSize(), d3dxBuff->GetBufferPointer());

    // Clean up...
    SAFE_RELEASE(d3dxBuff);
    SAFE_RELEASE(tmpTex);

    AssertFatal(offset, "AtlasTQTFile::writeLeafTile - Got zero offset!");

    // Return
    return offset;
}

U32 innerTileGenerator(AtlasTQTFile* file, FileStream& s, U32 level, Point2I pos);

// These aren't in the AtlasTQTFile as they're VERY implementation specific and at
// least in theory we want to avoid this sort of crud getting out into the
// base classes.
void getTileSurface(AtlasTQTFile* file, FileStream& s, U32 level, Point2I pos, IDirect3DSurface9* surf, RECT* dest)
{
    U32 filePos = file->mOffsets[file->getNodeIndex(level, pos.x, pos.y)];

    if (!filePos)
    {
        // We have to create it from children.
        filePos = innerTileGenerator(file, s, level, pos);
    }

    // Get the data from the file.
    FrameAllocatorMarker tmpAlloc;

    U32 oldFilePos = s.getPosition();

    s.setPosition(filePos);

    U32 imgSize;
    s.read(&imgSize);

    U8* imgData = (U8*)tmpAlloc.alloc(imgSize);

    s.read(imgSize, (void*)imgData);

    // Now, load the stuff into the appropriate location in the surface.
    D3DAssert(D3DXLoadSurfaceFromFileInMemory(surf, NULL, dest, imgData, imgSize, NULL, D3DX_FILTER_LINEAR | D3DX_FILTER_MIRROR, 0, NULL), "Failed to load saved texture.");

    s.setPosition(oldFilePos);

    // All done!
}

U32 innerTileGenerator(AtlasTQTFile* file, FileStream& s, U32 level, Point2I pos)
{
    // Note: due to the implementation of getTileSurface, we might have up to
    // file->mTreeDepth surfaces allocated at a time. This is not a lot, so
    // probably not a big deal.

    AssertFatal(level < file->mTreeDepth - 1, "innerTileGenerator - Tried to generate leaf tile!");

    // Use D3DX to load the image, DXT compress it, and write it out.
    IDirect3DTexture9* tmpTex;
    IDirect3DSurface9* tmpSurf;

    // Create an offscreen texture to play with.
    /*D3DAssert(D3DXCreateTexture(((GFXD3DDevice*)GFX)->getDevice(),
                                  file->mTileSize, file->mTileSize,
                                  D3DX_DEFAULT, 0, D3DFMT_DXT1, D3DPOOL_SCRATCH,
                                  &tmpTex), "Wooga!"); */
    D3DAssert(((GFXD3DDevice*)GFX)->getDevice()->CreateTexture(
        file->mTileSize, file->mTileSize, 0,
        0, D3DFMT_DXT1, D3DPOOL_MANAGED,
        &tmpTex, NULL), "Unable to create temp texture!");

    D3DAssert(tmpTex->GetSurfaceLevel(0, &tmpSurf), "Unable to get surface!");

    // Load the 4 children.
    RECT subSection;

    U32 halfSize = file->mTileSize / 2;
    // (0,0) child. 
    subSection.left = halfSize;         subSection.top = halfSize;
    subSection.right = file->mTileSize; subSection.bottom = file->mTileSize;
    getTileSurface(file, s, level + 1, (pos * 2) + Point2I(1, 1), tmpSurf, &subSection);

    // (0,1) child.
    subSection.left = 0;          subSection.top = halfSize;
    subSection.right = halfSize;  subSection.bottom = halfSize + halfSize;
    getTileSurface(file, s, level + 1, (pos * 2) + Point2I(1, 0), tmpSurf, &subSection);

    // (1,0) child.
    subSection.left = halfSize;           subSection.top = 0;
    subSection.right = file->mTileSize;   subSection.bottom = halfSize;
    getTileSurface(file, s, level + 1, (pos * 2) + Point2I(0, 1), tmpSurf, &subSection);

    // (1,1) child.
    subSection.left = 0;          subSection.top = 0;
    subSection.right = halfSize;  subSection.bottom = halfSize;
    getTileSurface(file, s, level + 1, (pos * 2) + Point2I(0, 0), tmpSurf, &subSection);

    // Clean up the surface and get some mips.
    SAFE_RELEASE(tmpSurf);

    genMips(tmpTex);

    // Save.
    ID3DXBuffer* d3dxBuff;

    D3DAssert(D3DXSaveTextureToFileInMemory(&d3dxBuff, D3DXIFF_DDS, tmpTex, NULL), "Weeba!");

    // also for debug, write it out to disk.
    if (AtlasTQTFile::smDumpGeneratedTextures)
        D3DAssert(D3DXSaveTextureToFileA(avar("tqt%d_%dX%d.dds", level, pos.x, pos.y), D3DXIFF_DDS, tmpTex, NULL), "Weeba x2!");

    // AWESOMEZ, let's write the buffer out.
    U32 offset = s.getPosition();

    // Write the length, then the file.
    s.write(U32(d3dxBuff->GetBufferSize()));
    s.write(d3dxBuff->GetBufferSize(), d3dxBuff->GetBufferPointer());

    // Clean up...
    SAFE_RELEASE(d3dxBuff);
    SAFE_RELEASE(tmpTex);

    // Store offset
    file->mOffsets[file->getNodeIndex(level, pos.x, pos.y)] = offset;

    // Return
    return offset;
}

void AtlasTQTFile::generateInnerTiles(FileStream& s)
{
    // Generate the inner tiles.
    innerTileGenerator(this, s, 0, Point2I(0, 0));
}
