//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "customMaterial.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "gfx/gfxDevice.h"
#include "materials/shaderData.h"
#include "gfx/cubemapData.h"
#include "gfx/gfxCubemap.h"
#include "../../game/shaders/shdrConsts.h"
#include "materialPropertyMap.h"


//****************************************************************************
// Custom Material
//****************************************************************************
IMPLEMENT_CONOBJECT(CustomMaterial);

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
CustomMaterial::CustomMaterial()
{
    dMemset(texFilename, 0, sizeof(texFilename));
    fallback = NULL;
    mType = custom;
    dMemset(pass, 0, sizeof(pass));

    dynamicLightingMaterial = NULL;

    mMaxTex = 0;
    mVersion = 1.1;
    translucent = false;
    dMemset(mFlags, 0, sizeof(mFlags));
    mCurPass = -1;
    blendOp = Material::Add;
    mShaderData = NULL;
    mShaderDataName = NULL;
    refract = false;
    mCullMode = -1;
}



//--------------------------------------------------------------------------
// Init fields
//--------------------------------------------------------------------------
void CustomMaterial::initPersistFields()
{
    Parent::initPersistFields();

    addField("texture", TypeFilename, Offset(texFilename, CustomMaterial), MAX_TEX_PER_PASS);

    addField("version", TypeF32, Offset(mVersion, CustomMaterial));

    addField("fallback", TypeSimObjectPtr, Offset(fallback, CustomMaterial));
    addField("pass", TypeSimObjectPtr, Offset(pass, CustomMaterial), MAX_PASSES);

    addField("dynamicLightingMaterial", TypeSimObjectPtr, Offset(dynamicLightingMaterial, CustomMaterial));

    addField("shader", TypeString, Offset(mShaderDataName, CustomMaterial));
    addField("blendOp", TypeEnum, Offset(blendOp, CustomMaterial), 1, &mBlendOpTable);

    addField("refract", TypeBool, Offset(refract, CustomMaterial));

}

//--------------------------------------------------------------------------
// On add - verify data settings
//--------------------------------------------------------------------------
bool CustomMaterial::onAdd()
{
    if (Parent::onAdd() == false)
        return false;

    mShaderData = static_cast<ShaderData*>(Sim::findObject(mShaderDataName));

    // TEMP: Disable this until we can figure out the z-fighting and brightness issues with this.
    // Allow translucent objects to be seen behind each other
    /*for (S32 i = 0; i < MAX_PASSES; i++)
    {
        CustomMaterial* mat = pass[i];
        if (mat && mat->translucent)
            subPassTranslucent = true;
    }*/

    return true;
}

//--------------------------------------------------------------------------
// On remove
//--------------------------------------------------------------------------
void CustomMaterial::onRemove()
{
    Parent::onRemove();
}

//--------------------------------------------------------------------------
// Set stage data
//--------------------------------------------------------------------------
void CustomMaterial::setStageData()
{
    if (hasSetStageData) return;
    hasSetStageData = true;

    S32 i;

    for (i = 0; i < MAX_TEX_PER_PASS; i++)
    {
        if (!texFilename[i]) continue;

        if (!dStrcmp(texFilename[i], "$dynamiclight"))
        {
            mFlags[i] = DynamicLight;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$lightmap"))
        {
            mFlags[i] = Lightmap;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$normmap"))
        {
            mFlags[i] = NormLightmap;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$fog"))
        {
            mFlags[i] = Fog;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$cubemap"))
        {
            if (mCubemapData)
            {
                mFlags[i] = Cube;
                mMaxTex = i + 1;
            }
            else
            {
                Con::warnf("Invalid cubemap data for CustomMaterial - %s : %s", getName(), cubemapName);
            }
            continue;
        }

        if (!dStrcmp(texFilename[i], "$dynamicCubemap"))
        {
            mFlags[i] = SGCube;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$backbuff"))
        {
            mFlags[i] = BackBuff;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$reflectbuff"))
        {
            mFlags[i] = ReflectBuff;
            mMaxTex = i + 1;
            continue;
        }

        if (!dStrcmp(texFilename[i], "$miscbuff"))
        {
            mFlags[i] = Misc;
            mMaxTex = i + 1;
            continue;
        }

        if (texFilename[i] && texFilename[i][0])
        {
            tex[i] = createTexture(texFilename[i], &GFXDefaultStaticDiffuseProfile);
            mFlags[i] = Standard;
            mMaxTex = i + 1;
        }
    }

    if (mCubemapData)
    {
        mCubemapData->createMap();
    }

    if (mShaderData && !mShaderData->shader)
    {
        mShaderData->initShader();
    }
}


//--------------------------------------------------------------------------
// Clean up any strange render states set
//--------------------------------------------------------------------------
void CustomMaterial::cleanup()
{
    for (U32 i = 0; i < mMaxTex; i++)
    {
        // set up textures
        switch (mFlags[i])
        {
        case 0:
        default:
            break;

        case BackBuff:
        {
            // have to unbind render targets or D3D complains
            GFX->setTexture(i, NULL);
            break;
        }
        case ReflectBuff:
        {
            // have to unbind render targets or D3D complains
            GFX->setTexture(i, NULL);
            break;
        }

        }
    }

    if (mCullMode != -1)
    {
        GFX->setCullMode((GFXCullMode)mCullMode);
        mCullMode = -1;
    }

    GFX->setAlphaBlendEnable(false);
    GFX->setAlphaTestEnable(false);
    GFX->setZWriteEnable(true);

    mCurPass = -1;
}

//--------------------------------------------------------------------------
// Falls back to material that is appropriate for current hardware
//--------------------------------------------------------------------------
bool CustomMaterial::setFallbackVersion(SceneGraphData& sgData)
{
    if (fallback)
    {
        return fallback->setupPass(sgData);
    }
    else
    {
        return false;  // don't render anything if no fallback

        // this code used to work as temp/hack fixed function
        // fallback - here for reference
  /*
        GFX->disableShaders();
        GFX->setTextureStageColorOp( 0, GFXTOPDisable );

        if( mCurPass > 0 )
        {
           cleanup();
           return false;
        }

        return true;
  */
    }



}

//--------------------------------------------------------------------------
// Set projection matrix for multipass
// TEMP HACK - this needs to be coordinated with scenegraph
//--------------------------------------------------------------------------
void CustomMaterial::setMultipassProjection()
{
    /*
       // Note - this only works if the construction function is:
       //        D3DXMatrixPerspectiveFovRH().  D3DXMatrixPerspectiveRH() does
       //        NOT work.  See D3D docs for construction details

       MatrixF f;
       D3DXMatrixPerspectiveFovRH( (D3DXMATRIX*)&f, 67.5 * M_PI/180.0, 1.3333333333, 0.101, 500.0 );
       D3D->SetTransform( D3DTS_PROJECTION, (D3DMATRIX*)&f );

       MatrixF rotMat(EulerF( (M_PI / 2.0), 0.0, 0.0));
       rotMat.transpose();
       D3D->MultiplyTransform( D3DTS_PROJECTION, (D3DMATRIX*) &rotMat );
    */
}


//--------------------------------------------------------------------------
// Setup texture stages
//--------------------------------------------------------------------------
void CustomMaterial::setupStages(SceneGraphData& sgData)
{
    for (U32 i = 0; i < mMaxTex; i++)
    {
        GFX->setTextureStageColorOp(i, GFXTOPModulate);

        // set up textures
        switch (mFlags[i])
        {
        case 0:
        default:
            break;

        case Mask:
        case Standard:
        case Bump:
        case Detail:
        {
            GFX->setTextureStageAddressModeU(i, GFXAddressWrap);
            GFX->setTextureStageAddressModeV(i, GFXAddressWrap);
            GFX->setTexture(i, tex[i]);
            break;
        }
        case Material::DynamicLight:
        {
            //GFX->setTextureBorderColor(i, ColorI(0, 0, 0, 0));
            GFX->setTextureStageAddressModeU(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeV(i, GFXAddressClamp);
            GFX->setTextureStageAddressModeW(i, GFXAddressClamp);
            GFX->setTexture(i, sgData.dynamicLight);
            break;
        }
        case Material::DynamicLightSecondary:
        {
            AssertFatal((false), "Custom materials cannot use secondary light textures - how did this happen?");
            break;
        }
        case Lightmap:
        {
            GFX->setTexture(i, sgData.lightmap);
            break;
        }
        case NormLightmap:
        {
            GFX->setTexture(i, sgData.normLightmap);
            break;
        }
        case Fog:
        {
            GFX->setTexture(i, sgData.fogTex);
            break;
        }
        case Cube:
        {
            GFX->setCubeTexture(i, mCubemapData->cubemap);
            break;
        }
        case SGCube:
        {
            GFX->setCubeTexture(i, sgData.cubemap);
            break;
        }
        case BackBuff:
        {
            GFX->setTexture(i, sgData.backBuffTex);
            break;
        }
        case ReflectBuff:
        {
            GFX->setTexture(i, sgData.reflectTex);
            break;
        }
        case Misc:
        {
            GFX->setTexture(i, sgData.miscTex);
            break;
        }

        }
    }
}


//--------------------------------------------------------------------------
// Sets up render states for a specific pass
//--------------------------------------------------------------------------
void CustomMaterial::setupSubPass(SceneGraphData& sgData)
{
    setBlendState(blendOp);

    // activate shader
    if (mShaderData && mShaderData->shader)
    {
        mShaderData->shader->process();
    }
    else
    {
        GFX->disableShaders();
        GFX->setTextureStageColorOp(mMaxTex, GFXTOPDisable);
    }

    setupStages(sgData);
}


//--------------------------------------------------------------------------
// Sets next pass either containing refraction or not depending on if
// refraction is on.
// Returns false if pass is not found.
//--------------------------------------------------------------------------
bool CustomMaterial::setNextRefractPass(bool refractOn)
{
    if (mCurPass == 0)
    {
        if ((refractOn && refract) ||
            (!refractOn && !refract))
        {
            return true;
        }
    }

    U32 i = 0;
    while (pass[i])
    {
        if ((refractOn && pass[i]->refract) ||
            (!refractOn && !pass[i]->refract))
        {
            mCurPass = i + 1;
            return true;
        }
        i++;
    }

    return false;
}


//--------------------------------------------------------------------------
// Setup pass -
// Sets up next rendering pass.  Will increment through passes until it
// runs out.  Returns false if interated through all passes.
//--------------------------------------------------------------------------
bool CustomMaterial::setupPass(SceneGraphData& sgData)
{
    // custom materials don't glow for the moment...
    if (sgData.glowPass)
    {
        return false;
    }

    ++mCurPass;

    if (mVersion > GFX->getPixelShaderVersion())
    {
        return setFallbackVersion(sgData);
    }

    if (mCurPass > 0 && !pass[mCurPass - 1])
    {
        // reset on last pass
        cleanup();
        return false;
    }

    if (!setNextRefractPass(sgData.refractPass))
    {
        cleanup();
        return false;
    }

    if (mCurPass > 0)
    {
        if (pass[mCurPass - 1])
        {
            GFX->setAlphaBlendEnable(true);
            setMultipassProjection();

            pass[mCurPass - 1]->setupSubPass(sgData);
            mCurPass++;
            return true;
        }
    }
    else
    {
        // do this only on the first pass?
        setShaderConstants(sgData, 0);

        if (doubleSided)
        {
            GFX->setCullMode(GFXCullNone);
            mCullMode = GFX->getCullMode();
        }

    }


    if (isTranslucent())
    {
        GFX->setAlphaBlendEnable(translucentBlendOp != Material::None );
        setBlendState(translucentBlendOp);
        GFX->setZWriteEnable(translucentZWrite);
        GFX->setAlphaTestEnable(alphaTest);
        GFX->setAlphaRef(alphaRef);
        GFX->setAlphaFunc(GFXCmpGreaterEqual);

        // set up register combiners
        GFX->setTextureStageAlphaOp(0, GFXTOPModulate);
        GFX->setTextureStageAlphaOp(1, GFXTOPDisable);
        GFX->setTextureStageAlphaArg1(0, GFXTATexture);
        GFX->setTextureStageAlphaArg2(0, GFXTADiffuse);
    }
    else
    {
        GFX->setAlphaBlendEnable(false);
    }

    setupSubPass(sgData);

    /*if (!setNextRefractPass(sgData.refractPass))
    {
        cleanup();
        return false;
    }*/

    return true;
}

//--------------------------------------------------------------------------
// Setup texture matrix
/*
Direct3D uses a subset of the 4x4
transformation matrix.  Unfortunately, this makes programming harder
and provides no additional functionality.

Exactly which elements are used depend on the value of TSS Texture
Transform Flags and the dimensionality of the input coordinates.
Presumably you have 2d texture coordinates in your vertices and by
saying D3DTTFF_COUNT2, you're telling D3D you want 2D coordinates out.
So that means you're using the upper-left 3x3 elements of the matrix as
a homogeneous matrix for 2D coordinates.

You need to stick the translation elements into the proper portion of
the upper-left 3x3.  IMO, the easiest way to deal with the layout
weirdness of the texture matrices is to use a class that does the
shuffling for you when you construct it with another 4x4 matrix.

Rich - some M$ dude - see http://discuss.microsoft.com/archives/DIRECTXDEV.html

*/
//--------------------------------------------------------------------------
MatrixF CustomMaterial::setupTexAnim(AnimType animType, Point2F& scrollDir,
    F32 scrollSpeed, U32 texUnit)
{
    MatrixF mat(true);

    switch (animType)
    {
    case Scroll:
    {
        scrollOffset[texUnit] += scrollDir * scrollSpeed * mDt;
        Point3F offset(scrollOffset[texUnit].x, scrollOffset[texUnit].y, 1.0);
        mat.setColumn(3, offset);
        break;
    }

    default:
        break;
    }

    return mat;
}

//--------------------------------------------------------------------------
// Set vertex and pixel shader constants
//--------------------------------------------------------------------------
void CustomMaterial::setShaderConstants(const SceneGraphData& sgData,
    U32 stageNum)
{
    Parent::setShaderConstants(sgData, stageNum);

    GFX->setPixelShaderConstF(PC_ACCUM_TIME, (float*)&mAccumTime, 1);

    //Point4F exp(exposure[stageNum], 0, 0, 0);
    //GFX->setPixelShaderConstF(PC_EXPOSURE, (float*)&exp, 1);
}

//--------------------------------------------------------------------------
// Map this material to the texture specified in the "mapTo" data variable
//--------------------------------------------------------------------------
void CustomMaterial::mapMaterial()
{
    if (!getName())
    {
        Con::warnf("Unnamed Material!  Could not map to: %s", mapTo);
        return;
    }

    if (!mapTo)
    {
        return;
    }

    MaterialPropertyMap* pMap = MaterialPropertyMap::get();
    if (pMap == NULL)
    {
        Con::errorf(ConsoleLogEntry::General, "Error, cannot find the global material map object");
    }
    else
    {
        char matName[128] = { "material: " };
        char* namePtrs[2];

        U32 strLen = dStrlen(matName);
        dStrcpy(&matName[strLen], getName());
        namePtrs[0] = (char*)mapTo;
        namePtrs[1] = matName;
        pMap->addMapping(2, (const char**)namePtrs);
    }

}

//--------------------------------------------------------------------------
// Set lightmaps
//--------------------------------------------------------------------------
void CustomMaterial::setLightmaps(SceneGraphData& sgData)
{
    // recurse into correct fallback
    if (mVersion > GFX->getPixelShaderVersion())
    {
        if (fallback)
        {
            fallback->setLightmaps(sgData);
        }
    }

    S32 passNum = mCurPass;
    AssertFatal(passNum >= 0, "pass invalid");

    if (passNum > 0)
    {
        pass[passNum - 1]->setupStages(sgData);
    }
    else
    {
        setupStages(sgData);
    }

}