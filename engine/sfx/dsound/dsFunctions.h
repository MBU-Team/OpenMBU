//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// The various functions we need to grab from DSound.dll
DS_FUNCTION( DirectSoundEnumerateA, HRESULT, (LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext) )
DS_FUNCTION( DirectSoundCreate8, HRESULT, (LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) )

