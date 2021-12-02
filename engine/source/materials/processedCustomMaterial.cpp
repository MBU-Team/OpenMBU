//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "processedCustomMaterial.h"
#include "gfx/cubemapData.h"
#include "materials/sceneData.h"
#include "shaderGen/featureMgr.h"
#include SHADER_CONSTANT_INCLUDE_FILE
#include "materials/customMaterial.h"
#include "materials/shaderData.h"

ProcessedCustomMaterial::ProcessedCustomMaterial(Material &mat)
{
    mMaterial = &mat;
    mHasSetStageData = false;
    mHasGlow = false;
    mMaxStages = 0;
    mCullMode = -1;
    mMaxTex = 0;
    mNumCustomPasses = 0;
    dMemset(mCustomPasses, 0, sizeof(mCustomPasses));
}

ProcessedCustomMaterial::~ProcessedCustomMaterial()
{
    for (U32 i = 0; i < mNumCustomPasses; i++)
    {
        if (mCustomPasses[i])
            delete mCustomPasses[i];
    }
}

void ProcessedCustomMaterial::setStageData()
{
    // Only do this once
    if( mHasSetStageData ) return;
    mHasSetStageData = true;

    // Should probably be keeping this as a member.
    CustomMaterial* custMat = static_cast<CustomMaterial*>(mMaterial);

    S32 i;

    // Loop through all the possible textures, set the right flags, and load them if needed
    for( i=0; i<CustomMaterial::MAX_TEX_PER_PASS; i++ )
    {
        mFlags[i] = Material::NoTexture;   // Set none as the default in case none of the cases below catch it.

        if( !custMat->texFilename[i] ) continue;

        if(!dStrcmp(custMat->texFilename[i], "$dynamiclight"))
        {
            mFlags[i] = Material::DynamicLight;
            mMaxTex = i+1;
            continue;
        }

        if(!dStrcmp(custMat->texFilename[i], "$dynamiclightmask"))
        {
            mFlags[i] = Material::DynamicLightMask;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$lightmap" ) )
        {
            mFlags[i] = Material::Lightmap;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$normmap" ) )
        {
            mFlags[i] = Material::NormLightmap;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$fog" ) )
        {
            mFlags[i] = Material::Fog;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$cubemap" ) )
        {
            if( custMat->mCubemapData )
            {
                mFlags[i] = Material::Cube;
                mMaxTex = i+1;
            }
            else
            {
                Con::warnf( "Invalid cubemap data for CustomMaterial - %s : %s", custMat->getName(), custMat->cubemapName );
            }
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$dynamicCubemap" ) )
        {
            mFlags[i] = Material::SGCube;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$backbuff" ) )
        {
            mFlags[i] = Material::BackBuff;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$reflectbuff" ) )
        {
            mFlags[i] = Material::ReflectBuff;
            mMaxTex = i+1;
            continue;
        }

        if( !dStrcmp( custMat->texFilename[i], "$miscbuff" ) )
        {
            mFlags[i] = Material::Misc;
            mMaxTex = i+1;
            continue;
        }

        if( custMat->texFilename[i] && custMat->texFilename[i][0] )
        {
            mStages[0].tex[i] = createTexture( custMat->texFilename[i], &GFXDefaultStaticDiffuseProfile );
            mFlags[i] = Material::Standard;
            mMaxTex = i+1;
        }
    }

    // We only get one cubemap
    if( custMat->mCubemapData )
    {
        custMat->mCubemapData->createMap();
        mStages[0].cubemap = mMaterial->mCubemapData->cubemap;
    }

    // Load our shader, if we have one.
    if( custMat->mShaderData && !custMat->mShaderData->getShader() )
    {
        custMat->mShaderData->initShader();
    }

    // Glow!
    if(custMat->glow[0])
        mHasGlow = true;
}

void ProcessedCustomMaterial::init(SceneGraphData& sgData, GFXVertexFlags vertFlags)
{
    // Just have to load the stage data, also loads the shader
    setStageData();

    CustomMaterial* customMat = static_cast<CustomMaterial*>(mMaterial);
    for (U32 i = 0; i < CustomMaterial::MAX_PASSES; i++)
    {
        if (!customMat->pass[i])
            break;
        mNumCustomPasses++;
        mCustomPasses[i] = new ProcessedCustomMaterial(*customMat->pass[i]);
        mCustomPasses[i]->init(sgData, vertFlags);
    }

    return;
}

bool ProcessedCustomMaterial::hasCubemap(U32 pass)
{
    // If the material doesn't have a cubemap, we don't
    if( mMaterial->mCubemapData ) return true;
    else return false;
}

bool ProcessedCustomMaterial::setupPassInternal(SceneGraphData& sgData, U32 pass)
{
    if (pass > mNumCustomPasses)
        return false;

    // Store cullmode
    if( pass == 0 && mMaterial->doubleSided )
    {
        GFX->setCullMode( GFXCullNone );
        mCullMode = GFX->getCullMode();
    }

    // Deal with translucency
    if( mMaterial->isTranslucent() )
    {
        GFX->setAlphaBlendEnable( mMaterial->translucentBlendOp != Material::None );
        //GFX->setAlphaBlendEnable( true );
        setBlendState( mMaterial->translucentBlendOp );
        GFX->setZWriteEnable( mMaterial->translucentZWrite );
        GFX->setAlphaTestEnable( mMaterial->alphaTest );
        //GFX->setAlphaTestEnable( true );
        GFX->setAlphaRef( mMaterial->alphaRef );
        //GFX->setAlphaRef( true );
        GFX->setAlphaFunc( GFXCmpGreaterEqual );

        // set up register combiners
        GFX->setTextureStageAlphaOp( 0, GFXTOPModulate );
        GFX->setTextureStageAlphaOp( 1, GFXTOPDisable );
        GFX->setTextureStageAlphaArg1( 0, GFXTATexture );
        GFX->setTextureStageAlphaArg2( 0, GFXTADiffuse );
    }
    else
    {
        GFX->setAlphaBlendEnable( false );
    }

    // Do more stuff
    setupSubPass( sgData, pass );

    return true;
}

void ProcessedCustomMaterial::setupSubPass(SceneGraphData& sgData, U32 pass)
{
    CustomMaterial* custMat = static_cast<CustomMaterial*>(mMaterial);

    // Set the right blend state
    setBlendState( custMat->blendOp );

    // activate shader
    if( custMat->mShaderData && custMat->mShaderData->getShader() )
    {
        custMat->mShaderData->getShader()->process();
    }
    else
    {
        GFX->disableShaders();
        GFX->setTextureStageColorOp( mMaxTex, GFXTOPDisable );
    }

    // Set our textures
    setTextureStages( sgData, pass );
}

bool ProcessedCustomMaterial::setupPass(SceneGraphData& sgData, U32 pass)
{
    if (mCustomPasses[pass])
        return mCustomPasses[pass]->setupPass(sgData, pass);

    if (sgData.refractPass || !static_cast<CustomMaterial*>(mMaterial)->refract)
    {
        // This setTexTrans nonsense is set to make sure that setTextureTransforms()
        // is called only once per material draw, not per pass
        static bool setTexTrans = false;
        if (pass == 0 && !setTexTrans)
        {
            setTextureTransforms();
            setTexTrans = true;
        }

        if (setupPassInternal(sgData, pass))
        {
            setShaderConstants(sgData, pass);
            GFX->setPixelShaderConstF(PC_ACCUM_TIME, &CustomMaterial::mAccumTime, 1);
            return true;
        }

        setTexTrans = false;
    }
    return false;
}

void ProcessedCustomMaterial::setTextureStages(SceneGraphData &sgData, U32 pass )
{
    for( U32 i=0; i<mMaxTex; i++ )
    {
        GFX->setTextureStageColorOp( i, GFXTOPModulate );

        U32 currTexFlag = mFlags[i];
        if (!getCurrentClientSceneGraph()->getLightManager()->setTextureStage(sgData, currTexFlag, i))
        {
            switch( currTexFlag )
            {
                case 0:
                default:
                    break;

                case Material::Mask:
                case Material::Standard:
                case Material::Bump:
                case Material::Detail:
                {
                    GFX->setTextureStageAddressModeU( i, GFXAddressWrap );
                    GFX->setTextureStageAddressModeV( i, GFXAddressWrap );
                    GFX->setTexture( i, mStages[0].tex[i] );
                    break;
                }
                case Material::Lightmap:
                {
                    GFX->setTexture( i, sgData.lightmap );
                    break;
                }
                case Material::NormLightmap:
                {
                    GFX->setTexture( i, sgData.normLightmap );
                    break;
                }
                case Material::Fog:
                {
                    GFX->setTextureStageAddressModeU( i, GFXAddressClamp );
                    GFX->setTextureStageAddressModeV( i, GFXAddressClamp );
                    GFX->setTexture( i, sgData.fogTex );
                    break;
                }
                case Material::Cube:
                {
                    GFX->setCubeTexture( i, mStages[0].cubemap );
                    break;
                }
                case Material::SGCube:
                {
                    GFX->setCubeTexture( i, sgData.cubemap );
                    break;
                }
                case Material::BackBuff:
                {
                    GFX->setTexture( i, sgData.backBuffTex );
                    break;
                }
                case Material::ReflectBuff:
                {
                    GFX->setTexture( i, sgData.reflectTex );
                    break;
                }
                case Material::Misc:
                {
                    GFX->setTexture( i, sgData.miscTex );
                    break;
                }

            }
        }
    }
}

bool ProcessedCustomMaterial::doesRefract(U32 pass) const {
    ProcessedCustomMaterial* customPass = mCustomPasses[pass];
    if (customPass)
        return customPass->doesRefract(pass);
    if (pass == mNumCustomPasses)
        return ((CustomMaterial*)mMaterial)->refract;
}

void ProcessedCustomMaterial::cleanup(U32 pass)
{
    // This needs to be modified to actually clean up the specified pass.
    // It's enough if we're just using shaders, but mix shader and FF
    // and you're in a whole world of trouble.
    for( U32 i=0; i<mMaxTex; i++ )
    {
        // set up textures
        switch( mFlags[i] )
        {
            case 0:
            default:
                break;

            case Material::BackBuff:
            {
                // have to unbind render targets or D3D complains
                GFX->setTexture( i, NULL );
                break;
            }
            case Material::ReflectBuff:
            {
                // have to unbind render targets or D3D complains
                GFX->setTexture( i, NULL );
                break;
            }

        }
    }

    // Reset cull mode
    if( mCullMode != -1 )
    {
        GFX->setCullMode( (GFXCullMode) mCullMode );
        mCullMode = -1;
    }

    // Misc. cleanup
    GFX->setAlphaBlendEnable( false );
    GFX->setAlphaTestEnable( false );
    GFX->setZWriteEnable( true );
}