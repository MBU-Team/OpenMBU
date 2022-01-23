#ifndef _LEVEL_GEOMETRY_RESURCE_H_
#define _LEVEL_GEOMETRY_RESURCE_H_

#include "core/resManager.h"
#include "math/mBox.h"
#include "LevelModel.h"

class LevelModelResource : public ResourceInstance
{
    typedef ResourceInstance Parent;
    static const U32 smIdentifier;
    static const U32 smFileVersion;

protected:
    Vector<LevelModel*> mLevelModels;

public:
    LevelModelResource();
    ~LevelModelResource();

    bool read(Stream &stream);
    bool write(Stream &stream) const;

    U32 getNumLevelModels() { return mLevelModels.size(); }
    LevelModel* getLevelModel(const U32 index);
};

extern ResourceInstance* constructLevelModelLMD(Stream& stream);

inline LevelModel *LevelModelResource::getLevelModel(const U32 index)
{
    AssertFatal(index < getNumLevelModels(), "LevelModelResource::getLevelModel: index out of range");
    return mLevelModels[index];
}

#endif // _LEVEL_GEOMETRY_RESURCE_H_