//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "bump.h"
#include "shaderOp.h"
#include "platform/platformVideo.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "../../game/shaders/shdrConsts.h"

//****************************************************************************
// Bumpmap
//****************************************************************************

//----------------------------------------------------------------------------
// Setup the light vector to be passed to the pixel shader
//----------------------------------------------------------------------------
Var* BumpFeat::setupLightVec(Vector<ShaderComponent*>& componentList,
    MultiLine* meta,
    GFXShaderFeatureData& fd)
{
    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* lightVec = connectComp->getElement(RT_TEXCOORD);
    lightVec->setName("outLightVec");
    lightVec->setStructName("OUT");
    lightVec->setType("float3");

    //if (fd.features[GFXShaderFeatureData::DynamicLight])
    //{
    //    Var* inpos = (Var*)LangElement::find("position");
    //    Var* lightpos = (Var*)LangElement::find("lightPos");
    //    Var* dlightcoord = (Var*)LangElement::find("dlightCoord");
    //    Var* N = (Var*)LangElement::find("N");

    //    // setup language elements to output incoming tex coords to output
    //    if (inpos && lightpos)
    //    {
    //        meta->addStatement(new GenOp("   @.xyz = normalize(@.xyz - @.xyz);\r\n", lightVec, lightpos, inpos));
    //        if (dlightcoord && N && (GFX->getPixelShaderVersion() >= 2.0))
    //            meta->addStatement(new GenOp("   @.w = saturate(dot(@, @) * 4.0);\r\n", dlightcoord, lightVec, N));
    //    }

    //    if (fd.features[GFXShaderFeatureData::DynamicLightDual])
    //    {
    //        Var* lightVecSec = connectComp->getElement(RT_TEXCOORD);
    //        lightVecSec->setName("outLightVecSec");
    //        lightVecSec->setStructName("OUT");
    //        lightVecSec->setType("float3");

    //        lightpos = (Var*)LangElement::find("lightPosSec");
    //        dlightcoord = (Var*)LangElement::find("dlightCoordSec");

    //        // setup language elements to output incoming tex coords to output
    //        if (inpos && lightpos)
    //        {
    //            meta->addStatement(new GenOp("   @.xyz = normalize(@.xyz - @.xyz);\r\n", lightVecSec, lightpos, inpos));
    //            if (dlightcoord && N && (GFX->getPixelShaderVersion() >= 2.0))
    //                meta->addStatement(new GenOp("   @.w = saturate(dot(@, @) * 4.0);\r\n", dlightcoord, lightVecSec, N));
    //        }
    //    }
    //}
    //else if (fd.features[GFXShaderFeatureData::SelfIllumination])
    //{
    //    Var* N = (Var*)LangElement::find("N");
    //    if (N)
    //    {
    //        meta->addStatement(new GenOp("   @.xyz = @;\r\n", lightVec, N));
    //    }
    //}
    //else
    //{
        if (fd.useLightDir)
        {
            // grab light direction var
            Var* inLightVec = new Var;
            inLightVec->setType("float3");
            inLightVec->setName("inLightVec");
            inLightVec->uniform = true;
            inLightVec->constNum = VC_LIGHT_DIR1;
            meta->addStatement(new GenOp("   @.xyz = -@;\r\n", lightVec, inLightVec));
        }
        else
        {
            // grab light position var
            Var* lightPos = new Var;
            lightPos->setType("float3");
            lightPos->setName("lightPos");
            lightPos->uniform = true;
            lightPos->constNum = VC_LIGHT_POS1;

            LangElement* position = LangElement::find("position");
            LangElement* assign = new GenOp("   @.xyz = normalize(@ - @.xyz);\r\n", lightVec, lightPos, position);
            meta->addStatement(assign);
        }
    //}

    return lightVec;
}

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void BumpFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    LangElement* texAssign = NULL;

    MultiLine* meta = new MultiLine;

    if (!fd.features[GFXShaderFeatureData::BaseTex])
    {
        // find incoming texture var
        Var* inTex = getVertBaseTex();

        // grab connector texcoord register
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        Var* outTex = connectComp->getElement(RT_TEXCOORD);
        outTex->setName("texCoord");
        outTex->setStructName("OUT");
        outTex->setType("float2");
        outTex->mapsToSampler = true;

        if (fd.features[GFXShaderFeatureData::TexAnim])
        {
            inTex->setType("float4");

            // create texture mat var
            Var* texMat = new Var;
            texMat->setType("float4x4");
            texMat->setName("texMat");
            texMat->uniform = true;
            texMat->constNum = VC_TEX_TRANS1;

            meta->addStatement(new GenOp("   @ = mul(@, @);\r\n", outTex, texMat, inTex));
        }
        else
        {
            // setup language elements to output incoming tex coords to output
            meta->addStatement(new GenOp("   @ = @;", outTex, inTex));
        }

    }
    else
    {
        // have to add bump tex coords if not ps 2.0
        if (GFX->getPixelShaderVersion() < 2.0)
        {
            LangElement* result = addBumpCoords(componentList);
            meta->addStatement(result);
        }
    }

    if (fd.features[GFXShaderFeatureData::LightNormMap])
    {
        output = meta;
        return;
    }


    //--------------------------------------------------------------
    // no light norm map present - assume a mesh - use vertex bump
    //--------------------------------------------------------------

    // setup texture space matrix
    Var* texSpaceMat = NULL;
    LangElement* texSpaceSetup = setupTexSpaceMat(componentList, &texSpaceMat);
    meta->addStatement(texSpaceSetup);

    // translate light vec into tangent space
    Var* lightVec = setupLightVec(componentList, meta, fd);
    LangElement* assign2 = new GenOp("   @.xyz = mul(@, @);\r\n", lightVec, texSpaceMat, lightVec);
    meta->addStatement(assign2);

    if (fd.features[GFXShaderFeatureData::DynamicLightDual])
    {
        Var* lightVecSec = (Var*)LangElement::find("outLightVecSec");
        if (lightVecSec)
            meta->addStatement(new GenOp("   @.xyz = mul(@, @);\r\n", lightVecSec, texSpaceMat, lightVecSec));
    }

    // shift tex coords on ps 1.1 because it clamps them from 0.0 to 1.0
    if (GFX->getPixelShaderVersion() < 2.0)
    {
        LangElement* shiftCoords = new GenOp("   @ = @ / 2.0 + 0.5;\r\n\n", lightVec, lightVec);
        meta->addStatement(shiftCoords);
    }


    output = meta;
}

//----------------------------------------------------------------------------
// Get lightVec from tex coord or light normal map - pixel shader
//----------------------------------------------------------------------------
Var* BumpFeat::getLightVec(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd,
    MultiLine* meta)
{
    Var* lightVec = NULL;

    if (fd.features[GFXShaderFeatureData::LightNormMap])
    {
        // grab lightmap normal
        lightVec = (Var*)LangElement::find("lightmapNormal");

        AssertFatal(lightVec, "Could not find lightmap normal.");
    }
    else
    {
        meta->addStatement(new GenOp("\r\n"));

        // grab vertex lightVec
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        lightVec = connectComp->getElement(RT_TEXCOORD);
        lightVec->setName("lightVec");
        lightVec->setStructName("IN");
        lightVec->setType("float3");

        if (GFX->getPixelShaderVersion() < 2.0)
        {
            // need to expand the lightVec on ps 1.x cards because it clamps tex coords
            // to a range from 0.0 to 1.0
            meta->addStatement(new GenOp("   @ = @ * 2.0 - 1.0;\r\n", lightVec, lightVec));
        }
    }

    return lightVec;
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void BumpFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    Var* texCoord = NULL;

    if (GFX->getPixelShaderVersion() < 2.0)
    {
        // grab connector texcoord register
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        texCoord = connectComp->getElement(RT_TEXCOORD);
        texCoord->setName("bumpCoord");
        texCoord->setStructName("IN");
        texCoord->setType("float2");
        texCoord->mapsToSampler = true;
    }
    else
    {
        texCoord = (Var*)LangElement::find("texCoord");
        if (!texCoord)
        {
            // grab connector texcoord register
            ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
            texCoord = connectComp->getElement(RT_TEXCOORD);
            texCoord->setName("texCoord");
            texCoord->setStructName("IN");
            texCoord->setType("float2");
            texCoord->mapsToSampler = true;
        }
    }


    // create texture var
    Var* bumpMap = new Var;
    bumpMap->setType("sampler2D");
    bumpMap->setName("bumpMap");
    bumpMap->uniform = true;
    bumpMap->sampler = true;
    bumpMap->constNum = Var::getTexUnitNum();     // used as texture unit num here


    LangElement* texOp = new GenOp("tex2D(@, @)", bumpMap, texCoord);


    // create bump normal
    Var* bumpNorm = new Var;
    bumpNorm->setName("bumpNormal");
    bumpNorm->setType("float4");

    LangElement* bumpNormDecl = new DecOp(bumpNorm);

    MultiLine* meta = new MultiLine;
    meta->addStatement(new GenOp("   @ = @;\r\n", bumpNormDecl, texOp));


    Var* lightVec = getLightVec(componentList, fd, meta);


    // create bumpDot var
    Var* bumpDot = new Var;
    bumpDot->setName("bumpDot");
    bumpDot->setType("float4");

    LangElement* bumpDotDecl = new DecOp(bumpDot);

#ifdef MB_ULTRA
    // In MBU x360, this was changed to make the edges more vibrant, and also
    // to make the grip texture not lose bump mapping when there is no light pointing at it.
    meta->addStatement(new GenOp("   @ = ( dot(@.xyz * 2.0 - 1.0, @.xyz) + 1.0 ) * 0.5;\r\n",
        bumpDotDecl, bumpNorm, lightVec));
#else

    Var* dlightcoord = (Var*)LangElement::find("dlightCoord");

    // assign it
    if ((GFX->getPixelShaderVersion() < 2.0) ||
        (!fd.features[GFXShaderFeatureData::DynamicLight]))
    {
        if (dlightcoord && (GFX->getPixelShaderVersion() >= 2.0))
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, @.xyz) * @.w );\r\n",
                bumpDotDecl, bumpNorm, lightVec, dlightcoord));
        else
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, @.xyz) );\r\n",
                bumpDotDecl, bumpNorm, lightVec));
    }
    else
    {
        if (dlightcoord && (GFX->getPixelShaderVersion() >= 2.0))
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, normalize(@.xyz)) * @.w );\r\n",
                bumpDotDecl, bumpNorm, lightVec, dlightcoord));
        else
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, normalize(@.xyz)) );\r\n",
                bumpDotDecl, bumpNorm, lightVec));

        if (fd.features[GFXShaderFeatureData::DynamicLightDual])
        {
            ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
            Var* lightVecSec = connectComp->getElement(RT_TEXCOORD);
            lightVecSec->setName("lightVecSec");
            lightVecSec->setStructName("IN");
            lightVecSec->setType("float3");
            Var* attnsec = (Var*)LangElement::find("attnsec");
            Var* dlightcoordsec = (Var*)LangElement::find("dlightCoordSec");
            if (attnsec && dlightcoordsec)
                meta->addStatement(new GenOp("   @ *= saturate( dot(@.xyz * 2.0 - 1.0, normalize(@.xyz)) * @.w );\r\n",
                    attnsec, bumpNorm, lightVecSec, dlightcoordsec));
        }
    }

#endif

    if (fd.features[GFXShaderFeatureData::LightNormMap])
    {
        meta->addStatement(new GenOp("   @;\r\n", assignColor(bumpDot)));
        output = meta;
        return;
    }

    // create ambient var
    Var* ambient = (Var*)LangElement::find("ambient");
    if (!ambient)
    {
        ambient = new Var;
        ambient->setType("float4");
        ambient->setName("ambient");
        ambient->uniform = true;
        ambient->constNum = PC_AMBIENT_COLOR;
    }

    Var* shading = (Var*)LangElement::find("shading");
    if (shading)
    {
        LangElement* final = new GenOp("(@ * @) + @;", shading, bumpDot, ambient);
        meta->addStatement(new GenOp("   @\r\n", assignColor(final)));
        output = meta;
        return;
    }

    if (fd.features[GFXShaderFeatureData::LightMap])
    {
        // find lightmap color
        Var* lmColor = (Var*)LangElement::find("lmColor");

        // calc final result
        LangElement* final = new GenOp("@ * @ + @;", bumpDot, lmColor, ambient);
        meta->addStatement(new GenOp("   @\r\n", assignColor(final)));
        output = meta;
        return;
    }

    if (fd.features[GFXShaderFeatureData::DynamicLight])
    {
        meta->addStatement(new GenOp("   @;\r\n", assignColor(bumpDot)));
    }
    else
    {
        LangElement* final = new GenOp("@ + @;", bumpDot, ambient);
        meta->addStatement(new GenOp("   @\r\n", assignColor(final)));
    }

    if (fd.features[GFXShaderFeatureData::DynamicLightDual])
    {
        Var* attnsec = (Var*)LangElement::find("attnsec");
        Var* diffuseColor = (Var*)LangElement::find("diffuseColor");
        if (attnsec && diffuseColor)
        {
            LangElement* blend = new GenOp("(@ * @)", attnsec, diffuseColor);
            meta->addStatement(new GenOp("   @;\r\n", assignColor(blend, true)));
        }
    }

    output = meta;
}


//----------------------------------------------------------------------------
// Add bumpmapping texture coords
//----------------------------------------------------------------------------
LangElement* BumpFeat::addBumpCoords(Vector<ShaderComponent*>& componentList)
{

    // grab outgoing texture coords
    Var* outBaseTexCoords = (Var*)LangElement::find("outTexCoord");

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outTex = connectComp->getElement(RT_TEXCOORD);
    outTex->setName("bumpCoord");
    outTex->setStructName("OUT");
    outTex->setType("float2");
    outTex->mapsToSampler = true;

    return new GenOp("   @ = @;\r\n", outTex, outBaseTexCoords);


}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources BumpFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;

    res.numTex = 1;


    if (GFX->getPixelShaderVersion() < 2.0)
    {
        // pixel version 1.x - each texture needs its own set of uv coords
        res.numTexReg++;
    }
    else
    {
        // if no base tex, then it needs to send its own tex coords
        if (!fd.features[GFXShaderFeatureData::BaseTex])
        {
            res.numTexReg++;
        }
    }

    // No light norm map - must send light dir to pixel shader
    if (!fd.features[GFXShaderFeatureData::LightNormMap])
    {
        res.numTexReg++;
    }

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void BumpFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    if (stageDat.tex[GFXShaderFeatureData::BumpMap])
    {
        passData.tex[texIndex++] = stageDat.tex[GFXShaderFeatureData::BumpMap];
    }
}

