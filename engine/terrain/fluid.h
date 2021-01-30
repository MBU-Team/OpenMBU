//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FLUID_H_
#define _FLUID_H_

//==============================================================================
//  TO DO LIST
//==============================================================================
//  - ARB support?
//  - Optimize the fog given water is horizontal
//  - New fog system
//  - Attempt to reject fully fogged low LOD blocks
//==============================================================================
//  - Designate specular map?
//  - Turn off specular?  (for lava)
//==============================================================================

//==============================================================================
//  ASSUMPTIONS:
//   - A single repetition of the terrain (or a "rep") is 256x256 squares.
//   - A terrain square is 8 meters on a side.
//   - The fluid can NOT be rotated on any axis.
//   - The "anchor point" for the fluid can be on any discrete square corner.
//   - The size of the fluid is constrained to be either a multiple of 4 or 8
//     squares.
//==============================================================================
//  DISCUSSION:
//   - If the overall size of the fluid is less than a quarter of a terrain rep,
//     then the fluid will use a "double resolution" mode.  Thus, when the fluid
//     is sufficiently small, it gets more detailed.
//   - The fluid renders blocks in one of two sizes, LOW and HIGH.
//   - Blocks are 8 squares in normal (LOW) resolution mode, and 4 in high (LOW).
//   - A quad tree is used to quickly cull down to the blocks.
//   - Reject and accept masks are used by the quad tree.
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#ifndef _TYPES_H_
#include "platform/types.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GTEXMANAGER_H_
//#include "dgl/gTexManager.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#include "core/color.h"

//==============================================================================
//  DEFINES
//==============================================================================

#define s32     S32
#define u32     U32
#define s16     S16
#define u16     U16
#define s8      S8
#define u8      U8
#define byte    U8
#define f32     F32

#define PI   (3.141592653589793238462643f)

#define SECONDS         ((f32)(Platform::getRealMilliseconds()) * 0.001f)
#define MALLOC          dMalloc
#define REALLOC         dRealloc
#define FREE            dFree
#define MEMSET          dMemset
#define TWO_PI          (PI * 2.0f)
#define FMOD(a,b)       (f32)mFmod( a, b )
#define LENGTH3(a,b,c)  (f32)mSqrt( (a)*(a) + (b)*(b) + (c)*(c) )
#define SINE(a)         (float)mSin( a )
#define COSINE(a)       (float)mCos( a )

#define DISTANCE(x,y,z) LENGTH3( (x)-m_Eye.X, (y)-m_Eye.Y, (z)-m_Eye.Z )

#define ASSERT(exp)

//==============================================================================
//  TYPES
//==============================================================================
//
//  Order for [4] element arrays...
//
//     2----3      Y
//     |    |      |
//     |    |      |
//     0----1      0----X
//
//==============================================================================

class fluid
{
    //------------------------------------------------------------------------------
    //  Public Types
    //
public:
    typedef f32 compute_fog_fn(f32 DeltaZ, f32 D);

    //------------------------------------------------------------------------------
    //  Public Functions
    //
public:
    fluid(void);
    ~fluid();

    //
    // Render (in FluidRender.cpp):
    //
    void    Render(bool& EyeSubmerged);

    //
    // Setup per frame (in FluidSupport.cpp):
    //
    void    SetEyePosition(f32 X, f32 Y, f32 Z);
    void    SetFrustrumPlanes(f32* pFrustrumPlanes);

    //
    // Setup at initialization (in FluidSupport.cpp):
    //
    void    SetInfo(f32& X0,
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
        f32  DistortGridScale,
        f32  DistortMagnitude,
        f32  DistortTime,
        ColorF SpecColor,
        F32  SpecPower);			// MM: Added Various Parameters.

    void    SetTerrainData(u16* pTerrainData);

    /*
        void    SetTextures         ( TextureHandle Base,
                                      TextureHandle EnvMapOverTexture,
                                      TextureHandle EnvMapUnderTexture,
                                      TextureHandle ShoreTexture,
                                      TextureHandle DepthTexture,
                                      TextureHandle ShoreDepthTexture,
                              TextureHandle SpecMaskTexture  );	// MM: Added Various Textures.

        void    SetLightMapTexture  ( TextureHandle LightMapTexture );
    */
    void    SetFogParameters(f32 R, f32 G, f32 B, f32 VisibleDistance);
    void    SetFogFn(compute_fog_fn* pFogFn);

    //
    // Run time interrogation (in FluidSupport.cpp):
    //
    s32     IsFluidAtXY(f32 X, f32 Y)    const;

    //------------------------------------------------------------------------------
    //  Private Types
    //
private:
    struct plane { f32 A, B, C, D; };
    struct rgba { f32 R, G, B, A; };
    struct xyz { f32 X, Y, Z; };
    struct uv { f32 U, V; };

    struct node
    {
        s32     Level;
        s32     MaskIndexX, MaskIndexY;
        s32     BlockX0, BlockY0;       // World Block
        f32     X0, Y0;            // World Position
        f32     X1, Y1;            // World Position
        byte    ClipBits[4];
    };

    struct block
    {
        f32     X0, Y0;
        f32     X1, Y1;
        f32     Distance[4];    // Distance from eye
        f32     LOD[4];    // Level of detail
    };

    // Rendering phases:
    //  Phase 1 - Base texture (two passes of cross faded textures)
    //  Phase 2 - Shadow map
    //  Phase 3 - Environment map / Specular
    //  Phase 4 - Fog

    struct vertex
    {
        xyz     XYZ;        // All phases - Position
//        rgba    RGBA1a;     // Phase 1a   - Base alpha, first pass (MM: Removed)
//        rgba    RGBA1b;     // Phase 1b   - Base alpha, second pass (MM: Removed)
        uv      UV1;        // Phase 1    - MM: Base UV.
        uv      UV2;        // Phase 2    - MM: Shoreline UV.
        rgba    RGBA3;      // Phase 3    - MM: EnvMap Colour/Alpha.
        uv      UV3;        // Phase 3    - EnvMap UV
        rgba    RGBA4;      // Phase 4    - Fog Color/Alpha
        uv		UV4;		// Test       - MM: Depth Alpha Map.
        ColorF  SPECULAR;
    };

    //------------------------------------------------------------------------------
    //  Private Variables
    //
public:
    s32     m_SquareX0, m_SquareY0;       // Anchor in terrain squares
    s32     m_SquaresInX, m_SquaresInY;     // Number of squares in fluid region
private:
    f32      m_DepthTexelX, m_DepthTexelY;   // MM: Added Depth Texel X/Y.
    s32     m_BlocksInX, m_BlocksInY;      // Number of blocks in fluid region
    f32     m_SurfaceZ;                     // Altitude of fluid surface
    s32     m_RemoveWetEdges;               // Dry fill all edges of the fluid block
    s32     m_HighResMode;                  // Blocks are 4x4 (high res) or 8x8 squares (normal)
    plane   m_Plane[6];                     // Frustrum clip planes: 0=T 1=B 2=L 3=R 4=N 5=F
    xyz     m_Eye;
    f32     m_Seconds;
    f32     m_BaseSeconds;
    rgba    m_FogColor;
    f32     m_VisibleDistance;
    f32     m_Opacity;
    f32     m_EnvMapIntensity;            // MM: Added Over/Under Env Texture Support.
    f32     m_WaveAmplitude;
    f32     m_WaveFactor;
    u16* m_pTerrain;         // 256x256 data for the terrain
    bool	m_UseDepthMap;					// MM: Use Depth Map Flag?
    F32		m_TessellationSurface;			// MM: Tessellation Surface.
    F32		m_TessellationShore;			// MM: Tessellation Shore.
    F32		m_SurfaceParallax;				// MM: Surface Parallax.
    F32		m_FlowAngle;					// MM: Flow Angle.
    F32		m_FlowRate;						// MM: Flow Rate.
    F32		m_FlowMagnitudeS;				// MM: Flow Magnitude S.
    F32		m_FlowMagnitudeT;				// MM: Flow Magnitude T.
    F32		m_DistortGridScale;				// MM: Distort Grid Scale.
    F32		m_DistortMagnitude;				// MM: Distort Magnitude.
    F32		m_DistortTime;					// MM: Distort Time.
    ColorF   m_SpecColor;
    F32      m_SpecPower;

    /*
        TextureHandle     m_BaseTexture;
        TextureHandle     m_EnvMapOverTexture;   // MM: Added Over/Under Env Texture Support.
        TextureHandle     m_EnvMapUnderTexture;   // MM: Added Over/Under Env Texture Support.
        TextureHandle     m_LightMapTexture;
        TextureHandle	  m_ShoreTexture;		// MM: Added Shore Texture.
        TextureHandle	  m_DepthTexture;		// MM: Added Depth Texture.
        TextureHandle	  m_ShoreDepthTexture;	// MM: Added Shore-Depth Texture.
       TextureHandle    m_SpecMaskTex;
    */

    f32     m_Step[5];          // [0] =  0
                                // [1] = 1/4 block step
                                // [2] = 1/2 block step
                                // [3] = 3/4 block step
                                // [4] =  1  block step

    compute_fog_fn* m_pFogFn;

    // Bit masks to trivially accept or reject the progressive levels for the
    // quad tree recursion.  No need for an accept mask at the highest level
    // since it is just the exact opposite of the reject mask at that level.

    static  s32     m_MaskOffset[6];  // Offset for given level into masks.

    byte            m_RejectMask[1 + 1 + 2 + 8 + 32 + 128];
    byte            m_AcceptMask[1 + 1 + 2 + 8 + 32];

    //
    // Shared among instances of fluid.
    //

    static  vertex* m_pVertex;
    static  s32     m_VAllocated;
    static  s32     m_VUsed;

    static  s16* m_pIndex;
    static  s16* m_pINext;
    static  s16     m_IOffset;
    static  s32     m_IAllocated;
    static  s32     m_IUsed;

    static  s32     m_Instances;

    //
    // Debug variables.
    //
public:
    s32     m_ShowWire;
    s32     m_ShowNodes;
    s32     m_ShowBlocks;
    s32     m_ShowBaseA;
    s32     m_ShowBaseB;
    s32     m_ShowLightMap;
    s32     m_ShowEnvMap;
    s32     m_ShowFog;

    //------------------------------------------------------------------------------
    //  Private Functions
    //
private:

    //
    // Functions in FluidSupport.cpp:
    //
    s32     GetAcceptBit(s32 Level, s32 IndexX, s32 IndexY) const;
    s32     GetRejectBit(s32 Level, s32 IndexX, s32 IndexY) const;
    void    SetAcceptBit(s32 Level, s32 IndexX, s32 IndexY, s32 Value);
    void    SetRejectBit(s32 Level, s32 IndexX, s32 IndexY, s32 Value);
    void    BuildLowerMasks(void);
    void    RebuildMasks(void);
    void    FloodFill(u8* pGrid, s32 x, s32 y, s32 SizeX, s32 SizeY);

    //
    // Functions in FluidQuadTree.cpp
    //
    void    RunQuadTree(bool& EyeSubmerged);

    f32     ComputeLOD(f32 Distance);
    byte    ComputeClipBits(f32 X, f32 Y, f32 Z);

    void    ProcessNode(node& Node);
    void    ProcessBlock(block& Block);

    void    ProcessBlockLODHigh(block& Block);
    void    ProcessBlockLODMorph(block& Block);
    void    ProcessBlockLODTrans(block& Block);
    void    ProcessBlockLODLow(block& Block);

    void    SetupVert(f32 X, f32 Y, f32 Distance, vertex* pV);

    void    InterpolateVerts(vertex* pV0,
        vertex* pV1,
        vertex* pV2,
        vertex* pV3,
        vertex* pV4,
        f32     LOD0,
        f32     LOD4);

    void    InterpolateVert(vertex* pV0,
        vertex* pV1,
        vertex* pV2,
        f32     LOD);

    void    ReleaseVertexMemory(void);
    vertex* AcquireVertices(s32 Count);
    void    AddTriangleIndices(s16 I1, s16 I2, s16 I3);

    void    CalcVertSpecular();
};

//==============================================================================
#endif // FLUID_HPP
//==============================================================================
