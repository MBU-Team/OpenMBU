//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _HOVERVEHICLE_H_
#define _HOVERVEHICLE_H_

#ifndef _VEHICLE_H_
#include "game/vehicles/vehicle.h"
#endif

class ParticleEmitter;
class ParticleEmitterData;

// -------------------------------------------------------------------------
class HoverVehicleData : public VehicleData
{
    typedef VehicleData Parent;

protected:
    bool onAdd();

    //-------------------------------------- Console set variables
public:
    enum Sounds {
        JetSound,
        EngineSound,
        FloatSound,
        MaxSounds
    };
    SFXProfile* sound[MaxSounds];

    enum Jets {
        // These enums index into a static name list.
        ForwardJetEmitter,      // Thrust forward
        BackwardJetEmitter,     // Thrust backward
        DownwardJetEmitter,     // Thrust down
        MaxJetEmitters,
    };
    ParticleEmitterData* jetEmitter[MaxJetEmitters];

    enum JetNodes {
        // These enums index into a static name list.
        ForwardJetNode,
        ForwardJetNode1,
        BackwardJetNode,
        BackwardJetNode1,
        DownwardJetNode,
        DownwardJetNode1,
        //
        MaxJetNodes,
        MaxDirectionJets = 2,
        ThrustJetStart = ForwardJetNode,
        MaxTrails = 4,
    };
    static const char* sJetNode[MaxJetNodes];
    S32 jetNode[MaxJetNodes];


    F32 dragForce;
    F32 vertFactor;
    F32 floatingThrustFactor;

    F32 mainThrustForce;
    F32 reverseThrustForce;
    F32 strafeThrustForce;
    F32 turboFactor;

    F32 stabLenMin;
    F32 stabLenMax;
    F32 stabSpringConstant;
    F32 stabDampingConstant;

    F32 gyroDrag;
    F32 normalForce;
    F32 restorativeForce;
    F32 steeringForce;
    F32 rollForce;
    F32 pitchForce;

    F32 floatingGravMag;

    F32 brakingForce;
    F32 brakingActivationSpeed;

    ParticleEmitterData* dustTrailEmitter;
    S32                   dustTrailID;
    Point3F               dustTrailOffset;
    F32                   triggerTrailHeight;
    F32                   dustTrailFreqMod;

    //-------------------------------------- load set variables
public:
    F32 maxThrustSpeed;

public:
    HoverVehicleData();
    ~HoverVehicleData();

    void packData(BitStream*);
    void unpackData(BitStream*);
    bool preload(bool server, char errorBuffer[256]);

    DECLARE_CONOBJECT(HoverVehicleData);
    static void initPersistFields();
};


// -------------------------------------------------------------------------
class HoverVehicle : public Vehicle
{
    typedef Vehicle Parent;

private:
    HoverVehicleData* mDataBlock;
    ParticleEmitter* mDustTrailEmitter;

protected:
    bool onAdd();
    void onRemove();
    bool onNewDataBlock(GameBaseData* dptr);
    void updateDustTrail(F32 dt);

    // Vehicle overrides
protected:
    void updateMove(const Move* move);

    // Physics
protected:
    void updateForces(F32);
    F32 getBaseStabilizerLength() const;

    bool mFloating;
    F32  mThrustLevel;

    F32  mForwardThrust;
    F32  mReverseThrust;
    F32  mLeftThrust;
    F32  mRightThrust;

    SFXSource* mJetSound;
    SFXSource* mEngineSound;
    SFXSource* mFloatSound;

    enum ThrustDirection {
        // Enums index into sJetActivationTable
        ThrustForward,
        ThrustBackward,
        ThrustDown,
        NumThrustDirections,
        NumThrustBits = 3
    };
    ThrustDirection mThrustDirection;

    // Jet Threads
    enum Jets {
        // These enums index into a static name list.
        BackActivate,
        BackMaintain,
        JetAnimCount
    };
    static const char* sJetSequence[HoverVehicle::JetAnimCount];
    TSThread* mJetThread[JetAnimCount];
    S32 mJetSeq[JetAnimCount];
    bool mBackMaintainOn;

    // Jet Particles
    struct JetActivation {
        // Convert thrust direction into nodes & emitters
        S32 node;
        S32 emitter;
    };
    static JetActivation sJetActivation[NumThrustDirections];
    SimObjectPtr<ParticleEmitter> mJetEmitter[HoverVehicleData::MaxJetNodes];

    U32 getCollisionMask();
    void updateJet(F32 dt);
    void updateEmitter(bool active, F32 dt, ParticleEmitterData* emitter, S32 idx, S32 count);
public:
    HoverVehicle();
    ~HoverVehicle();

    // Time/Move Management
public:
    void advanceTime(F32 dt);

    DECLARE_CONOBJECT(HoverVehicle);
    //   static void initPersistFields();

    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);
};

#endif // _H_HOVERVEHICLE

