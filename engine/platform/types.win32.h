//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TYPESWIN32_H_
#define _TYPESWIN32_H_


#define FN_CDECL __cdecl            ///< Calling convention

// size_t is needed to overload new
// size_t tends to be OS and compiler specific and may need to 
// be if/def'ed in the future
typedef unsigned int  dsize_t;      


/** Platform dependent file date-time structure.  The defination of this structure
  * will likely be different for each OS platform.
  */
struct FileTime                     
{
   U32 v1;
   U32 v2;
};


#ifndef NULL
#  define NULL 0
#endif


#endif //_TYPESWIN32_H_
