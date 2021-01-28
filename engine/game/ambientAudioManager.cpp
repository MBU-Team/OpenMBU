//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/ambientAudioManager.h"
#include "interior/interiorInstance.h"
#include "game/gameConnection.h"
#include "interior/interior.h"
#include "terrain/waterBlock.h"
#include "sceneGraph/sceneGraph.h"

AmbientAudioManager     gAmbientAudioManager;

//------------------------------------------------------------------------------
AmbientAudioManager::AmbientAudioManager()
{
   mOutsideScale = 1.f;
   mInteriorAudioHandle = NULL_AUDIOHANDLE;
   mPowerAudioHandle = NULL_AUDIOHANDLE;
   mLastAlarmState = false;
}

//------------------------------------------------------------------------------
ConsoleFunction(setPowerAudioProfiles, void, 3, 3, "( AudioProfile powerUp, AudioProfile powerDown)"
                "Set the ambient audio manager's power up/down profiles.")
{
   gAmbientAudioManager.mPowerUpProfile = dynamic_cast<AudioProfile*>(Sim::findObject(argv[1]));
   gAmbientAudioManager.mPowerDownProfile = dynamic_cast<AudioProfile*>(Sim::findObject(argv[2]));
}

void AmbientAudioManager::addEmitter(AudioEmitter * emitter)
{
   mEmitters.push_back(emitter);
   updateEmitter(emitter);
}

void AmbientAudioManager::removeEmitter(AudioEmitter * emitter)
{
   for(U32 i = 0; i < mEmitters.size(); i++)
      if(mEmitters[i] == emitter)
      {
         mEmitters.erase_fast(i);
         return;
      }
}

bool AmbientAudioManager::getOutsideScale(F32 * pScale, InteriorInstance ** pInterior)
{
   GameConnection * gc = dynamic_cast<GameConnection*>(NetConnection::getConnectionToServer());
   if(!gc)
      return(false);

   MatrixF camMat;
   if(!gc->getControlCameraTransform(0.032, &camMat))
      return(false);

   Point3F pos;
   camMat.getColumn(3, &pos);

   RayInfo collision;
   if(!gClientContainer.castRay(pos, Point3F(pos.x, pos.y, pos.z - 2000.f), InteriorObjectType, &collision))
   {
      *pScale = 1.f;
      *pInterior = 0;
      return(true);
   }

   // could have hit a null face.. dont change anything
   if(collision.face == -1)
   {
      *pScale = mOutsideScale;
      *pInterior = static_cast<InteriorInstance*>(mInteriorInstance);
      return(true);
   }

   InteriorInstance * interior = dynamic_cast<InteriorInstance *>(collision.object);
   if(!interior)
   {
      Con::errorf(ConsoleLogEntry::General, "AmbientAudioManager::getOutsideScale: invalid interior on collision");
      return(false);
   }

   // 'inside'?
   if(!interior->getDetailLevel(0)->isSurfaceOutsideVisible(collision.face))
   {
      // how 'inside' are we?
      F32 insideScale = 1.f;
      interior->getPointInsideScale(pos, &insideScale);

      // desire 'outside' scale...
      *pScale = 1.f - insideScale;
      *pInterior = interior;
   }
   else
   {
      *pScale = 1.f;
      *pInterior = 0;
   }

   return(true);
}

void AmbientAudioManager::stopInteriorAudio()
{
   if(mInteriorAudioHandle != NULL_AUDIOHANDLE)
      alxStop(mInteriorAudioHandle);
   if(mPowerAudioHandle != NULL_AUDIOHANDLE)
      alxStop(mPowerAudioHandle);

   mInteriorInstance = 0;
   mInteriorAudioHandle = mPowerAudioHandle = NULL_AUDIOHANDLE;
}

//---------------------------------------------------------------------------
void AmbientAudioManager::update()
{
   if(!bool(mInteriorInstance) && (mInteriorAudioHandle != NULL_AUDIOHANDLE))
      stopInteriorAudio();

   F32 scale = mOutsideScale;
   InteriorInstance * interior = 0;

   if(!getOutsideScale(&scale, &interior))
      stopInteriorAudio();

   // handle the interior sound...
   if(interior)
   {
      if(bool(mInteriorInstance) && (interior != static_cast<InteriorInstance*>(mInteriorInstance)))
         stopInteriorAudio();

      // update
      if(bool(mInteriorInstance))
      {
         AudioProfile * profile = interior->getAudioProfile();
         if(profile)
         {
            if(mLastAlarmState ^ mInteriorInstance->inAlarmState())
            {
               if(mLastAlarmState)
               {
                  mLastAlarmState = false;

                  // play powerup
                  if(bool(mPowerUpProfile))
                     mPowerAudioHandle = alxPlay(mPowerUpProfile, &mInteriorInstance->getTransform());
               }
               else
               {
                  alxStop(mInteriorAudioHandle);
                  mInteriorAudioHandle = NULL_AUDIOHANDLE;
                  mLastAlarmState = true;

                  // play powerdown
                  if(bool(mPowerDownProfile))
                     mPowerAudioHandle = alxPlay(mPowerDownProfile, &mInteriorInstance->getTransform());
               }
            }

            // play the ambient noise when the powerup sound is done
            if((mInteriorAudioHandle == NULL_AUDIOHANDLE) && (mPowerAudioHandle == NULL_AUDIOHANDLE)
               && (mLastAlarmState == false))
               mInteriorAudioHandle = alxPlay(profile, &mInteriorInstance->getTransform());

            if(mInteriorAudioHandle != NULL_AUDIOHANDLE)
               alxSourcef(mInteriorAudioHandle, AL_GAIN_LINEAR, 1.f - scale);

            if(mPowerAudioHandle != NULL_AUDIOHANDLE)
            {
               if(alxIsPlaying(mPowerAudioHandle))
                  alxSourcef(mPowerAudioHandle, AL_GAIN_LINEAR, 1.f - scale);
               else
                  mPowerAudioHandle = NULL_AUDIOHANDLE;
            }
         }
      }
      else // start
      {
         mInteriorInstance = interior;
         mLastAlarmState = mInteriorInstance->inAlarmState();

         if(!mLastAlarmState)
         {
            AudioProfile * profile = interior->getAudioProfile();
            if(profile)
               mInteriorAudioHandle = alxPlay(profile, &mInteriorInstance->getTransform());
         }
         else
            mInteriorAudioHandle = NULL_AUDIOHANDLE;
      }
   }
   else
      stopInteriorAudio();

   updateEnvironment();
   if(scale == mOutsideScale)
      return;

   mOutsideScale = scale;

   // do all the outside sounds
   for(U32 i = 0; i < mEmitters.size(); i++)
      updateEmitter(mEmitters[i]);

}

void AmbientAudioManager::updateEnvironment()
{
   F32 scale = 0.f;

   // inside?
   if(bool(mInteriorInstance))
   {
      mCurrentEnvironment = mInteriorInstance->getAudioEnvironment();
      scale = 1.f - mOutsideScale;
   }
   else
   {
      mCurrentEnvironment = 0;

      Point3F pos;
      alxGetListenerPoint3F(AL_POSITION, &pos);

      // check if submerged
      SimpleQueryList sql;
      gClientSceneGraph->getWaterObjectList(sql);

      // grab the audio environment from the waterblock
      for(U32 i = 0; i < sql.mList.size(); i++)
      {
         WaterBlock * wBlock = dynamic_cast<WaterBlock*>(sql.mList[i]);
         if(wBlock && wBlock->isPointSubmerged(pos))
         {
            mCurrentEnvironment = wBlock->getAudioEnvironment();
            scale = 1.f;
            break;
         }
      }
   }

   if(mCurrentEnvironment != alxGetEnvironment())
      alxSetEnvironment(mCurrentEnvironment);

   if(mEnvironmentScale != scale)
   {
      mEnvironmentScale = scale;
#pragma message("todo") /*
      alxEnvironmentf(AL_ENV_EFFECT_VOLUME_EXT, mEnvironmentScale);
*/
   }
}

void AmbientAudioManager::updateEmitter(AudioEmitter * emitter)
{
   if(emitter->mOutsideAmbient && (emitter->mAudioHandle != NULL_AUDIOHANDLE))
      alxSourcef(emitter->mAudioHandle, AL_GAIN_LINEAR, emitter->mDescription.mVolume * mOutsideScale);
}
