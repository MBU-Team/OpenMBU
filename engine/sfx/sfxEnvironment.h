//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXENVIRONMENT_H_
#define _SFXENVIRONMENT_H_

#ifndef _CONSOLETYPES_H_
   #include "console/consoleTypes.h"
#endif
#ifndef _SIMDATABLOCK_H_
   #include "console/simBase.h"
#endif


/// Warning: This class is non-functional and is only here
/// to allow InteriorInstance to compile.
///
/// Eventualy this functionaliy will resurface in a
/// way that will play well with multiple providers
/// which may have unique features.
///
class SFXEnvironment : public SimDataBlock
{
   private:

      typedef SimDataBlock Parent;

   public:

      /*
      //bool  mUseRoom;
      S32   mRoom;
      S32   mRoomHF;
      S32   mReflections;
      S32   mReverb;
      F32   mRoomRolloffFactor;
      F32   mDecayTime;
      F32   mDecayHFRatio;
      F32   mReflectionsDelay;
      F32   mReverbDelay;
      S32   mRoomVolume;
      F32   mEffectVolume;
      F32   mDamping;
      F32   mEnvironmentSize;
      F32   mEnvironmentDiffusion;
      F32   mAirAbsorption;
      S32   mFlags;
      */

      SFXEnvironment();
      DECLARE_CONOBJECT( SFXEnvironment );
      static void initPersistFields();

      virtual void packData( BitStream* stream );
      virtual void unpackData( BitStream* stream );

};

DECLARE_CONSOLETYPE( SFXEnvironment )


#endif // _SFXENVIRONMENT_H_