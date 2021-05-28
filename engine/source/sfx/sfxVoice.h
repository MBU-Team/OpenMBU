//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXVOICE_H_
#define _SFXVOICE_H_

#ifndef _MPOINT_H_
   #include "math/mPoint.h"
#endif
#ifndef _MMATH_H_
   #include "math/mMatrix.h"
#endif
#ifndef _REFBASE_H_
   #include "core/refBase.h"
#endif
#ifndef _SFXSTATUS_H_
   #include "sfx/sfxStatus.h"
#endif

class SFXBuffer;


/// The voice interface provides for playback of
/// sound buffers and positioning of 3D sounds.
class SFXVoice : public RefBase
{
   protected:

      explicit SFXVoice() {}

   public:

      /// The destructor.
      virtual ~SFXVoice() {}

      /// Sets the playback sample position.
      virtual void setPosition( U32 pos ) = 0;

      ///
      virtual SFXStatus getStatus() const = 0;

      /// Starts playback from the current position.
      virtual void play( bool looping ) = 0;

      /// Stops playback and moves the position to the start.
      virtual void stop() = 0;

      /// Pauses playback.
      virtual void pause() = 0;

      /// Sets the position and orientation for a 3d voice.
      virtual void setTransform( const MatrixF& transform ) = 0;

      /// Sets the velocity for a 3d voice.
      virtual void setVelocity( const VectorF& velocity ) = 0;

      /// Sets the minimum and maximum distances for 3d falloff.
      virtual void setMinMaxDistance( F32 min, F32 max ) = 0;

      /// Sets the volume.
      virtual void setVolume( F32 volume ) = 0;

      /// Sets the pitch scale.
      virtual void setPitch( F32 pitch ) = 0;
};


#endif // _SFXVOICE_H_