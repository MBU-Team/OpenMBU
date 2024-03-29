//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "langElement.h"
#include "core/tVector.h"

//**************************************************************************
// Language element
//**************************************************************************
Vector<LangElement*> LangElement::elementList;

//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
LangElement::LangElement()
{
    elementList.push_back(this);

    static U32 tempNum = 0;
    dSprintf((char*)name, sizeof(name), "tempName%d", tempNum++);
}

//--------------------------------------------------------------------------
// Find element of specified name
//--------------------------------------------------------------------------
LangElement* LangElement::find(StringTableEntry name)
{
    for (U32 i = 0; i < elementList.size(); i++)
    {
        if (!dStrcmp((char*)elementList[i]->name, name))
        {
            return elementList[i];
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------
// Delete existing elements
//--------------------------------------------------------------------------
void LangElement::deleteElements()
{
    for (U32 i = 0; i < elementList.size(); i++)
    {
        if (elementList[i] != NULL) {
            delete elementList[i];
            elementList[i] = NULL;
        }
    }

    elementList.setSize(0);

}

//--------------------------------------------------------------------------
// Set name
//--------------------------------------------------------------------------
void LangElement::setName(char* newName)
{
    dStrcpy((char*)name, newName);
}

//**************************************************************************
// Variable
//**************************************************************************
U32 Var::texUnitCount = 0;

Var::Var()
{
    dStrcpy((char*)type, "float4");
    structName[0] = '\0';
    uniform = false;
    vertData = false;
    connector = false;
    sampler = false;
    mapsToSampler = false;
    texCoordNum = 0;
}


//--------------------------------------------------------------------------
// Set struct name
//--------------------------------------------------------------------------
void Var::setStructName(char* newName)
{
    dStrcpy((char*)structName, newName);
}

//--------------------------------------------------------------------------
// Set connect name
//--------------------------------------------------------------------------
void Var::setConnectName(char* newName)
{
    dStrcpy((char*)connectName, newName);
}

//--------------------------------------------------------------------------
// Set type
//--------------------------------------------------------------------------
void Var::setType(char* newType)
{
    dStrcpy((char*)type, newType);
}

//--------------------------------------------------------------------------
// print
//--------------------------------------------------------------------------
void Var::print(Stream& stream)
{
    if (structName[0] != '\0')
    {
        stream.write(dStrlen((char*)structName), structName);
        stream.write(1, ".");
    }



    stream.write(dStrlen((char*)name), name);
}

//--------------------------------------------------------------------------
// Get next available texture unit number
//--------------------------------------------------------------------------
U32 Var::getTexUnitNum()
{
    return texUnitCount++;
}

//--------------------------------------------------------------------------
// Reset
//--------------------------------------------------------------------------
void Var::reset()
{
    texUnitCount = 0;
}

//**************************************************************************
// Multi line statement
//**************************************************************************
void MultiLine::addStatement(LangElement* elem)
{
    AssertFatal(elem, "Attempting to add empty statement");

    mStatementList.push_back(elem);
}

//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void MultiLine::print(Stream& stream)
{
    for (U32 i = 0; i < mStatementList.size(); i++)
    {
        mStatementList[i]->print(stream);
    }

}


