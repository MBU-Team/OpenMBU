//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/bitVector.h"
#include "map2dif/bspNode.h"
#include "map2dif/csgBrush.h"
#include "math/mRandom.h"

namespace {

void flushedOutput(const char* string)
{
   dPrintf(string);
   dFflushStdout();
}

}

//------------------------------------------------------------------------------
//-------------------------------------- EditBSPNode functions
//------------------------------------------------------------------------------
EditBSPNode::~EditBSPNode()
{
   for (U32 i = 0; i < portals.size(); i++) {
      delete portals[i];
      portals[i] = NULL;
   }

   if (solidBrush)
      gBrushArena.freeBrush(solidBrush);

   // We do NOT release our links.  they're doubled up, and we
   //  don't know if the bspnode on the other side is valid. let
   //  the arena remove them...
   // We do need to delete brushes tho
   for (U32 i = 0; i < brushList.size(); i++)
      gBrushArena.freeBrush(brushList[i]);
}


//------------------------------------------------------------------------------
void EditBSPNode::createBSP(PlaneType _planeType)
{
   if (brushList.size() != 0 && planeEQIndex != -1) {
      AssertFatal(_planeType != planeType,
                  "Error, must be a different phase of insertion for us to be here...");

      splitBrushList();
      pFront->splitBrushList();
      pBack->splitBrushList();
   }

   if (brushList.size() == 0) {
      if (planeEQIndex != -1) {
         pFront->createBSP(_planeType);
         pBack->createBSP(_planeType);
      } else {
         planeType = _planeType;
      }
      return;
   }
   AssertFatal(planeEQIndex == -1, "Must be a leaf if we are here...");

   U32 splitPlane = selectBestPlane();
   if (splitPlane == U32(-1)) {
      // We're done.  no more planes to insert...
      //
      if (brushList.size() > 1) {
         for (U32 i = 1; i < brushList.size(); i++) {
            AssertFatal(brushList[0]->isEquivalent(*brushList[i]), "Error, non-equivalent brushes in leaf!");
         }

         U32 maxIndex = 0;
         U32 maxVal   = brushList[0]->brushId;
         for (U32 i = 1; i < brushList.size(); i++) {
            if (brushList[i]->brushId > maxVal) {
               maxVal   = brushList[i]->brushId;
               maxIndex = i;
            }
         }
         CSGBrush* pKeepBrush = brushList[maxIndex];
         brushList[maxIndex] = NULL;

         for (U32 i = 0; i < brushList.size(); i++) {
            gBrushArena.freeBrush(brushList[i]);
         }

         brushList.setSize(1);
         brushList[0] = pKeepBrush;
      }

      return;
   }
   AssertFatal(planeInParents(splitPlane) == false, "error, inserting a plane that's in a parent list...");

   planeEQIndex = splitPlane;
   planeType    = _planeType;

   pFront = gWorkingGeometry->mNodeArena.allocateNode(this);
   pBack  = gWorkingGeometry->mNodeArena.allocateNode(this);
   pFront->isSolid = isSolid;
   pBack->isSolid  = isSolid;
   pFront->zoneId  = zoneId;
   pBack->zoneId   = zoneId;

   splitBrushList();
   pFront->createBSP(_planeType);
   pBack->createBSP(_planeType);
}


//------------------------------------------------------------------------------
void EditBSPNode::zoneFlood()
{
   // All we have to do is find an unflooded (empty) leaf, flood it, then continue until
   //  we're out...
   if (planeEQIndex == -1) {
      if (isSolid == true)
         return;

      // Do the flood fill here...
      if (zoneId == -1) {
         // Need to create the zone in the working geometry
         //
         gWorkingGeometry->mZones.push_back(new EditGeometry::Zone);
         S32 markZoneId = (gWorkingGeometry->mZones.size() - 1);
         gWorkingGeometry->mZones.last()->zoneId = markZoneId;
         floodZone(markZoneId);
      }
      return;
   }

   pFront->zoneFlood();
   pBack->zoneFlood();
}

S32 EditBSPNode::findZone(const Point3D& rPoint)
{
   if (planeEQIndex == -1 || zoneId != -1) {
      return zoneId;
   }

   PlaneSide side = gWorkingGeometry->getPlaneEQ(planeEQIndex).whichSide(rPoint);

   if (side == PlaneFront) {
      return pFront->findZone(rPoint);
   } else if (side == PlaneBack) {
      return pBack->findZone(rPoint);
   } else {
      if (pFront->isSolid == false) {
         return pFront->findZone(rPoint);
      } else if (pBack->isSolid == false) {
         return pBack->findZone(rPoint);
      } else {
         return -1;
      }
   }
}


//------------------------------------------------------------------------------
void EditBSPNode::gatherPortals()
{
   if (planeEQIndex == -1) {
      if (isSolid) {
         AssertFatal(portals.size() == 0, "error, no portals should be in a solid leaf!");
         return;
      }

      if (portals.size() == 0)
         return;

      // leaf
      for (U32 i = 0; i < portals.size(); i++) {
         const PortalEntry* pEntry = portals[i];

         // First, check if there is a vislink associated with this portal...
         const VisLink* associatedLink = NULL;
         for (U32 i = 0; i < visLinks.size(); i++) {
            if (U32(visLinks[i]->portalId) == U32(pEntry->portalId)) {
               associatedLink = visLinks[i];
               break;
            }
         }

         // Next, insert the appropriate information into the portal entry in the
         //  working geometry
         EditGeometry::Portal* pGeomPortal = gWorkingGeometry->mPortals[pEntry->portalId];
         if (pGeomPortal->planeEQIndex == -1) {
            pGeomPortal->planeEQIndex = pEntry->planeEQIndex;
         } else {
            AssertFatal(U32(pGeomPortal->planeEQIndex) == U32(pEntry->planeEQIndex), "Error, mismatched portal planes!");
         }

         if (associatedLink != NULL) {
            // Put the front/back info in place...
            EditBSPNode* actualFront;
            EditBSPNode* actualBack;
            if (pEntry->planeEQIndex == associatedLink->planeEQIndex) {
               actualFront = associatedLink->pFront;
               actualBack  = associatedLink->pBack;
            } else {
               AssertFatal(pEntry->planeEQIndex == gWorkingGeometry->getPlaneInverse(associatedLink->planeEQIndex), "Error, wtf!");

               actualFront = associatedLink->pBack;
               actualBack  = associatedLink->pFront;
            }

            if (pGeomPortal->frontZone == -1) {
               pGeomPortal->frontZone = actualFront->zoneId;
               pGeomPortal->backZone  = actualBack->zoneId;
            } else if (pGeomPortal->frontZone != actualFront->zoneId ||
                       pGeomPortal->backZone  != actualBack->zoneId) {

               gWorkingGeometry->mPortals.push_back(new EditGeometry::Portal);
               pGeomPortal = gWorkingGeometry->mPortals.last();
               pGeomPortal->portalId     = gWorkingGeometry->mPortals.size() - 1;
               pGeomPortal->planeEQIndex = pEntry->planeEQIndex;
               pGeomPortal->frontZone    = actualFront->zoneId;
               pGeomPortal->backZone     = actualBack->zoneId;

               const_cast<PortalEntry*>(pEntry)->portalId = pGeomPortal->portalId;
            }
         }

         // Regardless of whether or not we've set the zones, we need to insert the winding
         //  but only if it's unique.  Remember that we'll encounter each portal twice...
         AssertFatal(pEntry->windings.size() == 1, "error, should only be one winding on the portal at this point...");
         bool insert = true;
         for (U32 i = 0; i < pGeomPortal->windings.size(); i++) {
            if (windingsEquivalent(*pEntry->windings[0], pGeomPortal->windings[i]) == true) {
               insert = false;
               break;
            }
         }

         if (insert == true)
            pGeomPortal->windings.push_back(*pEntry->windings[0]);
      }

      return;
   }

   // Non-leaf, continue...

   AssertFatal(portals.size() == 0, "Error, shouldn't have portals here...");

   pFront->gatherPortals();
   pBack->gatherPortals();
}


//------------------------------------------------------------------------------
void EditBSPNode::floodZone(S32 markZoneId)
{
   AssertFatal(planeEQIndex == -1, "Must be leaf, wtf!");
   AssertFatal(zoneId == -1, "error, overwriting zone id, wtf!");

   zoneId = markZoneId;

   for (U32 i = 0; i < visLinks.size(); i++) {
      VisLink* pLink = visLinks[i];

      // Don't traverse through any portal links here...
      if (pLink->portalId != -1)
         continue;

      EditBSPNode* pOtherNode = NULL;
      if (pLink->pFront == this)
         pOtherNode = pLink->pBack;
      else if (pLink->pBack == this)
         pOtherNode = pLink->pFront;
      else
         AssertFatal(false, "Error, Armageddon will shortly occur.  Rip in the fabric of space-time.  Talk to DMoore");

      if (pOtherNode->zoneId == -1)
         pOtherNode->floodZone(markZoneId);
      else
         AssertFatal(pOtherNode->zoneId == markZoneId, "Hmm.  That's bad.  shouldn't have adjacent leaves with non -1 differing zoneIds");
   }
}

//------------------------------------------------------------------------------
void EditBSPNode::createVisLinks()
{
   // First if we are a leaf, we can delete any links that bound on solid nodes,
   //  and return...
   if (planeEQIndex == -1) {

      if( true == gWorkingGeometry->mGenerateGraph ){
         gWorkingGeometry->gatherLinksForGraph(this);
      }

      if (isSolid == true){
         eraseReferencingLinks();
      }

      // Ok, now we need to determine for all links, if any correspond to a
      //  portal.
      for (U32 i = 0; i < portals.size(); i++) {
         const PortalEntry* pEntry = portals[i];
         AssertISV(pEntry->windings.size() == 1, avar("Error, bad assumption (pEntry->windings.size() == %d, not 1)  Go smack DMoore", pEntry->windings.size()));

         for (U32 j = 0; j < visLinks.size(); j++) {
            if (windingsEquivalent(visLinks[j]->winding, *pEntry->windings[0])) {
               AssertFatal(visLinks[j]->portalId == -1 || U32(visLinks[j]->portalId) == pEntry->portalId,
                           "Error, different portals corresponding to the same link?");
               visLinks[j]->portalId = pEntry->portalId;
               break;
            }
         }
      }

      return;
   }

   // If not, we need to create a planeWinding for our plane.  This will be
   //  clipped to all the bounding faces of the gWorkingGeometry, and all
   //  the planes of the node's parents.
   //
   VisLink* pVisLink      = gWorkingGeometry->mVisLinkArena.allocateLink();
   pVisLink->planeEQIndex = planeEQIndex;
   pVisLink->pFront       = pFront;
   pVisLink->pBack        = pBack;

   createBoundedWinding(gWorkingGeometry->getMinBound(),
                        gWorkingGeometry->getMaxBound(),
                        planeEQIndex,
                        &pVisLink->winding);

   // Now, clip to parents...
   EditBSPNode* pParProbePrev = this;
   EditBSPNode* pParProbe     = pParent;
   while (pParProbe != NULL) {
      if (pParProbe->pFront == pParProbePrev)
         clipWindingToPlaneFront(&pVisLink->winding, pParProbe->planeEQIndex);
      else
         clipWindingToPlaneFront(&pVisLink->winding, gWorkingGeometry->getPlaneInverse(pParProbe->planeEQIndex));

      AssertFatal(pVisLink->winding.numIndices != 0, "This should never happen: 0 verts in a vis link?");

      pParProbePrev = pParProbe;
      pParProbe     = pParProbe->pParent;
   }

   // Ok, we now have a shiny new vis link that represents us
   // Now, we split our _current_ list, and push our links down to our children.
   //
   splitVisLinks();

   // Now we push the "us" link into front and back, and descend...
   //  Note that we want to shove this back ourselves...
   pFront->visLinks.push_back(pVisLink);
   pBack->visLinks.push_back(pVisLink);

   splitPortalEntries();

   pBack->createVisLinks();
   pFront->createVisLinks();
}


//------------------------------------------------------------------------------
void EditBSPNode::createPortalWindings(const PortalEntry& rBaseWindings,
                                       PortalEntry*       pFinalEntry)
{
   if (planeEQIndex == -1) {
      // Leaf.  Keep or toss winding based on solid flag.
      //
      if (isSolid == true)
         return;

      for (U32 i = 0; i < rBaseWindings.windings.size(); i++) {
         pFinalEntry->windings.push_back(new Winding);
         *(pFinalEntry->windings.last()) = *(rBaseWindings.windings[i]);
      }
      return;
   }

   if (gWorkingGeometry->isCoplanar(planeEQIndex, rBaseWindings.planeEQIndex)) {
      // Coplanar.  Have to handle this specially.
      //
      PortalEntry frontClipped;
      frontClipped.planeEQIndex = rBaseWindings.planeEQIndex;

      pFront->createPortalWindings(rBaseWindings,
                                   &frontClipped);

      // frontClipped now becomes the base windings that are submitted to
      //  the back side.
      //
      pBack->createPortalWindings(frontClipped, pFinalEntry);
   } else {
      // Split the windings, and pass them to children...
      //
      PortalEntry fCopy, bCopy;
      fCopy.planeEQIndex = bCopy.planeEQIndex = rBaseWindings.planeEQIndex;

      for (U32 i = 0; i < rBaseWindings.windings.size(); i++) {
         Winding* fWinding = new Winding;
         Winding* bWinding = new Winding;
         *fWinding = *(rBaseWindings.windings[i]);
         *bWinding = *(rBaseWindings.windings[i]);

         clipWindingToPlaneFront(fWinding, planeEQIndex);
         clipWindingToPlaneFront(bWinding, gWorkingGeometry->getPlaneInverse(planeEQIndex));
         AssertFatal(fWinding->numIndices != 0 || bWinding->numIndices != 0,
                     "Bad clipping.  Should never happen unless coplanar.");

         if (fWinding->numIndices != 0)
            fCopy.windings.push_back(fWinding);
         if (bWinding->numIndices != 0)
            bCopy.windings.push_back(bWinding);

         AssertFatal(fWinding->numIndices != 0 || bWinding->numIndices != 0,
                     "EditBSPNode::createPortalWindings: can only happen if coplanar, and then we shouldn't be here!");
         if (fWinding->numIndices == 0)
            delete fWinding;
         if (bWinding->numIndices == 0)
            delete bWinding;
      }

      if (fCopy.windings.size() != 0)
         pFront->createPortalWindings(fCopy, pFinalEntry);
      if (bCopy.windings.size() != 0)
         pBack->createPortalWindings(bCopy, pFinalEntry);
   }
}


//------------------------------------------------------------------------------
bool EditBSPNode::planeInParents(U32 testPlane)
{
   const EditBSPNode* pProbe = pParent;
   while (pProbe != NULL) {
      AssertFatal(pProbe->planeEQIndex != -1, "Huh");

      if (U32(pProbe->planeEQIndex) == testPlane ||
          U32(pProbe->planeEQIndex) == gWorkingGeometry->getPlaneInverse(testPlane))
         return true;

      pProbe = pProbe->pParent;
   }

   return false;
}


S32 EditBSPNode::calcPlaneRating(U32 testPlane)
{
   // Things that we don't like:
   //    - Splitting of many planes
   //    - Creation of tiny windings
   //    - Imbalance of planes on one side or the other
   // Things that we do like:
   //    - Planes that are coplanar with many other planes under consideration
   //    - axial planes

   AssertFatal((testPlane & 0x1) == 0, "Only test even plane nums!");

   bool  isAxial         = false;
   S32 numCoplanar     = 0;
   S32 numTinyWindings = 0;
   S32 numSplits       = 0;
   S32 numFront        = 0;
   S32 numBack         = 0;

   const PlaneEQ& rPlane = gWorkingGeometry->getPlaneEQ(testPlane);

   // Axial test
   U32 zeroCount = 0;
   zeroCount += (rPlane.normal.x == 0) ? 1 : 0;
   zeroCount += (rPlane.normal.y == 0) ? 1 : 0;
   zeroCount += (rPlane.normal.z == 0) ? 1 : 0;
   AssertFatal(zeroCount != 3, "Huh?  This is a really bad plane");
   if (zeroCount == 2)
      isAxial = true;

   // Count splits, sides, etc.
   for (U32 i = 0; i < brushList.size(); i++) {
      assessPlane(testPlane,
                  *brushList[i],
                  &numCoplanar,
                  &numTinyWindings,
                  &numSplits,
                  &numFront,
                  &numBack);
   }

   // Ok.  Now, I'm blatantly copying this from the qbsp3 source.  Their hueristic:
   //  score =  5    * coplanar            +
   //          -5    * splits              +
   //          -       abs(numFront - numBack) +
   //          -1000 * numTinyWindings  +
   //           5 *    isAxial
   // Plus some crap we don't care about.  For now, we'll just duplicate this.
   //
   S32 finalScore = 5 * numCoplanar - 5 * numSplits - mAbs(numFront - numBack);
   finalScore      -= 1000 * numTinyWindings;
   finalScore      += isAxial ? 5 : 0;

   return finalScore;
}


//------------------------------------------------------------------------------
void EditBSPNode::splitBrushList()
{
   if (planeEQIndex == -1)
      return;

   if (planeEQIndex == 630)
      U32 test = 0;

   // Ok, Ok, we have to do actual work here.
   for (S32 i = S32(brushList.size()) - 1; i >= 0 ; i--) {
      CSGBrush* pBrush = brushList[i];
      brushList.erase(i);

      CSGBrush* pFrontBrush = NULL;
      CSGBrush* pBackBrush = NULL;
      splitBrush(pBrush, planeEQIndex, pFrontBrush, pBackBrush);

      if (pFrontBrush != NULL) {
         for (U32 j = 0; j < pFrontBrush->mPlanes.size(); j++) {
            if (gWorkingGeometry->isCoplanar(pFrontBrush->mPlanes[j].planeEQIndex, planeEQIndex)) {
               AssertFatal(pFrontBrush->mPlanes[j].isInserted() == false, "Error, bad assumption somewhere in splitBrushList");
               pFrontBrush->mPlanes[j].markInserted();

               if (pFrontBrush->noMoreInsertables() == true) {
                  pFront->isSolid = true;

                  if (pFront->solidBrush == NULL) {
                     AssertFatal(pFront->solidBrush == NULL, "hmm");
                     pFront->solidBrush = pFrontBrush;
                     pFrontBrush        = NULL;
                     break;
                  } else {
                     gBrushArena.freeBrush(pFrontBrush);
                     pFrontBrush = NULL;
                     break;
                  }
               }
            }
         }

         if (pFrontBrush) {
            if (pFront->isSolid == false) {
               pFront->brushList.push_back(pFrontBrush);
            }
            else {
               // This next condition is a little wierd, so I'm separating it out.
               //  If the solid Brush is null, the brush has passed the test on
               //  a brush that knows better (i.e, our parent), and we can insert
               //  it.
               if (pFront->solidBrush == NULL ||
                   pFront->solidBrush->mBrushType >= pFrontBrush->mBrushType) {
                  pFront->brushList.push_back(pFrontBrush);
               } else {
                  gBrushArena.freeBrush(pFrontBrush);
                  pFrontBrush = NULL;
               }
            }
         }
      }
      if (pBackBrush != NULL) {
         for (U32 j = 0; j < pBackBrush->mPlanes.size(); j++) {
            if (gWorkingGeometry->isCoplanar(pBackBrush->mPlanes[j].planeEQIndex, planeEQIndex)) {
               AssertFatal(pBackBrush->mPlanes[j].isInserted() == false, "Error, bad assumption somewhere in splitBrushList");
               pBackBrush->mPlanes[j].markInserted();

               if (pBackBrush->noMoreInsertables() == true) {
                  pBack->isSolid = true;

                  if (pBack->solidBrush == NULL) {
                     AssertFatal(pBack->solidBrush == NULL, "hmm");
                     pBack->solidBrush = pBackBrush;
                     pBackBrush        = NULL;
                     break;
                  } else {
                     gBrushArena.freeBrush(pBackBrush);
                     pBackBrush = NULL;
                     break;
                  }
               }
            }
         }

         if (pBackBrush) {
            if (pBack->isSolid == false) {
               pBack->brushList.push_back(pBackBrush);
            }
            else {
               // This next condition is a little wierd, so I'm separating it out.
               //  If the solid Brush is null, the brush has passed the test on
               //  a brush that knows better (i.e, our parent), and we can insert
               //  it.
               if (pBack->solidBrush == NULL ||
                   pBack->solidBrush->mBrushType >= pBackBrush->mBrushType) {
                  pBack->brushList.push_back(pBackBrush);
               } else {
                  gBrushArena.freeBrush(pBackBrush);
                  pBackBrush = NULL;
               }
            }
         }
      }

      gBrushArena.freeBrush(pBrush);
   }
   brushList.compact();

   pFront->splitBrushList();
   pBack->splitBrushList();
}

//------------------------------------------------------------------------------
void EditBSPNode::splitVisLinks()
{
   AssertFatal(pFront->visLinks.size() == 0 && pBack->visLinks.size() == 0, "Error, children may not have visLinks at this point!");
   for (S32 i = S32(visLinks.size()) - 1; i >= 0; i--) {
      VisLink* pSplit = visLinks[i];
      visLinks[i] = NULL;
      visLinks.erase(i);

      if (pSplit->intersect(planeEQIndex) == true) {
         // Need to split this link.
         VisLink* pOther      = gWorkingGeometry->mVisLinkArena.allocateLink();
         pOther->planeEQIndex = pSplit->planeEQIndex;
         pOther->pFront       = pSplit->pFront;
         pOther->pBack        = pSplit->pBack;
         pOther->winding      = pSplit->winding;

         clipWindingToPlaneFront(&pSplit->winding, planeEQIndex);
         clipWindingToPlaneFront(&pOther->winding, gWorkingGeometry->getPlaneInverse(planeEQIndex));
         AssertFatal(pSplit->winding.numIndices != 0 && pOther->winding.numIndices != 0,
                     "Intersections must result in two non-zero windings!");

         if (pSplit->pFront == this) {
            AssertFatal(pOther->pFront == this, "Error in vissplit.  Back does not point to proper node");

            pSplit->pFront = pFront;
            pFront->enterLink(pSplit);

            pOther->pBack->visLinks.push_back(pOther);
            pOther->pFront = pBack;
            pBack->enterLink(pOther);
         }
         else if (pSplit->pBack == this) {
            AssertFatal(pOther->pBack == this, "Error in vissplit.  Back does not point to proper node");

            pSplit->pBack = pFront;
            pFront->enterLink(pSplit);

            pOther->pFront->visLinks.push_back(pOther);
            pOther->pBack = pBack;
            pBack->enterLink(pOther);
         }
         else {
            AssertFatal(false, "Error in vissplit.  Does not point to proper nodes");
         }
      } else {
         // Just find out what side to put it in...
         PlaneSide side = pSplit->whichSide(planeEQIndex);

         EditBSPNode* insertNode;
         if (side == PlaneFront) {
            insertNode = pFront;
         }
         else if (side == PlaneBack) {
            insertNode = pBack;
         }
         else {
            AssertISV(false, "Vislink cannot be coplanar with a node!");
            insertNode = NULL;
         }

         if (pSplit->pFront == this)
            pSplit->pFront = insertNode;
         else if (pSplit->pBack == this)
            pSplit->pBack = insertNode;
         else
            AssertISV(false, "Error in vissplit.  Does not point to proper nodes");

         insertNode->enterLink(pSplit);
      }
   }
}


//------------------------------------------------------------------------------
void EditBSPNode::splitPortalEntries()
{
   AssertFatal(planeEQIndex != -1 && pFront && pBack,
               "Shouldn't be here unless we're a node");


   for (S32 i = S32(portals.size()) - 1; i >= 0; i--) {
      // Now, these windings should be all on one side or the other.
      //  it is an error to split portal entries at this point...
      //
      PortalEntry* pEntry = portals[i];
      portals.erase(i);


      if (gWorkingGeometry->isCoplanar(pEntry->planeEQIndex, planeEQIndex) == false) {
         PortalEntry* pFrontCopy = new PortalEntry;
         PortalEntry* pBackCopy  = new PortalEntry;
         pFrontCopy->copyEntry(*pEntry);
         pBackCopy->copyEntry(*pEntry);

         PortalEntry* pInsertFront = NULL;
         PortalEntry* pInsertBack = NULL;

         for (S32 i = S32(pEntry->windings.size()) - 1; i >= 0; i--) {
            U32 side = windingWhichSide(*pEntry->windings[i], pEntry->planeEQIndex, planeEQIndex);
            AssertFatal(side != PlaneSpecialOn, "Error, should only happen on coplanar portal");

            if (side & PlaneSpecialFront) {
               // Gotta put it into the front node
               pInsertFront = pFrontCopy;

               // Gotta clip this winding?
               if (side & PlaneSpecialBack)
                  clipWindingToPlaneFront(pFrontCopy->windings[i], planeEQIndex);
            }
            else {
               // Delete this winding from the front
               delete pFrontCopy->windings[i];
               pFrontCopy->windings.erase(i);
            }

            if (side & PlaneSpecialBack) {
               pInsertBack = pBackCopy;

               if (side & PlaneSpecialFront)
                  clipWindingToPlaneFront(pBackCopy->windings[i], planeEQIndex);
            }
            else {
               delete pBackCopy->windings[i];
               pBackCopy->windings.erase(i);
            }
         }

         // Ok, we only insert the copies if at least one winding wound up in the
         //  appropriate child
         //
         if (pInsertFront != NULL)
            pFront->portals.push_back(pInsertFront);
         else
            delete pFrontCopy;

         if (pInsertBack != NULL)
            pBack->portals.push_back(pInsertBack);
         else
            delete pBackCopy;
      }
      else {
         // Portal is coplanar to node plane, push into both children...
         PortalEntry* pFrontCopy = new PortalEntry;
         PortalEntry* pBackCopy  = new PortalEntry;
         pFrontCopy->copyEntry(*pEntry);
         pBackCopy->copyEntry(*pEntry);
         pFront->portals.push_back(pFrontCopy);
         pBack->portals.push_back(pBackCopy);
      }

      delete pEntry;
   }
}


//------------------------------------------------------------------------------
U32 EditBSPNode::selectBestPlane()
{
   // Shuffle the brush indices...
   static MRandomLCG sRand;
   static Vector<U32> shuffledIndices;
   shuffledIndices.setSize(brushList.size());
   for (U32 i = 0; i < shuffledIndices.size(); i++)
      shuffledIndices[i] = i;

   for (S32 i = shuffledIndices.size() - 1; i > 0; i--) {
      U32 k = mFloor(sRand.randF() * F32(i)) + 1;
      U32 temp = shuffledIndices[i];
      shuffledIndices[i] = shuffledIndices[k];
      shuffledIndices[k] = temp;
   }

   BitVector uniqueSet(gWorkingGeometry->mPlaneEQs.size() / 2);
   uniqueSet.clear();

   extern U32 gMaxPlanesConsidered;
   U32 planeCount = 0;
   for (U32 i = 0; i < brushList.size() && planeCount < gMaxPlanesConsidered; i++) {
      CSGBrush* pBrush = brushList[shuffledIndices[i]];
      for (U32 j = 0; j < pBrush->mPlanes.size() && planeCount < gMaxPlanesConsidered; j++) {
         if (pBrush->mPlanes[j].isInserted())
            continue;

         if (uniqueSet.test(pBrush->mPlanes[j].planeEQIndex >> 1) == false)
            planeCount++;
         uniqueSet.set(pBrush->mPlanes[j].planeEQIndex >> 1);
      }
   }

   Vector<U32> uniquePlanes;
   uniquePlanes.reserve(planeCount);
   for (U32 i = 0; i < gWorkingGeometry->mPlaneEQs.size() / 2; i++) {
      if (uniqueSet.test(i))
         uniquePlanes.push_back(i << 1);
   }

   if (uniquePlanes.size() == 0)
      return U32(-1);

   S32 maxRating = -(1 << 30);
   S32 maxIndex  = -1;
   for (U32 i = 0; i < uniquePlanes.size(); i++) {
      S32 currRating = calcPlaneRating(uniquePlanes[i]);
      if (currRating > maxRating) {
         maxRating = currRating;
         maxIndex  = i;
      }
   }
   AssertFatal(maxIndex != -1, "Error, no index selected?");

   // Note that while we have rated only the front sides, we insert the planes
   //  as they are found...
   //
   return uniquePlanes[maxIndex];
}

//------------------------------------------------------------------------------
EditBSPNode::PortalEntry* EditBSPNode::mungePortalBrush(CSGBrush* pBrush)
{
   // We first need to determine which plane we are going to use.  This is
   //  determined by finding the plane with the winding that has the most
   //  surface area, before clipping.
   //
   F64 maxArea = -1;
   S32 maxIndex = -1;

   for (U32 i = 0; i < pBrush->mPlanes.size(); i++) {
      CSGPlane& rPlane = pBrush->mPlanes[i];

      F64 area = getWindingSurfaceArea(rPlane.winding, rPlane.planeEQIndex);
      AssertFatal(area >= 0.0, "Error, negative area returned for winding...");
      if (area > maxArea) {
         maxArea  = area;
         maxIndex = i;
      }
   }
   AssertFatal(maxIndex != -1, "error, no portal plane found to insert?");

   // Ok, we have our plane.  Toss out all planes on the brush but the selected
   //  one, and insert it in the BSP.
   //
   CSGPlane& rPlane = pBrush->mPlanes[maxIndex];
   Point3D ortho1;
   Point3D ortho2;
   bool found1 = false;
   for (U32 i = 0; i < pBrush->mPlanes.size(); i++) {
      if (i == maxIndex)
         continue;

      CSGPlane& rPoss = pBrush->mPlanes[i];
      if (mFabs(mDot(gWorkingGeometry->getPlaneEQ(rPoss.planeEQIndex).normal,
                     gWorkingGeometry->getPlaneEQ(rPlane.planeEQIndex).normal)) < 0.0000001) {
         ortho1 = gWorkingGeometry->getPlaneEQ(rPoss.planeEQIndex).normal;
         found1 = true;
         break;
      }
   }
   AssertFatal(found1, "Error, portal brush is likely not a cube!");
   mCross(ortho1, gWorkingGeometry->getPlaneEQ(rPlane.planeEQIndex).normal, &ortho2);
   ortho2.normalize();
   AssertFatal(mDot(ortho1, gWorkingGeometry->getPlaneEQ(rPlane.planeEQIndex).normal) < 0.0000001, "Bad normal vectors0 for portal");
   AssertFatal(mDot(ortho2, gWorkingGeometry->getPlaneEQ(rPlane.planeEQIndex).normal) < 0.0000001, "Bad normal vectors1 for portal");
   AssertFatal(mDot(ortho1, ortho2)        < 0.0000001, "Bad normal vectors2 for portal");

   if (maxIndex != 0)
      pBrush->mPlanes[0] = pBrush->mPlanes[maxIndex];
   pBrush->mPlanes.setSize(1);

   // First, create the portal windings...
   //
   PortalEntry baseWindings;
   baseWindings.windings.push_back(new Winding);
   *(baseWindings.windings[0]) = pBrush->mPlanes[0].winding;
   baseWindings.planeEQIndex   = pBrush->mPlanes[0].planeEQIndex;

   PortalEntry* pFinalWindings = new PortalEntry;
   pFinalWindings->planeEQIndex = pBrush->mPlanes[0].planeEQIndex;
   createPortalWindings(baseWindings, pFinalWindings);

   pFinalWindings->ortho1 = ortho1;
   pFinalWindings->ortho2 = ortho2;

   if (pFinalWindings->windings.size() != 0) {
      return pFinalWindings;
   } else {
      delete pFinalWindings;
      return NULL;
   }
}


//------------------------------------------------------------------------------
void EditBSPNode::insertPortalEntry(PortalEntry* pPortal)
{
   // This is just a straight plane insert, with one difference.  If we escape out
   //  on a co-planar planeEQ, we need to mark the node as a portal containing node,
   //  and place the portal windings in it.
   //
   if (planeEQIndex == -1) {
      // We can't be in a solid node because of the way that we clipped the portal
      //  windings.
      AssertFatal(pFront == NULL && pBack == NULL,
                  "EditBSPNode::insertPortalBrush: front/back must be NULL if planeEQ == -1");
      AssertFatal(isSolid == false, "Must insert into an empty node");
//      AssertISV(planeType == EditBSPNode::Structural,
//                "Error, portal breaking a portal leaf.  Bad.  "
//                "Probably a floating portal brush somewhere.  Talk to DMoore for more info");

      // A portal plane insertion into a leaf node creates two empty nodes on either
      //  side.
      planeEQIndex = pPortal->planeEQIndex;
      planeType    = EditBSPNode::Portal;

      pFront            = gWorkingGeometry->mNodeArena.allocateNode(this);
      pBack             = gWorkingGeometry->mNodeArena.allocateNode(this);
      pFront->planeType = EditBSPNode::Portal;
      pBack->planeType  = EditBSPNode::Portal;
      pFront->isSolid = pBack->isSolid = false;

      // We need to note that we're a portal, and enter the PortalEntry into our
      //  list...
      AssertISV(pPortal->windings.size() == 1, avar("Error, bad assumption (pEntry->windings.size() == %d, not 1)  Go smack DMoore", pPortal->windings.size()));
      portals.push_back(pPortal);

      return;
   }

   if (gWorkingGeometry->isCoplanar(planeEQIndex, pPortal->planeEQIndex)) {
      // Coplanar to this node.  Mark the node as a portal node, and insert the
      //  portal windings into the nodes list.  Make sure we can merge any superfluous
      //  windings...
      //
      if (pPortal->windings.size() > 1) {
         for (U32 i = 0; i < pPortal->windings.size(); i++)
            pPortal->windings[i]->numNodes = 0;

         bool collapsed = true;
         while (collapsed == true) {
            collapsed = false;
            for (U32 i = 0; i < pPortal->windings.size() && !collapsed; i++) {
               for (U32 j = i + 1; j < pPortal->windings.size() && !collapsed; j++) {
                  if (collapseWindings(*pPortal->windings[i], *pPortal->windings[j]) == true) {
                     collapsed = true;
                     delete pPortal->windings[j];
                     pPortal->windings.erase(j);
                  }
               }
            }
         }
      }
      AssertISV(pPortal->windings.size() == 1, avar("Error, bad assumption (pEntry->windings.size() == %d, not 1)  Go smack DMoore (portal %d)", pPortal->windings.size(), pPortal->portalId));

      portals.push_back(pPortal);
   } else {
      // Gotta split.  Make a copy, and split against the front and the back
      //  of our plane.
      //
      PortalEntry* backPortals = new PortalEntry;
      backPortals->planeEQIndex = pPortal->planeEQIndex;
      backPortals->portalId     = pPortal->portalId;

      for (U32 i = 0; i < pPortal->windings.size(); i++) {
         backPortals->windings.push_back(new Winding);
         *(backPortals->windings[i]) = *(pPortal->windings[i]);
      }

      for (U32 i = 0; i < pPortal->windings.size(); i++) {
         clipWindingToPlaneFront(pPortal->windings[i], planeEQIndex);
         clipWindingToPlaneFront(backPortals->windings[i], gWorkingGeometry->getPlaneInverse(planeEQIndex));
         AssertFatal(pPortal->windings[i]->numIndices + backPortals->windings[i]->numIndices != 0,
                     "Error, bad clip.  should only happen if coplanar");
      }

      for (S32 i = S32(pPortal->windings.size() - 1); i >= 0; i--) {
         if (pPortal->windings[i]->numIndices == 0) {
            delete pPortal->windings[i];
            pPortal->windings.erase(i);
         }
         if (backPortals->windings[i]->numIndices == 0) {
            delete backPortals->windings[i];
            backPortals->windings.erase(i);
         }
      }

      if (pPortal->windings.size() != 0)
         pFront->insertPortalEntry(pPortal);
      else
         delete pPortal;

      if (backPortals->windings.size() != 0)
         pBack->insertPortalEntry(backPortals);
      else
         delete backPortals;
   }
}


//------------------------------------------------------------------------------
void EditBSPNode::eraseReferencingLinks()
{
   for (S32 i = S32(visLinks.size()) - 1; i >= 0; i--) {
      VisLink* pRemove = visLinks[i];
      visLinks[i] = NULL;
      visLinks.erase(i);

      AssertFatal(pRemove->portalId == -1, "Error, removing a link associated with a portal!");

      EditBSPNode* pOtherNode;
      if (pRemove->pFront == this)
         pOtherNode = pRemove->pBack;
      else if (pRemove->pBack == this)
         pOtherNode = pRemove->pFront;
      else {
         AssertFatal(false, "Vislink points to wrong nodes!");
         pOtherNode = NULL;
      }

      bool found = false;
      for (U32 i = 0; i < pOtherNode->visLinks.size(); i++) {
         if (pOtherNode->visLinks[i] == pRemove) {
            found = true;
            pOtherNode->visLinks.erase(i);
            break;
         }
      }
      AssertFatal(found == true, "Error, visLink not on other node?");

      gWorkingGeometry->mVisLinkArena.releaseLink(pRemove);
   }
}


////------------------------------------------------------------------------------
CSGBrush* EditBSPNode::findSolidBrush()
{
   if (solidBrush != NULL)
      return solidBrush;

   if (pParent != NULL)
      return pParent->findSolidBrush();
   else
      return NULL;
}

void EditBSPNode::breakWindingS(const Vector<Winding>& windings,
                                U32                    windingPlaneIndex,
                                const CSGBrush*        sourceBrush,
                                Vector<Winding>&       outputWindings)
{
   if (planeEQIndex == -1) {
      if (isSolid) {
         CSGBrush* definingSolid = findSolidBrush();
         AssertFatal(definingSolid != NULL, "Error, solid leaves must have a solid brush!");

         // Toss if we're in someone else's zone...
         if (sourceBrush->brushId != definingSolid->brushId)
            return;
      }

      // Otherwise, we keep these windings...
      for (U32 i = 0; i < windings.size(); i++) {
         outputWindings.push_back(windings[i]);

         if (isSolid == false) {
            outputWindings.last().insertZone(zoneId);
         } else {
            bool found = false;
            for (U32 i = 0; i < outputWindings.last().numNodes; i++) {
               if (outputWindings.last().solidNodes[i] == nodeId) {
                  found = true;
                  break;
               }
            }
            if (found == false) {
               AssertFatal(outputWindings.last().numNodes < MaxWindingNodes, "Error, too many solid nodes affecting poly.  Plase ask DMoore to increase the allowed number!");
               outputWindings.last().solidNodes[outputWindings.last().numNodes++] = nodeId;
            }
         }
      }

      return;
   }

   if (planeType == Detail && sourceBrush->mBrushType == StructuralBrush) {
      // Break no further, keep these windings.
      // DMM: What to do about nodeIds?
//      AssertFatal(isSolid == false, "error, shouldn't be in solid here");

      for (U32 i = 0; i < windings.size(); i++) {
         outputWindings.push_back(windings[i]);
         outputWindings.last().insertZone(zoneId);
      }

      return;
   }

   if (gWorkingGeometry->isCoplanar(planeEQIndex, windingPlaneIndex)) {
      AssertISV(planeType != Portal, "Geometry coplanar with a portal plane.  This is bad.  Talk to DMoore");
      Vector<Winding> toFront(128);
      pFront->breakWindingS(windings, windingPlaneIndex, sourceBrush, toFront);
      pBack->breakWindingS(toFront, windingPlaneIndex, sourceBrush, outputWindings);
   }
   else {
      Vector<Winding> frontCopy = windings;
      Vector<Winding> backCopy  = windings;

      for (S32 i = S32(windings.size()) - 1; i >= 0; i--) {
         clipWindingToPlaneFront(&frontCopy[i], planeEQIndex);
         clipWindingToPlaneFront(&backCopy[i], gWorkingGeometry->getPlaneInverse(planeEQIndex));

         if (frontCopy[i].numIndices == 0)
            frontCopy.erase(i);
         if (backCopy[i].numIndices == 0)
            backCopy.erase(i);
      }

      Vector<Winding> childWindings(128);
      if (frontCopy.size() != 0)
         pFront->breakWindingS(frontCopy, windingPlaneIndex, sourceBrush, childWindings);

      if (backCopy.size() != 0)
         pBack->breakWindingS(backCopy, windingPlaneIndex, sourceBrush, childWindings);

      for (U32 i = 0; i < childWindings.size(); i++) {
         outputWindings.push_back(childWindings[i]);
      }
   }

   // Here, we try to stitch these back together, and fix any colinearities...
   //
   bool collapsed = true;
   while (collapsed == true) {
      collapsed = false;
      for (U32 i = 0; i < outputWindings.size() && !collapsed; i++) {
         for (U32 j = i + 1; j < outputWindings.size() && !collapsed; j++) {
            if (collapseWindings(outputWindings[i], outputWindings[j]) == true) {
               collapsed = true;
               outputWindings.erase(j);
            }
         }
      }
   }
}

void EditBSPNode::breakWinding(const Vector<Winding>& windings,
                               U32                    windingPlaneIndex,
                               const CSGBrush*        sourceBrush,
                               Vector<Winding>&       outputWindings)
{
   breakWindingS(windings, windingPlaneIndex, sourceBrush, outputWindings);
}


//------------------------------------------------------------------------------
void EditBSPNode::gatherBSPStats(U32* pNumLeaves,
                                 U32* pTotalLeafDepth,
                                 U32* pMaxLeafDepth,
                                 U32  depth)
{
   if (planeEQIndex == -1) {
      (*pNumLeaves)++;
      (*pTotalLeafDepth) += depth;
      if (depth > *pMaxLeafDepth)
         *pMaxLeafDepth = depth;

      return;
   }

   pFront->gatherBSPStats(pNumLeaves, pTotalLeafDepth, pMaxLeafDepth, depth + 1);
   pBack->gatherBSPStats(pNumLeaves, pTotalLeafDepth, pMaxLeafDepth, depth + 1);
}


//------------------------------------------------------------------------------
void EditBSPNode::gatherVISStats(U32*   pEmptyLeaves,
                                           U32*   pTotalLinks,
                                           U32*   pMaxLinks)
{
   if (planeEQIndex == -1) {
      if (isSolid) {
         AssertFatal(portals.size() == 0 && visLinks.size() == 0,
                     "EditBSPNode::gatherVISStats: error, must not have any portals or vislinks in a solid leaf!");
         return;
      }

      (*pEmptyLeaves)++;
      (*pTotalLinks) += visLinks.size();

      if (visLinks.size() > *pMaxLinks)
         *pMaxLinks = visLinks.size();

      return;
   }

   pFront->gatherVISStats(pEmptyLeaves, pTotalLinks, pMaxLinks);
   pBack->gatherVISStats(pEmptyLeaves, pTotalLinks, pMaxLinks);
}

void EditBSPNode::removeVISInfo()
{
   if (planeEQIndex == -1) {
      if (isSolid) {
         AssertFatal(portals.size() == 0 && visLinks.size() == 0,
                     "EditBSPNode::gatherVISStats: error, must not have any portals or vislinks in a solid leaf!");
         return;
      }

      visLinks.clear();
      visLinks.compact();
      return;
   }

   pFront->removeVISInfo();
   pBack->removeVISInfo();
}

//------------------------------------------------------------------------------
//-------------------------------------- VisLink stuff
//
bool EditBSPNode::VisLink::intersect(U32 in_planeEQIndex)
{
   bool anyFront = false;
   bool anyBack  = false;

   const PlaneEQ& rPlane = gWorkingGeometry->getPlaneEQ(in_planeEQIndex);

   for (U32 i = 0; i < winding.numIndices; i++) {
      switch (rPlane.whichSide(gWorkingGeometry->getPoint(winding.indices[i]))) {
        case PlaneFront:
         anyFront = true;
         break;
        case PlaneBack:
         anyBack = true;
         break;

        case PlaneOn:
         break;

        default:
         AssertFatal(false, "Error, misunderstood plane code");
         break;
      }
   }

   return (anyFront == true && anyBack == true);
}

PlaneSide EditBSPNode::VisLink::whichSide(U32 in_planeEQIndex)
{
   bool anyFront = false;
   bool anyBack  = false;

   const PlaneEQ& rPlane = gWorkingGeometry->getPlaneEQ(in_planeEQIndex);

   for (U32 i = 0; i < winding.numIndices; i++) {
      switch (rPlane.whichSide(gWorkingGeometry->getPoint(winding.indices[i]))) {
        case PlaneFront:
         anyFront = true;
         break;
        case PlaneBack:
         anyBack = true;
         break;

        case PlaneOn:
         break;

        default:
         AssertFatal(false, "Error, misunderstood plane code");
         break;
      }
   }

   AssertFatal(anyFront == false || anyBack == false, "error, should only be called for non-intersecting visLinks...");
   if (anyFront == true)
      return PlaneFront;
   else if (anyBack == true)
      return PlaneBack;
   else
      return PlaneOn;
}


EditBSPNode::PortalEntry::~PortalEntry()
{
   for (U32 i = 0; i < windings.size(); i++) {
      delete windings[i];
      windings[i] = NULL;
   }
}

void EditBSPNode::PortalEntry::copyEntry(const PortalEntry& rPortal)
{
   AssertFatal(windings.size() == 0, "Error, must have zero windings here");

   for (U32 i = 0; i < rPortal.windings.size(); i++) {
      windings.push_back(new Winding);
      *(windings.last()) = *(rPortal.windings[i]);
   }

   planeEQIndex = rPortal.planeEQIndex;
   portalId     = rPortal.portalId;
}

//------------------------------------------------------------------------------
//-------------------------------------- ARENA FUNCTIONS
NodeArena::NodeArena(U32 _arenaSize)
{
   AssertFatal(_arenaSize > 0, "Error, impossible size");
   arenaSize = _arenaSize;

   currBuffer   = new EditBSPNode[arenaSize];
   currPosition = 0;
   mBuffers.push_back(currBuffer);
}

NodeArena::~NodeArena()
{
   arenaSize    = 0;
   currBuffer   = NULL;
   currPosition = 0;

   for (U32 i = 0; i < mBuffers.size(); i++) {
      delete [] mBuffers[i];
      mBuffers[i] = NULL;
   }
}

VisLinkArena::VisLinkArena(U32 _arenaSize)
{
   arenaSize = _arenaSize;
   pCurrent  = new EditBSPNode::VisLink[arenaSize];
   for (U32 i = 0; i < arenaSize - 1; i++)
      pCurrent[i].pNext = &pCurrent[i+1];
   pCurrent[arenaSize - 1].pNext = NULL;

   mBuffers.push_back(pCurrent);
}

VisLinkArena::~VisLinkArena()
{
   clearAll();
}

void VisLinkArena::clearAll()
{
   for (U32 i = 0; i < mBuffers.size(); i++) {
      delete [] mBuffers[i];
      mBuffers[i] = NULL;
   }

   pCurrent = NULL;
}

EditBSPNode::VisLink* VisLinkArena::allocateLink()
{
   if (pCurrent == NULL) {
      pCurrent  = new EditBSPNode::VisLink[arenaSize];
      for (U32 i = 0; i < arenaSize - 1; i++)
         pCurrent[i].pNext = &pCurrent[i+1];
      pCurrent[arenaSize - 1].pNext = NULL;
      mBuffers.push_back(pCurrent);
   }

   EditBSPNode::VisLink* pRet = pCurrent;
   pCurrent               = pCurrent->pNext;

   pRet->pNext    = NULL;
   pRet->portalId = -1;
   return pRet;
}

void VisLinkArena::releaseLink(EditBSPNode::VisLink* pRelease)
{
   AssertFatal(pRelease->pNext == NULL, "Huh?  Should never happen.  Talk to DMoore");

   pRelease->pNext = pCurrent;
   pCurrent = pRelease;
}
