//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/fluid.h"

//==============================================================================
//  VARIABLES
//==============================================================================

fluid::vertex* fluid::m_pVertex = NULL;
s32             fluid::m_VAllocated = 0;
s32             fluid::m_VUsed = 0;

s16* fluid::m_pIndex = NULL;
s16* fluid::m_pINext = NULL;
s16             fluid::m_IOffset = 0;
s32             fluid::m_IAllocated = 0;
s32             fluid::m_IUsed = 0;

static  f32     sSurfaceAtEye;
static  f32     sFogZ;
//atic  f32     sFogTable[64];
//atic  f32     sFogAccessFactor;

//==============================================================================
//  FUNCTIONS
//==============================================================================

#define L0  150.0f
#define L1   75.0f

f32 fluid::ComputeLOD(f32 Distance)
{
    f32 LOD;
    if (Distance > L0)    return(0.0f);
    if (Distance < L1)    return(1.0f);
    LOD = (L0 - Distance) / (L0 - L1);
    return(LOD);
}

#undef L0
#undef L1

//==============================================================================
// Frustrum clip planes: 0=T 1=B 2=L 3=R 4=N 5=F

byte fluid::ComputeClipBits(f32 X, f32 Y, f32 Z)
{
    byte Result = 0x00;
    s32  i;

    for (i = 0; i < 6; i++)
        if (((X * m_Plane[i].A) + (Y * m_Plane[i].B) + (Z * m_Plane[i].C)) < -m_Plane[i].D)
            Result |= (1 << i);

    return(Result);
}

//==============================================================================

fluid::vertex* fluid::AcquireVertices(s32 Count)
{
    vertex* pResult;

    if (m_VAllocated == 0)
    {
        m_VAllocated = 100;
        m_pVertex = (vertex*)MALLOC(m_VAllocated * sizeof(vertex));
        ASSERT(m_pVertex);
    }

    while (Count + m_VUsed > m_VAllocated)
    {
        m_VAllocated += 100;
        m_pVertex = (vertex*)REALLOC(m_pVertex, m_VAllocated * sizeof(vertex));
        ASSERT(m_pVertex);
    }

    pResult = m_pVertex + m_VUsed;
    m_IOffset = m_VUsed;
    m_VUsed += Count;

    return(pResult);
}

//==============================================================================

void fluid::AddTriangleIndices(s16 I1, s16 I2, s16 I3)
{
    if (m_IAllocated == 0)
    {
        m_IAllocated = 100;
        m_IUsed = 0;
        m_pIndex = (s16*)MALLOC(m_IAllocated * sizeof(s16));
        m_pINext = m_pIndex;
        ASSERT(m_pIndex);
    }

    if (m_IUsed + 3 > m_IAllocated)
    {
        s32 Next = m_pINext - m_pIndex;
        m_IAllocated += 100;
        m_pIndex = (s16*)REALLOC(m_pIndex, m_IAllocated * sizeof(s16));
        m_pINext = m_pIndex + Next;
        ASSERT(m_pIndex);
    }

    m_IUsed += 3;

    *m_pINext = m_IOffset + I1;  m_pINext++;
    *m_pINext = m_IOffset + I2;  m_pINext++;
    *m_pINext = m_IOffset + I3;  m_pINext++;
}

//==============================================================================

void fluid::ReleaseVertexMemory(void)
{
    if (m_pVertex)
    {
        FREE(m_pVertex);
        m_pVertex = NULL;
        m_VAllocated = 0;
        m_VUsed = 0;
    }

    if (m_pIndex)
    {
        FREE(m_pIndex);
        m_pINext = NULL;
        m_pIndex = NULL;
        m_IAllocated = 0;
        m_IUsed = 0;
    }
}

//==============================================================================
/*
void fluid::BuildFogTable( void )
{
    f32 Distance;
    f32 Delta;
    f32 Z;
    s32 i;

    Z        = m_SurfaceZ - m_Eye.Z;
    Delta    = m_VisibleDistance / 64.0f;
    Distance = 0.0f;

    for( i = 0; i < 64; i++ )
    {
        Distance    += Delta;
        sFogTable[i] = m_pFogFn( Distance, Z );
    }

    // And, create a multiplier we will use to access the table.
    sFogAccessFactor = 64.0f / m_VisibleDistance;
}
*/
//==============================================================================

void fluid::RunQuadTree(bool& EyeSubmerged)
{
    node    Stack[32];
    s32     Top = -1;
    s32     i, j;
    s32     I, J;
    s32     BlocksPerRep = m_HighResMode ? 64 : 32;

    // Build a fog sample table.
//  BuildFogTable();

    // We will use a single 'altitude' for all fog (regardless of waves).
    sFogZ = m_SurfaceZ - m_Eye.Z;

    // Determine where fluid surface is directly above (or above) the eye.
    sSurfaceAtEye = m_SurfaceZ + (SINE((m_Eye.X * 0.05f) + m_Seconds) +
        SINE((m_Eye.Y * 0.05f) + m_Seconds)) * m_WaveFactor;

    // Set a flag to be returned to the caller.
    EyeSubmerged = (m_Eye.Z < sSurfaceAtEye) && IsFluidAtXY(m_Eye.X, m_Eye.Y);

    // Prepare to accumulate vertices and indices.
    m_VUsed = 0;
    m_IUsed = 0;
    m_pINext = m_pIndex;

    // Determine what "rep" of the terrain we are in.
    I = (s32)(m_Eye.X / 2048.0f);
    J = (s32)(m_Eye.Y / 2048.0f);
    if (m_Eye.X < 0.0f)  I--;
    if (m_Eye.Y < 0.0f)  J--;

    // Push 9 reps of the fluid onto the stack.
    for (j = J - 1; j <= J + 1; j++)
        for (i = I - 1; i <= I + 1; i++)
        {
            // New stack node.
            Top++;

            // Set up the level based masking.
            Stack[Top].Level = 0;
            Stack[Top].MaskIndexX = 0;
            Stack[Top].MaskIndexY = 0;

            // Set up "block" coordinates and factor in the rep of the terrain.
            Stack[Top].BlockX0 = (i * BlocksPerRep);
            Stack[Top].BlockY0 = (j * BlocksPerRep);

            // Note that the MaskIndexX and the BlockX0 are similar, but not the
            // same.  The BlockX0 is affected by the rep of the terrain, whereas
            // MaskIndexX is not.  Note that neither takes into consideration
            // the offset of the fluid within a terrain rep.

            // Compute world coordinates.  Account for fluid offset within terrain.
            Stack[Top].X0 = (Stack[Top].BlockX0 * m_Step[4]) + (m_SquareX0 * 8.0f);
            Stack[Top].Y0 = (Stack[Top].BlockY0 * m_Step[4]) + (m_SquareY0 * 8.0f);
            Stack[Top].X1 = Stack[Top].X0 + 2048.0f;
            Stack[Top].Y1 = Stack[Top].Y0 + 2048.0f;

            // Compute clip bits for each corner.
            Stack[Top].ClipBits[0] = ComputeClipBits(Stack[Top].X0, Stack[Top].Y0, m_SurfaceZ);
            Stack[Top].ClipBits[1] = ComputeClipBits(Stack[Top].X1, Stack[Top].Y0, m_SurfaceZ);
            Stack[Top].ClipBits[2] = ComputeClipBits(Stack[Top].X0, Stack[Top].Y1, m_SurfaceZ);
            Stack[Top].ClipBits[3] = ComputeClipBits(Stack[Top].X1, Stack[Top].Y1, m_SurfaceZ);
        }

    //
    // Let the "recursion" begin.
    //

    while (Top >= 0)
    {
        node& Node = Stack[Top];

        ASSERT(Top < 32);
        ASSERT(Node.MaskIndexX < (1 << Node.Level));
        ASSERT(Node.MaskIndexY < (1 << Node.Level));

        // Attempt to trivially reject the whole node.  See if the clip bits
        // indicate that all 4 verts are outside one of the frustrum planes.
        if (Node.ClipBits[0] &
            Node.ClipBits[1] &
            Node.ClipBits[2] &
            Node.ClipBits[3])
        {
            Top--;
            continue;
        }

        // Attempt to trivially reject the whole node based on the mask bits.
        if (GetRejectBit(Node.Level, Node.MaskIndexX, Node.MaskIndexY))
        {
            Top--;
            continue;
        }

        // If we have reached level 5 (the node is a single block), then
        // accept it.
        if (Node.Level == 5)
        {
            ProcessNode(Node);
            Top--;
            continue;
        }

        // Attempt to trivially accept the whole node.  It must pass BOTH the
        // clip bits and the accept mask.
        if (((Node.ClipBits[0] | Node.ClipBits[1] |
            Node.ClipBits[2] | Node.ClipBits[3]) == 0x00) &&
            GetAcceptBit(Node.Level, Node.MaskIndexX, Node.MaskIndexY))
        {
            ProcessNode(Node);
            Top--;
            continue;
        }

        // If we are here, then we need to subdivide the node.  We will break
        // the node into 4 equal parts.
        //
        //  *-----*                  *--@--*
        //  |     |                  | 2| 3|
        //  | Top |     becomes...   @--@--@
        //  |     |                  | 0| 1|
        //  *-----*                  *--@--*
        //
        {
            node& Node0 = Stack[Top + 0];
            node& Node1 = Stack[Top + 1];
            node& Node2 = Stack[Top + 2];
            node& Node3 = Stack[Top + 3];
            s32     Blocks;

            // We're going to copy the original node onto all of the new nodes
            // in a moment.  Before we do that, we might as well make some
            // changes to the original node that will need to be made in all
            // of the new nodes anyways.  (Node and Node0 are the same thing.)

            Node.Level += 1;
            Node.MaskIndexX = Node.MaskIndexX << 1;
            Node.MaskIndexY = Node.MaskIndexY << 1;

            // The number of blocks across a new node is needed repeatedly
            // below.  Go ahead and compute it now.

            Blocks = (32 >> Node.Level);

            // Now, just copy everything from the original node into all of
            // the new nodes.

            Node3 = Node2 = Node1 = Node0;

            // Update Node 0.

            Node0.X1 = Node0.X0 + (Blocks * m_Step[4]);
            Node0.Y1 = Node0.Y0 + (Blocks * m_Step[4]);
            Node0.ClipBits[1] = ComputeClipBits(Node0.X1, Node0.Y0, m_SurfaceZ);
            Node0.ClipBits[2] = ComputeClipBits(Node0.X0, Node0.Y1, m_SurfaceZ);
            Node0.ClipBits[3] = ComputeClipBits(Node0.X1, Node0.Y1, m_SurfaceZ);

            // Update Node 1.  Take advantage of updated Node 0.

            Node1.MaskIndexX += 1;
            Node1.BlockX0 += Blocks;
            Node1.X0 = Node0.X1;
            Node1.Y1 = Node0.Y1;
            Node1.ClipBits[0] = Node0.ClipBits[1];
            Node1.ClipBits[2] = Node0.ClipBits[3];
            Node1.ClipBits[3] = ComputeClipBits(Node1.X1, Node1.Y1, m_SurfaceZ);

            // Update Node 2.  Take advantage of updated Node 0.

            Node2.MaskIndexY += 1;
            Node2.BlockY0 += Blocks;
            Node2.X1 = Node0.X1;
            Node2.Y0 = Node0.Y1;
            Node2.ClipBits[0] = Node0.ClipBits[2];
            Node2.ClipBits[1] = Node0.ClipBits[3];
            Node2.ClipBits[3] = ComputeClipBits(Node2.X1, Node2.Y1, m_SurfaceZ);

            // Update Node 3.  Take advantage of updated Nodes 0, 1, and 2.

            Node3.MaskIndexX += 1;
            Node3.MaskIndexY += 1;
            Node3.BlockX0 += Blocks;
            Node3.BlockY0 += Blocks;
            Node3.X0 = Node0.X1;
            Node3.Y0 = Node0.Y1;
            Node3.ClipBits[0] = Node0.ClipBits[3];
            Node3.ClipBits[1] = Node1.ClipBits[3];
            Node3.ClipBits[2] = Node2.ClipBits[3];

            Top += 3;
        }
    }
}

//==============================================================================

void fluid::ProcessNode(node& Node)
{
    /*
        // Quick debug render.
        if( m_ShowNodes )
        {
            glDisable       ( GL_TEXTURE_2D );
            glDisable       ( GL_DEPTH_TEST );
            glEnable        ( GL_BLEND );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glPolygonMode   ( GL_FRONT_AND_BACK, GL_LINE );
            glColor4f       ( 0.5f, 0.5f, 0.0f, 0.5f );
            glBegin         ( GL_QUADS );
            glVertex3f      ( Node.X0, Node.Y0, m_SurfaceZ );
            glVertex3f      ( Node.X1, Node.Y0, m_SurfaceZ );
            glVertex3f      ( Node.X1, Node.Y1, m_SurfaceZ );
            glVertex3f      ( Node.X0, Node.Y1, m_SurfaceZ );
            glEnd           ();
            glEnable        ( GL_DEPTH_TEST );
            glColor4f       ( 0.8f, 0.8f, 0.3f, 0.8f );
            glBegin         ( GL_QUADS );
            glVertex3f      ( Node.X0, Node.Y0, m_SurfaceZ );
            glVertex3f      ( Node.X1, Node.Y0, m_SurfaceZ );
            glVertex3f      ( Node.X1, Node.Y1, m_SurfaceZ );
            glVertex3f      ( Node.X0, Node.Y1, m_SurfaceZ );
            glEnd           ();
        }

        // For each node, render all the blocks it contains.

        s32 X, Y;
        s32 Blocks;

        Blocks = (32 >> Node.Level);

        for( Y = 0; Y < Blocks; Y++ )
        for( X = 0; X < Blocks; X++ )
        {
            block Block;

            Block.X0 = Node.X0  + X * m_Step[4];
            Block.Y0 = Node.Y0  + Y * m_Step[4];
            Block.X1 = Block.X0 +     m_Step[4];
            Block.Y1 = Block.Y0 +     m_Step[4];

            Block.Distance[0] = DISTANCE( Block.X0, Block.Y0, m_SurfaceZ );
            Block.Distance[1] = DISTANCE( Block.X1, Block.Y0, m_SurfaceZ );
            Block.Distance[2] = DISTANCE( Block.X0, Block.Y1, m_SurfaceZ );
            Block.Distance[3] = DISTANCE( Block.X1, Block.Y1, m_SurfaceZ );

            Block.LOD[0] = ComputeLOD( Block.Distance[0] );
            Block.LOD[1] = ComputeLOD( Block.Distance[1] );
            Block.LOD[2] = ComputeLOD( Block.Distance[2] );
            Block.LOD[3] = ComputeLOD( Block.Distance[3] );

            ProcessBlock( Block );
        }
    */
}

//==============================================================================

void fluid::ProcessBlock(block& Block)
{
    /*
        // Quick debug render.
        if( m_ShowBlocks )
        {
            glDisable       ( GL_TEXTURE_2D );
            glDisable       ( GL_DEPTH_TEST );
            glEnable        ( GL_BLEND );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glPolygonMode   ( GL_FRONT_AND_BACK, GL_LINE );
            glColor4f       ( 0.0f, 0.0f, 0.0f, 0.5f );
            glBegin         ( GL_QUADS );
            glVertex3f      ( Block.X0, Block.Y0, m_SurfaceZ );
            glVertex3f      ( Block.X1, Block.Y0, m_SurfaceZ );
            glVertex3f      ( Block.X1, Block.Y1, m_SurfaceZ );
            glVertex3f      ( Block.X0, Block.Y1, m_SurfaceZ );
            glEnd           ();
            glEnable        ( GL_DEPTH_TEST );
            glColor4f       ( 0.3f, 0.3f, 0.3f, 0.8f );
            glBegin         ( GL_QUADS );
            glVertex3f      ( Block.X0, Block.Y0, m_SurfaceZ );
            glVertex3f      ( Block.X1, Block.Y0, m_SurfaceZ );
            glVertex3f      ( Block.X1, Block.Y1, m_SurfaceZ );
            glVertex3f      ( Block.X0, Block.Y1, m_SurfaceZ );
            glEnd           ();
        }

        if( (Block.LOD[0] > 0.0f) &&
            (Block.LOD[1] > 0.0f) &&
            (Block.LOD[2] > 0.0f) &&
            (Block.LOD[3] > 0.0f) )
        {
            if( (Block.LOD[0] == 1.0f) &&
                (Block.LOD[1] == 1.0f) &&
                (Block.LOD[2] == 1.0f) &&
                (Block.LOD[3] == 1.0f) )
            {
                // LOD on ALL verts is ONE!
                ProcessBlockLODHigh( Block );
            }
            else
            {
                // LOD on ALL verts is NON-ZERO, but it is NOT ONE on ALL verts!
                ProcessBlockLODMorph( Block );
            }
        }
        else
        {
            if( (Block.LOD[0] > 0.0f) ||
                (Block.LOD[1] > 0.0f) ||
                (Block.LOD[2] > 0.0f) ||
                (Block.LOD[3] > 0.0f) )
            {
                // LOD on SOME verts is NON-ZERO, and on SOME it is ZERO!
                ProcessBlockLODTrans( Block );
            }
            else
            {
                // LOD on ALL verts is ZERO!
                ProcessBlockLODLow( Block );
            }
        }
    */
}

//==============================================================================

static s16 HighLODIndices[96] =
{
     0,  1,  6,     6,  5,  0,          //  +Y
     2,  7,  6,     6,  1,  2,          //  20--21--22--23--24
     2,  3,  8,     8,  7,  2,          //   | \ | / | \ | / |
     4,  9,  8,     8,  3,  4,          //  15--16--17--18--19
                                        //   | / | \ | / | \ |
     6, 11, 10,    10,  5,  6,          //  10--11--12--13--14
     6,  7, 12,    12, 11,  6,          //   | \ | / | \ | / |
     8, 13, 12,    12,  7,  8,          //  05--06--07--08--09
     8,  9, 14,    14, 13,  8,          //   | / | \ | / | \ |
                                        // [00]-01--02--03--04 +X
    10, 11, 16,    16, 15, 10,          //
    12, 17, 16,    16, 11, 12,
    12, 13, 18,    18, 17, 12,
    14, 19, 18,    18, 13, 14,

    16, 21, 20,    20, 15, 16,
    16, 17, 22,    22, 21, 16,
    18, 23, 22,    22, 17, 18,
    18, 19, 24,    24, 23, 18,
};

//==============================================================================

void fluid::ProcessBlockLODHigh(block& Block)
{
    vertex* pV;
    s32     X, Y, i;
    f32     x, y;
    s16* pIndex = HighLODIndices;
    f32     Distance;

    // Get vetices.
    pV = AcquireVertices(25);

    // Put data in the corner verts.
    SetupVert(Block.X0, Block.Y0, Block.Distance[0], pV + 0);
    SetupVert(Block.X1, Block.Y0, Block.Distance[1], pV + 4);
    SetupVert(Block.X0, Block.Y1, Block.Distance[2], pV + 20);
    SetupVert(Block.X1, Block.Y1, Block.Distance[3], pV + 24);

    // Put data in all verts but the corners.
    i = 0;
    for (Y = 0; Y < 5; Y++)
        for (X = 0; X < 5; X++)
        {
            if ((i != 0) && (i != 4) && (i != 20) && (i != 24))
            {
                x = Block.X0 + m_Step[X];
                y = Block.Y0 + m_Step[Y];
                Distance = DISTANCE(x, y, m_SurfaceZ);
                SetupVert(x, y, Distance, pV + i);
            }

            i++;
        }

    // Add the triangle indices.
    for (i = 0; i < 32; i++)
    {
        AddTriangleIndices(*(pIndex + 0), *(pIndex + 1), *(pIndex + 2));
        pIndex += 3;
    }
}

//==============================================================================

void fluid::ProcessBlockLODMorph(block& Block)
{
    vertex* pV;
    s32     X, Y, i;
    f32     x, y;
    s16* pIndex = HighLODIndices;
    f32     Distance;
    f32     LOD;
    f32     MiddleX = Block.X0 + m_Step[2];
    f32     MiddleY = Block.Y0 + m_Step[2];
    f32     MiddleD = DISTANCE(MiddleX, MiddleY, m_SurfaceZ);

    // Get vetices.
    pV = AcquireVertices(25);

    // Put data in the corner verts.
    SetupVert(Block.X0, Block.Y0, Block.Distance[0], pV + 0);
    SetupVert(Block.X1, Block.Y0, Block.Distance[1], pV + 4);
    SetupVert(Block.X0, Block.Y1, Block.Distance[2], pV + 20);
    SetupVert(Block.X1, Block.Y1, Block.Distance[3], pV + 24);

    // Put data in all verts but the corners.
    i = 0;
    for (Y = 0; Y < 5; Y++)
        for (X = 0; X < 5; X++)
        {
            if ((i != 0) && (i != 4) && (i != 20) && (i != 24))
            {
                x = Block.X0 + m_Step[X];
                y = Block.Y0 + m_Step[Y];
                Distance = DISTANCE(x, y, m_SurfaceZ);
                SetupVert(x, y, Distance, pV + i);
            }

            i++;
        }

    // Get LOD values for the center AND the centers of each edge.
    LOD = Block.LOD[1] < Block.LOD[0] ? Block.LOD[1] : Block.LOD[0];
    LOD = Block.LOD[2] < LOD ? Block.LOD[2] : LOD;
    LOD = Block.LOD[3] < LOD ? Block.LOD[3] : LOD;

    // Interpolate all of the edges.
    InterpolateVerts(pV + 0, pV + 1, pV + 2, pV + 3, pV + 4, Block.LOD[0], Block.LOD[1]);
    InterpolateVerts(pV + 4, pV + 9, pV + 14, pV + 19, pV + 24, Block.LOD[1], Block.LOD[3]);
    InterpolateVerts(pV + 24, pV + 23, pV + 22, pV + 21, pV + 20, Block.LOD[3], Block.LOD[2]);
    InterpolateVerts(pV + 20, pV + 15, pV + 10, pV + 5, pV + 0, Block.LOD[2], Block.LOD[0]);

    // Now interpolate from the center out to get the remaining 8 verts.
    InterpolateVert(pV + 12, pV + 6, pV + 0, LOD);
    InterpolateVert(pV + 12, pV + 7, pV + 2, LOD);
    InterpolateVert(pV + 12, pV + 8, pV + 4, LOD);
    InterpolateVert(pV + 12, pV + 13, pV + 14, LOD);
    InterpolateVert(pV + 12, pV + 18, pV + 24, LOD);
    InterpolateVert(pV + 12, pV + 17, pV + 22, LOD);
    InterpolateVert(pV + 12, pV + 16, pV + 20, LOD);
    InterpolateVert(pV + 12, pV + 11, pV + 10, LOD);

    // Add the triangle indices.
    for (i = 0; i < 32; i++)
    {
        AddTriangleIndices(*(pIndex + 0), *(pIndex + 1), *(pIndex + 2));
        pIndex += 3;
    }
}

//==============================================================================

void fluid::ProcessBlockLODTrans(block& Block)
{
    vertex* pV;
    s32     Verts;
    s32     I;
    f32     X, Y;
    f32     Distance;
    f32     MiddleX = Block.X0 + m_Step[2];
    f32     MiddleY = Block.Y0 + m_Step[2];
    f32     MiddleD = DISTANCE(MiddleX, MiddleY, m_SurfaceZ);

    // Determine how many verts we need.
    Verts = 5;
    if ((Block.LOD[0] > 0.0f) && (Block.LOD[0] > 0.0f))    Verts += 3;
    if ((Block.LOD[1] > 0.0f) && (Block.LOD[3] > 0.0f))    Verts += 3;
    if ((Block.LOD[3] > 0.0f) && (Block.LOD[2] > 0.0f))    Verts += 3;
    if ((Block.LOD[2] > 0.0f) && (Block.LOD[0] > 0.0f))    Verts += 3;

    // Get vetices.
    pV = AcquireVertices(Verts);

    // Build the corner and center vertices.
    SetupVert(Block.X0, Block.Y0, Block.Distance[0], pV + 0);
    SetupVert(Block.X1, Block.Y0, Block.Distance[1], pV + 1);
    SetupVert(Block.X0, Block.Y1, Block.Distance[2], pV + 2);
    SetupVert(Block.X1, Block.Y1, Block.Distance[3], pV + 3);
    SetupVert(MiddleX, MiddleY, MiddleD, pV + 4);

    I = 5;

    //------------------------------------------------------

    if ((Block.LOD[0] > 0.0f) && (Block.LOD[1] > 0.0f))
    {
        X = Block.X0 + m_Step[1];
        Distance = DISTANCE(X, Block.Y0, m_SurfaceZ);
        SetupVert(X, Block.Y0, Distance, pV + I + 0);

        X = Block.X0 + m_Step[2];
        Distance = DISTANCE(X, Block.Y0, m_SurfaceZ);
        SetupVert(X, Block.Y0, Distance, pV + I + 1);

        X = Block.X0 + m_Step[3];
        Distance = DISTANCE(X, Block.Y0, m_SurfaceZ);
        SetupVert(X, Block.Y0, Distance, pV + I + 2);

        InterpolateVerts(pV + 0, pV + I + 0, pV + I + 1, pV + I + 2, pV + 1, Block.LOD[0], Block.LOD[1]);
        AddTriangleIndices(4, 0, I + 0);
        AddTriangleIndices(4, I + 0, I + 1);
        AddTriangleIndices(4, I + 1, I + 2);
        AddTriangleIndices(4, I + 2, 1);
        I += 3;
    }
    else
    {
        AddTriangleIndices(4, 0, 1);
    }

    //------------------------------------------------------

    if ((Block.LOD[1] > 0.0f) && (Block.LOD[3] > 0.0f))
    {
        Y = Block.Y0 + m_Step[1];
        Distance = DISTANCE(Block.X1, Y, m_SurfaceZ);
        SetupVert(Block.X1, Y, Distance, pV + I + 0);

        Y = Block.Y0 + m_Step[2];
        Distance = DISTANCE(Block.X1, Y, m_SurfaceZ);
        SetupVert(Block.X1, Y, Distance, pV + I + 1);

        Y = Block.Y0 + m_Step[3];
        Distance = DISTANCE(Block.X1, Y, m_SurfaceZ);
        SetupVert(Block.X1, Y, Distance, pV + I + 2);

        InterpolateVerts(pV + 1, pV + I + 0, pV + I + 1, pV + I + 2, pV + 3, Block.LOD[1], Block.LOD[3]);
        AddTriangleIndices(4, 1, I + 0);
        AddTriangleIndices(4, I + 0, I + 1);
        AddTriangleIndices(4, I + 1, I + 2);
        AddTriangleIndices(4, I + 2, 3);
        I += 3;
    }
    else
    {
        AddTriangleIndices(4, 1, 3);
    }

    //------------------------------------------------------

    if ((Block.LOD[3] > 0.0f) && (Block.LOD[2] > 0.0f))
    {
        X = Block.X1 - m_Step[1];
        Distance = DISTANCE(X, Block.Y1, m_SurfaceZ);
        SetupVert(X, Block.Y1, Distance, pV + I + 0);

        X = Block.X1 - m_Step[2];
        Distance = DISTANCE(X, Block.Y1, m_SurfaceZ);
        SetupVert(X, Block.Y1, Distance, pV + I + 1);

        X = Block.X1 - m_Step[3];
        Distance = DISTANCE(X, Block.Y1, m_SurfaceZ);
        SetupVert(X, Block.Y1, Distance, pV + I + 2);

        InterpolateVerts(pV + 3, pV + I + 0, pV + I + 1, pV + I + 2, pV + 2, Block.LOD[3], Block.LOD[2]);
        AddTriangleIndices(4, 3, I + 0);
        AddTriangleIndices(4, I + 0, I + 1);
        AddTriangleIndices(4, I + 1, I + 2);
        AddTriangleIndices(4, I + 2, 2);
        I += 3;
    }
    else
    {
        AddTriangleIndices(4, 3, 2);
    }

    //------------------------------------------------------

    if ((Block.LOD[2] > 0.0f) && (Block.LOD[0] > 0.0f))
    {
        Y = Block.Y1 - m_Step[1];
        Distance = DISTANCE(Block.X0, Y, m_SurfaceZ);
        SetupVert(Block.X0, Y, Distance, pV + I + 0);

        Y = Block.Y1 - m_Step[2];
        Distance = DISTANCE(Block.X0, Y, m_SurfaceZ);
        SetupVert(Block.X0, Y, Distance, pV + I + 1);

        Y = Block.Y1 - m_Step[3];
        Distance = DISTANCE(Block.X0, Y, m_SurfaceZ);
        SetupVert(Block.X0, Y, Distance, pV + I + 2);

        InterpolateVerts(pV + 2, pV + I + 0, pV + I + 1, pV + I + 2, pV + 0, Block.LOD[2], Block.LOD[0]);
        AddTriangleIndices(4, 2, I + 0);
        AddTriangleIndices(4, I + 0, I + 1);
        AddTriangleIndices(4, I + 1, I + 2);
        AddTriangleIndices(4, I + 2, 0);
        I += 3;
    }
    else
    {
        AddTriangleIndices(4, 2, 0);
    }

    //------------------------------------------------------
}

//==============================================================================
//
//   [2]-----[3]
//    |\  3  /|
//    | \   / |
//    |4 [4] 2|     Rendered as four triangles.
//    | /   \ |
//    |/  1  \|
//   [0]-----[1]
//

void fluid::ProcessBlockLODLow(block& Block)
{
    vertex* pV;
    f32     MiddleX = Block.X0 + m_Step[2];
    f32     MiddleY = Block.Y0 + m_Step[2];
    f32     MiddleD = DISTANCE(MiddleX, MiddleY, m_SurfaceZ);

    // Get vetices.
    pV = AcquireVertices(5);

    // Put data in the verts.
    SetupVert(Block.X0, Block.Y0, Block.Distance[0], pV + 0);
    SetupVert(Block.X1, Block.Y0, Block.Distance[1], pV + 1);
    SetupVert(Block.X0, Block.Y1, Block.Distance[2], pV + 2);
    SetupVert(Block.X1, Block.Y1, Block.Distance[3], pV + 3);
    SetupVert(MiddleX, MiddleY, MiddleD, pV + 4);

    //--Attempt to reject if all fogged.

        // Add the triangle indices.
    AddTriangleIndices(0, 1, 4);
    AddTriangleIndices(1, 3, 4);
    AddTriangleIndices(3, 2, 4);
    AddTriangleIndices(2, 0, 4);

    // Quick debug render.
#if 0
    if (0)
    {
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glColor4f(Block.LOD[0], 1.0f, Block.LOD[3], 1.0f);
        glVertex3f(pV[0].XYZ.X, pV[0].XYZ.Y, pV[0].XYZ.Z);
        glVertex3f(pV[1].XYZ.X, pV[1].XYZ.Y, pV[1].XYZ.Z);
        glVertex3f(pV[4].XYZ.X, pV[4].XYZ.Y, pV[4].XYZ.Z);
        glVertex3f(pV[2].XYZ.X, pV[2].XYZ.Y, pV[2].XYZ.Z);
        glVertex3f(pV[4].XYZ.X, pV[4].XYZ.Y, pV[4].XYZ.Z);
        glVertex3f(pV[1].XYZ.X, pV[1].XYZ.Y, pV[1].XYZ.Z);
        glVertex3f(pV[3].XYZ.X, pV[3].XYZ.Y, pV[3].XYZ.Z);
        glVertex3f(pV[2].XYZ.X, pV[2].XYZ.Y, pV[2].XYZ.Z);
        glEnd();
    }
#endif
}

//==============================================================================

void fluid::SetupVert(f32 X, f32 Y, f32 Distance, vertex* pV)
{
    f32 Z;

    //
    // Compute a Z value.
    //
    {
        f32 WarpZ, WarpD0, WarpD1;
        f32 WarpFactor;
        f32 Delta;

        Delta = SINE((X * 0.05f) + m_Seconds) +
            SINE((Y * 0.05f) + m_Seconds);

        Z = m_SurfaceZ + Delta * m_WaveFactor;

        // When the camera is close to the fluid surface, warp the surface to
        // prevent the fluid from intersecting the camera glass.

        WarpD0 = m_Step[2];
        if (Distance < WarpD0)
        {
            WarpD1 = m_Step[1];

            if (m_Eye.Z > sSurfaceAtEye)
            {
                WarpZ = m_Eye.Z - 0.25f;
                if (Z > WarpZ)
                {
                    if (Distance < WarpD1)
                    {
                        // Close enough to completely warp the vert.
                        Z = WarpZ;
                    }
                    else
                    {
                        // Range is between WarpD0 and WarpD1.  Interpolate.
                        WarpFactor = ((WarpD0 - Distance) / WarpD1);
                        Z = (WarpZ * WarpFactor) + (Z * (1.0f - WarpFactor));
                    }
                }
            }
            else
            {
                WarpZ = m_Eye.Z + 0.25f;
                if (Z < WarpZ)
                {
                    if (Distance < WarpD1)
                    {
                        // Close enough to completely warp the vert.
                        Z = WarpZ;
                    }
                    else
                    {
                        // Range is between WarpD0 and WarpD1.  Interpolate.
                        WarpFactor = ((WarpD0 - Distance) / WarpD1);
                        Z = (WarpZ * WarpFactor) + (Z * (1.0f - WarpFactor));
                    }
                }
            }
        }
    }

    // These fields are easy!

    pV->XYZ.X = X;
    pV->XYZ.Y = Y;
    pV->XYZ.Z = Z;
    pV->RGBA4.R = m_FogColor.R;
    pV->RGBA4.G = m_FogColor.G;
    pV->RGBA4.B = m_FogColor.B;
    pV->RGBA4.A = m_pFogFn(Distance, sFogZ);

    /*
        pV->RGBA4.A = Distance < m_VisibleDistance
                        ? sFogTable[ (s32)(Distance * sFogAccessFactor) ]
                        : 1.0f;
    */

    //
    // Compute the UV for the environment map / specular pass.
    //
    {
        xyz Vector;

        // The reflection vector is the "eye to point" vector, but with a positive
        // Z value.

        // MM:	I've 'rotated' these so that we get the environment map reflected in the
        //		correct orientation.
        Vector.X = X - m_Eye.X;
        Vector.Y = m_Eye.Y - Y;
        Vector.Z = Z - m_Eye.Z;

        if (Vector.Z < 0.0f)
            Vector.Z = -Vector.Z;

        if (Vector.Z < 0.001f)
            Vector.Z = 0.001f;

        // The UV values are simply the XY values from the normalized reflection
        // vector.

        if (Distance < 0.001f)
        {
            pV->UV3.U = 0.0f;
            pV->UV3.V = 0.0f;
        }
        else
        {
            // The standard UV reflection mapping tends to over emphasize the
            // outer edges of the environment map.  Try to "adjust" the values
            // to get more efficient use of the map.  Oh, and normalize, too.

            f32 Value = (Distance - Vector.Z) / (Distance * Distance);

            pV->UV3.U = Vector.X * Value;
            pV->UV3.V = Vector.Y * Value;
        }

        // Convert from [+1,-1] to [0,1].
        pV->UV3.U = pV->UV3.U * 0.5f + 0.5f;
        pV->UV3.V = pV->UV3.V * 0.5f + 0.5f;

        // Now, based on the XY, tweak the UV in some liquid manner.

        volatile static f32 Q1 = 150.00f;
        volatile static f32 Q2 = 2.00f;
        volatile static f32 Q3 = 0.01f;

        // MM: Modified the environment distortion times to match the surface distortion.
        f32 A1 = COSINE(((X / Q1) + (m_Seconds / m_DistortTime)));
        f32 A2 = SINE(((Y / Q1) + (m_Seconds / m_DistortTime)));

        pV->UV3.U += A1 * Q3;
        pV->UV3.V += A2 * Q3;

        // MM:	Removed the fresnel style blending as it makes the depth-map difficult
        //		to control.
        //f32 Swing = (A1+A2) * 0.15f + 0.5f;
        //pV->RGBA1a.A = ((1.0f-Swing) * m_Opacity) / (1.0f - (Swing * m_Opacity));
        //pV->RGBA1b.A = Swing * m_Opacity;

        // MM:	Added Fresnel style blending to the environment map.
        f32 Swing = (A1 + A2) * 0.15f + 0.5f;
        pV->RGBA3.R = pV->RGBA3.G = pV->RGBA3.B = 1.0f;
        pV->RGBA3.A = Swing * m_EnvMapIntensity;
    }

    // MM: Calculate Depth-map Position.
    f32 S1 = m_DistortMagnitude * COSINE((X * m_DistortGridScale) + (m_Seconds / m_DistortTime));
    f32 T1 = m_DistortMagnitude * SINE((Y * m_DistortGridScale) + (m_Seconds / m_DistortTime));
    pV->UV1.U = ((X - (m_SquareX0 * 8)) * m_DepthTexelX) * m_TessellationSurface + S1;
    pV->UV1.V = ((Y - (m_SquareY0 * 8)) * m_DepthTexelY) * m_TessellationSurface + T1;
    pV->UV2.U = ((X - (m_SquareX0 * 8)) * m_DepthTexelX) * m_TessellationShore + S1;
    pV->UV2.V = ((Y - (m_SquareY0 * 8)) * m_DepthTexelY) * m_TessellationShore + T1;

    // MM: Calculate Depth-map Position.
    pV->UV4.U = (X - (m_SquareX0 * 8)) * m_DepthTexelX;
    pV->UV4.V = (Y - (m_SquareY0 * 8)) * m_DepthTexelY;
}

//==============================================================================

void fluid::InterpolateVerts(vertex* pV0,
    vertex* pV1,
    vertex* pV2,
    vertex* pV3,
    vertex* pV4,
    f32     LOD0,
    f32     LOD4)
{
    f32 DeltaLOD = (LOD4 - LOD0) * 0.25f;
    f32 DeltaZ = (pV4->XYZ.Z - pV0->XYZ.Z) * 0.25f;
    f32 DeltaU = (pV4->UV3.U - pV0->UV3.U) * 0.25f;
    f32 DeltaV = (pV4->UV3.V - pV0->UV3.V) * 0.25f;

    f32 Z = pV0->XYZ.Z;
    uv  UV = pV0->UV3;

    f32 LODa = LOD0;
    f32 LODb = 1.0f;

    //--------------------------------------------------------------------------
    // Compute interpolated low detail data.
    Z += DeltaZ;
    UV.U += DeltaU;
    UV.V += DeltaV;
    LODa += DeltaLOD;
    LODb = 1.0f - LODa;

    // Interpolate based on LOD.
    pV1->UV3.U = (pV1->UV3.U * LODa) + (UV.U * LODb);
    pV1->UV3.V = (pV1->UV3.V * LODa) + (UV.V * LODb);
    pV1->XYZ.Z = (pV1->XYZ.Z * LODa) + (Z * LODb);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Compute complete interpolated low detail data.
    Z += DeltaZ;
    UV.U += DeltaU;
    UV.V += DeltaV;
    LODa += DeltaLOD;
    LODb = 1.0f - LODa;

    // Interpolate based on LOD.
    pV2->UV3.U = (pV2->UV3.U * LODa) + (UV.U * LODb);
    pV2->UV3.V = (pV2->UV3.V * LODa) + (UV.V * LODb);
    pV2->XYZ.Z = (pV2->XYZ.Z * LODa) + (Z * LODb);
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Compute complete interpolated low detail data.
    Z += DeltaZ;
    UV.U += DeltaU;
    UV.V += DeltaV;
    LODa += DeltaLOD;
    LODb = 1.0f - LODa;

    // Interpolate based on LOD.
    pV3->UV3.U = (pV3->UV3.U * LODa) + (UV.U * LODb);
    pV3->UV3.V = (pV3->UV3.V * LODa) + (UV.V * LODb);
    pV3->XYZ.Z = (pV3->XYZ.Z * LODa) + (Z * LODb);
    //--------------------------------------------------------------------------
}

//==============================================================================

void fluid::InterpolateVert(vertex* pV0,
    vertex* pV1,
    vertex* pV2,
    f32     LODa)
{
    f32 Z = (pV2->XYZ.Z + pV0->XYZ.Z) * 0.5f;
    f32 U = (pV2->UV3.U + pV0->UV3.U) * 0.5f;
    f32 V = (pV2->UV3.V + pV0->UV3.V) * 0.5f;
    f32 LODb = 1.0f - LODa;

    //--------------------------------------------------------------------------
    // Interpolate based on LOD.
    pV1->UV3.U = (pV1->UV3.U * LODa) + (U * LODb);
    pV1->UV3.V = (pV1->UV3.V * LODa) + (V * LODb);
    pV1->XYZ.Z = (pV1->XYZ.Z * LODa) + (Z * LODb);
    //--------------------------------------------------------------------------
}

//==============================================================================


