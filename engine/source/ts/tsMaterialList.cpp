//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShape.h"
#include "materials/materialPropertyMap.h"
#include "materials/material.h"
#include "materials/matInstance.h"
#include "materials/sceneData.h"
#include "sceneGraph/sceneGraph.h"

TSMaterialList::TSMaterialList(U32 materialCount,
    const char** materialNames,
    const U32* materialFlags,
    const U32* reflectanceMaps,
    const U32* bumpMaps,
    const U32* detailMaps,
    const F32* detailScales,
    const F32* reflectionAmounts)
    : MaterialList(materialCount, materialNames),
    mNamesTransformed(false)
{
    VECTOR_SET_ASSOCIATION(mFlags);
    VECTOR_SET_ASSOCIATION(mReflectanceMaps);
    VECTOR_SET_ASSOCIATION(mBumpMaps);
    VECTOR_SET_ASSOCIATION(mDetailMaps);
    VECTOR_SET_ASSOCIATION(mDetailScales);
    VECTOR_SET_ASSOCIATION(mReflectionAmounts);

    allocate(getMaterialCount());

    dMemcpy(mFlags.address(), materialFlags, getMaterialCount() * sizeof(U32));
    dMemcpy(mReflectanceMaps.address(), reflectanceMaps, getMaterialCount() * sizeof(U32));
    dMemcpy(mBumpMaps.address(), bumpMaps, getMaterialCount() * sizeof(U32));
    dMemcpy(mDetailMaps.address(), detailMaps, getMaterialCount() * sizeof(U32));
    dMemcpy(mDetailScales.address(), detailScales, getMaterialCount() * sizeof(F32));
    dMemcpy(mReflectionAmounts.address(), reflectionAmounts, getMaterialCount() * sizeof(F32));
}

TSMaterialList::TSMaterialList()
    : mNamesTransformed(false)
{
    VECTOR_SET_ASSOCIATION(mFlags);
    VECTOR_SET_ASSOCIATION(mReflectanceMaps);
    VECTOR_SET_ASSOCIATION(mBumpMaps);
    VECTOR_SET_ASSOCIATION(mDetailMaps);
    VECTOR_SET_ASSOCIATION(mDetailScales);
    VECTOR_SET_ASSOCIATION(mReflectionAmounts);
}

TSMaterialList::TSMaterialList(const TSMaterialList* pCopy)
    : MaterialList(pCopy)
{
    VECTOR_SET_ASSOCIATION(mFlags);
    VECTOR_SET_ASSOCIATION(mReflectanceMaps);
    VECTOR_SET_ASSOCIATION(mBumpMaps);
    VECTOR_SET_ASSOCIATION(mDetailMaps);
    VECTOR_SET_ASSOCIATION(mDetailScales);
    VECTOR_SET_ASSOCIATION(mReflectionAmounts);

    mFlags = pCopy->mFlags;
    mReflectanceMaps = pCopy->mReflectanceMaps;
    mBumpMaps = pCopy->mBumpMaps;
    mDetailMaps = pCopy->mDetailMaps;
    mDetailScales = pCopy->mDetailScales;
    mReflectionAmounts = pCopy->mReflectionAmounts;
    mNamesTransformed = pCopy->mNamesTransformed;
}

TSMaterialList::~TSMaterialList()
{
    free();
}

void TSMaterialList::free()
{
    // IflMaterials will duplicate names and textures found in other material slots
    // (In particular, IflFrame material slots).
    // Set the names to NULL now so our parent doesn't delete them twice.
    // Texture handles should stay as is...
    for (U32 i = 0; i < getMaterialCount(); i++)
        if (mFlags[i] & TSMaterialList::IflMaterial)
            mMaterialNames[i] = NULL;

    // these aren't found on our parent, clear them out here to keep in synch
    mFlags.clear();
    mReflectanceMaps.clear();
    mBumpMaps.clear();
    mDetailMaps.clear();
    mDetailScales.clear();
    mReflectionAmounts.clear();

    Parent::free();
}

void TSMaterialList::remap(U32 toIndex, U32 fromIndex)
{
    AssertFatal(toIndex < size() && fromIndex < size(), "TSMaterial::remap");

    // only remap texture handle...flags and maps should stay the same...

    mMaterials[toIndex] = mMaterials[fromIndex];
    mMaterialNames[toIndex] = mMaterialNames[fromIndex];
    mMatInstList[toIndex] = mMatInstList[fromIndex];
}

void TSMaterialList::push_back(const char* name, U32 flags, U32 rMap, U32 bMap, U32 dMap, F32 dScale, F32 emapAmount)
{
    Parent::push_back(name);
    mFlags.push_back(flags);
    if (rMap == 0xFFFFFFFF)
        mReflectanceMaps.push_back(getMaterialCount() - 1);
    else
        mReflectanceMaps.push_back(rMap);
    mBumpMaps.push_back(bMap);
    mDetailMaps.push_back(dMap);
    mDetailScales.push_back(dScale);
    mReflectionAmounts.push_back(emapAmount);
}

void TSMaterialList::allocate(U32 sz)
{
    mFlags.setSize(sz);
    mReflectanceMaps.setSize(sz);
    mBumpMaps.setSize(sz);
    mDetailMaps.setSize(sz);
    mDetailScales.setSize(sz);
    mReflectionAmounts.setSize(sz);
}

U32 TSMaterialList::getFlags(U32 index)
{
    AssertFatal(index < getMaterialCount(), "TSMaterialList::getFlags: index out of range");
    return mFlags[index];
}

void TSMaterialList::setFlags(U32 index, U32 value)
{
    AssertFatal(index < getMaterialCount(), "TSMaterialList::getFlags: index out of range");
    mFlags[index] = value;
}

void TSMaterialList::load(U32 index, const char* path)
{
    AssertFatal(index < getMaterialCount(), "TSMaterialList::getFlags: index out of range");

    if (mFlags[index] & TSMaterialList::NoMipMap)
        mTextureType = BitmapTexture;
    else if (mFlags[index] & TSMaterialList::MipMap_ZeroBorder)
        mTextureType = ZeroBorderTexture;
    else
        mTextureType = MeshTexture;

    Parent::load(index, path);
}

bool TSMaterialList::write(Stream& s)
{
    if (!Parent::write(s))
        return false;

    U32 i;
    for (i = 0; i < getMaterialCount(); i++)
        s.write(mFlags[i]);

    for (i = 0; i < getMaterialCount(); i++)
        s.write(mReflectanceMaps[i]);

    for (i = 0; i < getMaterialCount(); i++)
        s.write(mBumpMaps[i]);

    for (i = 0; i < getMaterialCount(); i++)
        s.write(mDetailMaps[i]);

    for (i = 0; i < getMaterialCount(); i++)
        s.write(mDetailScales[i]);

    for (i = 0; i < getMaterialCount(); i++)
        s.write(mReflectionAmounts[i]);

    return (s.getStatus() == Stream::Ok);
}

bool TSMaterialList::read(Stream& s)
{
    if (!Parent::read(s))
        return false;

    allocate(getMaterialCount());

    U32 i;
    if (TSShape::smReadVersion < 2)
    {
        for (i = 0; i < getMaterialCount(); i++)
            setFlags(i, S_Wrap | T_Wrap);
    }
    else
    {
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mFlags[i]);
    }

    if (TSShape::smReadVersion < 5)
    {
        for (i = 0; i < getMaterialCount(); i++)
        {
            mReflectanceMaps[i] = i;
            mBumpMaps[i] = 0xFFFFFFFF;
            mDetailMaps[i] = 0xFFFFFFFF;
        }
    }
    else
    {
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mReflectanceMaps[i]);
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mBumpMaps[i]);
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mDetailMaps[i]);
    }

    if (TSShape::smReadVersion > 11)
    {
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mDetailScales[i]);
    }
    else
    {
        for (i = 0; i < getMaterialCount(); i++)
            mDetailScales[i] = 1.0f;
    }

    if (TSShape::smReadVersion > 20)
    {
        for (i = 0; i < getMaterialCount(); i++)
            s.read(&mReflectionAmounts[i]);
    }
    else
    {
        for (i = 0; i < getMaterialCount(); i++)
            mReflectionAmounts[i] = 1.0f;
    }

    if (TSShape::smReadVersion < 16)
    {
        // make sure emapping is off for translucent materials on old shapes
        for (i = 0; i < getMaterialCount(); i++)
            if (mFlags[i] & TSMaterialList::Translucent)
                mFlags[i] |= TSMaterialList::NeverEnvMap;
    }

    // get rid of name of any ifl material names
    for (i = 0; i < getMaterialCount(); i++)
    {
        const char* str = dStrrchr(mMaterialNames[i], '.');
        if (mFlags[i] & TSMaterialList::IflMaterial ||
            (TSShape::smReadVersion < 6 && str && dStricmp(str, ".ifl") == 0))
        {
            delete[] mMaterialNames[i];
            mMaterialNames[i] = NULL;
        }
    }

    return (s.getStatus() == Stream::Ok);
}


//--------------------------------------------------------------------------
// Sets the specified material in the list to the specified texture.  also 
// remaps mat instances based on the new texture name.  Returns false if 
// the specified texture is not valid.
//--------------------------------------------------------------------------
bool TSMaterialList::setMaterial(U32 i, const char* texPath)
{
    if (i < 0 || i > mMaterials.size())
        return false;

    if (texPath == NULL || texPath[0] == NULL)
        return false;

    // figure out new material name to make sure that we are actually changing the material
    StringTableEntry path;
    StringTableEntry filename;
    ResManager::getPaths(texPath, path, filename);

    if (dStrlen(filename) <= 0)
        return false;

    char* matName = new char[dStrlen(filename) + 1];
    dStrcpy(matName, filename);
    // eliminate extension from filename
    //char* ext = dStrrchr(matName, '.');
    //if (ext)
    //   *ext = 0;

    // ok, is our current material same as the supposedly new material?
    if (mMaterials[i].isValid() && dStrcmp(mMaterialNames[i], matName) == 0)
    {
        // same material, return true since we aren't changing it
        delete[] matName;
        return true;
    }

    GFXTexHandle tex(texPath, &GFXDefaultStaticDiffuseProfile);
    if (!tex.isValid())
    {
        delete[] matName;
        return false;
    }

    // change texture
    mMaterials[i] = tex;

    // change material name
    if (mMaterialNames[i])
        delete[] mMaterialNames[i];
    mMaterialNames[i] = matName;

    // dump the old mat instance
    if (mMatInstList[i])
        delete mMatInstList[i];
    mMatInstList[i] = NULL;

    // see if we can map it
    MaterialPropertyMap* matMap = MaterialPropertyMap::get();
    if (!matMap)
        return true;

    const MaterialPropertyMap::MapEntry* entry = matMap->getMapEntry(matName);
    if (entry && entry->materialName)
    {
        Material* mat = dynamic_cast<Material*>(Sim::findObject(entry->materialName));
        if (mat)
        {
            MatInstance* matInst = new MatInstance(*mat);
            mMatInstList[i] = matInst;

            // initialize the new mat instance
            SceneGraphData sgData;
            sgData.useLightDir = true;
            sgData.useFog = SceneGraph::renderFog;
            GFXVertexPNT* tsVertex = NULL;
            matInst->init(sgData, (GFXVertexFlags)getGFXVertFlags(tsVertex));
        }
    } else {
#ifdef CREATE_MISSING_MATERIALS
        Con::warnf("TSMaterialList::setMaterial: unmapped material %s, generating new material", matName);

        const char* texPath;

        if (path)
        {
            char buffer[512];
            dSprintf(buffer, sizeof(buffer), "%s/%s", path, matName);
            texPath = StringTable->insert(buffer);
        }
        else
        {
            texPath = StringTable->insert(matName);
        }

        const char* bumpPath;
        char buffer2[512];
        dSprintf(buffer2, sizeof(buffer2), "%s.bump", texPath);
        bumpPath = StringTable->insert(buffer2);

        static int newMatIndex = 0;

        const char* newMatName;
        char buffer3[512];
        dSprintf(buffer3, sizeof(buffer3), "GeneratedMaterialTS%d", newMatIndex);
        newMatName = buffer3;
        newMatName = StringTable->insert(buffer3);
        newMatIndex++;

        Material *mat = new Material;
        mat->mapTo = matName;
        mat->baseTexFilename[0] = texPath;
        mat->bumpFilename[0] = bumpPath;
        mat->registerObject(newMatName);

        MatInstance *matInst = new MatInstance(*mat);
        mMatInstList[i] = matInst;

        // initialize the new mat instance
        SceneGraphData sgData;
        sgData.useLightDir = true;
        sgData.useFog = SceneGraph::renderFog;
        GFXVertexPNT* tsVertex = NULL;
        matInst->init(sgData, (GFXVertexFlags)getGFXVertFlags(tsVertex));
#endif
    }
    return true;
}



