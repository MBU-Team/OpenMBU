//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SFXDESCRIPTION_H_
#define _SFXDESCRIPTION_H_


#ifndef _CONSOLETYPES_H_
   #include "console/consoleTypes.h"
#endif
#ifndef _SIMDATABLOCK_H_
   #include "console/simBase.h"
#endif
#ifndef _MPOINT_H_
   #include "math/mPoint.h"
#endif


class SFXDescription : public SimDataBlock
{
   private:

      typedef SimDataBlock Parent;

   public:

      F32  mVolume;

      bool mIsLooping;

      bool mIsStreaming;

      bool mIs3D;

      F32 mReferenceDistance;

      F32 mMaxDistance;

      U32 mConeInsideAngle;

      U32 mConeOutsideAngle;

      F32 mConeOutsideVolume;

      Point3F mConeVector;

      S32 mChannel;

      SFXDescription();
      SFXDescription( const SFXDescription& desc );
      DECLARE_CONOBJECT( SFXDescription );
      static void initPersistFields();

      virtual bool onAdd();
      virtual void packData( BitStream* stream );
      virtual void unpackData( BitStream* stream );

      void validate();
};

DECLARE_CONSOLETYPE( SFXDescription )


#endif // _SFXDESCRIPTION_H_