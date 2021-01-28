//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MORIANUTIL_H_
#define _MORIANUTIL_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MORIANBASICS_H_
#include "map2dif/morianBasics.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif

//-------------------------------------- Util functions
S32 planeGCD(S32 x, S32 y, S32 z, S32 d);

F64 getWindingSurfaceArea(const Winding& rWinding, U32 planeEQIndex);
bool clipWindingToPlaneFront(Winding* rWinding, const PlaneEQ&);
bool clipWindingToPlaneFront(Winding* rWinding, U32 planeEQIndex);
U32 windingWhichSide(const Winding& rWinding, U32 windingPlaneIndex, U32 planeEQIndex);
bool windingsEquivalent(const Winding& winding1, const Winding& winding2);
bool pointsAreColinear(U32 p1, U32 p2, U32 p3);

bool collapseWindings(Winding& into, const Winding& from);

void createBoundedWinding(const Point3D& minBound,
                          const Point3D& maxBound,
                          U32         planeEQIndex,
                          Winding*       finalWinding);
bool createBaseWinding(const Vector<U32>& rPoints, const Point3D& normal, Winding* pWinding);
void dumpWinding(const Winding& winding, const char* prefixString);

bool intersectWGPlanes(const U32, const U32, const U32, Point3D*);

class CSGBrush;
void splitBrush(CSGBrush*& inBrush,
                U32        planeEQIndex,
                CSGBrush*& outFront,
                CSGBrush*& outBack);

struct PlaneEQ;
void assessPlane(const U32    testPlane,
                 const CSGBrush& rTestBrush,
                 S32*          numCoplanar,
                 S32*          numTinyWindings,
                 S32*          numSplits,
                 S32*          numFront,
                 S32*          numBack);

void reextendName(char* pName, const char* pExt);
void extendName(char* pName, const char* pExt);

class BrushArena {
  private:
   Vector<CSGBrush*>  mBuffers;

   bool      arenaValid;
   U32       arenaSize;
   CSGBrush* mFreeListHead;

   void newBuffer();
  public:
   BrushArena(U32 _arenaSize);
   ~BrushArena();

   CSGBrush* allocateBrush();
   void      freeBrush(CSGBrush*);
};
extern BrushArena gBrushArena;

#endif //_MORIANUTIL_H_

