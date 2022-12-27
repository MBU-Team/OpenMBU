//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATERIALLIST_H_
#define _MATERIALLIST_H_

#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif

#include "console/simBase.h"
#include "gfx/gfxTextureHandle.h"

class Material;
class MaterialPropertyMap;
class MatInstance;

enum TextureHandleType
{
    BitmapTexture = 0,         ///< Regular bitmap - does not to be pow2, but will be converted to pow2, only 1 mip level
    BitmapKeepTexture,         ///< Same as BitmapTexture, but the data will be kept after creation
    BitmapNoDownloadTexture,   ///< Same as BitmapTexture except data will not be loaded to OpenGL and cannot be "bound"
    RegisteredTexture,         ///< INTERNAL USE ONLY - texture that has already been created and is just being reinstated
    MeshTexture,               ///< Has "small textures"
    TerrainTexture,            ///< OFF LIMITS - for terrain engine use only.  You WILL cause problems if you use this type
    SkyTexture,                ///< Hooks into the sky texture detail level
    InteriorTexture,           ///< Has "small textures" and hooks into the interior texture detail level
    BumpTexture,               ///< Same as DetailTexture, except colors are halved
    InvertedBumpTexture,       ///< Same as BumpTexture, except colors are then inverted

    DetailTexture,             ///< If not palettized, will extrude mipmaps, only used for terrain detail maps
    ZeroBorderTexture          ///< Clears the border of the texture on all mip levels
};

//--------------------------------------
class MaterialList : public ResourceInstance
{
private:
    friend class TSMaterialList;

    enum Constants { BINARY_FILE_VERSION = 1 };

    Vector<MatInstance*> mMatInstList;



public:
    Vector<SimObjectPtr<Material> > mMappedMaterials;
    VectorPtr<char*> mMaterialNames;
    Vector<GFXTexHandle> mMaterials;
protected:
    bool mClampToEdge;
    TextureHandleType mTextureType;

public:
    MaterialList();
    MaterialList(U32 materialCount, const char** materialNames);
    ~MaterialList();

    /// Note: this is not to be confused with MaterialList(const MaterialList&).  Copying
    ///  a material list in the middle of it's lifetime is not a good thing, so we force
    ///  it to copy at construction time by retricting the copy syntax to
    ///  ML* pML = new ML(&copy);
    explicit MaterialList(const MaterialList*);

    U32 getMaterialCount() { return (U32)mMaterials.size(); }
    const char* getMaterialName(U32 index) { return mMaterialNames[index]; }
    GFXTexHandle& getMaterial(U32 index)
    {
        AssertFatal(index < (U32)mMaterials.size(), "MaterialList::getMaterial: index lookup out of range.");
        return mMaterials[index];
    }

    // material properties
    void setTextureType(TextureHandleType type) { mTextureType = type; }
    void setClampToEdge(bool tf) { mClampToEdge = tf; }

    void set(U32 materialCount, const char** materialNames);
    U32  push_back(GFXTexHandle textureHandle, const char* filename);
    U32  push_back(const char* filename);

    virtual void load(U32 index, const char* path = 0);
    bool load(const char* path = 0);
    bool load(TextureHandleType type, const char* path = 0, bool clampToEdge = false);
    void unload();
    virtual void free();
    void clearMatInstList();
    void clearMappedMaterials();

    typedef Vector<GFXTexHandle>::iterator iterator;
    typedef Vector<GFXTexHandle>::value_type value;
    GFXTexHandle& front() { return mMaterials.front(); }
    GFXTexHandle& first() { return mMaterials.first(); }
    GFXTexHandle& last() { return mMaterials.last(); }
    bool       empty() { return mMaterials.empty(); }
    U32        size() { return (U32)mMaterials.size(); }
    iterator   begin() { return mMaterials.begin(); }
    iterator   end() { return mMaterials.end(); }
    value operator[] (S32 index) { return getMaterial(U32(index)); }

    bool read(Stream& stream);
    bool write(Stream& stream);

    bool readText(Stream& stream, U8 firstByte);
    bool readText(Stream& stream);
    bool writeText(Stream& stream);

    void mapMaterials(const char* path = NULL);
    Material* getMappedMaterial(U32 index);
    MatInstance* getMaterialInst(U32 texIndex);
    void setMaterialInst(MatInstance* matInst, U32 texIndex);

};


//--------------------------------------
inline bool MaterialList::load(TextureHandleType type, const char* path, bool clampToEdge)
{
    mTextureType = type;
    mClampToEdge = clampToEdge;
    return load(path);
}


extern ResourceInstance* constructMaterialList(Stream& stream);


#endif
