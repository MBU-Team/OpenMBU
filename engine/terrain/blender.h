//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BLENDER_H_
#define _BLENDER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#ifndef _TERRRENDER_H_
#include "terrain/terrRender.h"
#endif


#define GRIDFLAGS(X,Y)   (TerrainRender::mCurrentBlock->findSquare( 0, Point2I( X, Y ) )->flags)
#define MATERIALSTART       (GridSquare::MaterialStart)

/// More and better docs forthcoming.
class Blender
{
    struct SourceTexture
    {
        /// This is the transform applied to the texture.
        ///
        /// This allows you to easily rotate, scale, and otherwise
        /// mess with textures on the surface of the terrain. This
        /// feature is best used to make tiling less obvious, or
        /// to compensate for mis-scaled source art.
        MatrixF xfrm;

        /// The texture itself.
        GFXTexHandle source;

        /// Alphamap.
        ///
        /// The alphamap lets us now how much of the texture to blend
        /// on different parts of the chunk. It will be stretched to
        /// cover the entire chunk, so in theory you could have very
        /// high or low res alphamaps. In practice, the blender just
        /// uses the same size alphamap in all cases.
        U8* alpha;

        SourceTexture()
            : source(NULL), xfrm(1)
        {
            alpha = NULL;
        }

        ~SourceTexture()
        {
            source = NULL;
            alpha = NULL;
        }
    };

    Vector<SourceTexture> mSources;
    Vector<GFXTexHandle>  mOpacity;
    TerrainBlock* mOwner;


    static GFXTexHandle  mMipBuffer; // We keep this bad boy around to help make mip levels.
    static S32           mTexSize;

public:
    Blender(TerrainBlock* owner);
    ~Blender();

    /// @param  x           Position in blocks.
    /// @param  y           Position in blocks.
    /// @param  lightmap    A lightmap texture.
    GFXTexHandle blend(S32 x, S32 y, S32 level, GFXTexHandle lightmap, GFXTexHandle oldSurface = NULL);

    /// Add a texture to use in blending.
    ///
    /// Since textures are refcounted, the texture you pass in won't be freed
    /// until the Blender is done with it.
    ///
    /// @param  materialIdx    Index of the material for use with alphamaps and such.
    /// @param  bmp            Handle to the texture.
    void addSourceTexture(U32 materialIdx, GFXTexHandle bmp, U8* alphas);

    /// Call this when you have done all your source texture updates.
    void updateOpacity();

    void zombify();
    void resurrect();
};

GFX_DeclareTextureProfile(TerrainBlenderTextureProfile);
GFX_DeclareTextureProfile(TerrainBlenderOpacityProfile);


#endif
