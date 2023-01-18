//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "shaderFeature.h"
#include "langElement.h"
#include "shaderOp.h"
#include "../../game/shaders/shdrConsts.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"

//****************************************************************************
// Misc functions
//****************************************************************************

//----------------------------------------------------------------------------
// Set up a texture space matrix
//----------------------------------------------------------------------------
LangElement* ShaderFeature::setupTexSpaceMat(Vector<ShaderComponent*>& componentList, Var** texSpaceMat)
{
    Var* T = (Var*)LangElement::find("T");
    Var* B = (Var*)LangElement::find("B");
    Var* N = (Var*)LangElement::find("N");

    // setup matrix var
    *texSpaceMat = new Var;
    (*texSpaceMat)->setType("float3x3");
    (*texSpaceMat)->setName("objToTangentSpace");

    MultiLine* meta = new MultiLine;
    meta->addStatement(new GenOp("\r\n"));
    meta->addStatement(new GenOp("   @;\r\n", new DecOp(*texSpaceMat)));
    meta->addStatement(new GenOp("   @[0] = @;\r\n", *texSpaceMat, T));
    meta->addStatement(new GenOp("   @[1] = @;\r\n", *texSpaceMat, B));
    meta->addStatement(new GenOp("   @[2] = @;\r\n\n", *texSpaceMat, N));

    return meta;

}

//----------------------------------------------------------------------------
// Get the color assignment
//----------------------------------------------------------------------------
LangElement* ShaderFeature::assignColor(LangElement* elem, bool add)
{
    LangElement* assign;

    // search for color var
    Var* color = (Var*)LangElement::find("col");

    if (!color)
    {
        // create color var
        color = new Var;
        color->setType("fragout");
        color->setName("col");
        color->setStructName("OUT");

        assign = new GenOp("@ = @", color, elem);
    }
    else
    {
        if (add)
        {
            assign = new GenOp("@ += @", color, elem);
        }
        else
        {
            assign = new GenOp("@ *= @", color, elem);
        }
    }


    return assign;

}

//****************************************************************************
// Shader Feature
//****************************************************************************
ShaderFeature::ShaderFeature()
{
    output = NULL;
}

//----------------------------------------------------------------------------
// Get the incoming base texture coords - useful for bumpmap and detail maps
//----------------------------------------------------------------------------
Var* ShaderFeature::getVertBaseTex()
{
    Var* inTex = NULL;

    for (U32 i = 0; i < LangElement::elementList.size(); i++)
    {
        if (!dStrcmp((char*)LangElement::elementList[i]->name, "texCoord"))
        {
            inTex = dynamic_cast<Var*>(LangElement::elementList[i]);

            if (inTex)
            {
                if (!dStrcmp((char*)inTex->structName, "IN"))
                {
                    break;
                }
                else
                {
                    inTex = NULL;
                }
            }
        }
    }

    return inTex;
}


//****************************************************************************
// Base Texture
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void BaseTexFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // find incoming texture var
    Var* inTex = getVertBaseTex();


    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outTex = connectComp->getElement(RT_TEXCOORD);
    outTex->setName("outTexCoord");  // named outTexCoord so it can be distinguished from IN.texCoord in var name search
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

        output = new GenOp("   @ = mul(@, @);\r\n", outTex, texMat, inTex);
        return;
    }

    // setup language elements to output incoming tex coords to output
    output = new GenOp("   @ = @;\r\n", outTex, inTex);

}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void BaseTexFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* inTex = connectComp->getElement(RT_TEXCOORD);
    inTex->setName("texCoord");
    inTex->setStructName("IN");
    inTex->setType("float2");
    inTex->mapsToSampler = true;

    // create texture var
    Var* diffuseMap = new Var;
    diffuseMap->setType("sampler2D");
    diffuseMap->setName("diffuseMap");
    diffuseMap->uniform = true;
    diffuseMap->sampler = true;
    diffuseMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

    if (fd.features[GFXShaderFeatureData::CubeMap] ||
        fd.features[GFXShaderFeatureData::PixSpecular] ||
        fd.features[GFXShaderFeatureData::DynamicLightDual]
        )
    {
        MultiLine* meta = new MultiLine;

        // create sample color
        Var* diffColor = new Var;
        diffColor->setType("float4");
        diffColor->setName("diffuseColor");
        LangElement* colorDecl = new DecOp(diffColor);

        meta->addStatement(new GenOp("   @ = tex2D(@, @);\r\n",
            colorDecl,
            diffuseMap,
            inTex));

        meta->addStatement(new GenOp("   @;\r\n", assignColor(diffColor)));
        output = meta;
    }
    else
    {
        LangElement* statement = new GenOp("tex2D(@, @)", diffuseMap, inTex);
        output = new GenOp("   @;\r\n", assignColor(statement));
    }

}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources BaseTexFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;
    res.numTex = 1;
    res.numTexReg = 1;

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void BaseTexFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    if (stageDat.tex[GFXShaderFeatureData::BaseTex])
    {
        passData.tex[texIndex++] = stageDat.tex[GFXShaderFeatureData::BaseTex];
    }
}

//****************************************************************************
// Lightmap
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void LightmapFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab tex register from incoming vert
    Var* inTex = (Var*)LangElement::find("lmCoord");

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outTex = connectComp->getElement(RT_TEXCOORD);
    outTex->setName("lmCoord");
    outTex->setStructName("OUT");
    outTex->setType("float2");
    outTex->mapsToSampler = true;

    // setup language elements to output incoming tex coords to output
    output = new GenOp("   @ = @;\r\n", outTex, inTex);
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void LightmapFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* inTex = connectComp->getElement(RT_TEXCOORD);
    inTex->setName("lmCoord");
    inTex->setStructName("IN");
    inTex->setType("float2");
    inTex->mapsToSampler = true;

    // create texture var
    Var* diffuseMap = new Var;
    diffuseMap->setType("sampler2D");
    diffuseMap->setName("lightMap");
    diffuseMap->uniform = true;
    diffuseMap->sampler = true;
    diffuseMap->constNum = Var::getTexUnitNum();     // used as texture unit num here


    // argh, pixel specular should prob use this too
    if (fd.features[GFXShaderFeatureData::BumpMap] &&
        !fd.features[GFXShaderFeatureData::LightNormMap])
    {
        Var* lmColor = new Var;
        lmColor->setName("lmColor");
        lmColor->setType("float4");
        LangElement* lmColorDecl = new DecOp(lmColor);

        output = new GenOp("   @ = tex2D(@, @);\r\n", lmColorDecl, diffuseMap, inTex);
        return;
    }

    // add gouraud shading if realtime lighting and no bumpmap
    LangElement* statement = NULL;
    if (fd.features[GFXShaderFeatureData::RTLighting])
    {
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

        // grab connector color register
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        Var* inColor = connectComp->getElement(RT_COLOR);
        inColor->setName("shading");
        inColor->setStructName("IN");
        inColor->setType("float4");

        statement = new GenOp("tex2D(@, @) * @ + @", diffuseMap, inTex, inColor, ambient);
    }
    else
    {
        statement = new GenOp("tex2D(@, @)", diffuseMap, inTex);
    }
    output = new GenOp("   @;\r\n", assignColor(statement));
}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources LightmapFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;
    res.numTex = 1;
    res.numTexReg = 1;

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void LightmapFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    passData.texFlags[texIndex++] = Material::Lightmap;
}

//****************************************************************************
// Detail map
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void DetailFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab incoming texture coords
    Var* inTex = getVertBaseTex();

    // create detail variable
    Var* detScale = new Var;
    detScale->setType("float");
    detScale->setName("detailScale");
    detScale->uniform = true;
    detScale->constNum = VC_DETAIL_SCALE;

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outTex = connectComp->getElement(RT_TEXCOORD);
    outTex->setName("detCoord");
    outTex->setStructName("OUT");
    outTex->setType("float2");
    outTex->mapsToSampler = true;

    // setup output to mul texCoord by detail scale
    output = new GenOp("   @ = @ * @;\r\n", outTex, inTex, detScale);
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void DetailFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* inTex = connectComp->getElement(RT_TEXCOORD);
    inTex->setName("detCoord");
    inTex->setStructName("IN");
    inTex->setType("float2");
    inTex->mapsToSampler = true;

    // create texture var
    Var* detailMap = new Var;
    detailMap->setType("sampler2D");
    detailMap->setName("detailMap");
    detailMap->uniform = true;
    detailMap->sampler = true;
    detailMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

    LangElement* statement = new GenOp("tex2D(@, @)", detailMap, inTex);
    output = new GenOp("   @;\r\n", assignColor(statement));

}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources DetailFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;
    res.numTex = 1;
    res.numTexReg = 1;

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void DetailFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    if (stageDat.tex[GFXShaderFeatureData::DetailMap])
    {
        passData.tex[texIndex++] = stageDat.tex[GFXShaderFeatureData::DetailMap];
    }
}

//****************************************************************************
// Vertex position
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void VertPosition::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab incoming vert position
    ConnectorStruct* vertComp = dynamic_cast<ConnectorStruct*>(componentList[C_VERT_STRUCT]);
    Var* inPosition = vertComp->getElement(RT_POSITION);
    inPosition->setName("position");
    inPosition->setStructName("IN");

    // grab connector position
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outPosition = connectComp->getElement(RT_POSITION);
    outPosition->setName("hpos");
    outPosition->setStructName("OUT");

    // create modelview variable
    Var* modelview = new Var;
    modelview->setType("float4x4");
    modelview->setName("modelview");
    modelview->uniform = true;
    modelview->constNum = VC_WORLD_PROJ;

    LangElement* statement = new GenOp("   @ = mul(@, @);\r\n", outPosition, modelview, inPosition);
    output = statement;

}


//****************************************************************************
// Light Normal Map
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void LightNormMapFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    if (GFX->getPixelShaderVersion() >= 2.0)
    {
        output = NULL;
        return;
    }


    // can safely assume there is a lightmap
    Var* texCoord = (Var*)LangElement::find("lmCoord");

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outTex = connectComp->getElement(RT_TEXCOORD);
    outTex->setName("lightNormCoord");
    outTex->setStructName("OUT");
    outTex->setType("float2");
    outTex->mapsToSampler = true;

    // setup output to mul texCoord by detail scale
    output = new GenOp("   @ = @;\r\n", outTex, texCoord);
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void LightNormMapFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    Var* texCoord = NULL;

    if (GFX->getPixelShaderVersion() < 2.0)
    {
        // grab connector texcoord register
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        texCoord = connectComp->getElement(RT_TEXCOORD);
        texCoord->setName("lightNormCoord");
        texCoord->setStructName("IN");
        texCoord->setType("float2");
        texCoord->mapsToSampler = true;
    }
    else
    {
        // can safely assume there is a lightmap
        texCoord = (Var*)LangElement::find("lmCoord");
    }


    // create texture var
    Var* lightNormMap = new Var;
    lightNormMap->setType("sampler2D");
    lightNormMap->setName("lightNormMap");
    lightNormMap->uniform = true;
    lightNormMap->sampler = true;
    lightNormMap->constNum = Var::getTexUnitNum();     // used as texture unit num here



    // create lightmap normal
    Var* lightmapNorm = new Var;
    lightmapNorm->setName("lightmapNormal");
    lightmapNorm->setType("float4");

    LangElement* lightNormDecl = new DecOp(lightmapNorm);

    output = new GenOp("   @ = tex2D(@, @) * 2.0 - 1.0;\r\n", lightNormDecl, lightNormMap, texCoord);
}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources LightNormMapFeat::getResources(GFXShaderFeatureData& fd)
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
        // if no lightmap tex, then it needs to send its own tex coords
        if (!fd.features[GFXShaderFeatureData::LightMap])
        {
            res.numTexReg++;
        }
    }
    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void LightNormMapFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    passData.texFlags[texIndex++] = Material::NormLightmap;
}

//****************************************************************************
// Reflect Cubemap
//****************************************************************************
//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void ReflectCubeFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    MultiLine* meta = new MultiLine;

    // If a base or bump tex is present in the material, but not in the
    // current pass - we need to add one to the current pass to use
    // its alpha channel as a gloss map.  Here we just need the tex coords.
    if (!fd.features[GFXShaderFeatureData::BaseTex] &&
        !fd.features[GFXShaderFeatureData::BumpMap])
    {
        if (fd.materialFeatures[GFXShaderFeatureData::BaseTex] ||
            fd.materialFeatures[GFXShaderFeatureData::BumpMap])
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

            // setup language elements to output incoming tex coords to output
            meta->addStatement(new GenOp("   @ = @;\r\n", outTex, inTex));
        }
    }

    // create cubeTrans
    Var* cubeTrans = new Var;
    cubeTrans->setType("float3x3");
    cubeTrans->setName("cubeTrans");
    cubeTrans->uniform = true;
    cubeTrans->constNum = VC_CUBE_TRANS;

    // create cubeEye position
    Var* cubeEyePos = new Var;
    cubeEyePos->setType("float3");
    cubeEyePos->setName("cubeEyePos");
    cubeEyePos->uniform = true;
    cubeEyePos->constNum = VC_CUBE_EYE_POS;

    // search for vert normal
    Var* inNormal = (Var*)LangElement::find("normal");

    // cube vert position
    Var* cubeVertPos = new Var;
    cubeVertPos->setName("cubeVertPos");
    cubeVertPos->setType("float3");
    LangElement* cubeVertPosDecl = new DecOp(cubeVertPos);

    meta->addStatement(new GenOp("   @ = mul(@, @).xyz;\r\n",
        cubeVertPosDecl, cubeTrans, LangElement::find("position")));

    // cube normal
    Var* cubeNormal = new Var;
    cubeNormal->setName("cubeNormal");
    cubeNormal->setType("float3");
    LangElement* cubeNormDecl = new DecOp(cubeNormal);

    meta->addStatement(new GenOp("   @ = normalize( mul(@, @).xyz );\r\n",
        cubeNormDecl, cubeTrans, inNormal));

    // eye to vert
    Var* eyeToVert = new Var;
    eyeToVert->setName("eyeToVert");
    eyeToVert->setType("float3");
    LangElement* e2vDecl = new DecOp(eyeToVert);

    meta->addStatement(new GenOp("   @ = @ - @;\r\n",
        e2vDecl, cubeVertPos, cubeEyePos));

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* reflectVec = connectComp->getElement(RT_TEXCOORD);
    reflectVec->setName("reflectVec");
    reflectVec->setStructName("OUT");
    reflectVec->setType("float3");
    reflectVec->mapsToSampler = true;

    meta->addStatement(new GenOp("   @ = reflect(@, @);\r\n", reflectVec, eyeToVert, cubeNormal));

    output = meta;
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void ReflectCubeFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    MultiLine* meta = new MultiLine;
    Var* glossColor = NULL;

    // If a base or bump tex is present in the material, but not in the
    // current pass - we need to add one to the current pass to use
    // its alpha channel as a gloss map.
    if (!fd.features[GFXShaderFeatureData::BaseTex] &&
        !fd.features[GFXShaderFeatureData::BumpMap])
    {
        if (fd.materialFeatures[GFXShaderFeatureData::BaseTex] ||
            fd.materialFeatures[GFXShaderFeatureData::BumpMap])
        {
            // grab connector texcoord register
            ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
            Var* inTex = connectComp->getElement(RT_TEXCOORD);
            inTex->setName("texCoord");
            inTex->setStructName("IN");
            inTex->setType("float2");
            inTex->mapsToSampler = true;

            // create texture var
            Var* newMap = new Var;
            newMap->setType("sampler2D");
            newMap->setName("glossMap");
            newMap->uniform = true;
            newMap->sampler = true;
            newMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

            // create sample color
            Var* color = new Var;
            color->setType("float4");
            color->setName("diffuseColor");
            LangElement* colorDecl = new DecOp(color);

            glossColor = color;

            meta->addStatement(new GenOp("   @ = tex2D( @, @ );\r\n", colorDecl, newMap, inTex));
        }
    }
    else
    {
        glossColor = (Var*)LangElement::find("diffuseColor");
        if (!glossColor)
        {
            glossColor = (Var*)LangElement::find("bumpNormal");
        }
    }

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* reflectVec = connectComp->getElement(RT_TEXCOORD);
    reflectVec->setName("reflectVec");
    reflectVec->setStructName("IN");
    reflectVec->setType("float3");
    reflectVec->mapsToSampler = true;

    // create cubemap var
    Var* cubeMap = new Var;
    cubeMap->setType("samplerCUBE");
    cubeMap->setName("cubeMap");
    cubeMap->uniform = true;
    cubeMap->sampler = true;
    cubeMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

    Var* attn = NULL;
    if (fd.materialFeatures[GFXShaderFeatureData::DynamicLight])
        attn = (Var*)LangElement::find("attn");

    if (glossColor)
    {
        LangElement* statement;
        if (attn)
            statement = new GenOp("@.x * @.a * texCUBE(@, @)", attn, glossColor, cubeMap, reflectVec);
        else
            statement = new GenOp("@.a * texCUBE(@, @)", glossColor, cubeMap, reflectVec);
        meta->addStatement(new GenOp("   @;\r\n", assignColor(statement, true)));
        output = meta;
    }
    else
    {
        LangElement* statement;
        if (attn)
            statement = new GenOp("@.x * texCUBE(@, @)", attn, cubeMap, reflectVec);
        else
            statement = new GenOp("texCUBE(@, @)", cubeMap, reflectVec);
        output = new GenOp("   @;\r\n", assignColor(statement, true));
    }

}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources ReflectCubeFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;

    if (fd.features[GFXShaderFeatureData::BaseTex] ||
        fd.features[GFXShaderFeatureData::BumpMap])
    {
        res.numTex = 1;
        res.numTexReg = 1;
    }
    else
    {
        res.numTex = 2;
        res.numTexReg = 2;
    }

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void ReflectCubeFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& stageFeatures,
    RenderPassData& passData,
    U32& texIndex)
{
    // set up a gloss map if one is not present in the current pass
    // but is present in the current material stage
    if (!passData.fData.features[GFXShaderFeatureData::BaseTex] &&
        !passData.fData.features[GFXShaderFeatureData::BumpMap])
    {
        if (stageFeatures.features[GFXShaderFeatureData::BaseTex] &&
            stageDat.tex[GFXShaderFeatureData::BaseTex])
        {
            passData.tex[texIndex++] = stageDat.tex[GFXShaderFeatureData::BaseTex];
        }
        else
        {
            if (stageFeatures.features[GFXShaderFeatureData::BumpMap] &&
                stageDat.tex[GFXShaderFeatureData::BumpMap])
            {
                passData.tex[texIndex++] = stageDat.tex[GFXShaderFeatureData::BumpMap];
            }
        }
    }

    if (stageDat.cubemap)
    {
        passData.cubeMap = stageDat.cubemap;
        passData.texFlags[texIndex++] = Material::Cube;
    }
    else
    {
        if (stageFeatures.CubeMap)
        {
            // assuming here that it is a scenegraph cubemap
            passData.texFlags[texIndex++] = Material::SGCube;
        }
    }

}


//****************************************************************************
// VerLightColor
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void VertLightColor::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // arrgh, need to know if vert has normal, if not, then do nothing

    if (!fd.useLightDir)
    {
        output = NULL;
        return;
    }

    // grab connector color
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* outColor = connectComp->getElement(RT_COLOR);
    outColor->setName("shading");
    outColor->setStructName("OUT");
    outColor->setType("float4");

    Var* inColor = new Var;
    inColor->setType("float4");
    inColor->setName("inLightColor");
    inColor->uniform = true;
    inColor->constNum = VC_LIGHT_DIFFUSE1;


    // if bumpmap present, we just want to transfer the color
    if (fd.features[GFXShaderFeatureData::BumpMap])
    {
        output = new GenOp("   @ = @;\r\n", outColor, inColor);
        return;
    }


    // search for vert normal
    Var* inNormal = (Var*)LangElement::find("normal");

    // search for light dir
    Var* inLightVec = (Var*)LangElement::find("inLightVec");
    if (!inLightVec)
    {
        // grab light direction var
        inLightVec = new Var;
        inLightVec->setType("float3");
        inLightVec->setName("inLightVec");
        inLightVec->uniform = true;
        inLightVec->constNum = VC_LIGHT_DIR1;
    }

    MultiLine* meta = new MultiLine;
    meta->addStatement(new GenOp("   @ = saturate( dot(-@, @) );\r\n", outColor, inLightVec, inNormal));
    meta->addStatement(new GenOp("   @.w = 1.0;\r\n", outColor));
    meta->addStatement(new GenOp("   @ *= @;\r\n", outColor, inColor));
    output = meta;

}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void VertLightColor::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // arrgh, need to know if vert has normal, if not, then do nothing


    if (fd.features[GFXShaderFeatureData::LightMap] ||
        !fd.useLightDir)
    {
        output = NULL;
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

    // grab connector color register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* inColor = connectComp->getElement(RT_COLOR);
    inColor->setName("shading");
    inColor->setStructName("IN");
    inColor->setType("float4");


    // if bumpmap present, do NOT add ambient to shading color, this happens later
    if (fd.features[GFXShaderFeatureData::BumpMap])
    {
        output = new GenOp("");// "   @;\r\n", assignColor( inColor ) );
        return;
    }

    LangElement* addAmbient = new GenOp("@ + @", inColor, ambient);
    output = new GenOp("   @;\r\n", assignColor(addAmbient));

}


//****************************************************************************
// Fog
//****************************************************************************

//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void FogFeat::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* fogCoord = connectComp->getElement(RT_TEXCOORD);
    fogCoord->setName("fogCoord");
    fogCoord->setStructName("OUT");
    fogCoord->setType("float2");
    fogCoord->mapsToSampler = true;

    // grab the eye position
    Var* eyePos = (Var*)LangElement::find("eyePos");
    if (!eyePos)
    {
        eyePos = new Var;
        eyePos->setType("float3");
        eyePos->setName("eyePos");
        eyePos->uniform = true;
        eyePos->constNum = VC_EYE_POS;
    }

    Var* position = (Var*)LangElement::find("position");

    Var* fogData = new Var;
    fogData->setType("float3");
    fogData->setName("fogData");
    fogData->uniform = true;
    fogData->constNum = VC_FOGDATA;

    Var* objTrans = (Var*)LangElement::find("objTrans");
    if (!objTrans)
    {
        objTrans = new Var;
        objTrans->setType("float4x4");
        objTrans->setName("objTrans");
        objTrans->uniform = true;
        objTrans->constNum = VC_OBJ_TRANS;
    }

    Var* transPos = new Var;
    transPos->setType("float3");
    transPos->setName("transPos");
    LangElement* transPosDecl = new DecOp(transPos);


    MultiLine* meta = new MultiLine;

    meta->addStatement(new GenOp("\r\n   // fog setup\r\n   @ = mul( @, @ );\r\n", transPosDecl, objTrans, position));


    LangElement* s1 = new GenOp("   @.x = 1.0 - ( distance( @, @ ) / @.z );\r\n",
        fogCoord,
        position,
        eyePos,
        fogData);

    meta->addStatement(s1);

    LangElement* s2 = new GenOp("   @.y = (@.z - @.x) * @.y;\r\n",
        fogCoord,
        transPos,
        fogData,
        fogData);
    meta->addStatement(s2);

    output = meta;
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void FogFeat::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // create texture var
    Var* fogMap = new Var;
    fogMap->setType("sampler2D");
    fogMap->setName("fogMap");
    fogMap->uniform = true;
    fogMap->sampler = true;
    fogMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

    // grab connector texcoord register
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* fogCoord = connectComp->getElement(RT_TEXCOORD);
    fogCoord->setName("fogCoord");
    fogCoord->setStructName("IN");
    fogCoord->setType("float2");
    fogCoord->mapsToSampler = true;


    Var* fogColor = new Var;
    fogColor->setType("float4");
    fogColor->setName("fogColor");
    LangElement* fogColorDecl = new DecOp(fogColor);

    MultiLine* meta = new MultiLine;
    meta->addStatement(new GenOp("   @ = tex2D(@, @);\r\n", fogColorDecl, fogMap, fogCoord));

    // if no color, then fog is only feature in pass
    Var* color = (Var*)LangElement::find("col");
    if (!color)
    {
        meta->addStatement(new GenOp("   @;\r\n", assignColor(fogColor)));
    }
    else
    {
        if (fd.features[GFXShaderFeatureData::DynamicLight])
        {
            meta->addStatement(new GenOp("   @.a = 1.0 - @.a;\r\n", color, fogColor));
        }
        else
        {
            LangElement* statement = new GenOp("lerp(@.rgb, @.rgb, @.a)", color, fogColor, fogColor);
            meta->addStatement(new GenOp("   @.rgb = @;\r\n", color, statement));
        }
    }

    output = meta;
    return;

}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources FogFeat::getResources(GFXShaderFeatureData& fd)
{
    Resources res;
    res.numTex = 1;
    res.numTexReg = 1;

    return res;
}

//----------------------------------------------------------------------------
// Set texture data
//----------------------------------------------------------------------------
void FogFeat::setTexData(Material::StageData& stageDat,
    GFXShaderFeatureData& fd,
    RenderPassData& passData,
    U32& texIndex)
{
    passData.texFlags[texIndex++] = Material::Fog;

}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void VisibilityFeat::processPix(Vector<ShaderComponent*>& componentList, GFXShaderFeatureData& fd)
{
    // create visibility var
    Var* visibility = new Var;
    visibility->setType("float");
    visibility->setName("visibility");
    visibility->uniform = true;
    visibility->constNum = PC_VISIBILITY;

    // search for color var
    Var* color = (Var*)LangElement::find("col");

    // Looks like its going to be a multiline statement
    MultiLine* meta = new MultiLine;

    if (!color)
    {
        // create color var
        color = new Var;
        color->setType("fragout");
        color->setName("col");
        color->setStructName("OUT");

        // link it to ConnectData.shading
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        Var* inColor = connectComp->getElement(RT_COLOR);
        inColor->setName("shading");
        inColor->setStructName("IN");
        inColor->setType("float4");

        meta->addStatement(new GenOp("   @ = @;\r\n", color, inColor));
    }

    meta->addStatement(new GenOp("   @.a *= @;\r\n", color, visibility));

    // Debugging
    //meta->addStatement( new GenOp( "   @ = float4( @, @, @, 1.0 );\r\n", color, visibility, visibility, visibility ) );
    //meta->addStatement( new GenOp( "   return OUT;\r\n" ) );

    output = meta;
}

//----------------------------------------------------------------------------
// Color multiply feature
//----------------------------------------------------------------------------
void ColorMultiplyFeat::processPix( Vector<ShaderComponent*> &componentList, GFXShaderFeatureData &fd )
{
    Var *colorMultiply  = new Var;
    colorMultiply->setType( "float4" );
    colorMultiply->setName( "colorMultiply" );
    colorMultiply->uniform = true;
    colorMultiply->constNum = PC_COLORMULTIPLY;

    // search for color var
    Var *color = (Var*) LangElement::find( "col" );
    if (color)
    {
        MultiLine* meta = new MultiLine;
        LangElement* statement = new GenOp("lerp(@.rgb, @.rgb, @.a)", color, colorMultiply, colorMultiply);
        meta->addStatement(new GenOp("   @.rgb = @;\r\n", color, statement));
        output = meta;
    }
}
