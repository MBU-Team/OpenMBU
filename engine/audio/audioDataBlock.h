//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _AUDIODATABLOCK_H_
#define _AUDIODATABLOCK_H_

#ifndef _PLATFORMAUDIO_H_
#include "platform/platformAudio.h"
#endif
#ifndef _AUDIOBUFFER_H_
#include "audio/audioBuffer.h"
#endif
#ifndef _BITSTREAM_H_
#include "core/bitStream.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

//--------------------------------------------------------------------------
class AudioEnvironment : public SimDataBlock
{
   typedef SimDataBlock Parent;

   public:

      bool  mUseRoom;
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

      AudioEnvironment();

      static void initPersistFields();
      void packData(BitStream* stream);
      void unpackData(BitStream* stream);

      DECLARE_CONOBJECT(AudioEnvironment);
};
DECLARE_CONSOLETYPE(AudioEnvironment)

//--------------------------------------------------------------------------
class AudioSampleEnvironment : public SimDataBlock
{
   typedef SimDataBlock Parent;

   public:

      S32      mDirect;
      S32      mDirectHF;
      S32      mRoom;
      S32      mRoomHF;
      F32      mObstruction;
      F32      mObstructionLFRatio;
      F32      mOcclusion;
      F32      mOcclusionLFRatio;
      F32      mOcclusionRoomRatio;
      F32      mRoomRolloff;
      F32      mAirAbsorption;
      S32      mOutsideVolumeHF;
      S32      mFlags;

      AudioSampleEnvironment();
      static void initPersistFields();

      void packData(BitStream* stream);
      void unpackData(BitStream* stream);

      DECLARE_CONOBJECT(AudioSampleEnvironment);
};
DECLARE_CONSOLETYPE(AudioSampleEnvironment)

//--------------------------------------------------------------------------
class AudioDescription: public SimDataBlock
{
private:
    typedef SimDataBlock Parent;

public:
   // field info
   Audio::Description mDescription;

   AudioDescription();
   DECLARE_CONOBJECT(AudioDescription);
   static void initPersistFields();
   virtual bool onAdd();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);

   const Audio::Description* getDescription() const { return &mDescription; }
};
DECLARE_CONSOLETYPE(AudioDescription)

//----------------------------------------------------------------------------
class AudioProfile: public SimDataBlock
{
private:
   typedef SimDataBlock Parent;

   Resource<AudioBuffer> mBuffer;

public:
   // field info
   AudioDescription       *mDescriptionObject;
   AudioSampleEnvironment *mSampleEnvironment;

   StringTableEntry mFilename;
   bool             mPreload;

   AudioProfile();
   DECLARE_CONOBJECT(AudioProfile);
   static void initPersistFields();

   virtual bool onAdd();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
   virtual bool preload(bool server, char errorBuffer[256]);

   const Audio::Description* getDescription() const { return mDescriptionObject ? mDescriptionObject->getDescription() : NULL; }
   bool isPreload() { return mPreload; }
};
DECLARE_CONSOLETYPE(AudioProfile)

#endif  // _H_AUDIODATABLOCK_
