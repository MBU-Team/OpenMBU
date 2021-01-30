//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PARTICLEEMITTERDUMMY_H_
#define _PARTICLEEMITTERDUMMY_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

class ParticleEmitterData;
class ParticleEmitter;

//*****************************************************************************
// ParticleEmitterNodeData
//*****************************************************************************
class ParticleEmitterNodeData : public GameBaseData
{
    typedef GameBaseData Parent;

protected:
    bool onAdd();

    //-------------------------------------- Console set variables
public:
    F32 timeMultiple;

    //-------------------------------------- load set variables
public:

public:
    ParticleEmitterNodeData();
    ~ParticleEmitterNodeData();

    void packData(BitStream*);
    void unpackData(BitStream*);
    bool preload(bool server, char errorBuffer[256]);

    DECLARE_CONOBJECT(ParticleEmitterNodeData);
    static void initPersistFields();
};


//*****************************************************************************
// ParticleEmitterNode
//*****************************************************************************
class ParticleEmitterNode : public GameBase
{
    typedef GameBase Parent;

private:
    ParticleEmitterNodeData* mDataBlock;

protected:
    bool onAdd();
    void onRemove();
    bool onNewDataBlock(GameBaseData* dptr);

    ParticleEmitterData* mEmitterDatablock;
    S32                  mEmitterDatablockId;

    ParticleEmitter* mEmitter;
    F32              mVelocity;

public:
    ParticleEmitterNode();
    ~ParticleEmitterNode();

    ParticleEmitter* getParticleEmitter() { return mEmitter; }

    // Time/Move Management
public:
    void advanceTime(F32 dt);

    DECLARE_CONOBJECT(ParticleEmitterNode);
    static void initPersistFields();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};

#endif // _H_PARTICLEEMISSIONDUMMY

