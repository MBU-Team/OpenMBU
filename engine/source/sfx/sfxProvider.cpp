//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "sfx/sfxProvider.h"


SFXProvider* SFXProvider::smProviders = NULL;


SFXProvider* SFXProvider::findProvider( const char* providerName )
{
   if ( !providerName || !providerName[0] )
      return NULL;

   SFXProvider* curr = smProviders;
   for ( ; curr != NULL; curr = curr->mNextProvider )
   {
      if ( dStricmp( curr->getName(), providerName ) == 0 )
         return curr;
   }

   return NULL;
}

void SFXProvider::regProvider( SFXProvider* provider )
{
   AssertFatal( provider, "Got null provider!" );
   AssertFatal( findProvider( provider->getName() ) == NULL, "Can't register provider twice!" );
   AssertFatal( provider->mNextProvider == NULL, "Can't register provider twice!" );

   SFXProvider* oldHead = smProviders;
   smProviders = provider;
   provider->mNextProvider = oldHead;
}

SFXProvider::SFXProvider( const char* name )
   :  mName( name ),
      mNextProvider( NULL )
{
}

SFXProvider::~SFXProvider()
{
   SFXDeviceInfoVector::iterator iter = mDeviceInfo.begin();
   for ( ; iter != mDeviceInfo.end(); iter++ )
      delete *iter;
}
