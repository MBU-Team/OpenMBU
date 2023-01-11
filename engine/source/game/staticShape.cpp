//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "core/bitStream.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "game/moveManager.h"
#include "ts/tsShapeInstance.h"
#include "core/resManager.h"
#include "game/staticShape.h"
#include "math/mathIO.h"
#include "game/shadow.h"
#include "lightingSystem/sgLighting.h"
#include "sim/netConnection.h"

extern void wireCube(F32 size, Point3F pos);

static const U32 sgAllowedDynamicTypes = 0xffffff;

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(StaticShapeData);

StaticShapeData::StaticShapeData()
{
    dynamicTypeField = 0;

    genericShadowLevel = StaticShape_GenericShadowLevel;
    noShadowLevel = StaticShape_NoShadowLevel;
    noIndividualDamage = false;

#ifdef MARBLE_BLAST
    scopeAlways = false;

    for (S32 i = 0; i < MAX_FORCE_TYPES; i++)
    {
        forceType[i] = NoForce;
        forceRadius[i] = 1.0f;
        forceStrength[i] = 1.0f;
        forceArc[i] = 0.7f;
        forceNode[i] = 0;
        forceVector[i].set(0.0f, 0.0f, 0.0f);
    }
#endif
}

static EnumTable::Enums enumForceStates[] =
{
   { StaticShapeData::ForceType::NoForce,        "NoForce"   },
   { StaticShapeData::ForceType::ForceSpherical, "Spherical" },
   { StaticShapeData::ForceType::ForceField,     "Field"     },
   { StaticShapeData::ForceType::ForceCone,      "Cone"      }
};
static EnumTable EnumForceState(4, &enumForceStates[0]);

void StaticShapeData::initPersistFields()
{
    Parent::initPersistFields();

    addField("noIndividualDamage", TypeBool, Offset(noIndividualDamage, StaticShapeData));
    addField("dynamicType", TypeS32, Offset(dynamicTypeField, StaticShapeData));

#ifdef MARBLE_BLAST
    addField("scopeAlways", TypeBool, Offset(scopeAlways, StaticShapeData));

    addField("forceType", TypeEnum, Offset(forceType, StaticShapeData), MAX_FORCE_TYPES, &EnumForceState);
    addField("forceNode", TypeS32, Offset(forceNode, StaticShapeData), MAX_FORCE_TYPES);
    addField("forceVector", TypePoint3F, Offset(forceVector, StaticShapeData), MAX_FORCE_TYPES);
    addField("forceRadius", TypeF32, Offset(forceRadius, StaticShapeData), MAX_FORCE_TYPES);
    addField("forceStrength", TypeF32, Offset(forceStrength, StaticShapeData), MAX_FORCE_TYPES);
    addField("forceArc", TypeF32, Offset(forceArc, StaticShapeData), MAX_FORCE_TYPES);
#endif
}

void StaticShapeData::packData(BitStream* stream)
{
    Parent::packData(stream);
    stream->writeFlag(noIndividualDamage);
    stream->write(dynamicTypeField);

    for (S32 i = 0; i < MAX_FORCE_TYPES; i++)
    {
        if (!stream->writeFlag(forceType[i] != NoForce))
            continue;

        stream->writeInt(forceType[i], 3);
        stream->writeInt(forceNode[i], 8);
        mathWrite(*stream, forceVector[i]);
        stream->write(forceRadius[i]);
        stream->write(forceStrength[i]);
        stream->write(forceArc[i]);
    }
}

void StaticShapeData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
    noIndividualDamage = stream->readFlag();
    stream->read(&dynamicTypeField);

    for (S32 i = 0; i < MAX_FORCE_TYPES; i++)
    {
        if (!stream->readFlag())
            continue;

        forceType[i] = (ForceType)stream->readInt(3);
        forceNode[i] = stream->readInt(8);
        mathRead(*stream, &forceVector[i]);
        stream->read(&forceRadius[i]);
        stream->read(&forceStrength[i]);
        stream->read(&forceArc[i]);
    }
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(StaticShape);

StaticShape::StaticShape()
{
    overrideOptions = false;

    mTypeMask |= StaticShapeObjectType | StaticObjectType;
    mDataBlock = 0;
}

StaticShape::~StaticShape()
{
}


//----------------------------------------------------------------------------

void StaticShape::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Lighting");
    addField("receiveSunLight", TypeBool, Offset(receiveSunLight, SceneObject));
    addField("receiveLMLighting", TypeBool, Offset(receiveLMLighting, SceneObject));
    //addField("useAdaptiveSelfIllumination", TypeBool, Offset(useAdaptiveSelfIllumination, SceneObject));
    addField("useCustomAmbientLighting", TypeBool, Offset(useCustomAmbientLighting, SceneObject));
    //addField("customAmbientSelfIllumination", TypeBool, Offset(customAmbientForSelfIllumination, SceneObject));
    addField("customAmbientLighting", TypeColorF, Offset(customAmbientLighting, SceneObject));
    addField("lightGroupName", TypeString, Offset(lightGroupName, SceneObject));
    endGroup("Lighting");
}

void StaticShape::inspectPostApply()
{
    if (isServerObject()) {
        setMaskBits(0xffffffff);
    }
}

bool StaticShape::onAdd()
{
    if (!Parent::onAdd() || !mDataBlock)
        return false;

    // We need to modify our type mask based on what our datablock says...
    mTypeMask |= (mDataBlock->dynamicTypeField & sgAllowedDynamicTypes);

    addToScene();

    if (isServerObject())
    {
#ifdef MARBLE_BLAST
        if (mDataBlock->scopeAlways)
            setScopeAlways();
#endif
        scriptOnAdd();
    }
    return true;
}

bool StaticShape::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<StaticShapeData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

#ifdef MARBLE_BLAST
    for (S32 i = 0; i < StaticShapeData::MAX_FORCE_TYPES; i++)
    {
        if (mDataBlock->forceType[i] != StaticShapeData::NoForce)
        {
            mTypeMask |= ForceObjectType;
            break;
        }
    }
#endif

    scriptOnNewDataBlock();
    return true;
}

void StaticShape::onRemove()
{
    scriptOnRemove();
    removeFromScene();
    Parent::onRemove();
}


//----------------------------------------------------------------------------

void StaticShape::processTick(const Move* move)
{
    Parent::processTick(move);

    // Image Triggers
    if (move && mDamageState == Enabled) {
        setImageTriggerState(0, move->trigger[0]);
        setImageTriggerState(1, move->trigger[1]);
    }

    if (isMounted()) {
        MatrixF mat;
        mMount.object->getMountTransform(mMount.node, &mat);
        Parent::setTransform(mat);
        Parent::setRenderTransform(mat);
    }
}

void StaticShape::interpolateTick(F32)
{
    if (isMounted()) {
        MatrixF mat;
        mMount.object->getRenderMountTransform(mMount.node, &mat);
        Parent::setRenderTransform(mat);
    }
}

void StaticShape::setTransform(const MatrixF& mat)
{
    Parent::setTransform(mat);
    setMaskBits(PositionMask);
}

void StaticShape::onUnmount(ShapeBase*, S32)
{
    // Make sure the client get's the final server pos.
    setMaskBits(PositionMask);
}


//----------------------------------------------------------------------------

U32 StaticShape::packUpdate(NetConnection* connection, U32 mask, BitStream* bstream)
{
    U32 retMask = Parent::packUpdate(connection, mask, bstream);
    if (bstream->writeFlag(mask & PositionMask)) {

        // Write the transform (do _not_ use writeAffineTransform.  Since this is a static
        //  object, the transform must be RIGHT THE *&)*$&^ ON or it will goof up the
        //  syncronization between the client and the server.
        mathWrite(*bstream, mObjToWorld);
        mathWrite(*bstream, mObjScale);
    }

    // powered?
    bstream->writeFlag(mPowered);

    // these fields are static, so try to avoid transmit
    if (bstream->writeFlag(mask & advancedStaticOptionsMask))
    {
        bstream->writeFlag(receiveSunLight);
        bstream->writeFlag(useAdaptiveSelfIllumination);
        bstream->writeFlag(useCustomAmbientLighting);
        bstream->writeFlag(customAmbientForSelfIllumination);
        bstream->write(customAmbientLighting);
        bstream->writeFlag(receiveLMLighting);
        if (isServerObject())
        {
            lightIds.clear();
            findLightGroup(connection);

            U32 maxcount = getMin(lightIds.size(), SG_TSSTATIC_MAX_LIGHTS);
            bstream->writeInt(maxcount, SG_TSSTATIC_MAX_LIGHT_SHIFT);
            for (U32 i = 0; i < maxcount; i++)
            {
                bstream->writeInt(lightIds[i], NetConnection::GhostIdBitSize);
            }
        }
        else
        {
            // recording demo...
            U32 maxcount = getMin(lightIds.size(), SG_TSSTATIC_MAX_LIGHTS);
            bstream->writeInt(maxcount, SG_TSSTATIC_MAX_LIGHT_SHIFT);
            for (U32 i = 0; i < maxcount; i++)
            {
                bstream->writeInt(lightIds[i], NetConnection::GhostIdBitSize);
            }
        }
    }

    return retMask;
}

void StaticShape::unpackUpdate(NetConnection* connection, BitStream* bstream)
{
    Parent::unpackUpdate(connection, bstream);
    if (bstream->readFlag()) {
        MatrixF mat;
        //      bstream->readAffineTransform(&mat);
        mathRead(*bstream, &mat);
        Parent::setTransform(mat);
        Parent::setRenderTransform(mat);

        VectorF scale;
        mathRead(*bstream, &scale);
        setScale(scale);
    }

    // powered?
    mPowered = bstream->readFlag();

    if (bstream->readFlag())
    {
        receiveSunLight = bstream->readFlag();
        useAdaptiveSelfIllumination = bstream->readFlag();
        useCustomAmbientLighting = bstream->readFlag();
        customAmbientForSelfIllumination = bstream->readFlag();
        bstream->read(&customAmbientLighting);
        receiveLMLighting = bstream->readFlag();

        U32 count = bstream->readInt(SG_TSSTATIC_MAX_LIGHT_SHIFT);
        lightIds.clear();
        for (U32 i = 0; i < count; i++)
        {
            S32 id = bstream->readInt(NetConnection::GhostIdBitSize);
            lightIds.push_back(id);
        }
    }
}

#ifdef MARBLE_BLAST
bool StaticShape::getForce(Point3F& pos, Point3F* force)
{
    bool retval = false;
    if (mDataBlock != NULL && (mTypeMask & ForceObjectType) != 0 && mPowered)
    {
        F32 strength = 0.0;
        F32 dot = 0.0;
        MatrixF node;
        Point3F nodeVec;
        for (int i = 0; i < 4; i++)
        {
            if (mDataBlock->forceType[i] == StaticShapeData::NoForce)
                continue;

            getMountTransform(mDataBlock->forceNode[i], &node);

            if (mDataBlock->forceVector[i].len() == 0.0f)
                node.getColumn(1, &nodeVec);
            else
                nodeVec = mDataBlock->forceVector[i];

            Point3F posVec = pos - node.getPosition();
            dot = posVec.len();

            if (mDataBlock->forceRadius[i] < dot)
                continue;

            StaticShapeData::ForceType forceType = mDataBlock->forceType[i];
            strength = (1.0f - dot / mDataBlock->forceRadius[i]) * mDataBlock->forceStrength[i];

            if (forceType == StaticShapeData::ForceSpherical)
            {
                dot = strength / dot;
                *force += posVec * dot;
                retval = true;
            }

            if (forceType == StaticShapeData::ForceField)
            {
                *force += nodeVec * strength;
                retval = true;
            }

            if (forceType == StaticShapeData::ForceCone)
            {

                posVec *= 1.0f / dot;

                F32 newDot = mDot(nodeVec, posVec);
                F32 arc = mDataBlock->forceArc[i];
                if (arc < newDot)
                {
                    *force += ((posVec * strength) * (newDot - arc)) / (1.0f - arc);
                    retval = true;
                }
            }
        }
    }
    return retval;
}
#endif


//----------------------------------------------------------------------------
ConsoleMethod(StaticShape, setPoweredState, void, 3, 3, "(bool isPowered)")
{
    if (!object->isServerObject())
        return;
    object->setPowered(dAtob(argv[2]));
}

ConsoleMethod(StaticShape, getPoweredState, bool, 2, 2, "")
{
    if (!object->isServerObject())
        return(false);
    return(object->isPowered());
}
