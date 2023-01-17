//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "pixSpecular.h"
#include "shaderOp.h"
#include "../../game/shaders/shdrConsts.h"
#include "gfx/gfxStructs.h"

//****************************************************************************
// Per-Pixel Specular
//****************************************************************************
//----------------------------------------------------------------------------
// Process vertex shader feature
//----------------------------------------------------------------------------
void PixelSpecular::processVert(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    MultiLine* meta = new MultiLine;
    Var* texSpaceMat = NULL;

    // setup the texture space matrix - necessary if bumpmapping or using light-normal map
    if (fd.features[GFXShaderFeatureData::BumpMap] ||
        fd.features[GFXShaderFeatureData::LightNormMap])
    {
        texSpaceMat = (Var*)LangElement::find("objToTangentSpace");
        if (!texSpaceMat)
        {
            LangElement* texSpaceSetup = setupTexSpaceMat(componentList, &texSpaceMat);
            meta->addStatement(texSpaceSetup);
        }
    }

    // grab the eye position
    Var* eyePos = new Var;
    eyePos->setType("float3");
    eyePos->setName("eyePos");
    eyePos->uniform = true;
    eyePos->constNum = VC_EYE_POS;


    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);

    // pass out normal
    if (!fd.features[GFXShaderFeatureData::BumpMap])
    {
        meta->addStatement(new GenOp("\r\n"));

        // search for vert normal
        Var* inNorm = (Var*)LangElement::find("normal");
        Var* outNorm = connectComp->getElement(RT_TEXCOORD);
        outNorm->setName("outNormal");
        outNorm->setStructName("OUT");
        outNorm->setType("float3");

        if (fd.features[GFXShaderFeatureData::LightNormMap])
        {
            meta->addStatement(new GenOp("   @ = mul( @, @ );\r\n", outNorm, texSpaceMat, inNorm));
        }
        else
        {
            meta->addStatement(new GenOp("   @ = @;\r\n", outNorm, inNorm));
        }

    }



    // pass out pixel position
    Var* inPos = (Var*)LangElement::find("position");

    Var* position = connectComp->getElement(RT_TEXCOORD);
    position->setName("pos");
    position->setStructName("OUT");
    position->setType("float3");

    LangElement* posStmt = NULL;

    if (fd.features[GFXShaderFeatureData::BumpMap] ||
        fd.features[GFXShaderFeatureData::LightNormMap])
    {
        posStmt = new GenOp("mul(@, @.xyz / 100.0);", texSpaceMat, inPos);
    }
    else
    {
        posStmt = new GenOp("@.xyz", inPos);
    }

    meta->addStatement(new GenOp("   @ = @;\r\n", position, posStmt));


    // pass out eye position
    Var* outEyePos = connectComp->getElement(RT_TEXCOORD);
    outEyePos->setName("outEyePos");
    outEyePos->setStructName("OUT");
    outEyePos->setType("float3");

    LangElement* eyeStmt = NULL;

    if (fd.features[GFXShaderFeatureData::BumpMap] ||
        fd.features[GFXShaderFeatureData::LightNormMap])
    {
        eyeStmt = new GenOp("mul(@, @.xyz / 100.0);", texSpaceMat, eyePos);
    }
    else
    {
        eyeStmt = new GenOp("@.xyz", eyePos);
    }

    meta->addStatement(new GenOp("   @.xyz = @;\r\n", outEyePos, eyeStmt));


    // pass out light position - only if there isn't lightNormMap
    if (!fd.features[GFXShaderFeatureData::LightNormMap])
    {
        bool useneg = true;
        Var* inLightVec = (Var*)LangElement::find("inLightVec");
        Var* outLightVec = (Var*)LangElement::find("outLightVec");

        //if (fd.features[GFXShaderFeatureData::SelfIllumination])
        //{
        //    inLightVec = (Var*)LangElement::find("N");
        //    useneg = false;
        //}
        //else
        //{
            if (!inLightVec)
            {
                // grab light direction var
                inLightVec = new Var;
                inLightVec->setType("float3");
                inLightVec->setName("inLightVec");
                inLightVec->uniform = true;
                inLightVec->constNum = VC_LIGHT_DIR1;
            }
        //}

        if (!outLightVec)
        {
            // grab connector texcoord register
            ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
            outLightVec = connectComp->getElement(RT_TEXCOORD);
            outLightVec->setName("outLightVec");
            outLightVec->setStructName("OUT");
            outLightVec->setType("float4");

            if (useneg)
                meta->addStatement(new GenOp("   @.xyz = -@;\r\n", outLightVec, inLightVec));
            else
                meta->addStatement(new GenOp("   @.xyz = @;\r\n", outLightVec, inLightVec));
        }
        else
        {
            outLightVec->setType("float4");
        }

        // Fix for backfacing specular
        Var* inNorm = (Var*)LangElement::find("normal");

        if (useneg)
            meta->addStatement(new GenOp("   @.w = step( -0.5, dot( -@, @ ) );", outLightVec, inLightVec, inNorm));
        else
            meta->addStatement(new GenOp("   @.w = step( -0.5, dot( @, @ ) );", outLightVec, inLightVec, inNorm));

    }



    output = meta;
}

//----------------------------------------------------------------------------
// Process pixel shader feature
//----------------------------------------------------------------------------
void PixelSpecular::processPix(Vector<ShaderComponent*>& componentList,
    GFXShaderFeatureData& fd)
{
    // grab normal
    Var* normal = NULL;

    if (fd.features[GFXShaderFeatureData::BumpMap])
    {
        normal = (Var*)LangElement::find("bumpNormal");
    }
    else
    {
        ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
        normal = connectComp->getElement(RT_TEXCOORD);
        normal->setName("normal");
        normal->setStructName("IN");
        normal->setType("float3");
    }

    // grab pixel position
    ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
    Var* pixPos = connectComp->getElement(RT_TEXCOORD);
    pixPos->setName("pixPos");
    pixPos->setStructName("IN");
    pixPos->setType("float3");

    // grab eye position
    Var* eyePos = connectComp->getElement(RT_TEXCOORD);
    eyePos->setName("eyePos");
    eyePos->setStructName("IN");
    eyePos->setType("float3");


    MultiLine* meta = new MultiLine;
    meta->addStatement(new GenOp("\r\n"));

    // create light vec
    Var* lightVec = NULL;


    // no lightNormMap? Use incoming light vec
    if (!fd.features[GFXShaderFeatureData::LightNormMap])
    {
        // grab vertex lightVec
        lightVec = (Var*)LangElement::find("lightVec");

        if (!lightVec)
        {
            ConnectorStruct* connectComp = dynamic_cast<ConnectorStruct*>(componentList[C_CONNECTOR]);
            lightVec = connectComp->getElement(RT_TEXCOORD);
            lightVec->setName("lightVec");
            lightVec->setStructName("IN");
            lightVec->setType("float4");
        }
        else
        {
            lightVec->setType("float4");
        }
    }
    else
    {
        lightVec = new Var;
        lightVec->setType("float3");
        lightVec->setName("lightVec");
        LangElement* lightVecDecl = new DecOp(lightVec);
        Var* lightMapNorm = (Var*)LangElement::find("lightmapNormal");
        meta->addStatement(new GenOp("   @ = @.xyz;\r\n", lightVecDecl, lightMapNorm));
    }


    // calc eye position
    Var* eyeVec = new Var;
    eyeVec->setType("float3");
    eyeVec->setName("eyeVec");

    LangElement* eyeVecDecl = new DecOp(eyeVec);
    LangElement* eyeVecDef = new GenOp("normalize(@ - @)", eyePos, pixPos);
    meta->addStatement(new GenOp("   @ = @;\r\n", eyeVecDecl, eyeVecDef));


    // calc half angle
    Var* halfAng = new Var;
    halfAng->setType("float3");
    halfAng->setName("halfAng");
    LangElement* halfAngDecl = new DecOp(halfAng);
    LangElement* halfAngDef = new GenOp("normalize(@ + @.xyz)", eyeVec, lightVec);

    meta->addStatement(new GenOp("   @ = @;\r\n", halfAngDecl, halfAngDef));


    // calc specular
    Var* specular = new Var;
    specular->setType("float");
    specular->setName("specular");
    LangElement* specDecl = new DecOp(specular);

    if (fd.features[GFXShaderFeatureData::LightNormMap])
    {
        if (fd.features[GFXShaderFeatureData::BumpMap])
        {
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, @) );\r\n", specDecl, normal, halfAng));
        }
        else
        {
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz, @) );\r\n", specDecl, normal, halfAng));
        }
    }
    else
    {
        if (fd.features[GFXShaderFeatureData::BumpMap])
        {
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz * 2.0 - 1.0, @) ) * @.w;\r\n", specDecl, normal, halfAng, lightVec));
        }
        else
        {
            meta->addStatement(new GenOp("   @ = saturate( dot(@.xyz, @) ) * @.w;\r\n", specDecl, normal, halfAng, lightVec));
        }
    }


    Var* specCol = new Var;
    specCol->setType("float4");
    specCol->setName("specularColor");
    specCol->uniform = true;
    specCol->constNum = PC_MAT_SPECCOLOR;

    Var* specPow = new Var;
    specPow->setType("float");
    specPow->setName("specularPower");
    specPow->uniform = true;
    specPow->constNum = PC_MAT_SPECPOWER;

    meta->addStatement(new GenOp("   @ = pow(@, @);\r\n", specular, specular, specPow));


    LangElement* specMul = new GenOp("@ * @", specCol, specular);
    LangElement* final = specMul;

    // mask out with lightmap if present
    if (fd.features[GFXShaderFeatureData::LightMap])
    {
        LangElement* lmColor = NULL;

        // find lightmap color
        lmColor = LangElement::find("lmColor");

        if (!lmColor)
        {
            LangElement* lightMap = LangElement::find("lightMap");
            LangElement* lmCoord = LangElement::find("lmCoord");

            lmColor = new GenOp("tex2D(@, @)", lightMap, lmCoord);
        }

        final = new GenOp("@ * @", specMul, lmColor);
    }
    else if (fd.features[GFXShaderFeatureData::DynamicLight])
    {
        LangElement* attn = LangElement::find("attn");
        if (attn)
            final = new GenOp("@ * @", specMul, attn);
    }

    if (fd.features[GFXShaderFeatureData::BaseTex])
    {
        // find base texture color
        Var* baseColor = (Var*)LangElement::find("diffuseColor");
        final = new GenOp("@ * @.a", final, baseColor);
    }
    else
    {
        if (fd.features[GFXShaderFeatureData::BumpMap])
        {
            Var* bumpColor = (Var*)LangElement::find("bumpNormal");
            final = new GenOp("@ * @.a", final, bumpColor);
        }
    }

    //final = new GenOp("@ * @.w", final, eyePos);

    // add to color
    meta->addStatement(new GenOp("   @;\r\n", assignColor(final, true)));
    if ((fd.features[GFXShaderFeatureData::Visibility]) && (!fd.materialFeatures[GFXShaderFeatureData::Translucent]))
    {
        Var* color = (Var*)LangElement::find("col");
        if (color)
        {
            meta->addStatement(new GenOp("   @.a = 1.0;\r\n", color));
        }
    }

    output = meta;
}

//----------------------------------------------------------------------------
// Get resources
//----------------------------------------------------------------------------
ShaderFeature::Resources PixelSpecular::getResources(GFXShaderFeatureData& fd)
{
    Resources res;

    // Have to pass out the eye and pixel position
    res.numTexReg = 2;


    if (!fd.features[GFXShaderFeatureData::BumpMap])
    {
        // pass out the surface normal
        res.numTexReg++;
    }

    if (!fd.features[GFXShaderFeatureData::LightNormMap])
    {
        // pass out light position
        res.numTexReg++;
    }


    return res;
}
