//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// These functions are kept in alphabetical order for easy lookups.

FMOD_FUNCTION(FMOD_Channel_GetPaused, (FMOD_CHANNEL *channel, FMOD_BOOL *paused))
FMOD_FUNCTION(FMOD_Channel_SetPaused, (FMOD_CHANNEL *channel, FMOD_BOOL paused));
FMOD_FUNCTION(FMOD_Channel_IsPlaying, (FMOD_CHANNEL *channel, FMOD_BOOL *isplaying))
FMOD_FUNCTION(FMOD_Channel_Set3DAttributes, (FMOD_CHANNEL *channel, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel))
FMOD_FUNCTION(FMOD_Channel_SetFrequency, (FMOD_CHANNEL *channel, float frequency))
FMOD_FUNCTION(FMOD_Channel_SetLoopCount, (FMOD_CHANNEL *channel, int loopcount))
FMOD_FUNCTION(FMOD_Channel_SetPosition, (FMOD_CHANNEL *channel, unsigned int position, FMOD_TIMEUNIT postype))
FMOD_FUNCTION(FMOD_Channel_SetVolume, (FMOD_CHANNEL *channel, float volume))
FMOD_FUNCTION(FMOD_Channel_GetVolume, (FMOD_CHANNEL *channel, float *volume));
FMOD_FUNCTION(FMOD_Channel_Stop, (FMOD_CHANNEL *channel))
FMOD_FUNCTION(FMOD_Channel_SetMode, (FMOD_CHANNEL *channel, FMOD_MODE mode));
FMOD_FUNCTION(FMOD_Channel_Set3DMinMaxDistance, (FMOD_CHANNEL *channel, float mindistance, float maxdistance));

FMOD_FUNCTION(FMOD_Sound_Lock, (FMOD_SOUND *sound, unsigned int offset, unsigned int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2))
FMOD_FUNCTION(FMOD_Sound_Release, (FMOD_SOUND *sound))
FMOD_FUNCTION(FMOD_Sound_Set3DMinMaxDistance, (FMOD_SOUND *sound, float min, float max))
FMOD_FUNCTION(FMOD_Sound_SetMode, (FMOD_SOUND *sound, FMOD_MODE mode))
FMOD_FUNCTION(FMOD_Sound_GetMode, (FMOD_SOUND *sound, FMOD_MODE *mode))
FMOD_FUNCTION(FMOD_Sound_SetLoopCount, (FMOD_SOUND *channel, int loopcount))
FMOD_FUNCTION(FMOD_Sound_Unlock, (FMOD_SOUND *sound, void *ptr1, void *ptr2, unsigned int len1, unsigned int len2))

FMOD_FUNCTION(FMOD_System_Close, (FMOD_SYSTEM *system))
FMOD_FUNCTION(FMOD_System_Create, (FMOD_SYSTEM **system))
FMOD_FUNCTION(FMOD_System_CreateSound, (FMOD_SYSTEM *system, const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, FMOD_SOUND **sound))
FMOD_FUNCTION(FMOD_System_SetDriver, (FMOD_SYSTEM *system, int driver))
FMOD_FUNCTION(FMOD_System_GetDriverCaps, (FMOD_SYSTEM *system, int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode))
FMOD_FUNCTION(FMOD_System_GetDriverName, (FMOD_SYSTEM *system, int id, char *name, int namelen))
FMOD_FUNCTION(FMOD_System_GetNumDrivers, (FMOD_SYSTEM *system, int *numdrivers))
FMOD_FUNCTION(FMOD_System_GetVersion, (FMOD_SYSTEM *system, unsigned int *version))
FMOD_FUNCTION(FMOD_System_Init, (FMOD_SYSTEM *system, int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata))
FMOD_FUNCTION(FMOD_System_PlaySound, (FMOD_SYSTEM *system, FMOD_CHANNELINDEX channelid, FMOD_SOUND *sound, FMOD_BOOL paused, FMOD_CHANNEL **channel))
FMOD_FUNCTION(FMOD_System_Set3DListenerAttributes, (FMOD_SYSTEM *system, int listener, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel, const FMOD_VECTOR *forward, const FMOD_VECTOR *up))
FMOD_FUNCTION(FMOD_System_Set3DSettings, (FMOD_SYSTEM *system, float dopplerscale, float distancefactor, float rolloffscale))
FMOD_FUNCTION(FMOD_System_SetDSPBufferSize, (FMOD_SYSTEM *system, unsigned int bufferlength, int numbuffers))
FMOD_FUNCTION(FMOD_System_SetSpeakerMode, (FMOD_SYSTEM *system, FMOD_SPEAKERMODE speakermode))
FMOD_FUNCTION(FMOD_System_Update, (FMOD_SYSTEM *system))
FMOD_FUNCTION(FMOD_Memory_GetStats, (int*, int*))
