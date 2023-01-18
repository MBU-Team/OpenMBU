#include "LevelModelResource.h"
#include "LevelModel.h"
#include "console/console.h"

const U32 LevelModelResource::smFileVersion = 0;
const U32 LevelModelResource::smIdentifier = 'LDML'; // LMDL

LevelModelResource::LevelModelResource()
{
    VECTOR_SET_ASSOCIATION(mLevelModels);
}

LevelModelResource::~LevelModelResource()
{
    U32 i;

    for (i = 0; i < mLevelModels.size(); i++)
        delete mLevelModels[i];
}

bool LevelModelResource::read(Stream &stream)
{
    AssertFatal(stream.hasCapability(Stream::StreamRead), "LevelModelResource::read: non-read capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModelResource::read: Error, stream in inconsistent state");

    U32 identifier;
    stream.read(&identifier);
    if (identifier != smIdentifier)
    {
        Con::errorf("LevelModelResource::read: Error, invalid file format.");
        return false;
    }

    U32 fileVersion;
    stream.read(&fileVersion);
    if (fileVersion > smFileVersion)
    {
        Con::printf("LevelModelResource::read: file version is too new");
        return false;
    }

    U32 numLevelModels;
    stream.read(&numLevelModels);
    mLevelModels.setSize(numLevelModels);
    for (U32 i = 0; i < numLevelModels; i++)
    {
        mLevelModels[i] = new LevelModel;
        if (!mLevelModels[i]->read(stream))
        {
            Con::printf("LevelModelResource::read: error reading level model");
            return false;
        }
    }

    return stream.getStatus() == Stream::Ok;
}

bool LevelModelResource::write(Stream &stream) const
{
    AssertFatal(stream.hasCapability(Stream::StreamWrite), "LevelModelResource::write: non-write capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModelResource::write: Error, stream in inconsistent state");

    stream.write(smIdentifier);
    stream.write(smFileVersion);

    stream.write(mLevelModels.size());
    for (U32 i = 0; i < mLevelModels.size(); i++)
    {
        if (!mLevelModels[i]->write(stream))
        {
            AssertISV(false, "Unable to write level model to stream");
            return false;
        }
    }

    return stream.getStatus() == Stream::Ok;
}

//------------------------------------------------------------------------------
//-------------------------------------- Level Model Resource constructor
ResourceInstance* constructLevelModelLMD(Stream& stream)
{
    LevelModelResource* pResource = new LevelModelResource;

    if (pResource->read(stream) == true)
        return pResource;
    else {
        delete pResource;
        return NULL;
    }
}
