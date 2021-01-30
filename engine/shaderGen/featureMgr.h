//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _FEATUREMGR_H_
#define _FEATUREMGR_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif

#include "core/tVector.h"
#include "shaderFeature.h"


//**************************************************************************
// Feature Manager
//**************************************************************************
class FeatureMgr
{

    Vector <ShaderFeature*> mFeatures;
    Vector <ShaderFeature*> mAuxFeatures;  // auxiliary features used internally

    void  init();
    void  shutdown();


public:
    enum Constants
    {
        NumAuxFeatures = 1,
    };

    FeatureMgr();
    ~FeatureMgr();

    ShaderFeature* get(U32 index);
    ShaderFeature* getAux(U32 index);


};

extern FeatureMgr gFeatureMgr;


#endif // FEATUREMGR