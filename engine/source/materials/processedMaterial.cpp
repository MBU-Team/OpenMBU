//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "processedMaterial.h"

void ProcessedMaterial::setBlendState(Material::BlendOp blendOp )
{
    switch( blendOp )
    {
        case Material::Add:
        {
            GFX->setSrcBlend( GFXBlendOne );
            GFX->setDestBlend( GFXBlendOne );
            break;
        }
        case Material::AddAlpha:
        {
            GFX->setSrcBlend( GFXBlendSrcAlpha );
            GFX->setDestBlend( GFXBlendOne );
            break;
        }
        case Material::Mul:
        {
            GFX->setSrcBlend( GFXBlendDestColor );
            GFX->setDestBlend( GFXBlendZero );
            break;
        }
        case Material::LerpAlpha:
        {
            GFX->setSrcBlend( GFXBlendSrcAlpha );
            GFX->setDestBlend( GFXBlendInvSrcAlpha );
            break;
        }

        default:
        {
            // default to LerpAlpha
            GFX->setSrcBlend( GFXBlendSrcAlpha );
            GFX->setDestBlend( GFXBlendInvSrcAlpha );
            break;
        }
    }
}

void ProcessedMaterial::setBuffers(GFXVertexBufferHandleBase* vertBuffer, GFXPrimitiveBufferHandle* primBuffer)
{
    GFX->setVertexBuffer( *vertBuffer );
    GFX->setPrimitiveBuffer( *primBuffer );
}

GFXTexHandle ProcessedMaterial::createTexture( const char* filename, GFXTextureProfile *profile)
{
    if (filename[0] == '.')
    {
        // full path pls
        char fullFilename[128];
        dStrncpy(fullFilename, mMaterial->getPath(), dStrlen(mMaterial->getPath()) + 1);
        dStrcat(fullFilename, filename);

        return GFXTexHandle(fullFilename, profile);
    }
    // if '/', then path is specified, open normally
    if( dStrstr( filename, "/" ) )
    {
        return GFXTexHandle( filename, profile );
    }

    // otherwise, construct path
    char fullFilename[128];
    dStrncpy( fullFilename, mMaterial->getPath(), dStrlen(mMaterial->getPath())+1 );
    dStrcat( fullFilename, filename );

    return GFXTexHandle( fullFilename, profile );
}
