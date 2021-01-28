//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleObject.h"
#include "console/typeValidators.h"
#include "console/simBase.h"
#include <stdarg.h>

void TypeValidator::consoleError(SimObject *object, const char *format, ...)
{
   char buffer[1024];
   va_list argptr;
   va_start(argptr, format);
   dVsprintf(buffer, sizeof(buffer), format, argptr);
   va_end(argptr);

   AbstractClassRep *rep = object->getClassRep();
   AbstractClassRep::Field &fld = rep->mFieldList[fieldIndex];
   const char *objectName = object->getName();
   if(!objectName)
      objectName = "unnamed";


   Con::warnf("%s - %s(%d) - invalid value for %s: %s",
      rep->getClassName(), objectName, object->getId(), fld.pFieldname, buffer);
}

void FRangeValidator::validateType(SimObject *object, void *typePtr)
{
	F32 *v = (F32 *) typePtr;
	if(*v < minV || *v > maxV)
	{
		consoleError(object, "Must be between %g and %g", minV, maxV);
		if(*v < minV)
			*v = minV;
		else if(*v > maxV)
			*v = maxV;
	}
}

void IRangeValidator::validateType(SimObject *object, void *typePtr)
{
	S32 *v = (S32 *) typePtr;
	if(*v < minV || *v > maxV)
	{
		consoleError(object, "Must be between %d and %d", minV, maxV);
		if(*v < minV)
			*v = minV;
		else if(*v > maxV)
			*v = maxV;
	}
}

void IRangeValidatorScaled::validateType(SimObject *object, void *typePtr)
{
	S32 *v = (S32 *) typePtr;
	*v /= factor;
	if(*v < minV || *v > maxV)
	{
		consoleError(object, "Scaled value must be between %d and %d", minV, maxV);
		if(*v < minV)
			*v = minV;
		else if(*v > maxV)
			*v = maxV;
	}
}
