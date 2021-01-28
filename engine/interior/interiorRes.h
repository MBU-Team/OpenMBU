//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORRES_H_
#define _INTERIORRES_H_

#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

class Stream;
class Interior;
class GBitmap;
class InteriorResTrigger;
class InteriorPath;
class InteriorPathFollower;
class ForceField;
class AISpecialNode;
class ItrGameEntity;

class InteriorResource : public ResourceInstance
{
   typedef ResourceInstance Parent;
   static const U32 smFileVersion;

  protected:
   Vector<Interior*>             mDetailLevels;
   Vector<Interior*>             mSubObjects;
   Vector<InteriorResTrigger*>   mTriggers;
   Vector<InteriorPathFollower*> mInteriorPathFollowers;
   Vector<ForceField*>           mForceFields;
   Vector<AISpecialNode*>        mAISpecialNodes;
   Vector<ItrGameEntity*>           mGameEntities;

   GBitmap* mPreviewBitmap;

  public:
   InteriorResource();
   ~InteriorResource();

   bool            read(Stream& stream);
   bool            write(Stream& stream) const;
   static GBitmap* extractPreview(Stream&);

   S32       getNumDetailLevels() const;
   S32       getNumSubObjects() const;
   S32       getNumTriggers() const;
   S32       getNumInteriorPathFollowers() const;
   S32       getNumForceFields() const;
   S32       getNumSpecialNodes() const;
   S32       getNumGameEntities() const;

   Interior*             getDetailLevel(const U32);
   Interior*             getSubObject(const U32);
   InteriorResTrigger*   getTrigger(const U32);
   InteriorPathFollower* getInteriorPathFollower(const U32);
   ForceField*           getForceField(const U32);
   AISpecialNode*        getSpecialNode(const U32);
   ItrGameEntity*        getGameEntity(const U32);
};
extern ResourceInstance* constructInteriorDIF(Stream& stream);

//--------------------------------------------------------------------------
inline S32 InteriorResource::getNumDetailLevels() const
{
   return mDetailLevels.size();
}

inline S32 InteriorResource::getNumSubObjects() const
{
   return mSubObjects.size();
}

inline S32 InteriorResource::getNumTriggers() const
{
   return mTriggers.size();
}

inline S32 InteriorResource::getNumSpecialNodes() const
{
   return mAISpecialNodes.size();
}

inline S32 InteriorResource::getNumGameEntities() const
{
   return mGameEntities.size();
}

inline S32 InteriorResource::getNumInteriorPathFollowers() const
{
   return mInteriorPathFollowers.size();
}

inline S32 InteriorResource::getNumForceFields() const
{
   return mForceFields.size();
}

inline Interior* InteriorResource::getDetailLevel(const U32 idx)
{
   AssertFatal(idx < getNumDetailLevels(), "Error, out of bounds detail level!");

   return mDetailLevels[idx];
}

inline Interior* InteriorResource::getSubObject(const U32 idx)
{
   AssertFatal(idx < getNumSubObjects(), "Error, out of bounds subObject!");

   return mSubObjects[idx];
}

inline InteriorResTrigger* InteriorResource::getTrigger(const U32 idx)
{
   AssertFatal(idx < getNumTriggers(), "Error, out of bounds trigger!");

   return mTriggers[idx];
}

inline InteriorPathFollower* InteriorResource::getInteriorPathFollower(const U32 idx)
{
   AssertFatal(idx < getNumInteriorPathFollowers(), "Error, out of bounds path follower!");

   return mInteriorPathFollowers[idx];
}

inline ForceField* InteriorResource::getForceField(const U32 idx)
{
   AssertFatal(idx < getNumForceFields(), "Error, out of bounds force field!");

   return mForceFields[idx];
}

inline AISpecialNode* InteriorResource::getSpecialNode(const U32 idx)
{
   AssertFatal(idx < getNumSpecialNodes(), "Error, out of bounds Special Nodes!");

   return mAISpecialNodes[idx];
}

inline ItrGameEntity* InteriorResource::getGameEntity(const U32 idx)
{
   AssertFatal(idx < getNumGameEntities(), "Error, out of bounds Game ENts!");

   return mGameEntities[idx];
}

#endif  // _H_INTERIORRES_

