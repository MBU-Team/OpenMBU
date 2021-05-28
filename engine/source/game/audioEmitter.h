#if 0
//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AUDIOEMITTER_H_
#define _AUDIOEMITTER_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _AUDIODATABLOCK_H_
#include "audio/audioDataBlock.h"
#endif
#ifndef _NETCONNECTION_H_
#include "sim/netConnection.h"
#endif

//---------------------------------------------------------------------------
class AudioEmitter : public SceneObject
{
    friend class AmbientAudioManager;
    friend class AudioEmitterToEvent;

private:
    typedef SceneObject              Parent;

    AUDIOHANDLE                      mAudioHandle;
    U32                              mAudioProfileId;
    U32                              mAudioDescriptionId;

    // field data
    AudioProfile* mAudioProfile;
    AudioDescription* mAudioDescription;
    StringTableEntry                 mFilename;

    bool                             mUseProfileDescription;
    static Audio::Description        smDefaultDescription;
    Audio::Description               mDescription;
    bool                             mOutsideAmbient;
    S32                              mLoopCount;
    U32                              mEventID;

    bool                             mOwnedByClient;

    enum Dirty {
        Profile = BIT(0),
        Description = BIT(1),
        Filename = BIT(2),
        UseProfileDescription = BIT(3),
        Volume = BIT(4),
        IsLooping = BIT(5),
        Is3D = BIT(6),
        ReferenceDistance = BIT(7),
        MaxDistance = BIT(8),
        ConeInsideAngle = BIT(9),
        ConeOutsideAngle = BIT(10),
        ConeOutsideVolume = BIT(11),
        ConeVector = BIT(12),
        LoopCount = BIT(13),
        MinLoopGap = BIT(14),
        MaxLoopGap = BIT(15),
        Transform = BIT(16),
        AudioType = BIT(17),
        OutsideAmbient = BIT(18),

        AllDirtyMask = BIT(19) - 1,

        SourceMask = (Profile | Description | Filename),
        LoopingMask = (LoopCount | MinLoopGap | MaxLoopGap),
        Is3DMask = (ReferenceDistance | MaxDistance | ConeInsideAngle | ConeOutsideAngle | ConeOutsideVolume | ConeVector),
        UseProfileDescriptionMask = (Volume | LoopingMask | Is3DMask | IsLooping | Is3D | AudioType),
    };
    BitSet32                   mDirty;
    bool readDirtyFlag(BitStream*, U32);

public:

    AudioEmitter(bool client = false);

    void processLoopEvent();
    bool update();

    enum UpdateMasks {
        InitialUpdateMask = BIT(0),
        TransformUpdateMask = BIT(1),
        DirtyUpdateMask = BIT(2),
    };

    void packData(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackData(NetConnection* conn, BitStream* stream);

    // SceneObject
    void setTransform(const MatrixF& mat);
    void setScale(const VectorF& scale);

    void inspectPreApply();
    void inspectPostApply();

    bool prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
    void renderObject(SceneState* state);

    // NetObject
    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    // SimObject
    bool onAdd();
    void onRemove();

    static void initPersistFields();

    DECLARE_CONOBJECT(AudioEmitter);
};

////---------------------------------------------------------------------------
//class ClientAudioEmitter : public AudioEmitter
//{
//   private:
//      typedef AudioEmitter    Parent;
//
//   public:
//      U32 mEventCount;
//      ClientAudioEmitter();
//      bool onAdd();
//
//      static void initPersistFields();
//      DECLARE_CONOBJECT(ClientAudioEmitter);
//};
//
////---------------------------------------------------------------------------
//class AudioEmitterToEvent : public NetEvent
//{
//   private:
//      SimObjectPtr<ClientAudioEmitter>  mEmitter;
//
//   public:
//      AudioEmitterToEvent(ClientAudioEmitter * emitter = 0);
//
//      void notifyDelivered(NetConnection *, bool);
//      void write(NetConnection *, BitStream *);
//      void pack(NetConnection *, BitStream *);
//      void unpack(NetConnection *, BitStream *);
//      void process(NetConnection *) {}
//
//      DECLARE_CONOBJECT(AudioEmitterToEvent);
//};

#endif
#endif