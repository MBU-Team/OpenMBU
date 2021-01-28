//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/fx/splash.h"

#include "console/consoleTypes.h"
#include "dgl/dgl.h"
#include "platform/platformAudio.h"
#include "audio/audioDataBlock.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "core/bitStream.h"
#include "math/mathIO.h"
#include "terrain/terrData.h"
#include "game/fx/explosion.h"
#include "game/fx/particleEmitter.h"

namespace
{

MRandomLCG sgRandom(0xdeadbeef);

} // namespace {}

//----------------------------------------------------------------------------
//--------------------------------------
//
IMPLEMENT_CO_DATABLOCK_V1(SplashData);
IMPLEMENT_CO_NETOBJECT_V1(Splash);

//--------------------------------------------------------------------------
// Splash Data
//--------------------------------------------------------------------------
SplashData::SplashData()
{
   soundProfile      = NULL;
   soundProfileId    = 0;

   scale.set(1, 1, 1);

   dMemset( emitterList, 0, sizeof( emitterList ) );
   dMemset( emitterIDList, 0, sizeof( emitterIDList ) );

   delayMS = 0;
   delayVariance = 0;
   lifetimeMS = 1000;
   lifetimeVariance = 0;
   width = 4.0;
   numSegments = 10;
   velocity = 5.0;
   height = 0.0;
   acceleration = 0.0;
   texWrap = 1.0;
   texFactor = 3.0;
   ejectionFreq = 5;
   ejectionAngle = 45.0;
   ringLifetime = 1.0;
   startRadius = 0.5;
   explosion = NULL;
   explosionId = 0;

   dMemset( textureName, 0, sizeof( textureName ) );

   U32 i;
   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      times[i] = 1.0;
   }
   times[0] = 0.0;

   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      colors[i].set( 1.0, 1.0, 1.0, 1.0 );
   }
}

IMPLEMENT_SETDATATYPE(SplashData)
   IMPLEMENT_GETDATATYPE(SplashData)

//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
   void SplashData::initPersistFields()
{
   Parent::initPersistFields();

   Con::registerType("SplashDataPtr", TypeSplashDataPtr, sizeof(SplashData*),
                     REF_GETDATATYPE(SplashData),
                     REF_SETDATATYPE(SplashData));


   addField("soundProfile",      TypeAudioProfilePtr,          Offset(soundProfile,       SplashData));
   addField("scale",             TypePoint3F,                  Offset(scale,              SplashData));
   addField("emitter",           TypeParticleEmitterDataPtr,   Offset(emitterList,        SplashData), NUM_EMITTERS);
   addField("delayMS",           TypeS32,                      Offset(delayMS,            SplashData));
   addField("delayVariance",     TypeS32,                      Offset(delayVariance,      SplashData));
   addField("lifetimeMS",        TypeS32,                      Offset(lifetimeMS,         SplashData));
   addField("lifetimeVariance",  TypeS32,                      Offset(lifetimeVariance,   SplashData));
   addField("width",             TypeF32,                      Offset(width,              SplashData));
   addField("numSegments",       TypeS32,                      Offset(numSegments,        SplashData));
   addField("velocity",          TypeF32,                      Offset(velocity,           SplashData));
   addField("height",            TypeF32,                      Offset(height,             SplashData));
   addField("acceleration",      TypeF32,                      Offset(acceleration,       SplashData));
   addField("times",             TypeF32,                      Offset(times,              SplashData), NUM_TIME_KEYS);
   addField("colors",            TypeColorF,                   Offset(colors,             SplashData), NUM_TIME_KEYS);
   addField("texture",           TypeFilename,                 Offset(textureName,        SplashData), NUM_TEX);
   addField("texWrap",           TypeF32,                      Offset(texWrap,            SplashData));
   addField("texFactor",         TypeF32,                      Offset(texFactor,          SplashData));
   addField("ejectionFreq",      TypeF32,                      Offset(ejectionFreq,       SplashData));
   addField("ejectionAngle",     TypeF32,                      Offset(ejectionAngle,      SplashData));
   addField("ringLifetime",      TypeF32,                      Offset(ringLifetime,       SplashData));
   addField("startRadius",       TypeF32,                      Offset(startRadius,        SplashData));
   addField("explosion",         TypeExplosionDataPtr,         Offset(explosion,          SplashData));

}

//--------------------------------------------------------------------------
// On add - verify data settings
//--------------------------------------------------------------------------
bool SplashData::onAdd()
{
   if (Parent::onAdd() == false)
      return false;

   return true;
}

//--------------------------------------------------------------------------
// Pack data
//--------------------------------------------------------------------------
void SplashData::packData(BitStream* stream)
{
   Parent::packData(stream);

   mathWrite(*stream, scale);
   stream->write(delayMS);
   stream->write(delayVariance);
   stream->write(lifetimeMS);
   stream->write(lifetimeVariance);
   stream->write(width);
   stream->write(numSegments);
   stream->write(velocity);
   stream->write(height);
   stream->write(acceleration);
   stream->write(texWrap);
   stream->write(texFactor);
   stream->write(ejectionFreq);
   stream->write(ejectionAngle);
   stream->write(ringLifetime);
   stream->write(startRadius);

   if( stream->writeFlag( explosion ) )
   {
      stream->writeRangedU32(explosion->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
   }

   S32 i;
   for( i=0; i<NUM_EMITTERS; i++ )
   {
      if( stream->writeFlag( emitterList[i] != NULL ) )
      {
         stream->writeRangedU32( emitterList[i]->getId(), DataBlockObjectIdFirst,  DataBlockObjectIdLast );
      }
   }

   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      stream->write( colors[i] );
   }

   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      stream->write( times[i] );
   }

   for( i=0; i<NUM_TEX; i++ )
   {
      stream->writeString(textureName[i]);
   }
}

//--------------------------------------------------------------------------
// Unpack data
//--------------------------------------------------------------------------
void SplashData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   mathRead(*stream, &scale);
   stream->read(&delayMS);
   stream->read(&delayVariance);
   stream->read(&lifetimeMS);
   stream->read(&lifetimeVariance);
   stream->read(&width);
   stream->read(&numSegments);
   stream->read(&velocity);
   stream->read(&height);
   stream->read(&acceleration);
   stream->read(&texWrap);
   stream->read(&texFactor);
   stream->read(&ejectionFreq);
   stream->read(&ejectionAngle);
   stream->read(&ringLifetime);
   stream->read(&startRadius);

   if( stream->readFlag() )
   {
      explosionId = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
   }

   U32 i;
   for( i=0; i<NUM_EMITTERS; i++ )
   {
      if( stream->readFlag() )
      {
         emitterIDList[i] = stream->readRangedU32( DataBlockObjectIdFirst, DataBlockObjectIdLast );
      }
   }

   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      stream->read( &colors[i] );
   }

   for( i=0; i<NUM_TIME_KEYS; i++ )
   {
      stream->read( &times[i] );
   }

   for( i=0; i<NUM_TEX; i++ )
   {
      textureName[i] = stream->readSTString();
   }
}

//--------------------------------------------------------------------------
// Preload data - load resources
//--------------------------------------------------------------------------
bool SplashData::preload(bool server, char errorBuffer[256])
{
   if (Parent::preload(server, errorBuffer) == false)
      return false;

   if (!server) {
      S32 i;
      for( i=0; i<NUM_EMITTERS; i++ )
      {
         if( !emitterList[i] && emitterIDList[i] != 0 )
         {
            if( Sim::findObject( emitterIDList[i], emitterList[i] ) == false)
            {
               Con::errorf( ConsoleLogEntry::General, "SplashData::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", emitterIDList[i] );
            }
         }
      }

      for( i=0; i<NUM_TEX; i++ )
      {
         if (textureName[i] && textureName[i][0])
         {
            textureHandle[i] = TextureHandle(textureName[i], MeshTexture );
         }
      }
   }

   if( !explosion && explosionId != 0 )
   {
      if( !Sim::findObject(explosionId, explosion) )
      {
         Con::errorf(ConsoleLogEntry::General, "SplashData::preload: Invalid packet, bad datablockId(explosion): %d", explosionId);
      }
   }

   return true;
}


//--------------------------------------------------------------------------
// Splash
//--------------------------------------------------------------------------
Splash::Splash()
{
   dMemset( mEmitterList, 0, sizeof( mEmitterList ) );

   mDelayMS = 0;
   mCurrMS = 0;
   mEndingMS = 1000;
   mActive = false;
   mRadius = 0.0;
   mVelocity = 1.0;
   mHeight = 0.0;
   mTimeSinceLastRing = 0.0;
   mDead = false;
   mElapsedTime = 0.0;

   mInitialNormal.set( 0.0, 0.0, 1.0 );
}

//--------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
Splash::~Splash()
{
}

//--------------------------------------------------------------------------
// Set initial state
//--------------------------------------------------------------------------
void Splash::setInitialState(const Point3F& point, const Point3F& normal, const F32 fade)
{
   mInitialPosition = point;
   mInitialNormal   = normal;
   mFade            = fade;
   mFog             = 0.0f;
}


//--------------------------------------------------------------------------
// OnAdd
//--------------------------------------------------------------------------
bool Splash::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mDelayMS = mDataBlock->delayMS + sgRandom.randI( -mDataBlock->delayVariance, mDataBlock->delayVariance );
   mEndingMS = mDataBlock->lifetimeMS + sgRandom.randI( -mDataBlock->lifetimeVariance, mDataBlock->lifetimeVariance );

   mVelocity = mDataBlock->velocity;
   mHeight = mDataBlock->height;
   mTimeSinceLastRing = 1.0 / mDataBlock->ejectionFreq;


   if( isClientObject() )
   {
      for( U32 i=0; i<SplashData::NUM_EMITTERS; i++ )
      {
         if( mDataBlock->emitterList[i] != NULL )
         {
            ParticleEmitter * pEmitter = new ParticleEmitter;
            pEmitter->onNewDataBlock( mDataBlock->emitterList[i] );
            if( !pEmitter->registerObject() )
            {
               Con::warnf( ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName() );
               delete pEmitter;
               pEmitter = NULL;
            }
            mEmitterList[i] = pEmitter;
         }
      }

      spawnExplosion();
   }


   mObjBox.min = Point3F( -1, -1, -1 );
   mObjBox.max = Point3F(  1,  1,  1 );
   resetWorldBox();

   gClientContainer.addObject(this);
   gClientSceneGraph->addObjectToScene(this);

   removeFromProcessList();
   gClientProcessList.addObject(this);

   return true;
}

//--------------------------------------------------------------------------
// OnRemove
//--------------------------------------------------------------------------
void Splash::onRemove()
{
   for( U32 i=0; i<SplashData::NUM_EMITTERS; i++ )
   {
      if( mEmitterList[i] )
      {
         mEmitterList[i]->deleteWhenEmpty();
         mEmitterList[i] = NULL;
      }
   }

   ringList.free();

   mSceneManager->removeObjectFromScene(this);
   getContainer()->removeObject(this);

   Parent::onRemove();
}


//--------------------------------------------------------------------------
// On New Data Block
//--------------------------------------------------------------------------
bool Splash::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<SplashData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   scriptOnNewDataBlock();
   return true;
}


//--------------------------------------------------------------------------
// Prep render image
//--------------------------------------------------------------------------
bool Splash::prepRenderImage(SceneState* state, const U32 stateKey,
                             const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
   if (isLastState(state, stateKey))
      return false;
   setLastState(state, stateKey);

   // This should be sufficient for most objects that don't manage zones, and
   //  don't need to return a specialized RenderImage...
   if (state->isObjectRendered(this)) {
      mFog = 0.0;

      SceneRenderImage* image = new SceneRenderImage;
      image->obj = this;
      image->isTranslucent = true;
      image->sortType = SceneRenderImage::Point;
      image->textureSortKey = (U32)(dsize_t)mDataBlock;
      state->setImageRefPoint(this, image);
      state->insertRenderImage(image);
   }

   return false;
}

//--------------------------------------------------------------------------
// Render
//--------------------------------------------------------------------------
void Splash::renderObject(SceneState* state, SceneRenderImage*)
{
   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

   RectI viewport;
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   dglGetViewport(&viewport);

   state->setupObjectProjection(this);

   render();

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

//--------------------------------------------------------------------------
// Process tick
//--------------------------------------------------------------------------
void Splash::processTick(const Move*)
{
   mCurrMS += TickMs;

   if( isServerObject() )
   {
      if( mCurrMS >= mEndingMS )
      {
         mDead = true;
         if( mCurrMS >= (mEndingMS + mDataBlock->ringLifetime * 1000) )
         {
            deleteObject();
         }
      }
   }
   else
   {
      if( mCurrMS >= mEndingMS )
      {
         mDead = true;
      }
   }
}

//--------------------------------------------------------------------------
// Advance time
//--------------------------------------------------------------------------
void Splash::advanceTime(F32 dt)
{
   if (dt == 0.0)
      return;

   mElapsedTime += dt;

   updateColor();
   updateWave( dt );
   updateEmitters( dt );
   updateRings( dt );

   if( !mDead )
   {
      emitRings( dt );
   }
}

//----------------------------------------------------------------------------
// Update emitters
//----------------------------------------------------------------------------
void Splash::updateEmitters( F32 dt )
{
   Point3F pos = getPosition();

   for( U32 i=0; i<SplashData::NUM_EMITTERS; i++ )
   {
      if( mEmitterList[i] )
      {
         mEmitterList[i]->emitParticles( pos, pos, mInitialNormal, Point3F( 0.0, 0.0, 0.0 ), (S32) (dt * 1000) );
      }
   }

}

//----------------------------------------------------------------------------
// Update wave
//----------------------------------------------------------------------------
void Splash::updateWave( F32 dt )
{
   mVelocity += mDataBlock->acceleration * dt;
   mRadius += mVelocity * dt;

}

//----------------------------------------------------------------------------
// Render splash
//----------------------------------------------------------------------------
void Splash::render()
{

   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);


   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glEnable(GL_TEXTURE_2D);


   SplashRing *ring = NULL;

   U32 i=0;
   while( bool( ring = ringList.next( ring ) ) )
   {
      SplashRing *next = ringList.next( ring );
      if( !next ) break;

      renderSegment( *ring, *next );
      i++;
   }


   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


}

//----------------------------------------------------------------------------
// Render horizontal segment created from 2 sets of points that are the
// top and bottom rings of the segment.
//----------------------------------------------------------------------------
void Splash::renderSegment( SplashRing &top, SplashRing &bottom )
{
   F32 texFactor = mDataBlock->texWrap;

   glBindTexture( GL_TEXTURE_2D, mDataBlock->textureHandle[0].getGLName() );


   F32 topAlpha = top.elapsedTime / top.lifetime;
   F32 bottomAlpha = bottom.elapsedTime / bottom.lifetime;

   if( topAlpha < 0.5 ) topAlpha *= 2.0;
   else topAlpha = 1.0 - topAlpha;

   if( bottomAlpha < 0.5 ) bottomAlpha *= 2.0;
   else bottomAlpha = 1.0 - bottomAlpha;

   top.color.alpha = topAlpha;
   bottom.color.alpha = bottomAlpha;


   glBegin( GL_QUAD_STRIP );
   {
      for( U32 i=0; i<top.points.size(); i++ )
      {
         F32 t = F32(i) / F32(top.points.size());

         glTexCoord2f( t * texFactor, top.v );
         glColor4fv( top.color );
         glVertex3fv( top.points[i].position );

         glTexCoord2f( t * texFactor, bottom.v );
         glColor4fv( bottom.color );
         glVertex3fv( bottom.points[i].position );
      }

      glTexCoord2f( 1.0 * texFactor, top.v );
      glColor4fv( top.color );
      glVertex3fv( top.points[0].position );

      glTexCoord2f( 1.0 * texFactor, bottom.v );
      glColor4fv( bottom.color );
      glVertex3fv( bottom.points[0].position );
   }
   glEnd();
}

//----------------------------------------------------------------------------
// Update color
//----------------------------------------------------------------------------
void Splash::updateColor()
{
   SplashRing *ring = NULL;

   while( bool( ring = ringList.next( ring ) ) )
   {
      F32 t = F32(ring->elapsedTime) / F32(ring->lifetime);

      for( U32 i = 1; i < SplashData::NUM_TIME_KEYS; i++ )
      {
         if( mDataBlock->times[i] >= t )
         {
            F32 firstPart =   t - mDataBlock->times[i-1];
            F32 total     =   (mDataBlock->times[i] -
                               mDataBlock->times[i-1]);

            firstPart /= total;

            ring->color.interpolate( mDataBlock->colors[i-1],
                                     mDataBlock->colors[i],
                                     firstPart);
            break;
         }
      }
   }
}

//----------------------------------------------------------------------------
// Create ring
//----------------------------------------------------------------------------
SplashRing Splash::createRing()
{

   SplashRing ring;
   U32 numPoints = mDataBlock->numSegments + 1;

   Point3F ejectionAxis( 0.0, 0.0, 1.0 );

   Point3F axisx;
   if (mFabs(ejectionAxis.z) < 0.999f)
      mCross(ejectionAxis, Point3F(0, 0, 1), &axisx);
   else
      mCross(ejectionAxis, Point3F(0, 1, 0), &axisx);
   axisx.normalize();



   for( U32 i=0; i<numPoints; i++ )
   {
      F32 t = F32(i) / F32(numPoints);

      AngAxisF thetaRot( axisx, mDataBlock->ejectionAngle * (M_PI / 180.0));
      AngAxisF phiRot(   ejectionAxis, t * (M_PI * 2.0));

      Point3F pointAxis = ejectionAxis;

      MatrixF temp;
      thetaRot.setMatrix(&temp);
      temp.mulP(pointAxis);
      phiRot.setMatrix(&temp);
      temp.mulP(pointAxis);

      Point3F startOffset = axisx;
      temp.mulV( startOffset );
      startOffset *= mDataBlock->startRadius;

      SplashRingPoint point;
      point.position = getPosition() + startOffset;
      point.velocity = pointAxis * mDataBlock->velocity;

      ring.points.push_back( point );
   }

   ring.color = mDataBlock->colors[0];
   ring.lifetime = mDataBlock->ringLifetime;
   ring.elapsedTime = 0.0;
   ring.v = mDataBlock->texFactor * mFmod( mElapsedTime, 1.0 );

   return ring;
}

//----------------------------------------------------------------------------
// Emit rings
//----------------------------------------------------------------------------
void Splash::emitRings( F32 dt )
{
   mTimeSinceLastRing += dt;

   S32 numNewRings = (S32) (mTimeSinceLastRing * F32(mDataBlock->ejectionFreq));

   mTimeSinceLastRing -= numNewRings / mDataBlock->ejectionFreq;


   for( S32 i=numNewRings-1; i>=0; i-- )
   {
      F32 t = F32(i) / F32(numNewRings);
      t *= dt;
      t += mTimeSinceLastRing;

      SplashRing ring = createRing();
      updateRing( &ring, t );

      ringList.link( ring );
   }
}

//----------------------------------------------------------------------------
// Update rings
//----------------------------------------------------------------------------
void Splash::updateRings( F32 dt )
{

   SplashRing *ring = NULL;
   while( bool( ring = ringList.next( ring ) ) )
   {
      ring->elapsedTime += dt;

      if( !ring->isActive() )
      {
         SplashRing *inactiveRing = ring;
         ring = ringList.prev( ring );
         ringList.free( inactiveRing );
      }
      else
      {
         updateRing( ring, dt );
      }
   }

}

//----------------------------------------------------------------------------
// Update ring
//----------------------------------------------------------------------------
void Splash::updateRing( SplashRing *ring, F32 dt )
{

   for( U32 i=0; i<ring->points.size(); i++ )
   {
      if( mDead )
      {
         Point3F vel = ring->points[i].velocity;
         vel.normalize();
         vel *= mDataBlock->acceleration;
         ring->points[i].velocity += vel * dt;
      }

      ring->points[i].velocity += Point3F( 0.0, 0.0, -9.8 ) * dt;
      ring->points[i].position += ring->points[i].velocity * dt;
   }
}

//----------------------------------------------------------------------------
// Explode
//----------------------------------------------------------------------------
void Splash::spawnExplosion()
{
   if( !mDataBlock->explosion ) return;

   Explosion* pExplosion = new Explosion;
   pExplosion->onNewDataBlock(mDataBlock->explosion);

   MatrixF trans = getTransform();
   trans.setPosition( getPosition() );

   pExplosion->setTransform( trans );
   pExplosion->setInitialState( trans.getPosition(), VectorF(0,0,1), 1);
   if (!pExplosion->registerObject())
      delete pExplosion;
}

//--------------------------------------------------------------------------
// packUpdate
//--------------------------------------------------------------------------
U32 Splash::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if( stream->writeFlag(mask & GameBase::InitialUpdateMask) )
   {
      mathWrite(*stream, mInitialPosition);
   }

   return retMask;
}

//--------------------------------------------------------------------------
// unpackUpdate
//--------------------------------------------------------------------------
void Splash::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if( stream->readFlag() )
   {
      mathRead(*stream, &mInitialPosition);
      setPosition( mInitialPosition );
   }
}
