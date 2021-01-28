//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_

#pragma pack(push,8)
#include <MAX.H>
#include <dummy.h>
#include <iparamm.h>
#pragma pack(pop)

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#define SEQUENCE_CLASS_ID    Class_ID(0x63a64197, 0xfd72a55)

#define PB_SEQ_BEGIN_END                      0

#define PB_SEQ_CYCLIC                         1
#define PB_SEQ_BLEND                          2

#define PB_SEQ_LAST_FIRST_FRAME_SAME          3 // only valid if cyclic

#define PB_SEQ_USE_FRAME_RATE                 4 // if this
#define PB_SEQ_FRAME_RATE                     5 // then this
#define PB_SEQ_NUM_FRAMES                     6 // else this

#define PB_SEQ_IGNORE_GROUND_TRANSFORM        7
#define PB_SEQ_USE_GROUND_FRAME_RATE          8 // if this
#define PB_SEQ_GROUND_FRAME_RATE              9 // then use this
#define PB_SEQ_NUM_GROUND_FRAMES              10// else use this

#define PB_SEQ_ENABLE_MORPH_ANIMATION         11
#define PB_SEQ_ENABLE_VIS_ANIMATION           12
#define PB_SEQ_ENABLE_TRANSFORM_ANIMATION     13

#define PB_SEQ_FORCE_MORPH_ANIMATION          14
#define PB_SEQ_FORCE_VIS_ANIMATION            15
#define PB_SEQ_FORCE_TRANSFORM_ANIMATION      16

#define PB_SEQ_DEFAULT_PRIORITY               17

#define PB_SEQ_BLEND_REFERENCE_TIME           18

#define PB_SEQ_ENABLE_TEXTURE_ANIMATION       19
#define PB_SEQ_ENABLE_IFL_ANIMATION           20
#define PB_SEQ_FORCE_TEXTURE_ANIMATION        21

#define PB_SEQ_TRIGGERS                       22

#define PB_SEQ_ENABLE_DECAL_ANIMATION         23
#define PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION   24
#define PB_SEQ_FORCE_DECAL_ANIMATION          25

#define PB_SEQ_OVERRIDE_DURATION              26
#define PB_SEQ_DURATION                       27

#define PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION   28
#define PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION 29
#define PB_SEQ_FORCE_SCALE_ANIMATION            30

// Class ID for old sequence class -- id for exporter used
// in starsiege, tribes 1, and other games around that time
// You know, games from the previous millenium.
#define OLD_SEQUENCE_CLASS_ID    Class_ID(0x09923023,0)

// param block constants for old sequence class
#define PB_OLD_SEQ_ONESHOT       3
#define PB_OLD_SEQ_FORCEVIS      4
#define PB_OLD_SEQ_VISONLY       5
#define PB_OLD_SEQ_NOCOLLAPSE    6
#define PB_OLD_SEQ_USECELRATE    7

extern ClassDesc * GetSequenceDesc();

extern bool  getBoolSequenceDefault(S32 index);
extern F32   getFloatSequenceDefault(S32 index);
extern S32   getIntSequenceDefault(S32 index);
extern void  resetSequenceDefaults();


class SequenceObject: public DummyObject
{
    public:

    static U32   defaultCyclic;
    static U32   defaultBlend;
    static U32   defaultFirstLastFrameSame;

    static U32   defaultUseFrameRate;
    static F32   defaultFrameRate;
    static S32   defaultNumFrames;

    static U32   defaultIgnoreGroundTransform;
    static U32   defaultUseGroundFrameRate;
    static F32   defaultGroundFrameRate;
    static S32   defaultNumGroundFrames;

    static U32   defaultEnableMorphAnimation;
    static U32   defaultEnableVisAnimation;
    static U32   defaultEnableTransformAnimation;

    static U32   defaultForceMorphAnimation;
    static U32   defaultForceVisAnimation;
    static U32   defaultForceTransformAnimation;

    static S32   defaultDefaultSequencePriority;

    static S32   defaultBlendReferenceTime;

    static U32   defaultEnableTextureAnimation;
    static U32   defaultEnableIflAnimation;
    static U32   defaultForceTextureAnimation;

    static U32   defaultEnableDecalAnimation;
    static U32   defaultEnableDecalFrameAnimation;
    static U32   defaultForceDecalAnimation;

    static U32   defaultOverrideDuration;
    static F32   defaultDuration;

    static U32   defaultEnableUniformScaleAnimation;
    static U32   defaultEnableArbitraryScaleAnimation;
    static U32   defaultForceScaleAnimation;

    // Class vars
    IParamBlock *pblock;
    Interval ivalid;

    //
    static IParamMap *pmapParam1;
    static IParamMap *pmapParam2;
    static IParamMap *pmapParam3;
    static SequenceObject *editOb;

    SequenceObject();
    ~SequenceObject();

    //  inherited virtual methods for Reference-management
    RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget,
                                PartID& partID,     RefMessage message );

    // From BaseObject
    void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
    void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
    RefTargetHandle Clone(RemapDir& remap = NoRemap());

    // From Object
    virtual TCHAR *GetObjectName() { return _T("Sequence II"); }
    void InitNodeName(TSTR& s);
    Interval ObjectValidity(TimeValue t);
    // unimplemented methods from Object class:
    //    ObjectState Eval(TimeValue time);
    //    ObjectHandle ApplyTransform(Matrix3& matrix) {return this;}
    //    Interval ObjectValidity(TimeValue t) {return FOREVER;}
    //    S32 CanConvertToType(Class_ID obtype) {return FALSE;}
    //    Object* ConvertToType(TimeValue t, Class_ID obtype) {assert(0);return NULL;}
    //    void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
    //    void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
    //    S32 DoOwnSelectHilite() { return 1; }

    // Animatable methods
    void FreeCaches();
    S32 NumSubs() { return 1; }
    Animatable* SubAnim(S32 i) { return pblock; }
    TSTR SubAnimName(S32 i);
    void DeleteThis() { delete this; }
    Class_ID ClassID() { return SEQUENCE_CLASS_ID; }
    void GetClassName(TSTR& s);
    //    S32 IsKeyable(){ return 0;}

    // From ref
    S32 NumRefs() {return 1;}
    RefTargetHandle GetReference(S32 i) {return pblock;}
    void SetReference(S32 i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}
    IOResult Load(ILoad *iload);
    //IOResult Save(ISave *isave);
    //RefTargetHandle Clone(RemapDir& remap = NoRemap());

    // where do these come from?
    virtual void InvalidateUI();
    IParamArray *GetParamBlock();
    S32 GetParamBlockIndex(S32 id);
    virtual ParamDimension *GetParameterDim(S32 pbIndex);
    virtual TSTR GetParameterName(S32 pbIndex);
};

#endif
