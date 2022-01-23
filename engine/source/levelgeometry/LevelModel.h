#ifndef _LEVEL_GEOMETRY_H_
#define _LEVEL_GEOMETRY_H_

#include "math/mBox.h"
#include "core/stream.h"

class LevelModel
{
public:
    LevelModel();
    ~LevelModel();

    bool read(Stream &stream);
    bool write(Stream &stream) const;

private:
    static const U32 smIdentifier;
    static const U32 smFileVersion;

public:
    bool prepForRendering(const char* path);
    Box3F getBoundingBox() const;
};

#endif // _LEVEL_GEOMETRY_H_