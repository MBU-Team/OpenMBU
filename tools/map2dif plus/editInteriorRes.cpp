//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gBitmap.h"
#include "interior/interior.h"
#include "map2dif plus/editInteriorRes.h"

namespace {

int FN_CDECL detailCmp(const void* p1, const void* p2)
{
   const Interior* pInterior1 = *(reinterpret_cast<Interior**>(const_cast<void*>(p1)));
   const Interior* pInterior2 = *(reinterpret_cast<Interior**>(const_cast<void*>(p2)));

   return S32(pInterior1->getDetailLevel()) - S32(pInterior2->getDetailLevel());
}

} // namespace {}


void EditInteriorResource::sortDetailLevels()
{
   AssertFatal(mDetailLevels.size() != 0, "Error, no detail levels to sort!");

   dQsort(mDetailLevels.address(), mDetailLevels.size(), sizeof(Interior*), detailCmp);

   for (U32 i = 0; i < mDetailLevels.size(); i++) {
      mDetailLevels[i]->mDetailLevel = i;
      if (i == 0)
         continue;
      AssertFatal(mDetailLevels[i]->getMinPixels() < mDetailLevels[i - 1]->getMinPixels(),
                  avar("Error, detail %d has greater or same minpixels as %d! (%d %d)",
                       i, i - 1, mDetailLevels[i]->getMinPixels(),
                       mDetailLevels[i - 1]->getMinPixels()));
   }
}


void EditInteriorResource::addDetailLevel(Interior* pInterior)
{
   mDetailLevels.push_back(pInterior);
}

void EditInteriorResource::setPreviewBitmap(GBitmap* bmp)
{
   delete mPreviewBitmap;
   mPreviewBitmap = bmp;
}

void EditInteriorResource::insertTrigger(InteriorResTrigger* pTrigger)
{
   mTriggers.push_back(pTrigger);
}

void EditInteriorResource::insertPathedChild(InteriorPathFollower *pPathFollower)
{
   mInteriorPathFollowers.push_back(pPathFollower);
}

void EditInteriorResource::insertGameEntity(ItrGameEntity *ent)
{
   mGameEntities.push_back(ent);
}

U32 EditInteriorResource::insertSubObject(Interior* pInterior)
{
   mSubObjects.push_back(pInterior);
   return mSubObjects.size() - 1;
}

U32 EditInteriorResource::insertField(ForceField* object)
{
   mForceFields.push_back(object);
   return mForceFields.size() - 1;
}

U32 EditInteriorResource::insertSpecialNode(AISpecialNode* object)
{
   mAISpecialNodes.push_back(object);
   return mAISpecialNodes.size() - 1;
}
