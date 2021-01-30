//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "collision/polyhedron.h"


//----------------------------------------------------------------------------

void Polyhedron::buildBox(const MatrixF& transform, const Box3F& box)
{
    // Box is assumed to be axis aligned in the source space.
    // Transform into geometry space
    Point3F xvec, yvec, zvec, min;
    transform.getColumn(0, &xvec);
    xvec *= box.len_x();
    transform.getColumn(1, &yvec);
    yvec *= box.len_y();
    transform.getColumn(2, &zvec);
    zvec *= box.len_z();
    transform.mulP(box.min, &min);

    // Initial vertices
    pointList.setSize(8);
    pointList[0] = min;
    pointList[1] = min + yvec;
    pointList[2] = min + xvec + yvec;
    pointList[3] = min + xvec;
    pointList[4] = pointList[0] + zvec;
    pointList[5] = pointList[1] + zvec;
    pointList[6] = pointList[2] + zvec;
    pointList[7] = pointList[3] + zvec;

    // Initial faces
    planeList.setSize(6);
    planeList[0].set(pointList[0], xvec);
    planeList[0].invert();
    planeList[1].set(pointList[2], yvec);
    planeList[2].set(pointList[2], xvec);
    planeList[3].set(pointList[0], yvec);
    planeList[3].invert();
    planeList[4].set(pointList[0], zvec);
    planeList[4].invert();
    planeList[5].set(pointList[4], zvec);

    // The edges are constructed so that the vertices
    // are oriented clockwise for face[0]
    edgeList.setSize(12);
    Edge* edge = edgeList.begin();
    S32 nextEdge = 0;
    for (int i = 0; i < 4; i++) {
        S32 n = (i == 3) ? 0 : i + 1;
        S32 p = (i == 0) ? 3 : i - 1;
        edge->vertex[0] = i;
        edge->vertex[1] = n;
        edge->face[0] = i;
        edge->face[1] = 4;
        edge++;
        edge->vertex[0] = 4 + i;
        edge->vertex[1] = 4 + n;
        edge->face[0] = 5;
        edge->face[1] = i;
        edge++;
        edge->vertex[0] = i;
        edge->vertex[1] = 4 + i;
        edge->face[0] = p;
        edge->face[1] = i;
        edge++;
    }
}


//----------------------------------------------------------------------------

void Polyhedron::render()
{
    /*
       glVertexPointer(3,GL_FLOAT,sizeof(Point3F),pointList.address());
       glEnableClientState(GL_VERTEX_ARRAY);
       glColor3f(1, 0, 1);

       for (S32 e = 0; e < edgeList.size(); e++)
          glDrawElements(GL_LINES,2,GL_UNSIGNED_INT,&edgeList[e].vertex);

       for (U32 i = 0; i < planeList.size(); i++) {
          Point3F origin(0, 0, 0);
          U32 num = 0;
          for (U32 j = 0; j < edgeList.size(); j++) {
             if (edgeList[j].face[0] == i || edgeList[j].face[1] == i) {
                origin += pointList[edgeList[j].vertex[0]];
                origin += pointList[edgeList[j].vertex[1]];
                num += 2;
             }
          }
          origin /= F32(num);

          glColor3f(1, 1, 1);
          glBegin(GL_LINES);
             glVertex3fv(origin);
             glVertex3f(origin.x + planeList[i].x * 0.2,
                        origin.y + planeList[i].y * 0.2,
                        origin.z + planeList[i].z * 0.2);
          glEnd();
       }

       glDisableClientState(GL_VERTEX_ARRAY);
    */
}

void Polyhedron::render(const VectorF& vec, F32 time)
{
    /*
       bool faceVisible[50];
       for (int f = 0; f < planeList.size(); f++)
          faceVisible[f] = mDot(planeList[f],vec) > 0;

       VectorF tvec = vec;
       tvec *= time;
       for (int e = 0; e < edgeList.size(); e++) {
          Polyhedron::Edge const& edge = edgeList[e];
          if (faceVisible[edge.face[0]] || faceVisible[edge.face[1]]) {
             Point3F pp;

             glBegin(GL_LINE_LOOP);
             glColor3f(0.5,0.5,0.5);
             const Point3F& p1 = pointList[edge.vertex[0]];
             const Point3F& p2 = pointList[edge.vertex[1]];
             glVertex3fv(p1);
             glVertex3fv(p2);
             pp = p2 + vec;
             glVertex3fv(pp);
             pp = p1 + vec;
             glVertex3fv(pp);
             glEnd();

             if (time <= 1.0) {
                glBegin(GL_LINES);
                glColor3f(0.5,0.5,0);
                pp = pointList[edge.vertex[0]];
                pp += tvec;
                glVertex3fv(pp);
                pp = pointList[edge.vertex[1]];
                pp += tvec;
                glVertex3fv(pp);
                glEnd();
             }
          }
       }
    */
}
