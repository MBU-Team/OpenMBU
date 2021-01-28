//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BSPNODE_H_
#define _BSPNODE_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MORIANBASICS_H_
#include "map2dif/morianBasics.h"
#endif

class CSGBrush;
class CSGPlane;

//------------------------------------------------------------------------------
class EditBSPNode {
  public:
   struct PortalEntry {
     public:
      ~PortalEntry();

      Vector<Winding*> windings;
      U32           planeEQIndex;
      U32           portalId;

      Point3D ortho1;
      Point3D ortho2;

      void copyEntry(const PortalEntry&);
   };
   struct VisLink {
     public:
      Winding winding;
      U32     planeEQIndex;

      S32     portalId;

      EditBSPNode* pFront;
      EditBSPNode* pBack;

      VisLink* pNext;

     public:
      bool intersect(U32 planeEQIndex);
      PlaneSide whichSide(U32 planeEQIndex);
   };

   enum PlaneType {
      Structural = 0,
      Portal     = 1,
      Detail     = 2
   };

  private:
   void         createPortalWindings(const PortalEntry& rBaseWindings, PortalEntry* pFinalEntry);

  private:
   S32  calcPlaneRating(U32 testPlane);
   bool planeInParents(U32 testPlane);
   U32  selectBestPlane();
   void eraseReferencingLinks();

   void splitBrushList();
   void splitVisLinks();
   void splitPortalEntries();


   CSGBrush* findSolidBrush();

  public:
   S32  planeEQIndex;

   EditBSPNode* pParent;

   EditBSPNode* pFront;
   EditBSPNode* pBack;

   PlaneType planeType;

   bool      isSolid;
   CSGBrush* solidBrush;

   S32     zoneId;
   U32     nodeId;

   // Portal information
   Vector<PortalEntry*> portals;

   // Vising information
   Vector<VisLink*> visLinks;
   void enterLink(VisLink*);

   // For building BSP.  List of Brushes
   Vector<CSGBrush*>    brushList;

  public:
   EditBSPNode() : planeEQIndex(-1), planeType(Detail),
               pParent(NULL), pFront(NULL), pBack(NULL) { isSolid = false; solidBrush = NULL; }
   ~EditBSPNode();

   //-------------------------------------- BSP/Portal/Zone creation/manipulation
  public:
   void createBSP(PlaneType _planeType);
   void createVisLinks();
   void zoneFlood();
   void floodZone(S32 markZoneId);
   void gatherPortals();

   S32 findZone(const Point3D& rPoint);


   PortalEntry* mungePortalBrush(CSGBrush* pBrush);
   void         insertPortalEntry(PortalEntry* pPortal);

   void breakWindingS(const Vector<Winding>& windings,
                        U32                 windingPlaneIndex,
                        const CSGBrush*        sourceBrush,
                        Vector<Winding>&       outputWindings);
   void breakWinding(const Vector<Winding>& windings,
                     U32                 windingPlaneIndex,
                     const CSGBrush*        sourceBrush,
                     Vector<Winding>&       outputWindings);

   //-------------------------------------- Statistics
  public:
   void gatherBSPStats(U32*   pNumLeaves,
                       U32*   pTotalLeafDepth,
                       U32*   pMaxLeafDepth,
                       U32    depth);
   void gatherVISStats(U32*   pEmptyLeaves,
                       U32*   pTotalLinks,
                       U32*   pMaxLinks);
   void removeVISInfo();
};

inline void EditBSPNode::enterLink(VisLink* pEnter)
{
   visLinks.push_back(pEnter);
}

//------------------------------------------------------------------------------
class NodeArena {
  public:
   Vector<EditBSPNode*>  mBuffers;

   U32   arenaSize;
   EditBSPNode* currBuffer;
   U32   currPosition;

  public:
   NodeArena(U32 _arenaSize);
   ~NodeArena();

   EditBSPNode* allocateNode(EditBSPNode* pParent);
};


inline EditBSPNode* NodeArena::allocateNode(EditBSPNode* _pParent)
{
   static U32 sNodeIdAlloc = 0;

   AssertFatal(currPosition <= arenaSize, "How did this happen?  Arenasize is absolute!");

   if (currPosition == arenaSize) {
      // Need to allocate a new buffer...
      currBuffer   = new EditBSPNode[arenaSize];
      currPosition = 0;
      mBuffers.push_back(currBuffer);
   }

   EditBSPNode* pRet = &currBuffer[currPosition++];

   pRet->pParent = _pParent;
   pRet->zoneId  = -1;
   pRet->nodeId  = sNodeIdAlloc++;
   return pRet;
}

class VisLinkArena {
   Vector<EditBSPNode::VisLink*> mBuffers;
   U32           arenaSize;

   EditBSPNode::VisLink*         pCurrent;

  public:
   VisLinkArena(U32 _arenaSize);
   ~VisLinkArena();

   EditBSPNode::VisLink* allocateLink();
   void                  releaseLink(EditBSPNode::VisLink*);

   void                  clearAll();
};

#endif //_BSPNODE_H_

