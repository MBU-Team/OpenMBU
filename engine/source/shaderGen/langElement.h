//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _LANG_ELEMENT_H_
#define _LANG_ELEMENT_H_

#include "platform/platform.h"
#include "core/stream.h"
#include "core/tVector.h"

#define WRITESTR( a ){ stream.write( dStrlen(a), a ); }


//**************************************************************************
/*!
   The LangElement class is the base building block for procedurally
   generated shader code.  LangElement and its subclasses are strung
   together using the static Vector 'elementList'.
   When a shader needs to be written to disk, the elementList is
   traversed and print() is called on each LangElement and the shader
   is output.  elementList is cleared after each shader is printed out.
*/
//**************************************************************************


//**************************************************************************
// Language element
//**************************************************************************
struct LangElement
{
    static Vector<LangElement*> elementList;
    static LangElement* find(StringTableEntry name);
    static void deleteElements();


    U8    name[32];

    LangElement();
    virtual ~LangElement() = default;
    virtual void print(Stream& stream) {};
    void setName(char* newName);

};

//----------------------------------------------------------------------------
/*!
   Var - Variable - used to specify a variable to be used in a shader.
   Var stores information such  that when it is printed out, its context
   can be identified and the proper information will automatically be printed.
   For instance, if a variable is created with 'uniform' set to true, when the
   shader function definition is printed, it will automatically add that
   variable to the incoming parameters of the shader.  There are several
   similar cases such as when a new variable is declared within a shader.

   example:

   @code

   Var *modelview = new Var;
   modelview->setType( "float4x4" );
   modelview->setName( "modelview" );
   modelview->uniform = true;
   modelview->constNum = VC_WORLD_PROJ;

   @endcode

   it prints out in the shader declaration as:

   @code
      ConnectData main( VertData IN,
                        uniform float4x4 modelview : register(C0) )
   @endcode

*/
//----------------------------------------------------------------------------
struct Var : public LangElement
{
    U8    type[32];
    U8    structName[32];
    char  connectName[32];
    U32   constNum;
    U32   texCoordNum;
    bool  uniform;       // argument passed in through constant registers
    bool  vertData;      // argument coming from vertex data
    bool  connector;     // variable that will be passed to pixel shader
    bool  sampler;       // texture
    bool  mapsToSampler; // for ps 1.x shaders - texcoords must be mapped to same sampler stage

    static U32  texUnitCount;
    static U32  getTexUnitNum();
    static void reset();

    Var();

    void setStructName(char* newName);
    void setConnectName(char* newName);
    void setType(char* newType);

    virtual void print(Stream& stream);

};

//----------------------------------------------------------------------------
/*!
   MultiLine - Multi Line Statement - This class simply ties multiple

   example:

   @code

   MultiLine *meta = new MultiLine;
   meta->addStatement( new GenOp( "foo = true;\r\n" ) );
   meta->addStatement( new GenOp( "bar = false;\r\n ) );

   @endcode

   it prints out in the shader declaration as:

   @code
      foo = true;
      bar = false;
   @endcode

*/
//----------------------------------------------------------------------------
class MultiLine : public LangElement
{
    Vector <LangElement*> mStatementList;

public:

    void addStatement(LangElement* elem);
    virtual void print(Stream& stream);
};



#endif _LANG_ELEMENT_H_
