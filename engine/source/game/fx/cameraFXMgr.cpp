//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/fx/cameraFXMgr.h"

#include "math/mRandom.h"
#include "math/mMatrix.h"

// global cam fx
CameraFXManager gCamFXMgr;


//**************************************************************************
// Camera effect
//**************************************************************************
CameraFX::CameraFX()
{
    mElapsedTime = 0.0;
    mDuration = 1.0;
}

//--------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------
void CameraFX::update(F32 dt)
{
    mElapsedTime += dt;
}





//**************************************************************************
// Camera shake effect
//**************************************************************************
CameraShake::CameraShake()
{
    mFreq.zero();
    mAmp.zero();
    mStartAmp.zero();
    mTimeOffset.zero();
    mCamFXTrans.identity();
    mFalloff = 10.0;
}

//--------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------
void CameraShake::update(F32 dt)
{
    Parent::update(dt);

    fadeAmplitude();

    VectorF camOffset;
    camOffset.x = mAmp.x * sin(M_2PI * (mTimeOffset.x + mElapsedTime) * mFreq.x);
    camOffset.y = mAmp.y * sin(M_2PI * (mTimeOffset.y + mElapsedTime) * mFreq.y);
    camOffset.z = mAmp.z * sin(M_2PI * (mTimeOffset.z + mElapsedTime) * mFreq.z);

    VectorF rotAngles;
    rotAngles.x = camOffset.x * 10.0 * M_PI / 180.0;
    rotAngles.y = camOffset.y * 10.0 * M_PI / 180.0;
    rotAngles.z = camOffset.z * 10.0 * M_PI / 180.0;
    MatrixF rotMatrix(EulerF(rotAngles.x, rotAngles.y, rotAngles.z));

    mCamFXTrans = rotMatrix;
    mCamFXTrans.setPosition(camOffset);
}

//--------------------------------------------------------------------------
// Fade out the amplitude over time
//--------------------------------------------------------------------------
void CameraShake::fadeAmplitude()
{
    F32 percentDone = (mElapsedTime / mDuration);
    if (percentDone > 1.0) percentDone = 1.0;

    F32 time = 1 + percentDone * mFalloff;
    time = 1 / (time * time);

    mAmp = mStartAmp * time;
}

//--------------------------------------------------------------------------
// Initialize
//--------------------------------------------------------------------------
void CameraShake::init()
{
    mTimeOffset.x = 0.0;
    mTimeOffset.y = gRandGen.randF();
    mTimeOffset.z = gRandGen.randF();
}

//**************************************************************************
// CameraFXManager
//**************************************************************************
CameraFXManager::CameraFXManager()
{
    mCamFXTrans.identity();
}

//--------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
CameraFXManager::~CameraFXManager()
{
    clear();
}

//--------------------------------------------------------------------------
// Add new effect to currently running list
//--------------------------------------------------------------------------
void CameraFXManager::addFX(CameraFX* newFX)
{
    mFXList.link(newFX);
}

//--------------------------------------------------------------------------
// Clear all currently running camera effects
//--------------------------------------------------------------------------
void CameraFXManager::clear()
{
    mFXList.free();
}

//--------------------------------------------------------------------------
// Update camera effects
//--------------------------------------------------------------------------
void CameraFXManager::update(F32 dt)
{
    CameraFXPtr* cur = NULL;
    mCamFXTrans.identity();

    for (cur = mFXList.next(cur); cur; cur = mFXList.next(cur))
    {
        CameraFX* curFX = *cur;
        curFX->update(dt);
        MatrixF fxTrans = curFX->getTrans();

        mCamFXTrans.mul(fxTrans);

        if (curFX->isExpired())
        {
            CameraFXPtr* prev = mFXList.prev(cur);
            mFXList.free(cur);
            cur = prev;
        }
    }
}
