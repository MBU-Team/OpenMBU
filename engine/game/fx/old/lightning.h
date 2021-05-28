//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LIGHTNING_H_
#define _LIGHTNING_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _LLIST_H_
#include "core/llist.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class ShapeBase;
class LightningStrikeEvent;
class AudioProfile;


// -------------------------------------------------------------------------
class LightningData : public GameBaseData
{
   typedef GameBaseData Parent;

  public:
   enum Constants {
      MaxThunders = 8,
      MaxTextures = 8
   };

   //-------------------------------------- Console set variables
  public:
   AudioProfile*    thunderSounds[MaxThunders];
   AudioProfile*    strikeSound;
   StringTableEntry strikeTextureNames[MaxTextures];

   //-------------------------------------- load set variables
  public:
   S32           thunderSoundIds[MaxThunders];
   S32           strikeSoundID;

   TextureHandle strikeTextures[MaxTextures];
   U32           numThunders;

  protected:
   bool onAdd();


  public:
   LightningData();
   ~LightningData();

   void packData(BitStream*);
   void unpackData(BitStream*);
   bool preload(bool server, char errorBuffer[256]);

   DECLARE_CONOBJECT(LightningData);
   static void initPersistFields();
};


// -------------------------------------------------------------------------
struct LightningBolt
{

   struct Node
   {
      Point3F        point;
      VectorF        dirToMainLine;
   };

   struct NodeManager
   {
      Node     nodeList[10];

      Point3F  startPoint;
      Point3F  endPoint;
      U32      numNodes;
      F32      maxAngle;

      void generateNodes();
   };

   NodeManager mMajorNodes;
   Vector< NodeManager > mMinorNodes;
   LList< LightningBolt > splitList;

   F32      lifetime;
   F32      elapsedTime;
   F32      fadeTime;
   bool     isFading;
   F32      percentFade;
   bool     startRender;
   F32      renderTime;

   F32      width;
   F32      chanceOfSplit;
   Point3F  startPoint;
   Point3F  endPoint;

   U32      numMajorNodes;
   F32      maxMajorAngle;
   U32      numMinorNodes;
   F32      maxMinorAngle;

   LightningBolt();
   ~LightningBolt();

   void  createSplit( Point3F startPoint, Point3F endPoint, U32 depth, F32 width );
   F32   findHeight( Point3F &point, SceneGraph* sceneManager );
   void  render( const Point3F &camPos );
   void  renderSegment( NodeManager &segment, const Point3F &camPos, bool renderLastPoint );
   void  generate();
   void  generateMinorNodes();
   void  startSplits();
   void  update( F32 dt );

};


// -------------------------------------------------------------------------
class Lightning : public GameBase
{
   typedef GameBase Parent;

  protected:
   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData* dptr);

   struct Strike {
      F32     xVal;             // Position in cloud layer of strike
      F32     yVal;             //  top

      bool    targetedStrike;   // Is this a targeted strike?
      U32     targetGID;

      F32     deathAge;         // Age at which this strike expires
      F32     currentAge;       // Current age of this strike (updated by advanceTime)

      LightningBolt bolt[3];

      Strike* next;
   };
   struct Thunder {
      F32      tRemaining;
      Thunder* next;
   };

  public:

   //-------------------------------------- Console set variables
  public:

   U32      strikesPerMinute;
   F32      strikeWidth;
   F32      chanceToHitTarget;
   F32      strikeRadius;
   F32      boltStartRadius;
   ColorF   color;
   ColorF   fadeColor;
   bool     useFog;

   // Rendering
  protected:
   bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
   void renderObject(SceneState *state, SceneRenderImage *image);

   // Time management
   void processTick(const Move *move);
   void interpolateTick(F32 delta);
   void advanceTime(F32 dt);

   // Strike management
   void scheduleThunder(Strike*);

   // Data members
  private:
   LightningData* mDataBlock;

  protected:
   U32     mLastThink;        // Valid only on server

   Strike*  mStrikeListHead;   // Valid on on the client
   Thunder* mThunderListHead;

   static const U32 csmTargetMask;

  public:
   Lightning();
   ~Lightning();

   void applyDamage( const Point3F& hitPosition, const Point3F& hitNormal, SceneObject* hitObject );
   void warningFlashes();
   void strikeRandomPoint();
   void strikeObject(ShapeBase*);
   void processEvent(LightningStrikeEvent*);

   DECLARE_CONOBJECT(Lightning);
   static void initPersistFields();

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);
};

#endif // _H_LIGHTNING

