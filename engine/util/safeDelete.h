//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Utility macros. We clobber to enforce our semantics!

#undef  SAFE_DELETE
#define SAFE_DELETE(a) if( (a) != NULL ) delete (a); (a) = NULL;

#undef  SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(a) if( (a) != NULL ) delete [] (a); (a) = NULL;

#undef  SAFE_FREE
#define SAFE_FREE(a)   if( (a) != NULL ) dFree (a); (a) = NULL;