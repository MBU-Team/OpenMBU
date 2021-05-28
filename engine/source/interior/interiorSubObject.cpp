//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "interior/interiorInstance.h"

#include "interior/interiorSubObject.h"
#include "interior/mirrorSubObject.h"


InteriorSubObject::InteriorSubObject()
{
   mInteriorInstance = NULL;
}

InteriorSubObject::~InteriorSubObject()
{
   mInteriorInstance = NULL;
}

InteriorSubObject* InteriorSubObject::readISO(Stream& stream)
{
   U32 soKey;
   stream.read(&soKey);

   InteriorSubObject* pObject = NULL;
   switch (soKey) {
     case MirrorSubObjectKey:
      pObject = new MirrorSubObject;
      break;

     default:
      Con::errorf(ConsoleLogEntry::General, "Bad key in subObject stream!");
      return NULL;
   };

   if (pObject) {
      bool readSuccess = pObject->_readISO(stream);
      if (readSuccess == false) {
         delete pObject;
         pObject = NULL;
      }
   }

   return pObject;
}

bool InteriorSubObject::writeISO(Stream& stream) const
{
   stream.write(getSubObjectKey());
   return _writeISO(stream);
}

bool InteriorSubObject::_readISO(Stream& stream)
{
   return (stream.getStatus() == Stream::Ok);
}

bool InteriorSubObject::_writeISO(Stream& stream) const
{
   return (stream.getStatus() == Stream::Ok);
}

const MatrixF& InteriorSubObject::getSOTransform() const
{
   static const MatrixF csBadMatrix(true);

   if (mInteriorInstance != NULL) {
      return mInteriorInstance->getTransform();
   } else {
      AssertWarn(false, "Returning bad transform for subobject");
      return csBadMatrix;
   }
}

const Point3F& InteriorSubObject::getSOScale() const
{
   return mInteriorInstance->getScale();
}

InteriorInstance* InteriorSubObject::getInstance()
{
   return mInteriorInstance;
}

void InteriorSubObject::noteTransformChange()
{
   //
}
