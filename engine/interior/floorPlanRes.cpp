//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "interior/floorPlanRes.h"
#include "math/mathIO.h"

const U32 FloorPlanResource::smFileVersion = 0;

FloorPlanResource::FloorPlanResource()
{
}
FloorPlanResource::~FloorPlanResource()
{
}

//--------------------------------------------------------------------------
// Define these so we can just write two vector IO functions

static bool mathRead(Stream & S, FloorPlanResource::Area * a){
   return S.read(&a->pointCount) && S.read(&a->pointStart) && S.read(&a->plane);
}
static bool mathWrite(Stream & S, const FloorPlanResource::Area & a){
   return S.write(a.pointCount) && S.write(a.pointStart) && S.write(a.plane);
}
inline bool mathRead(Stream & S, S32 * s)          { return S.read(s);     }
inline bool mathWrite(Stream & S, S32 s)           { return S.write(s);    }

//--------------------------------------------------------------------------
// Read a vector of items which define mathRead().
template <class T>
bool mathReadVector(Vector<T> & vec, Stream & stream, const char * msg)
{
   U32   num, i;
   bool  Ok = true;
   stream.read( & num );
   vec.setSize( num );
   for( i = 0; i < num && Ok; i++ ){
      Ok = mathRead(stream, & vec[i]);
      AssertISV( Ok, avar("math vec read error (%s) on elem %d", msg, i) );
   }
   return Ok;
}
// Write a vector of items which define mathWrite().
template <class T>
bool mathWriteVector(const Vector<T> & vec, Stream & stream, const char * msg)
{
   bool  Ok = true;
   stream.write( vec.size() );
   for( U32 i = 0; i < vec.size() && Ok; i++ ) {
      Ok = mathWrite(stream, vec[i]);
      AssertISV( Ok, avar("math vec write error (%s) on elem %d", msg, i) );
   }
   return Ok;
}

//--------------------------------------------------------------------------

bool FloorPlanResource::read(Stream& stream)
{
   AssertFatal(stream.hasCapability(Stream::StreamRead), "FLR::read: non-readable stream");
   AssertFatal(stream.getStatus() == Stream::Ok, "FLR::read: Error, weird stream state");

   // Version this stream
   U32 fileVersion, DohVal;
   stream.read(&fileVersion);
   if (fileVersion != smFileVersion) {
      AssertFatal(false, "FLR::read: incompatible file version found.");
      return false;
   }

   // For expansion purposes
   stream.read(&DohVal); stream.read(&DohVal); stream.read(&DohVal);

   // Read the vectors
   mathReadVector( mPlaneTable, stream, "FLR: mPlaneTable" );
   mathReadVector( mPointTable, stream, "FLR: mPointTable" );
   mathReadVector( mPointLists, stream, "FLR: mPointLists" );
   mathReadVector( mAreas, stream, "FLR: mAreas" );

   return (stream.getStatus() == Stream::Ok);
}

//--------------------------------------------------------------------------

bool FloorPlanResource::write(Stream& stream) const
{
   AssertFatal(stream.hasCapability(Stream::StreamWrite), "FLR::write: non-writeable stream");
   AssertFatal(stream.getStatus() == Stream::Ok, "FLR::write: Error, weird stream state");

   // Version the stream
   stream.write(smFileVersion);

   U32 Doh = 0xD0bD0b;     // So we don't later say Doh!
   stream.write(Doh); stream.write(Doh); stream.write(Doh);

   // Write the vectors
   mathWriteVector( mPlaneTable, stream, "FLR: mPlaneTable" );
   mathWriteVector( mPointTable, stream, "FLR: mPointTable" );
   mathWriteVector( mPointLists, stream, "FLR: mPointLists" );
   mathWriteVector( mAreas, stream, "FLR: mAreas" );

   return( stream.getStatus() == Stream::Ok );
}

//------------------------------------------------------------------------------
//       FloorPlan Resource constructor
//
ResourceInstance * constructFloorPlanFLR(Stream& stream)
{
   FloorPlanResource * pResource = new FloorPlanResource;

   if (pResource->read(stream) == true)
      return pResource;
   else {
      delete pResource;
      return NULL;
   }
}
