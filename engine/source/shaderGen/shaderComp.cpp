//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "shaderComp.h"
#include "langElement.h"
#include "core/tVector.h"
#include "gfx/gfxDevice.h"

//**************************************************************************
// Connector Struct Component
//**************************************************************************

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ConnectorStruct::ConnectorStruct()
{
    mCurTexElem = 0;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
ConnectorStruct::~ConnectorStruct()
{
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        if (mElementList[i] != NULL)
        {
            // delete mElementList[i];
            mElementList[i] = NULL;
        }
    }
}

//----------------------------------------------------------------------------
// Get Vertex element
//----------------------------------------------------------------------------
Var* ConnectorStruct::getElement(RegisterType type)
{
    switch (type)
    {
    case RT_POSITION:
    {
        Var* newVar = new Var;
        mElementList.push_back(newVar);
        newVar->setConnectName("POSITION");
        return newVar;
    }

    case RT_NORMAL:
    {
        Var* newVar = new Var;
        mElementList.push_back(newVar);
        newVar->setConnectName("NORMAL");
        return newVar;
    }

    case RT_COLOR:
    {
        Var* newVar = new Var;
        mElementList.push_back(newVar);
        newVar->setConnectName("COLOR");
        return newVar;
    }

    case RT_TEXCOORD:
    {
        Var* newVar = new Var;
        mElementList.push_back(newVar);

        char out[32];
        dSprintf((char*)out, sizeof(out), "TEXCOORD%d", mCurTexElem);
        newVar->setConnectName(out);
        newVar->constNum = mCurTexElem;
        mCurTexElem++;
        return newVar;
    }

    default:
        break;
    }



    return NULL;
}

//----------------------------------------------------------------------------
// Sort connector variables - They must be sorted on hardware that is running
// ps 1.4 and below.  The reason is that texture coordinate registers MUST
// map exactly to their respective texture stage.  Ie.  if you have fog
// coordinates being passed into a pixel shader in texture coordinate register
// number 4, the fog texture MUST reside in texture stage 4 for it to work.
// The problem is solved by pushing non-texture coordinate data to the end
// of the structure so that the texture coodinates are all at the "top" of the
// structure in the order that the features are processed.
//----------------------------------------------------------------------------
void ConnectorStruct::sortVars()
{
    if (GFX->getPixelShaderVersion() >= 2.0) return;

    // create list of just the texCoords, sorting by 'mapsToSampler'
    Vector< Var* > texCoordList;

    // - first pass is just coords mapped to a sampler
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        Var* var = mElementList[i];
        if (var->mapsToSampler)
        {
            texCoordList.push_back(var);
        }
    }

    // - next pass is for the others
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        Var* var = mElementList[i];
        if (dStrstr((const char*)var->connectName, "TEX") &&
            !var->mapsToSampler)
        {
            texCoordList.push_back(var);
        }
    }

    // rename the connectNames
    for (U32 i = 0; i < texCoordList.size(); i++)
    {
        char out[32];
        dSprintf((char*)out, sizeof(out), "TEXCOORD%d", i);
        texCoordList[i]->setConnectName(out);
    }

    // write new, sorted list over old one
    if (texCoordList.size())
    {
        U32 index = 0;

        for (U32 i = 0; i < mElementList.size(); i++)
        {
            Var* var = mElementList[i];
            if (dStrstr((const char*)var->connectName, "TEX"))
            {
                mElementList[i] = texCoordList[index];
                index++;
            }
        }
    }




}

//----------------------------------------------------------------------------
// Set name of structure
//----------------------------------------------------------------------------
void ConnectorStruct::setName(char* newName)
{
    dStrcpy((char*)mName, newName);
}

//--------------------------------------------------------------------------
// Reset
//--------------------------------------------------------------------------
void ConnectorStruct::reset()
{
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        mElementList[i] = NULL;
    }

    mElementList.setSize(0);
    mCurTexElem = 0;
}

//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void ConnectorStruct::print(Stream& stream)
{
    const char* header = "struct ";
    const char* header2 = "\r\n{\r\n";
    const char* footer = "};\r\n\r\n\r\n";
    const char* tab = "   ";


    stream.write(dStrlen(header), header);
    stream.write(dStrlen((char*)mName), mName);
    stream.write(dStrlen(header2), header2);


    // print out elements
    for (U32 i = 0; i < mElementList.size(); i++)
    {
        U8 output[256];

        Var* var = mElementList[i];
        dSprintf((char*)output, sizeof(output), "   %s %-15s : %s;\r\n", var->type, var->name, var->connectName);

        stream.write(dStrlen((char*)output), output);
    }

    stream.write(dStrlen(footer), footer);


}


//**************************************************************************
// Vertex Main Definition
//**************************************************************************


//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void VertexMainDef::print(Stream& stream)
{
    const char* opener = "ConnectData main( VertData IN";
    stream.write(dStrlen(opener), opener);

    // find all the uniform variables and print them out
    for (U32 i = 0; i < LangElement::elementList.size(); i++)
    {
        Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
        if (var)
        {
            if (var->uniform)
            {
                const char* nextVar = ",\r\n                  ";
                stream.write(dStrlen(nextVar), nextVar);


                U8 varNum[64];
                dSprintf((char*)varNum, sizeof(varNum), "register(C%d)", var->constNum);

                U8 output[256];
                dSprintf((char*)output, sizeof(output), "uniform %-8s %-15s : %s", var->type, var->name, varNum);

                stream.write(dStrlen((char*)output), output);
            }
        }
    }

    const char* closer = "\r\n)\r\n{\r\n   ConnectData OUT;\r\n\r\n";
    stream.write(dStrlen(closer), closer);


}


//**************************************************************************
// Pixel Main Definition
//**************************************************************************


//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void PixelMainDef::print(Stream& stream)
{
    const char* opener = "Fragout main( ConnectData IN";
    stream.write(dStrlen(opener), opener);

    // find all the sampler & uniform variables and print them out
    for (U32 i = 0; i < LangElement::elementList.size(); i++)
    {
        Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
        if (var)
        {
            if (var->uniform)
            {
                WRITESTR(",\r\n              ");

                U8 varNum[32];

                if (var->sampler)
                {
                    dSprintf((char*)varNum, sizeof(varNum), "register(S%d)", var->constNum);
                }
                else
                {
                    dSprintf((char*)varNum, sizeof(varNum), "register(C%d)", var->constNum);
                }

                U8 output[256];
                dSprintf((char*)output, sizeof(output), "uniform %-9s %-15s : %s", var->type, var->name, varNum);

                WRITESTR((char*)output);
            }
        }
    }

    const char* closer = "\r\n)\r\n{\r\n   Fragout OUT;\r\n\r\n";
    stream.write(dStrlen(closer), closer);


}
