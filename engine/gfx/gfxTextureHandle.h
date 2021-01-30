//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------
#ifndef _GFXTEXTUREHANDLE_H_
#define _GFXTEXTUREHANDLE_H_

#include "gfx/gfxTextureObject.h"
#include "gfx/gfxTextureProfile.h"


//*****************************************************************************
// GFX Texture Handle
//*****************************************************************************
class GFXTexHandle : public RefPtr<GFXTextureObject>
{
public:
    GFXTexHandle() { }
    GFXTexHandle(GFXTextureObject* obj);
    GFXTexHandle(const GFXTexHandle& handle);

    // load texture
    GFXTexHandle(StringTableEntry texName, GFXTextureProfile* profile);
    bool set(StringTableEntry texName, GFXTextureProfile* profile);

    // register texture
    GFXTexHandle(GBitmap* bmp, GFXTextureProfile* profile, bool deleteBmp);
    bool set(GBitmap* bmp, GFXTextureProfile* profile, bool deleteBmp);

    GFXTexHandle(DDSFile* bmp, GFXTextureProfile* profile, bool deleteDDS);
    bool set(DDSFile* bmp, GFXTextureProfile* profile, bool deleteDDS);

    // Sized bitmap
    GFXTexHandle(U32 width, U32 height, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels = 1);
    bool set(U32 width, U32 height, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels = 1);
    bool set(U32 width, U32 height, U32 depth, void* pixels, GFXFormat format, GFXTextureProfile* profile, U32 numMipLevels = 1);

    U32 getWidth() { return getPointer() ? getPointer()->getWidth() : 0; }
    U32 getHeight() { return getPointer() ? getPointer()->getHeight() : 0; }
    U32 getDepth() { return getPointer() ? getPointer()->getDepth() : 0; }

    void refresh();
    void free();   ///< Release any resources attached to this object.

    GFXLockedRect* lock(U32 mipLevel = 0, RectI* inRect = NULL)
    {
        return getPointer()->lock(mipLevel, inRect);
    }

    void unlock(U32 mipLevel = 0)
    {
        getPointer()->unlock(mipLevel);
    }

    // copy to bitmap.  see gfxTetureObject.h for description of what types of textures
    // can be copied into bitmaps.  returns true if successful, false otherwise
    bool copyToBmp(GBitmap* bmp) { return getPointer() ? getPointer()->copyToBmp(bmp) : false; }

    //---------------------------------------------------------------------------
    // Operator overloads
    //---------------------------------------------------------------------------
    GFXTexHandle& operator=(const GFXTexHandle& t)
    {
        RefObjectRef::set(t.getPointer());
        return *this;
    }

    GFXTexHandle& operator=(GFXTextureObject* to)
    {
        RefObjectRef::set(to);
        return *this;
    }

    bool operator==(const GFXTexHandle& t) const { return t.getPointer() == getPointer(); }
    bool operator!=(const GFXTexHandle& t) const { return t.getPointer() != getPointer(); }

    operator GFXTextureObject* ()
    {
        return (GFXTextureObject*)getPointer();
    }

    //---------------------------------------------------------------------------
    // Misc
    //---------------------------------------------------------------------------
    GBitmap* getBitmap();
};


#endif
