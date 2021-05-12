//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXNULLVOICE_H_
#define _SFXNULLVOICE_H_

#ifndef _SFXVOICE_H_
   #include "sfx/sfxVoice.h"
#endif
#ifndef _SFXSTATUS_H_
   #include "sfx/sfxStatus.h"
#endif


class SFXNullVoice : public SFXVoice
{
   friend class SFXNullDevice;

   protected:

      SFXNullVoice();

      SFXStatus mStatus;

   public:

      virtual ~SFXNullVoice();

      void setPosition( U32 pos );

      void setMinMaxDistance( F32 min, F32 max );

      SFXStatus getStatus() const;

      void play( bool looping );

      void pause();

      void stop();

      void setVelocity( const VectorF& velocity );

      void setTransform( const MatrixF& transform );

      void setVolume( F32 volume );

      void setPitch( F32 pitch );
};


/// A vector of SFXNullBuffer pointers.
typedef Vector<SFXNullVoice*> SFXNullVoiceVector;

#endif // _SFXNULLVOICE_H_