//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXNULLBUFFER_H_
#define _SFXNULLBUFFER_H_

#ifndef _SFXBUFFER_H_
   #include "sfx/sfxBuffer.h"
#endif


class SFXNullBuffer : public SFXBuffer
{
   friend class SFXNullDevice;

   protected:

      SFXNullBuffer();

   public:

      virtual ~SFXNullBuffer();

};


/// A vector of SFXNullBuffer pointers.
typedef Vector<SFXNullBuffer*> SFXNullBufferVector;

#endif // _SFXNULLBUFFER_H_