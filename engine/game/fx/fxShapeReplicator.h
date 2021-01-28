//-----------------------------------------------------------------------------
// Torque Shader Engine
// Written by Melvyn May, 13th March 2002.
//-----------------------------------------------------------------------------

#ifndef _SHAPEREPLICATOR_H_
#define _SHAPEREPLICATOR_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _TSSTATIC_H_
#include "game/tsStatic.h"
#endif

#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif

#define AREA_ANIMATION_ARC         (1.0f / 360.0f)

#define FXREPLICATOR_COLLISION_MASK   (   TerrainObjectType      |   \
                              InteriorObjectType      |   \
                              StaticObjectType      |   \
                              WaterObjectType      )

#define FXREPLICATOR_NOWATER_COLLISION_MASK   (   TerrainObjectType      |   \
                                    InteriorObjectType      |   \
                                    StaticObjectType      )


//------------------------------------------------------------------------------
// Class: fxShapeReplicatedStatic
//------------------------------------------------------------------------------
class fxShapeReplicatedStatic : public TSStatic
{
private:
   typedef SceneObject         Parent;

public:
   fxShapeReplicatedStatic() {};
   ~fxShapeReplicatedStatic() {};
   void touchNetFlags(const U32 m, bool setflag = true) { if (setflag) mNetFlags.set(m); else mNetFlags.clear(m); };
   TSShape* getShape(void) { return mShapeInstance->getShape(); };
   void setTransform(const MatrixF & mat) { Parent::setTransform(mat); setRenderTransform(mat); };

   DECLARE_CONOBJECT(fxShapeReplicatedStatic);
};


//------------------------------------------------------------------------------
// Class: fxShapeReplicator
//------------------------------------------------------------------------------
class fxShapeReplicator : public SceneObject
{
private:
   typedef SceneObject      Parent;

protected:

   void CreateShapes(void);
   void DestroyShapes(void);
   void RenewShapes(void);

   enum {   ReplicationMask   = (1 << 0) };

   U32                              mCreationAreaAngle;
   bool                             mClientReplicationStarted;
   bool                             mAddedToScene;
   U32                              mCurrentShapeCount;
   Vector<fxShapeReplicatedStatic*> mReplicatedShapes;
   MRandomLCG                       RandomGen;


public:
   fxShapeReplicator();
   ~fxShapeReplicator();


   void StartUp(void);
   void ShowReplication(void);
   void HideReplication(void);

   // SceneObject
   void renderObject(SceneState *state, SceneRenderImage *image);
   virtual bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone,
                        const bool modifyBaseZoneState = false);

   // SimObject
   bool onAdd();
   void onRemove();
   void onEditorEnable();
   void onEditorDisable();
   void inspectPostApply();

   // NetObject
   U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   // ConObject.
   static void initPersistFields();

   // Field Data.
   class tagFieldData
   {
      public:

      U32                mSeed;
      StringTableEntry   mShapeFile;
      U32                mShapeCount;
      U32                mShapeRetries;
      Point3F            mShapeScaleMin;
      Point3F            mShapeScaleMax;
      Point3F            mShapeRotateMin;
      Point3F            mShapeRotateMax;
      U32                mInnerRadiusX;
      U32                mInnerRadiusY;
      U32                mOuterRadiusX;
      U32                mOuterRadiusY;
      S32                mOffsetZ;
      bool              mAllowOnTerrain;
      bool              mAllowOnInteriors;
      bool              mAllowStatics;
      bool              mAllowOnWater;
      S32               mAllowedTerrainSlope;
      bool              mAlignToTerrain;
      bool              mAllowWaterSurface;
      Point3F           mTerrainAlignment;
      bool              mInteractions;
      bool              mHideReplications;
      bool              mShowPlacementArea;
      U32               mPlacementBandHeight;
      ColorF            mPlaceAreaColour;

      tagFieldData()
      {
         // Set Defaults.
         mSeed               = 1376312589;
         mShapeFile          = StringTable->insert("");
         mShapeCount         = 10;
         mShapeRetries       = 100;
         mInnerRadiusX       = 0;
         mInnerRadiusY       = 0;
         mOuterRadiusX       = 100;
         mOuterRadiusY       = 100;
         mOffsetZ            = 0;

         mAllowOnTerrain     = true;
         mAllowOnInteriors   = true;
         mAllowStatics       = true;
         mAllowOnWater       = false;
         mAllowWaterSurface  = false;
         mAllowedTerrainSlope= 90;
         mAlignToTerrain     = false;
         mInteractions       = true;

         mHideReplications   = false;

         mShowPlacementArea    = true;
         mPlacementBandHeight  = 25;
         mPlaceAreaColour      .set(0.4f, 0, 0.8f);

         mShapeScaleMin         .set(1, 1, 1);
         mShapeScaleMax         .set(1, 1, 1);
         mShapeRotateMin        .set(0, 0, 0);
         mShapeRotateMax        .set(0, 0, 0);
         mTerrainAlignment      .set(1, 1, 1);
      }

   } mFieldData;

   // Declare Console Object.
   DECLARE_CONOBJECT(fxShapeReplicator);
};

#endif // _SHAPEREPLICATOR_H_
