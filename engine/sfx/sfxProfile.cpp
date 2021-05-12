//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/sfxProfile.h"
#include "sfx/sfxDescription.h"
#include "sfx/sfxSystem.h"
#include "sim/netConnection.h"
#include "core/bitStream.h"


IMPLEMENT_CO_DATABLOCK_V1( SFXProfile );


SFXProfile::SFXProfile()
   :  mFilename( NULL ),
      mDescription( NULL ),
      mPreload( false ),
      mDescriptionId( 0 )
{
}

SFXProfile::SFXProfile( SFXDescription* desc, StringTableEntry filename, bool preload )
   :  mFilename( filename ),
      mDescription( desc ),
      mPreload( preload ),
      mDescriptionId( 0 )
{
}

SFXProfile::~SFXProfile()
{
}

IMPLEMENT_CONSOLETYPE( SFXProfile )
IMPLEMENT_GETDATATYPE( SFXProfile )
IMPLEMENT_SETDATATYPE( SFXProfile )


void SFXProfile::initPersistFields()
{
   Parent::initPersistFields();

   addField( "filename",    TypeFilename,              Offset(mFilename, SFXProfile));
   addField( "description", TypeSFXDescriptionPtr,     Offset(mDescription, SFXProfile));
   addField( "preload",     TypeBool,                  Offset(mPreload, SFXProfile));
}


bool SFXProfile::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   if (  mDescription == NULL &&
         mDescriptionId != 0 )
   {
      if ( !Sim::findObject( mDescriptionId, mDescription ) )
      {
         Con::errorf( 
            "SFXProfile(%s)::onAdd: Invalid packet, bad description id: %d", 
            getName(), mDescriptionId );
      }
   }

   if ( !mDescription )
   {
      Con::errorf( 
         "SFXProfile(%s)::onAdd: The profile is missing a description!", 
         getName() );
      return false;
   }

   if ( SFX )
   {
      // If preload is enabled we load the resource
      // and device buffer now to avoid a delay on
      // first playback.
      if ( mPreload && !_preloadBuffer() )
         Con::warnf( "SFXProfile(%s)::onAdd: The preload failed!", getName() );

      // We need to get device change notifications.
      SFXDevice::getEventSignal().notify( this, &SFXProfile::_onDeviceEvent );
   }

   return true;
}

void SFXProfile::onRemove()
{
   // Remove us from the signal.
   SFXDevice::getEventSignal().remove( this, &SFXProfile::_onDeviceEvent );

   Parent::onRemove();
}

bool SFXProfile::preload( bool server, char errorBuffer[256] )
{
   if ( !Parent::preload( server, errorBuffer ) )
      return false;

   // TODO: Investigate how NetConnection::filesWereDownloaded()
   // effects the system.

   // Validate the datablock... has nothing to do with mPreload.
   if (  !server &&
         NetConnection::filesWereDownloaded() &&
         ( !mFilename || !SFXResource::exists( mFilename ) ) )
      return false;

   return true;
}


void SFXProfile::packData(BitStream* stream)
{
   Parent::packData( stream );

   // audio description:
   if ( stream->writeFlag( mDescription ) )
   {
      stream->writeRangedU32( mDescription->getId(),  
                              DataBlockObjectIdFirst,
                              DataBlockObjectIdLast );
   }

   //
   char buffer[256];
   if ( !mFilename )
      buffer[0] = 0;
   else
      dStrncpy( buffer, mFilename, 256 );
   stream->writeString( buffer );

   stream->writeFlag( mPreload );
}


void SFXProfile::unpackData(BitStream* stream)
{
   Parent::unpackData( stream );

   // audio datablock:
   if ( stream->readFlag() )
      mDescriptionId = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );

   char buffer[256];
   stream->readString( buffer );
   mFilename = StringTable->insert( buffer );

   mPreload = stream->readFlag();

   // TODO: Do we release mResource here if it has
   // change?  If it was already loaded do we reload it?
}

void SFXProfile::_onDeviceEvent( SFXDevice *device, SFXDeviceEventType evt )
{
   switch ( evt )
   {
      case SFXDeviceEvent_Create:
      {
         if ( mPreload && !_preloadBuffer() )
            Con::warnf( "SFXProfile::_onDeviceEvent: The preload failed! %s", getName() );
         break;
      }
   }
}

bool SFXProfile::_preloadBuffer()
{
   mBuffer = NULL;

   if ( !mResource )
      mResource = SFXResource::load( mFilename );

   if ( mResource && SFX )
      mBuffer = SFX->_createBuffer( this );

   return mBuffer;
}

SFXBuffer* SFXProfile::getBuffer()
{
   if ( mBuffer )
      return mBuffer;

   _preloadBuffer();
   return mBuffer;
}
