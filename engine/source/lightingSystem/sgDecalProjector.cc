//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright � Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#include "lightingSystem/sgLighting.h"
#include "lightingSystem/sgDecalProjector.h"
#include "sim/decalManager.h"
#include "sim/netConnection.h"
#include "sceneGraph/sceneState.h"
#include "gfx/gfxDevice.h"
#include "gfx/primBuilder.h"
#include "renderInstance/renderInstMgr.h"

extern bool gEditingMission;


class sgDecalProjector : public GameBase
{
    typedef GameBase Parent;
    DecalData* mDataBlock;
    bool onNewDataBlock(GameBaseData* dptr);
protected:
    bool sgInitNeeded;
    bool sgProjection;
    Point3F sgProjectionPoint;
    Point3F sgProjectionNormal;
    bool onAdd();
    void onRemove();
    void sgResetProjection();
    void sgProject();
public:
    sgDecalProjector();
    void inspectPostApply();
    void setTransform(const MatrixF& mat);
    bool prepRenderImage(SceneState* state, const U32 stateKey,
        const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state, RenderInst* ri);
    U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* con, BitStream* stream);
    DECLARE_CONOBJECT(sgDecalProjector);
};

IMPLEMENT_CO_NETOBJECT_V1(sgDecalProjector);

sgDecalProjector::sgDecalProjector()
{
    mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);
    mDataBlock = NULL;
    sgProjection = false;
    sgProjectionPoint = Point3F(0.0f, 0.0f, 0.0f);
    sgProjectionNormal = Point3F(0.0f, 0.0f, 0.0f);

    // for after load relinking...
    sgInitNeeded = true;
}

bool sgDecalProjector::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mObjBox.min.set(-0.5, -0.5, -0.5);
    mObjBox.max.set(0.5, 0.5, 0.5);
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    addToScene();
    return true;
}

void sgDecalProjector::onRemove()
{
    if (isClientObject())
        gDecalManager->ageDecal(getId());

    removeFromScene();
    Parent::onRemove();
}

bool sgDecalProjector::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<DecalData*>(dptr);
    return Parent::onNewDataBlock(dptr);
}

bool sgDecalProjector::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 startZone, const bool modifyBaseZoneState)
{
    if (!gEditingMission)
        return false;
    if (isLastState(state, stateKey))
        return false;

    setLastState(state, stateKey);

    if (state->isObjectRendered(this))
    {
        RenderInst* ri = gRenderInstManager.allocInst();
        ri->obj = this;
        ri->state = state;
        ri->type = RenderInstManager::RIT_Object;
        gRenderInstManager.addInst(ri);

        /*SceneRenderImage* image = new SceneRenderImage;
        image->obj = this;
        image->isTranslucent = true;
        image->sortType = SceneRenderImage::EndSort;
        state->insertRenderImage(image);*/
    }

    return false;
}

void sgDecalProjector::renderObject(SceneState* state, RenderInst* ri)
{
    if (gEditingMission)
    {
        // render the directional line...
        GFX->setAlphaBlendEnable(true);
        GFX->setZEnable(true);
        GFX->setSrcBlend(GFXBlendOne);
        GFX->setDestBlend(GFXBlendZero);
        GFX->setTextureStageColorOp(0, GFXTOPDisable);
        GFX->disableShaders();

        Point3F vector = SG_STATIC_SPOT_VECTOR_NORMALIZED * 10.0f;
        Point3F origin = Point3F(0, 0, 0);
        Point3F point = vector;
        mObjToWorld.mulP(origin);
        mObjToWorld.mulP(point);
        PrimBuild::color3f(0, 1, 0);
        PrimBuild::begin(GFXLineStrip, 2);
        PrimBuild::vertex3fv(origin);
        PrimBuild::vertex3fv(point);
        PrimBuild::end();
    }
}

void sgDecalProjector::sgResetProjection()
{
    sgProjection = false;
    sgProjectionPoint = Point3F(0.0f, 0.0f, 0.0f);
    sgProjectionNormal = Point3F(0.0f, 0.0f, 0.0f);

    if (isClientObject())
        return;

    Point3F pos = getPosition();
    Point3F normal = SG_STATIC_SPOT_VECTOR_NORMALIZED * 100.0f;
    getTransform().mulV(normal);

    RayInfo info;
    if (!gServerContainer.castRay(pos, (pos + normal),
        InteriorObjectType | TerrainObjectType, &info))
    {
        Con::errorf("Error in _sgDropDecal: no drop object found.");
        return;
    }

    sgProjection = true;
    sgProjectionPoint = info.point;
    sgProjectionNormal = info.normal;

    setMaskBits(0xffffffff);
}

void sgDecalProjector::sgProject()
{
    if ((isServerObject()) || (!mDataBlock))
        return;

    // use the instead of getId()...
    // id's are flaky in zones...
    // TODO: This WILL break in 64 bit builds...
    U32 ownerid = static_cast<U32>(reinterpret_cast<size_t>(this));

    gDecalManager->ageDecal(ownerid);

    if (!sgProjection)
        return;

    Point3F tandir;
    getTransform().getColumn(0, &tandir);

    gDecalManager->addDecal(sgProjectionPoint, tandir, sgProjectionNormal, getScale(), mDataBlock, ownerid);
}

void sgDecalProjector::setTransform(const MatrixF& mat)
{
    Parent::setTransform(mat);
    sgResetProjection();
    setMaskBits(0xffffffff);
}

U32 sgDecalProjector::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 res = Parent::packUpdate(con, mask, stream);

    if (sgInitNeeded && isServerObject())
    {
        sgInitNeeded = false;
        sgResetProjection();
    }

    stream->writeAffineTransform(mObjToWorld);

    if (stream->writeFlag(sgProjection))
    {
        // this is a joke right (no Point3F support)?!?
        stream->write(sgProjectionPoint.x);
        stream->write(sgProjectionPoint.y);
        stream->write(sgProjectionPoint.z);
        stream->write(sgProjectionNormal.x);
        stream->write(sgProjectionNormal.y);
        stream->write(sgProjectionNormal.z);
    }

    return res;
}

void sgDecalProjector::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    MatrixF ObjectMatrix;
    stream->readAffineTransform(&ObjectMatrix);
    setTransform(ObjectMatrix);

    sgProjection = stream->readFlag();
    if (sgProjection)
    {
        // this is a joke right (no Point3F support)?!?
        stream->read(&sgProjectionPoint.x);
        stream->read(&sgProjectionPoint.y);
        stream->read(&sgProjectionPoint.z);
        stream->read(&sgProjectionNormal.x);
        stream->read(&sgProjectionNormal.y);
        stream->read(&sgProjectionNormal.z);
    }

    sgProject();
}

void sgDecalProjector::inspectPostApply()
{
    Parent::inspectPostApply();
    sgResetProjection();
    setMaskBits(0xffffffff);
}

//-----------------------------------------------

ConsoleFunction(_sgCreateDecal, void, 6, 6, "(Point3F pos, Point3F tandir, Point3F norm, "
    "Point3F scale, decalDataBlock) - this method must be called on the client side!")
{
    // is this the client side?
    // doesn't work...
    //if(gDecalManager->isServerObject())
    //	return;

    Point3F pos = Point3F(0, 0, 0);
    Point3F tandir = Point3F(1, 0, 0);
    Point3F normal = Point3F(0, 0, 1);
    Point3F scale = Point3F(1, 1, 1);
    DecalData* decaldata = NULL;

    dSscanf(argv[1], "%f %f %f", &pos.x, &pos.y, &pos.z);
    dSscanf(argv[2], "%f %f %f", &tandir.x, &tandir.y, &tandir.z);
    dSscanf(argv[3], "%f %f %f", &normal.x, &normal.y, &normal.z);
    dSscanf(argv[4], "%f %f %f", &scale.x, &scale.y, &scale.z);

    decaldata = dynamic_cast<DecalData*>(Sim::findObject(argv[5]));

    if (!decaldata)
        return;

    gDecalManager->addDecal(pos, tandir, normal, scale, decaldata);
}

ConsoleFunction(_sgDropDecal, void, 6, 6, "(Point3F pos, Point3F tandir, Point3F norm, "
    "Point3F scale, decalDataBlock) - this method must be called on the client side! "
    "This method drops a decal onto the interior or "
    "terrain directly below the given position.")
{
    // is this the client side?
    // doesn't work...
    //if(!gDecalManager->isGhost())
    //	return;

    Point3F pos = Point3F(0, 0, 0);
    Point3F tandir = Point3F(1, 0, 0);
    Point3F normal = Point3F(0, 0, 1);
    Point3F scale = Point3F(1, 1, 1);
    DecalData* decaldata = NULL;

    dSscanf(argv[1], "%f %f %f", &pos.x, &pos.y, &pos.z);
    dSscanf(argv[2], "%f %f %f", &tandir.x, &tandir.y, &tandir.z);
    dSscanf(argv[3], "%f %f %f", &normal.x, &normal.y, &normal.z);
    dSscanf(argv[4], "%f %f %f", &scale.x, &scale.y, &scale.z);

    decaldata = dynamic_cast<DecalData*>(Sim::findObject(argv[5]));

    if (!decaldata)
        return;

    RayInfo info;
    if (!getCurrentClientContainer()->castRay(pos, (pos + Point3F(0, 0, -100)),
        InteriorObjectType | TerrainObjectType, &info))
    {
        Con::errorf("Error in _sgDropDecal: no drop object found.");
        return;
    }

    pos = info.point;
    normal = info.normal;

    gDecalManager->addDecal(pos, tandir, normal, scale, decaldata);
}

