//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/stringTable.h"
#include "core/color.h"
#include "console/simBase.h"

//////////////////////////////////////////////////////////////////////////
// TypeString
//////////////////////////////////////////////////////////////////////////
ConsoleType( string, TypeString, sizeof(const char*) )

ConsoleGetType( TypeString )
{
   return *((const char **)(dptr));
}

ConsoleSetType( TypeString )
{
   if(argc == 1)
      *((const char **) dptr) = StringTable->insert(argv[0]);
   else
      Con::printf("(TypeString) Cannot set multiple args to a single string.");
}

//////////////////////////////////////////////////////////////////////////
// TypeCaseString
//////////////////////////////////////////////////////////////////////////
ConsoleType( caseString, TypeCaseString, sizeof(const char*) )

ConsoleSetType( TypeCaseString )
{
   if(argc == 1)
      *((const char **) dptr) = StringTable->insert(argv[0], true);
   else
      Con::printf("(TypeCaseString) Cannot set multiple args to a single string.");
}

ConsoleGetType( TypeCaseString )
{
   return *((const char **)(dptr));
}

//////////////////////////////////////////////////////////////////////////
// TypeFileName
//////////////////////////////////////////////////////////////////////////
ConsoleType( filename, TypeFilename, sizeof( const char* ) )

ConsoleSetType( TypeFilename )
{
   if(argc == 1)
   {
      char buffer[1024];
      if (Con::expandScriptFilename(buffer, 1024, argv[0]))
         *((const char **) dptr) = StringTable->insert(buffer);
      else
         Con::warnf("(TypeFilename) illegal filename detected: %s", argv[0]);
   }
   else
      Con::printf("(TypeFilename) Cannot set multiple args to a single filename.");
}

ConsoleGetType( TypeFilename )
{
   return *((const char **)(dptr));
}

//////////////////////////////////////////////////////////////////////////
// TypeS8
//////////////////////////////////////////////////////////////////////////
ConsoleType( char, TypeS8, sizeof(U8) )

ConsoleGetType( TypeS8 )
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d", *((U8 *) dptr) );
   return returnBuffer;
}

ConsoleSetType( TypeS8 )
{
   if(argc == 1)
      *((U8 *) dptr) = dAtoi(argv[0]);
   else
      Con::printf("(TypeU8) Cannot set multiple args to a single S32.");
}

//////////////////////////////////////////////////////////////////////////
// TypeS32
//////////////////////////////////////////////////////////////////////////
ConsoleType( int, TypeS32, sizeof(S32) )

ConsoleGetType( TypeS32 )
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d", *((S32 *) dptr) );
   return returnBuffer;
}

ConsoleSetType( TypeS32 )
{
   if(argc == 1)
      *((S32 *) dptr) = dAtoi(argv[0]);
   else
      Con::printf("(TypeS32) Cannot set multiple args to a single S32.");
}

//////////////////////////////////////////////////////////////////////////
// TypeS32Vector
//////////////////////////////////////////////////////////////////////////
ConsoleType( intList, TypeS32Vector, sizeof(Vector<S32>) )

ConsoleGetType( TypeS32Vector )
{
   Vector<S32> *vec = (Vector<S32> *)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<S32>::iterator itr = vec->begin(); itr != vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%d ", *itr);
      // update the length of the return string (so far)
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return returnBuffer;
}

ConsoleSetType( TypeS32Vector )
{
   Vector<S32> *vec = (Vector<S32> *)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if(argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      S32 value;
      // advance through the string, pulling off S32's and advancing the pointer
      while (values < endValues && dSscanf(values, "%d", &value) != 0)
      {
         vec->push_back(value);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtoi(argv[i]));
   }
   else
      Con::printf("Vector<S32> must be set as { a, b, c, ... } or \"a b c ...\"");
}

//////////////////////////////////////////////////////////////////////////
// TypeF32
//////////////////////////////////////////////////////////////////////////
ConsoleType( float, TypeF32, sizeof(F32) )

ConsoleGetType( TypeF32 )
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g", *((F32 *) dptr) );
   return returnBuffer;
}
ConsoleSetType( TypeF32 )
{
   if(argc == 1)
      *((F32 *) dptr) = dAtof(argv[0]);
   else
      Con::printf("(TypeF32) Cannot set multiple args to a single F32.");
}

//////////////////////////////////////////////////////////////////////////
// TypeF32Vector
//////////////////////////////////////////////////////////////////////////
ConsoleType( floatList, TypeF32Vector, sizeof(Vector<F32>) )

ConsoleGetType( TypeF32Vector )
{
   Vector<F32> *vec = (Vector<F32> *)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<F32>::iterator itr = vec->begin(); itr != vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%g ", *itr);
      // update the length of the return string (so far)
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return returnBuffer;
}

ConsoleSetType( TypeF32Vector )
{
   Vector<F32> *vec = (Vector<F32> *)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if(argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      F32 value;
      // advance through the string, pulling off F32's and advancing the pointer
      while (values < endValues && dSscanf(values, "%g", &value) != 0)
      {
         vec->push_back(value);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtof(argv[i]));
   }
   else
      Con::printf("Vector<F32> must be set as { a, b, c, ... } or \"a b c ...\"");
}

//////////////////////////////////////////////////////////////////////////
// TypeBool
//////////////////////////////////////////////////////////////////////////
ConsoleType( bool, TypeBool, sizeof(bool) )

ConsoleGetType( TypeBool )
{
   return *((bool *) dptr) ? "1" : "0";
}

ConsoleSetType( TypeBool )
{
   if(argc == 1)
      *((bool *) dptr) = dAtob(argv[0]);
   else
      Con::printf("(TypeBool) Cannot set multiple args to a single bool.");
}


//////////////////////////////////////////////////////////////////////////
// TypeBoolVector
//////////////////////////////////////////////////////////////////////////
ConsoleType( boolList, TypeBoolVector, sizeof(Vector<bool>) )

ConsoleGetType( TypeBoolVector )
{
   Vector<bool> *vec = (Vector<bool>*)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<bool>::iterator itr = vec->begin(); itr < vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%d ", (*itr == true ? 1 : 0));
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return(returnBuffer);
}

ConsoleSetType( TypeBoolVector )
{
   Vector<bool> *vec = (Vector<bool>*)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if (argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      S32 value;
      // advance through the string, pulling off bool's and advancing the pointer
      while (values < endValues && dSscanf(values, "%d", &value) != 0)
      {
         vec->push_back(value == 0 ? false : true);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtob(argv[i]));
   }
   else
      Con::printf("Vector<bool> must be set as { a, b, c, ... } or \"a b c ...\"");
}

//////////////////////////////////////////////////////////////////////////
// TypeEnum
//////////////////////////////////////////////////////////////////////////
ConsoleType( enumval, TypeEnum, sizeof(S32) )

ConsoleGetType( TypeEnum )
{
   AssertFatal(tbl, "Null enum table passed to getDataTypeEnum()");
   S32 dptrVal = *(S32*)dptr;
   for (S32 i = 0; i < tbl->size; i++)
   {
      if (dptrVal == tbl->table[i].index)
      {
         return tbl->table[i].label;
      }
   }

   //not found
   return "";
}

ConsoleSetType( TypeEnum )
{
   AssertFatal(tbl, "Null enum table passed to setDataTypeEnum()");
   if (argc != 1) return;

   S32 val = 0;
   for (S32 i = 0; i < tbl->size; i++)
   {
      if (! dStricmp(argv[0], tbl->table[i].label))
      {
         val = tbl->table[i].index;
         break;
      }
   }
   *((S32 *) dptr) = val;
}

//////////////////////////////////////////////////////////////////////////
// TypeFlag
//////////////////////////////////////////////////////////////////////////
ConsoleType( flag, TypeFlag, sizeof(S32) )

ConsoleGetType( TypeFlag )
{
   BitSet32 tempFlags = *(BitSet32 *)dptr;
   if (tempFlags.test(flag)) return "true";
   else return "false";
}

ConsoleSetType( TypeFlag )
{
   bool value = true;
   if (argc != 1)
   {
      Con::printf("flag must be true or false");
   }
   else
   {
      value = dAtob(argv[0]);
   }
   ((BitSet32 *)dptr)->set(flag, value);
}

//////////////////////////////////////////////////////////////////////////
// TypeColorF
//////////////////////////////////////////////////////////////////////////
ConsoleType( ColorF, TypeColorF, sizeof(ColorF) )

ConsoleGetType( TypeColorF )
{
   ColorF * color = (ColorF*)dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g %g %g %g", color->red, color->green, color->blue, color->alpha);
   return(returnBuffer);
}

ConsoleSetType( TypeColorF )
{
   ColorF *tmpColor = (ColorF *) dptr;
   if(argc == 1)
   {
      tmpColor->set(0, 0, 0, 1);
      F32 r,g,b,a;
      S32 args = dSscanf(argv[0], "%g %g %g %g", &r, &g, &b, &a);
      tmpColor->red = r;
      tmpColor->green = g;
      tmpColor->blue = b;
      if (args == 4)
         tmpColor->alpha = a;
   }
   else if(argc == 3)
   {
      tmpColor->red    = dAtof(argv[0]);
      tmpColor->green  = dAtof(argv[1]);
      tmpColor->blue   = dAtof(argv[2]);
      tmpColor->alpha  = 1.f;
   }
   else if(argc == 4)
   {
      tmpColor->red    = dAtof(argv[0]);
      tmpColor->green  = dAtof(argv[1]);
      tmpColor->blue   = dAtof(argv[2]);
      tmpColor->alpha  = dAtof(argv[3]);
   }
   else
      Con::printf("Color must be set as { r, g, b [,a] }");
}

//////////////////////////////////////////////////////////////////////////
// TypeColorI
//////////////////////////////////////////////////////////////////////////
ConsoleType( ColorI, TypeColorI, sizeof(ColorI) )

ConsoleGetType( TypeColorI )
{
   ColorI *color = (ColorI *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d %d %d %d", color->red, color->green, color->blue, color->alpha);
   return returnBuffer;
}

ConsoleSetType( TypeColorI )
{
   ColorI *tmpColor = (ColorI *) dptr;
   if(argc == 1)
   {
      tmpColor->set(0, 0, 0, 255);
      S32 r,g,b,a;
      S32 args = dSscanf(argv[0], "%d %d %d %d", &r, &g, &b, &a);
      tmpColor->red = r;
      tmpColor->green = g;
      tmpColor->blue = b;
      if (args == 4)
         tmpColor->alpha = a;
   }
   else if(argc == 3)
   {
      tmpColor->red    = dAtoi(argv[0]);
      tmpColor->green  = dAtoi(argv[1]);
      tmpColor->blue   = dAtoi(argv[2]);
      tmpColor->alpha  = 255;
   }
   else if(argc == 4)
   {
      tmpColor->red    = dAtoi(argv[0]);
      tmpColor->green  = dAtoi(argv[1]);
      tmpColor->blue   = dAtoi(argv[2]);
      tmpColor->alpha  = dAtoi(argv[3]);
   }
   else
      Con::printf("Color must be set as { r, g, b [,a] }");
}

//////////////////////////////////////////////////////////////////////////
// TypeSimObjectPtr
//////////////////////////////////////////////////////////////////////////
ConsoleType( SimObjectPtr, TypeSimObjectPtr, sizeof(SimObject*) )

ConsoleSetType( TypeSimObjectPtr )
{
   if(argc == 1)
   {
      SimObject **obj = (SimObject **)dptr;
      *obj = Sim::findObject(argv[0]);
   }
   else
      Con::printf("(TypeSimObjectPtr) Cannot set multiple args to a single S32.");
}

ConsoleGetType( TypeSimObjectPtr )
{
   SimObject **obj = (SimObject**)dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%s", *obj ? (*obj)->getName() : "");
   return returnBuffer;
}
