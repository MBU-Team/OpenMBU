//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/fluid.h"
#include <string.h>

//==============================================================================
//  VARIABLES
//==============================================================================

s32 fluid::m_Instances = 0;
s32 fluid::m_MaskOffset[6] = { 0, 1, 2, 4, 12, 44 };

//==============================================================================
//  FUNCTIONS
//==============================================================================

fluid::fluid(void)
{
    m_Instances += 1;

    // Fill out fields with a stable, if useless, state.
    m_SquareX0 = 0;
    m_SquareY0 = 0;
    m_SquaresInX = 4;
    m_SquaresInY = 4;
    m_BlocksInX = 1;
    m_BlocksInY = 1;
    m_HighResMode = 1;
    m_RemoveWetEdges = 0;
    m_SurfaceZ = 0.0f;
    m_WaveAmplitude = 0.0f;
    m_Opacity = 0.0f;
    m_EnvMapIntensity = 1.0f;
    m_BaseSeconds = SECONDS;
    m_pTerrain = NULL;

    // Set values on the debug flags.
    m_ShowWire = 0;
    m_ShowBlocks = 0;
    m_ShowNodes = 0;
    m_ShowBaseA = 1;
    m_ShowBaseB = 1;
    m_ShowLightMap = 1;
    m_ShowEnvMap = 1;
    m_ShowFog = 1;
}

//==============================================================================

fluid::~fluid()
{
    m_Instances -= 1;

    if (m_Instances == 0)
    {
        ReleaseVertexMemory();
    }
}

//==============================================================================

s32 fluid::GetRejectBit(s32 Level, s32 IndexX, s32 IndexY) const
{
    s32  BitNumber = (IndexY << Level) + IndexX;
    s32  ByteNumber = m_MaskOffset[Level] + (BitNumber >> 3);
    byte Byte = m_RejectMask[ByteNumber];
    s32  Bit = (Byte >> (BitNumber & 0x07)) & 0x01;

    return(Bit);
}

//==============================================================================

s32 fluid::GetAcceptBit(s32 Level, s32 IndexX, s32 IndexY) const
{
    if (Level == 5)
        return(!GetRejectBit(5, IndexX, IndexY));

    s32  BitNumber = (IndexY << Level) + IndexX;
    s32  ByteNumber = m_MaskOffset[Level] + (BitNumber >> 3);
    byte Byte = m_AcceptMask[ByteNumber];
    s32  Bit = (Byte >> (BitNumber & 0x07)) & 0x01;

    return(Bit);
}

//==============================================================================

void fluid::SetRejectBit(s32 Level, s32 IndexX, s32 IndexY, s32 Value)
{
    s32  BitNumber = (IndexY << Level) + IndexX;
    s32  ByteNumber = m_MaskOffset[Level] + (BitNumber >> 3);
    byte Byte = 1 << (BitNumber & 0x07);

    if (Value)     m_RejectMask[ByteNumber] |= Byte;
    else            m_RejectMask[ByteNumber] &= ~Byte;
}

//==============================================================================

void fluid::SetAcceptBit(s32 Level, s32 IndexX, s32 IndexY, s32 Value)
{
    if (Level == 5)
        SetRejectBit(5, IndexX, IndexY, !Value);

    s32  BitNumber = (IndexY << Level) + IndexX;
    s32  ByteNumber = m_MaskOffset[Level] + (BitNumber >> 3);
    byte Byte = 1 << (BitNumber & 0x07);

    if (Value)     m_AcceptMask[ByteNumber] |= Byte;
    else            m_AcceptMask[ByteNumber] &= ~Byte;
}

//==============================================================================

void fluid::BuildLowerMasks(void)
{
    s32 X, Y;

    // Initially, at all non-level-5 mask levels, we want to both accept and
    // reject everything.  Then, we'll go back and correct it all.

    MEMSET(m_AcceptMask, 0xFF, 1 + 1 + 2 + 8 + 32);
    MEMSET(m_RejectMask, 0xFF, 1 + 1 + 2 + 8 + 32);

    // Now, for each entry in the level 5 mask, push its implications down
    // through all the other levels.

    for (Y = 0; Y < 32; Y++)
        for (X = 0; X < 32; X++)
        {
            if (GetRejectBit(5, X, Y))
            {
                // The block is set for reject.
                // We cannot accept it on the lower levels.
                SetAcceptBit(4, X >> 1, Y >> 1, 0);
                SetAcceptBit(3, X >> 2, Y >> 2, 0);
                SetAcceptBit(2, X >> 3, Y >> 3, 0);
                SetAcceptBit(1, X >> 4, Y >> 4, 0);
                SetAcceptBit(0, X >> 5, Y >> 5, 0);
            }
            else
            {
                // The block is set for accept.
                // We cannot reject it on the lower levels.
                SetRejectBit(4, X >> 1, Y >> 1, 0);
                SetRejectBit(3, X >> 2, Y >> 2, 0);
                SetRejectBit(2, X >> 3, Y >> 3, 0);
                SetRejectBit(1, X >> 4, Y >> 4, 0);
                SetRejectBit(0, X >> 5, Y >> 5, 0);
            }
        }
}

//==============================================================================

struct fill_segment
{
    s32 Y;
    s32 X0, X1;
    s32 DY;             // +1 or -1
};

#define STACK_SIZE  50

//------------------------------------------------------------------------------
#define PUSH(y,x0,x1,dy)                                \
{                                                       \
    if( ((y+(dy)) >= 0) && ((y+(dy)) < SizeY) )         \
    {                                                   \
        if( Count < STACK_SIZE )                        \
        {                                               \
            Stack[Count].Y  = y;                        \
            Stack[Count].X0 = x0;                       \
            Stack[Count].X1 = x1;                       \
            Stack[Count].DY = dy;                       \
            Count++;                                    \
        }                                               \
        else                                            \
        {                                               \
            FloodFill( pGrid, x0, y, SizeX, SizeY );    \
        }                                               \
    }                                                   \
}
//------------------------------------------------------------------------------
#define POP(y,x0,x1,dy)                                 \
{                                                       \
    Count--;                                            \
    Y  = Stack[Count].Y + Stack[Count].DY;              \
    X0 = Stack[Count].X0;                               \
    X1 = Stack[Count].X1;                               \
    DY = Stack[Count].DY;                               \
}
//------------------------------------------------------------------------------

void fluid::FloodFill(u8* pGrid, s32 x, s32 y, s32 SizeX, s32 SizeY)
{
    fill_segment Stack[STACK_SIZE];
    s32          Count = 0;
    s32          X, Y, X0, X1, DY, Left;
    u8* p;

    if (!pGrid[(y * SizeX) + x])
        return;

    PUSH(y, x, x, 1);  // Needed in a few cases.
    PUSH(y + 1, x, x, -1);  // Primary seed point.  Popped first.

    while (Count > 0)
    {
        POP(Y, X0, X1, DY);

        // A span in y=(Y-DY) for X0<=x<=X1 was previously filled.  Now consider
        // adjacent entries in y=Y.

        // Clear going towards decreasing X.
        X = X0;
        p = &pGrid[(Y * SizeX) + X];
        while ((X >= 0) && *p)
        {
            *p = 0;
            X--;
            p--;
        }

        if (X >= X0)
            goto Skip;

        Left = X + 1;
        if (Left < X0)
            PUSH(Y, Left, X0 - 1, -DY)

            X = X0 + 1;

        do
        {
            // Clear going towards increasing X.
            p = &pGrid[(Y * SizeX) + X];
            while ((X < SizeX) && *p)
            {
                *p = 0;
                X++;
                p++;
            }

            PUSH(Y, Left, X - 1, DY);
            if (X > X1 + 1)
                PUSH(Y, X1 + 1, X - 1, -DY);

        Skip:       X++;
            p = &pGrid[(Y * SizeX) + X];
            while ((X <= X1) && !(*p))
            {
                X++;
                p++;
            }
            Left = X;
        } while (X <= X1);
    }
}

//==============================================================================

void fluid::RebuildMasks(void)
{
    u8* pGrid;
    u8* pG;         // Traveling grid pointer
    s32 GridSize;
    s32 X, Y;
    s32 x, y;
    s32 i;          // Index

    s32 SquaresPerBlock = m_HighResMode ? 4 : 8;
    s32 ShiftPerBlock = m_HighResMode ? 2 : 3;

    //
    // We need a grid to classify all terrain data points which are within the
    // fluid area.  We will use this grid to reject underground blocks, reject
    // "wet" edges if requested, and to dry fill where requested.
    //

    GridSize = (m_SquaresInX + 1) * (m_SquaresInY + 1);
    pGrid = (u8*)MALLOC(GridSize);

    // Classify each point as above or below ground.

    if (m_pTerrain)
    {
        u16 FluidLevel = (u16)((m_SurfaceZ + (m_WaveAmplitude / 2.0f)) * 32.0f);

        pG = pGrid;
        for (Y = 0; Y < m_SquaresInY + 1; Y++)
            for (X = 0; X < m_SquaresInX + 1; X++)
            {
                i = (((m_SquareY0 + Y) & 255) << 8) + ((m_SquareX0 + X) & 255);
                *pG = (u8)(FluidLevel > m_pTerrain[i]);
                pG++;
            }
    }

    // If requested, "dry up" all edges which "protrude" into the air.

    if (m_RemoveWetEdges && m_pTerrain)
    {
        for (X = 0; X < m_SquaresInX + 1; X++)
        {
            FloodFill(pGrid, X, 0, m_SquaresInX + 1, m_SquaresInY + 1);
            FloodFill(pGrid, X, m_SquaresInY, m_SquaresInX + 1, m_SquaresInY + 1);
        }

        for (Y = 0; Y < m_SquaresInY + 1; Y++)
        {
            FloodFill(pGrid, 0, Y, m_SquaresInX + 1, m_SquaresInY + 1);
            FloodFill(pGrid, m_SquaresInX, Y, m_SquaresInX + 1, m_SquaresInY + 1);
        }
    }

    // Time to build the masks.  First, reject everything!  We will work on the
    // level 5 reject mask.  (Level 5 is the most detailed mask, and there is no
    // accept mask at that level.)

    MEMSET(m_RejectMask + m_MaskOffset[5], 0xFF, 128);

    // Any block which as useful points left in the grid is to be kept.

    for (Y = 0; Y < m_BlocksInY; Y++)
        for (X = 0; X < m_BlocksInX; X++)
        {
            s32 Accept = 0;

            // If ANY point in the block is acceptable, then accept the whole block.

            for (y = 0; y <= SquaresPerBlock; y++)
                for (x = 0; x <= SquaresPerBlock; x++)
                {
                    s32 GridX = (X << ShiftPerBlock) + x;
                    s32 GridY = (Y << ShiftPerBlock) + y;

                    i = (GridY * (m_SquaresInX + 1)) + GridX;

                    if (pGrid[i])
                    {
                        Accept = 1;
                        goto BailOut;
                    }
                }

        BailOut:

            if (Accept)
                SetRejectBit(5, X, Y, 0);
        }

    FREE(pGrid);
    BuildLowerMasks();
}

//==============================================================================

void fluid::SetInfo(f32& X0,
    f32& Y0,
    f32& SizeX,
    f32& SizeY,
    f32  SurfaceZ,
    f32  WaveAmplitude,
    f32& Opacity,
    f32& EnvMapIntensity,
    s32  RemoveWetEdges,
    bool UseDepthMap,
    f32  TessellationSurface,
    f32  TessellationShore,
    f32  SurfaceParallax,
    f32  FlowAngle,
    f32  FlowRate,
    f32  mDistortGridScale,
    f32  mDistortMagnitude,
    f32  mDistortTime,
    ColorF SpecColor,
    F32 SpecPower)	// MM: Added Various Parameters.
{
    m_SpecColor = SpecColor;
    m_SpecPower = SpecPower;

    // MM: Calculate Depth-map Texel X/Y.
    m_DepthTexelX = 1.0f / SizeX;
    m_DepthTexelY = 1.0f / SizeY;

    // MM: Added Depth-Map Toggle.
    m_UseDepthMap = UseDepthMap;

    // MM: Tessellations.
    m_TessellationSurface = TessellationSurface;
    m_TessellationShore = TessellationShore;

    // MM: Surface Parallax.
    m_SurfaceParallax = SurfaceParallax;

    // MM: Flow Control.
    m_FlowAngle = FlowAngle;
    m_FlowRate = FlowRate;
    m_FlowMagnitudeS =
        m_FlowMagnitudeT = 0.0f;

    // MM: Surface Disturbance.  RemoveMe!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    m_DistortGridScale = mDistortGridScale;
    m_DistortMagnitude = mDistortMagnitude;
    m_DistortTime = mDistortTime;

    // MM: Put in neater constraints.
    m_EnvMapIntensity = mClampF(EnvMapIntensity, 0.0f, 1.0f);

    // MM: Removed Section.
/*
    // Constrain the range of parameters.
    if( Opacity         > 1.0f )    Opacity         = 1.0f;
    if( Opacity         < 0.0f )    Opacity         = 0.0f;
    if( EnvMapIntensity > 1.0f )    EnvMapIntensity = 1.0f;
    if( EnvMapIntensity < 0.0f )    EnvMapIntensity = 0.0f;
*/
// Get the easy stuff first.
    m_SurfaceZ = SurfaceZ;
    m_WaveAmplitude = WaveAmplitude;
    m_RemoveWetEdges = RemoveWetEdges;
    m_Opacity = mClampF(Opacity, 0.0f, 1.0f);	// MM: Put in neater constraints.
    m_EnvMapIntensity = EnvMapIntensity;

    m_WaveFactor = m_WaveAmplitude * 0.25f;

    // Place the "min" corner.
    m_SquareX0 = (s32)((X0 / 8.0f) + 0.5f);
    m_SquareY0 = (s32)((Y0 / 8.0f) + 0.5f);

    // Constrain the range of values.
    if (m_SquareX0 < 0.0f)  m_SquareX0 = 0;
    if (m_SquareY0 < 0.0f)  m_SquareY0 = 0;
    if (m_SquareX0 > 2040.0f)  m_SquareX0 = 2040;
    if (m_SquareY0 > 2040.0f)  m_SquareY0 = 2040;

    // Decide on the size of the block.
    m_SquaresInX = (s32)((SizeX / 8.0f) + 0.5f);
    m_SquaresInY = (s32)((SizeY / 8.0f) + 0.5f);

    //
    // If the fluid is meant to cover less than 1/4 of a terrain rep, then we
    // will enter "High Resolution Mode".  The fluid will cover the same area
    // as specified, but it will have twice the vertex resolution in each
    // direction.  So, on "small lakes", we get better memory utilization,
    // better terrain fitting, and so on.
    //

    if ((m_SquaresInX <= 128) && (m_SquaresInY <= 128))
    {
        // High Resolution Mode!
        m_HighResMode = 1;

        // A Block is now 4x4 terrain squares.  And the number of squares in
        // the fluid must be a multiple of 4 so we get whole blocks.

        m_SquaresInX = (m_SquaresInX + 3) & ~0x03;
        m_SquaresInY = (m_SquaresInY + 3) & ~0x03;

        // Constrain the range of values.
        if (m_SquaresInX <= 0)   m_SquaresInX = 4;
        if (m_SquaresInY <= 0)   m_SquaresInY = 4;

        m_BlocksInX = m_SquaresInX >> 2;
        m_BlocksInY = m_SquaresInY >> 2;
    }
    else
    {
        // Normal resolution.
        m_HighResMode = 0;

        // A Block is now 8x8 terrain squares.  And the number of squares in
        // the fluid must be a multiple of 8 so we get whole blocks.

        m_SquaresInX = (m_SquaresInX + 7) & ~0x07;
        m_SquaresInY = (m_SquaresInY + 7) & ~0x07;

        // Constrain the range of values.
        if (m_SquaresInX > 256)   m_SquaresInX = 256;
        if (m_SquaresInY > 256)   m_SquaresInY = 256;
        if (m_SquaresInX <= 0)   m_SquaresInX = 8;
        if (m_SquaresInY <= 0)   m_SquaresInY = 8;

        m_BlocksInX = m_SquaresInX >> 3;
        m_BlocksInY = m_SquaresInY >> 3;
    }

    // Set some internal values for later usage.
    if (m_HighResMode)
    {
        m_Step[4] = 32.0f;
        m_Step[3] = 24.0f;
        m_Step[2] = 16.0f;
        m_Step[1] = 8.0f;
        m_Step[0] = 0.0f;
    }
    else
    {
        m_Step[4] = 64.0f;
        m_Step[3] = 48.0f;
        m_Step[2] = 32.0f;
        m_Step[1] = 16.0f;
        m_Step[0] = 0.0f;
    }

    // Set values back into parameters for caller.
    X0 = m_SquareX0 * 8.0f;
    Y0 = m_SquareY0 * 8.0f;
    SizeX = m_SquaresInX * 8.0f;
    SizeY = m_SquaresInY * 8.0f;

    // Recompute our masks.
    RebuildMasks();
}

//==============================================================================

void fluid::SetTerrainData(u16* pTerrainData)
{
    m_pTerrain = pTerrainData;
    RebuildMasks();
}

//==============================================================================

void fluid::SetEyePosition(f32 X, f32 Y, f32 Z)
{
    m_Eye.X = X;
    m_Eye.Y = Y;
    m_Eye.Z = Z;
}

//==============================================================================
// Frustrum clip planes: 0=T 1=B 2=L 3=R 4=N 5=F

void fluid::SetFrustrumPlanes(f32* pFrustrumPlanes)
{
    f32 BackOff = m_WaveAmplitude * 0.5f;

    m_Plane[0].A = pFrustrumPlanes[0];
    m_Plane[0].B = pFrustrumPlanes[1];
    m_Plane[0].C = pFrustrumPlanes[2];
    m_Plane[0].D = pFrustrumPlanes[3] + BackOff;

    m_Plane[1].A = pFrustrumPlanes[4];
    m_Plane[1].B = pFrustrumPlanes[5];
    m_Plane[1].C = pFrustrumPlanes[6];
    m_Plane[1].D = pFrustrumPlanes[7] + BackOff;

    m_Plane[2].A = pFrustrumPlanes[8];
    m_Plane[2].B = pFrustrumPlanes[9];
    m_Plane[2].C = pFrustrumPlanes[10];
    m_Plane[2].D = pFrustrumPlanes[11] + BackOff;

    m_Plane[3].A = pFrustrumPlanes[12];
    m_Plane[3].B = pFrustrumPlanes[13];
    m_Plane[3].C = pFrustrumPlanes[14];
    m_Plane[3].D = pFrustrumPlanes[15] + BackOff;

    m_Plane[4].A = pFrustrumPlanes[16];
    m_Plane[4].B = pFrustrumPlanes[17];
    m_Plane[4].C = pFrustrumPlanes[18];
    m_Plane[4].D = pFrustrumPlanes[19] + BackOff;

    m_Plane[5].A = pFrustrumPlanes[20];
    m_Plane[5].B = pFrustrumPlanes[21];
    m_Plane[5].C = pFrustrumPlanes[22];
    m_Plane[5].D = pFrustrumPlanes[23];
}

//==============================================================================

/*
void fluid::SetTextures( TextureHandle Base,
                         TextureHandle EnvMapOverTexture,
                         TextureHandle EnvMapUnderTexture,
                         TextureHandle ShoreTexture,
                         TextureHandle DepthTexture,
                         TextureHandle ShoreDepthTexture,
                   TextureHandle SpecMaskTexture )	// MM: Added Various Textures.
{
    m_BaseTexture			= Base;
    m_EnvMapOverTexture		= EnvMapOverTexture;	// MM: Added Over/Under Env Texture Support.
    m_EnvMapUnderTexture	= EnvMapUnderTexture;	// MM: Added Over/Under Env Texture Support.
    m_ShoreTexture			= ShoreTexture;			// MM: Added Shore Texture.
    m_DepthTexture			= DepthTexture;			// MM: Added Depth-Map Texture.
    m_ShoreDepthTexture		= ShoreDepthTexture;	// MM: Added Depth-Map Texture.
   m_SpecMaskTex = SpecMaskTexture;
}
*/

//==============================================================================

/*
void fluid::SetLightMapTexture( TextureHandle LightMapTexture )
{
    m_LightMapTexture = LightMapTexture;
}
*/

//==============================================================================

void fluid::SetFogParameters(f32 R, f32 G, f32 B, f32 VisibleDistance)
{
    m_FogColor.R = R;
    m_FogColor.G = G;
    m_FogColor.B = B;
    m_FogColor.A = 1.0f;
    m_VisibleDistance = VisibleDistance;
}

//==============================================================================

void fluid::SetFogFn(compute_fog_fn* pFogFn)
{
    m_pFogFn = pFogFn;
}

//==============================================================================

s32 fluid::IsFluidAtXY(f32 X, f32 Y) const
{
    s32 x, y;
    s32 ShiftPerBlock = m_HighResMode ? 5 : 6;

    //
    // Convert fluid space (X,Y) to block (x,y).  Use the accept mask.  Note
    // that the masks are anchored at the min point rather than terrain (0,0).
    //

    // Convert to integer.
    x = (s32)X;
    y = (s32)Y;

    // Compensate for min point offset.
    x -= (m_SquareX0 << 3);
    y -= (m_SquareY0 << 3);

    // We only want points in the range [0,2048).
    x &= 2047;
    y &= 2047;

    // Convert to block coordinate.
    x >>= ShiftPerBlock;
    y >>= ShiftPerBlock;

    // When we are in high res mode, there are "virtually" 64 blocks per terrain
    // along a particular axis.  But only the first 32 of them are used.
    if (x >= 32)   return(0);
    if (y >= 32)   return(0);

    // Consult mask.
    return(GetAcceptBit(5, x, y));
}

//==============================================================================
