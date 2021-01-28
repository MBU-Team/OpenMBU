//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

/*
   lhdo:  
   overview above the types / process for generating the floor plan resource.  
   probably axe this file and migrate methods, several of which should be on the 
      resource editing class.  
*/

#include "map2dif plus/editGeometry.h"
#include "map2dif plus/csgBrush.h"
#include "core/fileStream.h"
#include "math/mMath.h"

// (rendered TMarker list as initial test).  
#define  MARKER_LIST_TEST  0

// In addition to a flag, it looks like the graph generation will need 
// to adjust the precision variables that are used.  
void EditGeometry::setGraphGeneration(bool doGeneration, bool doExtrusion)
{
   mGenerateGraph = doGeneration;
   mPerformExtrusion = doExtrusion;
   
   if( mPerformExtrusion ){
      mHashPoints = mHashPlanes = false;
   }
   else{
      mHashPoints = mHashPlanes = true;
   }
   
   // Leaves as before:
   mHashPoints = mHashPlanes = true;
}

// Called from bottom of VisLink creating recursion
void EditGeometry::gatherLinksForGraph(EditBSPNode * pLeaf)
{
   // Just do solid-to-empty links for now.  
   if( pLeaf->isSolid )
   {
      // iterate all visLinks that go to empty, save off winding and the plane.  
      for( U32 i = 0; i < pLeaf->visLinks.size(); i++ )
      {
         Winding              *  pWinding = NULL;
         EditBSPNode::VisLink *  pLink = pLeaf->visLinks[i];
         U32                     planeEq;

         if( pLink->pBack == pLeaf )
         {
            if( ! pLink->pFront->isSolid )
            {
               pWinding = & pLink->winding;
               planeEq = pLink->planeEQIndex;
            }
         }
         else
         {
            if( ! pLink->pBack->isSolid )
            {
               pWinding = & pLink->winding;
               planeEq = getPlaneInverse( pLink->planeEQIndex );
            }
         }
         
         if( pWinding )
         {
            #if MARKER_LIST_tEST
               for( U32 j = 0; j < pWinding->numIndices; j++ )
                  mGraphSurfacePts.pushBackUnique( pWinding->indices[j] );
            #endif
               
            // Save the windings for parsing later.  
            mEditFloorPlanRes.pushArea(* pWinding, planeEq);
         }
      }
   }
}

// CS file gives list with element count first 
// $list[0] = "19";
// $list[1] = "0 0 20";
// $list[2] = "0 0 30";
// .... 
static const char formatStr0[] = "$list[0] = \"%d\";\n";
static const char formatStr1[] = "$list[%d] = \"%lf %lf %lf\";\n";

U32 EditGeometry::writeGraphInfo()
{
#if MARKER_LIST_TEST
   if( U32 numPoints = mGraphSurfacePts.size() )
   {
      FileStream  pointFile;
      char        buff[1024];
      
      if( pointFile.open("points.cs", FileStream::Write) )
      {
         pointFile.write( dSprintf(buff,sizeof(buff),formatStr0,numPoints), buff );

         for( U32 i = 0; i < numPoints; i++ )
         {
            Point3D  pt = getPoint( mGraphSurfacePts[i] );
            pt /= 32.0;
            dSprintf(buff, sizeof(buff), formatStr1, i + 1, pt.x, pt.y, pt.z);
            pointFile.write( dStrlen(buff), buff );
         }
         pointFile.close();
      }
      else
         dPrintf( "Couldn't open point file for write" );
         
      mGraphSurfacePts.clear();
   }
   else
      dPrintf( "No points were generated for graph" );
#endif

   // Test writing the resource
   mEditFloorPlanRes.constructFloorPlan();
   FileStream fws;
   fws.open("sample.flr", FileStream::Write);
   mEditFloorPlanRes.write(fws);
   fws.close();
   
   return mGraphSurfacePts.size();
}

static Point3D extrusions[3] = 
{
   Point3D( 0.5, 0.0,  0.0 ), 
   Point3D( 0.0, 0.5,  0.0 ), 
   Point3D( 0.0, 0.0, -2.0 )
};

// Perform extrusions on the given mInteriorRes->mBrushes.  These have not yet been 
//    selfClip()ped, which is how we leave things.  Extrusion routine
//       needs the work done by the clipping though (Windings, etc). 
void EditGeometry::doGraphExtrusions(Vector<CSGBrush*> & brushList)
{
   for( U32 i = 0; i < brushList.size(); i++ )
   {
      for( U32 axis = 0; axis < 3; axis++ )
      {
         bool  extrudeBothWays = (axis != 2);
         CSGBrush * old = brushList[i];
         
         // Create a new extruded brush.  
         old->selfClip();
         brushList[i] = old->createExtruded( extrusions[axis] * 32.0, extrudeBothWays );
         gBrushArena.freeBrush( old );
      }
   }
}

// Just move detail mInteriorRes->mBrushes onto structural list for graph generation.  
void EditGeometry::xferDetailToStructural()
{
   for( S32 i = 0; i < mDetailBrushes.size(); i++ )
      mStructuralBrushes.push_back( mDetailBrushes[i] );
   mDetailBrushes.clear();
}

