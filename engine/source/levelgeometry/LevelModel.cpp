#include "LevelModel.h"
#include "LevelModelResource.h"
#include "console/console.h"

const U32 LevelModel::smFileVersion = 0;
const U32 LevelModel::smIdentifier = 'MDLX';

LevelModel::LevelModel()
{

}

LevelModel::~LevelModel()
{

}

bool LevelModel::read(Stream &stream)
{
    AssertFatal(stream.hasCapability(Stream::StreamRead), "LevelModel::read: non-read capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModel::read: Error, stream in inconsistent state");

    U32 identifier;
    stream.read(&identifier);
    if (identifier != smIdentifier)
    {
        Con::errorf("LevelModel::read: Error, invalid file format.");
        return false;
    }

    U32 fileVersion;
    stream.read(&fileVersion);
    if (fileVersion > smFileVersion)
    {
        Con::errorf("LevelModel::read - File version is too new.");
        return false;
    }

    // TODO: Read the model data from the stream.

    return stream.getStatus() == Stream::Ok;
}

bool LevelModel::write(Stream &stream) const
{
    AssertFatal(stream.hasCapability(Stream::StreamWrite), "LevelModel::write: non-write capable stream passed");
    AssertFatal(stream.getStatus() == Stream::Ok, "LevelModel::write: Error, stream in inconsistent state");

    stream.write(smIdentifier);
    stream.write(smFileVersion);

    // TODO: Write the model data to the stream.

    return stream.getStatus() == Stream::Ok;
}

bool LevelModel::prepForRendering(const char* path)
{
    // TODO: Prepare the model for rendering.
    return true;
}

Box3F LevelModel::getBoundingBox() const
{
    // TODO: Return the bounding box of the model.
    return Box3F(0, 0, 0, 1, 1, 1);
}
