//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "processedShaderMaterial.h"
#include "gfx/cubemapData.h"
#include "materials/sceneData.h"
#include "shaderGen/shaderFeature.h"
#include SHADER_CONSTANT_INCLUDE_FILE

ProcessedShaderMaterial::ProcessedShaderMaterial()
{

}

ProcessedShaderMaterial::ProcessedShaderMaterial(Material &mat)
{
    mMaterial = &mat;
    mHasSetStageData = false;
    mHasGlow = false;
    mMaxStages = 0;
    mCullMode = -1;
}

void ProcessedShaderMaterial::determineFeatures( U32 stageNum, GFXShaderFeatureData &fd, SceneGraphData& sgData )
{
    // Sanity check
    if( GFX->getPixelShaderVersion() == 0.0 )
    {
        AssertFatal(false, "Cannot create a shader material if we don't support shaders");
    }

    // CodeReview: [btr, 7/23/07] Is there a reason that this is a loop?  Why not just do fd.features[MyFeature] = blah?  (Except for the texture block at the bottom)
    for( U32 i=0; i<GFXShaderFeatureData::NumFeatures; i++ )
    {
        if (i == GFXShaderFeatureData::Translucent)
        {
            fd.features[i] = mMaterial->translucent;
        }
        // if normal/bump mapping disabled, continue
        if( Con::getBoolVariable( "$pref::Video::disableNormalmapping", false ) &&
            (i == GFXShaderFeatureData::BumpMap || i == GFXShaderFeatureData::LightNormMap) )
        {
            continue;
        }

        if((i == GFXShaderFeatureData::SelfIllumination) && mMaterial->emissive[stageNum])
            fd.features[i] = true;

        if((i == GFXShaderFeatureData::ExposureX2) &&
           (mMaterial->exposure[stageNum] == 2))
            fd.features[i] = true;

        if((i == GFXShaderFeatureData::ExposureX4) &&
           (mMaterial->exposure[stageNum] == 4))
            fd.features[i] = true;

        // texture coordinate animation
        if( i == GFXShaderFeatureData::TexAnim )
        {
            if( mMaterial->animFlags[stageNum] )
            {
                fd.features[i] = true;
            }
        }

        // vertex shading
        if( i == GFXShaderFeatureData::RTLighting )
        {
            if( sgData.useLightDir &&
                !mMaterial->emissive[stageNum])
            {
                fd.features[i] = true;
            }
        }

        // pixel specular
        if( GFX->getPixelShaderVersion() >= 2.0 )
        {
            if((i == GFXShaderFeatureData::PixSpecular) &&
               !Con::getBoolVariable( "$pref::Video::disablePixSpecular", false ) )
            {
                if( mMaterial->pixelSpecular[stageNum] )
                {
                    fd.features[i] = true;
                }
            }
        }

        // vertex specular
        if( i == GFXShaderFeatureData::VertSpecular )
        {
            if( mMaterial->vertexSpecular[stageNum] )
            {
                fd.features[i] = true;
            }
        }


        // lightmap
        if( i == GFXShaderFeatureData::LightMap &&
            !mMaterial->emissive[stageNum] &&
            !mMaterial->glow[stageNum] &&
            stageNum == (mMaxStages-1) )
        {
            if( sgData.lightmap)
            {
                fd.features[i] = true;
            }
        }

        // lightNormMap
        if( i == GFXShaderFeatureData::LightNormMap &&
            !mMaterial->emissive[stageNum] &&
            !mMaterial->glow[stageNum] &&
            stageNum == (mMaxStages-1) )
        {
            if( sgData.normLightmap )
            {
                fd.features[i] = true;
            }
        }

        // cubemap
        if( i == GFXShaderFeatureData::CubeMap &&
            stageNum < 1 &&      // cubemaps only available on stage 0 for now - bramage
            ( ( mMaterial->mCubemapData && mMaterial->mCubemapData->cubemap ) || mMaterial->dynamicCubemap ) &&
            !Con::getBoolVariable( "$pref::Video::disableCubemapping", false ) )
        {
            fd.features[i] = true;
        }

        // Visibility
        if ( i == GFXShaderFeatureData::Visibility)
        {
            fd.features[i] = true;
        }

        // Color multiply
        if (i == GFXShaderFeatureData::ColorMultiply)
        {
            if (mMaterial->colorMultiply[stageNum].alpha != 0)
                fd.features[i] = true;
        }

        // These features only happen in the last state
        //if (stageNum == (mMaxStages-1))
        //{
        //    // fog - last stage only
        //    if( i == GFXShaderFeatureData::Fog && sgData.useFog)
        //    {
        //        fd.features[i] = true;
        //    }
        //}

        // textures
        if( mStages[stageNum].tex[i] )
        {
            fd.features[i] = true;
        }
    }
}

void ProcessedShaderMaterial::createPasses( GFXShaderFeatureData &stageFeatures, U32 stageNum, SceneGraphData& sgData )
{
    // Creates passes for the given stage
    RenderPassData passData;
    U32 texIndex = 0;

    for( U32 i=0; i<GFXShaderFeatureData::NumFeatures; i++ )
    {
        if( !stageFeatures.features[i] )
            continue;

        ShaderFeature *sf = gFeatureMgr.get( i );
        if (!sf)
            continue;

        U32 numTexReg = sf->getResources( passData.fData ).numTexReg;

        // adds pass if blend op changes for feature
        setPassBlendOp( sf, passData, texIndex, stageFeatures, stageNum, sgData );

        // Add pass if num tex reg is going to be too high
        if( passData.numTexReg + numTexReg > GFX->getNumSamplers() )
        {
            addPass( passData, texIndex, stageFeatures, stageNum, sgData );
            setPassBlendOp( sf, passData, texIndex, stageFeatures, stageNum, sgData );
        }

        passData.numTexReg += numTexReg;
        passData.fData.features[i] = true;
        sf->setTexData( mStages[stageNum], stageFeatures, passData, texIndex );

        // Add pass if tex units are maxed out
        if( texIndex > GFX->getNumSamplers() )
        {
            addPass( passData, texIndex, stageFeatures, stageNum, sgData );
            setPassBlendOp( sf, passData, texIndex, stageFeatures, stageNum, sgData );
        }
    }

    if( passData.fData.codify() )
    {
        addPass( passData, texIndex, stageFeatures, stageNum, sgData );
    }
}

U32 ProcessedShaderMaterial::getNumStages()
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

void ProcessedShaderMaterial::cleanup(U32 pass)
{
    // This needs to be modified to actually clean up the specified pass.
    // It's enough if we're just using shaders, but mix shader and FF
    // and you're in a whole world of trouble.
    if( mPasses.size())
    {
        // DX warning if dynamic cubemap not explicitly unbound
        for( U32 i=0; i<mPasses[pass].numTex; i++ )
        {
            if( mPasses[pass].texFlags[i] == Material::BackBuff )
            {
                GFX->setTexture( i, NULL );
                continue;
            }
            if( mPasses[pass].texFlags[i] == Material::SGCube )
            {
                GFX->setTexture( i, NULL );
                continue;
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

bool ProcessedShaderMaterial::setupPass(SceneGraphData& sgData, U32 pass)
{
    // Make sure we have the pass
    if(pass >= mPasses.size())
        return false;
    // Deal with mulitpass blending
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
    if( mMaterial->translucent || sgData.visibility < 1.0f )
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

    //set shaders
    if( mPasses[pass].shader )
    {
        mPasses[pass].shader->process();
        setShaderConstants( sgData, pass);
    }
    else
    {
        GFX->disableShaders();
        GFX->setTextureStageColorOp( mPasses[pass].numTex, GFXTOPDisable );
    }

    // Set our textures
    setTextureStages( sgData, pass );

    if( pass == 0 )
    {
        // Only do this for the first pass
        setTextureTransforms();

        if( mMaterial->doubleSided )
        {
            mCullMode = GFX->getCullMode();
            GFX->setCullMode( GFXCullNone );
        }
    }
    return true;
}

void ProcessedShaderMaterial::setTextureStages( SceneGraphData &sgData, U32 pass )
{
    // Set all of the textures we need to render the give pass.
#ifdef TORQUE_DEBUG
    AssertFatal( pass<mPasses.size(), "Pass out of bounds" );
#endif

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

                case Material::NormalizeCube:
                    GFX->setCubeTexture(i, Material::getNormalizeCube());
                    break;

                case Material::Lightmap:
                    GFX->setTexture( i, sgData.lightmap );
                    break;

                case Material::NormLightmap:
                    GFX->setTexture( i, sgData.normLightmap );
                    break;

                case Material::Cube:
                    GFX->setCubeTexture( i, mPasses[pass].cubeMap );
                    break;

                case Material::SGCube:
                    GFX->setCubeTexture( i, sgData.cubemap );
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

void ProcessedShaderMaterial::setTextureTransforms()
{
    U32 i=0;

    MatrixF texMat( true );

    mMaterial->updateTimeBasedParams();
    F32 waveOffset = getWaveOffset( i ); // offset is between 0.0 and 1.0

    // handle scroll anim type
    if(  mMaterial->animFlags[i] & Material::Scroll )
    {
        if( mMaterial->animFlags[i] & Material::Wave )
        {
            Point3F scrollOffset;
            scrollOffset.x = mMaterial->scrollDir[i].x * waveOffset;
            scrollOffset.y = mMaterial->scrollDir[i].y * waveOffset;
            scrollOffset.z = 1.0;

            texMat.setColumn( 3, scrollOffset );
        }
        else
        {
            Point3F offset( mMaterial->scrollOffset[i].x,
                            mMaterial->scrollOffset[i].y,
                            1.0 );

            texMat.setColumn( 3, offset );
        }

    }

    // handle rotation
    if( mMaterial->animFlags[i] & Material::Rotate )
    {
        if( mMaterial->animFlags[i] & Material::Wave )
        {
            F32 rotPos = waveOffset * M_2PI;
            texMat.set( EulerF( 0.0, 0.0, rotPos ) );
            texMat.setColumn( 3, Point3F( 0.5, 0.5, 0.0 ) );

            MatrixF test( true );
            test.setColumn( 3, Point3F( mMaterial->rotPivotOffset[i].x,
                                        mMaterial->rotPivotOffset[i].y,
                                        0.0 ) );
            texMat.mul( test );
        }
        else
        {
            texMat.set( EulerF( 0.0, 0.0, mMaterial->rotPos[i] ) );

            texMat.setColumn( 3, Point3F( 0.5, 0.5, 0.0 ) );

            MatrixF test( true );
            test.setColumn( 3, Point3F( mMaterial->rotPivotOffset[i].x,
                                        mMaterial->rotPivotOffset[i].y,
                                        0.0 ) );
            texMat.mul( test );
        }
    }

    // Handle scale + wave offset
    if(  mMaterial->animFlags[i] & Material::Scale &&
         mMaterial->animFlags[i] & Material::Wave )
    {
        F32 wOffset = fabs( waveOffset );

        texMat.setColumn( 3, Point3F( 0.5, 0.5, 0.0 ) );

        MatrixF temp( true );
        temp.setRow( 0, Point3F( wOffset,  0.0,  0.0 ) );
        temp.setRow( 1, Point3F( 0.0,  wOffset,  0.0 ) );
        temp.setRow( 2, Point3F( 0.0,  0.0,  wOffset ) );
        temp.setColumn( 3, Point3F( -wOffset * 0.5, -wOffset * 0.5, 0.0 ) );
        texMat.mul( temp );
    }

    // handle sequence
    if( mMaterial->animFlags[i] & Material::Sequence )
    {
        U32 frameNum = (U32)(mMaterial->mAccumTime * mMaterial->seqFramePerSec[i]);
        F32 offset = frameNum * mMaterial->seqSegSize[i];

        Point3F texOffset = texMat.getPosition();
        texOffset.x += offset;
        texMat.setPosition( texOffset );
    }

    texMat.transpose();
    GFX->setVertexShaderConstF( VC_TEX_TRANS1, (float*) &texMat, 4 );

}

//--------------------------------------------------------------------------
// Get wave offset for texture animations using a wave transform
//--------------------------------------------------------------------------
F32 ProcessedShaderMaterial::getWaveOffset( U32 stage )
{
    switch( mMaterial->waveType[stage] )
    {
        case Material::Sin:
        {
            return mMaterial->waveAmp[stage] * mSin( M_2PI * mMaterial->wavePos[stage] );
            break;
        }

        case Material::Triangle:
        {
            F32 frac = mMaterial->wavePos[stage] - mFloor( mMaterial->wavePos[stage] );
            if( frac > 0.0 && frac <= 0.25 )
            {
                return mMaterial->waveAmp[stage] * frac * 4.0;
            }

            if( frac > 0.25 && frac <= 0.5 )
            {
                return mMaterial->waveAmp[stage] * ( 1.0 - ((frac-0.25)*4.0) );
            }

            if( frac > 0.5 && frac <= 0.75 )
            {
                return mMaterial->waveAmp[stage] * (frac-0.5) * -4.0;
            }

            if( frac > 0.75 && frac <= 1.0 )
            {
                return -mMaterial->waveAmp[stage] * ( 1.0 - ((frac-0.75)*4.0) );
            }

            break;
        }

        case Material::Square:
        {
            F32 frac = mMaterial->wavePos[stage] - mFloor( mMaterial->wavePos[stage] );
            if( frac > 0.0 && frac <= 0.5 )
            {
                return 0.0;
            }
            else
            {
                return mMaterial->waveAmp[stage];
            }
            break;
        }

    }

    return 0.0;
}

void ProcessedShaderMaterial::setShaderConstants(const SceneGraphData &sgData, U32 pass)
{
    U32 stageNum = getStageFromPass(pass);

    // set eye positions
    //-------------------------
    Point3F eyePos   = sgData.camPos;
    MatrixF objTrans = sgData.objTrans;

    setObjectXForm(objTrans, pass);
    setEyePosition(objTrans, eyePos, pass);
    setLightInfo(sgData, pass);

    // this is OK for now, will need to change later to support different
    // specular values per pass in custom materials
    //-------------------------
    GFX->setPixelShaderConstF( PC_MAT_SPECCOLOR, (float*)&mMaterial->specular[stageNum], 1 );
    GFX->setPixelShaderConstF( PC_MAT_SPECPOWER, (float*)&mMaterial->specularPower[stageNum], 1 );
    GFX->setVertexShaderConstF( VC_MAT_SPECPOWER, (float*)&mMaterial->specularPower[stageNum], 1 );

    // fog
    //-------------------------
    Point4F fogData;
    fogData.x = sgData.fogHeightOffset;
    fogData.y = sgData.fogInvHeightRange;
    fogData.z = sgData.visDist;
    GFX->setVertexShaderConstF( VC_FOGDATA, (float*)&fogData, 1 );

    // set detail scale
    //   F32 scale = 2.0;
    //   GFX->setVertexShaderConstF( VC_DETAIL_SCALE, &scale, 1 );

    // Visibility
    F32 visibility = sgData.visibility;
    GFX->setPixelShaderConstF( PC_VISIBILITY, &visibility, 1 );

    // Color multiply
    if (mMaterial->colorMultiply[stageNum].alpha > 0.0f)
    {
        GFX->setPixelShaderConstF(PC_COLORMULTIPLY, (float*)&mMaterial->colorMultiply[stageNum], 1);
    }
}

bool ProcessedShaderMaterial::hasCubemap(U32 pass)
{
    // Only support cubemap on the first stage
    if( mPasses[pass].stageNum > 0 )
    {
        return false;
    }

    if( mPasses[pass].cubeMap )
    {
        return true;
    }
    return false;
}

void ProcessedShaderMaterial::setObjectXForm(MatrixF xform, U32 pass)
{
    // Set cubemap stuff here (it's convenient!)
    if( hasCubemap(pass) || mMaterial->dynamicCubemap)
    {
        MatrixF cubeTrans = xform;
        cubeTrans.setPosition( Point3F( 0.0, 0.0, 0.0 ) );
        cubeTrans.transpose();
        GFX->setVertexShaderConstF( VC_CUBE_TRANS, (float*)&cubeTrans, 3 );
    }
    xform.transpose();
    GFX->setVertexShaderConstF( VC_OBJ_TRANS, (float*)&xform, 4 );
}

void ProcessedShaderMaterial::setWorldXForm(MatrixF xform)
{
    GFX->setVertexShaderConstF( 0, (float*)&xform, 4 );
}

void ProcessedShaderMaterial::setLightingBlendFunc()
{
    getCurrentClientSceneGraph()->getLightManager()->setLightingBlendFunc();
}

void ProcessedShaderMaterial::setLightInfo(const SceneGraphData& sgData, U32 pass)
{
    getCurrentClientSceneGraph()->getLightManager()->setLightInfo(this, mMaterial, sgData, pass);
}

void ProcessedShaderMaterial::setEyePosition(MatrixF objTrans, Point3F position, U32 pass)
{
    // Set cubemap stuff here (it's convenient!)
    if(hasCubemap(pass) || mMaterial->dynamicCubemap)
    {
        Point3F cubeEyePos = position - objTrans.getPosition();
        GFX->setVertexShaderConstF( VC_CUBE_EYE_POS, (float*)&cubeEyePos, 1 );
    }
    objTrans.inverse();
    objTrans.mulP( position );
    position.convolveInverse(objTrans.getScale());
    GFX->setVertexShaderConstF( VC_EYE_POS, (float*)&position, 1 );
}

void ProcessedShaderMaterial::init(SceneGraphData& sgData, GFXVertexFlags vertFlags)
{
    // Load our textures
    setStageData();

    // Determine how many stages we use
    mMaxStages = getNumStages();
    mVertFlags = vertFlags;
    for( U32 i=0; i<mMaxStages; i++ )
    {
        GFXShaderFeatureData fd;
        // Determine the features of this stage
        determineFeatures( i, fd, sgData );

        if( fd.codify() )
        {
            // Create the passes for this stage
            createPasses( fd, i, sgData );
        }
    }
}

void ProcessedShaderMaterial::setStageData()
{
    // Only do this once
    if( mHasSetStageData ) return;
    mHasSetStageData = true;

    U32 i;

    // Load up all the textures for every possible stage
    for( i=0; i<Material::MAX_STAGES; i++ )
    {
        if( mMaterial->baseTexFilename[i] && mMaterial->baseTexFilename[i][0] )
        {
            mStages[i].tex[BaseTex] = createTexture( mMaterial->baseTexFilename[i], &GFXDefaultStaticDiffuseProfile );
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

    // If we have a cubemap put it on stage 0 (cubemaps only supported on stage 0)
    if( mMaterial->mCubemapData )
    {
        mMaterial->mCubemapData->createMap();
        mStages[0].cubemap = mMaterial->mCubemapData->cubemap;
    }
}

void ProcessedShaderMaterial::addPass( RenderPassData &rpd,
                                       U32 &texIndex,
                                       GFXShaderFeatureData &fd,
                                       U32 stageNum,
                                       SceneGraphData& sgData )
{
    // Set number of textures, stage, glow, etc.
    rpd.numTex = texIndex;
    rpd.fData.useLightDir = sgData.useLightDir;
    rpd.stageNum = stageNum;
    rpd.glow |= mMaterial->glow[stageNum];

    // Copy over features
    dMemcpy( rpd.fData.materialFeatures, fd.features, sizeof( fd.features ) );
    
    // Generate shader
   //  Con::printf("Current material %s", this->mMaterial->getName());
    rpd.shader = GFX->createShader( rpd.fData, mVertFlags );

    // If a pass glows, we glow
    if( rpd.glow )
    {
        mHasGlow = true;
    }

    mPasses.push_back( rpd );

    rpd.reset();
    texIndex = 0;
}

void ProcessedShaderMaterial::setPassBlendOp( ShaderFeature *sf,
                                              RenderPassData &passData,
                                              U32 &texIndex,
                                              GFXShaderFeatureData &stageFeatures,
                                              U32 stageNum,
                                              SceneGraphData& sgData )
{
    if( sf->getBlendOp() == Material::None )
    {
        return;
    }

    // set up the current blend operation for multi-pass materials
    if( mPasses.size() > 0 )
    {
        // If passData.numTexReg is 0, this is a brand new pass, so set the
        // blend operation to the first feature.
        if( passData.numTexReg == 0 )
        {
            passData.blendOp = sf->getBlendOp();
        }
        else
        {
            // numTegReg is more than zero, if this feature
            // doesn't have the same blend operation, then
            // we need to create yet another pass
            if( sf->getBlendOp() != passData.blendOp )
            {
                addPass( passData, texIndex, stageFeatures, stageNum, sgData );
                passData.blendOp = sf->getBlendOp();
            }
        }
    }
}
