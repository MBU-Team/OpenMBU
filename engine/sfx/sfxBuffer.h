//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXBUFFER_H_
#define _SFXBUFFER_H_

#ifndef _REFBASE_H_
   #include "core/refBase.h"
#endif


/// The buffer interface hides the details of how
/// the device holds sound data for playback.
class SFXBuffer : public RefBase
{
   protected:

      SFXBuffer() {}

   public:

      /// The destructor.
      virtual ~SFXBuffer() {}
   
};


#endif // _SFXBUFFER_H_