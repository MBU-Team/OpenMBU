//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "core/resManager.h"
#include "core/stream.h"
#include "materials/materialList.h"
#include "materialPropertyMap.h"
#include "material.h"
#include "materials/sceneData.h"
#include "materials/matInstance.h"
#include "renderInstance/renderInstMgr.h"

//--------------------------------------
MaterialList::MaterialList()
{
    mTextureType = BitmapTexture;
    mClampToEdge = false;

    VECTOR_SET_ASSOCIATION(mMaterialNames);
    VECTOR_SET_ASSOCIATION(mMaterials);
}

MaterialList::MaterialList(const MaterialList* pCopy)
{
    VECTOR_SET_ASSOCIATION(mMaterialNames);
    VECTOR_SET_ASSOCIATION(mMaterials);

    mClampToEdge = pCopy->mClampToEdge;
    mTextureType = pCopy->mTextureType;

    mMaterialNames.setSize(pCopy->mMaterialNames.size());
    S32 i;
    for (i = 0; i < mMaterialNames.size(); i++) {
        if (pCopy->mMaterialNames[i]) {
            mMaterialNames[i] = new char[dStrlen(pCopy->mMaterialNames[i]) + 1];
            dStrcpy(mMaterialNames[i], pCopy->mMaterialNames[i]);
        }
        else {
            mMaterialNames[i] = NULL;
        }
    }

    mMaterials.setSize(pCopy->mMaterials.size());
    for (i = 0; i < mMaterials.size(); i++) {
        constructInPlace(&mMaterials[i]);
        mMaterials[i] = pCopy->mMaterials[i];
    }

    clearMatInstList();
    mMatInstList.setSize(pCopy->mMaterials.size());
    for (i = 0; i < mMatInstList.size(); i++)
    {
        if (i < pCopy->mMatInstList.size() && pCopy->mMatInstList[i])
        {
            mMatInstList[i] = new MatInstance(*pCopy->mMatInstList[i]->getMaterial());
        }
        else
        {
            mMatInstList[i] = NULL;
        }
    }

    clearMappedMaterials();
    mMappedMaterials.setSize(pCopy->mMaterials.size());
    for (i = 0; i < mMappedMaterials.size(); i++)
    {
        if (i < pCopy->mMappedMaterials.size() && pCopy->mMappedMaterials[i])
        {
            mMappedMaterials[i] = SimObjectPtr<Material>(pCopy->mMappedMaterials[i]);
        }
        else
        {
            mMappedMaterials[i] = NULL;
        }
    }
}



MaterialList::MaterialList(U32 materialCount, const char** materialNames)
{
    VECTOR_SET_ASSOCIATION(mMaterialNames);
    VECTOR_SET_ASSOCIATION(mMaterials);

    set(materialCount, materialNames);
}


//--------------------------------------
void MaterialList::set(U32 materialCount, const char** materialNames)
{
    free();
    mMaterials.setSize(materialCount);
    mMaterialNames.setSize(materialCount);
    clearMatInstList();
    mMatInstList.setSize(materialCount);
    for (U32 i = 0; i < materialCount; i++)
    {
        // vectors DO NOT initialize classes so manually call the constructor
        constructInPlace(&mMaterials[i]);
        mMaterialNames[i] = new char[dStrlen(materialNames[i]) + 1];
        dStrcpy(mMaterialNames[i], materialNames[i]);
        mMatInstList[i] = NULL;
    }
}


//--------------------------------------
MaterialList::~MaterialList()
{
    free();
}


//--------------------------------------
void MaterialList::load(U32 index, const char* path)
{
    AssertFatal(index < size(), "MaterialList:: index out of range.");
    if (index < size())
    {
        GFXTexHandle& handle = mMaterials[index];
        if (handle.isNull())
        {
            const char* name = mMaterialNames[index];
            if (name && *name)
            {
                if (path) {
                    char buffer[512];
                    dSprintf(buffer, sizeof(buffer), "%s/%s", path, name);
                    handle.set(buffer, &GFXDefaultStaticDiffuseProfile);
                }
                else
                    handle.set(name, &GFXDefaultStaticDiffuseProfile);


                if (!handle.isValid())
                {
                    bool displayMaterialErrors = Con::getBoolVariable("$pref::Material::DisplayMaterialErrors");

                    MatInstance* warningMat = gRenderInstManager.getWarningMat();
                    if (warningMat != NULL)
                    {
                        StringTableEntry* texName = warningMat->getMaterial()->baseTexFilename;
                        if (texName != NULL)
                        {
                            if (displayMaterialErrors)
                                Con::warnf("MaterialList::load: failed to load '%s', using Warning Material.", name);
                            handle.set(*texName, &GFXDefaultStaticDiffuseProfile);
                        }
                        else {
                            if (displayMaterialErrors)
                                Con::warnf("MaterialList::load: failed to load '%s' and Warning Material was not found...using blank texture.", name);
                            handle.set(new GBitmap(1, 1), &GFXDefaultStaticDiffuseProfile, false);
                        }
                    }
                    else {
                        if (displayMaterialErrors)
                            Con::warnf("MaterialList::load: failed to load '%s' and Warning Material was not found...using blank texture.", name);
                        handle.set(new GBitmap(1, 1), &GFXDefaultStaticDiffuseProfile, false);
                        return;
                    }

                }
            }
        }
    }
}


//--------------------------------------
bool MaterialList::load(const char* path)
{
    AssertFatal(mMaterials.size() == mMaterials.size(), "MaterialList::load: internal vectors out of sync.");

    for (S32 i = 0; i < mMaterials.size(); i++)
        load(i, path);

    for (S32 i = 0; i < mMaterials.size(); i++)
    {
        // TSMaterialList nulls out the names of IFL materials, so
        // we need to ignore empty names.
        const char* name = mMaterialNames[i];
        if (name && *name && !mMaterials[i])
        {
            //Con::warnf("MaterialList::load - failed to load material: %s", name);
            return false;
        }
    }
    return true;
}


//--------------------------------------
void MaterialList::unload()
{
    AssertFatal(mMaterials.size() == mMaterials.size(), "MaterialList::unload: internal vectors out of sync.");
    for (S32 i = 0; i < mMaterials.size(); i++)
        mMaterials[i].~GFXTexHandle();
}


//--------------------------------------
void MaterialList::free()
{
    AssertFatal(mMaterials.size() == mMaterialNames.size(), "MaterialList::free: internal vectors out of sync.");
    for (S32 i = 0; i < mMaterials.size(); i++)
    {
        if (mMaterialNames[i])
            delete[] mMaterialNames[i];
        //   mMaterials[i].~GFXTexHandle();
    }
    clearMatInstList();
    mMatInstList.setSize(0);
    mMappedMaterials.setSize(0);
    mMaterialNames.setSize(0);
    mMaterials.setSize(0);
}


//--------------------------------------
U32 MaterialList::push_back(GFXTexHandle textureHandle, const char* filename)
{
    mMaterials.increment();
    mMaterialNames.increment();
    mMappedMaterials.increment();

    // vectors DO NOT initialize classes so manually call the constructor
    constructInPlace(&mMaterials.last());
    constructInPlace(&mMappedMaterials.last());
    mMaterials.last() = textureHandle;
    mMaterialNames.last() = new char[dStrlen(filename) + 1];
    dStrcpy(mMaterialNames.last(), filename);

    // return the index
    return mMaterials.size() - 1;
}

//--------------------------------------
U32 MaterialList::push_back(const char* filename)
{
    mMaterials.increment();
    mMaterialNames.increment();
    mMappedMaterials.increment();

    // vectors DO NOT initialize classes so manually call the constructor
    constructInPlace(&mMaterials.last());
    constructInPlace(&mMappedMaterials.last());
    mMaterialNames.last() = new char[dStrlen(filename) + 1];
    dStrcpy(mMaterialNames.last(), filename);

    // return the index
    return mMaterials.size() - 1;
}

//--------------------------------------
bool MaterialList::read(Stream& stream)
{
    free();

    // check the stream version
    U8 version;
    if (stream.read(&version) && version != BINARY_FILE_VERSION)
        return readText(stream, version);

    // how many materials?
    U32 count;
    if (!stream.read(&count))
        return false;

    // pre-size the vectors for efficiency
    mMaterials.reserve(count);
    mMaterialNames.reserve(count);
    mMappedMaterials.reserve(count);

    // read in the materials
    for (U32 i = 0; i < count; i++)
    {
        // Load the bitmap name
        char buffer[256];
        stream.readString(buffer);
        if (!buffer[0])
        {
            AssertWarn(0, "MaterialList::read: error reading stream");
            return false;
        }

        // Material paths are a legacy of Tribes tools,
        // strip them off...
        char* name = &buffer[dStrlen(buffer)];
        while (name != buffer && name[-1] != '/' && name[-1] != '\\')
            name--;

        // Add it to the list
        mMaterials.increment();
        mMaterialNames.increment();
        mMappedMaterials.increment();
        // vectors DO NOT initialize classes so manually call the constructor
        constructInPlace(&mMaterials.last());
        constructInPlace(&mMappedMaterials.last());
        mMaterialNames.last() = new char[dStrlen(name) + 1];
        dStrcpy(mMaterialNames.last(), name);
    }

    return (stream.getStatus() == Stream::Ok);
}


//--------------------------------------
bool MaterialList::write(Stream& stream)
{
    AssertFatal(mMaterials.size() == mMaterialNames.size(), "MaterialList::write: internal vectors out of sync.");

    stream.write((U8)BINARY_FILE_VERSION);    // version
    stream.write((U32)mMaterials.size());     // material count

    for (S32 i = 0; i < mMaterials.size(); i++)  // material names
        stream.writeString(mMaterialNames[i]);

    return (stream.getStatus() == Stream::Ok);
}


//--------------------------------------
bool MaterialList::readText(Stream& stream, U8 firstByte)
{
    free();

    if (!firstByte)
        return (stream.getStatus() == Stream::Ok || stream.getStatus() == Stream::EOS);

    char buf[1024];
    buf[0] = firstByte;
    U32 offset = 1;

    for (;;)
    {
        stream.readLine((U8*)(buf + offset), sizeof(buf) - offset);
        if (!buf[0])
            break;
        offset = 0;

        // Material paths are a legacy of Tribes tools,
        // strip them off...
        char* name = &buf[dStrlen(buf)];
        while (name != buf && name[-1] != '/' && name[-1] != '\\')
            name--;

        // Add it to the list
        mMaterials.increment();
        mMaterialNames.increment();
        mMappedMaterials.increment();
        // vectors DO NOT initialize classes so manually call the constructor
        constructInPlace(&mMaterials.last());
        constructInPlace(&mMappedMaterials.last());
        mMaterialNames.last() = new char[dStrlen(name) + 1];
        dStrcpy(mMaterialNames.last(), name);
    }
    return (stream.getStatus() == Stream::Ok || stream.getStatus() == Stream::EOS);
}

bool MaterialList::readText(Stream& stream)
{
    U8 firstByte;
    stream.read(&firstByte);
    return readText(stream, firstByte);
}

//--------------------------------------
bool MaterialList::writeText(Stream& stream)
{
    AssertFatal(mMaterials.size() == mMaterialNames.size(), "MaterialList::writeText: internal vectors out of sync.");

    for (S32 i = 0; i < mMaterials.size(); i++)
        stream.writeLine((U8*)mMaterialNames[i]);
    stream.writeLine((U8*)"");

    return (stream.getStatus() == Stream::Ok);
}


//--------------------------------------
ResourceInstance* constructMaterialList(Stream& stream)
{
    MaterialList* matList = new MaterialList;
    if (matList->readText(stream))
        return matList;
    else
    {
        delete matList;
        return NULL;
    }
}

//--------------------------------------------------------------------------
// Clear all materials in the mMatInstList member variable
//--------------------------------------------------------------------------
void MaterialList::clearMatInstList()
{
    // clear out old materials.  any non null element of the list should be pointing at deletable memory,
    // although multiple indexes may be pointing at the same memory so we have to be careful (see
    // comment in loop body)
    for (U32 i = 0; i < mMatInstList.size(); i++)
    {
        if (mMatInstList[i])
        {
            MatInstance* current = mMatInstList[i];
            delete current;
            mMatInstList[i] = NULL;

            // ok, since ts material lists can remap difference indexes to the same object (e.g. for ifls),
            // we need to make sure that we don't delete the same memory twice.  walk the rest of the list
            // and null out any pointers that match the one we deleted.
            for (U32 j = 0; j < mMatInstList.size(); j++)
                if (mMatInstList[j] == current)
                    mMatInstList[j] = NULL;
        }
    }
}

void MaterialList::clearMappedMaterials()
{
    // clear out old materials.  any non null element of the list should be pointing at deletable memory,
    // although multiple indexes may be pointing at the same memory so we have to be careful (see
    // comment in loop body)
    for (U32 i = 0; i < mMappedMaterials.size(); i++)
    {
        if (mMappedMaterials[i])
        {
            SimObjectPtr<Material> current = mMappedMaterials[i];
            delete current;
            mMappedMaterials[i] = NULL;

            // ok, since ts material lists can remap difference indexes to the same object (e.g. for ifls),
            // we need to make sure that we don't delete the same memory twice.  walk the rest of the list
            // and null out any pointers that match the one we deleted.
            for (U32 j = 0; j < mMappedMaterials.size(); j++)
                if (mMappedMaterials[j] == current)
                    mMappedMaterials[j] = NULL;
        }
    }
}

//--------------------------------------------------------------------------
// Map materials - map materials to the textures in the list
//--------------------------------------------------------------------------
void MaterialList::mapMaterials(const char* path)
{

    MaterialPropertyMap* matMap = MaterialPropertyMap::get();
    if (!matMap) return;

    clearMatInstList();

    mMatInstList.setSize(mMaterials.size());

    for (U32 i = 0; i < mMaterials.size(); i++)
    {
        // lookup a material property entry
        const char* matName = getMaterialName(i);

        // JMQ: this code assumes that all materials have names, but ifls don't (they are nuked by tsmateriallist loader code).  what to do?
        if (!matName)
        {
            mMatInstList[i] = NULL;
            continue;
        }

        const MaterialPropertyMap::MapEntry* entry = matMap->getMapEntry(matName);

        if (entry && entry->materialName)
        {
            Material* mat = dynamic_cast<Material*>(Sim::findObject(entry->materialName));
            if (mat)
            {
                MatInstance* matInst = new MatInstance(*mat);
                mMatInstList[i] = matInst;
            }
            else
                mMatInstList[i] = NULL;
        }
        else
        {
#ifdef CREATE_MISSING_MATERIALS
            Con::warnf("MaterialList::mapMaterials: unmapped material %s, generating new material", matName);

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
            dSprintf(buffer3, sizeof(buffer3), "GeneratedMaterial%d", newMatIndex);
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
#else
            mMatInstList[i] = NULL;
#endif
        }
    }
}

Material* MaterialList::getMappedMaterial(U32 index)
{
    Material* mat = mMappedMaterials[index];
    if (!mat)
    {
        MaterialPropertyMap* map = MaterialPropertyMap::get();
        if (!map)
            return NULL;

        const MaterialPropertyMap::MapEntry* entry = map->getMapEntry(getMaterialName(index));
        if (entry)
            mat = (Material*)Sim::findObject(entry->materialName);

        mMappedMaterials[index] = mat;
    }
    return mat;
}

//--------------------------------------------------------------------------
// Get material instance
//--------------------------------------------------------------------------
MatInstance* MaterialList::getMaterialInst(U32 texIndex)
{
    if (mMatInstList.size() == 0)
    {
        return NULL;
    }

    return mMatInstList[texIndex];
}

//--------------------------------------------------------------------------
// Set material instance
//--------------------------------------------------------------------------
void MaterialList::setMaterialInst(MatInstance* matInst, U32 texIndex)
{
    if (texIndex >= mMatInstList.size())
    {
        AssertFatal(false, "Index out of range");
    }
    mMatInstList[texIndex] = matInst;
}














