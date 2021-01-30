//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFX_D3D_Texture_Manager_H_
#define _GFX_D3D_Texture_Manager_H_

#include "console/console.h"

#include "gfx/gfxTextureManager.h"
#include "gfxD3DTextureObject.h"

// #define D3D_TEXTURE_SPEW

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(a) if( (a) != NULL ) (a)->Release(); (a) = NULL;
#endif


//*****************************************************************************
// GFX D3D Texture Manager
//*****************************************************************************
class GFXD3DTextureManager : public GFXTextureManager
{
    friend class GFXD3DTextureManager;

    U32 mCurTexSet[TEXTURE_STAGE_COUNT];

private:
    LPDIRECT3DDEVICE9 mD3DDevice;
    void innerCreateTexture(GFXD3DTextureObject* obj, U32 height, U32 width, U32 depth, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels, bool forceMips = false);

protected:
    GFXTextureObject* _createTexture(U32 height, U32 width, U32 depth, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels, bool forceMips = false);
    GFXTextureObject* _loadDDSHack(const char* filename, GFXTextureProfile* profile);
    bool _loadTexture(GFXTextureObject* texture, DDSFile* dds);
    bool _loadTexture(GFXTextureObject* texture, GBitmap* bmp);
    bool _loadTexture(GFXTextureObject* texture, void* raw);
    bool _refreshTexture(GFXTextureObject* texture);
    bool _freeTexture(GFXTextureObject* texture, bool zombify = false);
    U32 _getTotalVideoMemory();

public:
    GFXD3DTextureManager(LPDIRECT3DDEVICE9 d3ddevice);

};

#endif
