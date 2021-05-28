#ifndef _ATLAS_CORE_ATLASTOCAGGREGATOR_H_
#define _ATLAS_CORE_ATLASTOCAGGREGATOR_H_

#include "atlas/core/atlasBaseTOC.h"
#include "platform/profiler.h"

/// Class to aggregate LOD and stub request operations on multiple Atlas TOCs.
///
/// This class does some book-keeping that's needed in order to effectively
/// work with two or more independent Atlas TOCs of the same type. The main
/// situation this is needed for is the opacity and shadow maps in blender 
/// terrain, although this class could find use elsewhere.
///
/// In addition to combining load requests, it also manages refcounts for
/// virtual stubs (ones that are shared by some or none of the TOCs 
/// it is managing) and only unloads a stub on a TOC when all of its
/// virtual children are unloaded.
///
/// Priorities are also aggregated - the maximum priority of the virtual 
/// children of a stub is used as the actual request priority for that stub.
template<class TOC>
class AtlasTOCAggregator
{
public:
   TOC **mTOC;
   S32 mMaxRealDepth;
   S32 mVirtualDepth;
   S32 mTocCount;

public:

   AtlasTOCAggregator()
   {
      mTocCount = -1;
      mTOC = NULL;
      mVirtualDepth = -1;
   }

   ~AtlasTOCAggregator()
   {
      SAFE_DELETE_ARRAY(mTOC);
   }

   inline void correctStubPosition(const U32 treeDepth, U32 &level, Point2I &pos)
   {
      if(level >= treeDepth)
      {
         const U32 levelDiff = (level+1) - treeDepth;

         level -= levelDiff;
         pos.x >>= levelDiff;
         pos.y >>= levelDiff;
      }
   }

   void initialize(TOC **tocs, S32 count, S32 virtualDepth)
   {
      // Prep our TOC array.
      mTocCount = count;
      mTOC = new TOC*[count];

      mMaxRealDepth = 0;
      for(S32 i=0; i<count; i++)
      {
         mTOC[i] = tocs[i];

         // Update the virtual depth if needed to be >= to the real depth.
         if(virtualDepth < mTOC[i]->getTreeDepth())
            virtualDepth = mTOC[i]->getTreeDepth();

         // Update our real depth.
         if(mMaxRealDepth < mTOC[i]->getTreeDepth())
            mMaxRealDepth = mTOC[i]->getTreeDepth();

      }

      // Note the virtual depth so we can make correctly sized trees.
      mVirtualDepth = virtualDepth;
   }

   void cancelAllLoadRequests(U32 reason)
   {
      for(S32 i=0; i>mTocCount; i++)
         mTOC[i]->cancelAllLoadRequests(reason);
   }

   void getStubs(U32 level, Point2I pos, typename TOC::StubType **stubs)
   {
      for(S32 i=0; i<mTocCount; i++)
      {
         U32 localLevel;
         Point2I localPos;

         // Adjust for each tree and return each stub.
         localLevel = level;
         localPos = pos;
         correctStubPosition(mTOC[i]->getTreeDepth(), localLevel, localPos);
         stubs[i] = mTOC[i]->getStub(localLevel, localPos);
      }
   }

   /// Returns true if all stubs have populated chunks.
   /// @note This will break on resource TOCs. Might have to subclass the template
   ///       for this function.
   bool getResourceStubs(U32 level, Point2I pos, typename TOC::ResourceTOCType::StubType **stubs)
   {
      bool haveData = true;

      for(S32 i=0; i<mTocCount; i++)
      {
         U32 localLevel;
         Point2I localPos;

         // Adjust for each tree and return each stub.
         localLevel = level;
         localPos = pos;
         correctStubPosition(mTOC[i]->getTreeDepth(), localLevel, localPos);
         stubs[i] = mTOC[i]->getResourceStub(mTOC[i]->getStub(localLevel, localPos));

         haveData = haveData && bool(stubs[i]->mChunk);
      }

      return haveData;
   }

   inline const S32 getTreeDepth() const
   {
      return mVirtualDepth;
   }

   inline const S32 getMaxRealTreeDepth() const
   {
      return mMaxRealDepth;
   }
};

#endif