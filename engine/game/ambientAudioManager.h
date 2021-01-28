//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AMBIENTAUDIOMANAGER_H_
#define _AMBIENTAUDIOMANAGER_H_

#ifndef _AUDIOEMITTER_H_
#include "game/audioEmitter.h"
#endif

class InteriorInstance;

/// The AmbientAudioManager manages varying the properties of audio emitters
/// based on the player's position.
///
/// It not only provides a notion of "outside"-ness and "inside"-ness to Torque's
/// sound library, but it also varies sounds based on the powered status of interiors,
/// and plays the PowerUp/PowerDown sounds as needed.
///
/// AudioEmitters automatically add themselves to the AmbientAudioManager, see
/// AudioEmitter::onAdd() and AudioEmitter::onRemove(). update() is called in
/// clientProcess(), and gAmbientAudioManager stores the global reference to the
/// AmbientAudioManager.
class AmbientAudioManager
{

   private:
      F32                              mOutsideScale;    ///< 0:inside -> 1:outside
      Vector<AudioEmitter*>            mEmitters;
      SimObjectPtr<InteriorInstance>   mInteriorInstance;

      SimObjectPtr<AudioEnvironment>   mCurrentEnvironment;
      F32                              mEnvironmentScale;

      AUDIOHANDLE                      mInteriorAudioHandle;
      AUDIOHANDLE                      mPowerAudioHandle;
      bool                             mLastAlarmState;

      bool getOutsideScale(F32 *, InteriorInstance **);
      void updateEnvironment();
      void updateEmitter(AudioEmitter *);
      void stopInteriorAudio();

   public:
      SimObjectPtr<AudioProfile>       mPowerUpProfile;
      SimObjectPtr<AudioProfile>       mPowerDownProfile;

      AmbientAudioManager();

      void addEmitter(AudioEmitter*);
      void removeEmitter(AudioEmitter*);
      void update();
};

extern AmbientAudioManager gAmbientAudioManager;

#endif
