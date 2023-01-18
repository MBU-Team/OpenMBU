//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

D3D9_FUNCTION( Direct3DCreate9, IDirect3D9*, (UINT SDKVersion) )

#ifndef _XBOX
D3D9_FUNCTION( D3DPERF_BeginEvent, int, ( D3DCOLOR col, LPCWSTR wszName ) )
D3D9_FUNCTION( D3DPERF_EndEvent, int, ( void ) )
D3D9_FUNCTION( D3DPERF_SetMarker, void, ( D3DCOLOR col, LPCWSTR wszName ) )
#endif