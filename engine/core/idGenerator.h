//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _IDGENERATOR_H_
#define _IDGENERATOR_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class IdGenerator
{
private:
   U32 mIdBlockBase;
   U32 mIdRangeSize;
   Vector<U32> mPool;
   U32 mNextId;

   void reclaim();

public:
   IdGenerator(U32 base, U32 numIds)
   {
      VECTOR_SET_ASSOCIATION(mPool);

      mIdBlockBase = base;
      mIdRangeSize = numIds;
      mNextId = mIdBlockBase;
   }

   void reset()
   {
      mPool.clear();
      mNextId = mIdBlockBase;
   }

   U32 alloc()
   {
      // fist check the pool:
      if(!mPool.empty())
      {
         U32 id = mPool.last();
         mPool.pop_back();
         reclaim();
         return id;
      }
      if(mIdRangeSize && mNextId >= mIdBlockBase + mIdRangeSize)
         return 0;

      return mNextId++;
   }

   void free(U32 id)
   {
      AssertFatal(id >= mIdBlockBase, "IdGenerator::alloc: invalid id, id does not belong to this IdGenerator.")
      if(id == mNextId - 1)
      {
         mNextId--;
         reclaim();
      }
      else
         mPool.push_back(id);
   }

   U32 numIdsUsed()
   {
      return mNextId - mIdBlockBase - mPool.size();
   }
};

#endif
