//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "audio/audioDataBlock.h"
#include "console/consoleTypes.h"
#include "platform/platformAL.h"
#include "sim/netConnection.h"

//--------------------------------------------------------------------------
namespace
{
   void writeRangedF32(BitStream * bstream, F32 val, F32 min, F32 max, U32 numBits)
   {
      val = (mClampF(val, min, max) - min) / (max - min);
      bstream->writeInt(val * ((1 << numBits) - 1), numBits);
   }

   F32 readRangedF32(BitStream * bstream, F32 min, F32 max, U32 numBits)
   {
      return(min + (F32(bstream->readInt(numBits)) / F32((1 << numBits) - 1)) * (max - min));
   }

   void writeRangedS32(BitStream * bstream, S32 val, S32 min, S32 max)
   {
      bstream->writeRangedU32((val - min), 0, (max - min));
   }

   S32 readRangedS32(BitStream * bstream, S32 min, S32 max)
   {
      return(bstream->readRangedU32(0, (max - min)) + min);
   }
}

//--------------------------------------------------------------------------
// Class AudioEnvironment:
//--------------------------------------------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(AudioEnvironment);

AudioEnvironment::AudioEnvironment()
{
   mUseRoom = true;
   mRoom = EAX_ENVIRONMENT_GENERIC;
   mRoomHF = 0;
   mReflections = 0;
   mReverb = 0;
   mRoomRolloffFactor = 0.1f;
   mDecayTime = 0.1f;
   mDecayHFRatio = 0.1f;
   mReflectionsDelay = 0.f;
   mReverbDelay = 0.f;
   mRoomVolume = 0;
   mEffectVolume = 0.f;
   mDamping = 0.f;
   mEnvironmentSize = 10.f;
   mEnvironmentDiffusion = 1.f;
   mAirAbsorption = 0.f;
   mFlags = 0;
}

static EnumTable::Enums roomEnums[] =
{
   { EAX_ENVIRONMENT_GENERIC,           "GENERIC" },               // 0
   { EAX_ENVIRONMENT_PADDEDCELL,        "PADDEDCELL" },
   { EAX_ENVIRONMENT_ROOM,              "ROOM" },
   { EAX_ENVIRONMENT_BATHROOM,          "BATHROOM" },
   { EAX_ENVIRONMENT_LIVINGROOM,        "LIVINGROOM" },
   { EAX_ENVIRONMENT_STONEROOM,         "STONEROOM" },             // 5
   { EAX_ENVIRONMENT_AUDITORIUM,        "AUDITORIUM" },
   { EAX_ENVIRONMENT_CONCERTHALL,       "CONCERTHALL" },
   { EAX_ENVIRONMENT_CAVE,              "CAVE" },
   { EAX_ENVIRONMENT_ARENA,             "ARENA" },
   { EAX_ENVIRONMENT_HANGAR,            "HANGAR" },                // 10
   { EAX_ENVIRONMENT_CARPETEDHALLWAY,   "CARPETEDHALLWAY" },
   { EAX_ENVIRONMENT_HALLWAY,           "HALLWAY" },
   { EAX_ENVIRONMENT_STONECORRIDOR,     "STONECORRIDOR" },
   { EAX_ENVIRONMENT_ALLEY,             "ALLEY" },
   { EAX_ENVIRONMENT_FOREST,            "FOREST" },                // 15
   { EAX_ENVIRONMENT_CITY,              "CITY" },
   { EAX_ENVIRONMENT_MOUNTAINS,         "MOUNTAINS" },
   { EAX_ENVIRONMENT_QUARRY,            "QUARRY" },
   { EAX_ENVIRONMENT_PLAIN,             "PLAIN" },
   { EAX_ENVIRONMENT_PARKINGLOT,        "PARKINGLOT" },            // 20
   { EAX_ENVIRONMENT_SEWERPIPE,         "SEWERPIPE" },
   { EAX_ENVIRONMENT_UNDERWATER,        "UNDERWATER" },
   { EAX_ENVIRONMENT_DRUGGED,           "DRUGGED" },
   { EAX_ENVIRONMENT_DIZZY,             "DIZZY" },
   { EAX_ENVIRONMENT_PSYCHOTIC,         "PSYCHOTIC" }              // 25
};
static EnumTable gAudioEnvironmentRoomTypes(sizeof(roomEnums) / sizeof(roomEnums[0]), &roomEnums[0]);


//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(AudioEnvironment)
IMPLEMENT_GETDATATYPE(AudioEnvironment)
IMPLEMENT_SETDATATYPE(AudioEnvironment)

void AudioEnvironment::initPersistFields()
{
   Parent::initPersistFields();

   addField("useRoom",              TypeBool,   Offset(mUseRoom, AudioEnvironment));
   addField("room",                 TypeEnum,   Offset(mRoom, AudioEnvironment), 1, &gAudioEnvironmentRoomTypes);
   addField("roomHF",               TypeS32,    Offset(mRoomHF, AudioEnvironment));
   addField("reflections",          TypeS32,    Offset(mReflections, AudioEnvironment));
   addField("reverb",               TypeS32,    Offset(mReverb, AudioEnvironment));
   addField("roomRolloffFactor",    TypeF32,    Offset(mRoomRolloffFactor, AudioEnvironment));
   addField("decayTime",            TypeF32,    Offset(mDecayTime, AudioEnvironment));
   addField("decayHFRatio",         TypeF32,    Offset(mDecayHFRatio, AudioEnvironment));
   addField("reflectionsDelay",     TypeF32,    Offset(mReflectionsDelay, AudioEnvironment));
   addField("reverbDelay",          TypeF32,    Offset(mReverbDelay, AudioEnvironment));
   addField("roomVolume",           TypeS32,    Offset(mRoomVolume, AudioEnvironment));
   addField("effectVolume",         TypeF32,    Offset(mEffectVolume, AudioEnvironment));
   addField("damping",              TypeF32,    Offset(mDamping, AudioEnvironment));
   addField("environmentSize",      TypeF32,    Offset(mEnvironmentSize, AudioEnvironment));
   addField("environmentDiffusion", TypeF32,    Offset(mEnvironmentDiffusion, AudioEnvironment));
   addField("airAbsorption",        TypeF32,    Offset(mAirAbsorption, AudioEnvironment));
   addField("flags",                TypeS32,    Offset(mFlags, AudioEnvironment));
}


void AudioEnvironment::packData(BitStream* stream)
{
   Parent::packData(stream);
   if(stream->writeFlag(mUseRoom))
      stream->writeRangedU32(mRoom, EAX_ENVIRONMENT_GENERIC, EAX_ENVIRONMENT_COUNT);
   else
   {
      writeRangedS32(stream, mRoomHF, -10000, 0);
      writeRangedS32(stream, mReflections, -10000, 10000);
      writeRangedS32(stream, mReverb, -10000, 2000);
      writeRangedF32(stream, mRoomRolloffFactor, 0.1f, 10.f, 8);
      writeRangedF32(stream, mDecayTime, 0.1f, 20.f, 8);
      writeRangedF32(stream, mDecayHFRatio, 0.1f, 20.f, 8);
      writeRangedF32(stream, mReflectionsDelay, 0.f, 0.3f, 9);
      writeRangedF32(stream, mReverbDelay, 0.f, 0.1f, 7);
      writeRangedS32(stream, mRoomVolume, -10000, 0);
      writeRangedF32(stream, mEffectVolume, 0.f, 1.f, 8);
      writeRangedF32(stream, mDamping, 0.f, 2.f, 9);
      writeRangedF32(stream, mEnvironmentSize, 1.f, 100.f, 10);
      writeRangedF32(stream, mEnvironmentDiffusion, 0.f, 1.f, 8);
      writeRangedF32(stream, mAirAbsorption, -100.f, 0.f, 10);
      stream->writeInt(mFlags, 6);
   }
}

void AudioEnvironment::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   mUseRoom = stream->readFlag();
   if(mUseRoom)
      mRoom = stream->readRangedU32(EAX_ENVIRONMENT_GENERIC, EAX_ENVIRONMENT_COUNT);
   else
   {
      mRoomHF = readRangedS32(stream, -10000, 0);
      mReflections = readRangedS32(stream, -10000, 10000);
      mReverb = readRangedS32(stream, -10000, 2000);
      mRoomRolloffFactor = readRangedF32(stream, 0.1f, 10.f, 8);
      mDecayTime = readRangedF32(stream, 0.1f, 20.f, 8);
      mDecayHFRatio = readRangedF32(stream, 0.1f, 20.f, 8);
      mReflectionsDelay = readRangedF32(stream, 0.f, 0.3f, 9);
      mReverbDelay = readRangedF32(stream, 0.f, 0.1f, 7);
      mRoomVolume = readRangedS32(stream, -10000, 0);
      mEffectVolume = readRangedF32(stream, 0.f, 1.f, 8);
      mDamping = readRangedF32(stream, 0.f, 2.f, 9);
      mEnvironmentSize = readRangedF32(stream, 1.f, 100.f, 10);
      mEnvironmentDiffusion = readRangedF32(stream, 0.f, 1.f, 8);
      mAirAbsorption = readRangedF32(stream, -100.f, 0.f, 10);
      mFlags = stream->readInt(6);
   }
}

//--------------------------------------------------------------------------
// Class AudioEnvironmentProfile:
//--------------------------------------------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(AudioSampleEnvironment);

AudioSampleEnvironment::AudioSampleEnvironment()
{
   mDirect = 0;
   mDirectHF = 0;
   mRoom = 0;
   mRoomHF = 0;
   mObstruction = 0.f;
   mObstructionLFRatio = 0.f;
   mOcclusion = 0.f;
   mOcclusionLFRatio = 0.f;
   mOcclusionRoomRatio = 0.f;
   mRoomRolloff = 0.f;
   mAirAbsorption = 0.f;
   mOutsideVolumeHF = 0.f;
   mFlags = 0;
}

//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(AudioSampleEnvironment)
IMPLEMENT_GETDATATYPE(AudioSampleEnvironment)
IMPLEMENT_SETDATATYPE(AudioSampleEnvironment)

void AudioSampleEnvironment::initPersistFields()
{
   Parent::initPersistFields();

   addField("direct",              TypeS32,    Offset(mDirect, AudioSampleEnvironment));
   addField("directHF",            TypeS32,    Offset(mDirectHF, AudioSampleEnvironment));
   addField("room",                TypeS32,    Offset(mRoom, AudioSampleEnvironment));
   addField("obstruction",         TypeF32,    Offset(mObstruction, AudioSampleEnvironment));
   addField("obstructionLFRatio",  TypeF32,    Offset(mObstructionLFRatio, AudioSampleEnvironment));
   addField("occlusion",           TypeF32,    Offset(mOcclusion, AudioSampleEnvironment));
   addField("occlusionLFRatio",    TypeF32,    Offset(mOcclusionLFRatio, AudioSampleEnvironment));
   addField("occlusionRoomRatio",  TypeF32,    Offset(mOcclusionRoomRatio, AudioSampleEnvironment));
   addField("roomRolloff",         TypeF32,    Offset(mRoomRolloff, AudioSampleEnvironment));
   addField("airAbsorption",       TypeF32,    Offset(mAirAbsorption, AudioSampleEnvironment));
   addField("outsideVolumeHF",     TypeS32,    Offset(mOutsideVolumeHF, AudioSampleEnvironment));
   addField("flags",               TypeS32,    Offset(mFlags, AudioSampleEnvironment));
}

void AudioSampleEnvironment::packData(BitStream* stream)
{
   Parent::packData(stream);
   writeRangedS32(stream, mDirect, -10000, 1000);
   writeRangedS32(stream, mDirectHF, -10000, 0);
   writeRangedS32(stream, mRoom, -10000, 1000);
   writeRangedS32(stream, mRoomHF, -10000, 0);
   writeRangedF32(stream, mObstruction, 0.f, 1.f, 9);
   writeRangedF32(stream, mObstructionLFRatio, 0.f, 1.f, 8);
   writeRangedF32(stream, mOcclusion, 0.f, 1.f, 9);
   writeRangedF32(stream, mOcclusionLFRatio, 0.f, 1.f, 8);
   writeRangedF32(stream, mOcclusionRoomRatio, 0.f, 10.f, 9);
   writeRangedF32(stream, mRoomRolloff, 0.f, 10.f, 9);
   writeRangedF32(stream, mAirAbsorption, 0.f, 10.f, 9);
   writeRangedS32(stream, mOutsideVolumeHF, -10000, 0);
   stream->writeInt(mFlags, 3);
}

void AudioSampleEnvironment::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   mDirect = readRangedS32(stream, -10000, 1000);
   mDirectHF = readRangedS32(stream, -10000, 0);
   mRoom = readRangedS32(stream, -10000, 1000);
   mRoomHF = readRangedS32(stream, -10000, 0);
   mObstruction = readRangedF32(stream, 0.f, 1.f, 9);
   mObstructionLFRatio = readRangedF32(stream, 0.f, 1.f, 8);
   mOcclusion = readRangedF32(stream, 0.f, 1.f, 9);
   mOcclusionLFRatio = readRangedF32(stream, 0.f, 1.f, 8);
   mOcclusionRoomRatio = readRangedF32(stream, 0.f, 10.f, 9);
   mRoomRolloff = readRangedF32(stream, 0.f, 10.f, 9);
   mAirAbsorption = readRangedF32(stream, 0.f, 10.f, 9);
   mOutsideVolumeHF = readRangedS32(stream, -10000, 0);
   mFlags = stream->readInt(3);
}

//--------------------------------------------------------------------------
// Class AudioDescription:
//--------------------------------------------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(AudioDescription);

AudioDescription::AudioDescription()
{
   mDescription.mVolume             = 1.0f;
   mDescription.mIsLooping          = false;
   mDescription.mIsStreaming		= false;
   mDescription.mIs3D               = false;
   mDescription.mReferenceDistance  = 1.0f;
   mDescription.mMaxDistance        = 100.0f;
   mDescription.mConeInsideAngle    = 360;
   mDescription.mConeOutsideAngle   = 360;
   mDescription.mConeOutsideVolume  = 1.0f;
   mDescription.mConeVector.set(0, 0, 1);
   mDescription.mEnvironmentLevel   = 0.f;
   mDescription.mLoopCount          = -1;
   mDescription.mMinLoopGap         = 0;
   mDescription.mMaxLoopGap         = 0;
   mDescription.mType               = 0;
}

//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(AudioDescription)
IMPLEMENT_GETDATATYPE(AudioDescription)
IMPLEMENT_SETDATATYPE(AudioDescription)

void AudioDescription::initPersistFields()
{
   Parent::initPersistFields();

   addField("volume",            TypeF32,     Offset(mDescription.mVolume, AudioDescription));
   addField("isLooping",         TypeBool,    Offset(mDescription.mIsLooping, AudioDescription));
   addField("isStreaming",       TypeBool,    Offset(mDescription.mIsStreaming, AudioDescription));
   addField("is3D",              TypeBool,    Offset(mDescription.mIs3D, AudioDescription));
   addField("referenceDistance", TypeF32,     Offset(mDescription.mReferenceDistance, AudioDescription));
   addField("maxDistance",       TypeF32,     Offset(mDescription.mMaxDistance, AudioDescription));
   addField("coneInsideAngle",   TypeS32,     Offset(mDescription.mConeInsideAngle, AudioDescription));
   addField("coneOutsideAngle",  TypeS32,     Offset(mDescription.mConeOutsideAngle, AudioDescription));
   addField("coneOutsideVolume", TypeF32,     Offset(mDescription.mConeOutsideVolume, AudioDescription));
   addField("coneVector",        TypePoint3F, Offset(mDescription.mConeVector, AudioDescription));
   addField("environmentLevel",  TypeF32,     Offset(mDescription.mEnvironmentLevel, AudioDescription));
   addField("loopCount",         TypeS32,     Offset(mDescription.mLoopCount, AudioDescription));
   addField("minLoopGap",        TypeS32,     Offset(mDescription.mMinLoopGap, AudioDescription));
   addField("maxLoopGap",        TypeS32,     Offset(mDescription.mMaxLoopGap, AudioDescription));
   addField("type",              TypeS32,     Offset(mDescription.mType, AudioDescription));

}

//--------------------------------------------------------------------------
bool AudioDescription::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // validate the data
   mDescription.mVolume  = mClampF(mDescription.mVolume, 0.0f, 1.0f);
   mDescription.mLoopCount = mClamp(mDescription.mLoopCount, -1, mDescription.mLoopCount);
   mDescription.mMaxLoopGap = mClamp(mDescription.mMaxLoopGap, mDescription.mMinLoopGap, mDescription.mMaxLoopGap);
   mDescription.mMinLoopGap = mClamp(mDescription.mMinLoopGap, 0, mDescription.mMaxLoopGap);

   if (mDescription.mIs3D)
   {
      // validate the data
      mDescription.mReferenceDistance   = mClampF(mDescription.mReferenceDistance, 0.f, mDescription.mReferenceDistance);
      mDescription.mMaxDistance  = (mDescription.mMaxDistance > mDescription.mReferenceDistance) ? mDescription.mMaxDistance : (mDescription.mReferenceDistance+0.01f);
      mDescription.mConeInsideAngle     = mClamp(mDescription.mConeInsideAngle, 0, 360);
      mDescription.mConeOutsideAngle    = mClamp(mDescription.mConeOutsideAngle, mDescription.mConeInsideAngle, 360);
      mDescription.mConeOutsideVolume   = mClampF(mDescription.mConeOutsideVolume, 0.0f, 1.0f);
      mDescription.mConeVector.normalize();
      mDescription.mEnvironmentLevel    = mClampF(mDescription.mEnvironmentLevel, 0.f, 1.f);
   }

   if(mDescription.mType >= Audio::NumAudioTypes)
      mDescription.mType = 0;

   return true;
}


//--------------------------------------------------------------------------
void AudioDescription::packData(BitStream* stream)
{
   Parent::packData(stream);
   stream->writeFloat(mDescription.mVolume, 6);
   if(stream->writeFlag(mDescription.mIsLooping))
   {
      stream->write(mDescription.mLoopCount);
      stream->write(mDescription.mMinLoopGap);
      stream->write(mDescription.mMaxLoopGap);
   }

   stream->writeFlag(mDescription.mIsStreaming);
   stream->writeFlag(mDescription.mIs3D);
   if (mDescription.mIs3D)
   {
      stream->write(mDescription.mReferenceDistance);
      stream->write(mDescription.mMaxDistance);
      stream->writeInt(mDescription.mConeInsideAngle, 9);
      stream->writeInt(mDescription.mConeOutsideAngle, 9);
      stream->writeInt(mDescription.mConeOutsideVolume, 6);
      stream->writeNormalVector(mDescription.mConeVector, 8);
      stream->write(mDescription.mEnvironmentLevel);
   }
   stream->writeInt(mDescription.mType, 3);
}

//--------------------------------------------------------------------------
void AudioDescription::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   mDescription.mVolume    = stream->readFloat(6);
   mDescription.mIsLooping = stream->readFlag();
   if(mDescription.mIsLooping)
   {
      stream->read(&mDescription.mLoopCount);
      stream->read(&mDescription.mMinLoopGap);
      stream->read(&mDescription.mMaxLoopGap);
   }

   mDescription.mIsStreaming = stream->readFlag();
   mDescription.mIs3D      = stream->readFlag();
   if ( mDescription.mIs3D )
   {
      stream->read(&mDescription.mReferenceDistance);
      stream->read(&mDescription.mMaxDistance);
      mDescription.mConeInsideAngle     = stream->readInt(9);
      mDescription.mConeOutsideAngle    = stream->readInt(9);
      mDescription.mConeOutsideVolume   = stream->readFloat(6);
      stream->readNormalVector(&mDescription.mConeVector, 8);
      stream->read(&mDescription.mEnvironmentLevel);
   }
   mDescription.mType = stream->readInt(3);
}

//--------------------------------------------------------------------------
// Class AudioProfile:
//--------------------------------------------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(AudioProfile);

AudioProfile::AudioProfile()
{
   mFilename   = NULL;
   mDescriptionObject = NULL;
   mSampleEnvironment = 0;
   mPreload = false;
}

//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(AudioProfile)
IMPLEMENT_GETDATATYPE(AudioProfile)
IMPLEMENT_SETDATATYPE(AudioProfile)

void AudioProfile::initPersistFields()
{
   Parent::initPersistFields();

   addField("filename",    TypeFilename,                    Offset(mFilename, AudioProfile));
   addField("description", TypeAudioDescriptionPtr,         Offset(mDescriptionObject, AudioProfile));
   addField("environment", TypeAudioSampleEnvironmentPtr,   Offset(mSampleEnvironment, AudioProfile));
   addField("preload",     TypeBool,                        Offset(mPreload, AudioProfile));
}

bool AudioProfile::preload(bool server, char errorBuffer[256])
{
   if(!Parent::preload(server, errorBuffer))
      return false;

   if(!server && NetConnection::filesWereDownloaded() && !bool(AudioBuffer::find(mFilename)))
      return false;
   return true;
}

//--------------------------------------------------------------------------
bool AudioProfile::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // if this is client side, make sure that description is as well
   if(mDescriptionObject)
   {  // client side dataBlock id's are not in the dataBlock id range
      if (getId() >= DataBlockObjectIdFirst && getId() <= DataBlockObjectIdLast)
      {
         SimObjectId pid = mDescriptionObject->getId();
         if (pid < DataBlockObjectIdFirst || pid > DataBlockObjectIdLast)
         {
            Con::errorf(ConsoleLogEntry::General,"AudioProfile: data dataBlock not networkable (use datablock to create).");
            return false;
         }
      }
   }

   if(mPreload && mFilename != NULL && alcGetCurrentContext())
   {
      mBuffer = AudioBuffer::find(mFilename);
      if(bool(mBuffer))
      {
         ALuint bufferId = mBuffer->getALBuffer();
      }
   }

   return(true);
}

//--------------------------------------------------------------------------
void AudioProfile::packData(BitStream* stream)
{
   Parent::packData(stream);

   // audio description:
   if (stream->writeFlag(mDescriptionObject))
      stream->writeRangedU32(mDescriptionObject->getId(), DataBlockObjectIdFirst,
                                                          DataBlockObjectIdLast);

   // environmental info:
   if (stream->writeFlag(mSampleEnvironment))
      stream->writeRangedU32(mSampleEnvironment->getId(), DataBlockObjectIdFirst,
                                                          DataBlockObjectIdLast);

   //
   char buffer[256];
   if(!mFilename)
      buffer[0] = 0;
   else
      dStrcpy(buffer, mFilename);
//   S32 len = dStrlen(buffer);
//   if(len > 3 && !dStricmp(buffer + len - 4, ".wav"))
//      buffer[len-4] = 0;
   stream->writeString(buffer);

}

//--------------------------------------------------------------------------
void AudioProfile::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   // audio datablock:
   if (stream->readFlag()) {
      SimObjectId id = stream->readRangedU32(DataBlockObjectIdFirst,
                                             DataBlockObjectIdLast);
      Sim::findObject(id, mDescriptionObject);
   }

   // sample environment:
   if (stream->readFlag()) {
      SimObjectId id = stream->readRangedU32(DataBlockObjectIdFirst,
                                             DataBlockObjectIdLast);
      Sim::findObject(id, mSampleEnvironment);
   }

   char buffer[256];
   stream->readString(buffer);
//   dStrcat(buffer, ".wav");

   mFilename = StringTable->insert(buffer);

   // Doh!  Something missing from the unpackData...don't want to break
   // network protocol, so set it to true always here.  This is good for
   // ThinkTanks only.  In the future, simply send the preload bit.
   mPreload = true;
}







