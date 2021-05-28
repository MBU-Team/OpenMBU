//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_SPLASH
#define _H_SPLASH

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _LLIST_H_
#include "core/llist.h"
#endif

class ParticleEmitter;
class ParticleEmitterData;
class AudioProfile;
class ExplosionData;


//--------------------------------------------------------------------------
// Ring Point
//--------------------------------------------------------------------------
struct SplashRingPoint
{
   Point3F  position;
   Point3F  velocity;
};

//--------------------------------------------------------------------------
// Splash Ring
//--------------------------------------------------------------------------
struct SplashRing
{
   Vector <SplashRingPoint> points;
   ColorF   color;
   F32      lifetime;
   F32      elapsedTime;
   F32      v;

   SplashRing()
   {
      color.set( 0.0, 0.0, 0.0, 1.0 );
      lifetime = 0.0;
      elapsedTime = 0.0;
      v = 0.0;
   }

   bool isActive()
   {
      return elapsedTime < lifetime;
   }
};

//--------------------------------------------------------------------------
// Splash Data
//--------------------------------------------------------------------------
class SplashData : public GameBaseData {
  public:
   typedef GameBaseData Parent;

   enum Constants
   {
      NUM_EMITTERS = 3,
      NUM_TIME_KEYS = 4,
      NUM_TEX = 2,
   };

  public:

   AudioProfile*           soundProfile;
   S32                     soundProfileId;

   ParticleEmitterData*    emitterList[NUM_EMITTERS];
   S32                     emitterIDList[NUM_EMITTERS];

   S32               delayMS;
   S32               delayVariance;
   S32               lifetimeMS;
   S32               lifetimeVariance;
   Point3F           scale;
   F32               width;
   F32               height;
   U32               numSegments;
   F32               velocity;
   F32               acceleration;
   F32               texWrap;
   F32               texFactor;

   F32               ejectionFreq;
   F32               ejectionAngle;
   F32               ringLifetime;
   F32               startRadius;

   F32               times[ NUM_TIME_KEYS ];
   ColorF            colors[ NUM_TIME_KEYS ];

   StringTableEntry  textureName[NUM_TEX];
   GFXTexHandle     textureHandle[NUM_TEX];

   ExplosionData*    explosion;
   S32               explosionId;

   SplashData();
   DECLARE_CONOBJECT(SplashData);
   bool onAdd();
   bool preload(bool server, char errorBuffer[256]);
   static void  initPersistFields();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
};


//--------------------------------------------------------------------------
// Splash
//--------------------------------------------------------------------------
class Splash : public GameBase
{
   typedef GameBase Parent;

  private:
   SplashData*    mDataBlock;

   ParticleEmitter * mEmitterList[ SplashData::NUM_EMITTERS ];

   LList <SplashRing> ringList;

   U32         mCurrMS;
   U32         mEndingMS;
   F32         mRandAngle;
   F32         mRadius;
   F32         mVelocity;
   F32         mHeight;
   ColorF      mColor;
   F32         mTimeSinceLastRing;
   bool        mDead;
   F32         mElapsedTime;

  protected:
   Point3F     mInitialPosition;
   Point3F     mInitialNormal;
   F32         mFade;
   F32         mFog;
   bool        mActive;
   S32         mDelayMS;

  protected:
   bool        onAdd();
   void        onRemove();
   void        processTick(const Move *move);
   void        advanceTime(F32 dt);
   void        updateEmitters( F32 dt );
   void        updateWave( F32 dt );
   void        updateColor();
   SplashRing  createRing();
   void        updateRings( F32 dt );
   void        updateRing( SplashRing *ring, F32 dt );
   void        emitRings( F32 dt );
   void        render();
   void        renderSegment( SplashRing &top, SplashRing &bottom );
   void        spawnExplosion();

   // Rendering
  protected:
   bool prepRenderImage  ( SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState=false);
   void renderObject     ( SceneState *state, SceneRenderImage *image);

  public:
   Splash();
   ~Splash();
   void setInitialState(const Point3F& point, const Point3F& normal, const F32 fade = 1.0);

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream* stream);
   void unpackUpdate(NetConnection *conn,           BitStream* stream);

   bool onNewDataBlock(GameBaseData* dptr);
   DECLARE_CONOBJECT(Splash);
};


#endif // _H_SPLASH
