#include "LevelModelInstance.h"
#include "LevelModelResource.h"
#include "LevelModel.h"

#include "sim/netConnection.h"
#include "core/bitStream.h"
#include "math/mathIO.h"

IMPLEMENT_CO_NETOBJECT_V1(LevelModelInstance);

LevelModelInstance::LevelModelInstance()
{
    mModelFileName = nullptr;
    mTypeMask = StaticObjectType | StaticRenderedObjectType | ShadowCasterObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);
}

LevelModelInstance::~LevelModelInstance()
{

}

void LevelModelInstance::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Media");
    addField("modelFile", TypeFilename, Offset(mModelFileName, LevelModelInstance), "The model file to use.");
    endGroup("Media");
}

bool LevelModelInstance::onAdd()
{
    mModelResource = ResourceManager->load(mModelFileName, true);
    if (mModelResource.isNull())
    {
        Con::errorf("LevelModelInstance::onAdd - Failed to load model resource '%s'.", mModelFileName);
        NetConnection::setLastError("Unable to load model: %s", mModelFileName);
        return false;
    }

    if (!isClientObject())
        mCRC = mModelResource.getCRC();

    if (isClientObject() || gSPMode)
    {
        if (mCRC != mModelResource.getCRC())
        {
            Con::errorf("LevelModelInstance::onAdd - Local Model resource '%s' does not match the version on the server.", mModelFileName);
            NetConnection::setLastError("Local Model resource '%s' does not match the version on the server.", mModelFileName);
            return false;
        }

        for (U32 i = 0; i < mModelResource->getNumLevelModels(); i++)
        {
            LevelModel *model = mModelResource->getLevelModel(i);
            if (!model->prepForRendering(mModelResource.getFilePath()))
            {
                if (mServerObject.isNull())
                    return false;
            }
        }
    }

    if (!Parent::onAdd())
        return false;

    // Setup bounding information
    mObjBox = mModelResource->getLevelModel(0)->getBoundingBox();
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    addToScene();
}

void LevelModelInstance::onRemove()
{
    removeFromScene();
    Parent::onRemove();
}

bool LevelModelInstance::prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState)
{
    LevelModel* model = mModelResource->getLevelModel(0);

    if (!isHidden())
    {
        GFX->pushWorldMatrix();
        model->prepBatchRender(this, state);
        GFX->popWorldMatrix();
    }

    return false;
}

U32 LevelModelInstance::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
    U32 retMask = Parent::packUpdate(conn, mask, stream);

    if (stream->writeFlag((mask & InitMask) != 0)) {
        stream->write(mCRC);
        stream->writeString(mModelFileName);

        mathWrite(*stream, mObjToWorld);
        mathWrite(*stream, mObjScale);
    } else {
        if (stream->writeFlag((mask & TransformMask) != 0)) {
            mathWrite(*stream, mObjToWorld);
            mathWrite(*stream, mObjScale);
        }
    }

    return retMask;
}

void LevelModelInstance::unpackUpdate(NetConnection *conn, BitStream *stream)
{
    Parent::unpackUpdate(conn, stream);

    MatrixF temp;
    Point3F tempScale;

    if (stream->readFlag())
    {
        stream->read(&mCRC);
        mModelFileName = stream->readSTString();

        mathRead(*stream, &temp);
        mathRead(*stream, &tempScale);
        setScale(tempScale);
        setTransform(temp);
    } else {
        if (stream->readFlag()) {
            mathRead(*stream, &temp);
            mathRead(*stream, &tempScale);
            setScale(tempScale);
            setTransform(temp);
        }
    }
}
