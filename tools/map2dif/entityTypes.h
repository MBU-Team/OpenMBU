//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ENTITYTYPES_H_
#define _ENTITYTYPES_H_

//Includes
#ifndef _MORIANBASICS_H_
#include "map2dif/morianBasics.h"
#endif
#ifndef _EDITGEOMETRY_H_
#include "map2dif/editGeometry.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _POLYHEDRON_H_
#include "collision/polyhedron.h"
#endif
#ifndef _CSGBRUSH_H_
#include "csgBrush.h"
#endif

//------------------------------------------------------------------------------
//-------------------------------------- Brush based entities
class WorldSpawnEntity : public EditGeometry::Entity
{
  public:
   U32   mDetailNumber;
   U32   mMinPixels;

   F32    mGeometryScale;
   //F32    mInsideLumelScale;
   //F32    mOutsideLumelScale;

  private:
   F32    mLumelScale;
  public:
   U32 sgGetLightingScale() {return mLumelScale;}   

   ColorF mAmbientColor;
   ColorF mEmergencyAmbientColor;

   bool mBrushLightIsPoint;

   char mWadPrefix[256];

  public:
   WorldSpawnEntity();
   ~WorldSpawnEntity();

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const;
   const char*    getName()   const;
   const Point3D& getOrigin() const;

   static const char* getClassName();
};

class sgLightingScaleEntity : public EditGeometry::Entity
{
public:
	F32 sgLightingScale;
	U32 sgGetLightingScale() {return sgLightingScale;}

	ColorI sgSelfIllumination;
	ColorI sgGetSelfIllumination() {return sgSelfIllumination;}

public:
	sgLightingScaleEntity()
	{
		sgLightingScale = 32.0;
		sgSelfIllumination = ColorI(0, 0, 0);
	}
	~sgLightingScaleEntity() {}
	
	bool parseEntityDescription(Tokenizer* pToker);
	BrushType getBrushType() {return StructuralBrush;}
	bool isPointClass() const {return false;}
	const char* getName() const
	{
		static char bogoChar = '\0';
		return &bogoChar;
	}
	const Point3D& getOrigin() const
	{
		static Point3D bogoPoint(0, 0, 0);
		return bogoPoint;
	}
	static const char* getClassName() {return "sgLightingScaleEntity";}
};

class DetailEntity : public EditGeometry::Entity
{
  public:
   DetailEntity();
   ~DetailEntity();

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const;
   const char*    getName()   const;
   const Point3D& getOrigin() const;

   static const char* getClassName();
};

class CollisionEntity : public EditGeometry::Entity
{
  public:
   CollisionEntity();
   ~CollisionEntity();

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const;
   const char*    getName()   const;
   const Point3D& getOrigin() const;

   static const char* getClassName();
};

class VehicleCollisionEntity : public EditGeometry::Entity
{
  public:
   VehicleCollisionEntity();
   ~VehicleCollisionEntity();

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const;
   const char*    getName()   const;
   const Point3D& getOrigin() const;

   static const char* getClassName();
};

class PortalEntity : public EditGeometry::Entity
{
  public:
   PortalEntity();
   ~PortalEntity();

   bool passAmbientLight;

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const;
   const char*    getName()   const;
   const Point3D& getOrigin() const;

   static const char* getClassName();
};

//------------------------------------------------------------------------------
//-------------------------------------- Lighting and Target types

class TargetEntity : public EditGeometry::Entity
{
  public:
   char    mTargetName[256];
   Point3D mOrigin;

  public:
   TargetEntity();
   ~TargetEntity();

   bool      parseEntityDescription(Tokenizer* pToker);
   BrushType getBrushType();

   bool           isPointClass() const { return(true); }
   const char*    getName()   const { return(mTargetName); }
   const Point3D& getOrigin() const { return(mOrigin); }

   static const char* getClassName() { return("target"); }
};

//------------------------------------------------------------------------------

class BaseLightEmitterEntity : public EditGeometry::Entity
{
   public:
      enum FalloffType {
         Distance = 0,
         Linear   = 1
      };

   char     mTargetLight[256];
   U32      mStateIndex;
   Point3D  mOrigin;

   BaseLightEmitterEntity();

   bool           isPointClass() const { return(true); }
   const char*    getName()   const { return(""); }
   const Point3D& getOrigin() const { return(mOrigin); }
   BrushType getBrushType();
};

class PointEmitterEntity : public BaseLightEmitterEntity
{
   public:

      BaseLightEmitterEntity::FalloffType mFalloffType;
      U32                                 mFalloff1;
      U32                                 mFalloff2;
      U32                                 mFalloff3;

      PointEmitterEntity();
      bool parseEntityDescription(Tokenizer* pToker);
      static const char* getClassName() { return("light_emitter_point"); }
};

class SpotEmitterEntity : public BaseLightEmitterEntity
{
   public:

      BaseLightEmitterEntity::FalloffType mFalloffType;
      U32                                 mFalloff1;
      U32                                 mFalloff2;
      U32                                 mFalloff3;
      Point3D                             mDirection;
      F32                                 mInnerAngle;
      F32                                 mOuterAngle;

      SpotEmitterEntity();

      bool parseEntityDescription(Tokenizer * pToker);
      static const char * getClassName() { return("light_emitter_spot"); }
};

//------------------------------------------------------------------------------

class BaseLightEntity : public EditGeometry::Entity
{
  public:
      enum Speed {
         VerySlow    = 0,
         Slow        = 1,
         Normal      = 3,
         Fast        = 4,
         VeryFast    = 5
      };

//      enum Flags {
//         AutoStart      = 1 << 0,
//         LoopToEndFrame = 1 << 1,
//         RandomFrame    = 1 << 2,
//
//         FlagMask = AutoStart | LoopToEndFrame | RandomFrame
//      };
      enum AlarmStatus {
         NormalOnly = 0,
         AlarmOnly  = 1,
         Both       = 2
      };

      U32            mFlags;
      AlarmStatus    mAlarmStatus;

   public:
      Point3D        mOrigin;
      char           mLightName[256];
      bool           mBumpSpec;

      BaseLightEntity();

      bool           isPointClass() const { return(true); }
      const char*    getName()   const { return(mLightName); }
      const Point3D& getOrigin() const { return(mOrigin); }
      BrushType getBrushType();
};

class LightEntity : public BaseLightEntity
{
   public:
      struct StateInfo {
         enum {
            DurationValid  = 1 << 0,
            ColorValid     = 1 << 1,

            Valid          = DurationValid | ColorValid,
            MaxStates      = 32,
         };

         F32      mDuration;
         ColorF   mColor;
         U8       mFlags;
      };

      U32         mNumStates;
      StateInfo   mStates[StateInfo::MaxStates];

   public:
      LightEntity();

      bool parseEntityDescription(Tokenizer* pToker);
      static const char* getClassName() { return("light"); }
};

//------------------------------------------------------------------------------

class LightOmniEntity : public BaseLightEntity
{
   public:
      ColorF   mColor;
      U32      mFalloff1;
      U32      mFalloff2;

      LightOmniEntity();

      bool parseEntityDescription(Tokenizer* pToker);
      static const char* getClassName() { return("light_omni"); }
};

//------------------------------------------------------------------------------

class LightBrushEntity : public BaseLightEntity
{
   public:
      ColorF   mColor;
      U32      mRadius;
      CSGBrush mBrush;
      F32      mIntensity;

      LightBrushEntity();

      bool parseEntityDescription(Tokenizer* pToker);
      static const char* getClassName() { return("light_brush"); }
      BrushType getBrushType(){ return LightBrush; }
};

//------------------------------------------------------------------------------

class LightSpotEntity : public BaseLightEntity
{
   public:
      char     mTarget[256];
      ColorF   mColor;
      U32      mInnerDistance;
      U32      mOuterDistance;
      U32      mFalloff1;
      U32      mFalloff2;

      LightSpotEntity();

      bool parseEntityDescription(Tokenizer* pToker);
      static const char* getClassName() { return("light_spot"); }
};

//------------------------------------------------------------------------------

class LightStrobeEntity : public BaseLightEntity
{
   public:

      U32      mSpeed;

      U32      mFalloff1;
      U32      mFalloff2;

      ColorF   mColor1;
      ColorF   mColor2;

      LightStrobeEntity();
      bool parseEntityDescription(Tokenizer* pToker);

      static const char* getClassName() {return("light_strobe");}
};

//------------------------------------------------------------------------------

class LightPulseEntity : public BaseLightEntity
{
   public:

      U32      mSpeed;

      U32      mFalloff1;
      U32      mFalloff2;

      ColorF   mColor1;
      ColorF   mColor2;

      LightPulseEntity();
      bool parseEntityDescription(Tokenizer* pToker);

      static const char* getClassName() {return("light_pulse");}
};

//------------------------------------------------------------------------------

class LightPulse2Entity : public BaseLightEntity
{
   public:

      ColorF   mColor1;
      ColorF   mColor2;

      U32      mFalloff1;
      U32      mFalloff2;

      F32      mAttack;
      F32      mSustain1;
      F32      mDecay;
      F32      mSustain2;

      LightPulse2Entity();
      bool parseEntityDescription(Tokenizer* pToker);

      static const char* getClassName() {return("light_pulse2");}
};

//------------------------------------------------------------------------------

class LightFlickerEntity : public BaseLightEntity
{
   public:
      U32      mSpeed;

      ColorF   mColor1;
      ColorF   mColor2;
      ColorF   mColor3;
      ColorF   mColor4;
      ColorF   mColor5;

      U32      mFalloff1;
      U32      mFalloff2;

      LightFlickerEntity();
      bool parseEntityDescription(Tokenizer* pToker);

      static const char* getClassName() {return("light_flicker");}
};

//------------------------------------------------------------------------------

class LightRunwayEntity : public BaseLightEntity
{
   public:

      char     mEndTarget[256];
      ColorF   mColor;
      U32      mSpeed;

      bool     mPingPong;
      U32      mSteps;

      U32      mFalloff1;
      U32      mFalloff2;

      LightRunwayEntity();
      bool parseEntityDescription(Tokenizer* pToker);

      static const char* getClassName() {return("light_runway");}
};


//--------------------------------------------------------------------------
//-------------------------------------- SPECIAL TYPES
//
class MirrorSurfaceEntity : public EditGeometry::Entity
{
   static U32 smZoneKeyAllocator;

  public:
   Point3D  mOrigin;
   float    mAlphaLevel;

   U32      mZoneKey;
   U32      mRealZone;

   MirrorSurfaceEntity();
   static const char* getClassName() { return("MirrorSurface"); }

   bool           parseEntityDescription(Tokenizer* pToker);
   bool           isPointClass() const { return true;    }
   const char*    getName()   const    { return NULL;    }
   const Point3D& getOrigin() const    { return mOrigin; }
   BrushType      getBrushType();

   void markSurface(Vector<CSGBrush*>&, Vector<CSGBrush*>&);
   void grabSurface(EditGeometry::Surface&);
};

//--------------------------------------------------------------------------
//-------------------------------------- PATH TYPES
//
class PathNodeEntity : public EditGeometry::Entity
{
  public:
   Point3D mOrigin;

   S32     mNumMSToNextNode;
   U32     mSmoothingType;

  public:
   PathNodeEntity();
   ~PathNodeEntity();
   BrushType getBrushType();
   bool      parseEntityDescription(Tokenizer* pToker);

   static const char* getClassName() { return "path_node"; }
   const char*    getName()   const { return("a_path_node"); }
   const Point3D&     getOrigin() const    { return mOrigin; }
   bool               isPointClass() const { return true;    }
};

//--------------------------------------------------------------------------
//-------------------------------------- Trigger types
//
class TriggerEntity : public EditGeometry::Entity
{
  public:
   char    mName[256];
   char    mDataBlock[256];
   Point3D mOrigin;

   bool       mValid;

   Polyhedron triggerPolyhedron;

   Vector<CSGBrush*> mBrushes;
   U32               mCurrBrushId;

  protected:
   void generateTrigger();

  public:
   TriggerEntity();
   ~TriggerEntity();
   BrushType getBrushType();
   bool      parseEntityDescription(Tokenizer* pToker);


   static const char* getClassName()       { return "trigger"; }
   const char*        getName()   const    { return mName;    }
   const Point3D&     getOrigin() const    { return mOrigin; }
   bool               isPointClass() const { return true;    }
};

//--------------------------------------------------------------------------
//-------------------------------------- REALLY REALLY SPECIAL TYPES
//
static Point3D gOrigin(0, 0, 0);

class DoorEntity : public EditGeometry::Entity
{
  public:
   Point3D mOrigin;
   char  mName[256];
   char  mPath[256];
   char  mDataBlock[256];
   Vector<U32> mTriggerIds;
   Vector<InteriorPathFollower::WayPoint> mWayPoints;
   U32 mTotalMS;

   Vector<CSGBrush*> mBrushes;
   U32               mCurrBrushId;

   Interior*         mInterior;

  public:
   DoorEntity();
   ~DoorEntity();
   void process();

   void addPathNode(PathNodeEntity *ent);
   static const char* getClassName() { return("Door_Elevator"); }
   bool               parseEntityDescription(Tokenizer* pToker);
   const char*        getName()   const    { return mName; }
   const Point3D&     getOrigin() const    { return gOrigin; }
   BrushType          getBrushType();

   // ok, not really, but as far as the parser is concered, yes.
   bool isPointClass() const { return true;    }
};

class ForceFieldEntity : public EditGeometry::Entity
{
  public:
   char  mName[256];
   U32   mMSToFade;
   char  mTrigger[8][256];

   Vector<CSGBrush*> mBrushes;
   U32               mCurrBrushId;
   ColorF            mColor;

   Interior*         mInterior;

  public:
   ForceFieldEntity();
   ~ForceFieldEntity();
   void process();

   static const char* getClassName() { return("Force_Field"); }
   bool               parseEntityDescription(Tokenizer* pToker);
   const char*        getName()   const    { return mName; }
   const Point3D&     getOrigin() const    { return gOrigin; }
   BrushType          getBrushType();

   // ok, not really, but as far as the parser is concered, yes.
   bool isPointClass() const { return true;    }
};

//--------------------------------------------------------------------------
//-----AI special Node type
//
class SpecialNodeEntity : public EditGeometry::Entity
{
  public:
   Point3D mOrigin;

   char    mName[256];

  public:
   SpecialNodeEntity();
   ~SpecialNodeEntity();
   BrushType getBrushType();
   bool      parseEntityDescription(Tokenizer* pToker);

   static const char* getClassName() { return "ai_special_node"; }
   const char*        getName()   const    { return mName;    }
   const Point3D&     getOrigin() const    { return mOrigin; }
   bool               isPointClass() const { return true;    }
};

class GameEntity : public EditGeometry::Entity
{
  public:
   Point3D mOrigin;

   char    mDatablock[256];
   char    mGameClass[256];

  public:
   GameEntity(const char *gameClassName);
   ~GameEntity();
   BrushType getBrushType();
   bool      parseEntityDescription(Tokenizer* pToker);

   static const char* getClassName() { return "game_entity_unused"; }
   const char*        getName()   const    { return NULL;    }
   const Point3D&     getOrigin() const    { return mOrigin; }
   bool               isPointClass() const { return true;    }
};

#endif //_ENTITYTYPES_H_
