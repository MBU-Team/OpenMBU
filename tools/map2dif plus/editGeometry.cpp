//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif plus/editGeometry.h"
#include "map2dif plus/entityTypes.h"
#include "core/tokenizer.h"
#include "map2dif plus/csgBrush.h"
#include "map2dif plus/lmapPacker.h"

#include "interior/interior.h"
#include "gfx/gBitmap.h"
#include "core/resManager.h"
#include "math/mMath.h"
#include "core/bitVector.h"
#include "core/fileStream.h"

extern bool gVerbose;
extern const char* gWadPath;
extern bool gTextureSearch;

//------------------------------------------------------------------------------
//-------------------------------------- Utility classes
//
inline U32 calcHash(const Point3D& in_rPoint)
{
   U32 x = U32(S32(in_rPoint.x) >> 5) & 0xf;
   U32 y = U32(S32(in_rPoint.y) >> 5) & 0xf;
   U32 z = U32(S32(in_rPoint.z) >> 5) & 0xf;

   return (x << 8) | (y << 4) | (z << 0);
}

inline U32 calcHash(const Point3D& in_rNormal, const F64 in_dist)
{
   F64 mul = getMax(getMax(mFabs(in_rNormal.x), mFabs(in_rNormal.y)), mFabs(in_rNormal.z));
   mul = mFloor(mul * 100.0 + 0.5) / 100.0;
   F64 val = mul * (mFloor(mFabs(in_dist) * 100.0 + 0.5) / 100.0);

   U32 hash = U32(val) % (1 << 12);
   return hash;
}

void flushedOutput(const char* string)
{
   dPrintf(string);
   dFflushStdout();
}

GBitmap* loadBitmap(const char* file)
{
   FileStream loadStream;
   int len = dStrlen(file);
   char* buf = new char[len + 5];
   char* ext = &buf[len];
   dStrcpy(buf,file);

   // PNG first
   dStrcpy(ext,".png");
   loadStream.open(buf, FileStream::Read);
   if (loadStream.getStatus() == Stream::Ok)
   {
      GBitmap * bitmap = new GBitmap;
      if(!bitmap->readPNG(loadStream))
      {
         AssertISV(false,avar("Bad PNG file: %s.\n", file));
      }
      return bitmap;
   }

   // JPEG next
   dStrcpy(ext,".jpg");
   loadStream.open(buf, FileStream::Read);
   if (loadStream.getStatus() == Stream::Ok)
   {
      GBitmap * bitmap = new GBitmap;
      if(!bitmap->readJPEG(loadStream))
      {
         AssertISV(false,avar("Bad JPEG file: %s.\n", file));
      }
      return bitmap;
   }

   // If unable to load texture in current directory look
   // in the parent directory. But never look in the root.
   if (gTextureSearch)
   {
      // If we don't flip the slashes then 
      // texture search will fail.
      char* rev;
      while( rev = dStrrchr( buf, '\\' ) )
      {
         *rev = '/';
      }

      *ext = 0;
      char *name = dStrrchr(buf, '/');
      if(name)
      {
         *name++ = 0;
         char *parent = dStrrchr(buf, '/');
         if(parent)
         {
            parent[1] = 0;
            dStrcat(buf, name);
            return loadBitmap(buf);
         }
      }
   }

   return NULL;
}

//------------------------------------------------------------------------------
//-------------------------------------- EditGeometry functionality
//
EditGeometry* gWorkingGeometry = NULL;

//------------------------------------------------------------------------------
EditGeometry::EditGeometry()
   : mPoints(1 << 15),
     mPlaneEQs(1 << 11),
     mNodeArena(512),
     mVisLinkArena(8192),
     mPlaneHashArena(1024),
     mPointHashArena(1024),
     mHasAlarmState(false)
{
   mNumAmbiguousBrushes = 0;
   mNumOrphanPolys = 0;

   for (U32 i = 0; i < (1 << 12); i++) {
      mPointHashTable[i].pNext = NULL;
      mPlaneHashTable[i].pNext = NULL;
   }

   mBSPRoot = NULL;

   mOutsideZoneIndex = -1;

   mMinBound =  Point3D(1 << 30, 1 << 30, 1 << 30);
   mMaxBound = -Point3D(1 << 30, 1 << 30, 1 << 30);

   mCurrBrushId = 0;
   mSurfaceKey  = 0;

   mWorldEntity = NULL;

   setGraphGeneration(false);
}

EditGeometry::~EditGeometry()
{
   for (U32 i = 0; i < mEntities.size(); i++) {
      delete mEntities[i];
      mEntities[i] = NULL;
   }
   delete mWorldEntity;
   mWorldEntity = NULL;

   for (U32 i = 0; i < mStructuralBrushes.size(); i++) {
      gBrushArena.freeBrush(mStructuralBrushes[i]);
      mStructuralBrushes[i] = NULL;
   }
   for (U32 i = 0; i < mDetailBrushes.size(); i++) {
      gBrushArena.freeBrush(mDetailBrushes[i]);
      mDetailBrushes[i] = NULL;
   }
   for (U32 i = 0; i < mPortalBrushes.size(); i++) {
      gBrushArena.freeBrush(mPortalBrushes[i]);
      mPortalBrushes[i] = NULL;
   }
   for (U32 i = 0; i < mSpecialCollisionBrushes.size(); i++) {
      gBrushArena.freeBrush(mSpecialCollisionBrushes[i]);
      mSpecialCollisionBrushes[i] = NULL;
   }
   for (U32 i = 0; i < mVehicleCollisionBrushes.size(); i++) {
      gBrushArena.freeBrush(mVehicleCollisionBrushes[i]);
      mVehicleCollisionBrushes[i] = NULL;
   }
   for (U32 i = 0; i < mTextureNames.size(); i++) {
      delete [] mTextureNames[i];
      mTextureNames[i] = NULL;
   }
   for (U32 i = 0; i < mTextures.size(); i++) {
      delete mTextures[i];
      mTextures[i] = NULL;
   }
   for (U32 i = 0; i < mZones.size(); i++) {
      delete mZones[i];
      mZones[i] = NULL;
   }
   for (U32 i = 0; i < mPortals.size(); i++) {
      delete mPortals[i];
      mPortals[i] = NULL;
   }

   for (U32 i = 0; i < mSurfaces.size(); i++) {
      delete mSurfaces[i].pLightDirMap;
      delete mSurfaces[i].pNormalLMap;
      delete mSurfaces[i].pAlarmLMap;
      mSurfaces[i].pNormalLMap = NULL;
      mSurfaces[i].pAlarmLMap  = NULL;
      mSurfaces[i].pLightDirMap = NULL;
   }
   for (U32 i = 0; i < mLightmaps.size(); i++) {
      delete mLightmaps[i];
      mLightmaps[i] = NULL;
   }

   for(U32 i = 0; i < mLightDirMaps.size(); i++)
   {
      delete mLightDirMaps[i];
      mLightDirMaps[i] = NULL;
   }

   for (U32 i = 0; i < mAnimatedLights.size(); i++) {
      AnimatedLight* pLight = mAnimatedLights[i];
      delete [] pLight->name;
      pLight->name = NULL;
      pLight->type = 0;

      for (U32 j = 0; j < pLight->states.size(); j++) {
         LightState* pState = pLight->states[j];
         for (U32 k = 0; k < pState->stateData.size(); k++)
            delete pState->stateData[k].pLMap;

         delete pState;
      }

      delete pLight;
   }
}

void EditGeometry::nudge()
{
}

//------------------------------------------------------------------------------
bool EditGeometry::parseMapFile(Tokenizer* pToker)
{
   extern int gQuakeVersion;

   // Make sure that "NULL" is the first (0 index) textures
   AssertFatal(mTextureNames.size() == 0, "Internal Error, mTextureNames must be empty here!");
   insertTexture("NULL");
   insertTexture("ORIGIN");
   insertTexture("TRIGGER");
   insertTexture("EMITTER");

   while (pToker->advanceToken(true)) {
      if (pToker->getToken()[0] != '{')
         goto EntityError;


      pToker->advanceToken(true);
      if (pToker->tokenICmp("classname") == false)
         goto EntityError;

      if (pToker->advanceToken(false) == false)
         goto EntityError;

      Entity* pEntity = parseEntity(pToker);
      if (dynamic_cast<WorldSpawnEntity*>(pEntity) != NULL) {
         if (mWorldEntity == NULL)
            mWorldEntity = static_cast<WorldSpawnEntity*>(pEntity);
         else
            mEntities.push_back(pEntity);
      } else {
         // Some other sort of entity.  Need to save these...
         if (pEntity != NULL)
            mEntities.push_back(pEntity);
      }
      AssertISV(pEntity != NULL, avar("Unknown entity classname: %s", pToker->getToken()));

      while (pToker->getToken()[0] == '{') {
         // In case we don't know what brush type we're dealing with, we can
         //  throw it out...
         CSGBrush  trashBrush;
         CSGBrush* pBrush;
         
         if (pEntity != NULL) {
            switch (pEntity->getBrushType()) {
               case StructuralBrush:
                  mStructuralBrushes.push_back(gBrushArena.allocateBrush());
                  pBrush = mStructuralBrushes.last();
                  pBrush->mBrushType = StructuralBrush;

                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;
               case DetailBrush:
                  mDetailBrushes.push_back(gBrushArena.allocateBrush());
                  pBrush = mDetailBrushes.last();
                  pBrush->mBrushType = DetailBrush;

                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;
               case PortalBrush:
                  AssertFatal(mWorldEntity != NULL, "World entity should have been first!");
                  mPortalBrushes.push_back(gBrushArena.allocateBrush());
                  mPortalEntities.push_back(pEntity);
                  pBrush = mPortalBrushes.last();
                  pBrush->mBrushType = PortalBrush;

                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;
               case CollisionBrush:
                  mSpecialCollisionBrushes.push_back(gBrushArena.allocateBrush());
                  pBrush = mSpecialCollisionBrushes.last();
                  pBrush->mBrushType = CollisionBrush;

                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;

               case VehicleCollisionBrush:
                  mVehicleCollisionBrushes.push_back(gBrushArena.allocateBrush());
                  pBrush = mVehicleCollisionBrushes.last();
                  pBrush->mBrushType = VehicleCollisionBrush;

                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;

               case LightBrush:
               {
                  LightBrushEntity *lb = (LightBrushEntity*) pEntity;
                  pBrush = &lb->mBrush;

                  pBrush->mBrushType   = LightBrush;
                  pBrush->materialType = 0;
                  pBrush->brushId      = mCurrBrushId++;
                  break;
               }

               default:
                  AssertISV(false, "Unknown brush type returned from entity");
                  pBrush = &trashBrush;
                  break;
            }
         }
         else {
            pBrush = &trashBrush;
         }
         CSGBrush& rBrush = *pBrush;

         pToker->advanceToken(true);

         if (!parseBrush (rBrush, pToker, *this)) goto EntityBrushlistError;

         if (pToker->getToken()[0] != '}')
            goto EntityBrushlistError;

         mNumAmbiguousBrushes += (rBrush.disambiguate() == true) ? 1 : 0;
         pToker->advanceToken(true);
      }
      
      if (pToker->getToken()[0] != '}')
         goto EntityBrushlistError;
   }

   for (U32 i = 0; i < mEntities.size(); i++) {
      if (dynamic_cast<MirrorSurfaceEntity*>(mEntities[i]) != NULL) {
         MirrorSurfaceEntity* entity = static_cast<MirrorSurfaceEntity*>(mEntities[i]);

         entity->markSurface(mStructuralBrushes, mDetailBrushes);
      }
   }

   // Finished!  At this point, all the Entities should be entered, each with
   //  their Brushes planed and entered into the appropriate brush list...
   //
   AssertISV(mWorldEntity != NULL, "Invalid map: must have a worldspawn entity");
   return true;

   // Parsing errors. Yuck. Gotos.
   //
EntityError:
   dPrintf("Error processing entity on or near line: %s: %d", pToker->getFileName(),
           pToker->getCurrentLine());
   return false;

EntityBrushlistError:
   dPrintf("Error processing entity brushlist on or near line: %s: %d", pToker->getFileName(),
           pToker->getCurrentLine());
   return false;
}   


EditGeometry::Entity* EditGeometry::parseEntity(Tokenizer* pToker)
{
   Entity* pEntity = NULL;

   //if (pToker->tokenICmp(WorldSpawnEntity::getClassName()))
   //   pEntity = new WorldSpawnEntity;
   //else if (pToker->tokenICmp(DetailEntity::getClassName()))
   //   pEntity = new DetailEntity;
   //else if (pToker->tokenICmp(CollisionEntity::getClassName()))
   //   pEntity = new CollisionEntity;
   //else if (pToker->tokenICmp(VehicleCollisionEntity::getClassName()))
   //   pEntity = new VehicleCollisionEntity;
   //else if (pToker->tokenICmp(PortalEntity::getClassName()))
   //   pEntity = new PortalEntity;
   //else if (pToker->tokenICmp(TargetEntity::getClassName()))
   //   pEntity = new TargetEntity;

   //// lighting: emitters
   //else if (pToker->tokenICmp(PointEmitterEntity::getClassName()))
   //   pEntity = new PointEmitterEntity;
   //else if (pToker->tokenICmp(SpotEmitterEntity::getClassName()))
   //   pEntity = new SpotEmitterEntity;

   //// lighting: lights
   //else if (pToker->tokenICmp(LightEntity::getClassName()))
   //   pEntity = new LightEntity;

   //// lighting: scripted lights
   //else if (pToker->tokenICmp(LightOmniEntity::getClassName()))
   //   pEntity = new LightOmniEntity;
   //else if (pToker->tokenICmp(LightSpotEntity::getClassName()))
   //   pEntity = new LightSpotEntity;
   //else if(pToker->tokenICmp(LightBrushEntity::getClassName()))
   //   pEntity = new LightBrushEntity;

   //// lighting: animated lights
   //else if (pToker->tokenICmp(LightStrobeEntity::getClassName()))
   //   pEntity = new LightStrobeEntity;
   //else if (pToker->tokenICmp(LightPulseEntity::getClassName()))
   //   pEntity = new LightPulseEntity;
   //else if (pToker->tokenICmp(LightPulse2Entity::getClassName()))
   //   pEntity = new LightPulse2Entity;
   //else if (pToker->tokenICmp(LightFlickerEntity::getClassName()))
   //   pEntity = new LightFlickerEntity;
   //else if (pToker->tokenICmp(LightRunwayEntity::getClassName()))
   //   pEntity = new LightRunwayEntity;

   //// Special classes
   //else if (pToker->tokenICmp(MirrorSurfaceEntity::getClassName()))
   //   pEntity = new MirrorSurfaceEntity;
   //else if (pToker->tokenICmp(DoorEntity::getClassName()))
   //   pEntity = new DoorEntity;
   //else if (pToker->tokenICmp(ForceFieldEntity::getClassName()))
   //   pEntity = new ForceFieldEntity;
   //else if (pToker->tokenICmp(SpecialNodeEntity::getClassName()))
   //   pEntity = new SpecialNodeEntity;

   // Path classes
   //else if (pToker->tokenICmp(PathNodeEntity::getClassName()))
   //   pEntity = new PathNodeEntity;

   //// Trigger Classes
   //else if (pToker->tokenICmp(TriggerEntity::getClassName()))
   //   pEntity = new TriggerEntity;

   //else {
   //   pEntity = new GameEntity(pToker->getToken());
   //   // Unknown
   //}

   //if (pEntity) {
   //   if (pEntity->parseEntityDescription(pToker) == false) {
   //      dPrintf("Error processing entity on or near line: %s: %d", pToker->getFileName(),
   //              pToker->getCurrentLine());
   //      delete pEntity;
   //      return NULL;
   //   }
   //}

   return pEntity;
}


const EditGeometry::Entity* EditGeometry::getNamedEntity(const char* name) const
{
   for (U32 i = 0; i < mEntities.size(); i++)
      if (dStricmp(name, mEntities[i]->getName()) == 0)
         return mEntities[i];

   return NULL;
}


//------------------------------------------------------------------------------
const char* EditGeometry::insertTexture(const char* pInsert)
{
   AssertFatal(pInsert != NULL && pInsert[0] != '\0', "EditGeometry::insertTexture: Bad texture name");

   for (U32 i = 0; i < mTextureNames.size(); i++) {
      if (dStricmp(mTextureNames[i], pInsert) == 0)
         return mTextureNames[i];
   }

   char* pCopy = new char[dStrlen(pInsert) + 1];
   dStrcpy(pCopy, pInsert);
   mTextureNames.push_back(pCopy);

   return pCopy;
}

//------------------------------------------------------------------------------

// F64   gPlaneNormThresh = 1.0;
// F64   gPlaneDistThresh = 0.0;
F64   gPlaneNormThresh = 0.99;
F64   gPlaneDistThresh = 0.01;

U32 EditGeometry::insertPlaneEQ(const Point3D& normal, const F64 dist)
{
   Point3D insertNormal = normal;
   F64  insertDist   = dist;

   F64 len    = insertNormal.len();
   insertNormal /= len;
   insertDist   /= len;

   U32 hash = calcHash(insertNormal, insertDist);
   
   PlaneHashEntry* pSearch = mPlaneHashTable[hash].pNext;
   if( mHashPlanes ){
      while (pSearch != NULL) {
         if (mDot(insertNormal, mPlaneEQs[pSearch->planeIndex].normal) > gPlaneNormThresh &&
             mFabs(insertDist - mPlaneEQs[pSearch->planeIndex].dist) < gPlaneDistThresh) {
            return pSearch->planeIndex;
         }

         pSearch = pSearch->pNext;
      }
   }
   else{
      while (pSearch != NULL) {
         if( insertNormal == mPlaneEQs[pSearch->planeIndex].normal &&
             insertDist == mPlaneEQs[pSearch->planeIndex].dist )
            return pSearch->planeIndex;
         pSearch = pSearch->pNext;
      }
   }

   // Gotta insert!  Also, put it's negative in the table as well...
   //
   PlaneHashEntry* pInsert     = mPlaneHashArena.allocateEntry();
   pInsert->pNext              = mPlaneHashTable[hash].pNext;
   mPlaneHashTable[hash].pNext = pInsert;
   pInsert->planeIndex         = mPlaneEQs.size();

   mPlaneEQs.increment();
   mPlaneEQs.last().normal.x  = insertNormal.x;
   mPlaneEQs.last().normal.y  = insertNormal.y;
   mPlaneEQs.last().normal.z  = insertNormal.z;
   mPlaneEQs.last().dist      = insertDist;
   
   hash = calcHash(-insertNormal, -insertDist);

   PlaneHashEntry* pNegative   = mPlaneHashArena.allocateEntry();
   pNegative->pNext            = mPlaneHashTable[hash].pNext;
   mPlaneHashTable[hash].pNext = pNegative;
   pNegative->planeIndex       = mPlaneEQs.size();

   mPlaneEQs.increment();
   mPlaneEQs.last().normal.x  = -insertNormal.x;
   mPlaneEQs.last().normal.y  = -insertNormal.y;
   mPlaneEQs.last().normal.z  = -insertNormal.z;
   mPlaneEQs.last().dist      = -insertDist;

   return pInsert->planeIndex;
}


//------------------------------------------------------------------------------
U32 EditGeometry::insertPoint(const Point3D& rPoint)
{
   U32 baseHash = calcHash(rPoint);

   if( ! mHashPoints ){
      // keeping all points distinct
      PointHashEntry * pSearch = mPointHashTable[baseHash].pNext;
      while( pSearch != NULL ) {
         if( mPoints[pSearch->pointIndex] == rPoint )
            return pSearch->pointIndex;
         pSearch = pSearch->pNext;
      }
      // exact match not found, insert
      PointHashEntry * pInsert = mPointHashArena.allocateEntry();
      pInsert->pNext = mPointHashTable[baseHash].pNext;
      mPointHashTable[baseHash].pNext = pInsert;
      mPoints.push_back( rPoint );
      return( pInsert->pointIndex = mPoints.size() - 1 );
   }

   PointHashEntry* pSearch = mPointHashTable[baseHash].pNext;
   while (pSearch != NULL) {
      if (mFabs(mPoints[pSearch->pointIndex].x - rPoint.x) < gcPlaneDistanceEpsilon &&
          mFabs(mPoints[pSearch->pointIndex].y - rPoint.y) < gcPlaneDistanceEpsilon &&
          mFabs(mPoints[pSearch->pointIndex].z - rPoint.z) < gcPlaneDistanceEpsilon) {
         if ((mPoints[pSearch->pointIndex] - rPoint).len() < gcPlaneDistanceEpsilon) {
            return pSearch->pointIndex;
         }
      }
      pSearch = pSearch->pNext;
   }

   // Oops, not found.  Insert it.  Lame and slow, but hey.
   const static F64 dists[3] = { -gcPlaneDistanceEpsilon, 0.0, gcPlaneDistanceEpsilon };

   UniqueVector bins(27);
   for (U32 i = 0; i < 3; i++) 
      for (U32 j = 0; j < 3; j++) 
         for (U32 k = 0; k < 3; k++)
            bins.pushBackUnique(calcHash(rPoint + Point3D(dists[i], dists[j], dists[k])));

   for (U32 i = 0; i < bins.size(); i++) {
      PointHashEntry* pInsert = mPointHashArena.allocateEntry();
      pInsert->pNext = mPointHashTable[bins[i]].pNext;
      mPointHashTable[bins[i]].pNext = pInsert;
      pInsert->pointIndex = mPoints.size();
   }

   mPoints.increment();
   mPoints.last() = rPoint;

   return mPoints.size() - 1;
}


//------------------------------------------------------------------------------
bool EditGeometry::createBSP()
{
   createBrushPolys();
   
   buildBSP();

   //if (gVerbose) {
   //   U32 totalLeaves    = 0;
   //   U32 totalLeafDepth = 0;
   //   U32 maxLeafDepth   = 0;
   //   mBSPRoot->gatherBSPStats(&totalLeaves, &totalLeafDepth, &maxLeafDepth, 0);

   //   dPrintf("     * BSP Stats\n"
   //           "        Total Nodes:  %d\n"
   //           "        (Av./Max) Depth: %g / %d\n", (mNodeArena.mBuffers.size() - 1) * mNodeArena.arenaSize + mNodeArena.currPosition,
   //           F32(totalLeafDepth) / F32(totalLeaves),
   //           maxLeafDepth);
   //   flushedOutput("\n");
   //}

   return true;
}


//------------------------------------------------------------------------------
void EditGeometry::createBrushPolys()
{
   // Has the side effect of creating the Geometry's bounding box.  We need
   //  to expand that box slightly at the end of the operation, to ensure
   //  good vislinks between nodes on the outside in some cases.

   // Structural
   for (U32 i = 0; i < mStructuralBrushes.size();)
   {
      if(mStructuralBrushes[i]->selfClip())
         i++;
      else
         mStructuralBrushes.erase(i);
   }
   // Detail
   for (U32 i = 0; i < mDetailBrushes.size();)
   {
      if(mDetailBrushes[i]->selfClip())
          i++;
      else
         mDetailBrushes.erase(i);
   }

   // Portal
   for (U32 i = 0; i < mPortalBrushes.size();)
   {
      if(mPortalBrushes[i]->selfClip())
         i++;
      else
         mPortalBrushes.erase(i);
   }

   mMinBound -= Point3D(1, 1, 1);
   mMaxBound += Point3D(1, 1, 1);
}

//------------------------------------------------------------------------------
void EditGeometry::buildBSP()
{
   Vector<CSGBrush*> copyStructural;
   Vector<CSGBrush*> copyDetail;
   for (U32 i = 0; i < mStructuralBrushes.size(); i++) {
      copyStructural.push_back(gBrushArena.allocateBrush());
      copyStructural.last()->copyBrush(mStructuralBrushes[i]);
   }
   for (U32 i = 0; i < mDetailBrushes.size(); i++) {
      copyDetail.push_back(gBrushArena.allocateBrush());
      copyDetail.last()->copyBrush(mDetailBrushes[i]);
   }

   mBSPRoot               = mNodeArena.allocateNode(NULL);
   mBSPRoot->planeType    = EditBSPNode::Structural;

   //-------------------------------------- Structural Brushes
   //
   mBSPRoot->brushList.reserve(copyStructural.size());
   for (U32 i = 0; i < copyStructural.size(); i++)
      mBSPRoot->brushList.push_back(copyStructural[i]);
   copyStructural.clear();

   mBSPRoot->createBSP(EditBSPNode::Structural);

   //-------------------------------------- Portal Brushes
   //
   Vector<EditBSPNode::PortalEntry*> portalEntries(mPortalBrushes.size());
   for (U32 i = 0; i < mPortalEntities.size(); i++) {
      CSGBrush* pBrush = mPortalBrushes[i];
      EditBSPNode::PortalEntry* pMunged = mBSPRoot->mungePortalBrush(pBrush);

      if (pMunged != NULL) {
         Entity* pEntity = mPortalEntities[i];
         PortalEntity* pPortalEntity = dynamic_cast<PortalEntity*>(pEntity);
         AssertFatal(pPortalEntity, "Internal Error: we should have had a portal here");

         portalEntries.push_back(pMunged);
         portalEntries.last()->portalId = portalEntries.size() - 1;

         // Get our own portal entry setup
         mPortals.push_back(new Portal);
         mPortals.last()->portalId         = portalEntries.size() - 1;
         mPortals.last()->planeEQIndex     = -1;
         mPortals.last()->frontZone        = -1;
         mPortals.last()->backZone         = -1;
         mPortals.last()->passAmbientLight = pPortalEntity->passAmbientLight;
         mPortals.last()->x                = portalEntries.last()->ortho1;
         mPortals.last()->y                = portalEntries.last()->ortho2;
      }
   }
   for (U32 i = 0; i < portalEntries.size(); i++)
      mBSPRoot->insertPortalEntry(portalEntries[i]);
   portalEntries.clear();
   
   mBSPRoot->createVisLinks();

   if (gVerbose) {
      U32 emptyLeaves = 0;
      U32 totalLinks  = 0;
      U32 maxLinks    = 0;
      mBSPRoot->gatherVISStats(&emptyLeaves, &totalLinks, &maxLinks);
   
      dPrintf("     * VIS Stats\n"
              "        # Empty leaves: %d\n"
              "        Av. Links/Node: %g\n"
              "        Max Links/Node: %d", emptyLeaves,
              F32(totalLinks) / F32(emptyLeaves),
              maxLinks);
      flushedOutput("\n");
   }

   // Very lame here.  We need to ensure that the outside zone is zero, so find the leaf
   //  for a point way outside the interior, and flood that manually.  Then we can proceed
   //  with the recursive function.
   Point3D testPoint = mMinBound - Point3D(10, 10, 10);
   EditBSPNode* currNode = mBSPRoot;
   while (currNode->planeEQIndex != -1) {
      const PlaneEQ& rPlane = getPlaneEQ(currNode->planeEQIndex);
      if (rPlane.whichSide(testPoint) == PlaneBack) currNode = currNode->pBack;
      else                                          currNode = currNode->pFront;
   }
   AssertFatal(currNode->isSolid == false, "Internal Error: should not be solid!");
   mZones.push_back(new EditGeometry::Zone);
   S32 markZoneId = (mZones.size() - 1);
   mZones.last()->zoneId = markZoneId;
   currNode->floodZone(markZoneId);

   mBSPRoot->zoneFlood();
   findOutsideZone();
   AssertFatal(mOutsideZoneIndex == 0, "Internal Error: outside zone must always be 0");

   mBSPRoot->gatherPortals();
   for (U32 i = 0; i < mPortals.size(); i++) {
      Portal* pPortal = mPortals[i];

      pPortal->mMin = Point3D( 1e10,  1e10,  1e10);
      pPortal->mMax = Point3D(-1e10, -1e10, -1e10);

      for (U32 j = 0; j < pPortal->windings.size(); j++) {
         for (U32 k = 0; k < pPortal->windings[j].numIndices; k++) {
            pPortal->mMin.setMin(getPoint(pPortal->windings[j].indices[k]));
            pPortal->mMax.setMax(getPoint(pPortal->windings[j].indices[k]));
         }
      }
   }

   enterPortalZoneRefs();

   floodAmbientLight();

   mBSPRoot->removeVISInfo();
   mVisLinkArena.clearAll();

   //-------------------------------------- Detail and Brushes
   //
   mBSPRoot->brushList.reserve(copyDetail.size());
   for (U32 i = 0; i < copyDetail.size(); i++)
      mBSPRoot->brushList.push_back(copyDetail[i]);
   copyDetail.clear();
   mBSPRoot->createBSP(EditBSPNode::Detail);
}

void EditGeometry::enterPortalZoneRefs()
{
   for (U32 i = 0; i < mZones.size(); i++) {
      Zone* pZone = mZones[i];
      for (U32 j = 0; j < mPortals.size(); j++) {
         Portal* pPortal = mPortals[j];

         if (U32(pPortal->frontZone) == U32(pZone->zoneId) ||
             U32(pPortal->backZone)  == U32(pZone->zoneId))
            pZone->referencingPortals.push_back(j);
      }
   }
}


void EditGeometry::ambientVisitZone(const U32 zone)
{
   if (mZones[zone]->ambientLit == true)
      return;

   Zone* pZone = mZones[zone];

   pZone->ambientLit = true;
   for (U32 i = 0; i < pZone->referencingPortals.size(); i++) {
      Portal* pPortal = mPortals[pZone->referencingPortals[i]];

      S32 otherZone = -1;
      if (pPortal->frontZone != pZone->zoneId)
         otherZone = pPortal->frontZone;
      if (pPortal->backZone != pZone->zoneId)
         otherZone = pPortal->backZone;

      if (otherZone != -1 && pPortal->passAmbientLight)
         ambientVisitZone(U32(otherZone));
   }
}

void EditGeometry::floodAmbientLight()
{
   for (U32 i = 0; i < mZones.size(); i++)
      mZones[i]->ambientLit = false;

   ambientVisitZone(0);
}


void EditGeometry::createSurfaces()
{
   for (U32 i = 0; i < mStructuralBrushes.size(); i++)
      createBrushSurfaces(*mStructuralBrushes[i]);
   sortSurfaces();

   for (U32 i = 0; i < mDetailBrushes.size(); i++)
      createBrushSurfaces(*mDetailBrushes[i]);
   sortSurfaces();

   for (U32 i = 0; i < mSurfaces.size(); i++) {
      mSurfaces[i].originalWinding = mSurfaces[i].winding;
      mSurfaces[i].pLightDirMap = NULL;
      mSurfaces[i].pNormalLMap = NULL;
      mSurfaces[i].pAlarmLMap  = NULL;
   }

   sortSurfaces();
   while (fixTJuncs() == false)
      sortSurfaces();

   markSurfaceOriginalPoints();
   convertToStrips();
}



//--------------------------------------------------------------------------
void EditGeometry::markEmptyZones()
{
   // Always leave zone 0, but strip out any zones that don't have
   for (U32 i = 0; i < mZones.size(); i++) {
      //if (i == 0 || mZones[i]->referencingPortals.size() != 0)
         mZones[i]->active = true;
      //else
      //   mZones[i]->active = false;
   }
}



//------------------------------------------------------------------------------
void EditGeometry::createBrushSurfaces(const CSGBrush& brush)
{
   static U32 sPassCount = 0;
   sPassCount++;

   Vector<Winding> planeFinalWindings(64);
   Vector<Winding> tempWindings(1);
   for (U32 i = 0; i < brush.mPlanes.size(); i++) {
      const CSGPlane& rPlane = brush.mPlanes[i];

      tempWindings.increment();
      tempWindings.last() = rPlane.winding;
      tempWindings.last().numNodes   = 0;
      tempWindings.last().numZoneIds = 0;

      mBSPRoot->breakWinding(tempWindings, rPlane.planeEQIndex,
                             &brush, planeFinalWindings);

      if (planeFinalWindings.size() != 0) {
         // Need to create a surface for each of these windings...
         //
         bool active = false;
         for (U32 k = 0; k < planeFinalWindings.size() && !active; k++) {
            for (U32 j = 0; j < planeFinalWindings[k].numZoneIds; j++) {
               if (mZones[planeFinalWindings[k].zoneIds[j]]->active == true)
                  active = true;
            }
         }

         if (rPlane.pTextureName &&
             dStricmp(rPlane.pTextureName, "NULL")    != 0 &&
             dStricmp(rPlane.pTextureName, "ORIGIN")  != 0 &&
             dStricmp(rPlane.pTextureName, "TRIGGER") != 0 &&
             active == true) {
            U32 texGenIndex = rPlane.texGenIndex;

            for (U32 j = 0; j < planeFinalWindings.size(); j++) {
               mSurfaces.increment();
               Surface& rSurface = mSurfaces.last();
               rSurface.uniqueKey = mSurfaceKey++;

               rSurface.planeIndex   = rPlane.planeEQIndex;

               rSurface.textureIndex = getMaterialIndex(rPlane.pTextureName);
               rSurface.texGenIndex  = texGenIndex;

               rSurface.winding         = planeFinalWindings[j];
               rSurface.winding.brushId = brush.brushId;

               // Set flags...

               rSurface.flags  = (brush.mBrushType == DetailBrush)      ? Interior::SurfaceDetail    : 0;
               rSurface.flags |= (brush.mIsAmbiguous == true)           ? Interior::SurfaceAmbiguous : 0;
               rSurface.flags |= (planeFinalWindings[j].numNodes == 0)  ? Interior::SurfaceOrphan    : 0;

               // We know that either all of a surfaces zones are outside zones, or they're
               //  all inside zones.  Therefore, we can just look at the first...
               if (rSurface.winding.numZoneIds > 0)
                  rSurface.flags |= (mZones[rSurface.winding.zoneIds[0]]->ambientLit) ? Interior::SurfaceOutsideVisible : 0;

               if (planeFinalWindings[i].numNodes == 0)
                  mNumOrphanPolys++;

               if (rPlane.owningEntity != NULL) {
                  // If the entity grabs the surface but leaves it in the interior
                  //  for processing (mirrors, for instance), it does it here.
                  rPlane.owningEntity->grabSurface(rSurface);
               }
            }
         } else {
            for (U32 j = 0; j < planeFinalWindings.size(); j++) {
               mNullSurfaces.increment();
               NullSurface& rSurface = mNullSurfaces.last();

               rSurface.planeIndex      = rPlane.planeEQIndex;
               rSurface.winding         = planeFinalWindings[j];
               rSurface.winding.brushId = brush.brushId;

               rSurface.flags  = (brush.mBrushType == DetailBrush)      ? Interior::SurfaceDetail    : 0;
               rSurface.flags |= (brush.mIsAmbiguous == true)           ? Interior::SurfaceAmbiguous : 0;
               rSurface.flags |= (planeFinalWindings[j].numNodes == 0)  ? Interior::SurfaceOrphan    : 0;

               if (planeFinalWindings[i].numNodes == 0)
                  mNumOrphanPolys++;
            }
         }
      }

      planeFinalWindings.clear();
      tempWindings.clear();
   }
}

//--------------------------------------------------------------------------
void EditGeometry::rotateSurfaceToNonColinear(Surface& surface)
{
   // Assumes the originalWinding and winding members are the same.  Make sure that
   //  the first three verts are non-colinear.  Might be best to search for the best
   //  corner...
   AssertISV(surface.originalWinding.numIndices == surface.winding.numIndices, "Internal Error: This should definitely never happen!");

   U32 i;
   U32 bestIndex = 0xFFFFFFFF;
   F64 bestDot   = 1.0f;

   for (i = 0; i < surface.winding.numIndices; i++) {
      U32 idx0 = (i + 0) % surface.winding.numIndices;
      U32 idx1 = (i + 1) % surface.winding.numIndices;
      U32 idx2 = (i + 2) % surface.winding.numIndices;

      Point3D vec0 = getPoint(surface.winding.indices[idx1]) - getPoint(surface.winding.indices[idx0]);
      Point3D vec1 = getPoint(surface.winding.indices[idx2]) - getPoint(surface.winding.indices[idx1]);
      vec0.normalize();
      vec1.normalize();

      F64 dot = mFabs(mDot(vec0, vec1));
      if (dot < bestDot) {
         bestIndex = i;
         bestDot = dot;
      }
   }
   AssertFatal(bestIndex != 0xFFFFFFFF, "Internal Error: that friggin poly has got to turn a corner at some point!");

   for (i = bestIndex; i < surface.winding.numIndices; i++)
      surface.winding.indices[i - bestIndex] = surface.originalWinding.indices[i];

   for (i = 0; i < bestIndex; i++)
      surface.winding.indices[i + (surface.winding.numIndices - bestIndex)] = surface.originalWinding.indices[i];

   surface.originalWinding = surface.winding;
}

//------------------------------------------------------------------------------
bool EditGeometry::fixTJuncs()
{
   
   // We need to do two passes here.  First structural, then detail...
   //  DMMNOTE: This just yells out for consolidation, also, it may not
   //   be necessary to restart the process after every insertion of a
   //   new surface anymore.  Check that out.

   // First, lets get all the unique points in this zone.  These are the
   //  only points that we are allowed to consider...
   BitVector includedPoints(mPoints.size());
   includedPoints.clear();

   Vector<U32> uniquePoints(mSurfaces.size() * 3);
   UniqueVector coplanar;

   for (U32 i = 0; i < mSurfaces.size(); i++) {
      if (mSurfaces[i].flags & Interior::SurfaceDetail)
         continue;
      for (U32 j = 0; j < mSurfaces[i].winding.numIndices; j++)
         includedPoints.set(mSurfaces[i].winding.indices[j]);
   }
   for (U32 i = 0; i < mPoints.size(); i++) {
      if (includedPoints.test(i))
         uniquePoints.push_back(i);
   }

   static const U32 maxTJuncPoints = 256;
   U32 finalPoints[256];
   U32 finalNumPoints;
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      coplanar.clear();
      Surface& rSurface = mSurfaces[i];
      if (rSurface.flags & Interior::SurfaceDetail)
         continue;
      dMemcpy(finalPoints, rSurface.winding.indices, sizeof(U32) * rSurface.winding.numIndices);
      finalNumPoints = rSurface.winding.numIndices;

      const PlaneEQ& rPlane = getPlaneEQ(rSurface.planeIndex);

      // Ok, only points that are coplanar are considered.  Make sure the
      //  winding points already in are at the front so we can disregard them...
      //
      for (U32 j = 0; j < finalNumPoints; j++)
         coplanar.pushBackUnique(finalPoints[j]);
      U32 myPoints = coplanar.size();

      for (U32 j = 0; j < uniquePoints.size(); j++)
         if (rPlane.whichSide(getPoint(uniquePoints[j])) == PlaneOn)
            coplanar.pushBackUnique(uniquePoints[j]);

      // Ok, we have a list of coplanar points.  We need to remove our own points
      //  from this list.  Slow.
      for (U32 j = 0; j < myPoints; j++)
         coplanar.erase(U32(0));

      // Now, spin through the edges, and insert any points that are colinear with
      //  an edge, and lie within that edge...
      U32 origSize = coplanar.size();
      while (coplanar.size() != 0) {
         U32 cp = coplanar.last();
         coplanar.decrement();

         U32 start = 0;
         U32 end   = 1;
         do {
            U32 ps = finalPoints[start];
            U32 pe = finalPoints[end];

            if (pointsAreColinear(ps, pe, cp)) {
               // Ok, the points are colinear, does coplanar[j] fall between ps and pe?
               Point3D v = getPoint(pe) - getPoint(ps);
               v.normalize();

               F64 dot = mDot(getPoint(cp) - getPoint(ps), v);
               if (dot > 0.0 &&
                   dot < mDot(getPoint(pe) - getPoint(ps), v)) {
                  // Yup!  Insert the sucker...
                  finalNumPoints++;
                  AssertFatal(finalNumPoints <= maxTJuncPoints, "Internal Error: too many points!");
                  if (end < start) {
                     // In this scenario, this can only happen if the point needs to
                     //  go at the very end.
                     finalPoints[finalNumPoints-1] = cp;
                  } else {
                     dMemmove(&finalPoints[end + 1],
                              &finalPoints[end],
                              (finalNumPoints - end - 1) * sizeof(U32));
                     finalPoints[end] = cp;
                  }
                  break;
               }
            }

            start = end;
            end   = (end + 1) % finalNumPoints;
         } while (end != 1);
      }

      // Copy the points back in...
      if (finalNumPoints <= 32) {
         dMemcpy(rSurface.winding.indices, finalPoints, sizeof(U32) * finalNumPoints);
         rSurface.winding.numIndices = finalNumPoints;
      } else {
         while (finalNumPoints > 32) {
            U32 poly1[256];
            U32 poly2[256];
            U32 poly1Verts = 32;
            U32 poly2Verts = finalNumPoints - 30;

            U32 splitPoint;
            for (splitPoint = 0; splitPoint < finalNumPoints; splitPoint++) {
               for (U32 j = 0; j < 32; j++) {
                  poly1[j] = finalPoints[(splitPoint + j) % finalNumPoints];
               }
               poly2[0] = poly1[0];
               poly2[1] = poly1[31];
               for (U32 j = 32; j < finalNumPoints; j++) {
                  poly2[j - 30] = finalPoints[(splitPoint + j) % finalNumPoints];
               }

               // Ok, check to make sure that both of these have at least _some_
               //  non-colinear verts...
               //
               bool poly1Colinear = true;
               bool poly2Colinear = true;
               for (U32 j = 0; j < 32; j++) {
                  U32 j1 = (j + 1) % 32;
                  U32 j2 = (j + 2) % 32;

                  if (pointsAreColinear(poly1[j], poly1[j1], poly1[j2]) == false) {
                     poly1Colinear = false;
                     break;
                  }
               }

               for (U32 j = 0; j < poly2Verts; j++) {
                  U32 j1 = (j + 1) % poly2Verts;
                  U32 j2 = (j + 2) % poly2Verts;

                  if (pointsAreColinear(poly2[j], poly2[j1], poly2[j2]) == false) {
                     poly2Colinear = false;
                     break;
                  }
               }

               if (poly1Colinear == false && poly2Colinear == false)
                  break;
            }
            AssertFatal(splitPoint != finalNumPoints, "Internal Error: there must be a possible split somewhere!");

            // Poly1 Holds the vertices for the new surface, poly2 is copied back
            //  into final points...
            mSurfaces.increment();
            Surface& lastSurface = mSurfaces.last();   // NOTENOTENOTE: rSurface can become invalid here!
            lastSurface = mSurfaces[i];
            lastSurface.uniqueKey = mSurfaceKey++;

            // We copy out the first 32 verts of the finalPoints
            dMemcpy(lastSurface.winding.indices, poly1, sizeof(U32) * 32);
            lastSurface.winding.numIndices = 32;

            lastSurface.originalWinding = lastSurface.winding;
            rotateSurfaceToNonColinear(lastSurface);

            dMemcpy(finalPoints, poly2, sizeof(U32) * poly2Verts);
            finalNumPoints = poly2Verts;
         }

         // rSurface could have become invalid above...
         dMemcpy(mSurfaces[i].winding.indices, finalPoints, sizeof(U32) * finalNumPoints);
         mSurfaces[i].winding.numIndices = finalNumPoints;
         mSurfaces[i].originalWinding = rSurface.winding;
         rotateSurfaceToNonColinear(mSurfaces[i]);

         return false;
      }
   }

   includedPoints.clear();
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      if (mSurfaces[i].flags & Interior::SurfaceDetail)
         for (U32 j = 0; j < mSurfaces[i].winding.numIndices; j++)
            includedPoints.set(mSurfaces[i].winding.indices[j]);
   }
   for (U32 i = 0; i < mPoints.size(); i++) {
      if (includedPoints.test(i))
         uniquePoints.push_back(i);
   }

   for (U32 i = 0; i < mSurfaces.size(); i++) {
      coplanar.clear();
      Surface& rSurface = mSurfaces[i];
      if (!(rSurface.flags & Interior::SurfaceDetail))
         continue;
      dMemcpy(finalPoints, rSurface.winding.indices, sizeof(U32) * rSurface.winding.numIndices);
      finalNumPoints = rSurface.winding.numIndices;

      const PlaneEQ& rPlane = getPlaneEQ(rSurface.planeIndex);

      // Ok, only points that are coplanar are considered.  Make sure the
      //  winding points already in are at the front so we can disregard them...
      //
      for (U32 j = 0; j < finalNumPoints; j++)
         coplanar.pushBackUnique(finalPoints[j]);
      U32 myPoints = coplanar.size();

      for (U32 j = 0; j < uniquePoints.size(); j++)
         if (rPlane.whichSide(getPoint(uniquePoints[j])) == PlaneOn)
            coplanar.pushBackUnique(uniquePoints[j]);

      // Ok, we have a list of coplanar points.  We need to remove our own points
      //  from this list.  Slow.
      for (U32 j = 0; j < myPoints; j++)
         coplanar.erase(U32(0));

      // Now, spin through the edges, and insert any points that are colinear with
      //  an edge, and lie within that edge...
      while (coplanar.size() != 0) {
         U32 cp = coplanar.last();
         coplanar.decrement();

         U32 start = 0;
         U32 end   = 1;
         do {
            U32 ps = finalPoints[start];
            U32 pe = finalPoints[end];

            if (pointsAreColinear(ps, pe, cp)) {
               // Ok, the points are colinear, does coplanar[j] fall between ps and pe?
               Point3D v = getPoint(pe) - getPoint(ps);
               v.normalize();

               F64 dot = mDot(getPoint(cp) - getPoint(ps), v);
               if (dot > 0.0 &&
                   dot < mDot(getPoint(pe) - getPoint(ps), v)) {
                  // Yup!  Insert the sucker...
                  finalNumPoints++;
                  AssertFatal(finalNumPoints <= MaxWindingPoints, "Internal Error: too many points!");
                  if (end < start) {
                     // In this scenario, this can only happen if the point needs to
                     //  go at the very end.
                     finalPoints[finalNumPoints-1] = cp;
                  } else {
                     dMemmove(&finalPoints[end + 1],
                              &finalPoints[end],
                              (finalNumPoints - end - 1) * sizeof(U32));
                     finalPoints[end] = cp;
                  }
                  break;
               }
            }

            start = end;
            end   = (end + 1) % finalNumPoints;
         } while (end != 1);
      }

      // Copy the points back in...
      if (finalNumPoints <= 32) {
         dMemcpy(rSurface.winding.indices, finalPoints, sizeof(U32) * finalNumPoints);
         rSurface.winding.numIndices = finalNumPoints;
      } else {
         while (finalNumPoints > 32) {
            U32 poly1[256];
            U32 poly2[256];
            U32 poly1Verts = 32;
            U32 poly2Verts = finalNumPoints - 30;

            U32 splitPoint;
            for (splitPoint = 0; splitPoint < finalNumPoints; splitPoint++) {
               for (U32 j = 0; j < 32; j++) {
                  poly1[j] = finalPoints[(splitPoint + j) % finalNumPoints];
               }
               poly2[0] = poly1[0];
               poly2[1] = poly1[31];
               for (U32 j = 32; j < finalNumPoints; j++) {
                  poly2[j - 30] = finalPoints[(splitPoint + j) % finalNumPoints];
               }

               // Ok, check to make sure that both of these have at least _some_
               //  non-colinear verts...
               //
               bool poly1Colinear = true;
               bool poly2Colinear = true;
               for (U32 j = 0; j < 32; j++) {
                  U32 j1 = (j + 1) % 32;
                  U32 j2 = (j + 2) % 32;

                  if (pointsAreColinear(poly1[j], poly1[j1], poly1[j2]) == false) {
                     poly1Colinear = false;
                     break;
                  }
               }

               for (U32 j = 0; j < poly2Verts; j++) {
                  U32 j1 = (j + 1) % poly2Verts;
                  U32 j2 = (j + 2) % poly2Verts;

                  if (pointsAreColinear(poly2[j], poly2[j1], poly2[j2]) == false) {
                     poly2Colinear = false;
                     break;
                  }
               }

               if (poly1Colinear == false && poly2Colinear == false)
                  break;
            }
            AssertFatal(splitPoint != finalNumPoints, "Internal Error: there must be a possible split somewhere!");

            // Poly1 Holds the vertices for the new surface, poly2 is copied back
            //  into final points...
            mSurfaces.increment();
            Surface& lastSurface = mSurfaces.last();   // NOTENOTENOTE: rSurface can become invalid here!
            lastSurface = mSurfaces[i];
            lastSurface.uniqueKey = mSurfaceKey++;

            // We copy out the first 32 verts of the finalPoints
            dMemcpy(lastSurface.winding.indices, poly1, sizeof(U32) * 32);
            lastSurface.winding.numIndices = 32;

            lastSurface.originalWinding = lastSurface.winding;
            rotateSurfaceToNonColinear(lastSurface);

            dMemcpy(finalPoints, poly2, sizeof(U32) * poly2Verts);
            finalNumPoints = poly2Verts;
         }

         // rSurface could have become invalid above...
         dMemcpy(mSurfaces[i].winding.indices, finalPoints, sizeof(U32) * finalNumPoints);
         mSurfaces[i].winding.numIndices = finalNumPoints;
         mSurfaces[i].originalWinding = rSurface.winding;
         rotateSurfaceToNonColinear(mSurfaces[i]);

         return false;
      }
   }

   return true;
}

void EditGeometry::markSurfaceOriginalPoints()
{
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      Surface& rSurface = mSurfaces[i];
      U32 mask = 0;

      AssertFatal(mSurfaces[i].originalWinding.numIndices <= rSurface.winding.numIndices,
                  avar("How did we lose verts? (%d / %d)", mSurfaces[i].originalWinding.numIndices, rSurface.winding.numIndices));

      for (U32 j = 0; j < rSurface.winding.numIndices; j++) {
         bool found = false;
         for (U32 k = 0; k < mSurfaces[i].originalWinding.numIndices; k++) {

            if (rSurface.winding.indices[j] == mSurfaces[i].originalWinding.indices[k]) {
               found = true;
               break;
            }
         }

         if (found)
            mask |= (1 << j);
      }

      rSurface.fanMask = mask;
   }
}

void EditGeometry::convertToStrips()
{
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      Surface& rSurface = mSurfaces[i];

      // Convert to strip order.  This proceedes by taking the first three
      //  verts from the fan, and then the last, the fourth, then the next
      //  to last, etc, etc.
      U32 newWinding[32];

      newWinding[0] = rSurface.winding.indices[0];
      U32 idx = 1;

      U32 front = 1;
      U32 back  = rSurface.winding.numIndices - 1;
      while (idx < rSurface.winding.numIndices) {
         // Get the back
         newWinding[idx++] = rSurface.winding.indices[front++];
         if (idx >= rSurface.winding.numIndices)
            break;
         newWinding[idx++] = rSurface.winding.indices[back--];
      }
      AssertFatal(idx == rSurface.winding.numIndices, "Internal Error, problem converting fan to strip!");
      dMemcpy(rSurface.winding.indices, newWinding, idx * sizeof(U32));
   }
}


//--------------------------------------------------------------------------
// NOTE! DO NOT Change this without knowing EXACTLY what you're doing.  Rendering
//  performance for the interiors can be _extremely_ sensitive to surface sort
//  order.
//--------------------------------------------------------------------------
S32 QSORT_CALLBACK
cmpSurfaceFunc(const void* p1, const void* p2)
{
   // Sort criteria is:
   //  Alarm/Normal lightmap
   //  Zone    id
   //  Texture index
   //  TexGen  plane eq
   //
   const EditGeometry::Surface* pS1 = reinterpret_cast<const EditGeometry::Surface*>(p1);
   const EditGeometry::Surface* pS2 = reinterpret_cast<const EditGeometry::Surface*>(p2);

   if ((pS1->pAlarmLMap == NULL) != (pS2->pAlarmLMap == NULL)) {
      // Surfaces sharing a lightmap go to the front...
      if (pS1->pAlarmLMap == NULL)
         return 1;
      else
         return -1;
   } else {
      if (pS1->textureIndex != pS2->textureIndex) {
         return S32(pS1->textureIndex - pS2->textureIndex);
      } else {
         // Let's try to sort by zone...
         int index = 0;
         while (index < pS1->winding.numZoneIds && index < pS2->winding.numZoneIds) {
            if (pS1->winding.zoneIds[index] != pS2->winding.zoneIds[index]) {
               if (pS1->winding.zoneIds[index] < pS2->winding.zoneIds[index])
                  return -1;
               else
                  return 1;
            }
            index++;
         }

         if (pS1->winding.numZoneIds < pS2->winding.numZoneIds)
            return -1;
         else if (pS2->winding.numZoneIds < pS1->winding.numZoneIds)
            return 1;
         else {
            // Well, the zones are the same, let's sort on plane...
            if (pS1->planeIndex > pS2->planeIndex)
               return -1;
            else if (pS2->planeIndex > pS1->planeIndex)
               return 1;
            else
               return 0;
         }
      }
   }
}


//--------------------------------------------------------------------------
// NOTE! DO NOT Change this without knowing EXACTLY what you're doing.  Rendering
//  performance for the interiors can be _extremely_ sensitive to surface sort
//  order.
//--------------------------------------------------------------------------
S32 QSORT_CALLBACK
cmpSurfaceLitFunc(const void* p1, const void* p2)
{
   // Sort criteria is:
   //  Alarm/Normal lightmap
   //  Animated lights affect surface/don't affect surface
   //  Texture index
   //  Zone    id
   //  TexGen  plane eq
   //
   const EditGeometry::Surface* pS1 = reinterpret_cast<const EditGeometry::Surface*>(p1);
   const EditGeometry::Surface* pS2 = reinterpret_cast<const EditGeometry::Surface*>(p2);

   if ((pS1->pAlarmLMap == NULL) != (pS2->pAlarmLMap == NULL)) {
      // Surfaces sharing a lightmap go to the front...
      if (pS1->pAlarmLMap == NULL)
         return 1;
      else
         return -1;
   } else {
      // If one surface isn't affected by animated lights, and the other is
      if ((pS1->numLights == 0) != (pS2->numLights == 0))
      {
         // sort the "not" to the front
         if (pS1->numLights == 0)
            return -1;
         else
            return 1;
      }
      else
      {
         if (pS1->textureIndex != pS2->textureIndex)
         {
            return S32(pS1->textureIndex - pS2->textureIndex);
         } else
         {
            // Let's try to sort by zone...
            int index = 0;
            while (index < pS1->winding.numZoneIds && index < pS2->winding.numZoneIds)
            {
               if (pS1->winding.zoneIds[index] != pS2->winding.zoneIds[index])
               {
                  if (pS1->winding.zoneIds[index] < pS2->winding.zoneIds[index])
                     return -1;
                  else
                     return 1;
               }
               index++;
            }

            if (pS1->winding.numZoneIds < pS2->winding.numZoneIds)
            {
               return -1;
            }
            else if (pS2->winding.numZoneIds < pS1->winding.numZoneIds)
            {
               return 1;
            }
            else
            {
               // Well, the zones are the same, let's sort on plane...
               if (pS1->planeIndex > pS2->planeIndex)
                  return -1;
               else if (pS2->planeIndex > pS1->planeIndex)
                  return 1;
               else
                  return 0;
            }
         }
      }
   }
}

S32 QSORT_CALLBACK
cmpSurfaceLitIndexFunc(const void* p1, const void* p2)
{
   U32 idx1 = *((U32*)p1);
   U32 idx2 = *((U32*)p2);

   const EditGeometry::Surface* pS1 = &gWorkingGeometry->mSurfaces[idx1];
   const EditGeometry::Surface* pS2 = &gWorkingGeometry->mSurfaces[idx2];

   return cmpSurfaceLitFunc(pS1, pS2);
}

S32 QSORT_CALLBACK
cmpZoneIds(const void* p1, const void* p2)
{
   U32 i1 = *((const U32*)p1);
   U32 i2 = *((const U32*)p2);

   if (i1 < i2)
      return 1;
   else if (i2 < i1)
      return -1;
   else
      return 0;
}


void EditGeometry::sortSurfaces()
{
   for (U32 i = 0; i < mSurfaces.size(); i++) {
      if (mSurfaces[i].winding.numZoneIds > 1) {
         dQsort(mSurfaces[i].winding.zoneIds, mSurfaces[i].winding.numZoneIds, sizeof(U32), cmpZoneIds);
         dMemcpy(mSurfaces[i].originalWinding.zoneIds, mSurfaces[i].winding.zoneIds, sizeof(U32) * 8);
      }
   }

   dQsort(mSurfaces.address(), mSurfaces.size(), sizeof(Surface), cmpSurfaceFunc);
}


void EditGeometry::sortLitSurfaces()
{
   U32* pIndices        = new U32[mSurfaces.size()];
   U32* pReverseIndices = new U32[mSurfaces.size()];
   for (U32 i = 0; i < mSurfaces.size(); i++)
      pIndices[i] = i;

   dQsort(pIndices, mSurfaces.size(), sizeof(U32), cmpSurfaceLitIndexFunc);
   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      mSurfaces[i].temptemptemp = i;
      pReverseIndices[pIndices[i]] = i;
   }

   // Back patch the surfaces into the lights
   for (U32 i = 0; i < mAnimatedLights.size(); i++)
   {
      AnimatedLight* pLight = mAnimatedLights[i];
      for (U32 j = 0; j < pLight->states.size(); j++)
      {
         LightState* pState = pLight->states[j];
         for (U32 k = 0; k < pState->stateData.size(); k++)
         {
            StateData& rData = pState->stateData[k];
            rData.surfaceIndex = pReverseIndices[rData.surfaceIndex];
         }
      }
   }

   // Actually do the surface sort
   dQsort(mSurfaces.address(), mSurfaces.size(), sizeof(Surface), cmpSurfaceLitFunc);
   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      AssertFatal(mSurfaces[i].temptemptemp == pIndices[i], "Internal Error: sorts mismatch!");
   }
   
   delete [] pIndices;
   delete [] pReverseIndices;
}


//------------------------------------------------------------------------------
void EditGeometry::packLMaps()
{
   // We need to sort the lightmaps into 8 catagories
   // Shared Alarm/Normal
   //   Indoor
   //     Unanimated
   //     Animated
   //   Outdoor
   //     Unanimated
   //     Animated
   // Normal/Alarm
   //   Indoor
   //     Unanimated
   //     Animated
   //   Outdoor
   //     Unanimated
   //     Animated
   //
   // Arbitrarily, well just assign enums to the top level as follows:
   //  Normal: 0
   //  Shared: 1

   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      mSurfaces[i].sheetIndex = 0xFFFFFFFF;
      mSurfaces[i].alarmSheetIndex = 0xFFFFFFFF;
   }
   
   Vector<U32> surfaceLists[8];

   if (!mHasAlarmState)
   {
      for (U32 i = 0; i < mSurfaces.size(); i++)
      {
         U32 code = 0;
         U32 zoneId = mSurfaces[i].winding.zoneIds[0];
         if (mZones[zoneId] && mZones[zoneId]->ambientLit)
            code |= 0x2;
         if (mSurfaces[i].numLights != 0)
            code |= 0x1;
         surfaceLists[code].push_back(i);
      }      
      for (U32 i = 0; i < 4; i++)
      {
         SheetManager* pManager = new SheetManager;
         
         pManager->begin();
         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];

            AssertFatal(rSurface.pNormalLMap != NULL, "Internal Error: No lightmap?");
            rSurface.sheetIndex = pManager->enterLightMap(rSurface.pNormalLMap);
         }
         pManager->end();

         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            SheetManager::LightMapEntry rEntry = pManager->getLightmap(rSurface.sheetIndex);
            rSurface.sheetIndex      = mLightmaps.size() + rEntry.sheetId;
            rSurface.offsetX         = rEntry.x;
            rSurface.offsetY         = rEntry.y;

            AssertFatal(U32(rSurface.lMapDimX) == U32(rEntry.width) && U32(rSurface.lMapDimY) == U32(rEntry.height),
                        "Internal Error: Something got mixed up here");
         }
         for (U32 i = 0; i < pManager->m_sheets.size(); i++)
            mLightmaps.push_back(new GBitmap(*pManager->m_sheets[i].pData));

         delete pManager;
      }

      // light dir map
      for(U32 i = 0; i < 4; i++)
      {
         SheetManager* pManager = new SheetManager;

         pManager->begin();
         for(U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];

            AssertFatal(rSurface.pNormalLMap != NULL, "Internal Error: No lightmap?");
            rSurface.sheetIndex = pManager->enterLightMap(rSurface.pLightDirMap);
         }
         pManager->end();

         for(U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            SheetManager::LightMapEntry rEntry = pManager->getLightmap(rSurface.sheetIndex);
            rSurface.sheetIndex      = mLightDirMaps.size() + rEntry.sheetId;
            rSurface.offsetX         = rEntry.x;
            rSurface.offsetY         = rEntry.y;

            AssertFatal(U32(rSurface.lMapDimX) == U32(rEntry.width) && U32(rSurface.lMapDimY) == U32(rEntry.height),
                        "Internal Error: Something got mixed up here");
         }
         for(U32 i = 0; i < pManager->m_sheets.size(); i++)
         {
            mLightDirMaps.push_back(new GBitmap(*pManager->m_sheets[i].pData));
         }

         delete pManager;
      }
   }
   else
   {
      for (U32 i = 0; i < mSurfaces.size(); i++) {
         if (mSurfaces[i].pNormalLMap != NULL) {
            AssertFatal(mSurfaces[i].pAlarmLMap,
                        "Internal Error: if we have an alarm state, any surface with a normal lightmap must have an alarm equivalent");

            const U8* normalBits = mSurfaces[i].pNormalLMap->getBits();
            const U8* alarmBits  = mSurfaces[i].pAlarmLMap->getBits();

            if (dMemcmp(alarmBits, normalBits, mSurfaces[i].pNormalLMap->byteSize) == 0) {
               // Lightmaps are the same.  Nuke the alarm version...
               delete mSurfaces[i].pAlarmLMap;
               mSurfaces[i].pAlarmLMap = NULL;
            }
         }
      }
      
      for (U32 i = 0; i < mSurfaces.size(); i++)
      {
         U32 code = 0;
         if (mSurfaces[i].pNormalLMap != NULL && mSurfaces[i].pAlarmLMap == NULL)
            code = 1 << 2;
         if (mZones[mSurfaces[i].winding.zoneIds[0]]->ambientLit)
            code |= 0x2;
         if (mSurfaces[i].numLights != 0)
            code |= 0x1;
         surfaceLists[code].push_back(i);
      }      

      // Do the shared lightmaps first
      for (U32 i = 4; i < 8; i++)
      {
         SheetManager* pManager = new SheetManager;
         
         pManager->begin();
         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];

            AssertFatal(rSurface.pNormalLMap != NULL, "Internal Error: No lightmap?");
            rSurface.sheetIndex = pManager->enterLightMap(rSurface.pNormalLMap);
         }
         pManager->end();

         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            SheetManager::LightMapEntry rEntry = pManager->getLightmap(rSurface.sheetIndex);
            rSurface.sheetIndex      = mLightmaps.size() + rEntry.sheetId;
            rSurface.alarmSheetIndex = mLightmaps.size() + rEntry.sheetId;
            rSurface.offsetX         = rEntry.x;
            rSurface.offsetY         = rEntry.y;

            AssertFatal(U32(rSurface.lMapDimX) == U32(rEntry.width) && U32(rSurface.lMapDimY) == U32(rEntry.height),
                        "Internal Error: Something got mixed up here");
         }
         for (U32 i = 0; i < pManager->m_sheets.size(); i++)
            mLightmaps.push_back(new GBitmap(*pManager->m_sheets[i].pData));

         delete pManager;
      }
      
      // Do the normal/alarm pairs next
      for (U32 i = 0; i < 4; i++)
      {
         SheetManager* pManager = new SheetManager;
         
         pManager->begin();
         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            AssertFatal(rSurface.pNormalLMap != NULL, "Internal Error: No lightmap?");
            rSurface.sheetIndex = pManager->enterLightMap(rSurface.pNormalLMap);
         }
         pManager->end();

         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            SheetManager::LightMapEntry rEntry = pManager->getLightmap(rSurface.sheetIndex);
            rSurface.sheetIndex      = mLightmaps.size() + (rEntry.sheetId * 2) + 0;
            rSurface.alarmSheetIndex = mLightmaps.size() + (rEntry.sheetId * 2) + 1;
            rSurface.offsetX         = rEntry.x;
            rSurface.offsetY         = rEntry.y;

            AssertFatal(U32(rSurface.lMapDimX) == U32(rEntry.width) && U32(rSurface.lMapDimY) == U32(rEntry.height),
                        "Internal Error: Something got mixed up here");
         }
         for (U32 i = 0; i < pManager->m_sheets.size(); i++)
         {
            mLightmaps.push_back(new GBitmap(*pManager->m_sheets[i].pData));
            mLightmaps.push_back(new GBitmap(*pManager->m_sheets[i].pData));
         }

         for (U32 j = 0; j < surfaceLists[i].size(); j++)
         {
            Surface& rSurface = mSurfaces[(surfaceLists[i])[j]];
            GBitmap* pMap = mLightmaps[rSurface.alarmSheetIndex];
            for (U32 y = 0; y < rSurface.lMapDimY; y++)
            {
               const U8* pSrc = rSurface.pAlarmLMap->getAddress(0, y);
               U8* pDst       = pMap->getAddress(rSurface.offsetX, rSurface.offsetY + y);
               dMemcpy(pDst, pSrc, rSurface.lMapDimX * rSurface.pAlarmLMap->bytesPerPixel);
            }
         }         
         
         delete pManager;
      }
   }
   
   // Remove the lightmaps, and adjust the texgen for all surfaces
   //
   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      delete mSurfaces[i].pLightDirMap;
      delete mSurfaces[i].pNormalLMap;
      delete mSurfaces[i].pAlarmLMap;
      mSurfaces[i].pLightDirMap = NULL;
      mSurfaces[i].pNormalLMap = NULL;
      mSurfaces[i].pAlarmLMap  = NULL;

      AssertFatal(mSurfaces[i].sheetIndex != 0xFFFFFFFF, "Internal Error: Bogus sheet index!");
      if (mHasAlarmState)
         AssertFatal(mSurfaces[i].alarmSheetIndex != 0xFFFFFFFF, "Internal Error: Bogus sheet index!");

      adjustLMapTexGen(mSurfaces[i]);
   }
}


//------------------------------------------------------------------------------
void EditGeometry::findOutsideZone()
{
   // Really easy.  Basically, just create a point outside the bounding
   //  box of the shape, find the leaf that it's in, and grab it's zone
   //  id.  We require that the leaf be empty, and the zoneId not be -1.

   Point3D testPoint = mMaxBound + Point3D(10, 10, 10);
   EditBSPNode* currNode = mBSPRoot;

   while (currNode->planeEQIndex != -1) {
      const PlaneEQ& rPlane = getPlaneEQ(currNode->planeEQIndex);

      // We arbitrarily place "on" points in the front.  it shouldn't matter.
      //
      if (rPlane.whichSide(testPoint) == PlaneBack)
         currNode = currNode->pBack;
      else
         currNode = currNode->pFront;
   }

   AssertFatal(currNode->isSolid == false, "Internal Error: we should be in an empty leaf");
   AssertFatal(currNode->zoneId != -1, "Internal Error: leaf's zoneId cannot be -1");

   mOutsideZoneIndex = currNode->zoneId;
}

//------------------------------------------------------------------------------
//-------------------------------------- Arenas
//
EditGeometry::PlaneHashArena::PlaneHashArena(U32 _arenaSize)
{
   AssertFatal(_arenaSize > 0, "Internal Error: impossible size");
   arenaSize = _arenaSize;

   currBuffer   = new EditGeometry::PlaneHashEntry[arenaSize];
   currPosition = 0;
   mBuffers.push_back(currBuffer);
}

EditGeometry::PlaneHashArena::~PlaneHashArena()
{
   arenaSize    = 0;
   currBuffer   = NULL;
   currPosition = 0;

   for (U32 i = 0; i < mBuffers.size(); i++) {
      delete [] mBuffers[i];
      mBuffers[i] = NULL;
   }
}

EditGeometry::PointHashArena::PointHashArena(U32 _arenaSize)
{
   AssertFatal(_arenaSize > 0, "Internal Error: impossible size");
   arenaSize = _arenaSize;

   currBuffer   = new EditGeometry::PointHashEntry[arenaSize];
   currPosition = 0;
   mBuffers.push_back(currBuffer);
}

EditGeometry::PointHashArena::~PointHashArena()
{
   arenaSize    = 0;
   currBuffer   = NULL;
   currPosition = 0;

   for (U32 i = 0; i < mBuffers.size(); i++) {
      delete [] mBuffers[i];
      mBuffers[i] = NULL;
   }
}


int FN_CDECL cmpZoneNum(const void* p1, const void* p2)
{
   U32 u1 = *((const U32*)p1);
   U32 u2 = *((const U32*)p2);

   if ((u1 & 0x80000000) || (u2 & 0x80000000)) {
      // special zones, make sure they sort to the end...
      if ((u1 & 0x80000000) && (u2 & 0x80000000)) {
         return S32(u1 & 0x7FFFFFFF) - S32(u2 & 0x7FFFFFFF);
      } else if (u1 & 0x80000000) {
         // u1 to end
         return -1;
      } else {
         // u2 to end
         return 1;
      }
   } else {
      return S32(u1) - S32(u2);
   }
}

bool EditGeometry::Surface::isMemberOfZone(U32 zone)
{
   for (U32 i = 0; i < winding.numZoneIds; i++)
      if (winding.zoneIds[i] == zone)
         return true;
   return false;
}
