//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/null/sfxNullVoice.h"

#include "sfx/null/sfxNullDevice.h"


SFXNullVoice::SFXNullVoice() 
   : mStatus( SFXStatusNull )
{
}

SFXNullVoice::~SFXNullVoice()
{

}

void SFXNullVoice::setPosition( U32 pos )
{

}

void SFXNullVoice::setMinMaxDistance( F32 min, F32 max )
{

}


void SFXNullVoice::play( bool looping )
{
   mStatus = SFXStatusPlaying;
}

void SFXNullVoice::pause()
{
   mStatus = SFXStatusPaused;
}

void SFXNullVoice::stop()
{
   mStatus = SFXStatusStopped;
}

SFXStatus SFXNullVoice::getStatus() const
{
   return mStatus;
}

void SFXNullVoice::setVelocity( const VectorF& velocity )
{
}

void SFXNullVoice::setTransform( const MatrixF& transform )
{
}

void SFXNullVoice::setVolume( F32 volume )
{
}

void SFXNullVoice::setPitch( F32 pitch )
{ 
}

