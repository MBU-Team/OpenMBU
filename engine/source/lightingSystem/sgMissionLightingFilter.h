//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGMISSIONLIGHTINGFILTER_H_
#define _SGMISSIONLIGHTINGFILTER_H_

#include "game/gameBase.h"

/**
 */
class sgMissionLightingFilterData : public GameBaseData
{
    typedef GameBaseData Parent;

public:
    F32 sgLightingIntensity;
    ColorF sgLightingFilter;
    bool sgCinematicFilter;
    F32 sgCinematicFilterAmount;
    F32 sgCinematicFilterReferenceIntensity;
    ColorF sgCinematicFilterReferenceColor;

    sgMissionLightingFilterData()
    {
        sgCinematicFilter = false;
        sgLightingIntensity = 1.0;
        sgLightingFilter = ColorF(1.0, 1.0, 1.0);
        sgCinematicFilterAmount = 1.0;
        sgCinematicFilterReferenceIntensity = 1.0;
        sgCinematicFilterReferenceColor = ColorF(1.0, 1.0, 1.0);
    }
    static void  initPersistFields();
    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);

    DECLARE_CONOBJECT(sgMissionLightingFilterData);
};

/**
 */
class sgMissionLightingFilter : public GameBase
{
    typedef GameBase Parent;
    sgMissionLightingFilterData* mDataBlock;
    bool onNewDataBlock(GameBaseData* dptr)
    {
        mDataBlock = dynamic_cast<sgMissionLightingFilterData*>(dptr);
        return Parent::onNewDataBlock(dptr);
    }

public:
    sgMissionLightingFilter()
    {
        mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
        mNetFlags.set(Ghostable | ScopeAlways);
        mDataBlock = NULL;
    }

    static void initPersistFields();
    virtual void inspectPostApply();
    bool onAdd();
    void onRemove();
    U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* con, BitStream* stream);

    DECLARE_CONOBJECT(sgMissionLightingFilter);
};

#endif//_SGMISSIONLIGHTINGFILTER_H_
