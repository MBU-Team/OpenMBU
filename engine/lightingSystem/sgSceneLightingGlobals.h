//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------


static const Point3F BoxNormals[] =
{
    Point3F(1, 0, 0),
        Point3F(-1, 0, 0),
        Point3F(0, 1, 0),
        Point3F(0,-1, 0),
        Point3F(0, 0, 1),
        Point3F(0, 0,-1)
};

static U32 BoxVerts[][4] = {
    {7,6,4,5},     // +x
    {0,2,3,1},     // -x
    {7,3,2,6},     // +y
    {0,1,5,4},     // -y
    {7,5,1,3},     // +z
    {0,4,6,2}      // -z
};

static U32 BoxSharedEdgeMask[][6] = {
    {0, 0, 1, 4, 8, 2},
    {0, 0, 2, 8, 4, 1},
    {8, 2, 0, 0, 1, 4},
    {4, 1, 0, 0, 2, 8},
    {1, 4, 8, 2, 0, 0},
    {2, 8, 4, 1, 0, 0}
};

static U32 TerrainSquareIndices[][3] = {
    {2, 1, 0},  // 45
    {3, 2, 0},
    {3, 1, 0},  // 135
    {3, 2, 1}
};

static Point3F BoxPnts[] = {
    Point3F(0,0,0),
        Point3F(0,0,1),
        Point3F(0,1,0),
        Point3F(0,1,1),
        Point3F(1,0,0),
        Point3F(1,0,1),
        Point3F(1,1,0),
        Point3F(1,1,1)
};

extern SceneLighting* gLighting;
extern F32 gParellelVectorThresh;
extern F32 gPlaneNormThresh;
extern F32 gPlaneDistThresh;

