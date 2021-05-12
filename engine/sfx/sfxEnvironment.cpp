//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/sfxEnvironment.h"
#include "sim/netConnection.h"
#include "core/bitStream.h"


IMPLEMENT_CO_DATABLOCK_V1( SFXEnvironment );


SFXEnvironment::SFXEnvironment()

   /*
   :  //mUseRoom( true ),
      mRoom( 0 ),
      mRoomHF( 0 ),
      mReflections( 0 ),
      mReverb( 0 ),
      mRoomRolloffFactor( 0.1f ),
      mDecayTime( 0.1f ),
      mDecayHFRatio( 0.1f ),
      mReflectionsDelay( 0 ),
      mReverbDelay( 0 ),
      mRoomVolume( 0 ),
      mEffectVolume( 0 ),
      mDamping( 0 ),
      mEnvironmentSize( 10 ),
      mEnvironmentDiffusion( 1 ),
      mAirAbsorption( 0 ),
      mFlags( 0 )
   */
{
}


IMPLEMENT_CONSOLETYPE( SFXEnvironment )
IMPLEMENT_GETDATATYPE( SFXEnvironment )
IMPLEMENT_SETDATATYPE( SFXEnvironment )


void SFXEnvironment::initPersistFields()
{
   Parent::initPersistFields();

   /*
   // TODO: Deal with room presets... develop our 
   // own based on EAX settings?

   //addField( "useRoom",              TypeBool,   Offset(mUseRoom, SFXEnvironment));
   //addField( "room",                 TypeEnum,   Offset(mRoom, SFXEnvironment), 1, &gAudioEnvironmentRoomTypes);
   addField( "roomHF",               TypeS32,    Offset(mRoomHF, SFXEnvironment));
   addField( "reflections",          TypeS32,    Offset(mReflections, SFXEnvironment));
   addField( "reverb",               TypeS32,    Offset(mReverb, SFXEnvironment));
   addField( "roomRolloffFactor",    TypeF32,    Offset(mRoomRolloffFactor, SFXEnvironment));
   addField( "decayTime",            TypeF32,    Offset(mDecayTime, SFXEnvironment));
   addField( "decayHFRatio",         TypeF32,    Offset(mDecayHFRatio, SFXEnvironment));
   addField( "reflectionsDelay",     TypeF32,    Offset(mReflectionsDelay, SFXEnvironment));
   addField( "reverbDelay",          TypeF32,    Offset(mReverbDelay, SFXEnvironment));
   addField( "roomVolume",           TypeS32,    Offset(mRoomVolume, SFXEnvironment));
   addField( "effectVolume",         TypeF32,    Offset(mEffectVolume, SFXEnvironment));
   addField( "damping",              TypeF32,    Offset(mDamping, SFXEnvironment));
   addField( "environmentSize",      TypeF32,    Offset(mEnvironmentSize, SFXEnvironment));
   addField( "environmentDiffusion", TypeF32,    Offset(mEnvironmentDiffusion, SFXEnvironment));
   addField( "airAbsorption",        TypeF32,    Offset(mAirAbsorption, SFXEnvironment));
   addField( "flags",                TypeS32,    Offset(mFlags, SFXEnvironment));
   */
}


void SFXEnvironment::packData( BitStream* stream )
{
   Parent::packData( stream );

   /* 
   if ( stream->writeFlag( mUseRoom ) )
      stream->writeRangedU32(mRoom, EAX_ENVIRONMENT_GENERIC, EAX_ENVIRONMENT_COUNT);
   else
   {
      stream->writeRangedS32( mRoomHF, -10000, 0 );
      stream->writeRangedS32( mReflections, -10000, 10000 );
      stream->writeRangedS32( mReverb, -10000, 2000 );

      stream->writeRangedF32( mRoomRolloffFactor, 0.1f, 10.f, 8 );
      stream->writeRangedF32( mDecayTime, 0.1f, 20.f, 8 );
      stream->writeRangedF32( mDecayHFRatio, 0.1f, 20.f, 8 );
      stream->writeRangedF32( mReflectionsDelay, 0.f, 0.3f, 9 );
      stream->writeRangedF32( mReverbDelay, 0.f, 0.1f, 7 );
      stream->writeRangedS32( mRoomVolume, -10000, 0 );
      stream->writeRangedF32( mEffectVolume, 0.f, 1.f, 8 );
      stream->writeRangedF32( mDamping, 0.f, 2.f, 9 );
      stream->writeRangedF32( mEnvironmentSize, 1.f, 100.f, 10 );
      stream->writeRangedF32( mEnvironmentDiffusion, 0.f, 1.f, 8 );
      stream->writeRangedF32( mAirAbsorption, -100.f, 0.f, 10 );

      stream->writeInt( mFlags, 6 );
   }
   */
}


void SFXEnvironment::unpackData( BitStream* stream )
{
   Parent::unpackData( stream );
   
   /*
   mUseRoom = stream->readFlag();
   if(mUseRoom)
      mRoom = stream->readRangedU32(EAX_ENVIRONMENT_GENERIC, EAX_ENVIRONMENT_COUNT);
   else
   {
      mRoomHF        = stream->readRangedS32( -10000, 0 );
      mReflections   = stream->readRangedS32( -10000, 10000 );
      mReverb        = stream->readRangedS32( -10000, 2000 );

      mRoomRolloffFactor      = stream->readRangedF32( 0.1f, 10.f, 8 );
      mDecayTime              = stream->readRangedF32( 0.1f, 20.f, 8 );
      mDecayHFRatio           = stream->readRangedF32( 0.1f, 20.f, 8 );
      mReflectionsDelay       = stream->readRangedF32( 0.f, 0.3f, 9 );
      mReverbDelay            = stream->readRangedF32( 0.f, 0.1f, 7 );
      mRoomVolume             = stream->readRangedS32( -10000, 0 );
      mEffectVolume           = stream->readRangedF32( 0.f, 1.f, 8 );
      mDamping                = stream->readRangedF32( 0.f, 2.f, 9 );
      mEnvironmentSize        = stream->readRangedF32( 1.f, 100.f, 10 );
      mEnvironmentDiffusion   = stream->readRangedF32( 0.f, 1.f, 8 );
      mAirAbsorption          = stream->readRangedF32( -100.f, 0.f, 10 );

      mFlags = stream->readInt( 6 );
   }
   */
}





