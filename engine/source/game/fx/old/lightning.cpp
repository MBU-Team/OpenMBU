//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/fx/lightning.h"

#include "dgl/dgl.h"
#include "sceneGraph/sceneState.h"
#include "console/consoleTypes.h"
#include "math/mathIO.h"
#include "core/bitStream.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "math/mRandom.h"
#include "math/mathUtils.h"
#include "audio/audioDataBlock.h"
#include "platform/platformAudio.h"
#include "terrain/terrData.h"
#include "sceneGraph/sceneGraph.h"
#include "game/player.h"
#include "game/camera.h"


IMPLEMENT_CO_DATABLOCK_V1(LightningData);
IMPLEMENT_CO_NETOBJECT_V1(Lightning);

MRandomLCG sgLightningRand;

ConsoleMethod( Lightning, warningFlashes, void, 2, 2, "")
{
   if (object->isServerObject()) object->warningFlashes();
}

ConsoleMethod( Lightning, strikeRandomPoint, void, 2, 2, "")
{
   if (object->isServerObject()) object->strikeRandomPoint();
}

ConsoleMethod( Lightning, strikeObject, void, 3, 3, "(ShapeBase id)")
{
   S32 id = dAtoi(argv[2]);
   ShapeBase* pSB;

   if (object->isServerObject() && Sim::findObject(id, pSB))
      object->strikeObject(pSB);
}

S32 QSORT_CALLBACK cmpSounds(const void* p1, const void* p2)
{
   U32 i1 = *((const S32*)p1);
   U32 i2 = *((const S32*)p2);

   if (i1 < i2) {
      return 1;
   } else if (i1 > i2) {
      return -1;
   } else {
      return 0;
   }
}

//--------------------------------------------------------------------------
//--------------------------------------
//
class LightningStrikeEvent : public NetEvent
{
   typedef NetEvent Parent;

  public:
   enum EventType {
      WarningFlash   = 0,
      Strike         = 1,
      TargetedStrike = 2,

      TypeMin        = WarningFlash,
      TypeMax        = TargetedStrike
   };
   enum Constants {
      PositionalBits = 10
   };

   Point2F                   mStart;
   SimObjectPtr<SceneObject> mTarget;

   Lightning*                mLightning;

   // Set by unpack...
  public:
   S32                       mClientId;

  public:
   LightningStrikeEvent();
   ~LightningStrikeEvent();

   void pack(NetConnection*, BitStream*);
   void write(NetConnection*, BitStream*){}
   void unpack(NetConnection*, BitStream*);
   void process(NetConnection*);

   DECLARE_CONOBJECT(LightningStrikeEvent);
};
IMPLEMENT_CO_CLIENTEVENT_V1(LightningStrikeEvent);

LightningStrikeEvent::LightningStrikeEvent()
{
   mLightning = NULL;
   mTarget = NULL;
}

LightningStrikeEvent::~LightningStrikeEvent()
{

}

void LightningStrikeEvent::pack(NetConnection* con, BitStream* stream)
{
   if(!mLightning)
   {
      stream->writeFlag(false);
      return;
   }
   S32 id = con->getGhostIndex(mLightning);
   if(id == -1)
   {
      stream->writeFlag(false);
      return;
   }
   stream->writeFlag(true);
   stream->writeRangedU32(U32(id), 0, NetConnection::MaxGhostCount);
   stream->writeFloat(mStart.x, PositionalBits);
   stream->writeFloat(mStart.y, PositionalBits);

   if( mTarget )
   {
      S32 ghostIndex = con->getGhostIndex(mTarget);
      if (ghostIndex == -1)
         stream->writeFlag(false);
      else
      {
         stream->writeFlag(true);
         stream->writeRangedU32(U32(ghostIndex), 0, NetConnection::MaxGhostCount);
      }
   }
   else
      stream->writeFlag( false );
}

void LightningStrikeEvent::unpack(NetConnection* con, BitStream* stream)
{
   if(!stream->readFlag())
      return;
   S32 mClientId = stream->readRangedU32(0, NetConnection::MaxGhostCount);
   mLightning = NULL;
   NetObject* pObject = con->resolveGhost(mClientId);
   if (pObject)
      mLightning = dynamic_cast<Lightning*>(pObject);

   mStart.x = stream->readFloat(PositionalBits);
   mStart.y = stream->readFloat(PositionalBits);

   if( stream->readFlag() )
   {
      // target id
      S32 mTargetID    = stream->readRangedU32(0, NetConnection::MaxGhostCount);

      NetObject* pObject = con->resolveGhost(mTargetID);
      if( pObject != NULL )
      {
         mTarget = dynamic_cast<SceneObject*>(pObject);
      }
      if( bool(mTarget) == false )
      {
         Con::errorf(ConsoleLogEntry::General, "LightningStrikeEvent::unpack: could not resolve target ghost properly");
      }

   }

}

void LightningStrikeEvent::process(NetConnection*)
{
   if (mLightning)
      mLightning->processEvent(this);
}


//--------------------------------------------------------------------------
//--------------------------------------
//
LightningData::LightningData()
{
   strikeSound = NULL;
   strikeSoundID = -1;

   dMemset( strikeTextureNames, 0, sizeof( strikeTextureNames ) );
   dMemset( strikeTextures, 0, sizeof( strikeTextures ) );

   U32 i;
   for (i = 0; i < MaxThunders; i++) {
      thunderSounds[i]   = NULL;
      thunderSoundIds[i] = -1;
   }
}

LightningData::~LightningData()
{

}


//--------------------------------------------------------------------------
void LightningData::initPersistFields()
{
   Parent::initPersistFields();

   addField("strikeSound",     TypeAudioProfilePtr, Offset(strikeSound,        LightningData));
   addField("thunderSounds",   TypeAudioProfilePtr, Offset(thunderSounds,      LightningData), MaxThunders);
   addField("strikeTextures",  TypeString,          Offset(strikeTextureNames, LightningData), MaxTextures);
}


//--------------------------------------------------------------------------
bool LightningData::onAdd()
{
   if(!Parent::onAdd())
      return false;

   for (U32 i = 0; i < MaxThunders; i++) {
      if (!thunderSounds[i] && thunderSoundIds[i] != -1) {
         if (Sim::findObject(thunderSoundIds[i], thunderSounds[i]) == false)
            Con::errorf(ConsoleLogEntry::General, "LightningData::onAdd: Invalid packet, bad datablockId(sound: %d", thunderSounds[i]);
      }
   }

   if( !strikeSound && strikeSoundID != -1 )
   {
      if( Sim::findObject( strikeSoundID, strikeSound ) == false)
         Con::errorf(ConsoleLogEntry::General, "LightningData::onAdd: Invalid packet, bad datablockId(sound: %d", strikeSound);
   }

   return true;
}


bool LightningData::preload(bool server, char errorBuffer[256])
{
   if (Parent::preload(server, errorBuffer) == false)
      return false;

   dQsort(thunderSounds, MaxThunders, sizeof(AudioProfile*), cmpSounds);
   for (numThunders = 0; numThunders < MaxThunders && thunderSounds[numThunders] != NULL; numThunders++) {
      //
   }

   if (server == false) {
      for (U32 i = 0; i < MaxTextures; i++) {
         strikeTextures[i] = TextureHandle(strikeTextureNames[i], MeshTexture);
      }
   }


   return true;
}


//--------------------------------------------------------------------------
void LightningData::packData(BitStream* stream)
{
   Parent::packData(stream);

   U32 i;
   for (i = 0; i < MaxThunders; i++) {
      if (stream->writeFlag(thunderSounds[i] != NULL)) {
         stream->writeRangedU32(thunderSounds[i]->getId(), DataBlockObjectIdFirst,
                                                           DataBlockObjectIdLast);
      }
   }
   for (i = 0; i < MaxTextures; i++) {
      stream->writeString(strikeTextureNames[i]);
   }

   if( stream->writeFlag( strikeSound != NULL) )
   {
      stream->writeRangedU32( strikeSound->getId(), DataBlockObjectIdFirst,
                                                    DataBlockObjectIdLast);
   }
}

void LightningData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   U32 i;
   for (i = 0; i < MaxThunders; i++) {
      if (stream->readFlag())
         thunderSoundIds[i] = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
      else
         thunderSoundIds[i] = -1;
   }
   for (i = 0; i < MaxTextures; i++) {
      strikeTextureNames[i] = stream->readSTString();
   }

   if (stream->readFlag())
      strikeSoundID = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   else
      strikeSoundID = -1;
}


//--------------------------------------------------------------------------
//--------------------------------------
//
Lightning::Lightning()
{
   mNetFlags.set(Ghostable|ScopeAlways);
   mTypeMask |= StaticObjectType|EnvironmentObjectType;

   mLastThink = 0;

   mStrikeListHead  = NULL;
   mThunderListHead = NULL;

   strikesPerMinute = 12;
   strikeWidth = 2.5;
   chanceToHitTarget = 0.5;
   strikeRadius = 20.0;
   boltStartRadius = 20.0;
   color.set( 1.0, 1.0, 1.0, 1.0 );
   fadeColor.set( 0.1, 0.1, 1.0, 1.0 );
   useFog = true;

   setScale( VectorF( 512, 512, 300 ) );
}

Lightning::~Lightning()
{
   //
}

//--------------------------------------------------------------------------
void Lightning::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Strikes");	// MM: Added Group Header.
   addField("strikesPerMinute",TypeS32,             Offset(strikesPerMinute,   Lightning));
   addField("strikeWidth",     TypeF32,             Offset(strikeWidth,        Lightning));
   addField("strikeRadius",    TypeF32,             Offset(strikeRadius,       Lightning));
   endGroup("Strikes");	// MM: Added Group Footer.

   addGroup("Colors");	// MM: Added Group Header.
   addField("color",           TypeColorF,          Offset(color,              Lightning));
   addField("fadeColor",       TypeColorF,          Offset(fadeColor,          Lightning));
   endGroup("Colors");	// MM: Added Group Footer.

   addGroup("Bolts");	// MM: Added Group Header.
   addField("chanceToHitTarget", TypeF32,           Offset(chanceToHitTarget,  Lightning));
   addField("boltStartRadius", TypeF32,             Offset(boltStartRadius,    Lightning));
   addField("useFog",          TypeBool,            Offset(useFog,             Lightning));
   endGroup("Bolts");	// MM: Added Group Footer.
}

//--------------------------------------------------------------------------
bool Lightning::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mObjBox.min.set( -0.5, -0.5, -0.5 );
   mObjBox.max.set(  0.5,  0.5,  0.5 );

   resetWorldBox();
   addToScene();

   return true;
}


void Lightning::onRemove()
{
   removeFromScene();

   Parent::onRemove();
}


bool Lightning::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<LightningData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   scriptOnNewDataBlock();
   return true;
}


//--------------------------------------------------------------------------
bool Lightning::prepRenderImage(SceneState* state, const U32 stateKey,
                                const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
   if (isLastState(state, stateKey))
      return false;
   setLastState(state, stateKey);

   // This should be sufficient for most objects that don't manage zones, and
   //  don't need to return a specialized RenderImage...
   if (state->isObjectRendered(this)) {
      SceneRenderImage* image = new SceneRenderImage;
      image->obj = this;
      image->isTranslucent = true;
      image->sortType = SceneRenderImage::EndSort;
      state->insertRenderImage(image);
   }

   return false;
}


void Lightning::renderObject(SceneState* state, SceneRenderImage*)
{
   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

   RectI viewport;
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   dglGetViewport(&viewport);

   // Uncomment this if this is a "simple" (non-zone managing) object
   state->setupObjectProjection(this);


   // RENDER CODE HERE
   MatrixF mv;
   dglGetModelview(&mv);
   Point3F camAxis;
   mv.getRow(1, &camAxis);

   glDisable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glDepthMask( GL_FALSE );

   if( useFog )
   {

      if (dglDoesSupportARBMultitexture() && dglDoesSupportFogCoord()) {
         glEnable(GL_FOG);
         glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
         GLfloat fogColor[4];
         fogColor[0] = state->getFogColor().red;
         fogColor[1] = state->getFogColor().green;
         fogColor[2] = state->getFogColor().blue;
         fogColor[3] = 0.5f;
         glFogfv(GL_FOG_COLOR, fogColor);
         glFogi(GL_FOG_MODE, GL_LINEAR);
         glFogf(GL_FOG_START, 0.0f);
         glFogf(GL_FOG_END, 1.0f);
      }
   }

   Strike* walk = mStrikeListHead;
   while (walk != NULL) {

      glBindTexture(GL_TEXTURE_2D, mDataBlock->strikeTextures[0].getGLName());

      for( U32 i=0; i<3; i++ )
      {
         if( walk->bolt[i].isFading )
         {
            F32 alpha = 1.0 - walk->bolt[i].percentFade;
            if( alpha < 0.0 ) alpha = 0.0;
            glColor4f( fadeColor.red, fadeColor.green, fadeColor.blue, alpha );
         }
         else
         {
            glColor4fv( color );
         }
         walk->bolt[i].render( state->getCameraPosition() );
      }


      walk = walk->next;
   }

   glDepthMask( GL_TRUE );

   glDisable(GL_FOG);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

void Lightning::scheduleThunder(Strike* newStrike)
{
   AssertFatal(isClientObject(), "Lightning::scheduleThunder: server objects should not enter this version of the function");

   // If no thunder sounds, don't schedule anything!
   if (mDataBlock->numThunders == 0)
      return;

   GameConnection* connection = GameConnection::getConnectionToServer();
   if (connection) {
      MatrixF cameraMatrix;

      if (connection->getControlCameraTransform(0, &cameraMatrix)) {
         Point3F worldPos;
         cameraMatrix.getColumn(3, &worldPos);

         worldPos.x -= newStrike->xVal;
         worldPos.y -= newStrike->yVal;
         worldPos.z  = 0;

         F32 dist = worldPos.len();
         F32 t    = dist / 330.0;

         // Ok, we need to schedule a random strike sound t secs in the future...
         //
         if (t <= 0.03) {
            // If it's really close, just play it...
            U32 thunder = sgLightningRand.randI(0, mDataBlock->numThunders - 1);
            alxPlay(mDataBlock->thunderSounds[thunder]);
         } else {
            Thunder* pThunder = new Thunder;
            pThunder->tRemaining = t;
            pThunder->next       = mThunderListHead;
            mThunderListHead     = pThunder;
         }
      }
   }
}


//--------------------------------------------------------------------------
void Lightning::processTick(const Move* move)
{
   Parent::processTick(move);

   if (isServerObject()) {
      S32 msBetweenStrikes = (S32)(60.0 / strikesPerMinute * 1000.0);

      mLastThink += TickMs;
      if( mLastThink > msBetweenStrikes )
      {
         strikeRandomPoint();
         mLastThink -= msBetweenStrikes;
      }
   }
}

void Lightning::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);
}

void Lightning::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   Strike** pWalker = &mStrikeListHead;
   while (*pWalker != NULL) {
      Strike* pStrike = *pWalker;

      for( U32 i=0; i<3; i++ )
      {
         pStrike->bolt[i].update( dt );
      }

      pStrike->currentAge += dt;
      if (pStrike->currentAge > pStrike->deathAge) {
         *pWalker = pStrike->next;
         delete pStrike;
      } else {
         pWalker = &((*pWalker)->next);
      }
   }

   Thunder** pThunderWalker = &mThunderListHead;
   while (*pThunderWalker != NULL) {
      Thunder* pThunder = *pThunderWalker;

      pThunder->tRemaining -= dt;
      if (pThunder->tRemaining <= 0.0) {
         *pThunderWalker = pThunder->next;
         delete pThunder;

         // Play the sound...
         U32 thunder = sgLightningRand.randI(0, mDataBlock->numThunders - 1);
         alxPlay(mDataBlock->thunderSounds[thunder]);
      } else {
         pThunderWalker = &((*pThunderWalker)->next);
      }
   }
}


//--------------------------------------------------------------------------
void Lightning::processEvent(LightningStrikeEvent* pEvent)
{
      AssertFatal(pEvent->mStart.x >= 0 && pEvent->mStart.x <= 1.0, "Out of bounds coord!");

      Strike* pStrike = new Strike;

      Point3F strikePoint;
      strikePoint.zero();

      if( pEvent->mTarget )
      {
         Point3F objectCenter;
         pEvent->mTarget->getObjBox().getCenter( &objectCenter );
         objectCenter.convolve( pEvent->mTarget->getScale() );
         pEvent->mTarget->getTransform().mulP( objectCenter );

         strikePoint = objectCenter;
      }
      else
      {
         strikePoint.x = pEvent->mStart.x;
         strikePoint.y = pEvent->mStart.y;
         strikePoint *= mObjScale;
         strikePoint += getPosition();
         strikePoint += Point3F( -mObjScale.x * 0.5, -mObjScale.y * 0.5, 0.0 );

         RayInfo rayInfo;
         Point3F start = strikePoint;
         start.z = mObjScale.z * 0.5 + getPosition().z;
         strikePoint.z += -mObjScale.z * 0.5;
         bool rayHit = gClientContainer.castRay( start, strikePoint,
                                      (STATIC_COLLISION_MASK | WaterObjectType),
                                      &rayInfo);
         if( rayHit )
         {
            strikePoint.z = rayInfo.point.z;
         }
         else
         {
            strikePoint.z = pStrike->bolt[0].findHeight( strikePoint, mSceneManager );
         }
      }

      pStrike->xVal       = strikePoint.x;
      pStrike->yVal       = strikePoint.y;

      pStrike->deathAge   = 1.6;
      pStrike->currentAge = 0.0;
      pStrike->next       = mStrikeListHead;

      for( U32 i=0; i<3; i++ )
      {
         F32 randStart = boltStartRadius;
         F32 height = mObjScale.z * 0.5 + getPosition().z;
         pStrike->bolt[i].startPoint = Point3F( pStrike->xVal + gRandGen.randF( -randStart, randStart ), pStrike->yVal + gRandGen.randF( -randStart, randStart ), height );
         pStrike->bolt[i].endPoint = strikePoint;
         pStrike->bolt[i].width = strikeWidth;
         pStrike->bolt[i].numMajorNodes = 10;
         pStrike->bolt[i].maxMajorAngle = 30;
         pStrike->bolt[i].numMinorNodes = 4;
         pStrike->bolt[i].maxMinorAngle = 15;
         pStrike->bolt[i].generate();
         pStrike->bolt[i].startSplits();
         pStrike->bolt[i].lifetime = 1.0;
         pStrike->bolt[i].fadeTime = 0.2;
         pStrike->bolt[i].renderTime = gRandGen.randF(0.0, 0.25);
      }

      mStrikeListHead     = pStrike;

      scheduleThunder(pStrike);

      MatrixF trans(true);
      trans.setPosition( strikePoint );

      if (mDataBlock->strikeSound)
      {
         alxPlay(mDataBlock->strikeSound, &trans );
      }

}

void Lightning::warningFlashes()
{
   AssertFatal(isServerObject(), "Error, client objects may not initiate lightning!");


   SimGroup* pClientGroup = Sim::getClientGroup();
   for (SimGroup::iterator itr = pClientGroup->begin(); itr != pClientGroup->end(); itr++) {
      NetConnection* nc = static_cast<NetConnection*>(*itr);
      if (nc != NULL)
      {
         LightningStrikeEvent* pEvent = new LightningStrikeEvent;
         pEvent->mLightning = this;

         nc->postNetEvent(pEvent);
      }
   }
}

void Lightning::strikeRandomPoint()
{
   AssertFatal(isServerObject(), "Error, client objects may not initiate lightning!");


   Point3F strikePoint;
   strikePoint.x = gRandGen.randF( 0.0, 1.0 );
   strikePoint.y = gRandGen.randF( 0.0, 1.0 );
   strikePoint.z = 0.0;

   // check if an object is within target range

   strikePoint *= mObjScale;
   strikePoint += getPosition();
   strikePoint += Point3F( -mObjScale.x * 0.5, -mObjScale.y * 0.5, 0.0 );

   Box3F queryBox;
   F32 boxWidth = strikeRadius * 2;

   queryBox.min.set( -boxWidth * 0.5, -boxWidth * 0.5, -mObjScale.z * 0.5 );
   queryBox.max.set(  boxWidth * 0.5,  boxWidth * 0.5,  mObjScale.z * 0.5 );
   queryBox.min += strikePoint;
   queryBox.max += strikePoint;

   SimpleQueryList sql;
   getContainer()->findObjects(queryBox, DAMAGEABLE_MASK,
                               SimpleQueryList::insertionCallback, &sql);

   SceneObject *highestObj = NULL;
   F32 highestPnt = 0.0;

   for( U32 i = 0; i < sql.mList.size(); i++ )
   {
      Point3F objectCenter;
      sql.mList[i]->getObjBox().getCenter(&objectCenter);
      objectCenter.convolve(sql.mList[i]->getScale());
      sql.mList[i]->getTransform().mulP(objectCenter);

      // check if object can be struck

      RayInfo rayInfo;
      Point3F start = objectCenter;
      start.z = mObjScale.z * 0.5 + getPosition().z;
      Point3F end = objectCenter;
      end.z = -mObjScale.z * 0.5 + getPosition().z;
      bool rayHit = gServerContainer.castRay( start, end,
                                   (0xFFFFFFFF),
                                   &rayInfo);

      if( rayHit && rayInfo.object == sql.mList[i] )
      {
         if( !highestObj )
         {
            highestObj = sql.mList[i];
            highestPnt = objectCenter.z;
            continue;
         }

         if( objectCenter.z > highestPnt )
         {
            highestObj = sql.mList[i];
            highestPnt = objectCenter.z;
         }
      }


   }

   // hah haaaaa, we have a target!
   SceneObject *targetObj = NULL;
   if( highestObj )
   {
      F32 chance = gRandGen.randF();
      if( chance <= chanceToHitTarget )
      {
         Point3F objectCenter;
         highestObj->getObjBox().getCenter(&objectCenter);
         objectCenter.convolve(highestObj->getScale());
         highestObj->getTransform().mulP(objectCenter);

         bool playerInWarmup = false;
         Player *playerObj = dynamic_cast< Player * >(highestObj);
         if( playerObj )
         {
            if( !playerObj->getControllingClient() )
            {
               playerInWarmup = true;
            }
         }

         if( !playerInWarmup )
         {
            applyDamage( objectCenter, VectorF( 0.0, 0.0, 1.0 ), highestObj );
            targetObj = highestObj;
         }
      }
   }

   SimGroup* pClientGroup = Sim::getClientGroup();
   for (SimGroup::iterator itr = pClientGroup->begin(); itr != pClientGroup->end(); itr++)
   {
      NetConnection* nc = static_cast<NetConnection*>(*itr);

      LightningStrikeEvent* pEvent = new LightningStrikeEvent;
      pEvent->mLightning = this;

      pEvent->mStart.x = strikePoint.x;
      pEvent->mStart.y = strikePoint.y;
      pEvent->mTarget = targetObj;

      nc->postNetEvent(pEvent);
   }


}

//--------------------------------------------------------------------------
void Lightning::strikeObject(ShapeBase*)
{
   AssertFatal(isServerObject(), "Error, client objects may not initiate lightning!");

   AssertFatal(false, "Lightning::strikeObject is not implemented.");
}


//--------------------------------------------------------------------------
U32 Lightning::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   // Only write data if this is the initial packet or we've been inspected.
   if (stream->writeFlag(mask & (InitialUpdateMask | ExtendedInfoMask)))
   {
      // Initial update
      mathWrite(*stream, getPosition());
      mathWrite(*stream, mObjScale);

      stream->write(strikeWidth);
      stream->write(chanceToHitTarget);
      stream->write(strikeRadius);
      stream->write(boltStartRadius);
      stream->write(color.red);
      stream->write(color.green);
      stream->write(color.blue);
      stream->write(fadeColor.red);
      stream->write(fadeColor.green);
      stream->write(fadeColor.blue);
      stream->write(useFog);
      stream->write(strikesPerMinute);
   }

   return retMask;
}

//--------------------------------------------------------------------------
void Lightning::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag())
   {
      // Initial update
      Point3F pos;
      mathRead(*stream, &pos);
      setPosition( pos );

      mathRead(*stream, &mObjScale);

      stream->read(&strikeWidth);
      stream->read(&chanceToHitTarget);
      stream->read(&strikeRadius);
      stream->read(&boltStartRadius);
      stream->read(&color.red);
      stream->read(&color.green);
      stream->read(&color.blue);
      stream->read(&fadeColor.red);
      stream->read(&fadeColor.green);
      stream->read(&fadeColor.blue);
      stream->read(&useFog);
      stream->read(&strikesPerMinute);
   }
}

//--------------------------------------------------------------------------
void Lightning::applyDamage( const Point3F& hitPosition,
                             const Point3F& hitNormal,
                             SceneObject*   hitObject)
{
   if (!isClientObject() && hitObject != NULL)
   {
      char *posArg = Con::getArgBuffer(64);
      char *normalArg = Con::getArgBuffer(64);

      dSprintf(posArg, 64, "%f %f %f", hitPosition.x, hitPosition.y, hitPosition.z);
      dSprintf(normalArg, 64, "%f %f %f", hitNormal.x, hitNormal.y, hitNormal.z);

      Con::executef(mDataBlock, 5, "applyDamage",
         Con::getIntArg(getId()),
         Con::getIntArg(hitObject->getId()),
         posArg,
         normalArg);
   }
}

//**************************************************************************
// Lightning Bolt
//**************************************************************************
LightningBolt::LightningBolt()
{
   width = 0.1;
   startPoint.zero();
   endPoint.zero();
   chanceOfSplit = 0.0;
   isFading = false;
   elapsedTime = 0.0;
   lifetime = 1.0;
   startRender = false;
}

//--------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
LightningBolt::~LightningBolt()
{
   splitList.free();
}

//--------------------------------------------------------------------------
// Generate nodes
//--------------------------------------------------------------------------
void LightningBolt::NodeManager::generateNodes()
{
   F32 overallDist = VectorF( endPoint - startPoint ).magnitudeSafe();
   F32 minDistBetweenNodes = overallDist / (numNodes-1);
   F32 maxDistBetweenNodes = minDistBetweenNodes / mCos( maxAngle * M_PI / 180.0 );

   VectorF mainLineDir = endPoint - startPoint;
   mainLineDir.normalizeSafe();

   for( U32 i=0; i<numNodes; i++ )
   {
      Node node;

      if( i == 0 )
      {
         node.point = startPoint;
         node.dirToMainLine = mainLineDir;
         nodeList[i] = node;
         continue;
      }
      if( i == numNodes - 1 )
      {
         node.point = endPoint;
         nodeList[i] = node;
         break;
      }

      Node lastNode = nodeList[i-1];

      F32 segmentLength = gRandGen.randF( minDistBetweenNodes, maxDistBetweenNodes );
      VectorF segmentDir = MathUtils::randomDir( lastNode.dirToMainLine, 0, maxAngle );
      node.point = lastNode.point + segmentDir * segmentLength;

      node.dirToMainLine = endPoint - node.point;
      node.dirToMainLine.normalizeSafe();
      nodeList[i] = node;
   }
}


//--------------------------------------------------------------------------
// Render bolt
//--------------------------------------------------------------------------
void LightningBolt::render( const Point3F &camPos )
{
   if( !startRender )
   {
      return;
   }

   if( !isFading )
   {
      generateMinorNodes();
   }

   glBegin( GL_TRIANGLE_STRIP );

   U32 i;
   for( i=0; i<mMinorNodes.size(); i++ )
   {
      if( i+1 == mMinorNodes.size() )
      {
         renderSegment( mMinorNodes[i], camPos, true );
      }
      else
      {
         renderSegment( mMinorNodes[i], camPos, false );
      }
   }

   glEnd();


   LightningBolt *curBolt = NULL;
   for( curBolt = splitList.next( curBolt ); curBolt; curBolt = splitList.next( curBolt ) )
   {
      if( isFading )
      {
         curBolt->isFading = true;
      }
      curBolt->render( camPos );
   }


}

//--------------------------------------------------------------------------
// Render segment
//--------------------------------------------------------------------------
void LightningBolt::renderSegment( NodeManager &segment, const Point3F &camPos, bool renderLastPoint )
{

   for( int i=0; i<segment.numNodes; i++ )
   {
      Point3F  curPoint = segment.nodeList[i].point;

      Point3F  nextPoint;
      Point3F  segDir;

      if( i == (segment.numNodes-1) )
      {
         if( renderLastPoint )
         {
            segDir = curPoint - segment.nodeList[i-1].point;
         }
         else
         {
            continue;
         }
      }
      else
      {
         nextPoint = segment.nodeList[i+1].point;
         segDir = nextPoint - curPoint;
      }
      segDir.normalizeSafe();


      Point3F dirFromCam = curPoint - camPos;
      Point3F crossVec;
      mCross(dirFromCam, segDir, &crossVec);
      crossVec.normalize();
      crossVec *= width * 0.5;

      F32 u = i % 2;

      glTexCoord2f( u, 1.0 );
      glVertex3fv( curPoint - crossVec );

      glTexCoord2f( u, 0.0 );
      glVertex3fv( curPoint + crossVec );
   }

}

//----------------------------------------------------------------------------
// Find height
//----------------------------------------------------------------------------
F32 LightningBolt::findHeight( Point3F &point, SceneGraph *sceneManager )
{
   TerrainBlock* pTerrain = sceneManager->getCurrentTerrain();
   if( !pTerrain ) return 0.0;

   Point3F terrPt = point;
   pTerrain->getWorldTransform().mulP(terrPt);
   F32 h;
   if (pTerrain->getHeight(Point2F(terrPt.x, terrPt.y), &h))
   {
      return h;
   }


   return 0.0;
}


//----------------------------------------------------------------------------
// Generate lightning bolt
//----------------------------------------------------------------------------
void LightningBolt::generate()
{
   mMajorNodes.startPoint   = startPoint;
   mMajorNodes.endPoint     = endPoint;
   mMajorNodes.numNodes     = numMajorNodes;
   mMajorNodes.maxAngle     = maxMajorAngle;

   mMajorNodes.generateNodes();

   generateMinorNodes();

}

//----------------------------------------------------------------------------
// Generate Minor Nodes
//----------------------------------------------------------------------------
void LightningBolt::generateMinorNodes()
{
   mMinorNodes.clear();

   for( int i=0; i<mMajorNodes.numNodes - 1; i++ )
   {
      NodeManager segment;
      segment.startPoint = mMajorNodes.nodeList[i].point;
      segment.endPoint = mMajorNodes.nodeList[i+1].point;
      segment.numNodes = numMinorNodes;
      segment.maxAngle = maxMinorAngle;
      segment.generateNodes();

      mMinorNodes.increment(1);
      mMinorNodes[i] = segment;
   }
}

//----------------------------------------------------------------------------
// Recursive algo to create bolts that split off from main bolt
//----------------------------------------------------------------------------
void LightningBolt::createSplit( Point3F startPoint, Point3F endPoint, U32 depth, F32 width )
{
   if( depth == 0 ) return;
   F32 chanceToEnd = gRandGen.randF();
   if( chanceToEnd > 0.70 ) return;

   if( width < 0.75 ) width = 0.75;

   VectorF diff = endPoint - startPoint;
   F32 length = diff.len();
   diff.normalizeSafe();

   LightningBolt newBolt;
   newBolt.startPoint = startPoint;
   newBolt.endPoint = endPoint;
   newBolt.width = width;
   newBolt.numMajorNodes = 3;
   newBolt.maxMajorAngle = 30;
   newBolt.numMinorNodes = 3;
   newBolt.maxMinorAngle = 10;
   newBolt.startRender = true;
   newBolt.generate();

   splitList.link( newBolt );

   VectorF newDir1 = MathUtils::randomDir( diff, 10.0, 45.0 );
   Point3F newEndPoint1 = endPoint + newDir1 * gRandGen.randF( 0.5, 1.5 ) * length;

   VectorF newDir2 = MathUtils::randomDir( diff, 10.0, 45.0 );
   Point3F newEndPoint2 = endPoint + newDir2 * gRandGen.randF( 0.5, 1.5 ) * length;

   createSplit( endPoint, newEndPoint1, depth - 1, width * 0.30 );
   createSplit( endPoint, newEndPoint2, depth - 1, width * 0.30 );

}

//----------------------------------------------------------------------------
// Start split - kick off the recursive 'createSplit' procedure
//----------------------------------------------------------------------------
void LightningBolt::startSplits()
{

   for( U32 i=0; i<mMajorNodes.numNodes-1; i++ )
   {
      if( gRandGen.randF() > 0.3 ) continue;

      Node node = mMajorNodes.nodeList[i];
      Node node2 = mMajorNodes.nodeList[i+1];

      VectorF segDir = node2.point - node.point;
      F32 length = segDir.len();
      segDir.normalizeSafe();

      VectorF newDir = MathUtils::randomDir( segDir, 20.0, 40.0 );
      Point3F newEndPoint = node.point + newDir * gRandGen.randF( 0.5, 1.5 ) * length;


      createSplit( node.point, newEndPoint, 4, width * 0.30 );
   }


}

//----------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------
void LightningBolt::update( F32 dt )
{
   elapsedTime += dt;

   F32 percentDone = elapsedTime / lifetime;

   if( elapsedTime > fadeTime )
   {
      isFading = true;
      percentFade = percentDone + (fadeTime/lifetime);
   }

   if( elapsedTime > renderTime && !startRender )
   {
      startRender = true;
      isFading = false;
      elapsedTime = 0.0;
   }
}
