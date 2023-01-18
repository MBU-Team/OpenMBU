//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "shaderOp.h"

#ifdef _XBOX
#  include "platformXbox/platformXbox.h"
#elif defined(TORQUE_OS_WIN)
#  include "platformWin32/platformWin32.h"
#else
#include <cstdarg>
#endif

//**************************************************************************
// Shader Operations
//**************************************************************************
ShaderOp::ShaderOp(LangElement* in1, LangElement* in2)
{
    mInput[0] = in1;
    mInput[1] = in2;
}

//**************************************************************************
// Declaration Operation - for variables
//**************************************************************************
DecOp::DecOp(Var* in1) : Parent(in1, NULL)
{
    mInput[0] = in1;
}

//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void DecOp::print(Stream& stream)
{
    Var* var = dynamic_cast<Var*>(mInput[0]);

    WRITESTR((char*)var->type);
    WRITESTR(" ");

    mInput[0]->print(stream);
}

//**************************************************************************
// Echo operation - deletes incoming statement!
//**************************************************************************
EchoOp::EchoOp(const char* statement) : Parent(NULL, NULL)
{
    mStatement = statement;
}

//--------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
EchoOp::~EchoOp()
{
    delete (void*)mStatement;
}

//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void EchoOp::print(Stream& stream)
{
    WRITESTR(mStatement);
}


//**************************************************************************
// General operation
//**************************************************************************
GenOp::GenOp(const char* statement, ...) : Parent(NULL, NULL)
{
    va_list args;
    va_start(args, statement);

    char* lastEntry = (char*)statement;

    while (1)
    {
        // search 'statement' for @ symbol
        char* str = dStrstr(lastEntry, (char*)"@");

        if (!str)
        {
            // not found, handle end of line
            str = (char*)&statement[dStrlen((char*)statement)];

            U32 diff = str - lastEntry + 1;
            if (diff == 1) break;

            char* newStr = new char[diff];

            dMemcpy((void*)newStr, lastEntry, diff);

            mElemList.push_back(new EchoOp(newStr));


            break;
        }

        // create and store statement fragment
        U32 diff = str - lastEntry + 1;

        if (diff == 1)
        {
            // store langElement
            LangElement* elem = va_arg(args, LangElement*);
            AssertFatal(elem, "NULL arguement.");
            mElemList.push_back(elem);
            lastEntry++;
            continue;
        }

        char* newStr = new char[diff];

        dMemcpy((void*)newStr, lastEntry, diff);
        newStr[diff - 1] = '\0';

        lastEntry = str + 1;

        mElemList.push_back(new EchoOp(newStr));

        // store langElement
        LangElement* elem = va_arg(args, LangElement*);
        AssertFatal(elem, "NULL arguement.");
        mElemList.push_back(elem);

    }



    va_end(args);

}

//--------------------------------------------------------------------------
// Print
//--------------------------------------------------------------------------
void GenOp::print(Stream& stream)
{
    for (U32 i = 0; i < mElemList.size(); i++)
    {
        mElemList[i]->print(stream);
    }

}
