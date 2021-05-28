//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "sfx/fmod/sfxFMODDevice.h"

#include "sfx/fmod/sfxFMODBuffer.h"

#include "sfx/sfxListener.h"
#include "sfx/sfxSystem.h"

FMOD_SYSTEM *SFXFMODDevice::smSystem = NULL;
FModFNTable *SFXFMODDevice::smFunc = NULL;


SFXFMODDevice::SFXFMODDevice( SFXProvider* provider, 
                              FModFNTable *fmodFnTbl, 
                              int deviceIdx, 
                              const char* name )
   :  SFXDevice( provider, true, 32 ),
      mName( name )
{
	AssertISV(smSystem, 
      "SFXFMODDevice::SFXFMODDevice - can't init w/o an existing FMOD system handle!");

	// Initialize everything from fmod.
	FMOD_SPEAKERMODE speakermode;
	FMOD_CAPS        caps;
	FModAssert(fmodFnTbl->FMOD_System_GetDriverCaps(smSystem, 0, &caps, 0, 0, &speakermode), 
      "SFXFMODDevice::SFXFMODDevice - Failed to get driver caps!");

	FModAssert(fmodFnTbl->FMOD_System_SetDriver(smSystem, deviceIdx), 
      "SFXFMODDevice::SFXFMODDevice - Failed to set driver!");

	FModAssert(fmodFnTbl->FMOD_System_SetSpeakerMode(smSystem, speakermode), 
      "SFXFMODDevice::SFXFMODDevice - Failed to set the user selected speaker mode.");

	if (caps & FMOD_CAPS_HARDWARE_EMULATED)             /* The user has the 'Acceleration' slider set to off!  This is really bad for latency!. */
	{                                                   /* You might want to warn the user about this. */
		FModAssert(fmodFnTbl->FMOD_System_SetDSPBufferSize(smSystem, 1024, 10), 
         "SFXFMODDevice::SFXFMODDevice - Failed to set DSP buffer size!");
	}

	int result = fmodFnTbl->FMOD_System_Init(smSystem, 100, FMOD_INIT_NORMAL, 0);
	if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)         /* Ok, the speaker mode selected isn't supported by this soundcard.  Switch it back to stereo... */
	{
		FModAssert(fmodFnTbl->FMOD_System_SetSpeakerMode(smSystem, FMOD_SPEAKERMODE_STEREO), 
         "SFXFMODDevice::SFXFMODDevice - failed on fallback speaker mode setup");
		FModAssert(fmodFnTbl->FMOD_System_Init(smSystem, 100, FMOD_INIT_NORMAL, 0), 
         "SFXFMODDevice::SFXFMODDevice - failed to reinit");
	}

	/*
	Set the distance units. (meters/feet etc).
	*/
	FModAssert(fmodFnTbl->FMOD_System_Set3DSettings(smSystem, 1.0f, 1.0f, 1.0f), 
      "SFXFMODDevice::SFXFMODDevice - Failed to setup 3d settings!");

	// Store off the function pointers for later use.
	smFunc = fmodFnTbl;

   m3drolloffmode = FMOD_3D_LINEARROLLOFF; 

   // we let FMod handle this stuff, instead of having sfx do it
   //mCullInaudible = false;
}

SFXFMODDevice::~SFXFMODDevice()
{
   // First cleanup the buffers... then voices.

   SFXFMODBufferVector::iterator buffer = mBuffers.begin();
   for ( ; buffer != mBuffers.end(); buffer++ )
      delete (*buffer);
   mBuffers.clear();

   SFXFMODVoiceVector::iterator voice = mVoices.begin();
   for ( ; voice != mVoices.end(); voice++ )
      delete (*voice);
   mVoices.clear();

	if ( smFunc && smSystem )
		smFunc->FMOD_System_Close( smSystem );
}

/*
SFXBuffer* SFXFMODDevice::preload(const SFXPreloadedBuffer* r)
{
   if (r == NULL)
      return NULL;
   if (r->profile == NULL)
      return NULL;
   if (r->resource == NULL)
      return NULL;

   //Con::printf("Preloading sfx resource: %s", r->resource->mSourceResource->getFullPath());

   // pass false to createBuffer so that the preload buffer is not appended to buffer list - 
   // we don't want it in there
   SFXBuffer* buf = createBuffer(r->profile->getDescription()->mIs3D, r->resource, NULL, false);
   AssertFatal(dynamic_cast<SFXFMODBuffer*>(buf) != NULL, "doh");
   SFXFMODBuffer* fBuf = static_cast<SFXFMODBuffer*>(buf);
   fBuf->setOwnSound(true);

   // since this is a preload buffer, copyData now to fill it.  use the internal version
   // so that we just fill the buffer, but don't start a sound playing
   AssertFatal(fBuf->_copyData( 0, r->resource->getData(), r->resource->getSize() ), "preload copydata failed");

   return fBuf;
}
*/

SFXBuffer* SFXFMODDevice::createBuffer( SFXProfile *profile )
{
   AssertFatal( profile, "SFXFMODDevice::createBuffer() - Got null profile!" );

   SFXFMODBuffer *buffer = SFXFMODBuffer::create( profile );
   if ( !buffer )
      return NULL;

   mBuffers.push_back( buffer );
	return buffer;
}

SFXVoice* SFXFMODDevice::createVoice( SFXBuffer* buffer )
{
   AssertFatal( buffer, "SFXFMODDevice::createVoice() - Got null buffer!" );

   SFXFMODBuffer* fmodBuffer = dynamic_cast<SFXFMODBuffer*>( buffer );
   AssertFatal( fmodBuffer, "SFXFMODDevice::createVoice() - Got bad buffer!" );

   SFXFMODVoice* voice = SFXFMODVoice::create( this, fmodBuffer );
   if ( !voice )
      return NULL;

   mVoices.push_back( voice );
	return voice;
}

void SFXFMODDevice::deleteVoice( SFXVoice* voice )
{
   AssertFatal( voice, "SFXFMODDevice::deleteVoice() - Got null voice!" );

   SFXFMODVoice* fmodVoice = dynamic_cast<SFXFMODVoice*>( voice );
   AssertFatal( fmodVoice, "SFXFMODDevice::deleteVoice() - Got bad voice!" );

	// Find the buffer...
	SFXFMODVoiceVector::iterator iter = find( mVoices.begin(), mVoices.end(), fmodVoice );
	AssertFatal( iter != mVoices.end(), "SFXFMODDevice::deleteVoice() - Got unknown voice!" );

	mVoices.erase( iter );
	delete fmodVoice;
}

void SFXFMODDevice::update( const SFXListener& listener )
{
	// Set the listener state on fmod!
	Point3F position, vel, fwd, up;
	vel = listener.getVelocity();
	listener.getTransform().getColumn( 3, &position );
	listener.getTransform().getColumn( 1, &fwd );
	listener.getTransform().getColumn( 2, &up );

	// We have to convert to FMOD_VECTOR, thus this cacophony.
	// NOTE: we correct for handedness here. We model off of the d3d device.
	//       Basically, XYZ => XZY.
	FMOD_VECTOR fposition, fvel, ffwd, fup;
#define COPY_FMOD_VECTOR(a) f##a.x = a.x; f##a.y = a.z; f##a.z = a.y;
	COPY_FMOD_VECTOR(position)
	COPY_FMOD_VECTOR(vel)
	COPY_FMOD_VECTOR(fwd)
	COPY_FMOD_VECTOR(up)
#undef COPY_FMOD_VECTOR

	// Do the listener state update, then update!
	FModAssert(smFunc->FMOD_System_Set3DListenerAttributes(smSystem, 0, &fposition, &fvel, &ffwd, &fup), "Failed to set 3d listener attribs!");
	FModAssert(smFunc->FMOD_System_Update(smSystem), "Failed to update system!");
}

ConsoleFunction(fmodDumpMemoryStats, void, 1, 1, "()")
{
   int current = 0;
   int max = 0;

   if (SFXFMODDevice::smFunc && SFXFMODDevice::smFunc->FMOD_Memory_GetStats)
         SFXFMODDevice::smFunc->FMOD_Memory_GetStats(&current, &max);
   Con::printf("Fmod current: %d, max: %d", current, max);
}