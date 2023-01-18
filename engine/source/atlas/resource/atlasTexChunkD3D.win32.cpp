//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include <d3d9.h>
#include "d3dx9math.h"
#include "core/fileStream.h"
#include "core/frameAllocator.h"
#include "core/memstream.h"
#include "console/console.h"
#include "gfx/primBuilder.h"
#include "platform/profiler.h"
#include "gfx/D3D/gfxD3DDevice.h"
#include "gfx/D3D/gfxD3DCardProfiler.h"
#include "gfx/D3D/gfxD3DVertexBuffer.h"
#include "gfx/D3D/screenShotD3D.h"
#include "atlas/resource/atlasTexChunk.h"

void AtlasTexChunk::writeDDS(Stream* s)
{
    // Load the DDS into a texture, then use D3DX to write it out.
    GFXTexHandle tex;
    tex.set(dds, &GFXDefaultStaticDiffuseProfile, false);
    GFXD3DTextureObject* gdto = (GFXD3DTextureObject*)tex.getPointer();

    // Write to buffer.
    ID3DXBuffer* buffer = NULL;
    D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, gdto->getTex(), NULL);

    // And write that buffer... to a stream. Ho ho ho!
    s->write(buffer->GetBufferSize(), buffer->GetBufferPointer());
}

GBitmap* AtlasTexChunk::loadDDSIntoGBitmap(const U8* ddsBuffer, U32 ddsBufferSize)
{
    IDirect3DSurface9* surf = NULL;

    // We want JPEGs, let's convert it in a klunky way...
    D3DAssert(D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL,
        ddsBuffer, ddsBufferSize, NULL,
        D3DX_DEFAULT, 0, NULL),
        "AtlasTexChunk::loadDDSIntoGBitmap - D3DX failed to load from buffer.");

    ID3DXBuffer* buff = NULL;
    D3DAssert(D3DXSaveSurfaceToFileInMemory(&buff, D3DXIFF_PNG, surf, NULL, NULL),
        "AtlasTexChunk::loadDDSIntoGBitmap - D3DX failed to save back to buffer.");
    MemStream ms(buff->GetBufferSize(), buff->GetBufferPointer(), true, false);

    GBitmap* bitmap = new GBitmap;
    bitmap->readPNG(ms, true);

    // Cleanup!
    buff->Release();
    surf->Release();

    return bitmap;
}
