//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/shapeBase.h"
#include "game/item.h"
#include "game/trigger.h"

//----------------------------------------------------------------------------

void collisionFilter(SceneObject* object,void *key)
{
   Container::CallbackInfo* info = reinterpret_cast<Container::CallbackInfo*>(key);
   ShapeBase* ptr = reinterpret_cast<ShapeBase*>(info->key);

   if (object->getTypeMask() & ItemObjectType) {
      // We've hit it's bounding box, that's close enough for items.
      Item* item = static_cast<Item*>(object);
      if (ptr != item->getCollisionObject())
         ptr->queueCollision(item,ptr->getVelocity() - item->getVelocity());
   }
   else
      if (object->getTypeMask() & TriggerObjectType) {
         // We've hit it's bounding box, that's close enough for triggers
         Trigger* pTrigger = static_cast<Trigger*>(object);
         pTrigger->potentialEnterObject(ptr);
      }
      else
         if (object->getTypeMask() & CorpseObjectType)  {
            // Ok, guess it's close enough for corpses too...
            ShapeBase* col = static_cast<ShapeBase*>(object);
            ptr->queueCollision(col,ptr->getVelocity() - col->getVelocity());
         }
         else
            object->buildPolyList(info->polyList,info->boundingBox,info->boundingSphere);
}

void defaultFilter(SceneObject* object,void * key)
{
   Container::CallbackInfo* info = reinterpret_cast<Container::CallbackInfo*>(key);
   object->buildPolyList(info->polyList,info->boundingBox,info->boundingSphere);
}

