//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsSortedMesh.h"
#include "math/mMath.h"
#include "ts/tsShapeInstance.h"

// Not worth the effort, much less the effort to comment, but if the draw types
// are consecutive use addition rather than a table to go from index to command value...
/*
#if ((GL_TRIANGLES+1==GL_TRIANGLE_STRIP) && (GL_TRIANGLE_STRIP+1==GL_TRIANGLE_FAN))
   #define getDrawType(a) (GL_TRIANGLES+(a))
#else
   U32 drawTypes[] = { GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN };
   #define getDrawType(a) (drawTypes[a])
#endif
*/

// found in tsmesh
extern void forceFaceCamera();
extern void forceFaceCameraZAxis();

//-----------------------------------------------------
// TSSortedMesh render methods
//-----------------------------------------------------

void TSSortedMesh::render(S32 frame, S32 matFrame, TSMaterialList* materials)
{
    /*
       if (getFlags(Billboard))
       {
          if (getFlags(BillboardZAxis))
             forceFaceCameraZAxis();
          else
             forceFaceCamera();
       }

       MatrixF toCam;
       dglGetModelview(&toCam);
       toCam.inverse();
       Point3F cameraCenter;
       toCam.getColumn(3,&cameraCenter);

       S32 firstVert  = firstVerts[frame];
       S32 vertCount  = numVerts[frame];
       // note on the following:  a TSSortedMesh can animate frame or matFrame but not both.
       // The reason for this is simple:  the number of verts change depending on frame, so
       // if both varied then there would have to be a new set of tverts for every (frame x matFrame)
       // combination, rather than for every matFrame as for other mesh types.  However, either frame
       // or matFrame is allowed to vary, accounting for the following odd looking statement.
       S32 firstTVert = firstTVerts[matFrame ? matFrame : frame];

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);
       glNormalPointer(GL_FLOAT,0,getNormals(firstVert));
       glTexCoordPointer(2,GL_FLOAT,0,&tverts[firstTVert]);
       if (TSShapeInstance::smRenderData.detailMapMethod == TSShapeInstance::DETAIL_MAP_MULTI_1 ||
           TSShapeInstance::smRenderData.detailMapMethod == TSShapeInstance::DETAIL_MAP_MULTI_2)
       {
          glClientActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.detailMapTE);
          glTexCoordPointer(2,GL_FLOAT,0,&tverts[firstTVert]);
          glClientActiveTextureARB(GL_TEXTURE0_ARB + TSShapeInstance::smRenderData.baseTE);
       }

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertCount);

       if (alwaysWriteDepth)
          glDepthMask(GL_TRUE);

       Cluster * cluster;
       S32 nextCluster = startCluster[frame];
       do
       {
          // the cluster...
          cluster = &clusters[nextCluster];

          // render the cluster...
          for (S32 i=cluster->startPrimitive; i<cluster->endPrimitive; i++)
          {
             TSDrawPrimitive & draw = primitives[i];
             AssertFatal((draw.matIndex & TSDrawPrimitive::Indexed)!=0,"TSSortedMesh::render: rendering of non-indexed meshes no longer supported");

             // material change?
             if ( (TSShapeInstance::smRenderData.materialIndex ^ draw.matIndex) & (TSDrawPrimitive::MaterialMask|TSDrawPrimitive::NoMaterial))
             {
                setMaterial(draw.matIndex,materials);
                if (alwaysWriteDepth)
                   glDepthMask(GL_TRUE);
             }

             S32 drawType  = getDrawType(draw.matIndex>>30);

             glDrawElements(drawType,draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
          }

          // determine next cluster...
          if (cluster->frontCluster!=cluster->backCluster)
             nextCluster = (mDot(cluster->normal,cameraCenter) > cluster->k) ? cluster->frontCluster : cluster->backCluster;
          else
             nextCluster = cluster->frontCluster;
       } while (nextCluster>=0);

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();

       if ((TSShapeInstance::smRenderData.materialFlags & TSMaterialList::Translucent) && alwaysWriteDepth)
          glDepthMask(GL_FALSE);
    */
}

void TSSortedMesh::renderFog(S32 frame)
{
    /*
       if (getFlags(Billboard))
       {
          if (getFlags(BillboardZAxis))
             forceFaceCameraZAxis();
          else
             forceFaceCamera();
       }

       MatrixF toCam;
       dglGetModelview(&toCam);
       toCam.inverse();
       Point3F cameraCenter;
       toCam.getColumn(3,&cameraCenter);

       S32 firstVert  = firstVerts[frame];
       S32 vertCount  = numVerts[frame];

       // set up vertex arrays -- already enabled in TSShapeInstance::render
       glVertexPointer(3,GL_FLOAT,0,&verts[firstVert]);

       // lock...
       bool lockArrays = dglDoesSupportCompiledVertexArray();
       if (lockArrays)
          glLockArraysEXT(0,vertCount);

       Cluster * cluster;
       S32 nextCluster = startCluster[frame];
       do
       {
          // the cluster...
          cluster = &clusters[nextCluster];

          // render the cluster...
          for (S32 i=cluster->startPrimitive; i<cluster->endPrimitive; i++)
          {
             TSDrawPrimitive & draw = primitives[i];
             glDrawElements(getDrawType(draw.matIndex>>30),draw.numElements,GL_UNSIGNED_SHORT,&indices[draw.start]);
          }

          // determine next cluster...
          if (cluster->frontCluster!=cluster->backCluster)
             nextCluster = (mDot(cluster->normal,cameraCenter) > cluster->k) ? cluster->frontCluster : cluster->backCluster;
          else
             nextCluster = cluster->frontCluster;
       } while (nextCluster>=0);

       // unlock...
       if (lockArrays)
          glUnlockArraysEXT();
    */
}

//-----------------------------------------------------
// TSSortedMesh collision methods
//-----------------------------------------------------

bool TSSortedMesh::buildPolyList(S32 frame, AbstractPolyList* polyList, U32& surfaceKey)
{
    frame, polyList, surfaceKey;
    return false;
}

bool TSSortedMesh::castRay(S32 frame, const Point3F& start, const Point3F& end, RayInfo* rayInfo)
{
    frame, start, end, rayInfo;
    return false;
}

bool TSSortedMesh::buildConvexHull()
{
    return false;
}

S32 TSSortedMesh::getNumPolys()
{
    S32 count = 0;
    S32 cIdx = !clusters.size() ? -1 : 0;
    while (cIdx >= 0)
    {
        Cluster& cluster = clusters[cIdx];
        for (S32 i = cluster.startPrimitive; i < cluster.endPrimitive; i++)
        {
            if (primitives[i].matIndex & TSDrawPrimitive::Triangles)
                count += primitives[i].numElements / 3;
            else
                count += primitives[i].numElements - 2;
        }
        cIdx = cluster.frontCluster; // always use frontCluster...we assume about the same no matter what
    }
    return count;
}

//-----------------------------------------------------
// TSSortedMesh assembly/dissembly methods
// used for transfer to/from memory buffers
//-----------------------------------------------------

#define alloc TSShape::alloc

void TSSortedMesh::assemble(bool skip)
{
    bool save1 = TSMesh::smUseTriangles;
    bool save2 = TSMesh::smUseOneStrip;
    TSMesh::smUseTriangles = false;
    TSMesh::smUseOneStrip = false;

    TSMesh::assemble(skip);

    TSMesh::smUseTriangles = save1;
    TSMesh::smUseOneStrip = save2;

    S32 numClusters = alloc.get32();
    S32* ptr32 = alloc.copyToShape32(numClusters * 8);
    clusters.set(ptr32, numClusters);

    S32 sz = alloc.get32();
    ptr32 = alloc.copyToShape32(sz);
    startCluster.set(ptr32, sz);

    sz = alloc.get32();
    ptr32 = alloc.copyToShape32(sz);
    firstVerts.set(ptr32, sz);

    sz = alloc.get32();
    ptr32 = alloc.copyToShape32(sz);
    numVerts.set(ptr32, sz);

    sz = alloc.get32();
    ptr32 = alloc.copyToShape32(sz);
    firstTVerts.set(ptr32, sz);

    alwaysWriteDepth = alloc.get32() != 0;

    alloc.checkGuard();
}

void TSSortedMesh::disassemble()
{
    TSMesh::disassemble();

    alloc.set32(clusters.size());
    alloc.copyToBuffer32((S32*)clusters.address(), clusters.size() * 8);

    alloc.set32(startCluster.size());
    alloc.copyToBuffer32((S32*)startCluster.address(), startCluster.size());

    alloc.set32(firstVerts.size());
    alloc.copyToBuffer32((S32*)firstVerts.address(), firstVerts.size());

    alloc.set32(numVerts.size());
    alloc.copyToBuffer32((S32*)numVerts.address(), numVerts.size());

    alloc.set32(firstTVerts.size());
    alloc.copyToBuffer32((S32*)firstTVerts.address(), firstTVerts.size());

    alloc.set32(alwaysWriteDepth ? 1 : 0);

    alloc.setGuard();
}



