#ifndef _LEVEL_GEOMETRY_INSTANCE_H
#define _LEVEL_GEOMETRY_INSTANCE_H

#include "sim/sceneObject.h"
#include "LevelModelResource.h"

class LevelModelInstance : public SceneObject
{
   typedef SceneObject Parent;

public:
    LevelModelInstance();
    ~LevelModelInstance();

    DECLARE_CONOBJECT(LevelModelInstance);
    static void initPersistFields();

protected:
    bool onAdd() override;
    void onRemove() override;

private:
    StringTableEntry mModelFileName;
    Resource<LevelModelResource> mModelResource;

    U32 mCRC;
};

#endif // _LEVEL_GEOMETRY_INSTANCE_H