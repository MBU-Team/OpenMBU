#include "collision/convexBrush.h"

#include "math/mRandom.h"
#include "game/gameConnection.h"
#include "math/mPlaneTransformer.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

ConvexBrush::ConvexBrush()
{
    mBounds.min = Point3F(0, 0, 0);
    mBounds.max = Point3F(0, 0, 0);

    mBrushScale = 1.0f;
    mType = 0;
    mCentroid = Point3F(0.0f, 0.0f, 0.0f);
    mStatus = Unprocessed;
    mSelected = false;

    // Set our transform to no translation or rotation
    mTransform.identity();

    mTransformScratch.identity();
    mRotationScratch.identity();

    mScale.set(1.0f, 1.0f, 1.0f);
    mRotation.set(EulerF(0.0f, 0.0f, 0.0f));

    // No id assigned yet
    mID = -1;

    mOwner = NULL;
}

ConvexBrush::~ConvexBrush()
{
}

// Just adds a plane to the list and default values for texture info
bool ConvexBrush::addPlane(PlaneF pln)
{
    mFaces.mPolyList.increment();
    mFaces.mPolyList.last().material = 0;
    mFaces.mPolyList.last().object = NULL;
    mFaces.mPolyList.last().surfaceKey = 0;
    mFaces.mPolyList.last().vertexCount = 0;
    mFaces.mPolyList.last().vertexStart = 0;
    mFaces.mPolyList.last().plane = mFaces.addPlane(pln);

    // The texgens and the faces increase together
    mTexInfos.increment();
    mTexInfos.last().texGens[0] = PlaneF(1.0f, 0.0f, 0.0f, 1.0f);
    mTexInfos.last().texGens[1] = PlaneF(0.0f, 0.0f, 1.0f, 1.0f);
    mTexInfos.last().scale[0] = 1.0f;
    mTexInfos.last().scale[1] = 1.0f;
    mTexInfos.last().rot = 0.0f;
    mTexInfos.last().texDiv[0] = 1.0f;
    mTexInfos.last().texDiv[1] = 1.0f;
    mTexInfos.last().texture = StringTable->insert("");

    return true;
}

bool ConvexBrush::addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], char* texture)
{
    addPlane(pln);

    mTexInfos.last().texGens[0] = texGen[0];
    mTexInfos.last().texGens[1] = texGen[1];
    mTexInfos.last().scale[0] = scale[0];
    mTexInfos.last().scale[1] = scale[1];
    mTexInfos.last().texture = StringTable->insert(texture);

    return true;
}

bool ConvexBrush::addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], char* texture, U32 matIdx)
{
    addFace(pln, texGen, scale, texture);

    mFaces.mPolyList.last().material = matIdx;

    return true;
}

bool ConvexBrush::addFace(PlaneF pln, PlaneF texGen[2], F32 scale[2], U32 matIdx)
{
    return addFace(pln, texGen, scale, "", matIdx);;
}

// Call this after you have added the planes to generate the rest of the data
bool ConvexBrush::processBrush()
{
    // It is possible that our status was changed externally by the parser
    if (mStatus != Unprocessed)
        return false;

    // First check to see if we have enough planes
    if (mFaces.mPlaneList.size() < 4)
    {
        mStatus = ShortPlanes;
        char debug[512];
        dSprintf(debug, 512, "This brush only has %d planes which is insufficent to create a convex volume. Possibly a badly formatted .map?", mFaces.mPlaneList.size());
        mDebugInfo = StringTable->insert(debug);
        return false;
    }

    // Generate the geometry
    selfClip();

    // Verify that all of the faces have at least 3 vertices
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        if (mFaces.mPolyList[i].vertexCount < 3)
        {
            mStatus = BadFaces;
            mDebugInfo = StringTable->insert("There are bad faces on this brush. This is most likely caused by a flat or non-volumetric brush.");
            return false;
        }
    }

    // Generate proper windings for the faces
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        // Create a temporary index list
        Vector<U32> indexes;
        for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount; j++)
            indexes.push_back(mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j]);

        // Create a buffer to return the winding to
        Vector<U32> winding;

        if (createWinding(indexes, mFaces.mPlaneList[mFaces.mPolyList[i].plane], winding))
        {
            // Copy the correctly order indices back
            for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount; j++)
                mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j] = winding[j];
        }
        else
        {
            mStatus = BadWinding;
            mDebugInfo = StringTable->insert("Unable to generate windings for some of the faces. This is often caused by having brushes that are too thin.");
            return false;
        }

        indexes.clear();
        winding.clear();
    }

    //validateWindings();

    generateEdgelist();

    if (!validateEdges())
        return false;

    calcBounds();

    // If we got this far without any errors consider the brush to be good
    if (mStatus == Unprocessed)
        mStatus = Good;
    else
        return false;

    return true;
}

void ConvexBrush::calcCentroid()
{
    // First find the centroid
    for (U32 i = 0; i < mFaces.mVertexList.size(); i++)
        mCentroid += mFaces.mVertexList[i];

    mCentroid /= mFaces.mVertexList.size();
}

void ConvexBrush::setupTransform()
{
    // First calculate the centroid
    calcCentroid();

    mTransform.setColumn(3, mCentroid);

    mCentroid.set(0.0f, 0.0f, 0.0f);

    // Now untransform the verts and planes
    MatrixF invTrans = mTransform;
    invTrans.inverse();

    // Untransform our verts
    for (U32 i = 0; i < mFaces.mVertexList.size(); i++)
        invTrans.mulP(mFaces.mVertexList[i]);

    // Untransform our bounds
    invTrans.mulP(mBounds.min);
    invTrans.mulP(mBounds.max);

    // Untransform our planes
    PlaneTransformer trans;
    trans.set(invTrans, Point3F(1.0f, 1.0f, 1.0f));

    for (U32 i = 0; i < mFaces.mPlaneList.size(); i++)
    {
        PlaneF temp;
        trans.transform(mFaces.mPlaneList[i], temp);
        mFaces.mPlaneList[i] = temp;
    }
}

bool ConvexBrush::validateEdges()
{
    // Check to make sure that ever edge has two faces
    for (U32 i = 0; i < mFaces.mEdgeList.size(); i++)
    {
        OptimizedPolyList::Edge ed = mFaces.mEdgeList[i];
        if (mFaces.mEdgeList[i].faces[1] == -1)
        {
            mStatus = Concave;
            mDebugInfo = StringTable->insert("There are missing faces on this brush. This is caused by having concave edges in the brush.");
            return false;
        }
    }

    // Now test for concavity
    for (U32 i = 0; i < mFaces.mEdgeList.size(); i++)
    {
        U32 face0 = mFaces.mEdgeList[i].faces[0];
        U32 face1 = mFaces.mEdgeList[i].faces[1];

        // Get the indices that make up the edge
        U32 i0 = mFaces.mEdgeList[i].vertexes[0];
        U32 i1 = mFaces.mEdgeList[i].vertexes[1];

        // Now get a point from face0 that isn't on the edge
        U32 nsx;
        for (U32 j = 0; j < mFaces.mPolyList[face0].vertexCount; j++)
        {
            nsx = mFaces.mIndexList[mFaces.mPolyList[face0].vertexStart + j];

            if (nsx != i0 && nsx != i1)
                break;
        }

        // Get the vertices
        Point3F ns = mFaces.mVertexList[nsx];

        // Grab the plane of face1
        PlaneF pln = mFaces.mPlaneList[mFaces.mPolyList[face1].plane];

        // Test to see if the point is in front or behind
        if (pln.whichSide(ns) == PlaneF::Back)
            mFaces.mEdgeList[i].type = OptimizedPolyList::Convex;
        else
        {
            mFaces.mEdgeList[i].type = OptimizedPolyList::Concave;
            mStatus = Concave;
            mDebugInfo = StringTable->insert("Found some concave edges on this brush.");
        }
    }

    if (mStatus == Concave)
        return false;
    else
        return true;
}

void ConvexBrush::validateWindings()
{
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        if (mFaces.mPolyList[i].vertexCount < 3)
            continue;

        U32 i0 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart];
        U32 i1 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + 1];
        U32 i2 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + 2];

        Point3F p0 = mFaces.mVertexList[i0];
        Point3F p1 = mFaces.mVertexList[i1];
        Point3F p2 = mFaces.mVertexList[i2];

        PlaneF wnorm(p0, p1, p2);

        if (wnorm.whichSide(mCentroid) == PlaneF::Front)
        {
            Con::errorf("Face %d's winding is inverted", i);

            // Let's reverse the winding
            Vector<U32> fixed;

            for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount; j++)
                fixed.push_front(mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j]);

            for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount; j++)
                mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j] = fixed[j];
        }
    }
}

void ConvexBrush::addIndex(Vector<U32>& indices, U32 index)
{
    for (U32 i = 0; i < indices.size(); i++)
    {
        if (indices[i] == index)
            return;
    }

    indices.push_back(index);
}

// This clips the planes against each other
bool ConvexBrush::selfClip()
{
    // Used to hold our indices temporarily
    Vector<U32>* faceIndices = new Vector<U32>[mFaces.mPolyList.size()];

    //Con::errorf("New brush");
    //for (U32 i = 0; i < mFaces.mPlaneList.size(); i++)
    //   Con::printf("plane %d(%f, %f, %f, %f)", i, mFaces.mPlaneList[i].x, mFaces.mPlaneList[i].y, mFaces.mPlaneList[i].z, mFaces.mPlaneList[i].d);

    // Clip them against each other to generate the vertexes
    for (U32 i = 0; i < mFaces.mPlaneList.size(); i++)
    {
        for (U32 j = i + 1; j < mFaces.mPlaneList.size(); j++)
        {
            for (U32 k = j + 1; k < mFaces.mPlaneList.size(); k++)
            {
                Point3D interd;
                Point3F interf;

                bool doesSersect = intersectPlanes(mFaces.mPlaneList[i], mFaces.mPlaneList[j], mFaces.mPlaneList[k], &interd);

                interf = Point3F(interd.x, interd.y, interd.z);

                if (doesSersect)
                {
                    // Check to make sure that the point is behind or on all of the
                    // planes
                    bool inOrOn = true;

                    for (U32 l = 0; l < mFaces.mPlaneList.size(); l++)
                    {
                        if (mFaces.mPlaneList[l].whichSide(interf) == PlaneF::Front)
                        {
                            inOrOn = false;
                            break;
                        }
                    }

                    if (inOrOn == true)
                    {
                        U32 vdx = mFaces.addPoint(interf);

                        addIndex(faceIndices[i], vdx);
                        addIndex(faceIndices[j], vdx);
                        addIndex(faceIndices[k], vdx);
                    }
                }
            }
        }
    }

    // Add the indices for the faces
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        // Set the starting index
        mFaces.mPolyList[i].vertexStart = mFaces.mIndexList.size();
        // Set the number of indexes
        mFaces.mPolyList[i].vertexCount = faceIndices[i].size();

        // Copy the indexes into the index list
        for (U32 j = 0; j < faceIndices[i].size(); j++)
        {
            mFaces.mIndexList.push_back(faceIndices[i][j]);
        }
    }

    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
        faceIndices[i].clear();

    delete[] faceIndices;

    return true;
}

bool ConvexBrush::intersectPlanes(const PlaneF& plane1, const PlaneF& plane2, const PlaneF& plane3, Point3D* pOutput)
{
    F64 bc = (plane2.y * plane3.z) - (plane3.y * plane2.z);
    F64 ac = (plane2.x * plane3.z) - (plane3.x * plane2.z);
    F64 ab = (plane2.x * plane3.y) - (plane3.x * plane2.y);
    F64 det = (plane1.x * bc) - (plane1.y * ac) + (plane1.z * ab);

    if (mFabs(det) < 1e-7)
    {
        // Parallel planes
        return false;
    }

    F64 dc = (plane2.d * plane3.z) - (plane3.d * plane2.z);
    F64 db = (plane2.d * plane3.y) - (plane3.d * plane2.y);
    F64 ad = (plane3.d * plane2.x) - (plane2.d * plane3.x);
    F64 detInv = 1.0 / det;

    pOutput->x = ((plane1.y * dc) - (plane1.d * bc) - (plane1.z * db)) * detInv;
    pOutput->y = ((plane1.d * ac) - (plane1.x * dc) - (plane1.z * ad)) * detInv;
    pOutput->z = ((plane1.y * ad) + (plane1.x * db) - (plane1.d * ab)) * detInv;

    return true;
}

bool ConvexBrush::createWinding(const Vector<U32>& rPoints, const Point3F& normal, Vector<U32>& rWinding)
{
    if (rPoints.size() < 3)
        return false;

    Vector<U32> modPoints;
    Point3F centroid(0, 0, 0);
    Point3F projcent(0, 0, 0);

    modPoints = rPoints;

    // Create a centroid first...
    for (U32 i = 0; i < modPoints.size(); i++)
        centroid += mFaces.mVertexList[modPoints[i]];
    centroid /= F32(modPoints.size());

    // Create a point projected along the normal
    projcent = centroid + normal;

    // Ok, we have a centroid.  We (arbitrarily) start with the last point
    rWinding.increment();
    rWinding.last() = modPoints[modPoints.size() - 1];
    modPoints.erase(modPoints.size() - 1);

    while (modPoints.size() > 0)
    {
        // Get our last winding point
        Point3F& currPoint = mFaces.mVertexList[rWinding.last()];

        // Calculate the vector between the centroid and the current point
        VectorF currVector = currPoint - centroid;
        currVector.normalize();

        // Create a plane with the winding point, the centroid, and the projected centroid
        PlaneF testPlane(currPoint, centroid, projcent);

        F32 maxDot = -1e10;
        S32 maxIdx = -1;

        for (U32 i = 0; i < modPoints.size(); i++)
        {
            Point3F& testPoint = mFaces.mVertexList[modPoints[i]];

            if (testPlane.whichSide(testPoint) == PlaneF::Front)
            {
                // Get the vector between this point and the centroid
                VectorF testVector = testPoint - centroid;
                testVector.normalize();

                F32 dot = mDot(testVector, currVector);

                if (dot > maxDot)
                {
                    maxDot = dot;
                    maxIdx = i;
                }
            }
        }

        if (maxIdx == -1)
        {
            // Sometimes the last vertex doesn't make it onto the list
            if (modPoints.size() == 1)
            {
                rWinding.increment();
                rWinding.last() = modPoints.last();
                return true;
            }
            else
            {
                Con::errorf("ConvexBrush::createWinding(): was unable to find the next winding point");

                // For debugging
                U32 pt = 0;
                for (U32 i = 0; i < rWinding.size(); i++)
                {
                    Point3F& point = mFaces.mVertexList[rWinding[i]];

                    Con::printf("   point %d (%g, %g, %g)", pt, point.x, point.y, point.z);
                }

                for (U32 i = 0; i < modPoints.size(); i++)
                {
                    Point3F& point = mFaces.mVertexList[modPoints[i]];

                    Con::printf("   point %d (%g, %g, %g)", pt, point.x, point.y, point.z);
                }

                return false;
            }
        }

        rWinding.increment();
        rWinding.last() = modPoints[maxIdx];
        modPoints.erase(maxIdx);
    }

    modPoints.clear();

    return true;
}

bool ConvexBrush::render(bool genColors)
{
    /*
       // Renders it in colors only
       if (genColors)
          gRandGen.setSeed(1978);

       glEnable(GL_CULL_FACE);

       // Pick a random color for our brush
       if (!genColors)
       {
          ColorF color(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f));
          glColor3f(color.red, color.green, color.blue);
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
       }

       for (U32 j = 0; j < mFaces.mPolyList.size(); j++)
       {
          S32 tx = mFaces.mPolyList[j].material;

          if (tx == -2)
             continue;

          glBegin(GL_TRIANGLES);
             for (U32 k = 1; k < mFaces.mPolyList[j].vertexCount - 1; k++)
             {
                U32 i0 = mFaces.mIndexList[mFaces.mPolyList[j].vertexStart];
                U32 i1 = mFaces.mIndexList[mFaces.mPolyList[j].vertexStart + k];
                U32 i2 = mFaces.mIndexList[mFaces.mPolyList[j].vertexStart + k + 1];

                Point3F p0 = mFaces.mVertexList[i0];
                Point3F p1 = mFaces.mVertexList[i1];
                Point3F p2 = mFaces.mVertexList[i2];

                if (genColors)
                {
                   ColorF color(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f));
                   glColor3f(color.red, color.green, color.blue);
                   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
                }

                PlaneF normal = mFaces.mPlaneList[mFaces.mPolyList[k].plane];
                glNormal3f(normal.x, normal.y, normal.z);

                F32 s, t;

                PlaneF texGenX = mTexInfos[k].texGens[0];
                PlaneF texGenY = mTexInfos[k].texGens[1];

                s = texGenX.distToPlane(p0 * mBrushScale);
                t = texGenY.distToPlane(p0 * mBrushScale);

                glTexCoord2f(s, t);
                glVertex3f(p0.x, p0.y, p0.z);

                s = texGenX.distToPlane(p1 * mBrushScale);
                t = texGenY.distToPlane(p1 * mBrushScale);

                glTexCoord2f(s, t);
                glVertex3f(p1.x, p1.y, p1.z);

                s = texGenX.distToPlane(p2 * mBrushScale);
                t = texGenY.distToPlane(p2 * mBrushScale);

                glTexCoord2f(s, t);
                glVertex3f(p2.x, p2.y, p2.z);
             }
          glEnd();
       }
    */

    return true;
}

bool ConvexBrush::renderFace(U32 face, bool renderLighting)
{
    /*
       if (face > mFaces.mPolyList.size())
       {
          Con::errorf("ConvexBrush::debugRenderFace(): face is outside range");
          return false;
       }

       if (mFaces.mPolyList[face].vertexCount < 3)
          return false;

       const OptimizedPolyList::triangleLighting *lighting = NULL;
       Point3F tp;
       PlaneF lmGenX, lmGenY;
       if(renderLighting)
           glActiveTextureARB(GL_TEXTURE1_ARB);

          for (U32 k = 1; k < mFaces.mPolyList[face].vertexCount - 1; k++)
          {
             U32 i0 = mFaces.mIndexList[mFaces.mPolyList[face].vertexStart];
             U32 i1 = mFaces.mIndexList[mFaces.mPolyList[face].vertexStart + k];
             U32 i2 = mFaces.mIndexList[mFaces.mPolyList[face].vertexStart + k + 1];

             Point3F p0 = mFaces.mVertexList[i0];
             Point3F p1 = mFaces.mVertexList[i1];
             Point3F p2 = mFaces.mVertexList[i2];

           if((renderLighting) && ((lighting = mFaces.getTriangleLighting(mFaces.mPolyList[face].triangleLightingStartIndex + (k - 1))) != NULL))
           {
               lmGenX = lighting->lightMapEquationX;
               lmGenY = lighting->lightMapEquationY;
               glBindTexture(GL_TEXTURE_2D, lighting->lightMapId);
           }

           glBegin(GL_TRIANGLES);

           PlaneF normal = mFaces.mPlaneList[mFaces.mPolyList[face].plane];
           glNormal3f(normal.x, normal.y, normal.z);

           F32 s, t;

           PlaneF texGenX = mTexInfos[face].texGens[0];
           PlaneF texGenY = mTexInfos[face].texGens[1];

           tp = p0;
           s = texGenX.distToPlane(tp);
           t = texGenY.distToPlane(tp);
           glMultiTexCoord2fARB(GL_TEXTURE0_ARB, s, t);

           if(lighting)
           {
               mLightingTransform.mulP(tp);
               s = lmGenX.distToPlane(tp);
               t = lmGenY.distToPlane(tp);
               glMultiTexCoord2fARB(GL_TEXTURE1_ARB, s, t);
           }

           glVertex3f(p0.x, p0.y, p0.z);

           tp = p1;
           s = texGenX.distToPlane(tp);
           t = texGenY.distToPlane(tp);
           glMultiTexCoord2fARB(GL_TEXTURE0_ARB, s, t);

           if(lighting)
           {
               mLightingTransform.mulP(tp);
               s = lmGenX.distToPlane(tp);
               t = lmGenY.distToPlane(tp);
               glMultiTexCoord2fARB(GL_TEXTURE1_ARB, s, t);
           }

           glVertex3f(p1.x, p1.y, p1.z);

           tp = p2;
           s = texGenX.distToPlane(tp);
           t = texGenY.distToPlane(tp);
           glMultiTexCoord2fARB(GL_TEXTURE0_ARB, s, t);

           if(lighting)
           {
               mLightingTransform.mulP(tp);
               s = lmGenX.distToPlane(tp);
               t = lmGenY.distToPlane(tp);
               glMultiTexCoord2fARB(GL_TEXTURE1_ARB, s, t);
           }

           glVertex3f(p2.x, p2.y, p2.z);

           glEnd();
       }

       if(renderLighting)
           glActiveTextureARB(GL_TEXTURE0_ARB);
    */

    return true;
}

bool ConvexBrush::renderEdges(ColorF color)
{
    /*
       // Save ModelView Matrix so we can restore Canonical state at exit.
        glPushMatrix();

       // Transform by the objects' transform e.g move it.
        dglMultMatrix(&mTransform);

       // Scale
       //glScalef(mScale.x, mScale.y, mScale.z);

       glBegin(GL_LINES);

       for (U32 i = 0; i < mFaces.mEdgeList.size(); i++)
       {
          OptimizedPolyList::Edge ed = mFaces.mEdgeList[i];
          //if (mFaces.mEdgeList[i].type == OptimizedPolyList::Convex)
          //   glColor3f(0.0f, 1.0f, 0.0f);
          //else if (mFaces.mEdgeList[i].type == OptimizedPolyList::Concave)
          //   glColor3f(1.0f, 0.0f, 0.0f);
          //else
          //   glColor3f(0.0f, 0.0f, 1.0f);

          glColor4f(color.red, color.green, color.blue, color.alpha);

          U32 i0 = mFaces.mEdgeList[i].vertexes[0];
          U32 i1 = mFaces.mEdgeList[i].vertexes[1];

          Point3F p0 = mFaces.mVertexList[i0];
          Point3F p1 = mFaces.mVertexList[i1];

          glVertex3f(p0.x, p0.y, p0.z);
          glVertex3f(p1.x, p1.y, p1.z);
       }

       glEnd();

       glPopMatrix();
    */

    return true;
}

bool ConvexBrush::calcBounds()
{
    // Rebuild our bounds
    if (mFaces.mVertexList.size() > 0)
    {
        mBounds.min.set(1e10, 1e10, 1e10);
        mBounds.max.set(-1e10, -1e10, -1e10);

        // Build our bounds
        for (U32 i = 0; i < mFaces.mVertexList.size(); i++)
        {
            mBounds.min.setMin(mFaces.mVertexList[i]);
            mBounds.max.setMax(mFaces.mVertexList[i]);
        }
    }

    return true;
}

bool ConvexBrush::setScale(F32 scale)
{
    mBrushScale = scale;

    // Scale our verts
    for (U32 i = 0; i < mFaces.mVertexList.size(); i++)
        mFaces.mVertexList[i] /= mBrushScale;

    // Scale the planes
    for (U32 i = 0; i < mFaces.mPlaneList.size(); i++)
        mFaces.mPlaneList[i].d /= mBrushScale;

    calcBounds();

    return true;
}

void ConvexBrush::addEdge(U16 zero, U16 one, U32 face)
{
    // Sort the edges
    U16 newEdge0, newEdge1;

    newEdge0 = getMin(zero, one);
    newEdge1 = getMax(zero, one);

    // Check the current edges
    bool found = false;
    U32 index = 0;

    for (index = 0; index < mFaces.mEdgeList.size(); index++)
    {
        if (mFaces.mEdgeList[index].vertexes[0] == newEdge0 && mFaces.mEdgeList[index].vertexes[1] == newEdge1)
        {
            found = true;
            break;
        }
    }

    // If we didn't find it add a new edge
    if (!found)
    {
        index = mFaces.mEdgeList.size();
        mFaces.mEdgeList.increment();
        mFaces.mEdgeList.last().vertexes[0] = newEdge0;
        mFaces.mEdgeList.last().vertexes[1] = newEdge1;
        mFaces.mEdgeList.last().faces[0] = -1;
        mFaces.mEdgeList.last().faces[1] = -1;
        mFaces.mEdgeList.last().type = OptimizedPolyList::NonShared;
    }

    // Add the face
    if (mFaces.mEdgeList[index].faces[0] == -1)
        mFaces.mEdgeList[index].faces[0] = face;
    else if (mFaces.mEdgeList[index].faces[1] == -1)
        mFaces.mEdgeList[index].faces[1] = face;
};

bool ConvexBrush::generateEdgelist()
{
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        // Add all of the edges of the polygon
        for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount - 1; j++)
        {
            U32 i0 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j];
            U32 i1 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j + 1];

            addEdge(i0, i1, i);
        }

        // Add the connecting edge from the last vertex to the first
        U32 i0 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart];
        U32 i1 = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + mFaces.mPolyList[i].vertexCount - 1];

        addEdge(i0, i1, i);
    }

    return true;
}

U32 ConvexBrush::getPolySide(S32 side)
{
    if (side == PlaneF::Back)
        return Back;
    else if (side == PlaneF::Front)
        return Front;
    else if (side == PlaneF::On)
        return On;
    else
        return Unknown;
}

bool ConvexBrush::isInsideBox(PlaneF left, PlaneF right, PlaneF top, PlaneF bottom)
{
    if (mStatus == Deleted)
        return false;

    // If the brush is in front of any of the planes then it is outside
    U32 side = whichSide(left);
    if (side == Front || side == Unknown)
        return false;
    side = whichSide(right);
    if (side == Front || side == Unknown)
        return false;
    side = whichSide(top);
    if (side == Front || side == Unknown)
        return false;
    side = whichSide(bottom);
    if (side == Front || side == Unknown)
        return false;

    return true;
}

U32 ConvexBrush::whichSide(PlaneF pln)
{
    if (mFaces.mPolyList.size() == 0)
        return Unknown;

    // Find out which side the first face is on
    U32 side = On;

    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        U32 nsd = whichSide(pln, i);

        if (nsd == Split)
            return Split;
        else if (nsd != On)
            side = nsd;
    }

    if (side == On)
        Con::errorf("ConvexBrush::whichSide(): brush has no volume");

    return side;
}

U32 ConvexBrush::whichSide(PlaneF pln, U32 faceIndex)
{
    if (faceIndex > mFaces.mPolyList.size())
        return Unknown;

    OptimizedPolyList::Poly* face = &mFaces.mPolyList[faceIndex];

    Point3F currv, nextv;
    S32 csd, nsd;

    U32 side = On;

    // Find out which side the first vert is on
    U32 idx = mFaces.mIndexList[face->vertexStart];
    currv = mFaces.mVertexList[idx];

    csd = pln.whichSide(currv);

    if (csd != PlaneF::On)
        side = getPolySide(csd);

    for (U32 k = 1; k < face->vertexCount; k++)
    {
        idx = mFaces.mIndexList[face->vertexStart + k];
        nextv = mFaces.mVertexList[idx];

        nsd = pln.whichSide(nextv);

        if ((csd == PlaneF::Back && nsd == PlaneF::Front) ||
            (csd == PlaneF::Front && nsd == PlaneF::Back))
        {
            return Split;
        }
        else if (nsd != PlaneF::On)
            side = getPolySide(nsd);

        currv = nextv;
        csd = nsd;
    }

    // Loop back to the first vert
    idx = mFaces.mIndexList[face->vertexStart];
    nextv = mFaces.mVertexList[idx];

    nsd = pln.whichSide(nextv);

    if ((csd == PlaneF::Back && nsd == PlaneF::Front) ||
        (csd == PlaneF::Front && nsd == PlaneF::Back))
    {
        return Split;
    }
    else if (nsd != PlaneF::On)
        side = getPolySide(nsd);

    return side;
}

bool ConvexBrush::getPolyList(AbstractPolyList* list)
{
    if (mFaces.mVertexList.size() == 0 || mFaces.mPolyList.size() == 0)
    {
        Con::errorf("ConvexBrush::getPolyList(): no verts or faces");
        return false;
    }

    PlaneTransformer trans;
    trans.set(mTransform, Point3F(1.0f, 1.0f, 1.0f));

    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        if (mFaces.mPolyList[i].vertexCount < 3)
        {
            Con::errorf("ConvexBrush::getPolyList(): ran into a face with less than 3 verts");
            continue;
        }

        list->begin(0, i);

        for (U32 j = 0; j < mFaces.mPolyList[i].vertexCount; j++)
        {
            U32 idx = mFaces.mIndexList[mFaces.mPolyList[i].vertexStart + j];
            Point3F pt = mFaces.mVertexList[idx];

            // Transform the point by the brush transform
            mTransform.mulP(pt);

            U32 newidx = list->addPoint(pt);
            list->vertex(newidx);
        }

        PlaneF pln = mFaces.mPlaneList[mFaces.mPolyList[i].plane];
        //pln.invert();

        PlaneF temp;
        trans.transform(pln, temp);

        U32 pdx = list->addPlane(temp);
        list->plane(pdx);

        list->end();
    }

    return true;
}

bool ConvexBrush::splitBrush(PlaneF pln, ConvexBrush* fbrush, ConvexBrush* bbrush)
{
    F32 scale = mBrushScale;

    // Scale the brush up
    setScale(1.0f / scale);
    pln.d *= scale;

    // Loop through the brush faces and sort them
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        U32 side = whichSide(pln, i);

        if (side == Front)
            fbrush->addPlane(mFaces.mPlaneList[mFaces.mPolyList[i].plane]);
        else if (side == Back)
            bbrush->addPlane(mFaces.mPlaneList[mFaces.mPolyList[i].plane]);
        else if (side == Split)
        {
            fbrush->addPlane(mFaces.mPlaneList[mFaces.mPolyList[i].plane]);
            bbrush->addPlane(mFaces.mPlaneList[mFaces.mPolyList[i].plane]);
        }
        else
        {
            Con::errorf("BspTree::splitBrush(): this brush can not be split");
            return false;
        }
    }

    // Next push the split plane into both
    bbrush->addPlane(pln);
    fbrush->addPlane(PlaneF(-pln.x, -pln.y, -pln.z, -pln.d));

    // Now generate the brushes with the planes supplied
    fbrush->processBrush();
    bbrush->processBrush();

    // Now that we've generated the new polys copy over the properties
    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        // We can match the new polys up with the old ones using their planes
        PlaneF oldpln = mFaces.mPlaneList[mFaces.mPolyList[i].plane];

        for (U32 j = 0; j < fbrush->mFaces.mPolyList.size(); j++)
        {
            PlaneF newpln = fbrush->mFaces.mPlaneList[fbrush->mFaces.mPolyList[j].plane];

            if (oldpln == newpln && oldpln.d == newpln.d)
            {
                // Copy over the properties
                fbrush->mFaces.mPolyList[j].object = mFaces.mPolyList[i].object;
                fbrush->mFaces.mPolyList[j].material = mFaces.mPolyList[i].material;
                fbrush->mFaces.mPolyList[j].material = mFaces.mPolyList[i].surfaceKey;
            }
        }

        for (U32 j = 0; j < bbrush->mFaces.mPolyList.size(); j++)
        {
            PlaneF newpln = bbrush->mFaces.mPlaneList[bbrush->mFaces.mPolyList[j].plane];

            if (oldpln == newpln && oldpln.d == newpln.d)
            {
                // Copy over the properties
                bbrush->mFaces.mPolyList[j].object = mFaces.mPolyList[i].object;
                bbrush->mFaces.mPolyList[j].material = mFaces.mPolyList[i].material;
                bbrush->mFaces.mPolyList[j].material = mFaces.mPolyList[i].surfaceKey;
            }
        }
    }

    // Now find the new poly and flag it as a non-solid face
    for (U32 i = 0; i < fbrush->mFaces.mPolyList.size(); i++)
    {
        PlaneF newpln = fbrush->mFaces.mPlaneList[fbrush->mFaces.mPolyList[i].plane];

        if (pln.x == -newpln.x && pln.y == -newpln.y && pln.z == -newpln.z && pln.d == -newpln.d)
            fbrush->mFaces.mPolyList[i].material = -2;
    }

    for (U32 i = 0; i < bbrush->mFaces.mPolyList.size(); i++)
    {
        PlaneF newpln = bbrush->mFaces.mPlaneList[bbrush->mFaces.mPolyList[i].plane];

        if (pln == newpln && pln.d == newpln.d)
            bbrush->mFaces.mPolyList[i].material = -2;
    }

    // scale everything back down
    fbrush->setScale(scale);
    bbrush->setScale(scale);

    setScale(scale);

    return true;
}

bool ConvexBrush::castRay(const Point3F& s, const Point3F& e, RayInfo* info)
{
    if (mStatus == Deleted)
        return false;

    // First transform the start and end points into the brushes space
    Point3F start, end;
    MatrixF invTrans = mTransform;
    invTrans.inverse();
    invTrans.mulP(s, &start);
    invTrans.mulP(e, &end);
    start.convolveInverse(mScale);
    end.convolveInverse(mScale);

    // Ganked from TSMesh::castRay()
    // Keep track of startTime and endTime.  They start out at just under 0 and just over 1, respectively.
    // As we check against each plane, prune start and end times back to represent current intersection of
    // line with all the planes (or rather with all the half-spaces defined by the planes).
    // But, instead of explicitly keeping track of startTime and endTime, keep track as numerator and denominator
    // so that we can avoid as many divisions as possible.

    //   F32 startTime = -0.01f;
    F32 startNum = -0.01f;
    F32 startDen = 1.00f;
    //   F32 endTime   = 1.01f;
    F32 endNum = 1.01f;
    F32 endDen = 1.00f;

    S32 curPlane = 0;
    U32 curMaterial = 0;
    bool found = false;

    // the following block of code is an optimization...
    // it isn't necessary if the longer version of the main loop is used
    bool tmpFound;
    S32 tmpPlane;
    F32 sgn = -1.0f;
    F32* pnum = &startNum;
    F32* pden = &startDen;
    S32* pplane = &curPlane;
    bool* pfound = &found;

    for (U32 i = 0; i < mFaces.mPlaneList.size(); i++)
    {
        Point3F planeNormal = mFaces.mPlaneList[i];
        F32 planeConstant = mFaces.mPlaneList[i].d;

        // if start & end outside, no collision
        // if start & end inside, continue
        // if start outside, end inside, or visa versa, find intersection of line with plane
        //    then update intersection of line with hull (using startTime and endTime)
        F32 dot1 = mDot(planeNormal, start) + planeConstant;
        F32 dot2 = mDot(planeNormal, end) + planeConstant;
        if (dot1 * dot2 > 0.0f)
        {
            // same side of the plane...which side -- dot==0 considered inside
            if (dot1 > 0.0f)
                // start and end outside of this plane, no collision
                return false;
            // start and end inside plane, continue
            continue;
        }

        AssertFatal(dot1 / (dot1 - dot2) >= 0.0f && dot1 / (dot1 - dot2) <= 1.0f, "ConvexBrush::castRay (1)");

        // find intersection (time) with this plane...
        // F32 time = dot1 / (dot1-dot2);
        F32 num = mFabs(dot1);
        F32 den = mFabs(dot1 - dot2);

        // the following block of code is an optimized version...
        // this can be commented out and the following block of code used instead
        // if debugging a problem in this code, that should probably be done
        // if you want to see how this works, look at the following block of code,
        // not this one...
        // Note that this does not get optimized appropriately...it is included this way
        // as an idea for future optimization.
        if (sgn * dot1 >= 0)
        {
            sgn *= -1.0f;
            pnum = (F32*)((dsize_t)pnum ^ (dsize_t)&endNum ^ (dsize_t)&startNum);
            pden = (F32*)((dsize_t)pden ^ (dsize_t)&endDen ^ (dsize_t)&startDen);
            pplane = (S32*)((dsize_t)pplane ^ (dsize_t)&tmpPlane ^ (dsize_t)&curPlane);
            pfound = (bool*)((dsize_t)pfound ^ (dsize_t)&tmpFound ^ (dsize_t)&found);
        }
        bool noCollision = num * endDen * sgn < endNum* den* sgn&& num* startDen* sgn < startNum* den* sgn;
        if (num * *pden * sgn < *pnum * den * sgn && !noCollision)
        {
            *pnum = num;
            *pden = den;
            *pplane = i;
            *pfound = true;
        }
        else if (noCollision)
            return false;
    }

    // setup rayInfo
    if (found && info)
    {
        info->t = (F32)startNum / (F32)startDen; // finally divide...
        info->normal = mFaces.mPlaneList[curPlane];
        // Find the face that uses this plane and its material
        for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
        {
            if (mFaces.mPolyList[i].plane == curPlane)
            {
                info->material = mFaces.mPolyList[i].material;
                break;
            }
        }

        return true;
    }
    else if (found)
        return true;

    // only way to get here is if start is inside hull...
    // we could return null and just plug in garbage for the material and normal...   
    return false;
}

void ConvexBrush::resetBrush()
{
    mBounds.min = Point3F(0, 0, 0);
    mBounds.max = Point3F(0, 0, 0);

    mCentroid = Point3F(0.0f, 0.0f, 0.0f);
    mStatus = Unprocessed;

    // Set our transform to no translation or rotation
    mTransform.identity();

    mScale.set(1.0f, 1.0f, 1.0f);
    mRotation.set(EulerF(0.0f, 0.0f, 0.0f));

    // Data
    mFaces.clear();
    mTexInfos.clear();
}

OptimizedPolyList ConvexBrush::getIntersectingPolys(OptimizedPolyList* list)
{
    OptimizedPolyList ret;

    for (U32 i = 0; i < list->mPolyList.size(); i++)
    {
        if (isPolyInside(list, i))
            list->copyPolyToList(&ret, i);
    }

    return ret;
}

OptimizedPolyList ConvexBrush::getNonIntersectingPolys(OptimizedPolyList* list)
{
    OptimizedPolyList ret;

    for (U32 i = 0; i < list->mPolyList.size(); i++)
    {
        if (!isPolyInside(list, i))
            list->copyPolyToList(&ret, i);
    }

    return ret;
}

bool ConvexBrush::isPolyInside(OptimizedPolyList* list, U32 pdx)
{
    // This relies on the brush to be convex
    bool inside = true;

    OptimizedPolyList::Poly& poly = list->mPolyList[pdx];

    for (U32 i = 0; i < mFaces.mPolyList.size(); i++)
    {
        OptimizedPolyList::Poly& face = mFaces.mPolyList[i];
        PlaneF& pln = mFaces.mPlaneList[face.plane];

        bool behind = true;

        for (U32 j = 0; j < poly.vertexCount; j++)
        {
            U32 idx = list->mIndexList[j + poly.vertexStart];
            Point3F& pt = list->mVertexList[idx];

            U32 tside = pln.whichSide(pt);

            if (tside == PlaneF::Front)
            {
                behind = false;
                break;
            }
        }

        if (!behind)
        {
            inside = false;
            break;
        }
    }

    return inside;
}

