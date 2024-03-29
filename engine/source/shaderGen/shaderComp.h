//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SHADERCOMP_H_
#define _SHADERCOMP_H_

#include "materials/miscShdrDat.h"
#include "core/stream.h"
#include "core/tVector.h"

struct Var;

//**************************************************************************
// Shader Component - these objects are the main logical breakdown of a
//    high level shader.  They represent the various data structures
//    and the main() procedure necessary to create a shader.
//**************************************************************************
class ShaderComponent
{
public:
    virtual ~ShaderComponent() = default;
    virtual void print(Stream& stream) {};
};


//**************************************************************************
// Connector Struct Component - used for incoming Vertex struct and also the
//    "connection" struct shared by the vertex and pixel shader
//**************************************************************************
class ConnectorStruct : public ShaderComponent
{
    enum Consts
    {
        NUM_TEX_REGS = 8,
    };

    enum Elements
    {
        POSITION = 0,
        NORMAL,
        COLOR,
        NUM_BASIC_ELEMS
    };

    Vector <Var*> mElementList;

    U32 mCurTexElem;
    U8 mName[32];



public:

    ConnectorStruct();
    virtual ~ConnectorStruct();

    Var* getElement(RegisterType type);
    void setName(char* newName);
    void reset();
    void sortVars();

    virtual void print(Stream& stream);


};


//**************************************************************************
// Vertex Main Definition
//**************************************************************************
class VertexMainDef : public ShaderComponent
{
public:

    virtual void print(Stream& stream);

};

//**************************************************************************
//**************************************************************************
class PixelMainDef : public ShaderComponent
{
public:

    virtual void print(Stream& stream);

};



#endif _SHADERCOMP_H_