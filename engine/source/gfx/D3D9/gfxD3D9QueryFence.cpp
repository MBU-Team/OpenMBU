//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/gfxD3D9QueryFence.h"

GFXD3D9QueryFence::~GFXD3D9QueryFence()
{
   SAFE_RELEASE( mQuery );
}

//------------------------------------------------------------------------------

void GFXD3D9QueryFence::issue()
{
   PROFILE_START( GFXD3D9QueryFence_issue );

   // Create the query if we need to
   if( mQuery == NULL )
   {
      HRESULT hRes = static_cast<GFXD3D9Device *>( mDevice )->getDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &mQuery );

      AssertFatal( hRes != D3DERR_NOTAVAILABLE, "Hardware does not support D3D9 Queries, this should be caught before this fence type is created" );
      AssertISV( hRes != E_OUTOFMEMORY, "Out of memory" );
   }

   // Issue the query
   mQuery->Issue( D3DISSUE_END );

   PROFILE_END();
}

//------------------------------------------------------------------------------

GFXFence::FenceStatus GFXD3D9QueryFence::getStatus() const
{
   if( mQuery == NULL )
      return GFXFence::Unset;

   HRESULT hRes = mQuery->GetData( NULL, 0, 0 );

   return ( hRes == S_OK ? GFXFence::Processed : GFXFence::Pending );
}

//------------------------------------------------------------------------------

void GFXD3D9QueryFence::block()
{
   PROFILE_SCOPE(GFXD3D9QueryFence_block);

   // Calling block() before issue() is valid, catch this case
   if( mQuery == NULL )
      return;

   HRESULT hRes;
   BOOL bData;
   while( ( hRes = mQuery->GetData( &bData, sizeof(BOOL), D3DGETDATA_FLUSH ) ) == S_FALSE )
      ;

   // Check for D3DERR_DEVICELOST, if we lost the device, the fence will get 
   // re-created next issue()
   if( hRes == D3DERR_DEVICELOST )
      SAFE_RELEASE( mQuery );
}

void GFXD3D9QueryFence::zombify()
{
   // Release our query
   SAFE_RELEASE( mQuery );
}

void GFXD3D9QueryFence::resurrect()
{
   // Recreate the query
   if( mQuery == NULL )
   {
      HRESULT hRes = static_cast<GFXD3D9Device *>( mDevice )->getDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &mQuery );

      AssertFatal( hRes != D3DERR_NOTAVAILABLE, "GFXD3D9QueryFence::resurrect - Hardware does not support D3D9 Queries, this should be caught before this fence type is created" );
      AssertISV( hRes != E_OUTOFMEMORY, "GFXD3D9QueryFence::resurrect - Out of memory" );
   }
}

void GFXD3D9QueryFence::describeSelf( char* buffer, U32 sizeOfBuffer )
{
   // We've got nothing
   buffer[0] = NULL;
}
