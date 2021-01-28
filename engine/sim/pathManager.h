//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _PATHMANAGER_H_
#define _PATHMANAGER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif

class NetConnection;
class BitStream;

class PathManager
{
   friend class PathManagerEvent;

  private:
   struct PathEntry {
      U32             totalTime;

      Vector<Point3F> positions;
      Vector<QuatF>   rotations;
      Vector<U32>     smoothingType;
      Vector<U32>     msToNext;

      PathEntry() {
         VECTOR_SET_ASSOCIATION(positions);
         VECTOR_SET_ASSOCIATION(rotations);
         VECTOR_SET_ASSOCIATION(smoothingType);
         VECTOR_SET_ASSOCIATION(msToNext);
      }
   };

   Vector<PathEntry*> mPaths;

  public:
   enum PathType {
      BackAndForth,
      Looping
   };

  public:
   PathManager(const bool isServer);
   ~PathManager();

   static void init();
   static void destroy();
   void clearPaths();

   //-------------------------------------- Path querying
  public:
   bool isValidPath(const U32 id) const;
   void getPathPosition(const U32 id, const F64 msPosition, Point3F& rPosition, QuatF &rotation);
   U32  getPathTotalTime(const U32 id) const;
   U32  getPathNumWaypoints(const U32 id) const;
   U32  getWaypointTime(const U32 id, const U32 wayPoint) const;

   U32 getPathTimeBits(const U32 id);
   U32 getPathWaypointBits(const U32 id);

   //-------------------------------------- Path Registration/Transmission/Management
  public:
   // Called after mission load to clear out the paths on the client, and to transmit
   //  the information for the current mission's paths.
   void transmitPaths(NetConnection*);
   void transmitPath(U32);

   U32  allocatePathId();
   void updatePath(const U32 id, const Vector<Point3F>&, const Vector<QuatF>&, const Vector<U32> &, const Vector<U32>&);

   //-------------------------------------- State dumping/reading
  public:
   bool dumpState(BitStream*) const;
   bool readState(BitStream*);

  private:
   bool mIsServer;
   bool mPathsSent;
};

struct PathNode {
   Point3F  position;
   QuatF    rotation;
   U32      smoothingType;
   U32      msToNext;
};

extern PathManager* gClientPathManager;
extern PathManager* gServerPathManager;

//--------------------------------------------------------------------------
inline bool PathManager::isValidPath(const U32 id) const
{
   return (id < mPaths.size()) && mPaths[id]->positions.size() > 0;
}



#endif // _H_PATHMANAGER
