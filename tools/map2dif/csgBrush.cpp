//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gfx/gBitmap.h"
#include "map2dif/csgBrush.h"
#include "map2dif/tokenizer.h"

extern int gQuakeVersion;

F32	baseaxis[18][3] =
{
{0,0,1}, {1,0,0}, {0,-1,0},			// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};


//------------------------------------------------------------------------------
void textureAxisFromPlane(const VectorF& normal, F32* xv, F32* yv)
{
   int		bestaxis;
   F32		dot,best;
   int		i;
	
   best = 0;
   bestaxis = 0;
	
   for (i=0 ; i<6 ; i++) {
      dot = mDot (normal, VectorF (baseaxis[i*3][0], baseaxis[i*3][1], baseaxis[i*3][2]));
      if (dot > best)
      {
         best = dot;
         bestaxis = i;
      }
   }
	
   xv[0] = baseaxis [bestaxis * 3 + 1][0];
   xv[1] = baseaxis [bestaxis * 3 + 1][1];
   xv[2] = baseaxis [bestaxis * 3 + 1][2];
   yv[0] = baseaxis [bestaxis * 3 + 2][0];
   yv[1] = baseaxis [bestaxis * 3 + 2][1];
   yv[2] = baseaxis [bestaxis * 3 + 2][2];
}


//------------------------------------------------------------------------------
void quakeTextureVecs(const VectorF& normal, 
                                    F32 offsetX, 
                                    F32 offsetY, 
                                    F32 rotate, 
                                    F32 scaleX, 
                                    F32 scaleY,
                                    PlaneF* u, 
                                    PlaneF *v) 
{ 
   F32   vecs[2][3];
   int   sv, tv;
   F32   ang, sinv, cosv;
   F32   ns, nt;
   int   i, j;

   textureAxisFromPlane(normal, vecs[0], vecs[1]);

   ang = rotate / 180 * M_PI;
   sinv = sin(ang);
   cosv = cos(ang);

   if (vecs[0][0] != 0.0)
      sv = 0;
   else if (vecs[0][1] != 0.0)
      sv = 1;
   else
      sv = 2;
				
   if (vecs[1][0] != 0.0)
      tv = 0;
   else if (vecs[1][1] != 0.0)
      tv = 1;
   else
      tv = 2;
					
   for (i=0 ; i<2 ; i++) {
      ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
      nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
      vecs[i][sv] = ns;
      vecs[i][tv] = nt;
   }

   u->x = vecs[0][0] / scaleX;
   u->y = vecs[0][1] / scaleX;
   u->z = vecs[0][2] / scaleX;
   u->d = offsetX;

   v->x = vecs[1][0] / scaleY;
   v->y = vecs[1][1] / scaleY;
   v->z = vecs[1][2] / scaleY;
   v->d = offsetY;
}

bool parseBrush (CSGBrush& rBrush, Tokenizer* pToker, EditGeometry& geom)
{
   while (pToker->getToken()[0] == '(') {
      // Enter the plane...
      F64 points[3][3];
      for (S32 i = 0; i < 3; i++) {
         if (pToker->getToken()[0] != '(')
            goto EntityBrushlistError;

         for (S32 j = 0; j < 3; j++) {
            pToker->advanceToken(false);
            points[i][j] = dAtof(pToker->getToken());
         }

         pToker->advanceToken(false);
         if (pToker->getToken()[0] != ')')
            goto EntityBrushlistError;
         pToker->advanceToken(false);
      }

      CSGPlane& rPlane = rBrush.constructBrushPlane(Point3D(points[0][0], points[0][1], points[0][2]),
                                                    Point3D(points[1][0], points[1][1], points[1][2]),
                                                    Point3D(points[2][0], points[2][1], points[2][2]));

      // advanced already...
      if (pToker->tokenAvailable() == false)
         goto EntityBrushlistError;
      rPlane.pTextureName = geom.insertTexture(pToker->getToken());

      // convert tex name to upper case
      char *str = (char *) rPlane.pTextureName;
      while( *str )
      {
         *str = dToupper( (const char) *str );
         str++;
      }

      U32 bmIndex = geom.getMaterialIndex(rPlane.pTextureName);
      const GBitmap* pBitmap = geom.mTextures[bmIndex];

      PlaneF tGenX;
      PlaneF tGenY;

      if (gQuakeVersion == 2)
      {
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->getToken()[0] != '[')         goto EntityBrushlistError;
   
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenX.x = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenX.y = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenX.z = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenX.d = dAtof(pToker->getToken());

         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->getToken()[0] != ']')         goto EntityBrushlistError;
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->getToken()[0] != '[')         goto EntityBrushlistError;

         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenY.x = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenY.y = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenY.z = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         tGenY.d = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->getToken()[0] != ']')         goto EntityBrushlistError;

         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 scaleX = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 scaleY = dAtof(pToker->getToken());

         tGenX.x /= scaleX * F32(pBitmap->getWidth());
         tGenX.y /= scaleX * F32(pBitmap->getWidth());
         tGenX.z /= scaleX * F32(pBitmap->getWidth());
         tGenX.d /= F32(pBitmap->getWidth());

         tGenY.x /= scaleY * F32(pBitmap->getHeight());
         tGenY.y /= scaleY * F32(pBitmap->getHeight());
         tGenY.z /= scaleY * F32(pBitmap->getHeight());
         tGenY.d /= F32(pBitmap->getHeight());
      }
      else if (gQuakeVersion == 3)
      {
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 shiftU = dAtof (pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 shiftV = dAtof (pToker->getToken());

         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 rot = dAtof (pToker->getToken());

         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 scaleX = dAtof(pToker->getToken());
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         F32 scaleY = dAtof(pToker->getToken());

         // Skip last 3 tokens
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;
         if (pToker->advanceToken(false) == false) goto EntityBrushlistError;

         // Compute the normal
         VectorF normal;
         VectorF t1, t2;

         t1 = VectorF (points[0][0], points[0][1], points[0][2]) - VectorF (points[1][0], points[1][1], points[1][2]);
         t2 = VectorF (points[2][0], points[2][1], points[2][2]) - VectorF (points[1][0], points[1][1], points[1][2]);
         mCross (t1, t2, normal);
         normal.normalize ();

         quakeTextureVecs (normal, shiftU, shiftV, rot, scaleX, scaleY, &tGenX, &tGenY);

         F32 width = F32(pBitmap->getWidth());
         F32 height = F32(pBitmap->getHeight());

         tGenX.x /= F32(pBitmap->getWidth());
         tGenX.y /= F32(pBitmap->getWidth());
         tGenX.z /= F32(pBitmap->getWidth());
         tGenX.d /= F32(pBitmap->getWidth());

         tGenY.x /= F32(pBitmap->getHeight());
         tGenY.y /= F32(pBitmap->getHeight());
         tGenY.z /= F32(pBitmap->getHeight());
         tGenY.d /= F32(pBitmap->getHeight());
      }
      else
      {
         goto EntityBrushlistError;
      }

      // add it...
      bool found = false;
      for(U32 i = 0; !found && (i < geom.mTexGenEQs.size()); i++) 
         if(!dMemcmp(&geom.mTexGenEQs[i].planeX, &tGenX, sizeof(PlaneF)) &&
            !dMemcmp(&geom.mTexGenEQs[i].planeY, &tGenY, sizeof(PlaneF)))
         {
            found = true;
            rPlane.texGenIndex = i;
         }

      //
      if(!found)
      {
         geom.mTexGenEQs.increment();
         geom.mTexGenEQs.last().planeX = tGenX;
         geom.mTexGenEQs.last().planeY = tGenY;
         rPlane.texGenIndex = geom.mTexGenEQs.size() - 1;
      }

      pToker->advanceToken(true);
   }

   return true;

EntityBrushlistError:
   return false;
}

void CSGPlane::construct(const Point3D& Point1, const Point3D& Point2, const Point3D& Point3)
{
   // |yi zi 1|     |xi zi 1|     |xi yi 1|     |xi yi zi|
   // |yj zj 1| x + |xj zj 1| y + |xj yj 1| z = |xj yj zj|
   // |yk zk 1|     |xk zk 1|     |xk yk 1|     |xk yk zk|
   //
   Point3D normal;
   F64  dist;

   normal.x = Point1.y * Point2.z - Point1.y * Point3.z +
              Point3.y * Point1.z - Point2.y * Point1.z +
              Point2.y * Point3.z - Point3.y * Point2.z;
   normal.y = Point1.x * Point2.z - Point1.x * Point3.z +
              Point3.x * Point1.z - Point2.x * Point1.z +
              Point2.x * Point3.z - Point3.x * Point2.z;
   normal.z = Point1.x * Point2.y - Point1.x * Point3.y +
              Point3.x * Point1.y - Point2.x * Point1.y +
              Point2.x * Point3.y - Point3.x * Point2.y;
   dist     = Point1.x * Point2.y * Point3.z - Point1.x * Point2.z * Point3.y +
              Point1.y * Point2.z * Point3.x - Point1.y * Point2.x * Point3.z +
              Point1.z * Point2.x * Point3.y - Point1.z * Point2.y * Point3.x;

   normal.x = -normal.x;
   normal.z = -normal.z;
   
   //
   planeEQIndex = gWorkingGeometry->insertPlaneEQ(normal, dist);
   flags = 0;
}

//------------------------------------------------------------------------------
// Needs to insert new plane equation.  parameter is positive amount 
// to extrude outward.
void CSGPlane::extrude(F64 byAmount)
{
   const PlaneEQ & eq = gWorkingGeometry->getPlaneEQ(planeEQIndex);
   planeEQIndex = gWorkingGeometry->insertPlaneEQ(eq.normal, eq.dist - byAmount);
}

//------------------------------------------------------------------------------

bool CSGPlane::createBaseWinding(const Vector<U32>& rPoints)
{
   return ::createBaseWinding(rPoints, getNormal(), &winding);
}


//------------------------------------------------------------------------------

// Try a more accurate version of this.  
PlaneSide CSGPlane::sideCheckEpsilon(const Point3D& testPoint, F64 epsilon) const
{
   F64 distance = distanceToPlane(testPoint);
   if (distance < - epsilon)
      return PlaneBack;
   else if (distance > epsilon)
      return PlaneFront;
   else
      return PlaneOn;
}

// Need more info for asserts.  
const char* CSGPlane::sideCheckInfo(const Point3D & P, const char * msg, F64 E) const
{
   F64            D = distanceToPlane(P);
   const char *   txt = D < -E ? "BACK" : (D > E ? "FRONT" : "ON");
   return avar( "%s:  Side=%s E=%lf D=%1.20lf P=(%lf,%lf,%lf)", msg, txt, E, D, P.x, P.y, P.z );
}

// See if this plane lies along an edge of the supplied winding.  Return the 
// points if so (they're needed), else return NULL for false.  
//
// Maybe need a better point here.  
Point3D * CSGPlane::sharesEdgeWith(const Winding & W) const
{
   static Point3D    edgePts[2];
   for( U32 i = 0; i < W.numIndices; i++ )
   {
      edgePts[0] = gWorkingGeometry->getPoint(W.indices[i]);
      // if( sideCheckEpsilon( edgePts[0] ) == PlaneOn )
      if( whichSide( edgePts[0] ) == PlaneOn )
      {
         edgePts[1] = gWorkingGeometry->getPoint(W.indices[(i + 1) % W.numIndices]);
         // if( sideCheckEpsilon( edgePts[1] ) == PlaneOn )
         if( whichSide( edgePts[1] ) == PlaneOn )
            return edgePts;
      }
   }
   return NULL;
}


// Assuming this is a brush that has already been selfClip()d, make 
//    and return a new brush that is extruded version of this 
//       one (tho not clipped) in given direction.  
// 
// If doBoth, then extrude also in negative of direction.  
//
// Adds additional planes on those edges which are boundaries with 
//    respect to the direction axis.  That is, those edges which will
//    will be on the outside of the shape perimeter when viewing
//    along the given direction axis, and so planes need to be added
//    to limit unwanted extrusion outside of the intended axis.  
// 
CSGBrush* CSGBrush::createExtruded(const Point3D & extrudeDir, bool doBoth) const
{
   CSGBrush * newBrush = gBrushArena.allocateBrush();
   
   newBrush->copyBrush( this );

   for( U32 i = 0; i < mPlanes.size(); i++ )
   {
      // Get extrusion component along normal.  
      const Point3D &   curNormal = mPlanes[i].getNormal();
      F64               extrudeComponent = mDot( extrudeDir, curNormal );

      if( mFabsD(extrudeComponent) > 0.001 ) 
      {
         const Winding &   curWinding = mPlanes[i].winding;
         
         // Look for planes that are opposite with respect to this 
         // direction and which have a shared edge.  We create 'clamping'
         // planes along such edges to avoid spikes.  
         for( U32 j = i + 1; j < mPlanes.size(); j++ )
         {
            if( mDot(mPlanes[j].getNormal(),extrudeDir) * extrudeComponent < -0.0001 )
            {
               if( Point3D * edgePts = mPlanes[j].sharesEdgeWith(curWinding) )
               {
                  Point3D  other = edgePts[0] + extrudeDir;
                  CSGPlane & rPlane = (extrudeComponent > 0) 
                                       ?
                     newBrush->constructBrushPlane( other, edgePts[0], edgePts[1] )
                                       :
                     newBrush->constructBrushPlane( edgePts[1], edgePts[0], other );
                        
                  rPlane.pTextureName = mPlanes[0].pTextureName;
                  rPlane.xShift = 0;
                  rPlane.yShift = 0;
                  rPlane.rotation = 0;
                  rPlane.xScale = rPlane.yScale = 1.0;
               }
            }
         }
         
         if( extrudeComponent > 0 || doBoth )
            newBrush->mPlanes[i].extrude( mFabsD( extrudeComponent ) );
      }
   }
   
   AssertFatal( newBrush->mPlanes.size() >= mPlanes.size(), 
         "CSGBrush::createExtruded(): new brush shouldn't be smaller" );
         
   return newBrush;
}


//------------------------------------------------------------------------------
bool CSGBrush::intersectPlanes(U32 i, U32 j, U32 k,
                               Point3D* pOutput)
{
   AssertFatal(i < mPlanes.size() && j < mPlanes.size() && k < mPlanes.size() &&
               i != j && i != k && j != k, "CSGBrush::intersectPlanes: bad plane indices");

   CSGPlane& rPlane1 = mPlanes[i];
   CSGPlane& rPlane2 = mPlanes[j];
   CSGPlane& rPlane3 = mPlanes[k];

   const PlaneEQ& rPlaneEQ1 = gWorkingGeometry->getPlaneEQ(rPlane1.planeEQIndex);
   const PlaneEQ& rPlaneEQ2 = gWorkingGeometry->getPlaneEQ(rPlane2.planeEQIndex);
   const PlaneEQ& rPlaneEQ3 = gWorkingGeometry->getPlaneEQ(rPlane3.planeEQIndex);

   F64 bc  = (rPlaneEQ2.normal.y * rPlaneEQ3.normal.z) - (rPlaneEQ3.normal.y * rPlaneEQ2.normal.z);
   F64 ac  = (rPlaneEQ2.normal.x * rPlaneEQ3.normal.z) - (rPlaneEQ3.normal.x * rPlaneEQ2.normal.z);
   F64 ab  = (rPlaneEQ2.normal.x * rPlaneEQ3.normal.y) - (rPlaneEQ3.normal.x * rPlaneEQ2.normal.y);
   F64 det = (rPlaneEQ1.normal.x * bc) - (rPlaneEQ1.normal.y * ac) + (rPlaneEQ1.normal.z * ab);

   if (mFabs(det) < 1e-7) {
      // Parallel planes
      return false;
   }

   F64 dc     = (rPlaneEQ2.dist * rPlaneEQ3.normal.z) - (rPlaneEQ3.dist * rPlaneEQ2.normal.z);
   F64 db     = (rPlaneEQ2.dist * rPlaneEQ3.normal.y) - (rPlaneEQ3.dist * rPlaneEQ2.normal.y);
   F64 ad     = (rPlaneEQ3.dist * rPlaneEQ2.normal.x) - (rPlaneEQ2.dist * rPlaneEQ3.normal.x);
   F64 detInv = 1.0 / det;

   pOutput->x = ((rPlaneEQ1.normal.y * dc) - (rPlaneEQ1.dist     * bc) - (rPlaneEQ1.normal.z * db)) * detInv;
   pOutput->y = ((rPlaneEQ1.dist     * ac) - (rPlaneEQ1.normal.x * dc) - (rPlaneEQ1.normal.z * ad)) * detInv;
   pOutput->z = ((rPlaneEQ1.normal.y * ad) + (rPlaneEQ1.normal.x * db) - (rPlaneEQ1.dist     * ab)) * detInv;

   return true;
}


//------------------------------------------------------------------------------
bool CSGBrush::selfClip()
{
   static const U32 VectorBufferSize = 64;
   static UniqueVector pPlanePoints[VectorBufferSize];
   AssertISV(mPlanes.size() < VectorBufferSize,
             "CSGBrush::selfClip: talk to Dave Moore.  Must increase planepoint buffer size");

   for (U32 i = 0; i < mPlanes.size(); i++) {
      for (U32 j = i+1; j < mPlanes.size(); j++) {
         for (U32 k = j+1; k < mPlanes.size(); k++) {
            Point3D intersection;
            bool doesSersect = intersectPlanes(i, j, k, &intersection);

            if (doesSersect) {
               
               F64   E = gcPlaneDistanceEpsilon;
               const char * msg = "CSGBrush::selfClip: intersectionPoint not on plane";
               AssertFatal( mPlanes[i].sideCheckEpsilon(intersection,E) == PlaneOn, 
                            mPlanes[i].sideCheckInfo(intersection, msg, E)    );
               AssertFatal( mPlanes[j].sideCheckEpsilon(intersection,E) == PlaneOn, 
                            mPlanes[j].sideCheckInfo(intersection, msg, E)    );
               AssertFatal( mPlanes[k].sideCheckEpsilon(intersection,E) == PlaneOn, 
                            mPlanes[k].sideCheckInfo(intersection, msg, E)    );

               bool inOrOn = true;
               for (U32 l = 0; l < mPlanes.size(); l++) {
                  if (mPlanes[l].whichSide(intersection) == PlaneFront) {
                     inOrOn = false;
                     break;
                  }
               }

               if (inOrOn == true) {
                  U32 pIndex = gWorkingGeometry->insertPoint(intersection);
                  pPlanePoints[i].pushBackUnique(pIndex);
                  pPlanePoints[j].pushBackUnique(pIndex);
                  pPlanePoints[k].pushBackUnique(pIndex);

                  gWorkingGeometry->mMinBound.setMin(intersection);
                  gWorkingGeometry->mMaxBound.setMax(intersection);
               }
            }
         }
      }
   }

   mMinBound.set(1e10, 1e10, 1e10);
   mMaxBound.set(-1e10, -1e10, -1e10);
   for (U32 i = 0; i < mPlanes.size(); i++) {
      bool success = mPlanes[i].createBaseWinding(pPlanePoints[i]);
      if(!success)
         return false;
      for (U32 j = 0; j < mPlanes[i].winding.numIndices; j++) {
         const Point3D& rPoint = gWorkingGeometry->getPoint(mPlanes[i].winding.indices[j]);
         mMinBound.setMin(rPoint);
         mMaxBound.setMax(rPoint);
      }

      AssertISV(success, "Error creating winding.  DMM - Need to remove brush here.");
      pPlanePoints[i].clear();
   }
   return true;
}

void CSGBrush::copyBrush(const CSGBrush* pCopy)
{
   mPlanes      = pCopy->mPlanes;
   mIsAmbiguous = pCopy->mIsAmbiguous;
   mBrushType   = pCopy->mBrushType;
   mMinBound    = pCopy->mMinBound;
   mMaxBound    = pCopy->mMaxBound;

   sgLightingScale = pCopy->sgLightingScale;
   sgSelfIllumination = pCopy->sgSelfIllumination;

   brushId      = pCopy->brushId;
   materialType = pCopy->materialType;
}

//------------------------------------------------------------------------------
bool CSGBrush::disambiguate()
{
   AssertFatal(mIsAmbiguous == false, "error, already disambiguated?");

   for (U32 i = 0; i < mPlanes.size(); i++) {
      for (U32 j = i + 1; j < mPlanes.size();) {
         // Compare i to j.  if j == i, with different tex parameters, increment
         //  ambiguous brushes (once only), and remove the plane.
         //
         CSGPlane& rPlane1 = mPlanes[i];
         CSGPlane& rPlane2 = mPlanes[j];
         if (rPlane1.planeEQIndex == rPlane2.planeEQIndex) {
            // Possible ambiguous plane pairing...
            //
            if (rPlane1.pTextureName != rPlane2.pTextureName ||
                rPlane1.xShift       != rPlane2.xShift       ||
                rPlane1.yShift       != rPlane2.yShift       ||
                rPlane1.rotation     != rPlane2.rotation     ||
                rPlane1.xScale       != rPlane2.xScale       ||
                rPlane1.yScale       != rPlane2.yScale) {
               mIsAmbiguous = true;
            } else {
               // Same texture parameters, should be fine, just erase it...
               //
            }

            mPlanes.erase(j);
         } else {
            // Plane is fine...
            j++;
         }
      }
   }

   return mIsAmbiguous;
}


//------------------------------------------------------------------------------
/*CSGPlane& CSGBrush::constructBrushPlane(const Point3I& rPoint1,
                                        const Point3I& rPoint2,
                                        const Point3I& rPoint3)
{
   mPlanes.increment();
   CSGPlane& rPlane = mPlanes.last();
   rPlane.flags = 0;
   rPlane.owningEntity = NULL;
   rPlane.winding.numNodes = 0;
   rPlane.winding.numZoneIds = 0;

   Point3D Point1(rPoint1.x, rPoint1.y, rPoint1.z);
   Point3D Point2(rPoint2.x, rPoint2.y, rPoint2.z);
   Point3D Point3(rPoint3.x, rPoint3.y, rPoint3.z);

   rPlane.construct(Point1, Point2, Point3);
   
   if (rPlane.getNormal().x == 0 && rPlane.getNormal().y == 0 && rPlane.getNormal().z == 0) {
      AssertISV(false, "Error, degenerate plane (colinear points)");
      mPlanes.decrement();
      return *((CSGPlane*)NULL);
   }
   
   return rPlane;
}*/
CSGPlane& CSGBrush::constructBrushPlane(const Point3D& Point1,
                                        const Point3D& Point2,
                                        const Point3D& Point3)
{
   mPlanes.increment();
   CSGPlane& rPlane = mPlanes.last();
   rPlane.flags = 0;
   rPlane.owningEntity = NULL;
   rPlane.winding.numNodes = 0;
   rPlane.winding.numZoneIds = 0;

   rPlane.construct(Point1, Point2, Point3);

   if (rPlane.getNormal().x == 0 && rPlane.getNormal().y == 0 && rPlane.getNormal().z == 0) {
      AssertISV(false, "Error, degenerate plane (colinear points)");
      mPlanes.decrement();
      return *((CSGPlane*)NULL);
   }

   return rPlane;
}

bool CSGBrush::isEquivalent(const CSGBrush& testBrush) const
{
   for (U32 i = 0; i < mPlanes.size(); i++) {
      U32 j;
      for (j = 0; j < testBrush.mPlanes.size(); j++) {
         if (testBrush.mPlanes[j].planeEQIndex == mPlanes[i].planeEQIndex)
            break;
      }

      if (j == testBrush.mPlanes.size())
         return false;
   }

   return true;
}
