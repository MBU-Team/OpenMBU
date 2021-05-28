//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FLYINGVEHICLE_H_
#define _FLYINGVEHICLE_H_

#ifndef _VEHICLE_H_
#include "game/vehicles/vehicle.h"
#endif

#ifndef _CLIPPEDPOLYLIST_H_
#include "collision/clippedPolyList.h"
#endif

class ParticleEmitter;
class ParticleEmitterData;


//----------------------------------------------------------------------------

struct FlyingVehicleData : public VehicleData {
    typedef VehicleData Parent;

    enum Sounds {
        JetSound,
        EngineSound,
        MaxSounds,
    };
    SFXProfile* sound[MaxSounds];

    enum Jets {
        // These enums index into a static name list.
        ForwardJetEmitter,      // Thrust forward
        BackwardJetEmitter,     // Thrust backward
        DownwardJetEmitter,     // Thrust down
        TrailEmitter,           // Contrail
        MaxJetEmitters,
    };
    ParticleEmitterData* jetEmitter[MaxJetEmitters];
    F32 minTrailSpeed;

    //
    F32 maneuveringForce;
    F32 horizontalSurfaceForce;
    F32 verticalSurfaceForce;
    F32 autoInputDamping;
    F32 steeringForce;
    F32 steeringRollForce;
    F32 rollForce;
    F32 autoAngularForce;
    F32 rotationalDrag;
    F32 maxAutoSpeed;
    F32 autoLinearForce;
    F32 hoverHeight;
    F32 createHoverHeight;

    F32 vertThrustMultiple;

    // Initialized in preload
    ClippedPolyList rigidBody;
    S32 surfaceCount;
    F32 maxSpeed;

    enum JetNodes {
        // These enums index into a static name list.
        ForwardJetNode,
        ForwardJetNode1,
        BackwardJetNode,
        BackwardJetNode1,
        DownwardJetNode,
        DownwardJetNode1,
        //
        TrailNode,
        TrailNode1,
        TrailNode2,
        TrailNode3,
        //
        MaxJetNodes,
        MaxDirectionJets = 2,
        ThrustJetStart = ForwardJetNode,
        NumThrustJets = TrailNode,
        MaxTrails = 4,
    };
    static const char* sJetNode[MaxJetNodes];
    S32 jetNode[MaxJetNodes];

    //
    FlyingVehicleData();
    DECLARE_CONOBJECT(FlyingVehicleData);
    static void initPersistFields();
    bool preload(bool server, char errorBuffer[256]);
    void packData(BitStream* stream);
    void unpackData(BitStream* stream);
};


//----------------------------------------------------------------------------

class FlyingVehicle : public Vehicle
{
    typedef Vehicle Parent;

    FlyingVehicleData* mDataBlock;

    SFXSource* mJetSound;
    SFXSource* mEngineSound;

    enum NetMaskBits {
        InitMask = BIT(0),
        HoverHeight = BIT(1)
    };
    bool createHeightOn;
    F32 mCeilingFactor;

    enum ThrustDirection {
        // Enums index into sJetActivationTable
        ThrustForward,
        ThrustBackward,
        ThrustDown,
        NumThrustDirections,
        NumThrustBits = 3
    };
    Point2F mThrust;
    ThrustDirection mThrustDirection;

    // Jet Threads
    enum Jets {
        // These enums index into a static name list.
        BackActivate,
        BackMaintain,
        BottomActivate,
        BottomMaintain,
        JetAnimCount
    };
    static const char* sJetSequence[FlyingVehicle::JetAnimCount];
    TSThread* mJetThread[JetAnimCount];
    S32 mJetSeq[JetAnimCount];
    bool mBackMaintainOn;
    bool mBottomMaintainOn;
    // Jet Particles
    struct JetActivation {
        // Convert thrust direction into nodes & emitters
        S32 node;
        S32 emitter;
    };
    static JetActivation sJetActivation[NumThrustDirections];
    SimObjectPtr<ParticleEmitter> mJetEmitter[FlyingVehicleData::MaxJetNodes];

    //
    bool onNewDataBlock(GameBaseData* dptr);
    void updateMove(const Move* move);
    void updateForces(F32);
    //   bool collideBody(const MatrixF& mat,Collision* info);
    F32 getHeight();

    // Client sounds & particles
    void updateJet(F32 dt);
    void updateEngineSound(F32 level);
    void updateEmitter(bool active, F32 dt, ParticleEmitterData* emitter, S32 idx, S32 count);

    U32 getCollisionMask();
public:
    DECLARE_CONOBJECT(FlyingVehicle);
    static void initPersistFields();

    FlyingVehicle();
    ~FlyingVehicle();

    bool onAdd();
    void onRemove();
    void advanceTime(F32 dt);

    void writePacketData(GameConnection* conn, BitStream* stream);
    void readPacketData(GameConnection* conn, BitStream* stream);
    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
    void useCreateHeight(bool val);
};


#endif
