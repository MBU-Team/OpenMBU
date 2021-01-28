//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "map2dif plus/createLightmaps.h"
#include "gfx/gBitmap.h"
#include "math/mMath.h"
#include "core/bitMatrix.h"
#include "interior/interior.h"

#ifdef DUMP_LIGHTMAPS
#include "core/fileStream.h"
#endif

//------------------------------------------------------------------------------
// globals
//------------------------------------------------------------------------------
namespace {
   static char * newStrDup(const char * src)
   {
      char * buffer = new char [dStrlen(src) + 1];
      dStrcpy(buffer, src);
      return(buffer);
   }

   static const F32 AnimationTimes[] = { 2.f, 1.f, 0.5f, 0.25f, 0.125f };
      
   Lighting * gWorkingLighting;
}

extern bool gBuildAsLowDetail;

//------------------------------------------------------------------------------
// Class Lighting
//------------------------------------------------------------------------------

Lighting::Lighting() :
   mNumAmbiguousPlanes(0),
   mNodeRepository(0),
   mWindingStore(0),
   mSurfaceEmitterInfos(0),
   mEmitterSurfaceIndices(0)
{
}

Lighting::~Lighting()
{
   for(U32 i = 0; i < mLights.size(); i++)
      delete mLights[i];
   for(U32 i = 0; i < mEmitters.size(); i++)
      delete mEmitters[i];

   // destroy all the vectors
   for(U32 i = 0; i < mSurfaces.size(); i++)
      for(U32 j = 0; j < mSurfaces[i]->mNumEmitters; j++)
         mSurfaces[i]->mEmitters[j]->mShadowed.~UniqueVector();
   
   //
   mEmitterInfoChunker.clear();
   mNodeChunker.clear();
   mSurfaceChunker.clear();
   
   //
   delete [] mSurfaceEmitterInfos;
   delete [] mEmitterSurfaceIndices;
}

//------------------------------------------------------------------------------

Lighting::SVNode * Lighting::getShadowVolume(U32 index)
{
   return(mShadowVolumes[index]);
}

Lighting::Surface * Lighting::getSurface(U32 index)
{
   return(mSurfaces[index]);
}

//------------------------------------------------------------------------------

F64 Lighting::getLumelScale(Surface &surf)
{
   EditGeometry::Surface & surface = gWorkingGeometry->mSurfaces[surf.mSurfaceIndex];

   if(surface.isInside)
      return(gWorkingGeometry->mWorldEntity->mInsideLumelScale);
   else
      return(gWorkingGeometry->mWorldEntity->mOutsideLumelScale);
}

//------------------------------------------------------------------------------

void Lighting::grabLights(bool alarmMode)
{
   // grab all the light emitters and targets first..
   for(U32 i = 0; i < gWorkingGeometry->mEntities.size(); i++)
   {
      BaseLightEmitterEntity * emitter = dynamic_cast<BaseLightEmitterEntity*>(gWorkingGeometry->mEntities[i]);
      if(emitter)
         mBaseLightEmitters.push_back(emitter);

      TargetEntity * target = dynamic_cast<TargetEntity*>(gWorkingGeometry->mEntities[i]);
      if(target)
         mTargets.push_back(target);
   }
 
   // process all the lights
   for(U32 i = 0; i < gWorkingGeometry->mEntities.size(); i++)
   {
      BaseLightEntity * entity = dynamic_cast<BaseLightEntity*>(gWorkingGeometry->mEntities[i]);
      if(entity)
      {
         if((entity->mAlarmStatus == BaseLightEntity::NormalOnly && alarmMode) ||
            (entity->mAlarmStatus == BaseLightEntity::AlarmOnly && !alarmMode))
            continue;

         Light * light = new Light;
         if(!light->build(entity))
         {
            delete light;
            continue;
         }
         
         light->alarm = alarmMode;
         mLights.push_back(light);
      }
   }

   // Merge the animated lights that allow it...
   //
   for (U32 i = 0; i < mLights.size(); i++)
   {
      Light* lightCompare = mLights[i];
      if (!lightCompare->mAnimated)
         continue;
      
      for (U32 j = i + 1; j < mLights.size();)
      {
         Light* lightPossible = mLights[j];
         if (!lightPossible->mAnimated ||
             (lightCompare->mNumStates != lightPossible->mNumStates ||
              lightCompare->mAnimType  != lightPossible->mAnimType  ||
              lightCompare->alarm      != lightPossible->alarm))
         {
            j++;
            continue;
         }

         
         bool valid = true;
         for (U32 k = 0; k < lightCompare->mNumStates && valid == true; k++)
         {
            const Light::LightState& state1 = mLightStates[lightCompare->mStateIndex + k];
            const Light::LightState& state2 = mLightStates[lightPossible->mStateIndex + k];

            if (state1.mDuration != state2.mDuration)
               valid = false;

            // All the emitters in an animated light share the same color, so
            //  we can just check the first...
            if (mEmitters[state1.mEmitterIndex]->mColor != mEmitters[state2.mEmitterIndex]->mColor)
               valid = false;
         }

         if (valid)
         {
            // Since the states are the same, we just want to loop through them, and reassign their
            //  emitters.  We'll just put the new emitter range at the end of the mEmitters array,
            //  null the current pointers.
            for (U32 k = 0; k < lightCompare->mNumStates; k++)
            {
               Light::LightState*       state1 = &mLightStates[lightCompare->mStateIndex + k];
               const Light::LightState* state2 = &mLightStates[lightPossible->mStateIndex + k];

               U32 newEmitterIndex = mEmitters.size();
               Emitter* pEmitter;
               for (U32 l = 0; l < state1->mNumEmitters; l++)
               {
                  pEmitter = mEmitters[state1->mEmitterIndex + l];
                  pEmitter->mIndex = mEmitters.size();
                  mEmitters.push_back(pEmitter);
                  mEmitters[state1->mEmitterIndex + l] = NULL;
               }
               for (U32 l = 0; l < state2->mNumEmitters; l++)
               {
                  pEmitter = mEmitters[state2->mEmitterIndex + l];
                  pEmitter->mIndex = mEmitters.size();
                  mEmitters.push_back(pEmitter);
                  mEmitters[state2->mEmitterIndex + l] = NULL;
               }
               state1->mEmitterIndex = newEmitterIndex;
               state1->mNumEmitters  = mEmitters.size() - newEmitterIndex;
            }

            delete mLights[j];
            mLights.erase(j);
         }
         else
         {
            // Step to the next light...
            j++;
         }
      }
   }
}

//------------------------------------------------------------------------------

void Lighting::convertToFan(MiniWinding & winding, U32 fanMask)
{
   U32 tmpIndices[LightingMaxWindingPoints];
   U32 fanIndices[LightingMaxWindingPoints];
   
   tmpIndices[0] = 0;
   
   U32 idx = 1;
   for(U32 i = 1; i < winding.mNumIndices; i += 2)
      tmpIndices[idx++] = i;
   for(U32 i = ((winding.mNumIndices - 1) & (~0x1)); i > 0; i -= 2)
      tmpIndices[idx++] = i;
      
   idx = 0;
   for(U32 i = 0; i < winding.mNumIndices; i++) 
      if(fanMask & (1 << i))
         fanIndices[idx++] = winding.mIndices[tmpIndices[i]];

   // set the data
   winding.mNumIndices = idx;
   for(U32 i = 0; i < winding.mNumIndices; i++)
      winding.mIndices[i] = fanIndices[i];
}

void Lighting::copyWinding(MiniWinding & dest, Winding & src)
{
   dest.mNumIndices = src.numIndices;
   dMemcpy(dest.mIndices, src.indices, src.numIndices * sizeof(U32));
}

//------------------------------------------------------------------------------

void Lighting::grabSurfaces()
{
   for(U32 i = 0; i < gWorkingGeometry->mSurfaces.size(); i++)
   {
      Surface * surface = createSurface();
      surface->mSurfaceIndex = i;
      copyWinding(surface->mWinding, gWorkingGeometry->mSurfaces[i].winding);
      surface->mPlaneIndex = gWorkingGeometry->mSurfaces[i].planeIndex;

      // convert the surface...
      convertToFan(surface->mWinding, gWorkingGeometry->mSurfaces[i].fanMask);
      mSurfaces.push_back(surface);
   }
}

Lighting::EmitterInfo * Lighting::createEmitterInfo()
{
   // clear it out (will not call vector ctor's)
   EmitterInfo * info = mEmitterInfoChunker.alloc();
   dMemset(info, 0, sizeof(EmitterInfo));
   return(info);
}

//------------------------------------------------------------------------------

void Lighting::processSurfaces()
{
   Vector<U32> list;
   list.reserve(mSurfaces.size() * 6);
   
   // create a run list for the surfaces..
   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      mSurfaces[i]->mNumEmitters = 0;

      //
      for(U32 j = 0; j < mEmitters.size(); j++)
      {
         if(mEmitters[j] != NULL && mEmitters[j]->isSurfaceLit(mSurfaces[i]))
         {
            mSurfaces[i]->mNumEmitters++;
            list.push_back(j);
         }
      }
   }
   
   if(!list.size())
      return;

   mSurfaceEmitterInfos = new EmitterInfo* [list.size()];

   U32 curOffset = 0;

dPrintf( "Setting Emitter Info On Surfaces\n" );

   // now walk this and set the emitterinfo pntr/size, also create the infos
   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      mSurfaces[i]->mEmitters = &mSurfaceEmitterInfos[curOffset];
      
      const U32 & numEmitters = mSurfaces[i]->mNumEmitters;

      if(numEmitters)
      {
         for(U32 j = 0; j < numEmitters; j++)
         {
            EmitterInfo * eInfo = createEmitterInfo();
            eInfo->mEmitter = list[curOffset++];
            mSurfaces[i]->mEmitters[j] = eInfo;
            mEmitters[eInfo->mEmitter]->mNumSurfaces++;
         }
      }
   }

dPrintf( "Setting Surface Info On Emitters\n" );

   // now do the same for the emitters - equally sized lists..
   mEmitterSurfaceIndices = new U32 [list.size()];

   curOffset = 0;
   for(U32 i = 0; i < mEmitters.size(); i++)
   {
      if (mEmitters[i] == NULL)
         continue;
      
      mEmitters[i]->mSurfaces = &mEmitterSurfaceIndices[curOffset];

      U32 count = mEmitters[i]->mNumSurfaces;
      curOffset += count;
      U32 curSurface = 0;
      
      // go until filled
      for(U32 j = 0; curSurface != count; j++)
         for(U32 k = 0; k < mSurfaces[j]->mNumEmitters; k++)
            if(mSurfaces[j]->mEmitters[k]->mEmitter == i)
               mEmitters[i]->mSurfaces[curSurface++] = j;
   }
}

//------------------------------------------------------------------------------

U32 Lighting::constructPlane(const Point3D& Point1, const Point3D& Point2, const Point3D& Point3) const
{
   // |yi zi 1|     |xi zi 1|     |xi yi 1|     |xi yi zi|
   // |yj zj 1| x + |xj zj 1| y + |xj yj 1| z = |xj yj zj|
   // |yk zk 1|     |xk zk 1|     |xk yk 1|     |xk yk zk|
   //
   Point3D normal;
   F64  dist;

   normal.x = Point1.y * Point2.z - Point1.y * Point3.z +
              Point3.y * Point1.z - Point2.y * Point1.z +
              Point2.y * Point3.z - Point3.y * Point2.z;
   normal.y = Point1.x * Point2.z - Point1.x * Point3.z +
              Point3.x * Point1.z - Point2.x * Point1.z +
              Point2.x * Point3.z - Point3.x * Point2.z;
   normal.z = Point1.x * Point2.y - Point1.x * Point3.y +
              Point3.x * Point1.y - Point2.x * Point1.y +
              Point2.x * Point3.y - Point3.x * Point2.y;
   dist     = Point1.x * Point2.y * Point3.z - Point1.x * Point2.z * Point3.y +
              Point1.y * Point2.z * Point3.x - Point1.y * Point2.x * Point3.z +
              Point1.z * Point2.x * Point3.y - Point1.z * Point2.y * Point3.x;

   normal.x = -normal.x;
   normal.z = -normal.z;
   
   //
   return(gWorkingGeometry->insertPlaneEQ(normal, dist));
}

//------------------------------------------------------------------------------

void Lighting::createShadowVolumes()
{
   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      //
      if(!mSurfaces[i]->mNumEmitters)
         continue;

      // create this surface's poly node
      mSurfaces[i]->mPolyNode = createNode(SVNode::PolyPlane);
      *(mSurfaces[i]->mPolyNode->mWinding) = mSurfaces[i]->mWinding;
      mSurfaces[i]->mPolyNode->mPlaneIndex = mSurfaces[i]->mPlaneIndex;

      // create a shadow volume for each of the emitters for this surface
      for(U32 j = 0; j < mSurfaces[i]->mNumEmitters; j++)
      {
         const MiniWinding & winding = mSurfaces[i]->mWinding;
         const Emitter * emitter = mEmitters[mSurfaces[i]->mEmitters[j]->mEmitter];
         
         SVNode * root = 0;
         SVNode ** current = &root;
   
         for(U32 k = 0; k < winding.mNumIndices; k++)
         {
            SVNode * sNode = createNode(SVNode::ShadowPlane);

            // create the plane for this node
            sNode->mPlaneIndex = constructPlane(
               gWorkingGeometry->getPoint(winding.mIndices[(k+1) % winding.mNumIndices]),
               gWorkingGeometry->getPoint(winding.mIndices[k]),
               gWorkingGeometry->getPoint(emitter->mPoint));

            // target the original poly
            sNode->mTarget = mSurfaces[i]->mPolyNode;
            
            // add to the front
            *current = sNode;
            current = &(*current)->mFront;
         }
         
         mSurfaces[i]->mEmitters[j]->mShadowVolume = mShadowVolumes.size();
         mShadowVolumes.push_back(root);
      }
   }
}

//------------------------------------------------------------------------------

void Lighting::processEmitterBSPs()
{
   for(U32 i = 0; i < mEmitters.size(); i++)
      if (mEmitters[i] != NULL)
         mEmitters[i]->processBSP();
}

//------------------------------------------------------------------------------
// simple point store for use with the lexel nodes

void Lighting::flushLexelPoints()
{
   mLexelPoints.setSize(0);
   mLexelPoints.reserve(LexelPointStoreSize);
}

const Point3D & Lighting::getLexelPoint(U32 index)
{
   AssertFatal(index < mLexelPoints.size(), "Lighting::getLexelPoint: index out of range");
   return(mLexelPoints[index]);
}

U32 Lighting::insertLexelPoint(const Point3D & pnt)
{
   if(!mLexelPoints.size())
      mLexelPoints.reserve(LexelPointStoreSize);

   if(mLexelPoints.size() == (mLexelPoints.capacity() - 1))
      mLexelPoints.reserve(mLexelPoints.size() + LexelPointStoreSize);

   mLexelPoints.push_back(pnt);
   return(mLexelPoints.size() - 1);
}

//------------------------------------------------------------------------------

void Lighting::lightSurfaces()
{
   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      F32 percentDone = F32(i) / F32(mSurfaces.size()) * 100.0;
      dPrintf( "Lighting surface %d of %d : %2f percent done.\n", i, mSurfaces.size(), percentDone );

      Surface * surface = mSurfaces[i];

      // must have emitters on this surface
      if(!surface->mNumEmitters)
         continue;

      const PlaneEQ & plane = gWorkingGeometry->getPlaneEQ(surface->mPlaneIndex);
      const EditGeometry::Surface * editSurface = &gWorkingGeometry->mSurfaces[surface->mSurfaceIndex];

      // must have a lightmap
      if(!editSurface->pLMap)
         continue;

      const F32* const & lGenX = editSurface->lmapTexGenX;
      const F32* const & lGenY = editSurface->lmapTexGenY;

      //   
      AssertFatal((lGenX[0] * lGenX[1] == 0.f) && 
                  (lGenX[0] * lGenX[2] == 0.f) &&
                  (lGenX[1] * lGenX[2] == 0.f), "Bad texgen!");
      AssertFatal((lGenY[0] * lGenY[1] == 0.f) && 
                  (lGenY[0] * lGenY[2] == 0.f) &&
                  (lGenY[1] * lGenY[2] == 0.f), "Bad texgen!");


      // get the axis index for the texgens (could be swapped)
      S32 si;
      S32 ti;
      S32 axis = -1;

      //
      if(lGenX[0] == 0.f && lGenY[0] == 0.f)          // YZ
      {
         axis = 0;
         if(lGenX[1] == 0.f) { // swapped?
            si = 2;
            ti = 1;
         } else {
            si = 1;
            ti = 2;
         }
      }
      else if(lGenX[1] == 0.f && lGenY[1] == 0.f)     // XZ
      {
         axis = 1;
         if(lGenX[0] == 0.f) { // swapped?
            si = 2;
            ti = 0;
         } else {
            si = 0;
            ti = 2;
         }
      }
      else if(lGenX[2] == 0.f && lGenY[2] == 0.f)     // XY
      {
         axis = 2;
         if(lGenX[0] == 0.f) { // swapped?
            si = 1;
            ti = 0;
         } else {
            si = 0;
            ti = 1;
         }
      }
      AssertFatal(!(axis == -1), "Lighting::lightSurfaces: bad TexGen!");

      // get the start point for this lightmap (project min to surface)
      Point3D start;
      F64 * pStart = ((F64*)start);
      const F64 * pNormal = ((const F64*)plane.normal);

      // TexGens come in scaled by (1/lightmapscale)
      pStart[si] = -lGenX[3] * getLumelScale(*surface);
      pStart[ti] = -lGenY[3] * getLumelScale(*surface);

      pStart[axis] = ((pNormal[si] * pStart[si]) +
                      (pNormal[ti] * pStart[ti]) + plane.dist) / -pNormal[axis];

      // get the s/t vecs oriented on the surface
      Point3D sVec;
      Point3D tVec;

      F64 * pSVec = ((F64*)sVec);
      F64 * pTVec = ((F64*)tVec);

      Point3D planeNormal;
      F64 angle;

      // s
      pSVec[si] = 1.f;
      pSVec[ti] = 0.f;

      planeNormal = plane.normal;

      ((F64*)planeNormal)[ti] = 0.f;
      planeNormal.normalize();

      angle = mAcos(mClampF(((F64*)planeNormal)[axis], -1.f, 1.f));
      pSVec[axis] = (((F64*)planeNormal)[si] < 0.f) ? mTan(angle) : -mTan(angle);

      // t
      pTVec[ti] = 1.f;
      pTVec[si] = 0.f;

      planeNormal = plane.normal;

      ((F64*)planeNormal)[si] = 0.f;
      planeNormal.normalize();

      angle = mAcos(mClampF(((F64*)planeNormal)[axis], -1.f, 1.f));
      pTVec[axis] = (((F64*)planeNormal)[ti] < 0.f) ? mTan(angle) : -mTan(angle);

      // scale them
      sVec *= getLumelScale(*surface);
      tVec *= getLumelScale(*surface);

      // create all the animated light bitmaps
      for(U32 j = 0; j < surface->mNumEmitters; j++)
      {
         EmitterInfo * emitterInfo = surface->mEmitters[j];
         Emitter * emitter = mEmitters[emitterInfo->mEmitter];
         if(!emitter->mAnimated)
            continue;

         emitterInfo->mLightMap = new GBitmap(editSurface->lMapDimX, editSurface->lMapDimY, false, GFXFormatA8);
      }
      
      Point3D & curPos = start;
      Point3D sRun = sVec * editSurface->lMapDimX;

      // get the lexel area                     
      Point3D cross;
      mCross(sVec, tVec, &cross);
      F64 maxLexelArea = cross.len();

//      BitMatrix outsideLexels(editSurface->lMapDimX, editSurface->lMapDimY);
//      outsideLexels.clearAllBits();

      // get the world coordinate for each lexel
      for(U32 y = 0; y < editSurface->lMapDimY; y++)
      {
         for(U32 x = 0; x < editSurface->lMapDimX; x++)
         {
            SVNode * lNode = createNode(SVNode::LexelPlane);
            lNode->mPlaneIndex = surface->mPlaneIndex;
            lNode->mWinding->mNumIndices = 4;

            // set the poly indices
            lNode->mWinding->mIndices[0] = insertLexelPoint(curPos);
            lNode->mWinding->mIndices[1] = insertLexelPoint(curPos + sVec);
            lNode->mWinding->mIndices[2] = insertLexelPoint(curPos + sVec + tVec);
            lNode->mWinding->mIndices[3] = insertLexelPoint(curPos + tVec);

            ColorF colSum(0,0,0);
            Point3F normalSum( 0.0, 0.0, 0.0 );

            // walk the emitters for each that is not shadowed, light it
            for(U32 j = 0; j < surface->mNumEmitters; j++)
            {
               EmitterInfo * emitterInfo = surface->mEmitters[j];
               Emitter * emitter = mEmitters[emitterInfo->mEmitter];
               
               // get the light value
               RadEmitter *radEmitter = dynamic_cast<RadEmitter*>(emitter);
               F64 intensity;
               if( radEmitter )
               {
                  intensity = radEmitter->calcIntensity( lNode->getCenter(), plane.normal );
               }
               else
               {
                  intensity = emitter->calcIntensity(lNode);
               }

               F64 shadowScale = 1.f;

               // calc the normal sum - THIS NEEDS REVISION
               if( emitter->mBumpSpec )
               {
                  Point3D dir = emitter->mPos - lNode->getCenter();
                  F32 dist = dir.len();

                  F64 scale = 400.0 * 400.0 * 400.0 / (dist * dist * dist);

                  Point3D doubleNorm = Point3D( emitter->mPos - lNode->getCenter() );
                  Point3F norm( doubleNorm.x, doubleNorm.y, doubleNorm.z );
                  norm.normalize();
                  normalSum += norm * scale;
               }
               
               if(emitterInfo->mShadowed.size())
               {
                  // insert a copy of the lexel node, unless last emitter
                  SVNode * nodeStore = 0;
                  if(j == (surface->mNumEmitters-1))
                  {
                     // use the lexel node
                     nodeStore = lNode;
                     lNode = 0;
                  }
                  else
                  {
                     // copy
                     nodeStore = createNode(SVNode::LexelPlane);
                     *nodeStore->mWinding = *lNode->mWinding;
                     nodeStore->mPlaneIndex = lNode->mPlaneIndex;
                  }

//                  // clip the node to own shadow volume
//                  SVNode * tmp = 0;
//                  nodeStore->clipToInverseVolume(&tmp, getShadowVolume(emitterInfo->mShadowVolume));
//                  nodeStore = tmp;
//
//                  // mark the lexel if it is not within own poly
//                  if(!nodeStore)
//                  {
//                     outsideLexels.setBit(x, y);
//                     continue;
//                  }

                  F64 lexelArea = nodeStore->getWindingSurfaceArea();

                  // clip the lexel node(s) to the shadow volumes
                  for(U32 k = 0; nodeStore && (k < emitterInfo->mShadowed.size()); k++)
                  {
                     SVNode * nodeList = 0;
                     SVNode * traverse = nodeStore;
            
                     while(traverse)
                     {
                        SVNode * next = traverse->mFront;
                        SVNode * currentStore = 0;

                        traverse->clipToVolume(&currentStore, getShadowVolume(emitterInfo->mShadowed[k]));
                     
                        if(currentStore)
                           currentStore->move(&nodeList);
                      
                        traverse = next;
                     }
                     nodeStore = nodeList;
                  }

                  // sum the lexel area of the remaining poly fragments
                  F64 area = nodeStore ? nodeStore->getWindingSurfaceArea() : 0.f;

                  // clamp it
                  if(area > lexelArea)
                     area = lexelArea;

                  // set the scale
                  shadowScale = area / lexelArea;
                  recycleNode(nodeStore);
               }

               // fill in animated light here...
               if(emitterInfo->mLightMap)
               {
                  *(emitterInfo->mLightMap->getAddress(x,y)) = U8(intensity * shadowScale * 255.f);
               }
               else if(shadowScale != 0.f)
               {
                  ColorF col = emitter->mColor;
                  col *= intensity;
                  col *= shadowScale;
                  colSum += col;
                  colSum.clamp();
               }
            }
            
            //   
            recycleNode(lNode);
               
            // set the col
            U8 * pDest = editSurface->pLMap->getAddress(x,y);
               
            // add in the ambient...
            if(!(editSurface->flags & Interior::SurfaceOutsideVisible))
               colSum += mAmbientColor;

            colSum.clamp();

            pDest[0] = U8(colSum.red   * 255.f);
            pDest[1] = U8(colSum.green * 255.f);
            pDest[2] = U8(colSum.blue  * 255.f);

            // set the norm
            normalSum.normalizeSafe();
            editSurface->texSpace.mulV( normalSum );

            U8 * normMap = editSurface->pLightDirMap->getAddress(x,y);
            normMap[0] = 127 + normalSum.x * 128;
            normMap[1] = 127 + normalSum.y * 128;
            normMap[2] = 127 + normalSum.z * 128;

            //
            flushLexelPoints();
            curPos += sVec;
         }

         //
         curPos -= sRun;
         curPos += tVec;
      }
   
//      // filter the outside lexels
//      static F32 filterTable[3][3] = 
//         {{1, 2, 1},
//          {2, 0, 2},
//          {1, 2, 1}};
//
//      for(S32 y = 0; y < editSurface->lMapDimY; y++)
//         for(S32 x = 0; x < editSurface->lMapDimX; x++)
//         {
//            if(!outsideLexels.isSet(x, y))
//               continue;
//
//            F32 rSum = 0.f;
//            F32 gSum = 0.f;
//            F32 bSum = 0.f;
//
//            F32 totalWeight = 0.f;
//
//            for(S32 i = -1; i <= 1; i++)
//               for(S32 j = -1; j <= 1; j++)
//               {
//                  Point2I pos(x+j, y+i);
//
//                  if(pos.x < 0 || pos.x >= editSurface->lMapDimX ||
//                     pos.y < 0 || pos.y >= editSurface->lMapDimY)
//                     continue;
//
//                  //
//                  if(!outsideLexels.isSet(pos.x, pos.y))
//                  {
//                     U8 * pCol = editSurface->pLMap->getAddress(pos.x, pos.y);
//                     F32 weight = filterTable[i+1][j+1];
//
//                     rSum += weight * F32(pCol[0]);
//                     gSum += weight * F32(pCol[1]);
//                     bSum += weight * F32(pCol[2]);
//
//                     totalWeight += weight;
//                  }
//               }
//
//            //
//            if(totalWeight != 0.f)
//            {
//               U8 * pCol = editSurface->pLMap->getAddress(x, y);
//               pCol[0] = U8(rSum / totalWeight);
//               pCol[1] = U8(gSum / totalWeight);
//               pCol[2] = U8(bSum / totalWeight);
//            }
//         }

#ifdef DUMP_LIGHTMAPS
      // 
      static U32 majCnt = 0;
      U32 minCnt = 0;
      for(U32 j = 0; j < surface->mNumEmitters; j++)
      {
         EmitterInfo * emitterInfo = surface->mEmitters[j];

         if(emitterInfo->mLightMap)
         {
            // convert the intensity bitmap to RGB
            GBitmap bitmap(emitterInfo->mLightMap->getWidth(), emitterInfo->mLightMap->getHeight());
            for(U32 y = 0; y < emitterInfo->mLightMap->getHeight(); y++)
               for(U32 x = 0; x < emitterInfo->mLightMap->getWidth(); x++)
               {
                  U8 * pDest = bitmap.getAddress(x, y);
                  U8 src = *emitterInfo->mLightMap->getAddress(x, y);

                  *pDest++ = src;
                  *pDest++ = src;
                  *pDest++ = src;
               }   

            //
            FileStream output;
            output.open(avar("lightmap_%d_%d.png", majCnt, minCnt++), FileStream::Write);
            bitmap.writePNG(output);
         }
      }
      majCnt++;
#endif
   }
}

//------------------------------------------------------------------------------
// Class Lighting::SVNode
//------------------------------------------------------------------------------

Lighting::MiniWinding * Lighting::createWinding()
{
   if(mWindingStore)
   {
      MiniWinding * winding = mWindingStore;
      mWindingStore = mWindingStore->mNext;

      //
      winding->mNumIndices = 0;
      return(winding);
   }

   // create it
   MiniWinding * winding = mWindingChunker.alloc();
   winding->mNumIndices = 0;
   return(winding);   
}

void Lighting::recycleWinding(MiniWinding * winding)
{
   if(!winding)
      return;

   // add to head
   winding->mNext = mWindingStore;
   mWindingStore = winding;
}

void Lighting::recycleNode(SVNode * node)
{
   if(!node)
      return;
      
   recycleNode(node->mFront);
   recycleNode(node->mBack);
   
   // fresh'n it up a bit
   node->mFront = 0;
   node->mTarget = 0;
   node->mEmitterInfo = 0;

   recycleWinding(node->mWinding);
   node->mWinding = 0;

   // add
   node->mBack = mNodeRepository;
   mNodeRepository = node;
}

//-------------------------------------------------------------------

Lighting::SVNode * Lighting::createNode(SVNode::Type type)
{
   // try to get a recycled node first...
   if(mNodeRepository)
   {
      SVNode * node = mNodeRepository;
      mNodeRepository = mNodeRepository->mBack;

      //
      node->mType = type;
      node->mBack = 0;
      node->mWinding = 0;

      // winding..
      if(type == SVNode::PolyPlane || type == SVNode::LexelPlane)
         node->mWinding = createWinding();
      return(node);
   }

   // create the node
   SVNode * node = mNodeChunker.alloc();
   node->mFront = node->mBack = 0;
   node->mTarget = 0;
   node->mEmitterInfo = 0;
   node->mWinding = 0;
   
   // winding..
   if(type == SVNode::PolyPlane || type == SVNode::LexelPlane)
      node->mWinding = createWinding();
   node->mType = type;

   return(node);
}

//------------------------------------------------------------------------------

Lighting::SVNode::Side Lighting::SVNode::whichSide(SVNode * node)
{
   AssertFatal(node, "SVNode::whichSide - NULL node");
   
   bool front = false;
   bool back = false;

   const PlaneEQ & plane = gWorkingGeometry->getPlaneEQ(mPlaneIndex);

   for(U32 i = 0; i < node->mWinding->mNumIndices; i++)
   {
      switch(plane.whichSide(node->mType == LexelPlane ? gWorkingLighting->getLexelPoint(node->mWinding->mIndices[i]) :
         gWorkingGeometry->getPoint(node->mWinding->mIndices[i])))
      {
         case PlaneFront:
            if(back)
               return(Split);
            front = true;
            break;
            
         case PlaneBack:
            if(front)
               return(Split);
            back = true;
            break;
            
         default:
            break;
      }
   }
   
   AssertFatal(!(front && back), "SVNode::whichSide - failed to classify node");

   if(!front && !back)
      return(On);
      
   return(front ? Front : Back);
}

//------------------------------------------------------------------------------

void Lighting::SVNode::addToList(SVNode ** list)
{
   AssertFatal(list, "Lighting::SVNode::addToList - invalid list");
   
   SVNode * head = (*list);
   
   // add at the head...
   (*list) = this;
   mFront = head;
   mBack = 0;
}

//------------------------------------------------------------------------------

void Lighting::SVNode::split(SVNode ** front, SVNode ** back, U32 planeIndex)
{
   AssertFatal(front && !*front, "Lighting::SVNode::splitNode - invalid front param");
   AssertFatal(back && !*back, "Lighting::SVNode::splitNode - invalid back param");
   
   *front = gWorkingLighting->createNode(mType);
   *back = gWorkingLighting->createNode(mType);

   // copy the node
   *(*front)->mWinding = *(*back)->mWinding = *mWinding;
   (*front)->mPlaneIndex = (*back)->mPlaneIndex = mPlaneIndex;
   (*front)->mEmitterInfo = (*back)->mEmitterInfo = mEmitterInfo;
   (*front)->mTarget = (*back)->mTarget = mTarget;

   bool success;
   success = (*front)->clipWindingToPlaneFront(planeIndex);
   AssertFatal(success, "Lighting::SVNode::splitNode - failed to generate front lexel poly");

   success = (*back)->clipWindingToPlaneFront(gWorkingGeometry->getPlaneInverse(planeIndex));
   AssertFatal(success, "Lighting::SVNode::splitNode - failed to generate back lexel poly");
}

//------------------------------------------------------------------------------

void Lighting::SVNode::insertShadowVolume(SVNode ** root)
{
   AssertFatal(root, "Lighting::SVNode::insertShadowVolume- invalid root");
   AssertFatal(!*root, "Lighting::SVNode::insertShadowVolume- root non-null");
   AssertFatal(mType == PolyPlane, "Lighting::SVNode::insertShadowVolume - invalid node type");
   AssertFatal(mEmitterInfo, "Lighting::SVNode::insertShadowVolume - no emitter info");

   SVNode ** current = root;
   SVNode * traverse = gWorkingLighting->getShadowVolume(mEmitterInfo->mShadowVolume);

   AssertFatal(traverse, "Lighting::Emitter::insertShadowVolume - bad shadow volume");

   // walk the shadow planes and add at current root
   while(traverse)
   {
      SVNode * sNode = gWorkingLighting->createNode(ShadowPlane);
      sNode->mPlaneIndex = traverse->mPlaneIndex;
   
      *current = sNode;
      current = &(*current)->mFront;
   
      traverse = traverse->mFront;
   }

   // add the poly node
   *current = this;
}

//------------------------------------------------------------------------------

void Lighting::SVNode::insertFront(SVNode ** root)
{
   AssertFatal(root, "Lighting::SVNode::insertFront - invalid root");
   AssertFatal(mType == PolyPlane, "Lighting::SVNode::insertFront - invalid node type");

   if(!(*root)->mFront)
   {
      AssertFatal((*root)->mType == PolyPlane, "Lighting::SVNode::insertFront - invalid node");
      mTarget = *root;
      insert(&(*root)->mBack);
   }
   else
      insert(&(*root)->mFront);
}

//------------------------------------------------------------------------------

void Lighting::SVNode::insertBack(SVNode ** root)
{
   AssertFatal(root, "Lighting::SVNode::insertBack - invalid root");
   AssertFatal(mType == PolyPlane, "Lighting::SVNode::insertBack - invalid node type");

   if((*root)->mType == PolyPlane)
   {
      mEmitterInfo->mShadowed.pushBackUnique((*root)->mEmitterInfo->mShadowVolume);
      gWorkingLighting->recycleNode(this);
   }
   else
      insert(&(*root)->mBack);
}

//------------------------------------------------------------------------------

void Lighting::SVNode::insert(SVNode ** root)
{
   AssertFatal(root, "Lighting::SVNode::insert - invalid root");
   AssertFatal(mType == PolyPlane, "Lighting::SVNode::insert - invalid node type");

   if(!*root)
   {
      insertShadowVolume(root);
      if(mTarget)
         mTarget->mEmitterInfo->mShadowed.pushBackUnique(mEmitterInfo->mShadowVolume);
   }
   else
   {
      switch((*root)->whichSide(this))
      {
         case Front:
            insertFront(root);
            break;

         case Back:
            insertBack(root);
            break;
         case Split:
         {
            SVNode * front = 0;
            SVNode * back = 0;
            split(&front, &back, (*root)->mPlaneIndex);
            AssertFatal(front && back, "Lighting::SVNode::insert - failed to split node");

            gWorkingLighting->recycleNode(this);
            
            // push down both sides            
            front->insertFront(root);
            back->insertBack(root);
            break;
         }
         default:
         	// sliver poly encountered (just remove for now..)
            gWorkingLighting->mNumAmbiguousPlanes++;
            gWorkingLighting->recycleNode(this);
            //AssertFatal(false, "Lighting::SVNode::insert - invalid classification!");
            break;
      }
   }
}

//------------------------------------------------------------------------------
// * this functino is used to clip a lexel node to it's own surface
void Lighting::SVNode::clipToInverseVolume(SVNode ** store, SVNode * volume)
{
   AssertFatal(store, "Lighting::SVNode::clipToInverseVolume: invalid store param");
   AssertFatal(mType == LexelPlane, "Lighting::SVNode::clipToInverseVolume: node must be of lexelPlane");

   if(!volume)
   {
      addToList(store);
      return;
   }   

   AssertFatal(volume->mType == ShadowPlane, "Lighting::SVNode::clipToInverseVolume: invalid node in volume");
   
   switch(volume->whichSide(this))
   {
      case Front:
         clipToInverseVolume(store, volume->mFront);
         break;
      case Back:
      {
         mFront = mBack = 0;
         gWorkingLighting->recycleNode(this);
         break;
      }
      case Split:
      {
         SVNode * front = 0;
         SVNode * back = 0;
         split(&front, &back, volume->mPlaneIndex);
         AssertFatal(front && back, "Lighting::SVNode::clipToInverseVolume - invalid split!");

         mFront = mBack = 0;
         gWorkingLighting->recycleNode(this);
   
         back->mFront = back->mBack = 0;
         gWorkingLighting->recycleNode(back);

         front->clipToInverseVolume(store, volume->mFront);
         break;
      }
      case On:
      {
         // sliver poly:
         mFront = mBack = 0;
         gWorkingLighting->recycleNode(this);
         break;
      }
   }
}

void Lighting::SVNode::clipToVolume(SVNode ** store, SVNode * volume)
{
   AssertFatal(store, "Lighting::SVNode::clipToVolume - invalid store param");
   AssertFatal(mType == LexelPlane, "Lighting::SVNode::clipToVolume - node must be a lexelPlane");

   if(!volume)
   {
      mFront = mBack = 0;
      gWorkingLighting->recycleNode(this);
      return;
   }
   
   AssertFatal(volume->mType == ShadowPlane, "Lighting::SVNode::clipToVolume - volume must be shadow planes");
   
   switch(volume->whichSide(this))
   {
      case Back:
         addToList(store);
         break;
      case Front:
         clipToVolume(store, volume->mFront);
         break;
      case Split:
      {
         SVNode * front = 0;
         SVNode * back = 0;
         split(&front, &back, volume->mPlaneIndex);
         AssertFatal(front && back, "Lighting::SVNode::clipToVolume - invalid split!");

         mFront = mBack = 0;
         gWorkingLighting->recycleNode(this);
         
         back->addToList(store);
         front->clipToVolume(store, volume->mFront);
         break;
      }
      case On:
      {
         // sliver poly
         mFront = mBack = 0;
         gWorkingLighting->recycleNode(this);
         break;
      }
   }
}

//------------------------------------------------------------------------------

Point3D Lighting::SVNode::getCenter()
{
   AssertFatal(mWinding, "Lighting::SVNode::getCenter - no winding");
   AssertFatal(mWinding->mNumIndices >= 3, "Lighting::SVNode::getCenter - invalid node");
   
   Point3D pnt(0,0,0);
   for(U32 i = 0; i < mWinding->mNumIndices; i++)
      pnt += gWorkingLighting->getLexelPoint(mWinding->mIndices[i]);
   pnt /= mWinding->mNumIndices;

   return(pnt);
}

//------------------------------------------------------------------------------

void Lighting::SVNode::move(SVNode ** list)
{
   AssertFatal(list, "Lighting::SVNode::moveNodes - invalid list param");
   
   SVNode * node = this;
   while(node)
   {
      AssertFatal(node->mType == LexelPlane, "Lighting::SVNode::moveNodes - invalid node type");
      SVNode * next = node->mFront;
      node->addToList(list);
      node = next;
   }
}

//------------------------------------------------------------------------------

F64 Lighting::SVNode::getWindingSurfaceArea()
{
   Point3D areaNormal(0, 0, 0);
   for (U32 i = 0; i < mWinding->mNumIndices; i++) {
      U32 j = (i + 1) % mWinding->mNumIndices;

      Point3D temp;
      mCross(getPoint(mWinding->mIndices[i]), getPoint(mWinding->mIndices[j]), &temp);
      areaNormal += temp;
   }

   F64 area = mDot(gWorkingGeometry->getPlaneEQ(mPlaneIndex).normal, areaNormal);
   if (area < 0.0)
      area *= -0.5;
   else
      area *= 0.5;

   //
   if(mFront)
      area += mFront->getWindingSurfaceArea();

   return(area);
}

//------------------------------------------------------------------------------

const Point3D & Lighting::SVNode::getPoint(U32 index)
{
   if(mType == LexelPlane)
      return(gWorkingLighting->getLexelPoint(index));
   return(gWorkingGeometry->getPoint(index));
}

U32 Lighting::SVNode::insertPoint(const Point3D & pnt)
{
   if(mType == LexelPlane)
      return(gWorkingLighting->insertLexelPoint(pnt));
   else
      return(gWorkingGeometry->insertPoint(pnt));
}

bool Lighting::SVNode::clipWindingToPlaneFront(U32 planeEQIndex)
{
   const PlaneEQ& rClipPlane = gWorkingGeometry->getPlaneEQ(planeEQIndex);
   
   U32 start;
   for (start = 0; start < mWinding->mNumIndices; start++) {
      const Point3D& rPoint = getPoint(mWinding->mIndices[start]);
      if (rClipPlane.whichSide(rPoint) == PlaneFront)
         break;
   }

   if (start == mWinding->mNumIndices) {
      mWinding->mNumIndices = 0;
      return true;
   }

   U32 finalIndices[LightingMaxWindingPoints];
   U32 numFinalIndices = 0;

   U32 baseStart = start;
   U32 end       = (start + 1) % mWinding->mNumIndices;

   bool modified = false;
   while (end != baseStart) {
      const Point3D& rStartPoint = getPoint(mWinding->mIndices[start]);
      const Point3D& rEndPoint   = getPoint(mWinding->mIndices[end]);

      PlaneSide fSide = rClipPlane.whichSide(rStartPoint);
      PlaneSide eSide = rClipPlane.whichSide(rEndPoint);

      S32 code = fSide * 3 + eSide;
      switch (code) {
        case 4:   // f f
        case 3:   // f o
        case 1:   // o f
        case 0:   // o o
         // No Clipping required
         finalIndices[numFinalIndices++] = mWinding->mIndices[start];
         start = end;
         end   = (end + 1) % mWinding->mNumIndices;
         break;


        case 2: { // f b
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...
            //
            finalIndices[numFinalIndices++] = mWinding->mIndices[start];

            Point3D vector = rEndPoint - rStartPoint;
            F64 t = -(rClipPlane.distanceToPlane(rStartPoint) / mDot(rClipPlane.normal, vector));
            
            Point3D intersection = rStartPoint + (vector * t);
            AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                        "Lighting::SVNode::clipWindingToPlaneFront: error in computing intersection");
            finalIndices[numFinalIndices++] = insertPoint(intersection);

            U32 endSeek = (end + 1) % mWinding->mNumIndices;
            while (rClipPlane.whichSide(getPoint(mWinding->mIndices[endSeek])) == PlaneBack)
               endSeek = (endSeek + 1) % mWinding->mNumIndices;

            PlaneSide esSide = rClipPlane.whichSide(getPoint(mWinding->mIndices[endSeek]));
            if(esSide == PlaneFront) {
               end   = endSeek;
               start = (end + (mWinding->mNumIndices - 1)) % mWinding->mNumIndices;

               const Point3D& rNewStartPoint = getPoint(mWinding->mIndices[start]);
               const Point3D& rNewEndPoint   = getPoint(mWinding->mIndices[end]);

               vector = rNewEndPoint - rNewStartPoint;
               t = -(rClipPlane.distanceToPlane(rNewStartPoint) / mDot(rClipPlane.normal, vector));
            
               intersection = rNewStartPoint + (vector * t);
               AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                           "Lighting::SVNode::clipWindingToPlaneFront: error in computing intersection");
            
               mWinding->mIndices[start] = insertPoint(intersection);
               AssertFatal(mWinding->mIndices[start] != mWinding->mIndices[end], "Error");
            } else {
               start = endSeek;
               end = (endSeek + 1) % mWinding->mNumIndices;
            }
            modified = true;
         }
         break;

        case -1: {// o b
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //
            finalIndices[numFinalIndices++] = mWinding->mIndices[start];

            U32 endSeek = (end + 1) % mWinding->mNumIndices;
            while (rClipPlane.whichSide(getPoint(mWinding->mIndices[endSeek])) == PlaneBack)
               endSeek = (endSeek + 1) % mWinding->mNumIndices;

            PlaneSide esSide = rClipPlane.whichSide(getPoint(mWinding->mIndices[endSeek]));
            if(esSide == PlaneFront) {
            
               end   = endSeek;
               start = (end + (mWinding->mNumIndices - 1)) % mWinding->mNumIndices;

               const Point3D& rNewStartPoint = getPoint(mWinding->mIndices[start]);
               const Point3D& rNewEndPoint   = getPoint(mWinding->mIndices[end]);

               Point3D vector = rNewEndPoint - rNewStartPoint;
               F64  t      = -(rClipPlane.distanceToPlane(rNewStartPoint) / mDot(rClipPlane.normal, vector));
            
               Point3D intersection = rNewStartPoint + (vector * t);
               AssertFatal(rClipPlane.whichSide(intersection) == PlaneOn,
                           "Lighting::SVNode::clipWindingToPlaneFront: error in computing intersection");
            
               mWinding->mIndices[start] = insertPoint(intersection);
               AssertFatal(mWinding->mIndices[start] != mWinding->mIndices[end], "Error");
            } else {
               start = endSeek;
               end   = (endSeek + 1) % mWinding->mNumIndices;
            }
            modified = true;
         }
         break;

        case -2:  // b f
        case -3:  // b o
        case -4:  // b b
         // In the algorithm used here, this should never happen...
         AssertISV(false, "Lighting::SVNode::clipWindingToPlaneFront: error in polygon clipper");
         break;

        default:
         AssertFatal(false, "Lighting::SVNode::clipWindingToPlaneFront: bad outcode");
         break;
      }

   }
   
   // Emit the last point.
   AssertFatal(rClipPlane.whichSide(getPoint(mWinding->mIndices[start])) != PlaneBack,
               "Lighting::SVNode::clipWindingToPlaneFront: bad final point in clipper");
   finalIndices[numFinalIndices++] = mWinding->mIndices[start];
   AssertFatal(numFinalIndices >= 3, "Error, line out of clip!");
   
   // Copy the new rWinding, and we're set!
   //
   dMemcpy(mWinding->mIndices, finalIndices, numFinalIndices * sizeof(U32));
   mWinding->mNumIndices = numFinalIndices;

   AssertISV(mWinding->mNumIndices <= LightingMaxWindingPoints,
             avar("Increase lightingMaxWindingPoints.  Talk to JohnF(%d, %d)",
                  LightingMaxWindingPoints, numFinalIndices));

#if defined(TORQUE_DEBUG)
   // valid point indices?
   for(U32 i = 0; i < mWinding->mNumIndices; i++)
   {
      U32 size = mType == LexelPlane ? gWorkingLighting->mLexelPoints.size() : 
         gWorkingGeometry->mPoints.size();
      AssertFatal(mWinding->mIndices[i] < size, "Lighting::SVNode::clipWindingToPlaneFront: invalid point index");
   }
#endif

   return(modified);
}

//------------------------------------------------------------------------------
// Class Lighting::Surface
//------------------------------------------------------------------------------

Lighting::Surface * Lighting::createSurface()
{
   Surface * surface = mSurfaceChunker.alloc();

   // clear it out (will not call vector ctor's)
   dMemset(surface, 0, sizeof(Surface));
   return(surface);
}

//------------------------------------------------------------------------------

Lighting::EmitterInfo * Lighting::Surface::getEmitterInfo(U32 emitter)
{
   for(U32 i = 0; i < mNumEmitters; i++)
      if(mEmitters[i]->mEmitter == emitter)
         return(mEmitters[i]);
      
   AssertFatal(false, "Lighting::Surface::getEmitterInfo - failed to get emitter info");
   return NULL;
}

//------------------------------------------------------------------------------
// Class Lighting::Emitter
//------------------------------------------------------------------------------

Lighting::Emitter::Emitter()
{
   mIndex = 0;
   mNumSurfaces = 0;
   mSurfaces = 0;
   mAnimated = false;
   mBumpSpec = false;
}

void Lighting::Emitter::processBSP()
{
   SVNode * root = 0;

   // the polynode gets trashed, so insert a copy
   for(U32 i = 0; i < mNumSurfaces; i++)
   {
      Surface * surface = gWorkingLighting->getSurface(mSurfaces[i]);
      AssertFatal(surface && surface->mPolyNode, "Lighting::Emitter::processBSP - invalid surface");
   
      SVNode * pNode = gWorkingLighting->createNode(Lighting::SVNode::PolyPlane);
      *pNode->mWinding = *surface->mPolyNode->mWinding;
      pNode->mPlaneIndex = surface->mPolyNode->mPlaneIndex;
      pNode->mEmitterInfo = surface->getEmitterInfo(mIndex);

      pNode->insert(&root);
   }
      
   gWorkingLighting->recycleNode(root);
}

//------------------------------------------------------------------------------
// Class Lighting::PointEmitter
//------------------------------------------------------------------------------
bool Lighting::PointEmitter::isSurfaceLit(const Surface * surface)
{
   AssertFatal(surface, "Lighting::PointEmitter::isSurfaceLit - NULL surface");
   const PlaneEQ & plane = gWorkingGeometry->getPlaneEQ(surface->mPlaneIndex);

   if(plane.whichSide(mPos) != PlaneFront)
      return(false);
      
   // can light something on the surface?
   F64 distance = plane.distanceToPlane(mPos);
   if(distance > mFalloffs[1])
      return(false);

   // check if the point is inside the poly..
   Point3D pnt = mPos - (plane.normal * distance);
   const MiniWinding & winding = surface->mWinding;

   // inside this surface?
   bool inside = true;
   for(U32 i = 0; inside && (i < winding.mNumIndices); i++)
   {
      U32 j = (i+1) % winding.mNumIndices;
      Point3D vec1 = gWorkingGeometry->getPoint(winding.mIndices[j]) - gWorkingGeometry->getPoint(winding.mIndices[i]);
      Point3D vec2 = pnt - gWorkingGeometry->getPoint(winding.mIndices[i]);

      Point3D cross;
      mCross(vec1, vec2, &cross);

      if(mDot(plane.normal, cross) > 0.f)
         inside = false;
   }
   
   if(inside)
      return(true);

   // check if any of the line segments intersect the light sphere
   for(U32 i = 0; i < winding.mNumIndices; i++)
   {
      // substitute the line equation into the sphere and 
      // solve the quadratic
      U32 j = (i+1) % winding.mNumIndices;
      Point3D p1 = gWorkingGeometry->getPoint(winding.mIndices[i]);
      Point3D p2 = gWorkingGeometry->getPoint(winding.mIndices[j]);
      Point3D & p3 = mPos;
      F64 r = mFalloffs[1];
      
      // get the quadratic terms
      F64 a = (p2.x - p1.x) * (p2.x - p1.x) + 
              (p2.y - p1.y) * (p2.y - p1.y) +
              (p2.z - p1.z) * (p2.z - p1.z);
              
      F64 b = 2 * ((p2.x - p1.x) * (p1.x - p3.x) +
                   (p2.y - p1.y) * (p1.y - p3.y) +
                   (p2.z - p1.z) * (p1.z - p3.z));

      F64 c = p3.x * p3.x + p3.y * p3.y + p3.z * p3.z +
              p1.x * p1.x + p1.y * p1.y + p1.z * p1.z -
              2 * (p3.x * p1.x + p3.y * p1.y + p3.z * p1.z) - r * r;
      
      // if the expression within the square root is:
      // <0: the line does not intersect the sphere
      // 0: the line is tangential
      // >0: intersects at two points
      F64 d = (b * b - 4 * a * c);
      
      if(d > 0.f)
      {
         // get the times of intersection...
         // if either time in 0>1 then intersected, otherwise
         // if there is a t<0 && t>1 then contained inside the sphere
         d = mSqrtD(d);
         F64 t1 = (-b + d) / (2 * a);
         if(t1 > 0.f && t1 < 1.f)
            return(true);
            
         F64 t2 = (-b - d) / (2 * a);
         if(t2 > 0.f && t2 < 1.f)
            return(true);
            
         // contained?
         if(t1 * t2 < 0.f)
            return(true);
      }
   }
   
   return(false);
}

//------------------------------------------------------------------------------

F64 Lighting::PointEmitter::calcIntensity(SVNode * lSurface)
{
   AssertFatal(lSurface, "Lighting::PointEmitter::calcIntensity - NULL surface");
   AssertFatal(lSurface->mType == SVNode::LexelPlane, "Lighting::PointEmitter::calcIntensity - invalid node passed");
   
   Point3D center = lSurface->getCenter();
   F64 len = Point3D(mPos - center).len();

   if(len > mFalloffs[1])
      return(0.f);

   if(len > mFalloffs[0])
      return(1.f - (len - F64(mFalloffs[0])) / F64((mFalloffs[1]-mFalloffs[0])));

   return(1);
}

//------------------------------------------------------------------------------
// calculates the intensity at the centroid of the node


F64 Lighting::RadEmitter::calcIntensity(const Point3D & pnt, const Point3D &surfNormal)
{
   // calc falloff
   F64 distFactor = Point3D(mPos - pnt).len();
   distFactor = mSqrt( distFactor );


   Point3D dir = pnt - mPos;
   dir.normalize();

   F64 ang1 = mDot( dir, mDirection );
   if( ang1 < 0.0 )
   {
      return 0.0;
   }

   F64 ang2 = mDot( -dir, surfNormal );

   F64 intensity = ang1 * ang2;
   if( intensity < 0.0 )
   {
      intensity = 0.0;
   }

   F64 geomScale = gWorkingGeometry->mWorldEntity->mGeometryScale;

   return intensity * mIntensity * geomScale / distFactor;
}

//------------------------------------------------------------------------------
// Class Lighting::SpotEmitter
//------------------------------------------------------------------------------

bool Lighting::SpotEmitter::isSurfaceLit(const Surface * surface)
{
   AssertFatal(surface, "Lighting::SpotEmitter::isSurfaceLit - NULL surface");
   const PlaneEQ & plane = gWorkingGeometry->getPlaneEQ(surface->mPlaneIndex);

   if(plane.whichSide(mPos) != PlaneFront)
      return(false);
      
   // can light something on the surface?
   F64 distance = plane.distanceToPlane(mPos);
   if(distance > mFalloffs[1])
      return(false);

   // check if the point is inside the poly..
   Point3D pnt = mPos - (plane.normal * distance);
   const MiniWinding & winding = surface->mWinding;

   // inside this surface?
   bool inside = true;
   for(U32 i = 0; inside && (i < winding.mNumIndices); i++)
   {
      U32 j = (i+1) % winding.mNumIndices;
      Point3D vec1 = gWorkingGeometry->getPoint(winding.mIndices[j]) - gWorkingGeometry->getPoint(winding.mIndices[i]);
      Point3D vec2 = pnt - gWorkingGeometry->getPoint(winding.mIndices[i]);

      Point3D cross;
      mCross(vec1, vec2, &cross);

      if(mDot(plane.normal, cross) > 0.f)
         inside = false;
   }
   
   if(inside)
      return(true);

   // check if any of the line segments intersect the light sphere
   for(U32 i = 0; i < winding.mNumIndices; i++)
   {
      // substitute the line equation into the sphere and 
      // solve the quadratic
      U32 j = (i+1) % winding.mNumIndices;
      Point3D p1 = gWorkingGeometry->getPoint(winding.mIndices[i]);
      Point3D p2 = gWorkingGeometry->getPoint(winding.mIndices[j]);
      Point3D & p3 = mPos;
      F64 r = mFalloffs[1];
      
      // get the quadratic terms
      F64 a = (p2.x - p1.x) * (p2.x - p1.x) + 
              (p2.y - p1.y) * (p2.y - p1.y) +
              (p2.z - p1.z) * (p2.z - p1.z);
              
      F64 b = 2 * ((p2.x - p1.x) * (p1.x - p3.x) +
                   (p2.y - p1.y) * (p1.y - p3.y) +
                   (p2.z - p1.z) * (p1.z - p3.z));

      F64 c = p3.x * p3.x + p3.y * p3.y + p3.z * p3.z +
              p1.x * p1.x + p1.y * p1.y + p1.z * p1.z -
              2 * (p3.x * p1.x + p3.y * p1.y + p3.z * p1.z) - r * r;
      
      // if the expression within the square root is:
      // <0: the line does not intersect the sphere
      // 0: the line is tangential
      // >0: intersects at two points
      F64 d = (b * b - 4 * a * c);
      
      if(d > 0.f)
      {
         // get the times of intersection...
         // if either time in 0>1 then intersected, otherwise
         // if there is a t<0 && t>1 then contained inside the sphere
         d = mSqrtD(d);
         F64 t1 = (-b + d) / (2 * a);
         if(t1 > 0.f && t1 < 1.f)
            return(true);
            
         F64 t2 = (-b - d) / (2 * a);
         if(t2 > 0.f && t2 < 1.f)
            return(true);
            
         // contained?
         if(t1 * t2 < 0.f)
            return(true);
      }
   }
   
   return(false);
}

//------------------------------------------------------------------------------

F64 Lighting::SpotEmitter::calcIntensity(SVNode * lSurface)
{
   AssertFatal(lSurface, "Lighting::SpotEmitter::calcIntensity - NULL surface");
   AssertFatal(lSurface->mType == SVNode::LexelPlane, "Lighting::SpotEmitter::calcIntensity - invalid node passed");

   Point3D center = lSurface->getCenter();
   F64 len = Point3D(center - mPos).len();

   if(len > mFalloffs[1])
      return(0.f);

   //
   F64 intensity = (len > mFalloffs[0]) ? 1 - ((len - F64(mFalloffs[0])) / F64((mFalloffs[1]-mFalloffs[0]))) : 1;
   
   Point3D vec = Point3D(center - mPos);
   vec.normalize();

   F64 angle = mAcos(mDot(mDirection, vec));

   if(angle <= mAngles[0])
      return(intensity);

   if(angle > mAngles[1])
      return(0.f);

   return(intensity * (1 - ((angle - mAngles[0]) / (mAngles[1] - mAngles[0]))));
}

//------------------------------------------------------------------------------
// Class Lighting::Light
//------------------------------------------------------------------------------

Lighting::Light::Light()
{
   mNumStates = 0;
   mStateIndex = 0;
   mAnimated = false;
   mAnimType = 0;
   mName = 0;
}

Lighting::Light::~Light()
{
   if(!mAnimated)
      delete [] mName;
   mName = NULL;
}

//------------------------------------------------------------------------------

bool Lighting::Light::getTargetPosition(const char * name, Point3D & pos)
{
   TargetEntity * target = 0;   
   for(U32 i = 0; !target && i < gWorkingLighting->mTargets.size(); i++)
      if(!dStricmp(gWorkingLighting->mTargets[i]->mTargetName, name))
         target = gWorkingLighting->mTargets[i];

   //
   if(!target)
   {
      dPrintf(" ! Target entity not found: '%s'", name); dFflushStdout();
      return(false);
   }

   pos = target->mOrigin;
   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildLight - NULL entity");

   LightEntity * light = static_cast<LightEntity*>(entity);

   AssertFatal(light->mLightName[0], "Lighting::Light::buildLight - unnamed light encountered.");
   AssertFatal(light->mNumStates, avar("Lighting::Light::buildLight - light '%s' has no states.", light->mLightName));

   mName = newStrDup(light->mLightName);
   mNumStates = light->mNumStates;
   mStateIndex = gWorkingLighting->mLightStates.size();

   // assume the light to be animated if there is more than one state
   if(mNumStates > 1)
   {
      // animated stuff...
      mAnimType = light->mFlags;
      mAnimated = true;
   }

   // grab the emitters for this light
   Vector<BaseLightEmitterEntity *> myEmitters;
   for(U32 i = 0; i < gWorkingLighting->mBaseLightEmitters.size(); i++)
      if(gWorkingLighting->mBaseLightEmitters[i]->mTargetLight[0] && 
         !dStricmp(gWorkingLighting->mBaseLightEmitters[i]->mTargetLight, mName))
         myEmitters.push_back(gWorkingLighting->mBaseLightEmitters[i]);

   // process the states....
   for(U32 i = 0; i < mNumStates; i++)
   {
      LightState state;
      state.mNumEmitters = 0;
      state.mEmitterIndex = gWorkingLighting->mLightStateEmitterStore.size();
      state.mDuration = light->mStates[i].mDuration;

      // grab them emitters
      for(U32 j = 0; j < myEmitters.size(); j++)
      {
         if(myEmitters[j]->mStateIndex != i)
            continue;

         // process the entity
         if(dynamic_cast<PointEmitterEntity*>(myEmitters[j]))
         {
            PointEmitterEntity * pointEntity = (PointEmitterEntity*)myEmitters[j];

            // add a point emitter
            PointEmitter * emitter = new PointEmitter;
            emitter->mPos = light->mOrigin;
            emitter->mColor = light->mStates[i].mColor;
            emitter->mPoint = gWorkingGeometry->insertPoint(pointEntity->mOrigin);
            emitter->mIndex = state.mEmitterIndex + state.mNumEmitters++;
            emitter->mAnimated = mAnimated;

            emitter->mFalloffs[0] = pointEntity->mFalloff1;
            emitter->mFalloffs[1] = pointEntity->mFalloff2;

            state.mNumEmitters++;
         }
         else if(dynamic_cast<SpotEmitterEntity*>(myEmitters[j]))
         {
            SpotEmitterEntity * spotEntity = (SpotEmitterEntity*)myEmitters[j];

            // add the spot emitter
            SpotEmitter * emitter = new SpotEmitter;

            emitter->mPos = light->mOrigin;
            emitter->mColor = light->mStates[i].mColor;
            emitter->mPoint = gWorkingGeometry->insertPoint(spotEntity->mOrigin);
            emitter->mIndex = state.mEmitterIndex + state.mNumEmitters++;
            emitter->mAnimated = mAnimated;

            emitter->mFalloffs[0] = spotEntity->mFalloff1;
            emitter->mFalloffs[1] = spotEntity->mFalloff2;

            emitter->mDirection = spotEntity->mDirection;
            emitter->mAngles[0] = spotEntity->mInnerAngle;
            emitter->mAngles[1] = spotEntity->mOuterAngle;

            state.mNumEmitters++;
         }
         else 
            AssertFatal(false, "Lighting::Light::buildLight - invalid entity encountered.");
      }

      gWorkingLighting->mLightStates.push_back(state);
   }
 
   return(true);
}

//------------------------------------------------------------------------------
// All the scripted lights:
//------------------------------------------------------------------------------

bool Lighting::Light::buildOmniLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildOmniLight - NULL entity");

   LightOmniEntity * omniLight = static_cast<LightOmniEntity*>(entity);
   AssertFatal(omniLight, "Lighting::Light::buildOmniLight - invalid entity");
   
   // fill the light, state and then the emittter info   
   mNumStates = 1;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mName = newStrDup(omniLight->mLightName);

   Light::LightState state;
   state.mNumEmitters = 1;
   state.mEmitterIndex = gWorkingLighting->mEmitters.size();
   gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

   PointEmitter * emitter = new PointEmitter;
   emitter->mPos = omniLight->mOrigin;
   emitter->mColor = omniLight->mColor;
   emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
   emitter->mIndex = state.mEmitterIndex;
   emitter->mBumpSpec = omniLight->mBumpSpec;

   emitter->mFalloffs[0] = omniLight->mFalloff1;
   emitter->mFalloffs[1] = omniLight->mFalloff2;

   gWorkingLighting->mEmitters.push_back(emitter);
   gWorkingLighting->mLightStates.push_back(state);
   
   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildSpotLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildSpotLight - NULL entity");

   LightSpotEntity * spotLight = static_cast<LightSpotEntity*>(entity);
   AssertFatal(spotLight, "Lighting::Light::buildSpotLight - invalid entity");

   //
   Point3D target;
   if(!getTargetPosition(spotLight->mTarget, target))
      return(false);

   // fill the light, state and then the emittter info   
   mNumStates = 1;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mName = newStrDup(spotLight->mLightName);

   Light::LightState state;
   state.mNumEmitters = 1;
   state.mDuration = 0.f;
   state.mEmitterIndex = gWorkingLighting->mEmitters.size();
   gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

   SpotEmitter * emitter = new SpotEmitter;
   emitter->mPos = spotLight->mOrigin;
   emitter->mColor = spotLight->mColor;

   // calculate the direction and angles...
   emitter->mDirection = target - spotLight->mOrigin;
   F32 len = emitter->mDirection.len();

   //
   emitter->mDirection.normalize();
   emitter->mAngles[0] = mAtan(spotLight->mInnerDistance, len);
   emitter->mAngles[1] = mAtan(spotLight->mOuterDistance, len);

   emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
   emitter->mIndex = state.mEmitterIndex;

   emitter->mFalloffs[0] = spotLight->mFalloff1;
   emitter->mFalloffs[1] = spotLight->mFalloff2;

   gWorkingLighting->mEmitters.push_back(emitter);
   gWorkingLighting->mLightStates.push_back(state);
   
   return(true);
}

bool Lighting::Light::buildBrushLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildBrushLight - NULL entity");

   LightBrushEntity * brushLight = static_cast<LightBrushEntity*>(entity);
   AssertFatal(brushLight, "Lighting::Light::buildBrushLight - invalid entity");


   brushLight->mBrush.selfClip();

   if( gWorkingGeometry->mWorldEntity->mBrushLightIsPoint )
   {
      for( U32 planeIndex=0; planeIndex < brushLight->mBrush.mPlanes.size(); planeIndex++ )
      {
         CSGPlane &plane = brushLight->mBrush.mPlanes[planeIndex];

         // only emit from surfaces that have emitter texture
         if( !dStrstr( plane.pTextureName, "EMITTER" ) )
         {
            continue;
         }


         PlaneEQ planeEQ = gWorkingGeometry->getPlaneEQ( plane.planeEQIndex );
         Point3D planeEQNormal = planeEQ.normal;


         Point3D center = (brushLight->mBrush.mMaxBound + brushLight->mBrush.mMinBound) * 0.5;

         Light::LightState state;
         state.mNumEmitters = 1;
         state.mEmitterIndex = gWorkingLighting->mEmitters.size();
         gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);


         RadEmitter *emitter = new RadEmitter;
         emitter->mFalloffs[1] = brushLight->mRadius;
         emitter->mPos = center;
         emitter->mColor = brushLight->mColor;
         emitter->mDirection = planeEQNormal;
         emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
         emitter->mIntensity = brushLight->mIntensity;
         emitter->mIndex = state.mEmitterIndex;
         emitter->mBumpSpec = brushLight->mBumpSpec;

         gWorkingLighting->mEmitters.push_back(emitter);
         gWorkingLighting->mLightStates.push_back(state);

      }

   }
   else
   {
      for( U32 planeIndex=0; planeIndex < brushLight->mBrush.mPlanes.size(); planeIndex++ )
      {
         CSGPlane &plane = brushLight->mBrush.mPlanes[planeIndex];

         // only emit from surfaces that have emitter texture
         if( dStrcmp( plane.pTextureName, "EMITTER" ) )
         {
            continue;
         }


         PlaneEQ planeEQ = gWorkingGeometry->getPlaneEQ( plane.planeEQIndex );
         Point3D planeEQNormal = planeEQ.normal;


         // get the two plane dirs
         Point3D s, t;

         t = gWorkingGeometry->getPoint( plane.winding.indices[0] );
         t -= gWorkingGeometry->getPoint( plane.winding.indices[1] );
         s = gWorkingGeometry->getPoint( plane.winding.indices[2] );
         s -= gWorkingGeometry->getPoint( plane.winding.indices[1] );

         F32 lumelScale = gWorkingGeometry->mWorldEntity->mInsideLumelScale;
         U32 tIterations = getMax( t.len() / lumelScale, (F64) 1 );
         U32 sIterations = getMax( s.len() / lumelScale, (F64) 1 );

         Point3D sLightRun = s;
         sLightRun.normalize();
         sLightRun *= sIterations * lumelScale;

         t.normalize();
         t *= lumelScale;

         s.normalize();
         s *= lumelScale;


         Point3D lightPoint = gWorkingGeometry->getPoint( plane.winding.indices[1] );

         F64 localIntensity = 0.0;

         // loop over the plane matrix and soak in the emitter surface
         for( U32 a=0; a < tIterations; a++ )
         {
            for( U32 b=0; b < sIterations; b++ )
            {

               Light::LightState state;
               state.mNumEmitters = 1;
               state.mEmitterIndex = gWorkingLighting->mEmitters.size();
               gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);


               RadEmitter *emitter = new RadEmitter;
               emitter->mFalloffs[1] = brushLight->mRadius;
               emitter->mPos = lightPoint;
               emitter->mColor = brushLight->mColor;
               emitter->mDirection = planeEQNormal;
               emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
               emitter->mIntensity = brushLight->mIntensity / (sIterations * tIterations);
               emitter->mIndex = state.mEmitterIndex;
               emitter->mBumpSpec = brushLight->mBumpSpec;

               gWorkingLighting->mEmitters.push_back(emitter);

               gWorkingLighting->mLightStates.push_back(state);


               lightPoint += s;
            }
            lightPoint -= sLightRun;
            lightPoint += t;
         }
      }
   }

   return false;

}

// Animated script lights: -----------------------------------------------------

bool Lighting::Light::buildStrobeLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildStrobeLight - NULL entity");

   LightStrobeEntity * strobeLight = static_cast<LightStrobeEntity*>(entity);
   AssertFatal(strobeLight, "Lighting::Light::buildStrobeLight - invalid entity");

   if(strobeLight->mSpeed > LightStrobeEntity::VeryFast)
   {
      dPrintf(" ! Invalid speed for strobe light: %s", strobeLight->mLightName);
      return(false);
   }

   // total of 4 states
   mNumStates = 4;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mAnimated = true;  
   mAnimType = (strobeLight->mFlags & ~Interior::AnimationFlicker) | Interior::AnimationLoop;
   mName = newStrDup(strobeLight->mLightName);

   for(U32 i = 0; i < 4; i++)
   {
      // create this state
      Light::LightState state;
      state.mNumEmitters = 1;
      state.mEmitterIndex = gWorkingLighting->mEmitters.size();
      gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

      // create the emitter
      PointEmitter * emitter = new PointEmitter;
      emitter->mPos = strobeLight->mOrigin;
      emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
      emitter->mIndex = state.mEmitterIndex;
      emitter->mFalloffs[0] = strobeLight->mFalloff1;
      emitter->mFalloffs[1] = strobeLight->mFalloff2;
      emitter->mAnimated = true;

      state.mDuration = (i & 0x01) ? 0.f : AnimationTimes[strobeLight->mSpeed];
      emitter->mColor = (i & 0x02) ? strobeLight->mColor2 : strobeLight->mColor1;

      gWorkingLighting->mEmitters.push_back(emitter);
      gWorkingLighting->mLightStates.push_back(state);
   }   

   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildPulseLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildPulseLight - NULL entity");

   LightPulseEntity * pulseLight = static_cast<LightPulseEntity*>(entity);
   AssertFatal(pulseLight, "Lighting::Light::buildPulseLight - invalid entity");

   if(pulseLight->mSpeed > LightPulseEntity::VeryFast)
   {
      dPrintf(" ! Invalid speed for pulse light: %s", pulseLight->mLightName);
      return(false);
   }

   // total of 2 states
   mNumStates = 2;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mAnimated = true;
   mAnimType = (pulseLight->mFlags & ~Interior::AnimationFlicker) | Interior::AnimationLoop;
   mName = newStrDup(pulseLight->mLightName);

   for(U32 i = 0; i < 2; i++)
   {
      // create this state
      Light::LightState state;
      state.mNumEmitters = 1;
      state.mEmitterIndex = gWorkingLighting->mEmitters.size();
      gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

      // create the emitter
      PointEmitter * emitter = new PointEmitter;
      emitter->mPos = pulseLight->mOrigin;
      emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
      emitter->mIndex = state.mEmitterIndex;
      emitter->mFalloffs[0] = pulseLight->mFalloff1;
      emitter->mFalloffs[1] = pulseLight->mFalloff2;
      emitter->mAnimated = true;

      // color and duration
      state.mDuration = AnimationTimes[pulseLight->mSpeed];
      emitter->mColor = i ? pulseLight->mColor2 : pulseLight->mColor1;

      gWorkingLighting->mEmitters.push_back(emitter);
      gWorkingLighting->mLightStates.push_back(state);
   }   

   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildPulse2Light(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildPulse2Light - NULL entity");

   LightPulse2Entity * pulseLight = static_cast<LightPulse2Entity*>(entity);
   AssertFatal(pulseLight, "Lighting::Light::buildPulse2Light - invalid entity");

   // total of 4 states
   mNumStates = 4;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mAnimated = true;
   mAnimType = (pulseLight->mFlags & ~Interior::AnimationFlicker) | Interior::AnimationLoop;
   mName = newStrDup(pulseLight->mLightName);

   F32 durations[] = { pulseLight->mAttack, pulseLight->mSustain1, pulseLight->mDecay, pulseLight->mSustain2 };
      
   for(U32 i = 0; i < 4; i++)
   {
      // create this state
      Light::LightState state;
      state.mNumEmitters = 1;
      state.mEmitterIndex = gWorkingLighting->mEmitters.size();
      gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

      // create the emitter
      PointEmitter * emitter = new PointEmitter;
      emitter->mPos = pulseLight->mOrigin;
      emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
      emitter->mIndex = state.mEmitterIndex;
      emitter->mFalloffs[0] = pulseLight->mFalloff1;
      emitter->mFalloffs[1] = pulseLight->mFalloff2;
      emitter->mAnimated = true;

      // color and duration
      state.mDuration = durations[i];
      emitter->mColor = (i == 0 || i == 3) ? pulseLight->mColor1 : pulseLight->mColor2;

      gWorkingLighting->mEmitters.push_back(emitter);
      gWorkingLighting->mLightStates.push_back(state);
   }   

   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildFlickerLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildFlickerLight - NULL entity");

   LightFlickerEntity * flickerLight = static_cast<LightFlickerEntity*>(entity);
   AssertFatal(flickerLight, "Lighting::Light::buildFlickerLight - invalid entity");

   if(flickerLight->mSpeed > LightFlickerEntity::VeryFast)
   {
      dPrintf(" ! Invalid speed for flicker light: %s", flickerLight->mLightName);
      return(false);
   }

   // total of 5 states
   mNumStates = 5;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mAnimated = true;

   mAnimType = (flickerLight->mFlags & ~Interior::AnimationLoop) | Interior::AnimationFlicker;
   mName = newStrDup(flickerLight->mLightName);

   ColorF colors [] = {
      flickerLight->mColor1,
      flickerLight->mColor2,
      flickerLight->mColor3,
      flickerLight->mColor4,
      flickerLight->mColor5};

   for(U32 i = 0; i < 5; i++)
   {
      // create this state
      Light::LightState state;
      state.mNumEmitters = 1;
      state.mEmitterIndex = gWorkingLighting->mEmitters.size();
      gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

      // create the emitter
      PointEmitter * emitter = new PointEmitter;
      emitter->mPos = flickerLight->mOrigin;
      emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
      emitter->mIndex = state.mEmitterIndex;
      emitter->mFalloffs[0] = flickerLight->mFalloff1;
      emitter->mFalloffs[1] = flickerLight->mFalloff2;
      emitter->mAnimated = true;

      // color and duration
      state.mDuration = AnimationTimes[flickerLight->mSpeed];
      emitter->mColor = colors[i];

      gWorkingLighting->mEmitters.push_back(emitter);
      gWorkingLighting->mLightStates.push_back(state);
   }   

   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::buildRunwayLight(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::buildRunwayLight - NULL entity");

   LightRunwayEntity * runwayLight = static_cast<LightRunwayEntity*>(entity);
   AssertFatal(runwayLight, "Lighting::Light::buildRunwayLight - invalid entity");

   if(runwayLight->mSpeed > LightRunwayEntity::VeryFast)
   {
      dPrintf(" ! Invalid speed for runway light: %s", runwayLight->mLightName);
      return(false);
   }

   Point3D endPos;
   if(!getTargetPosition(runwayLight->mEndTarget, endPos))
      return(false);

   //
   mNumStates = 0;
   mStateIndex = gWorkingLighting->mLightStates.size();
   mAnimated = true;
   mAnimType = (runwayLight->mFlags & ~Interior::AnimationFlicker) | Interior::AnimationLoop;
   mName = newStrDup(runwayLight->mLightName);
   
   Point3D step = (endPos - runwayLight->mOrigin) / (runwayLight->mSteps + 1);
   Point3D lightPos = runwayLight->mOrigin;

   for(U32 i = 0; i < (runwayLight->mSteps+2); i++)
   {
      // create this state
      Light::LightState state;
      state.mNumEmitters = 1;
      state.mEmitterIndex = gWorkingLighting->mEmitters.size();
      state.mDuration = AnimationTimes[runwayLight->mSpeed];
      gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

      // create the emitter
      PointEmitter * emitter = new PointEmitter;
      emitter->mIndex = state.mEmitterIndex;
      emitter->mFalloffs[0] = runwayLight->mFalloff1;
      emitter->mFalloffs[1] = runwayLight->mFalloff2;
      emitter->mAnimated = true;
      emitter->mColor = runwayLight->mColor;

      // figure out the position...
      emitter->mPos = lightPos;
      emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);
      lightPos += step;

      mNumStates++;
      gWorkingLighting->mEmitters.push_back(emitter);
      gWorkingLighting->mLightStates.push_back(state);
   }

   // pingpong???
   if(runwayLight->mPingPong)
   {
      lightPos -= step;

      for(U32 i = 0; i < runwayLight->mSteps; i++)
      {
         // create this state
         Light::LightState state;
         state.mNumEmitters = 1;
         state.mEmitterIndex = gWorkingLighting->mEmitters.size();
         state.mDuration = AnimationTimes[runwayLight->mSpeed];
         gWorkingLighting->mLightStateEmitterStore.push_back(state.mEmitterIndex);

         // create the emitter
         PointEmitter * emitter = new PointEmitter;
         emitter->mIndex = state.mEmitterIndex;
         emitter->mFalloffs[0] = runwayLight->mFalloff1;
         emitter->mFalloffs[1] = runwayLight->mFalloff2;
         emitter->mAnimated = true;
         emitter->mColor = runwayLight->mColor;

         // figure out the position...
         lightPos -= step;
         emitter->mPos = lightPos;
         emitter->mPoint = gWorkingGeometry->insertPoint(emitter->mPos);

         mNumStates++;
         gWorkingLighting->mEmitters.push_back(emitter);
         gWorkingLighting->mLightStates.push_back(state);
      }
   }

   return(true);
}

//------------------------------------------------------------------------------

bool Lighting::Light::build(BaseLightEntity * entity)
{
   AssertFatal(entity, "Lighting::Light::build - NULL entity");

   // build the light here....   
   if(dynamic_cast<LightEntity*>(entity))
      return(buildLight(entity));

   // scripted lights...
   if(dynamic_cast<LightOmniEntity*>(entity))
      return(buildOmniLight(entity));
   if(dynamic_cast<LightSpotEntity*>(entity))
      return(buildSpotLight(entity));
   if(dynamic_cast<LightBrushEntity*>(entity))
      return(buildBrushLight(entity));

   // animated lights...
   if(dynamic_cast<LightStrobeEntity*>(entity))
      return(buildStrobeLight(entity));
   if(dynamic_cast<LightPulseEntity*>(entity))
      return(buildPulseLight(entity));
   if(dynamic_cast<LightPulse2Entity*>(entity))
      return(buildPulse2Light(entity));
   if(dynamic_cast<LightFlickerEntity*>(entity))
      return(buildFlickerLight(entity));
   if(dynamic_cast<LightRunwayEntity*>(entity))
      return(buildRunwayLight(entity));

   // unsupported light type
   return(false);         
}

//------------------------------------------------------------------------------
// Class Lighting::Light::LightState
//------------------------------------------------------------------------------

void Lighting::Light::LightState::fillStateData(EditGeometry::LightState * state)
{
   AssertFatal(state, "Lighting::Light::LightState::fillStateData - invalid state");
   AssertFatal(mNumEmitters, "Lighting::Light::LightState::fillStateData - no emitters in state");

   // all emitters are same color.. just grab from first
   Emitter * emitter = gWorkingLighting->mEmitters[mEmitterIndex];
   state->color.set(U8(emitter->mColor.red * 255.0f),
                    U8(emitter->mColor.green * 255.0f),
                    U8(emitter->mColor.blue * 255.0f));

   // 
   state->duration = mDuration;

   // merge the lightmaps?
   for(U32 i = 0; i < mNumEmitters; i++)
   {
      emitter = gWorkingLighting->mEmitters[mEmitterIndex + i];

      for(U32 k = 0; k < emitter->mNumSurfaces; k++)
      {
         Surface * surface = gWorkingLighting->mSurfaces[emitter->mSurfaces[k]];
         EmitterInfo * eInfo = surface->getEmitterInfo(mEmitterIndex + i);
         AssertFatal(eInfo->mLightMap, "Lighting::Light::LightState::fillStateData - null Lightmap");
         
         EditGeometry::StateData stateData;
         stateData.pLMap = eInfo->mLightMap;

#ifdef DUMP_LIGHTMAPS
      //
      if(stateData.pLMap)
      {
         // convert the intensity bitmap to RGB
         GBitmap bitmap(stateData.pLMap->getWidth(), stateData.pLMap->getHeight());
         for(U32 y = 0; y < stateData.pLMap->getHeight(); y++)
            for(U32 x = 0; x < stateData.pLMap->getWidth(); x++)
            {
               U8 * pDest = bitmap.getAddress(x, y);
               U8 src = *stateData.pLMap->getAddress(x, y);

               *pDest++ = src;
               *pDest++ = src;
               *pDest++ = src;
            }   

         //
         FileStream output;
         output.open(avar("anim_%x_%d_%d.png", state, i, k), FileStream::Write);
         bitmap.writePNG(output);
      }
#endif

         stateData.surfaceIndex = surface->mSurfaceIndex;

         state->stateData.push_back(stateData);
      }
   }
}

//------------------------------------------------------------------------------

void Lighting::processAnimatedLights()
{
   // walk the animated lights...
   for(U32 i = 0; i < mLights.size(); i++)
   {
      Light * light = mLights[i];
      if(!light->mAnimated)
         continue;

      EditGeometry::AnimatedLight * animLight = new EditGeometry::AnimatedLight;

      // fill in state info
      for(U32 j = 0; j < light->mNumStates; j++)
      {
         EditGeometry::LightState * destLightState = new EditGeometry::LightState;
         Light::LightState srcLightState = mLightStates[light->mStateIndex + j];

         // should we be merging the lightmaps here?
         srcLightState.fillStateData(destLightState);

         animLight->states.push_back(destLightState);
      }

      // Copy the properties
      animLight->name  = light->mName;
      animLight->type  = light->mAnimType;
      animLight->alarm = light->alarm;

      gWorkingGeometry->mAnimatedLights.push_back(animLight);
   }
}

//------------------------------------------------------------------------------
// Class EditGeometry
//------------------------------------------------------------------------------

void EditGeometry::computeLightmaps(const bool alarmMode)
{
   // clear the current surfaces - low detail just white's out the lightmaps
   ColorF ambientF = alarmMode ? mWorldEntity->mEmergencyAmbientColor :
                                 mWorldEntity->mAmbientColor;
   ColorI ambientI(ambientF.red * 255.f, ambientF.green * 255.f, ambientF.blue * 255.f);

   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      Surface & surface = mSurfaces[i];
      if(!alarmMode)
      {
         fillInLightmapInfo(surface);
         fillInTexSpaceMat(surface);
      }

      surface.pLMap = alarmMode ? surface.pAlarmLMap : surface.pNormalLMap;
   
      //
      if(surface.pLMap)
      {
         if(gBuildAsLowDetail)
         {
            for(U32 y = 0; y < surface.pLMap->getHeight(); y++)
               dMemset(surface.pLMap->getAddress(0, y), 0xff, (surface.pLMap->getWidth() *
               surface.pLMap->bytesPerPixel));
         }
         else if(!(surface.flags & Interior::SurfaceOutsideVisible))
         {
            // fill with ambient
            for(U32 y = 0; y < surface.pLMap->getHeight(); y++)
               for(U32 x = 0; x < surface.pLMap->getWidth(); x++)
               {
                  U8 * pPixel = surface.pLMap->getAddress(x,y);
                  *(pPixel++) = ambientI.red;
                  *(pPixel++) = ambientI.green;
                  *(pPixel)   = ambientI.blue;
               }
         } 
         else // clear out the outside lightmaps
         {
            for(U32 y = 0; y < surface.pLMap->getHeight(); y++)
               dMemset(surface.pLMap->getAddress(0, y), 0, (surface.pLMap->getWidth() *
               surface.pLMap->bytesPerPixel));
         }
      }
   }

   if(gBuildAsLowDetail)
      return;

   Lighting * lighting = new Lighting;
   gWorkingLighting = lighting;
   gWorkingLighting->mAmbientColor = ambientF;

   //
   lighting->grabLights(alarmMode);

   //
   if(alarmMode == true)
   {
      if((lighting->mLights.size() != 0))
         mHasAlarmState = true;
      else if((mWorldEntity->mEmergencyAmbientColor != ColorF(0,0,0)) &&
         (mWorldEntity->mEmergencyAmbientColor != mWorldEntity->mAmbientColor))
      {
         delete lighting;
         mHasAlarmState = true;
         return;
      }
      else
      {
         delete lighting;
         mHasAlarmState = false;
         return;
      }
   }

   dPrintf( "Grabbing Surfaces\n" );
   lighting->grabSurfaces();
dPrintf( "Grabbing Surfaces Complete\n" );

dPrintf( "Processing Surfaces\n" );
   lighting->processSurfaces();
dPrintf( "Processing Surfaces Complete\n" );

dPrintf( "Creating Shadow Volumes\n" );
   lighting->createShadowVolumes();

dPrintf( "Processing EmitterBSPs\n" );
   lighting->processEmitterBSPs();
dPrintf( "Processing EmitterBSPs Complete\n" );
   lighting->lightSurfaces();
   lighting->processAnimatedLights();

   if(lighting->mNumAmbiguousPlanes)
      dPrintf("\n  * Number of ambiguous planes (dropped):  %d\n", lighting->mNumAmbiguousPlanes);

   //
   delete lighting;
}


