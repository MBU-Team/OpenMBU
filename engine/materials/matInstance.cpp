//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "matInstance.h"
#include "materials/customMaterial.h"
#include "gfx/gfxDevice.h"
#include "../../example/shaders/shdrConsts.h"
#include "shaderGen/featureMgr.h"
#include "gfx/cubemapData.h"
#include "gfx/gfxCubemap.h"

//****************************************************************************
// Material Instance
//****************************************************************************
MatInstance::MatInstance(Material& mat)
{
    mMaterial = &mat;
    mCurPass = 0;
    mProcessedMaterial = false;
    mVertFlags = (GFXVertexFlags)NULL;
    mMaxStages = 1;

    mHasGlow = false;
}

MatInstance::MatInstance(char* matName)
{
    // Get the material
    Material* foundMat;

    if (!Sim::findObject(matName, foundMat))
    {
        Con::errorf("MatInstance::MatInstance() - Unable to find material '%s'", matName);
        foundMat = NULL;
    }

    mMaterial = foundMat;
    mCurPass = 0;
    mProcessedMaterial = false;
    mVertFlags = (GFXVertexFlags)NULL;
    mMaxStages = 1;

}

//----------------------------------------------------------------------------
// Determine number of active stages in material
//----------------------------------------------------------------------------
U32 MatInstance::getNumStages()
{
    U32 numStages = 0;

    U32 i;
    for (i = 0; i < Material::MAX_STAGES; i++)
    {
        bool stageActive = false;

        if (i == 0)
        {
            if (mMaterial->mCubemapData || mMaterial->dynamicCubemap)
            {
                numStages++;
                continue;
            }
        }

        for (U32 j = 0; j < GFXShaderFeatureData::NumFeatures; j++)
        {
            if (mMaterial->stages[i].tex[j])
            {
                stageActive = true;
                break;
            }
        }

        if (mMaterial->pixelSpecular[i] ||
            mMaterial->vertexSpecular[i])
        {
            stageActive = true;
        }

        numStages += stageActive;
    }


    return numStages;
}

//----------------------------------------------------------------------------
// Determine features
//----------------------------------------------------------------------------
void MatInstance::determineFeatures(U32 stageNum, GFXShaderFeatureData& fd)
{
    for (U32 i = 0; i < GFXShaderFeatureData::NumFeatures; i++)
    {
        if (GFX->getPixelShaderVersion() == 0.0)
        {
            if (i == GFXShaderFeatureData::BumpMap ||
                i == GFXShaderFeatureData::LightNormMap ||
                i == GFXShaderFeatureData::CubeMap ||
                i == GFXShaderFeatureData::PixSpecular ||
                i == GFXShaderFeatureData::VertSpecular)
            {
                continue;
            }
        }

        // texture coordinate animation
        if (i == GFXShaderFeatureData::TexAnim)
        {
            if (mMaterial->animFlags[stageNum])
            {
                fd.features[i] = true;
            }
        }

        // vertex shading
        if (i == GFXShaderFeatureData::RTLighting)
        {
            if (mSGData.useLightDir &&
                !mMaterial->emissive[stageNum])
            {
                fd.features[i] = true;
            }
        }

        // pixel specular
        if (GFX->getPixelShaderVersion() >= 2.0)
        {
            if (i == GFXShaderFeatureData::PixSpecular)
            {
                if (mMaterial->pixelSpecular[stageNum])
                {
                    fd.features[i] = true;
                }
            }
        }

        // vertex specular
        if (i == GFXShaderFeatureData::VertSpecular)
        {
            if (mMaterial->vertexSpecular[stageNum])
            {
                fd.features[i] = true;
            }
        }


        // lightmap
        if (i == GFXShaderFeatureData::LightMap &&
            !mMaterial->emissive[stageNum] &&
            stageNum == (mMaxStages - 1))
        {
            if (mSGData.lightmap)
            {
                fd.features[i] = true;
            }
        }

        // lightNormMap
        if (i == GFXShaderFeatureData::LightNormMap &&
            !mMaterial->emissive[stageNum] &&
            stageNum == (mMaxStages - 1))
        {
            if (mSGData.normLightmap)
            {
                fd.features[i] = true;
            }
        }

        // cubemap
        if (i == GFXShaderFeatureData::CubeMap &&
            stageNum < 1 &&      // cubemaps only available on stage 0 for now - bramage
            ((mMaterial->mCubemapData && mMaterial->mCubemapData->cubemap) || mMaterial->dynamicCubemap))
        {
            fd.features[i] = true;
        }

        // fog - last stage only
        if (i == GFXShaderFeatureData::Fog &&
            stageNum == (mMaxStages - 1) &&
            mSGData.useFog)
        {
            fd.features[i] = true;
        }

        // textures
        if (mMaterial->stages[stageNum].tex[i])
        {
            fd.features[i] = true;
        }
    }
}

//----------------------------------------------------------------------------
// Temp function
//----------------------------------------------------------------------------
void MatInstance::init(SceneGraphData& dat, GFXVertexFlags vertFlags)
{
    mSGData = dat;
    mVertFlags = vertFlags;

    mMaterial->setStageData();


    if (!mProcessedMaterial)
    {
        processMaterial();
        mProcessedMaterial = true;
    }

}

//----------------------------------------------------------------------------
// Add pass
//----------------------------------------------------------------------------
void MatInstance::addPass(RenderPassData& rpd,
    U32& texIndex,
    GFXShaderFeatureData& fd,
    U32 stageNum)
{
    rpd.numTex = texIndex;
    rpd.fData.useLightDir = mSGData.useLightDir;
    rpd.stageNum = stageNum;
    rpd.glow |= mMaterial->glow[stageNum];

    if (GFX->getPixelShaderVersion() > 0.0)
    {
        dMemcpy(rpd.fData.materialFeatures, fd.features, sizeof(fd.features));
        rpd.shader = GFX->createShader(rpd.fData, mVertFlags);
    }

    if (rpd.glow)
    {
        mHasGlow = true;
    }

    mPasses.push_back(rpd);

    rpd.reset();
    texIndex = 0;
}

//----------------------------------------------------------------------------
// Set blend operation for pass
//----------------------------------------------------------------------------
void MatInstance::setPassBlendOp(ShaderFeature* sf,
    RenderPassData& passData,
    U32& texIndex,
    GFXShaderFeatureData& stageFeatures,
    U32 stageNum)
{
    if (sf->getBlendOp() == Material::BlendOp::None)
    {
        return;
    }

    // set up the current blend operation for multi-pass materials
    if (mPasses.size() > 0)
    {
        // If passData.numTexReg is 0, this is a brand new pass, so set the
        // blend operation to the first feature.
        if (passData.numTexReg == 0)
        {
            passData.blendOp = sf->getBlendOp();
        }
        else
        {
            // numTegReg is more than zero, if this feature
            // doesn't have the same blend operation, then
            // we need to create yet another pass
            if (sf->getBlendOp() != passData.blendOp)
            {
                addPass(passData, texIndex, stageFeatures, stageNum);
                passData.blendOp = sf->getBlendOp();
            }
        }
    }

}

//----------------------------------------------------------------------------
// Create passes
//----------------------------------------------------------------------------
void MatInstance::createPasses(GFXShaderFeatureData& stageFeatures, U32 stageNum)
{
    RenderPassData passData;
    U32 texIndex = 0;

    for (U32 i = 0; i < GFXShaderFeatureData::NumFeatures; i++)
    {
        if (!stageFeatures.features[i]) continue;

        ShaderFeature* sf = gFeatureMgr.get(i);
        U32 numTexReg = sf->getResources(passData.fData).numTexReg;

        // adds pass if blend op changes for feature
        setPassBlendOp(sf, passData, texIndex, stageFeatures, stageNum);

        // Add pass if num tex reg is going to be too high
        if (passData.numTexReg + numTexReg > GFX->getNumSamplers())
        {
            addPass(passData, texIndex, stageFeatures, stageNum);
            setPassBlendOp(sf, passData, texIndex, stageFeatures, stageNum);
        }

        passData.numTexReg += numTexReg;
        passData.fData.features[i] = true;
        sf->setTexData(mMaterial->stages[stageNum], stageFeatures, passData, texIndex);

        // Add pass if tex units are maxed out
        if (texIndex >= GFX->getNumSamplers())
        {
            addPass(passData, texIndex, stageFeatures, stageNum);
            setPassBlendOp(sf, passData, texIndex, stageFeatures, stageNum);
        }
    }

    if (passData.fData.codify())
    {
        addPass(passData, texIndex, stageFeatures, stageNum);
    }
}


//----------------------------------------------------------------------------
// Process stages
//----------------------------------------------------------------------------
void MatInstance::processMaterial()
{
    if (dynamic_cast<CustomMaterial*>(mMaterial)) return;

    mMaxStages = getNumStages();

    for (U32 i = 0; i < mMaxStages; i++)
    {
        GFXShaderFeatureData fd;
        determineFeatures(i, fd);

        if (fd.codify())
        {
            createPasses(fd, i);
        }
    }
}

//----------------------------------------------------------------------------
// Set the textures for each stage used.
//----------------------------------------------------------------------------
void MatInstance::setTextureStages(SceneGraphData& sgData)
{
    for (U32 i = 0; i < mPasses[mCurPass].numTex; i++)
    {
        GFX->setTextureStageColorOp(i, GFXTOPModulate);

        switch (mPasses[mCurPass].texFlags[i])
        {
            case 0:
                GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
                GFX->setTextureStageAddressModeV(i, GFXAddressWrap);
                GFX->setTexture(i, mPasses[mCurPass].tex[i]);
                break;

            case Material::Lightmap:
                GFX->setTexture(i, sgData.lightmap);
                break;

            case Material::NormLightmap:
                GFX->setTexture(i, sgData.normLightmap);
                break;

            case Material::Cube:
                GFX->setCubeTexture(i, mPasses[mCurPass].cubeMap);
                break;

            case Material::SGCube:
                GFX->setCubeTexture(i, sgData.cubemap);
                break;

            case Material::Fog:
            {
                GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
                GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
                GFX->setTexture(i, sgData.fogTex);
                break;
            }

            case Material::BackBuff:
                GFX->setTexture(i, sgData.backBuffTex);
                break;
        }
    }
}

//----------------------------------------------------------------------------
// Filter glow passes - this function filters out passes that do not glow.
//    Returns true if there are no more glow passes, false otherwise.
//----------------------------------------------------------------------------
bool MatInstance::filterGlowPasses(SceneGraphData& sgData)
{
    // if rendering to glow buffer, render only glow passes
    if (sgData.glowPass)
    {
        while (!mPasses[mCurPass].glow)
        {
            mCurPass++;
            if (mCurPass >= mPasses.size())
            {
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------
// Setup pass - needs scenegraph data because the lightmap will change across
//    several materials.
//----------------------------------------------------------------------------
bool MatInstance::setupPass(SceneGraphData& sgData)
{
    CustomMaterial* custMat = dynamic_cast<CustomMaterial*>(mMaterial);

    if (custMat)
    {
        // This setTexTrans nonsense is set to make sure that setTextureTransforms()
        // is called only once per material draw, not per pass
        static bool setTexTrans = false;
        if (!setTexTrans)
        {
            setTextureTransforms();
            setTexTrans = true;
        }

        if (custMat->setupPass(sgData))
        {
            return true;
        }
        else
        {
            setTexTrans = false;
            return false;
        }
    }

    // return when done with all passes
    if (mCurPass >= mPasses.size() ||
        filterGlowPasses(sgData) ||
        sgData.refractPass)  // only custom materials support refraction for now
    {
        cleanup();
        return false;
    }

    // set alpha blend
    if (mCurPass > 0)
    {
        GFX->setAlphaBlendEnable(true);
        mMaterial->setBlendState(mPasses[mCurPass].blendOp);
    }
    else
    {
        GFX->setAlphaBlendEnable(false);
    }

    if (mMaterial->translucent)
    {
        GFX->setAlphaBlendEnable(true);
        mMaterial->setBlendState(mMaterial->translucentBlendOp);
        GFX->setZWriteEnable(false);
        GFX->setAlphaTestEnable(true);
        GFX->setAlphaRef(1);
        GFX->setAlphaFunc(GFXCmpGreaterEqual);

        // set up register combiners
        GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
        GFX->setTextureStageAlphaOp(1, GFXTOPDisable);
        GFX->setTextureStageAlphaArg1(0, GFXTATexture);
        GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);
    }

    // set shaders
    if (mPasses[mCurPass].shader)
    {
        mPasses[mCurPass].shader->process();
        mMaterial->setShaderConstants(sgData, mPasses[mCurPass].stageNum);
    }
    else
    {
        GFX->disableShaders();
        GFX->setTextureStageColorOp(mPasses[mCurPass].numTex, GFXTOPDisable);
    }

    setTextureStages(sgData);

    if (mCurPass == 0)
    {
        setTextureTransforms();
    }

    mCurPass++;

    return true;
}

//--------------------------------------------------------------------------
// Clean up any strange render states set
//--------------------------------------------------------------------------
void MatInstance::cleanup()
{
    mCurPass = 0;

    if (mPasses.size())
    {
        // DX warning if dynamic cubemap not explicitly unbound
        for (U32 i = 0; i < mPasses[mCurPass].numTex; i++)
        {
            if (mPasses[mCurPass].texFlags[i] == Material::BackBuff)
            {
                GFX->setTexture(i, NULL);
                continue;
            }
            if (mPasses[mCurPass].texFlags[i] == Material::SGCube)
            {
                GFX->setTexture(i, NULL);
                continue;
            }
        }
    }

    GFX->setAlphaBlendEnable(false);
    GFX->setAlphaTestEnable(false);
    GFX->setZWriteEnable(true);


}

//--------------------------------------------------------------------------
// Setup texture matrix for texture coordinate animation
//--------------------------------------------------------------------------
void MatInstance::setTextureTransforms()
{

    U32 i = 0;

    MatrixF texMat(true);

    F32 waveOffset = getWaveOffset(i); // offset is between 0.0 and 1.0


    // handle scroll anim type
    if (mMaterial->animFlags[i] & Material::AnimType::Scroll)
    {
        if (mMaterial->animFlags[i] & Material::AnimType::Wave)
        {
            Point3F scrollOffset;
            scrollOffset.x = mMaterial->scrollDir[i].x * waveOffset;
            scrollOffset.y = mMaterial->scrollDir[i].y * waveOffset;
            scrollOffset.z = 1.0;

            texMat.setColumn(3, scrollOffset);
        }
        else
        {
            mMaterial->scrollOffset[i] += mMaterial->scrollDir[i] *
                mMaterial->scrollSpeed[i] *
                mMaterial->mDt;

            Point3F offset(mMaterial->scrollOffset[i].x,
                mMaterial->scrollOffset[i].y,
                1.0);

            texMat.setColumn(3, offset);
        }

    }

    // handle rotation
    if (mMaterial->animFlags[i] & Material::Rotate)
    {
        if (mMaterial->animFlags[i] & Material::AnimType::Wave)
        {
            F32 rotPos = waveOffset * M_2PI;
            texMat.set(EulerF(0.0, 0.0, rotPos));
            texMat.setColumn(3, Point3F(0.5, 0.5, 0.0));

            MatrixF test(true);
            test.setColumn(3, Point3F(mMaterial->rotPivotOffset[i].x,
                mMaterial->rotPivotOffset[i].y,
                0.0));
            texMat.mul(test);
        }
        else
        {
            mMaterial->rotPos[i] += mMaterial->rotSpeed[i] * mMaterial->mDt;
            texMat.set(EulerF(0.0, 0.0, mMaterial->rotPos[i]));

            texMat.setColumn(3, Point3F(0.5, 0.5, 0.0));

            MatrixF test(true);
            test.setColumn(3, Point3F(mMaterial->rotPivotOffset[i].x,
                mMaterial->rotPivotOffset[i].y,
                0.0));
            texMat.mul(test);
        }
    }

    // Handle scale + wave offset
    if (mMaterial->animFlags[i] & Material::AnimType::Scale &&
        mMaterial->animFlags[i] & Material::AnimType::Wave)
    {
        F32 wOffset = fabs(waveOffset);

        texMat.setColumn(3, Point3F(0.5, 0.5, 0.0));

        MatrixF temp(true);
        temp.setRow(0, Point3F(wOffset, 0.0, 0.0));
        temp.setRow(1, Point3F(0.0, wOffset, 0.0));
        temp.setRow(2, Point3F(0.0, 0.0, wOffset));
        temp.setColumn(3, Point3F(-wOffset * 0.5, -wOffset * 0.5, 0.0));
        texMat.mul(temp);
    }

    // handle sequence
    if (mMaterial->animFlags[i] & Material::AnimType::Sequence)
    {
        static F32 accumTime = 0.0;
        accumTime += mMaterial->mDt;

        U32 frameNum = accumTime * mMaterial->seqFramePerSec[i];
        F32 offset = frameNum * mMaterial->seqSegSize[i];

        Point3F texOffset = texMat.getPosition();
        texOffset.x += offset;
        texMat.setPosition(texOffset);
    }

    texMat.transpose();
    GFX->setVertexShaderConstF(VC_TEX_TRANS1, (float*)&texMat, 4);

}

//--------------------------------------------------------------------------
// Get wave offset for texture animations using a wave transform
//--------------------------------------------------------------------------
F32 MatInstance::getWaveOffset(U32 stage)
{
    mMaterial->wavePos[stage] += mMaterial->waveFreq[stage] * mMaterial->mDt;

    switch (mMaterial->waveType[stage])
    {
        case Material::WaveType::Sin:
        {
            return mMaterial->waveAmp[stage] * mSin(M_2PI * mMaterial->wavePos[stage]);
            break;
        }

        case Material::WaveType::Triangle:
        {
            F32 frac = mMaterial->wavePos[stage] - mFloor(mMaterial->wavePos[stage]);
            if (frac > 0.0 && frac <= 0.25)
            {
                return mMaterial->waveAmp[stage] * frac * 4.0;
            }

            if (frac > 0.25 && frac <= 0.5)
            {
                return mMaterial->waveAmp[stage] * (1.0 - ((frac - 0.25) * 4.0));
            }

            if (frac > 0.5 && frac <= 0.75)
            {
                return mMaterial->waveAmp[stage] * (frac - 0.5) * -4.0;
            }

            if (frac > 0.75 && frac <= 1.0)
            {
                return -mMaterial->waveAmp[stage] * (1.0 - ((frac - 0.75) * 4.0));
            }

            break;
        }

        case Material::WaveType::Square:
        {
            F32 frac = mMaterial->wavePos[stage] - mFloor(mMaterial->wavePos[stage]);
            if (frac > 0.0 && frac <= 0.5)
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

//------------------------------------------------------------------------------
// has cubemap data - used to determine whether to pass cubemap shader constants
// to GPU
//------------------------------------------------------------------------------
bool MatInstance::hasCubemap()
{
    if (mMaterial->getType() != Material::base)
    {
        if (mMaterial->mCubemapData) return true;
        else return false;
    }

    // cubemaps are currently only on stage 0
    //if (getCurStageNum() > 0)
    //{
    //    return false;
    //}

    if (mPasses[mCurPass].cubeMap)
    {
        return true;
    }

    return false;
}
