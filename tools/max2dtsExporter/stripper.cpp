//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2dtsExporter/stripper.h"

F32 Stripper::adjacencyWeight = 5;
F32 Stripper::noswapWeight = 3;
F32 Stripper::alreadyCachedWeight = 1;
U32 Stripper::cacheSize = 16;
U32 Stripper::simK = 5;


Stripper::Stripper(Vector<TSDrawPrimitive> & _faces, Vector<U16> & _faceIndices)
   : faces(_faces), faceIndices(_faceIndices), adjacent(_faces.size(),_faces.size())
{
   // keep track of whether face used in a strip yet
   used.setSize(faces.size());
   for (S32 i=0; i<faces.size(); i++)
      used[i]=false;

   clearCache();
   cacheMisses = 0; // definitely don't want to clear this every time we clear the cache!

   bestLength = -1;
   errorStr = NULL;
}

Stripper::Stripper(Stripper & stripper)
   : faces(stripper.faces), faceIndices(stripper.faceIndices), adjacent(stripper.faces.size(),stripper.faces.size()),
     numAdjacent(stripper.numAdjacent), used(stripper.used), vertexCache(stripper.vertexCache),
     recentFaces(stripper.recentFaces), strips(stripper.strips), stripIndices(stripper.stripIndices)
{
   currentFace = stripper.currentFace;
   limitStripLength = stripper.limitStripLength;
   bestLength = stripper.bestLength;
   cacheMisses = stripper.cacheMisses;
   adjacent.clearAllBits();
   for (S32 i=0; i<faces.size(); i++)
      for (S32 j=0; j<faces.size(); j++)
         if (stripper.adjacent.isSet(i,j))
            adjacent.setBit(i,j);
   errorStr = NULL;
}

Stripper::~Stripper()
{
   dFree(errorStr);
}

void Stripper::testCache(S32 addedFace)
{
   S32 * cache = &vertexCache.last();

   // make sure last 3 verts on strip list are last 3 verts in cache
   U16 * indices = &stripIndices.last();
   if (*indices!=*cache || *(indices-1)!=*(cache-1) || *(indices-2)!=*(cache-2))
   {
      setExportError("Assertion failed when stripping (11)");
      return;
   }

   if (addedFace>=0)
   {
      // make sure current face is still last 3 items in the cache
      TSDrawPrimitive & face = faces[addedFace];
      U32 idx0 = faceIndices[face.start+0];
      U32 idx1 = faceIndices[face.start+1];
      U32 idx2 = faceIndices[face.start+2];
      if ( idx0!=*(cache) && idx0!=*(cache-1) && idx0!=*(cache-2))
      {
         setExportError("Assertion failed when stripping (8)");
         return;
      }
      if ( idx1!=*(cache) && idx1!=*(cache-1) && idx1!=*(cache-2))
      {
         setExportError("Assertion failed when stripping (9)");
         return;
      }
      if ( idx2!=*(cache) && idx2!=*(cache-1) && idx2!=*(cache-2))
      {
         setExportError("Assertion failed when stripping (10)");
         return;
      }
   }
}

void Stripper::copyParams(Stripper * from)
{
   limitStripLength = from->limitStripLength;
}

void Stripper::makeStrips()
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // main strip loop...
   U32 start, end, i, sz;
   Vector<TSDrawPrimitive> someFaces;
   Vector<U16> someIndices;
   for (start = 0; start<faces.size(); start=end)
   {
      for (end=start; end<faces.size() && faces[start].matIndex==faces[end].matIndex; end++)
         ;

      if (start==0 && end==faces.size())
      {
         // all same material, no need to copy anything
         makeStripsB();
         return;
      }

      // copy start to end faces into new list -- this is so we end up doing less copying
      // down the road (when we are doing the look ahead simulation)
      someFaces.clear();
      someIndices.clear();
      for (i=start;i<end;i++)
      {
         someFaces.push_back(faces[i]);
         someFaces.last().start = someIndices.size();
         someIndices.push_back(faceIndices[faces[i].start + 0]);
         someIndices.push_back(faceIndices[faces[i].start + 1]);
         someIndices.push_back(faceIndices[faces[i].start + 2]);
      }

      // strip these...
      Stripper someStrips(someFaces,someIndices);
      someStrips.copyParams(this);
      someStrips.makeStripsB();
      if (isError()) return;

      // copy these strips into our arrays
      sz = strips.size();
      strips.setSize(sz+someStrips.strips.size());
      for (i=0; i<someStrips.strips.size(); i++)
      {
         strips[i+sz] = someStrips.strips[i];
         strips[i+sz].start += stripIndices.size();
      }
      sz = stripIndices.size();
      stripIndices.setSize(sz+someStrips.stripIndices.size());
      dMemcpy(&stripIndices[sz],someStrips.stripIndices.address(),someStrips.stripIndices.size()*sizeof(U16));

      // update cache misses
      cacheMisses += someStrips.getCacheMisses();
   }
}

void Stripper::makeStripsB()
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // set adjacency info
   setAdjacency(0,faces.size());

   while (1)
   {
      strips.increment();
      TSDrawPrimitive & strip = strips.last();
      strip.start = stripIndices.size();
      strip.numElements = 0;
      if (faces[0].matIndex & TSDrawPrimitive::NoMaterial)
         strip.matIndex = TSDrawPrimitive::NoMaterial;
      else
         strip.matIndex = faces[0].matIndex & TSDrawPrimitive::MaterialMask;
      strip.matIndex |= TSDrawPrimitive::Strip | TSDrawPrimitive::Indexed;

      if (!startStrip(strip,0,faces.size()))
      {
         strips.decrement();
         break;
      }

      while (addStrip(strip,0,faces.size()));

      // if already encountered an error, then
      // we'll just go through the motions
      if (isError()) return;
   }

   // let's make sure everything is legit up till here
   U32 i;
   for (i=0; i<faces.size(); i++)
   {
      if (!used[i])
      {
         setExportError("Assertion failed when stripping (1)");
         return;
      }
   }
   for (i=0; i<numAdjacent.size(); i++)
   {
      if (numAdjacent[i])
      {
         setExportError("Assertion failed when stripping (2)");
         return;
      }
   }

   // make sure all faces were used, o.w. it's an error
   for (i=0; i<used.size(); i++)
   {
      if (!used[i])
      {
         setExportError("Assertion failed when stripping (3)");
         return;
      }
   }
}

/*
void Stripper::makeStrips()
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // main strip loop...
   U32 start, end, i;
   for (start = 0; start<faces.size(); start=end)
   {
      for (end=start; end<faces.size() && faces[start].matIndex==faces[end].matIndex; end++)
         ;

      // set adjacency info
      setAdjacency(start,end);

      while (1)
      {
         strips.increment();
         TSDrawPrimitive & strip = strips.last();
         strip.start = stripIndices.size();
         strip.numElements = 0;
         if (faces[start].matIndex & TSDrawPrimitive::NoMaterial)
            strip.matIndex = TSDrawPrimitive::NoMaterial;
         else
            strip.matIndex = faces[start].matIndex & TSDrawPrimitive::MaterialMask;
         strip.matIndex |= TSDrawPrimitive::Strip | TSDrawPrimitive::Indexed;

         if (!startStrip(strip,start,end))
         {
            strips.decrement();
            break;
         }

         while (addStrip(strip,start,end));

         // if already encountered an error, then
         // we'll just go through the motions
         if (isError()) return;
      }

      // let's make sure everything is legit up till here
      for (i=start; i<end; i++)
      {
         if (!used[i])
         {
            setExportError("Assertion failed when stripping (1)");
            return;
         }
      }
      for (i=0; i<numAdjacent.size(); i++)
      {
         if (numAdjacent[i])
         {
            setExportError("Assertion failed when stripping (2)");
            return;
         }
      }
   }

   // make sure all faces were used, o.w. it's an error
   for (i=0; i<used.size(); i++)
   {
      if (!used[i])
      {
         setExportError("Assertion failed when stripping (3)");
         return;
      }
   }
}
*/

// used for simulating addition of "len" more faces with a forced strip restart after "restart" faces
S32 Stripper::continueStrip(S32 startFace, S32 endFace, S32 len, S32 restart)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   TSDrawPrimitive & strip = strips.last();

   while (restart && addStrip(strip,startFace,endFace))
      restart--,len--;

   // either restarted when we were forced to or restarted on our own
   // either way, continue adding faces till len==0
   while (len)
   {
      strips.increment();
      TSDrawPrimitive & strip = strips.last();
      strip.start = stripIndices.size();
      strip.numElements = 0;
      if (faces[startFace].matIndex & TSDrawPrimitive::NoMaterial)
         strip.matIndex = TSDrawPrimitive::NoMaterial;
      else
         strip.matIndex = faces[startFace].matIndex & TSDrawPrimitive::MaterialMask;
      strip.matIndex |= TSDrawPrimitive::Strip | TSDrawPrimitive::Indexed;

      if (!startStrip(strip,startFace,endFace))
      {
         strips.decrement();
         break;
      }
      len--;

      while (len && addStrip(strip,startFace,endFace))
         len--;

      // if already encountered an error, then
      // we'll just go through the motions
      if (isError()) return 0;
   }

   return restart;
}

void Stripper::setAdjacency(S32 startFace, S32 endFace)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // two faces adjacent only if wound the same way (so shared edge must appear in opp. order)
   S32 i,j;
   numAdjacent.setSize(faces.size());
   for (i=0; i<numAdjacent.size(); i++)
      numAdjacent[i] = 0;

   adjacent.clearAllBits();

   for (i=startFace; i<endFace-1; i++)
   {
      // the i-indices...
      U32 idx0 = faceIndices[faces[i].start + 0];
      U32 idx1 = faceIndices[faces[i].start + 1];
      U32 idx2 = faceIndices[faces[i].start + 2];

      for (j=i+1; j<endFace; j++)
      {
         // the j-indices...
         U32 jdx0 = faceIndices[faces[j].start + 0];
         U32 jdx1 = faceIndices[faces[j].start + 1];
         U32 jdx2 = faceIndices[faces[j].start + 2];

         // we don't want to be adjacent if we share all our vertices...
         if ( ( idx0==jdx0 || idx0==jdx1 || idx0==jdx2) &&
              ( idx1==jdx0 || idx1==jdx1 || idx1==jdx2) &&
              ( idx2==jdx0 || idx2==jdx1 || idx2==jdx2) )
            continue;

         // test adjacency...
         if ( ((idx0==jdx1) && (idx1==jdx0)) || ((idx0==jdx2) && (idx1==jdx1)) || ((idx0==jdx0) && (idx1==jdx2)) ||
              ((idx1==jdx1) && (idx2==jdx0)) || ((idx1==jdx2) && (idx2==jdx1)) || ((idx1==jdx0) && (idx2==jdx2)) ||
              ((idx2==jdx1) && (idx0==jdx0)) || ((idx2==jdx2) && (idx0==jdx1)) || ((idx2==jdx0) && (idx0==jdx2)) )
         {
            // i,j are adjacent
            if (adjacent.isSet(i,j) || adjacent.isSet(j,i))
            {
               setExportError("wtf (1)");
               return;
            }
            adjacent.setBit(i,j);
            adjacent.setBit(j,i);
            if (!adjacent.isSet(i,j) || !adjacent.isSet(j,i))
            {
               setExportError("wtf (2)");
               return;
            }
            numAdjacent[i]++;
            numAdjacent[j]++;
         }
      }
   }
}

void Stripper::clearCache()
{
   vertexCache.setSize(cacheSize);
   for (S32 i=0; i<vertexCache.size(); i++)
      vertexCache[i] = -1;
}

void Stripper::addToCache(S32 vertexIndex)
{
   S32 i;
   for (i=0; i<vertexCache.size(); i++)
      if (vertexCache[i]==vertexIndex)
         break;
   if (i==vertexCache.size())
      cacheMisses++;

   vertexCache.erase((U32)0);
   vertexCache.push_back(vertexIndex);
}

void Stripper::addToCache(S32 vertexIndex, U32 posFromBack)
{
   // theoretically this could result in a cache miss, but never used that way so
   // we won't check...

   vertexCache.erase((U32)0);
   vertexCache.insert(vertexCache.size()-posFromBack);
   vertexCache[vertexCache.size()-1-posFromBack] = vertexIndex;
}

bool Stripper::startStrip(TSDrawPrimitive & strip, S32 startFace, S32 endFace)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   S32 i,j;

   S32 bestFace  = -1;

   // first search the list of faces with neighbors that were recently visited
   for (i=0;i<recentFaces.size();i++)
   {
      if (!used[recentFaces[i]])
      {
         bestFace = recentFaces[i];
         break;
      }
   }
   recentFaces.clear();

   // if we didn't find one above, search for a good face
   if (bestFace<0)
   {
      S32 bestScore = -1;
      for (i=startFace; i<endFace; i++)
      {
         if (used[i])
            continue;

         // how many of the verts are in the cache?
         // Question: should we favor verts that occur early/late in the cache?
         U32 inCache = 0;
         U32 idx0 = faceIndices[faces[i].start + 0];
         U32 idx1 = faceIndices[faces[i].start + 1];
         U32 idx2 = faceIndices[faces[i].start + 2];

         for (j=0; j<vertexCache.size(); j++)
            if (vertexCache[j] == idx0)
            {
               inCache++;
               break;
            }
         for (j=0; j<vertexCache.size(); j++)
            if (vertexCache[j] == idx1)
            {
               inCache++;
               break;
            }
         for (j=0; j<vertexCache.size(); j++)
            if (vertexCache[j] == idx2)
            {
               inCache++;
               break;
            }
         S32 currentScore = (inCache<<4) + numAdjacent[i];
         if (currentScore>bestScore)
         {
            bestFace = i;
            bestScore = currentScore;
         }
      }
   }

   // if no face...
   if (bestFace<0)
      return false;

   // start the strip -- add in standard order...may be changed later
   strip.numElements += 3;
   stripIndices.push_back(faceIndices[faces[bestFace].start + 0]);
   addToCache(stripIndices.last());
   stripIndices.push_back(faceIndices[faces[bestFace].start + 1]);
   addToCache(stripIndices.last());
   stripIndices.push_back(faceIndices[faces[bestFace].start + 2]);
   addToCache(stripIndices.last());

   testCache(bestFace);

   // adjust adjacency information
   for (j=startFace; j<endFace; j++)
   {
      if (j==bestFace || used[j])
         continue;
      if (adjacent.isSet(bestFace,j))
      {
         numAdjacent[j]--;
         if (numAdjacent[j]<0)
         {
            setExportError("Assertion failed when stripping (4)");
            return false;
         }
      }
   }

   // mark face as used
   used[bestFace] = true;
   numAdjacent[bestFace] = 0;
   currentFace = bestFace;

   return true;
}

void Stripper::getVerts(S32 face, S32 & oldVert0, S32 & oldVert1, S32 & addVert)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   Vector<S32> prev;
   addVert = -1;
   TSDrawPrimitive & addFace = faces[face];
   U32 idx0 = faceIndices[addFace.start+0];
   U32 idx1 = faceIndices[addFace.start+1];
   U32 idx2 = faceIndices[addFace.start+2];
   S32 * cache = &vertexCache.last();
   if (idx0==*cache || idx0==*(cache-1) || idx0==*(cache-2))
      prev.push_back(idx0);
   else
      addVert = idx0;
   if (idx1==*cache || idx1==*(cache-1) || idx1==*(cache-2))
      prev.push_back(idx1);
   else
      addVert = idx1;
   if (idx2==*cache || idx2==*(cache-1) || idx2==*(cache-2))
      prev.push_back(idx2);
   else
      addVert = idx2;
   if (addVert<0 || prev.size()!=2)
   {
      setExportError("Assertion failed when stripping (5)");
      return;
   }

   if (idx1==addVert)
   {
      // swap order of oldVerts
      oldVert0 = prev[1];
      oldVert1 = prev[0];
   }
   else
   {
      // keep order
      oldVert0 = prev[0];
      oldVert1 = prev[1];
   }
}

// assumes start + 0,1,2 is a triangle or first 3 indices of a strip
void Stripper::rotateFace(S32 start, Vector<U16> & indices)
{
   U32 tmp = indices[start];
   indices[start] = indices[start+1];
   indices[start+1] = indices[start+2];
   indices[start+2] = tmp;

   S32 * cache = &vertexCache.last();
   tmp = *(cache-2);
   *(cache-2) = *(cache-1);
   *(cache-1) = *cache;
   *cache     = tmp;

   testCache(currentFace);
}

bool Stripper::swapNeeded(S32 oldVert0, S32 oldVert1)
{
   S32 * cache = &vertexCache.last();

   return (*cache!=oldVert0 || *(cache-1)!=oldVert1) && (*cache!=oldVert1 || *(cache-1)!=oldVert0);

// Long form:
//   if ( (*cache==oldVert0 && *(cache-1)==oldVert1) || (*cache==oldVert1 && *(cache-1)==oldVert0) )
//      return false;
//   else
//      return true;
}

F32 Stripper::getScore(S32 face, bool ignoreOrder)
{
   // score face depending on following criteria:
   // -- select face with fewest adjacencies?
   // -- select face that continues strip without swap?
   // -- select face with new vertex already in cache?
   // weighting of each criteria controlled by adjacencyWeight, noswapWeight, and alreadyCachedWeight
   // if ignoreOrder is true, don't worry about swaps

   if (face<0)
      return -100000.0f;

   // get the 2 verts from the last face and the 1 new vert
   S32 oldVert0, oldVert1, addVert;
   getVerts(face, oldVert0, oldVert1, addVert);

   F32 score = 0.0f;

   // fewer adjacent faces the better...
   score -= numAdjacent[face] * adjacencyWeight;

   // reward if no swap needed...don't worry about order (only same facing faces get here)
   if (!ignoreOrder && !swapNeeded(oldVert0,oldVert1))
      score += noswapWeight;

   // if new vertex in the cache, add the already in cache bonus...
   for (S32 i=0;i<vertexCache.size();i++)
      if (vertexCache[i]==addVert)
      {
         score += alreadyCachedWeight;
         break;
      }

   return score;
}

bool Stripper::faceHasEdge(S32 face, U32 idx0, U32 idx1)
{
   // return true if face has edge idx0, idx1 (in that order)
   S32 start = faces[face].start;
   U32 vi0 = faceIndices[start+0];
   U32 vi1 = faceIndices[start+1];
   U32 vi2 = faceIndices[start+2];

   if ( (vi0==idx0 && vi1==idx1) || (vi1==idx0 && vi2==idx1) || (vi2==idx0 && vi0==idx1) )
      return true;
   return false;
}

void Stripper::getAdjacentFaces(S32 startFace, S32 endFace, S32 face, S32 & face0, S32 & face1, S32 & face2)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // we return one face per edge...ties go to face with fewest adjacencies
   S32 adj0=-1,adj1=-1,adj2=-1;
   face0=face1=face2=-1;

   U32 idx0 = faceIndices[faces[face].start + 0];
   U32 idx1 = faceIndices[faces[face].start + 1];
   U32 idx2 = faceIndices[faces[face].start + 2];
   for (S32 i=startFace; i<endFace; i++)
   {
      if (i==face || used[i])
         continue;

      if (!adjacent.isSet(face,i))
         continue;

      // which edge is this face on
      if (faceHasEdge(i,idx1,idx0))
      {
         if (adj0<0 || numAdjacent[i]<adj0)
         {
            face0 = i;
            adj0 = numAdjacent[i];
         }
      }
      else if (faceHasEdge(i,idx2,idx1))
      {
         if (adj1<0 || numAdjacent[i]<adj1)
         {
            face1 = i;
            adj1 = numAdjacent[i];
         }
      }
      else if (faceHasEdge(i,idx0,idx2))
      {
         if (adj2<0 || numAdjacent[i]<adj2)
         {
            face2 = i;
            adj2 = numAdjacent[i];
         }
      }
      else
      {
         setExportError("Assertion failed when stripping (6)");
         return;
      }
   }
}

bool Stripper::stripLongEnough(S32 startFace, S32 endFace)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (!limitStripLength)
      return false;

   // simulate stopping the strip here and continuing for cacheSize+simK more rounds
   Stripper strip0(*this);
   strip0.setLimitStripLength(false);
   strip0.resetCacheMisses();
   strip0.continueStrip(startFace,endFace,cacheSize+simK,0);
   U32 stop0 = strip0.getCacheMisses();

   // re-simulate previous best length (-1)
   U32 bestMisses;
   if (--bestLength<2)
      bestLength = 1;
   Stripper stripper(*this);
   stripper.setLimitStripLength(false);
   stripper.resetCacheMisses();
   stripper.continueStrip(startFace,endFace,cacheSize+simK,bestLength);
   bestMisses = stripper.getCacheMisses();
   if (bestMisses<=stop0)
      return false;

   for (S32 i=1; i<cacheSize+simK; i++)
   {
      if (i==bestLength)
         continue;

      Stripper stripper(*this);
      stripper.setLimitStripLength(false);
      stripper.resetCacheMisses();
      S32 underShoot = stripper.continueStrip(startFace,endFace,cacheSize+simK,i);
      U32 misses = stripper.getCacheMisses();
      if (misses<bestMisses)
      {
         bestMisses = misses;
         bestLength = i - underShoot;
      }
      if (misses<=stop0)
         return false;
      // undershoot is how much we missed our restart target by...i.e., if we wanted
      // to restart after 5 faces and underShoot is 1, then we restarted after 4.
      // If undershoot is positive, then we're going to end up restarting at the same
      // place on future iterations, so break out of the loop now to save time
      if (underShoot>0)
         break;
   }

   // survived the gauntlet...
   return true;
}

bool Stripper::canGo(S32 face)
{
   if (face<0)
      return false;

   U32 idx0 = faceIndices[faces[face].start + 0];
   U32 idx1 = faceIndices[faces[face].start + 1];
   U32 idx2 = faceIndices[faces[face].start + 2];

   U32 last = stripIndices.last();
   if (last==idx0 || last==idx1 || last==idx2)
      return true;
   return false;
}

bool Stripper::addStrip(TSDrawPrimitive & strip, S32 startFace, S32 endFace)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (strip.numElements>3 && stripLongEnough(startFace,endFace))
      return false;

   testCache(currentFace);

   // get unused faces adjacent to current face (choose only faces pointing same way)
   // if more than one face adjacent on an edge (unusual case) chooses one with lowest adjacency count
   S32 face0, face1, face2;
   getAdjacentFaces(startFace,endFace,currentFace,face0,face1,face2);

   // one more check -- make sure most recent vert is in face
   // this can happen in exceptional case where we "back up"
   // using common edge between previous two faces, but not the
   // previous face (a v-junction?)
   bool bad0,bad1,bad2;
   bad0=bad1=bad2=false;
   if (strip.numElements!=3 && !canGo(face0))
      face0=-1;
   if (strip.numElements!=3 && !canGo(face1))
      face1=-1;
   if (strip.numElements!=3 && !canGo(face2))
      face2=-1;

   if (face0<0 && face1<0 && face2<0)
      return false;

   testCache(currentFace);

   // score faces, choose highest score
   F32 score0 = getScore(face0,strip.numElements==3);
   F32 score1 = getScore(face1,strip.numElements==3);
   F32 score2 = getScore(face2,strip.numElements==3);
   S32 bestFace = -1;
   if (score0>=score1 && score0>=score2)
      bestFace = face0;
   else if (score1>=score0 && score1>=score2)
      bestFace = face1;
   else if (score2>=score1 && score2>=score0)
      bestFace = face2;

   testCache(currentFace);

   // add new element
   S32 oldVert0, oldVert1, addVert;
   getVerts(bestFace,oldVert0,oldVert1,addVert);

   testCache(currentFace);

   if (strip.numElements==3)
   {
      testCache(currentFace);

      // special case...rotate previous element until we can add this face
      U32 doh=0;
      while (swapNeeded(oldVert0,oldVert1))
      {
         rotateFace(strip.start,stripIndices);
         if (++doh==3)
         {
            setExportError("Assertion error while stripping: infinite loop");
            return false;
         }
      }
      stripIndices.push_back(addVert);
      addToCache(addVert);
      strip.numElements++;

      testCache(bestFace);
   }
   else
   {
      testCache(currentFace);

      if (swapNeeded(oldVert0,oldVert1))
      {
         U32 sz = stripIndices.size();
         U32 dup = stripIndices[sz-3];
         stripIndices.insert(sz-1);
         stripIndices[sz-1] = dup;
         addToCache(dup,1);
         strip.numElements++;

         testCache(-1);
      }

      stripIndices.push_back(addVert);
      strip.numElements++;
      addToCache(addVert);

      testCache(bestFace);
   }

   // add other faces to recentFaces list
   if (face0>=0 && face0!=bestFace)
      recentFaces.push_back(face0);
   if (face1>=0 && face1!=bestFace)
      recentFaces.push_back(face1);
   if (face2>=0 && face2!=bestFace)
      recentFaces.push_back(face2);

   // adjust adjacency information
   for (S32 j=startFace; j<endFace; j++)
   {
      if (j==bestFace || used[j])
         continue;
      if (adjacent.isSet(bestFace,j))
      {
         numAdjacent[j]--;
         if (numAdjacent[j]<0)
         {
            setExportError("Assertion failed when stripping (7)");
            return false;
         }
      }
   }

   // mark face as used
   used[bestFace] = true;
   numAdjacent[bestFace] = 0;
   currentFace = bestFace;

   return true;
}




