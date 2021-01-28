//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif/entityTypes.h"
#include "map2dif/tokenizer.h"
#include "map2dif/csgBrush.h"
#include "interior/interior.h"
#include "map2dif/morianUtil.h"
#include "gfx/gBitmap.h"

DoorEntity *gCurrentDoor = NULL;
U32 gTriggerId = 0;

void parseBrushList(Vector<CSGBrush*>& brushes, Tokenizer* pToker, EditGeometry& geom)
{
   // Ok, this is where it gets interesting.  We have to parse in the brushes, and place
   //  them in our list before we return to the editGeometry.  I'm copying almost all this
   //  code from the parseMapFile function in editGeometry.cc
   //
   while (pToker->getToken()[0] == '{') {
      pToker->advanceToken(true);

      brushes.push_back(gBrushArena.allocateBrush());
      CSGBrush& rBrush = *brushes.last();

      if (!parseBrush (rBrush, pToker, geom)) return;

      rBrush.disambiguate();
      pToker->advanceToken(true);
   }
}


//------------------------------------------------------------------------------
WorldSpawnEntity::WorldSpawnEntity()
{
   mDetailNumber     = 0;
   mMinPixels        = 250;

   mGeometryScale       = 32.0f;
   
   //mInsideLumelScale    = 32.0f;
   //mOutsideLumelScale   = 32.0f;
   mLumelScale = 32.0f;

   mAmbientColor.set(0,0,0);
   mEmergencyAmbientColor.set(0,0,0);

   mBrushLightIsPoint = false;

   mWadPrefix[0] = '\0';
}

WorldSpawnEntity::~WorldSpawnEntity()
{

}

bool WorldSpawnEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("detail_number")) {
         pToker->advanceToken(false, true);
         mDetailNumber = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("min_pixels")) {
         pToker->advanceToken(false, true);
         mMinPixels = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("geometry_scale")) {
         pToker->advanceToken(false, true);
         mGeometryScale = dAtof(pToker->getToken());
         AssertISV(isPow2(U32(mGeometryScale)), avar("Error in: %s.  Geometry scale must be a power of 2", pToker->getFileName()));
      }
      else if (pToker->tokenICmp("outside_light_scale")) {
         pToker->advanceToken(false, true);
         mLumelScale = dAtof(pToker->getToken());
         AssertISV(isPow2(U32(mGeometryScale)), avar("Error in: %s.  Geometry scale must be a power of 2", pToker->getFileName()));
      }
      else if (pToker->tokenICmp("inside_light_scale")) {
         pToker->advanceToken(false, true);
         mLumelScale = dAtof(pToker->getToken());
         AssertISV(isPow2(U32(mGeometryScale)), avar("Error in: %s.  Geometry scale must be a power of 2", pToker->getFileName()));
      }
      else if (pToker->tokenICmp("ambient_color")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%f %f %f", &mAmbientColor.red, &mAmbientColor.green, &mAmbientColor.blue);
         mAmbientColor.red   /= 255.0f;
         mAmbientColor.green /= 255.0f;
         mAmbientColor.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("brush_light_is_point")) {
         pToker->advanceToken(false, true);
         mBrushLightIsPoint = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("emergency_ambient_color")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%f %f %f", &mEmergencyAmbientColor.red, &mEmergencyAmbientColor.green, &mEmergencyAmbientColor.blue);
         mEmergencyAmbientColor.red   /= 255.0f;
         mEmergencyAmbientColor.green /= 255.0f;
         mEmergencyAmbientColor.blue  /= 255.0f;
      } else if (pToker->tokenICmp("wad")) {
         // Only need to parse the wad for T2 compatability
         char buffer[1024];
         pToker->advanceToken(false, true);
         dStrcpy(buffer, pToker->getToken());

         if (char* semi = dStrchr(buffer, ';'))  {
            dPrintf("\n  *** More than one WAD file assigned to this map\n"
                    "  *** The map may not load correctly\n");
            *semi = 0;
         }

         char* pSlash = dStrrchr(buffer, '\\');
         AssertFatal(pSlash != NULL, "Error, there should be a slash in there somewhere!");

         U32 pos = 0;
         while (pSlash[pos + 1] != '.') {
            mWadPrefix[pos] = pSlash[pos + 1];
            pos++;
         }
         mWadPrefix[pos++] = '/';
         mWadPrefix[pos] = '\0';
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

BrushType WorldSpawnEntity::getBrushType()
{
   return StructuralBrush;
}

const char* WorldSpawnEntity::getClassName()
{
   return "worldspawn";
}

bool WorldSpawnEntity::isPointClass() const
{
   return false;
}

const char* WorldSpawnEntity::getName() const
{
   static char bogoChar = '\0';
   return &bogoChar;
}
const Point3D& WorldSpawnEntity::getOrigin() const
{
   static Point3D bogoPoint(0, 0, 0);
   return bogoPoint;
}


//------------------------------------------------------------------------------


bool sgLightingScaleEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("light_geometry_scale")) {
         pToker->advanceToken(false, true);
         sgLightingScale = dAtof(pToker->getToken());
         AssertISV(isPow2(U32(sgLightingScale)), avar("Error in: %s.  Geometry scale must be a power of 2", pToker->getFileName()));
      }
      else if (pToker->tokenICmp("self_illumination_color")) {
         pToker->advanceToken(false, true);

		 ColorF temp;
         dSscanf(pToker->getToken(), "%f %f %f", &temp.red,
			 &temp.green, &temp.blue);
		 sgSelfIllumination.red = temp.red;
		 sgSelfIllumination.green = temp.green;
		 sgSelfIllumination.blue = temp.blue;
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}


//------------------------------------------------------------------------------
DetailEntity::DetailEntity()
{

}

DetailEntity::~DetailEntity()
{

}

bool DetailEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      pToker->advanceToken(false);  // For keyless entity fields...
      pToker->advanceToken(true);
   }

   return true;
}

BrushType DetailEntity::getBrushType()
{
   return DetailBrush;
}

const char* DetailEntity::getClassName()
{
   return "detail";
}

bool DetailEntity::isPointClass() const
{
   return false;
}

const char* DetailEntity::getName() const
{
   static char bogoChar = '\0';
   return &bogoChar;
}
const Point3D& DetailEntity::getOrigin() const
{
   static Point3D bogoPoint(0, 0, 0);
   return bogoPoint;
}


//------------------------------------------------------------------------------
CollisionEntity::CollisionEntity()
{

}

CollisionEntity::~CollisionEntity()
{

}

bool CollisionEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      pToker->advanceToken(false);  // For keyless entity fields...
      pToker->advanceToken(true);
   }

   return true;
}

BrushType CollisionEntity::getBrushType()
{
   return CollisionBrush;
}

const char* CollisionEntity::getClassName()
{
   return "collision";
}

bool CollisionEntity::isPointClass() const
{
   return false;
}

const char* CollisionEntity::getName() const
{
   static char bogoChar = '\0';
   return &bogoChar;
}
const Point3D& CollisionEntity::getOrigin() const
{
   static Point3D bogoPoint(0, 0, 0);
   return bogoPoint;
}


//------------------------------------------------------------------------------
VehicleCollisionEntity::VehicleCollisionEntity()
{

}

VehicleCollisionEntity::~VehicleCollisionEntity()
{

}

bool VehicleCollisionEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      pToker->advanceToken(false);  // For keyless entity fields...
      pToker->advanceToken(true);
   }

   return true;
}

BrushType VehicleCollisionEntity::getBrushType()
{
   return VehicleCollisionBrush;
}

const char* VehicleCollisionEntity::getClassName()
{
   return "vehicle_collision";
}

bool VehicleCollisionEntity::isPointClass() const
{
   return false;
}

const char* VehicleCollisionEntity::getName() const
{
   static char bogoChar = '\0';
   return &bogoChar;
}
const Point3D& VehicleCollisionEntity::getOrigin() const
{
   static Point3D bogoPoint(0, 0, 0);
   return bogoPoint;
}


//------------------------------------------------------------------------------
PortalEntity::PortalEntity()
{
   passAmbientLight = false;
}

PortalEntity::~PortalEntity()
{

}

bool PortalEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("ambient_light")) {
         pToker->advanceToken(false, true);
         passAmbientLight = dAtoi(pToker->getToken()) == 0 ? false : true;
      } else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

BrushType PortalEntity::getBrushType()
{
   return PortalBrush;
}

const char* PortalEntity::getClassName()
{
   return "portal";
}

bool PortalEntity::isPointClass() const
{
   return false;
}

const char* PortalEntity::getName() const
{
   static char bogoChar = '\0';
   return &bogoChar;
}
const Point3D& PortalEntity::getOrigin() const
{
   static Point3D bogoPoint(0, 0, 0);
   return bogoPoint;
}


//------------------------------------------------------------------------------

TargetEntity::TargetEntity()
{
   mTargetName[0] = '\0';
   mOrigin.set(0,0,0);
}

TargetEntity::~TargetEntity()
{
   //
}

bool TargetEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mTargetName, pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

BrushType TargetEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with targets!");
   return DetailBrush;
}

//------------------------------------------------------------------------------
//-------------------------------------- LIGHT TYPES...
//
BaseLightEmitterEntity::BaseLightEmitterEntity()
{
   mTargetLight[0] = '\0';
   mStateIndex  = 0;
   mOrigin.set(0,0,0);
}

BrushType BaseLightEmitterEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with lights!");
   return DetailBrush;
}

//------------------------------------------------------------------------------
// Light Emitters
//------------------------------------------------------------------------------

PointEmitterEntity::PointEmitterEntity()
{
   mFalloffType = BaseLightEmitterEntity::Linear;
   mFalloff1 = 10;
   mFalloff2 = 100;
   mFalloff3 = 0;
}

bool PointEmitterEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("target")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mTargetLight, pToker->getToken());
      }
      else if (pToker->tokenICmp("state_index")) {
         pToker->advanceToken(false, true);
         mStateIndex = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff_type")) {
         pToker->advanceToken(false, true);
         mFalloffType = (dAtoi(pToker->getToken()) == 0) ? BaseLightEmitterEntity::Distance :
                                                           BaseLightEmitterEntity::Linear;
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff3")) {
         pToker->advanceToken(false, true);
         mFalloff3 = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

SpotEmitterEntity::SpotEmitterEntity()
{
   mFalloffType = BaseLightEmitterEntity::Linear;
   mDirection.set(0,0,-1);
   mFalloff1   = 10;
   mFalloff2   = 100;
   mFalloff3   = 0;
   mInnerAngle = 0.2;
   mOuterAngle = 0.4;
}

bool SpotEmitterEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("target")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mTargetLight, pToker->getToken());
      }
      else if (pToker->tokenICmp("state_index")) {
         pToker->advanceToken(false, true);
         mStateIndex = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff_type")) {
         pToker->advanceToken(false, true);
         mFalloffType = (dAtoi(pToker->getToken()) == 0) ? BaseLightEmitterEntity::Distance :
                                                           BaseLightEmitterEntity::Linear;
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff3")) {
         pToker->advanceToken(false, true);
         mFalloff3 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("direction")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%lf %lf %lf", &mDirection.x, &mDirection.y, &mDirection.z);
      }
      else if (pToker->tokenICmp("theta")) {
         pToker->advanceToken(false, true);
         mInnerAngle = dAtof(pToker->getToken());
      }
      else if (pToker->tokenICmp("phi")) {
         pToker->advanceToken(false, true);
         mOuterAngle = dAtof(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

BaseLightEntity::BaseLightEntity()
{
   mFlags         = Interior::AnimationAmbient | Interior::AnimationLoop;
   mAlarmStatus   = NormalOnly;
   mOrigin.set(0,0,0);
   mLightName[0]  = '\0';
   mBumpSpec = true;
}

BrushType BaseLightEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with lights!");
   return DetailBrush;
}

//------------------------------------------------------------------------------
// Light
//------------------------------------------------------------------------------

LightEntity::LightEntity()
{
   mNumStates     = 0;
   dMemset(mStates, 0, sizeof(mStates));
}

bool LightEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);
         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if(dStrncmp(pToker->getToken(), "state", 5))
      {
         char buffer[Tokenizer::MaxTokenSize];
         dStrcpy(buffer, pToker->getToken() + 5);

         pToker->advanceToken(false, true);

         // parse the number
         char * end = dStrchr(buffer, '_');
         if(!end)
         {
            pToker->advanceToken(true);
            continue;
         }
         *end++ = '\0';
         S32 stateNum = dAtoi(buffer);
         if(stateNum < 0 || stateNum > StateInfo::MaxStates)
         {
            pToker->advanceToken(true);
            continue;
         }

         // read in the value
         if(!dStricmp(end, "duration"))
         {
            mStates[stateNum].mDuration = dAtof(pToker->getToken());
            mStates[stateNum].mFlags |= StateInfo::DurationValid;
         }
         else if(!dStricmp(end, "color"))
         {
            dSscanf(pToker->getToken(), "%f %f %f",
               &mStates[stateNum].mColor.red,
               &mStates[stateNum].mColor.green,
               &mStates[stateNum].mColor.blue);

            mStates[stateNum].mColor.red /= 255.0f;
            mStates[stateNum].mColor.green /= 255.0f;
            mStates[stateNum].mColor.blue /= 255.0f;

            mStates[stateNum].mFlags |= StateInfo::ColorValid;
         }
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   // check the state info
   if(!mNumStates)
      return(false);

   for(U32 i = 0; i < mNumStates; i++)
      if(!(mStates[i].mFlags & StateInfo::Valid))
         return(false);

   return true;
}

//------------------------------------------------------------------------------
// static stock lights
//------------------------------------------------------------------------------

LightOmniEntity::LightOmniEntity()
{
   mColor.set(1,1,1);
   mFalloff1 = 10;
   mFalloff2 = 100;
}

bool LightOmniEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor.red, &mColor.green, &mColor.blue);
         mColor.red   /= 255.0f;
         mColor.green /= 255.0f;
         mColor.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("bumpSpec")) {
         pToker->advanceToken(false, true);
         mBumpSpec = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------
// LightBrushEntity
//------------------------------------------------------------------------------
LightBrushEntity::LightBrushEntity()
{
   mColor.set(1,1,1);
   mRadius = 1000;
}

bool LightBrushEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("falloff")) {
         pToker->advanceToken(false, true);
         mRadius = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("intensity")) {
         pToker->advanceToken(false, true);
         mIntensity = dAtof(pToker->getToken());
      }
      else if (pToker->tokenICmp("color")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor.red, &mColor.green, &mColor.blue);
         mColor.red   /= 255.0f;
         mColor.green /= 255.0f;
         mColor.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("bumpSpec")) {
         pToker->advanceToken(false, true);
         mBumpSpec = dAtoi(pToker->getToken());
      }
      else{
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}


//------------------------------------------------------------------------------

LightSpotEntity::LightSpotEntity()
{
   mTarget[0]     = '\0';
   mColor.set(1,1,1);
   mInnerDistance = 10;
   mOuterDistance = 100;
   mFalloff1      = 10;
   mFalloff2      = 100;
}

bool LightSpotEntity::parseEntityDescription(Tokenizer * pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("target")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mTarget, pToker->getToken());
      }
      else if (pToker->tokenICmp("color")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor.red, &mColor.green, &mColor.blue);
         mColor.red   /= 255.0f;
         mColor.green /= 255.0f;
         mColor.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("distance1")) {
         pToker->advanceToken(false, true);
         mInnerDistance = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("distance2")) {
         pToker->advanceToken(false, true);
         mOuterDistance = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------
// animated stock lignts
//------------------------------------------------------------------------------

LightStrobeEntity::LightStrobeEntity()
{
   mSpeed      = Normal;
   mColor1.set(0,0,0);
   mColor2.set(1,1,1);
   mFalloff1   = 10;
   mFalloff2   = 100;
}

bool LightStrobeEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color1")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor1.red, &mColor1.green, &mColor1.blue);
         mColor1.red   /= 255.0f;
         mColor1.green /= 255.0f;
         mColor1.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color2")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor2.red, &mColor2.green, &mColor2.blue);
         mColor2.red   /= 255.0f;
         mColor2.green /= 255.0f;
         mColor2.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("speed")) {
         pToker->advanceToken(false, true);
         mSpeed = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

LightPulseEntity::LightPulseEntity()
{
   mSpeed      = Normal;
   mColor1.set(0,0,0);
   mColor2.set(1,1,1);
   mFalloff1   = 10;
   mFalloff2   = 100;
}

bool LightPulseEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color1")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor1.red, &mColor1.green, &mColor1.blue);
         mColor1.red   /= 255.0f;
         mColor1.green /= 255.0f;
         mColor1.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color2")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor2.red, &mColor2.green, &mColor2.blue);
         mColor2.red   /= 255.0f;
         mColor2.green /= 255.0f;
         mColor2.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("speed")) {
         pToker->advanceToken(false, true);
         mSpeed = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

LightPulse2Entity::LightPulse2Entity()
{
   mColor1.set(0,0,0);
   mColor2.set(1,1,1);
   mFalloff1   = 10;
   mFalloff2   = 100;
   mAttack = mSustain1 = mDecay = mSustain2 = 1.f;
}

bool LightPulse2Entity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color1")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor1.red, &mColor1.green, &mColor1.blue);
         mColor1.red   /= 255.0f;
         mColor1.green /= 255.0f;
         mColor1.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color2")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor2.red, &mColor2.green, &mColor2.blue);
         mColor2.red   /= 255.0f;
         mColor2.green /= 255.0f;
         mColor2.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("attack")) {
         pToker->advanceToken(false, true);
         mAttack = dAtof(pToker->getToken());
      }
      else if (pToker->tokenICmp("sustain1")) {
         pToker->advanceToken(false, true);
         mSustain1  = dAtof(pToker->getToken());
      }
      else if (pToker->tokenICmp("decay")) {
         pToker->advanceToken(false, true);
         mDecay = dAtof(pToker->getToken());
      }
      else if (pToker->tokenICmp("sustain2")) {
         pToker->advanceToken(false, true);
         mSustain2 = dAtof(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

LightFlickerEntity::LightFlickerEntity()
{
   mSpeed      = Normal;
   mColor1.set(1,1,1);
   mColor2.set(0,0,0);
   mColor3.set(0,0,0);
   mColor4.set(0,0,0);
   mColor5.set(0,0,0);
   mFalloff1   = 10;
   mFalloff2   = 100;
}

bool LightFlickerEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color1")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor1.red, &mColor1.green, &mColor1.blue);
         mColor1.red   /= 255.0f;
         mColor1.green /= 255.0f;
         mColor1.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color2")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor2.red, &mColor2.green, &mColor2.blue);
         mColor2.red   /= 255.0f;
         mColor2.green /= 255.0f;
         mColor2.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color3")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor3.red, &mColor3.green, &mColor3.blue);
         mColor3.red   /= 255.0f;
         mColor3.green /= 255.0f;
         mColor3.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color4")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor4.red, &mColor4.green, &mColor4.blue);
         mColor4.red   /= 255.0f;
         mColor4.green /= 255.0f;
         mColor4.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("color5")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor5.red, &mColor5.green, &mColor5.blue);
         mColor5.red   /= 255.0f;
         mColor5.green /= 255.0f;
         mColor5.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("speed")) {
         pToker->advanceToken(false, true);
         mSpeed = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//------------------------------------------------------------------------------

LightRunwayEntity::LightRunwayEntity()
{
   mEndTarget[0]  = '\0';
   mColor.set(1,1,1);
   mSpeed         = Normal;
   mPingPong      = false;
   mSteps         = 0;
   mFalloff1      = 10;
   mFalloff2      = 100;
}

bool LightRunwayEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("spawnflags")) {
         pToker->advanceToken(false, true);
         mFlags = dAtoi(pToker->getToken()) & Interior::AnimationTypeMask;
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mLightName, pToker->getToken());
      }
      else if (pToker->tokenICmp("target")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mEndTarget, pToker->getToken());
      }
      else if (pToker->tokenICmp("color")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%f %f %f", &mColor.red, &mColor.green, &mColor.blue);
         mColor.red   /= 255.0f;
         mColor.green /= 255.0f;
         mColor.blue  /= 255.0f;
      }
      else if (pToker->tokenICmp("alarm_type")) {
         pToker->advanceToken(false, true);
         mAlarmStatus = (AlarmStatus)dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff1")) {
         pToker->advanceToken(false, true);
         mFalloff1 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("falloff2")) {
         pToker->advanceToken(false, true);
         mFalloff2 = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("speed")) {
         pToker->advanceToken(false, true);
         mSpeed = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("steps")) {
         pToker->advanceToken(false, true);
         mSteps = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("pingpong")) {
         pToker->advanceToken(false, true);
         mPingPong = dAtoi(pToker->getToken()) ? true : false;
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------
U32 MirrorSurfaceEntity::smZoneKeyAllocator = 0;

MirrorSurfaceEntity::MirrorSurfaceEntity()
{
   mZoneKey = smZoneKeyAllocator | 0x40000000;
   smZoneKeyAllocator++;

   mRealZone = 0xFFFFFFFF;
}

bool MirrorSurfaceEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("alpha_level")) {
         pToker->advanceToken(false, true);
         S32 alevel = dAtoi(pToker->getToken());
         if (alevel < 0)
            alevel = 0;
         else if (alevel > 6)
            alevel = 6;

         switch (alevel) {
           case 0: mAlphaLevel = 0.0;           break;
           case 1: mAlphaLevel = 0.1666666666;  break;
           case 2: mAlphaLevel = 0.3333333333;  break;
           case 3: mAlphaLevel = 0.5;           break;
           case 4: mAlphaLevel = 0.6666666666;  break;
           case 5: mAlphaLevel = 0.8333333333;  break;
           case 6: mAlphaLevel = 1.0;           break;
         }
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

BrushType MirrorSurfaceEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with mirrors!");
   return DetailBrush;
}

void MirrorSurfaceEntity::markSurface(Vector<CSGBrush*>& rStructural,
                                      Vector<CSGBrush*>& /*rDetail*/)
{
   U32 brushIndex = 0;
   U32 planeIndex = 0;
   F64 minDist    = 1e10;
   for (U32 i = 0; i < rStructural.size(); i++) {
      CSGBrush* pBrush = rStructural[i];

      for (U32 j = 0; j < pBrush->mPlanes.size(); j++) {
         CSGPlane& rPlane        = pBrush->mPlanes[j];
         const PlaneEQ& rPlaneEQ = gWorkingGeometry->getPlaneEQ(rPlane.planeEQIndex);
         F64 dist                = mFabs(rPlaneEQ.distanceToPlane(getOrigin()));

         if (dist < minDist) {
            bool goodPlane = true;

            // First, confirm that the point is behind all the other planes...
            for (U32 k = 0; k < pBrush->mPlanes.size(); k++) {
               if (k == j)
                  continue;

               CSGPlane& rPlaneTest        = pBrush->mPlanes[k];
               const PlaneEQ& rPlaneEQTest = gWorkingGeometry->getPlaneEQ(rPlaneTest.planeEQIndex);

               if (rPlaneEQTest.whichSidePerfect(getOrigin()) == PlaneFront) {
                  goodPlane = false;
                  break;
               }
            }

            if (goodPlane == true) {
               minDist = dist;
               brushIndex = i;
               planeIndex = j;
            }
         }
      }
   }

   // Plane must be very close to be considered
   if (minDist < 1.0f) {
      CSGBrush* pBrush = rStructural[brushIndex];
      CSGPlane& rPlane = pBrush->mPlanes[planeIndex];

      AssertFatal(rPlane.owningEntity == NULL, "Error, brush already owned!");
      rPlane.owningEntity = this;
   }
}

void MirrorSurfaceEntity::grabSurface(EditGeometry::Surface& rSurface)
{
   if (mRealZone == 0xFFFFFFFF) {
      mRealZone = rSurface.winding.zoneIds[0];
   } else {
      if (rSurface.winding.zoneIds[0] != mRealZone) {
         dPrintf("\n  *** ACK!  Mirrored brush surfaces may NOT cross zone boundaries.  Surface disregarded");
         return;
      }
   }

   // This must be unique to the entity...
   rSurface.winding.zoneIds[0] = mZoneKey;
}


//--------------------------------------------------------------------------
DoorEntity::DoorEntity()
{
   dStrcpy(mName, "MustChange");
   mDataBlock[0] = 0;

   mCurrBrushId = 0;
   mInterior    = NULL;

   gCurrentDoor = this;
}

DoorEntity::~DoorEntity()
{
   for (U32 i = 0; i < mBrushes.size(); i++)
      gBrushArena.freeBrush(mBrushes[i]);

   delete mInterior;
   mInterior = NULL;
}

bool DoorEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an door name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mName, pToker->getToken());
      }
      else if (pToker->tokenICmp("datablock")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is a datablock name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mDataBlock, pToker->getToken());
      }
      else if (pToker->tokenICmp("path_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an path name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mPath, pToker->getToken());
      }
      else {
         InteriorDictEntry e;
         dStrncpy(e.name, pToker->getToken(), sizeof(e.name)-1);
         pToker->advanceToken(false, true);
         dStrncpy(e.value, pToker->getToken(), sizeof(e.value)-1);
         if(dStrlen(e.value))
            mDictionary.push_back(e);
      }

      pToker->advanceToken(true);
   }

   // Ok, this is where it gets interesting.  We have to parse in the brushes, and place
   //  them in our list before we return to the editGeometry.  I'm copying almost all this
   //  code from the parseMapFile function in editGeometry.cc
   parseBrushList(mBrushes, pToker, *gWorkingGeometry);

   // We need to find the origin brush...
   CSGBrush* pOrigin = NULL;
   for (U32 i = 0; i < mBrushes.size(); i++) {
      if (dStricmp(mBrushes[i]->mPlanes[0].pTextureName, "ORIGIN") == 0) {
         // That's the one!
         pOrigin = mBrushes[i];
         mBrushes.erase(i);
         break;
      }
   }

   if (pOrigin) {
      pOrigin->selfClip();
      // Grab the centroid of this brush...
      Point3D centroid(0, 0, 0);
      U32 numPoints = 0;
      for (U32 i = 0; i < pOrigin->mPlanes.size(); i++) {
         for (U32 j = 0; j < pOrigin->mPlanes[i].winding.numIndices; j++) {
            U32 index = pOrigin->mPlanes[i].winding.indices[j];
            centroid += gWorkingGeometry->getPoint(index);
            numPoints++;
         }
      }
      centroid /= F64(numPoints);
      mOrigin = centroid / gWorkingGeometry->mWorldEntity->mGeometryScale;
   } else {
      mOrigin.set(0, 0, 0);
   }


   return true;
}

BrushType DoorEntity::getBrushType()
{
   AssertISV(false, "Brushes should have been parsed out in the entity!");
   return DetailBrush;
}

void DoorEntity::addPathNode(PathNodeEntity *ent)
{
   InteriorPathFollower::WayPoint wp;
   *((Point3F *) &wp.pos) = Point3F(ent->getOrigin().x, ent->getOrigin().y, ent->getOrigin().z);
   wp.rot.set(1,0,0,0);
   wp.msToNext = ent->mNumMSToNextNode;
   wp.smoothingType = ent->mSmoothingType;
   mWayPoints.push_back(wp);
   if(mWayPoints.size() > 1)
      mTotalMS += mWayPoints[mWayPoints.size() - 2].msToNext;
}

void DoorEntity::process()
{
   U32 i, j;

   EditGeometry* pOldGeometry = gWorkingGeometry;

   gWorkingGeometry = new EditGeometry;
   gWorkingGeometry->mWorldEntity = pOldGeometry->mWorldEntity;
   gWorkingGeometry->insertTexture("NULL");
   gWorkingGeometry->insertTexture("ORIGIN");
   gWorkingGeometry->insertTexture("TRIGGER");
   gWorkingGeometry->insertTexture("FORCEFIELD");

   // Now, we have some trickiness here.  We already have our brushes parsed in,
   //  but all their plane indices are in the old geometry.  Arg.  We'll move them
   //  over to the new geometry.  Really, this should be fixed.  DMM
   for (i = 0; i < mBrushes.size(); i++) {
      CSGBrush* pBrush = mBrushes[i];

      for (j = 0; j < pBrush->mPlanes.size(); j++) {
         CSGPlane& rPlane          = pBrush->mPlanes[j];
         const PlaneEQ& oldPlaneEQ = pOldGeometry->getPlaneEQ(rPlane.planeEQIndex);

         rPlane.planeEQIndex = gWorkingGeometry->insertPlaneEQ(oldPlaneEQ.normal, oldPlaneEQ.dist);
         rPlane.pTextureName = gWorkingGeometry->insertTexture(rPlane.pTextureName);
         U32 texGenIndex = gWorkingGeometry->mTexGenEQs.size();
         gWorkingGeometry->mTexGenEQs.push_back(pOldGeometry->mTexGenEQs[rPlane.texGenIndex]);
         rPlane.texGenIndex = texGenIndex;
      }
   }

   for (i = 0; i < mBrushes.size(); i++) {
      gWorkingGeometry->mStructuralBrushes.push_back(mBrushes[i]);
      gWorkingGeometry->mStructuralBrushes.last()->brushId = mCurrBrushId++;
      mBrushes[i] = NULL;
   }
   mBrushes.clear();

   if (gWorkingGeometry->createBSP() == false) {
      // Handle the error.  Since we don't mark ourselves as valid, we won't be exported
      //
      gWorkingGeometry->mWorldEntity = NULL;
      delete gWorkingGeometry;
      gWorkingGeometry = pOldGeometry;
      return;
   }

   gWorkingGeometry->markEmptyZones();
   gWorkingGeometry->createSurfaces();

   extern bool gBuildAsLowDetail;
   bool store = gBuildAsLowDetail;
   gBuildAsLowDetail = true;
   gWorkingGeometry->computeLightmaps(false);
   gWorkingGeometry->computeLightmaps(true);
   gWorkingGeometry->preprocessLighting();
   gWorkingGeometry->sortLitSurfaces();
   gBuildAsLowDetail = store;
   gWorkingGeometry->packLMaps();

   // Yes!  Everything worked perfectly!  Export ourselves, and we're through!
   mInterior = new Interior;
   gWorkingGeometry->exportToRuntime(mInterior, NULL);

   gWorkingGeometry->mWorldEntity = NULL;
   delete gWorkingGeometry;
   gWorkingGeometry = pOldGeometry;
}


//--------------------------------------------------------------------------
//--------------------------------------
//
PathNodeEntity::PathNodeEntity()
{
   mNumMSToNextNode = 1000;
   mSmoothingType = 0;

   mOrigin.set(0, 0, 0);
}

PathNodeEntity::~PathNodeEntity()
{
   //
}

BrushType PathNodeEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with Path nodes!");
   return DetailBrush;
}

bool PathNodeEntity::parseEntityDescription(Tokenizer* pToker)
{
   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("next_time")) {
         pToker->advanceToken(false, true);
         mNumMSToNextNode = dAtoi(pToker->getToken());
         if (mNumMSToNextNode < 0)
            mNumMSToNextNode = 0;
      }
      else if (pToker->tokenICmp("smoothing")) {
         pToker->advanceToken(false, true);
         mSmoothingType = dAtoi(pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }
   gCurrentDoor->addPathNode(this);

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------
//
TriggerEntity::TriggerEntity()
{
   dStrcpy(mName, "MustChange");
   mDataBlock[0] = 0;

   mOrigin.set(0, 0, 0);

   mCurrBrushId = 0;
   mValid       = false;
}

TriggerEntity::~TriggerEntity()
{
   for (U32 i = 0; i < mBrushes.size(); i++)
      gBrushArena.freeBrush(mBrushes[i]);
}

BrushType TriggerEntity::getBrushType()
{
   AssertISV(false, "Brushes should have been parsed out in the entity!");
   return DetailBrush;
}

bool TriggerEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {

      if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mName, pToker->getToken());
      }
      else if(pToker->tokenICmp("datablock")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is a trigger datablock.  Must be less than 255", pToker->getToken()));
         dStrcpy(mDataBlock, pToker->getToken());
      }
      else
      {
         InteriorDictEntry e;
         dStrncpy(e.name, pToker->getToken(), sizeof(e.name)-1);
         pToker->advanceToken(false, true);
         dStrncpy(e.value, pToker->getToken(), sizeof(e.value)-1);
         if(dStrlen(e.value))
            mDictionary.push_back(e);
      }
      pToker->advanceToken(true);
   }

   // Ok, this is where it gets interesting.  We have to parse in the brushes, and place
   //  them in our list before we return to the editGeometry.  I'm copying almost all this
   //  code from the parseMapFile function in editGeometry.cc
   parseBrushList(mBrushes, pToker, *gWorkingGeometry);
   generateTrigger();
   if(gCurrentDoor)
      gCurrentDoor->mTriggerIds.push_back(gTriggerId);
   gTriggerId++;
   return true;
}

void TriggerEntity::generateTrigger()
{
   if (mBrushes.size() != 1) {
      dPrintf(" * Error * Trigger must have one and only one brush (name: %s has %d)", getName(), mBrushes.size());
      mValid = false;
      return;
   }

   // Ok, we assume that this brush is a cube, check by looking at the number of planes.
   //  Note that this is a really lousy check, we might want to add some better ones later.
   //  Also, most of this is really hacky, there's definitely a better way to extract the
   //  canonical box...
   //
   CSGBrush* pBrush = mBrushes[0];
   if (pBrush->mPlanes.size() != 6) {
      dPrintf(" * Error * Trigger must be a rectilinear polytope (name: %s isn't)", getName());
      mValid = false;
      return;
   }

   Point3D normals[3];
   U32     normalIndices[3];

   normals[0]       = pBrush->mPlanes[0].getNormal();
   normalIndices[0] = 0;

   U32 currentNormal = 1;
   for (U32 i = 1; i < 6 && currentNormal < 3; i++) {
      bool ortho = true;
      for (U32 j = 0; j < currentNormal; j++) {
         if (mFabs(mDot(pBrush->mPlanes[i].getNormal(), normals[j])) > 0.01)
            ortho = false;
      }

      // Found the next normal?
      if (ortho) {
         normals[currentNormal]       = pBrush->mPlanes[i].getNormal();
         normalIndices[currentNormal] = i;
         currentNormal++;
      }
   }

   if (currentNormal < 3) {
      dPrintf(" * Error * Trigger missing 3 ortho planes. (name: %s)", getName());
      mValid = false;
      return;
   }

   F64 diameters[3] = { 0, 0, 0 };
   for (U32 i = 0; i < 3; i++) {
      for (U32 j = 0; j < 6; j++) {
         if (mDot(pBrush->mPlanes[j].getNormal(), normals[i]) < -0.999) {
            // Counter part
            diameters[i] += pBrush->mPlanes[j].getDist();
         } else if (mDot(pBrush->mPlanes[j].getNormal(), normals[i]) > 0.9999) {
            // actual
            diameters[i] += pBrush->mPlanes[j].getDist();
         }
      }

      diameters[i] = mFabs(diameters[i]);
   }

   // This is a really hacky way to get the center coordinate...
   U32 antiNormalIndices[3];
   U32 currentAnti = 0;
   for (U32 i = 0; i < 6; i++) {
      bool n = false;
      for (U32 j = 0; j < 3; j++)
         if (normalIndices[j] == i)
            n = true;
      if (n == false)
         antiNormalIndices[currentAnti++] = i;
   }
   AssertFatal(currentAnti == 3, "Hm, that's bad");

   Point3D min, antiMin;

   bool intersect = pBrush->intersectPlanes(normalIndices[0], normalIndices[1], normalIndices[2], &min);
   AssertFatal(intersect, "Also, this is bad");
   intersect = pBrush->intersectPlanes(antiNormalIndices[0], antiNormalIndices[1], antiNormalIndices[2], &antiMin);
   AssertFatal(intersect, "Also, this is bad");

   mOrigin = (min + antiMin) * 0.5;
   mOrigin /= gWorkingGeometry->mWorldEntity->mGeometryScale;

   Box3F tempBox;
   tempBox.min.set(-diameters[0] / 2.0, -diameters[1] / 2.0, -diameters[2] / 2.0);
   tempBox.min /= gWorkingGeometry->mWorldEntity->mGeometryScale;
   tempBox.max.set( diameters[0] / 2.0,  diameters[1] / 2.0,  diameters[2] / 2.0);
   tempBox.max /= gWorkingGeometry->mWorldEntity->mGeometryScale;

   Point3F temp;
   Point3D realNormals[3];
   realNormals[0] = pBrush->mPlanes[normalIndices[0]].getNormal();
   realNormals[1] = pBrush->mPlanes[normalIndices[1]].getNormal();
   realNormals[2] = pBrush->mPlanes[normalIndices[2]].getNormal();
   mCross(realNormals[0], realNormals[1], &realNormals[2]);
   realNormals[2].neg();
   mCross(realNormals[0], realNormals[2], &realNormals[1]);
   realNormals[0].normalize();
   realNormals[1].normalize();
   realNormals[2].normalize();

   MatrixF tempXForm(true);
   for (U32 i = 0; i < 3; i++) {
      const Point3D& rNorm = realNormals[i];
      temp.x = rNorm.x;
      temp.y = rNorm.y;
      temp.z = rNorm.z;

      tempXForm.setColumn(i, temp);
   }
   // Export the polyhedron...
   triggerPolyhedron.buildBox(tempXForm, tempBox);

   mValid = true;
}


//--------------------------------------------------------------------------
ForceFieldEntity::ForceFieldEntity()
{
   dStrcpy(mName, "MustChange");
   mMSToFade = 1000;

   mColor = ColorF(125.0/255.0, 216.0/255.0, 255.0/255.0);

   mCurrBrushId = 0;
   mInterior    = NULL;

   for (U32 i = 0; i < 8; i++)
      mTrigger[i][0] = '\0';
}

ForceFieldEntity::~ForceFieldEntity()
{
   for (U32 i = 0; i < mBrushes.size(); i++)
      gBrushArena.freeBrush(mBrushes[i]);

   delete mInterior;
   mInterior = NULL;
}

bool ForceFieldEntity::parseEntityDescription(Tokenizer* pToker)
{
   // Tokenizer comes in pointing to the classname.  We'll check that, just to be
   //  thorough.
   AssertFatal(pToker->tokenICmp(getClassName()),
               avar("Wrong class name? (expected %s, got %s)", getClassName(), pToker->getToken()));

   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an door name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mName, pToker->getToken());
      }
      else if (pToker->tokenICmp("color")) {
         pToker->advanceToken(false, true);
         U32 r, g, b;
         dSscanf(pToker->getToken(), "%d %d %d", &r, &g, &b);
         mColor = ColorF(F32(r) / 255.0f, F32(g) / 255.0f, F32(b) / 255.0f);

         mMSToFade = dAtoi(pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger0_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[0], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger1_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[1], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger2_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[2], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger3_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[3], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger4_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[4], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger5_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[5], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger6_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[6], pToker->getToken());
      }
      else if (pToker->tokenICmp("trigger7_name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 255, avar("Error: %s is an trigger name.  Must be less than 255", pToker->getToken()));
         dStrcpy(mTrigger[7], pToker->getToken());
      }
      else {
      }

      pToker->advanceToken(true);
   }

   // Ok, this is where it gets interesting.  We have to parse in the brushes, and place
   //  them in our list before we return to the editGeometry.  I'm copying almost all this
   //  code from the parseMapFile function in editGeometry.cc
   parseBrushList(mBrushes, pToker, *gWorkingGeometry);

   return true;
}

BrushType ForceFieldEntity::getBrushType()
{
   AssertISV(false, "Brushes should have been parsed out in the entity!");
   return DetailBrush;
}

void ForceFieldEntity::process()
{
   U32 i, j;

   EditGeometry* pOldGeometry = gWorkingGeometry;

   gWorkingGeometry = new EditGeometry;
   gWorkingGeometry->mWorldEntity = pOldGeometry->mWorldEntity;
   gWorkingGeometry->insertTexture("NULL");
   gWorkingGeometry->insertTexture("ORIGIN");
   gWorkingGeometry->insertTexture("TRIGGER");
   gWorkingGeometry->insertTexture("FORCEFIELD");

   // Now, we have some trickiness here.  We already have our brushes parsed in,
   //  but all their plane indices are in the old geometry.  Arg.  We'll move them
   //  over to the new geometry.  Really, this should be fixed.  DMM
   for (i = 0; i < mBrushes.size(); i++) {
      CSGBrush* pBrush = mBrushes[i];

      for (j = 0; j < pBrush->mPlanes.size(); j++) {
         CSGPlane& rPlane          = pBrush->mPlanes[j];
         const PlaneEQ& oldPlaneEQ = pOldGeometry->getPlaneEQ(rPlane.planeEQIndex);

         rPlane.planeEQIndex = gWorkingGeometry->insertPlaneEQ(oldPlaneEQ.normal, oldPlaneEQ.dist);
         rPlane.pTextureName = gWorkingGeometry->insertTexture(rPlane.pTextureName);
      }
   }

   for (i = 0; i < mBrushes.size(); i++) {
      gWorkingGeometry->mStructuralBrushes.push_back(mBrushes[i]);
      gWorkingGeometry->mStructuralBrushes.last()->brushId = mCurrBrushId++;
      mBrushes[i] = NULL;
   }
   mBrushes.clear();

   if (gWorkingGeometry->createBSP() == false) {
      // Handle the error.  Since we don't mark ourselves as valid, we won't be exported
      //
      gWorkingGeometry->mWorldEntity = NULL;
      delete gWorkingGeometry;
      gWorkingGeometry = pOldGeometry;
      return;
   }

   gWorkingGeometry->createSurfaces();

   extern bool gBuildAsLowDetail;
   bool store = gBuildAsLowDetail;
   gBuildAsLowDetail = true;
   gWorkingGeometry->computeLightmaps(false);
   gBuildAsLowDetail = store;
   gWorkingGeometry->packLMaps();

   // Yes!  Everything worked perfectly!  Export ourselves, and we're through!
   mInterior = new Interior;
   gWorkingGeometry->exportToRuntime(mInterior, NULL);

   gWorkingGeometry->mWorldEntity = NULL;
   delete gWorkingGeometry;
   gWorkingGeometry = pOldGeometry;
}

//--------------------------------------------------------------------------
//--------------------------------------
//
SpecialNodeEntity::SpecialNodeEntity()
{
   mName[0]         = '\0';
   mOrigin.set(0, 0, 0);
}

SpecialNodeEntity::~SpecialNodeEntity()
{
   //
}

BrushType SpecialNodeEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with Special nodes!");
   return DetailBrush;
}

bool SpecialNodeEntity::parseEntityDescription(Tokenizer* pToker)
{
   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("name")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mName, pToker->getToken());
      }
      else {
         pToker->advanceToken(false);  // For keyless entity fields...
      }

      pToker->advanceToken(true);
   }

   return true;
}

GameEntity::GameEntity(const char *datablock)
{
   dStrcpy(mDatablock, datablock);
   mOrigin.set(0, 0, 0);
}

GameEntity::~GameEntity()
{
   //
}

BrushType GameEntity::getBrushType()
{
   AssertISV(false, "Brushes cannot be associated with GameEntities!");
   return DetailBrush;
}

bool GameEntity::parseEntityDescription(Tokenizer* pToker)
{
   pToker->advanceToken(true);
   while (pToker->getToken()[0] != '{' &&
          pToker->getToken()[0] != '}') {
      if (pToker->tokenICmp("origin")) {
         pToker->advanceToken(false, true);

         dSscanf(pToker->getToken(), "%lf %lf %lf", &mOrigin.x, &mOrigin.y, &mOrigin.z);
      }
      else if (pToker->tokenICmp("game_class")) {
         pToker->advanceToken(false, true);
         AssertISV(dStrlen(pToker->getToken()) < 256, avar("Error: %s is an invalid target name.  Must be less than 256", pToker->getToken()));
         dStrcpy(mGameClass, pToker->getToken());
      }
      else {
         InteriorDictEntry e;
         dStrncpy(e.name, pToker->getToken(), sizeof(e.name)-1);
         pToker->advanceToken(false, true);
         dStrncpy(e.value, pToker->getToken(), sizeof(e.value)-1);
         if(dStrlen(e.value))
            mDictionary.push_back(e);
      }

      pToker->advanceToken(true);
   }

   return true;
}
