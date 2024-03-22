//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "math/mMath.h"
#include "shaderGen.h"
#include "pixSpecular.h"
#include "bump.h"
#include "featureMgr.h"
#include "gfx/gfxShader.h"
#include "core/fileStream.h"
#include "gfx/gfxDevice.h"
#include "shaderGen/shaderGenManager.h"

#define GEN_NEW_SHADERS

//****************************************************************************
// ShaderGen
//****************************************************************************
ShaderGen gShaderGen;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ShaderGen::ShaderGen()
{
}

//----------------------------------------------------------------------------
// Generate shader
//----------------------------------------------------------------------------
void ShaderGen::generateShader(const GFXShaderFeatureData& featureData,
    char* vertFile,
    char* pixFile,
    F32* pixVersion,
    GFXVertexFlags flags)
{
    mFeatureData = featureData;
    mVertFlags = flags;

    uninit();
    init();

    FileStream fs;
    char vertShaderName[256];
    char pixShaderName[256];
    static U32 shaderNum = 0;

    const char* shaderPath = Con::getVariable("$Pref::Video::ShaderPath");
    if (dStrlen(shaderPath) == 0)
    {
        shaderPath = "shaders";
    }

    dSprintf(vertShaderName, sizeof(vertShaderName), "%s/shaderV%03d.hlsl", shaderPath, shaderNum);
    dSprintf(pixShaderName, sizeof(pixShaderName), "%s/shaderP%03d.hlsl", shaderPath, shaderNum);
   // Con::printf("Generating shader %s", vertShaderName);

    shaderNum++;


#ifdef GEN_NEW_SHADERS
    // create vertex shader
    //------------------------
    Stream* s = ShaderGenManager::openNewShaderStream(vertShaderName);

    AssertFatal(s != NULL, "Failed to open Shader Stream");

    processVertFeatures();
    printVertShader(*s);

    ShaderGenManager::closeNewShaderStream(s);


    ((ConnectorStruct*)mComponents[C_CONNECTOR])->reset();
    LangElement::deleteElements();


    // create pixel shader
    //------------------------
    s = ShaderGenManager::openNewShaderStream(pixShaderName);

    AssertFatal(s != NULL, "Failed to open Shader Stream");

    processPixFeatures();
    printPixShader(*s);

    ShaderGenManager::closeNewShaderStream(s);
#endif

    dStrcpy(vertFile, vertShaderName);
    dStrcpy(pixFile, pixShaderName);

    // this needs to change - need to optimize down to ps v.1.1
    *pixVersion = GFX->getPixelShaderVersion();

}

//----------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------
void ShaderGen::init()
{
    createComponents();
    initVertexDef();

}

//----------------------------------------------------------------------------
// Uninit
//----------------------------------------------------------------------------
void ShaderGen::uninit()
{
    for (U32 i = 0; i < mComponents.size(); i++)
    {
        delete mComponents[i];
        mComponents[i] = NULL;
    }
    mComponents.setSize(0);

    LangElement::deleteElements();

    Var::reset();
}


//----------------------------------------------------------------------------
// Create shader components
//----------------------------------------------------------------------------
void ShaderGen::createComponents()
{

    // Make sure this order matches the "Components" enum!!
    ConnectorStruct* vertComp = new ConnectorStruct;
    mComponents.push_back(vertComp);
    vertComp->setName("VertData");



    // Set up connector structure
    mComponents.push_back(new ConnectorStruct);
    ((ConnectorStruct*)mComponents[1])->setName("ConnectData");

    mComponents.push_back(new VertexMainDef);
    mComponents.push_back(new PixelMainDef);
}

//----------------------------------------------------------------------------
// Allocate vertex variables
//----------------------------------------------------------------------------
void ShaderGen::initVertexDef()
{
    // Grab tex coord 0 as default for base texture - also used
    //    for bumpmap and detail map coords
    ConnectorStruct* vertComp = dynamic_cast<ConnectorStruct*>(mComponents[C_VERT_STRUCT]);

    Var* inTex = vertComp->getElement(RT_TEXCOORD);
    inTex->setName("texCoord");
    inTex->setStructName("IN");
    inTex->setType("float2");

    // set up lightmap coords
    Var* lmCoord = vertComp->getElement(RT_TEXCOORD);
    lmCoord->setName("lmCoord");
    lmCoord->setStructName("IN");
    lmCoord->setType("float2");

    // grab incoming tex space matrix
    Var* T = vertComp->getElement(RT_TEXCOORD);
    T->setName("T");
    T->setStructName("IN");
    T->setType("float3");

    // grab incoming tex space matrix
    Var* B = vertComp->getElement(RT_TEXCOORD);
    B->setName("B");
    B->setStructName("IN");
    B->setType("float3");

    // grab incoming tex space matrix
    Var* N = vertComp->getElement(RT_TEXCOORD);
    N->setName("N");
    N->setStructName("IN");
    N->setType("float3");

    // handle normal
    if (mVertFlags & GFXVertexFlagNormal)
    {
        Var* inNormal = vertComp->getElement(RT_NORMAL);
        inNormal->setName("normal");
        inNormal->setStructName("IN");
        inNormal->setType("float3");
    }



}

//----------------------------------------------------------------------------
// Process features
//----------------------------------------------------------------------------
void ShaderGen::processVertFeatures()
{
    // process auxiliary features
    for (U32 i = 0; i < FeatureMgr::NumAuxFeatures; i++)
    {
        gFeatureMgr.getAux(i)->processVert(mComponents, mFeatureData);
    }

    // process the other features
    for (U32 i = 0; i < GFXShaderFeatureData::NumFeatures; i++)
    {
        if (mFeatureData.features[i])
        {
            gFeatureMgr.get(i)->processVert(mComponents, mFeatureData);
        }
    }

    ConnectorStruct* connect = dynamic_cast<ConnectorStruct*>(mComponents[C_CONNECTOR]);
    connect->sortVars();
}

//----------------------------------------------------------------------------
// Process features
//----------------------------------------------------------------------------
void ShaderGen::processPixFeatures()
{
    // process auxiliary features
    for (U32 i = 0; i < FeatureMgr::NumAuxFeatures; i++)
    {
        gFeatureMgr.getAux(i)->processPix(mComponents, mFeatureData);
    }

    // process the other features
    for (U32 i = 0; i < GFXShaderFeatureData::NumFeatures; i++)
    {
        if (mFeatureData.features[i])
        {
            gFeatureMgr.get(i)->processPix(mComponents, mFeatureData);
        }
    }

    ConnectorStruct* connect = dynamic_cast<ConnectorStruct*>(mComponents[C_CONNECTOR]);
    connect->sortVars();
}

//----------------------------------------------------------------------------
// Print vertex OR pixel shader features
//----------------------------------------------------------------------------
void ShaderGen::printFeatures(Stream& stream)
{
    // process auxiliary features
    for (U32 i = 0; i < FeatureMgr::NumAuxFeatures; i++)
    {
        LangElement* le = gFeatureMgr.getAux(i)->getOutput();
        if (le)
        {
            le->print(stream);
        }
    }

    // process the other features
    for (U32 i = 0; i < GFXShaderFeatureData::NumFeatures; i++)
    {
        if (mFeatureData.features[i])
        {
            LangElement* le = gFeatureMgr.get(i)->getOutput();
            if (le)
            {
                le->print(stream);
            }

        }
    }
}

//----------------------------------------------------------------------------
// Print header
//----------------------------------------------------------------------------
void ShaderGen::printShaderHeader(Stream& stream)
{

    const char* header1 = "//*****************************************************************************\r\n";
    const char* header2 = "// TSE -- HLSL procedural shader                                               \r\n";

    stream.write(dStrlen(header1), header1);
    stream.write(dStrlen(header2), header2);
    stream.write(dStrlen(header1), header1);


    const char* header3 = "//-----------------------------------------------------------------------------\r\n";
    const char* header4 = "// Structures                                                                  \r\n";

    stream.write(dStrlen(header3), header3);
    stream.write(dStrlen(header4), header4);
    stream.write(dStrlen(header3), header3);
}

//----------------------------------------------------------------------------
// Print components to stream
//----------------------------------------------------------------------------
void ShaderGen::printVertShader(Stream& stream)
{
    printShaderHeader(stream);

    // print out structures
    mComponents[C_VERT_STRUCT]->print(stream);
    mComponents[C_CONNECTOR]->print(stream);


    // Print out main function definition
    const char* header5 = "// Main                                                                        \r\n";
    const char* line = "//-----------------------------------------------------------------------------\r\n";

    stream.write(dStrlen(line), line);
    stream.write(dStrlen(header5), header5);
    stream.write(dStrlen(line), line);

    mComponents[C_VERT_MAIN]->print(stream);


    // print out the function
    printFeatures(stream);

    const char* closer = "   return OUT;\r\n}\r\n";
    stream.write(dStrlen(closer), closer);

}

//----------------------------------------------------------------------------
// Print components to stream
//----------------------------------------------------------------------------
void ShaderGen::printPixShader(Stream& stream)
{
    printShaderHeader(stream);

    mComponents[C_CONNECTOR]->print(stream);

    WRITESTR("struct Fragout\r\n");
    WRITESTR("{\r\n");
    WRITESTR("   float4 col : COLOR0;\r\n");
    WRITESTR("};\r\n");
    WRITESTR("\r\n");
    WRITESTR("\r\n");


    // Print out main function definition
    const char* header5 = "// Main                                                                        \r\n";
    const char* line = "//-----------------------------------------------------------------------------\r\n";

    WRITESTR(line);
    WRITESTR(header5);
    WRITESTR(line);

    mComponents[C_PIX_MAIN]->print(stream);

    // print out the function
    printFeatures(stream);


    WRITESTR("\r\n   return OUT;\r\n}\r\n");

}
