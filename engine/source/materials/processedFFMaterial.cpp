//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "processedFFMaterial.h"
#include "gfx/cubemapData.h"
#include "materials/sceneData.h"

ProcessedFFMaterial::ProcessedFFMaterial()
{

}

ProcessedFFMaterial::ProcessedFFMaterial(Material &mat)
{
    mMaterial = &mat;
    mHasSetStageData = false;
    mCullMode = -1;
    mHasGlow = false;
}

void ProcessedFFMaterial::createPasses(U32 stageNum, SceneGraphData& sgData)
{
    FixedFuncFeatureData featData {};
    featData.features[0] = 1;
    determineFeatures(stageNum, featData, sgData);
    // Just create a simple pass!
    addPass(0, featData);
}

void ProcessedFFMaterial::determineFeatures(U32 stageNum, FixedFuncFeatureData& featData, SceneGraphData& sgData)
{
    if(mStages[stageNum].tex[ProcessedMaterial::BaseTex])
    {
        featData.features[FixedFuncFeatureData::BaseTex] = true;
    }
    if(!mMaterial->emissive[stageNum])
    {
        if( sgData.lightmap)
        {
            featData.features[FixedFuncFeatureData::Lightmap] = true;
        }
    }
    if (mMaterial->noiseTexFileName)
    {
        featData.features[FixedFuncFeatureData::NoiseTex] = true;
    }
}

U32 ProcessedFFMaterial::getNumStages()
{
    // Loops through all stages to determine how many stages we actually use
    U32 numStages = 0;

    U32 i;
    for( i=0; i<Material::MAX_STAGES; i++ )
    {
        // Assume stage is inactive
        bool stageActive = false;

        // Cubemaps only on first stage
        if( i == 0 )
        {
            // If we have a cubemap the stage is active
            if( mMaterial->mCubemapData || mMaterial->dynamicCubemap )
            {
                numStages++;
                continue;
            }
        }

        // Loop through all features
        for( U32 j=0; j<NumFeatures; j++ )
        {
            // If we have a texture for the feature the stage is active
            if( mStages[i].tex[j] )
            {
                stageActive = true;
                break;
            }
        }

        // If this stage has specular lighting, it's active
        if( mMaterial->pixelSpecular[i] ||
            mMaterial->vertexSpecular[i] )
        {
            stageActive = true;
        }

        // Increment the number of active stages
        numStages += stageActive;
    }


    return numStages;
}

void ProcessedFFMaterial::cleanup(U32 pass)
{
    if( mPasses.size())
    {
        for( U32 i=0; i<mPasses[pass].numTex; i++ )
        {
            GFX->setTexture(i, NULL);
        }
    }

    // Reset the previous cullmode
    if( mCullMode != -1 )
    {
        GFX->setCullMode( (GFXCullMode) mCullMode );
        mCullMode = -1;
    }

    // Misc. cleanup
    GFX->setAlphaBlendEnable( false );
    GFX->setAlphaTestEnable( false );
    GFX->setZWriteEnable( true );
    GFX->setTextureStageLODBias(0, 0.0f);
}

bool ProcessedFFMaterial::setupPass(SceneGraphData& sgData, U32 pass)
{
    // Make sure we have a pass
    if(pass >= mPasses.size())
        return false;
    // blend op for multipassing
    if( pass > 0 )
    {
        GFX->setAlphaBlendEnable( true );
        setBlendState( mPasses[pass].blendOp );
    }
    else
    {
        GFX->setAlphaBlendEnable( false );
    }

    // Deal with translucency
    if( mMaterial->translucent )
    {
        GFX->setAlphaBlendEnable( mMaterial->translucentBlendOp != Material::None );
        setBlendState( mMaterial->translucentBlendOp );
        GFX->setZWriteEnable( mMaterial->translucentZWrite );
        GFX->setAlphaTestEnable( mMaterial->alphaTest );
        GFX->setAlphaRef( mMaterial->alphaRef );
        GFX->setAlphaFunc( GFXCmpGreaterEqual );

        // set up register combiners
        GFX->setTextureStageAlphaOp( 0, GFXTOPModulate );
        GFX->setTextureStageAlphaOp( 1, GFXTOPDisable );
        GFX->setTextureStageAlphaArg1( 0, GFXTATexture );
        GFX->setTextureStageAlphaArg2( 0, GFXTADiffuse );
    }

    // Store the current cullmode so we can reset it when we're done
    if( mMaterial->doubleSided )
    {
        mCullMode = GFX->getCullMode();
        GFX->setCullMode( GFXCullNone );
    }

    GFX->setTextureStageLODBias(0, mMaterial->softwareMipOffset);

    // Bind our textures
    setTextureStages(sgData, pass);
    return true;
}

void ProcessedFFMaterial::setTextureStages(SceneGraphData& sgData, U32 pass)
{
    // We may need to do some trickery in here for fixed function, this is just copy/paste from MatInstance
#ifdef TORQUE_DEBUG
    AssertFatal( pass<mPasses.size(), "Pass out of bounds" );
#endif

    GFXTextureObject* baseTex;
    Point3I sz;
    MatrixF texMatrix;
    Point3F scale;

    for( U32 i=0; i<mPasses[pass].numTex; i++ )
    {
        GFX->setTextureStageColorOp( i, GFXTOPModulate );
        U32 currTexFlag = mPasses[pass].texFlags[i];
        if (!getCurrentClientSceneGraph()->getLightManager()->setTextureStage(sgData, currTexFlag, i))
        {
            switch( currTexFlag )
            {
                case 0:
                    GFX->setTextureStageAddressModeU( i, GFXAddressWrap );
                    GFX->setTextureStageAddressModeV( i, GFXAddressWrap );

                    if( mMaterial->isIFL() && sgData.miscTex )
                    {
                        GFX->setTexture( i, sgData.miscTex );
                    }
                    else
                    {
                        GFX->setTexture( i, mPasses[pass].tex[i] );
                    }
                    GFX->setTextureStageColorOp(i, GFXTOPModulate);
                    break;

                case Material::Misc:

                    GFX->setTextureStageAddressModeU( i, GFXAddressWrap );
                    GFX->setTextureStageAddressModeV( i, GFXAddressWrap );

                    GFX->setTextureStageMinFilter(i, GFXTextureFilterNone );
                    GFX->setTextureStageMagFilter(i, GFXTextureFilterNone );

                    GFX->setTextureStageColorOp(i, GFXTOPModulate2X);

                    GFX->setTexture(i, mPasses[pass].tex[i]);

                    GFX->setTextureStageTransform(i, GFXTTFCount2);

                    baseTex = mPasses[pass].tex[i];
                    sz = baseTex->mTextureSize;
                    scale = Point3F(1.0f / (F32)sz.x, 1.0f / (F32)sz.y, 1.0f);

                    texMatrix = MatrixF(true);
                    texMatrix.scale(scale);
                    GFX->setTextureMatrix(i, texMatrix);

                    break;

                case Material::NormalizeCube:
                    GFX->setCubeTexture(i, Material::getNormalizeCube());
                    break;

                case Material::Lightmap:
                    GFX->setTexture( i, sgData.lightmap );
                    GFX->setTextureStageColorOp(i, GFXTOPModulate);
                    break;

                case Material::NormLightmap:
                    GFX->setTexture( i, sgData.normLightmap );
                    break;

                case Material::Cube:
                    GFX->setTexture( i, mPasses[pass].tex[GFXShaderFeatureData::BaseTex] );
                    break;

                case Material::SGCube:
                    // No cubemap support just yet
                    //GFX->setCubeTexture( i, sgData.cubemap );
                    GFX->setTexture( i, mPasses[pass].tex[GFXShaderFeatureData::BaseTex] );
                    break;

                case Material::Fog:
                {
                    GFX->setTextureStageAddressModeU( i, GFXAddressClamp );
                    GFX->setTextureStageAddressModeV( i, GFXAddressClamp );
                    GFX->setTexture( i, sgData.fogTex );
                    break;
                }

                case Material::BackBuff:
                    GFX->setTexture( i, sgData.backBuffTex );
                    break;
            }
        }
    }
}

void ProcessedFFMaterial::setShaderConstants(const SceneGraphData &sgData, U32 pass)
{

}

void ProcessedFFMaterial::setObjectXForm(MatrixF xform, U32 pass)
{

}

void ProcessedFFMaterial::setWorldXForm(MatrixF xform)
{
    // The matrix we're given is Model * View * Projection, so set two matrices to identity and the third to this.
    MatrixF ident(1);
    xform.transpose();
    GFX->setWorldMatrix(ident);
    GFX->setViewMatrix(ident);
    GFX->setProjectionMatrix(xform);
}

void ProcessedFFMaterial::setLightInfo(const SceneGraphData& sgData, U32 pass)
{
    setPrimaryLightInfo(sgData.objTrans, (LightInfo*)&sgData.light, pass);
    setSecondaryLightInfo(sgData.objTrans, (LightInfo*)&sgData.lightSecondary);
}

void ProcessedFFMaterial::setPrimaryLightInfo(MatrixF objTrans, LightInfo* light, U32 pass)
{
    // Just in case
    GFX->setGlobalAmbientColor(ColorF(0.0f, 0.0f, 0.0f, 1.0f));
    if(light->mType == LightInfo::Ambient)
    {
        // Ambient light
        GFX->setGlobalAmbientColor(light->mAmbient);
        return;
    }

    GFX->setLight(0, NULL);
    GFX->setLight(1, NULL);
    // This is a quick hack that lets us use FF lights
    GFXLightMaterial lightMat;
    lightMat.ambient = ColorF(1.0f, 1.0f, 1.0f, 1.0f);
    lightMat.diffuse = ColorF(1.0f, 1.0f, 1.0f, 1.0f);
    lightMat.emissive = ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    lightMat.specular = ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    lightMat.shininess = 128.0f;
    GFX->setLightMaterial(lightMat);
    GFX->setLightingEnable(true);

    // set object transform
    objTrans.inverse();

    // fill in primary light
    //-------------------------
    LightInfo xlatedLight;
    light->setGFXLight(&xlatedLight);
    Point3F lightPos = light->mPos;
    Point3F lightDir = light->mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    xlatedLight.mPos = lightPos;
    xlatedLight.mDirection = lightDir;

    GFX->setLight(0, &xlatedLight);
}

void ProcessedFFMaterial::setSecondaryLightInfo(MatrixF objTrans, LightInfo* light)
{
    // set object transform
    objTrans.inverse();

    // fill in secondary light
    //-------------------------
    LightInfo xlatedLight;
    light->setGFXLight(&xlatedLight);

    Point3F lightPos = light->mPos;
    Point3F lightDir = light->mDirection;
    objTrans.mulP(lightPos);
    objTrans.mulV(lightDir);

    xlatedLight.mPos = lightPos;
    xlatedLight.mDirection = lightDir;

    GFX->setLight(1, &xlatedLight);
}

void ProcessedFFMaterial::setEyePosition(MatrixF objTrans, Point3F position, U32 pass)
{

}

void ProcessedFFMaterial::init(SceneGraphData& sgData, GFXVertexFlags vertFlags)
{
    setStageData();
    // Just create a simple pass
    createPasses(0, sgData);
}

void ProcessedFFMaterial::setStageData()
{
    // Make sure we don't do this more than once
    if( mHasSetStageData ) return;
    mHasSetStageData = true;

    // Load the textures for every possible stage
    for( U32 i=0; i<Material::MAX_STAGES; i++ )
    {
        if( mMaterial->baseTexFilename[i] && mMaterial->baseTexFilename[i][0] )
        {
            mStages[i].tex[BaseTex] = createTexture( mMaterial->baseTexFilename[i], &GFXDefaultStaticDiffuseProfile );
        }

        if (mMaterial->noiseTexFileName && mMaterial->noiseTexFileName[0])
        {
            mStages[i].tex[DetailMap] = createTexture(mMaterial->noiseTexFileName, &GFXFFNoiseMapProfile );
        }

        if( mMaterial->detailFilename[i] && mMaterial->detailFilename[i][0] )
        {
            mStages[i].tex[DetailMap] = createTexture( mMaterial->detailFilename[i], &GFXDefaultStaticDiffuseProfile );
        }

        if( mMaterial->bumpFilename[i] && mMaterial->bumpFilename[i][0] )
        {
            mStages[i].tex[BumpMap] = createTexture( mMaterial->bumpFilename[i], &GFXDefaultStaticDiffuseProfile );
        }

        if( mMaterial->envFilename[i] && mMaterial->envFilename[i][0] )
        {
            mStages[i].tex[EnvMap] = createTexture( mMaterial->envFilename[i], &GFXDefaultStaticDiffuseProfile );
        }
    }

    // If we have a cubemap, load it for stage 0 (cubemaps are not supported on other stages)
    if( mMaterial->mCubemapData )
    {
        mMaterial->mCubemapData->createMap();
        mStages[0].cubemap = mMaterial->mCubemapData->cubemap;
    }
}

void ProcessedFFMaterial::setTextureTransforms()
{

}

void ProcessedFFMaterial::addPass(U32 stageNum, FixedFuncFeatureData& featData)
{
    U32 numTex = 0;
    // Just creates a simple pass, but it can still glow!
    RenderPassData rpd;
    // Base texture, texunit 0
    if(featData.features[FixedFuncFeatureData::BaseTex])
    {
        rpd.tex[0] = mStages[stageNum].tex[BaseTex];
        rpd.texFlags[0] = 0;
        numTex++;
    }
    // lightmap, texunit 1
    if(featData.features[FixedFuncFeatureData::Lightmap])
    {
        rpd.tex[1] = mStages[stageNum].tex[LightMap];
        rpd.texFlags[1] = Material::Lightmap;
        numTex++;
    }
    if (featData.features[FixedFuncFeatureData::NoiseTex])
    {
        rpd.tex[2] = mStages[stageNum].tex[DetailMap];
        rpd.texFlags[2] = Material::Misc;
        numTex++;
    }
    rpd.numTex = numTex;
    rpd.stageNum = stageNum;
    rpd.glow = false;

    mPasses.push_back( rpd );
}

void ProcessedFFMaterial::setPassBlendOp()
{

}

void ProcessedFFMaterial::setLightingBlendFunc()
{
    // don't break the material multipass rendering...
    GFX->setAlphaBlendEnable(true);
    GFX->setSrcBlend(GFXBlendOne);
    GFX->setDestBlend(GFXBlendOne);
}
