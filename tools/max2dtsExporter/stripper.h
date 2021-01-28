//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _BITMATRIX_H_
#include "core/bitMatrix.h"
#endif

class Stripper
{
   Vector<S32> numAdjacent;
   Vector<bool> used;
   BitMatrix adjacent;
   Vector<S32> vertexCache;
   Vector<S32> recentFaces;
   S32 currentFace;
   bool limitStripLength;
   S32 bestLength;
   U32 cacheMisses;

   Vector<TSDrawPrimitive> strips;
   Vector<U16> stripIndices;

   Vector<TSDrawPrimitive> & faces;
   Vector<U16> & faceIndices;

   void clearCache();
   void addToCache(S32 vertexIndex);
   void addToCache(S32 vertexIndex, U32 posFromBack);

   void getVerts(S32 face, S32 & oldVert0, S32 & oldVert1, S32 & addVert);
   void rotateFace(S32 start, Vector<U16> & indices);
   bool swapNeeded(S32 oldVert0, S32 oldVert1);
   F32 getScore(S32 face, bool ignoreOrder);
   bool faceHasEdge(S32 face, U32 idx0, U32 idx1);
   void getAdjacentFaces(S32 startFace, S32 endFace, S32 face, S32 & face0, S32 & face1, S32 & face2);

   void setAdjacency(S32 startFace, S32 endFace);
   bool startStrip(TSDrawPrimitive & strip, S32 startFace, S32 endFace);
   bool addStrip(TSDrawPrimitive & strip, S32 startFace, S32 endFace);
   bool stripLongEnough(S32 startFace, S32 endFace);

   void testCache(S32 addedFace);
   bool canGo(S32 face);

   void makeStripsB(); // makeStrips() from faces...assumes all faces have same material index
   void copyParams(Stripper *from);

   char * errorStr;
   void setExportError(const char * str) { errorStr = errorStr ? errorStr : dStrdup(str); }

   public:

   Stripper(Vector<TSDrawPrimitive> & faces, Vector<U16> & indices);
   Stripper(Stripper &);
   ~Stripper();

   void makeStrips();
   S32 continueStrip(S32 startFace, S32 endFace, S32 len, S32 restart); // used for simulation...
   void getStrips(Vector<TSDrawPrimitive> & s, Vector<U16> & si) { s=strips; si=stripIndices; }

   void setLimitStripLength(bool lim) { limitStripLength = lim; }
   void resetCacheMisses() { cacheMisses = 0; }
   U32 getCacheMisses() { return cacheMisses; }

   const char * getError() { return errorStr; }
   bool isError() { return errorStr!=NULL; }

   // adjust strip building strategy
   static F32 adjacencyWeight;
   static F32 noswapWeight;
   static F32 alreadyCachedWeight;
   static U32 cacheSize;
   static U32 simK;
};


