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

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState) override;

protected:
    bool onAdd() override;
    void onRemove() override;

private:
    StringTableEntry mModelFileName;
    Resource<LevelModelResource> mModelResource;

    U32 mCRC;

    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
    void unpackUpdate(NetConnection* conn, BitStream* stream) override;

    enum UpdateMaskBits
    {
        InitMask = BIT(0),
        TransformMask = BIT(1),
    };
};

#endif // _LEVEL_GEOMETRY_INSTANCE_H