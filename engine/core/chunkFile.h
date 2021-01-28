//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _CORE_CHUNKFILE_H_
#define _CORE_CHUNKFILE_H_

#include "core/bitStream.h"
#include "core/resManager.h"
#include "console/simBase.h"

//                    class,    fourcc,            rev
#define DECLARE_CHUNK(className, fourcc, rev)                     \
   static ConcreteClassRep<className> dynClassRep;                \
   static AbstractClassRep* getParentStaticClassRep();            \
   static AbstractClassRep* getStaticClassRep();                  \
   virtual AbstractClassRep* getClassRep() const;                 \
   virtual const U32 getFourCC() { return makeFourCCTag fourcc; } \
   virtual const U32 getChunkVersion() { return rev; }

class SimChunk : public SimGroup
{
   typedef SimGroup Parent;

public:
   DECLARE_CHUNK(SimChunk, ('S','C','H','K'), 1);

   /// Called to read the chunk's data. We are provided with all the
   /// information that we got from the file, just in case it's relevant.
   virtual void readChunk(BitStream &s, const U32 length, const U32 version, const U32 crc, const U32 fourCC);

   /// Write a chunk. Length is defined by the size of the data we write out,
   /// version by the DECLARE_CHUNK macro, crc by the data we write, and
   /// FourCC by the macro. So all we have to do is write data!
   virtual void writeChunk(BitStream &s);

   /// Called post console init to set up the 4cc->class mappings.
   struct FourCCToAcr
   {
      U32 fourCC;
      AbstractClassRep *acr;
   };

   virtual const char *getFourCCString() 
   {
      U8 tmp[5];
      U32 cc = getFourCC();

      tmp[0] = (cc >> 0) & 0xFF;
      tmp[1] = (cc >> 8) & 0xFF;
      tmp[2] = (cc >> 16) & 0xFF;
      tmp[3] = (cc >> 24) & 0xFF;
      tmp[4] = 0;

      return StringTable->insert((const char*)tmp, true);
   }

   static Vector<FourCCToAcr*> smFourCCList;
   static void initChunkMappings();
   static SimChunk *createChunkFromFourCC(U32 fourCC);
};

/// This is a special purpose chunk we use to deal with situations where
/// we have an unknown FourCC. Basically, it loads everything into a buffer,
/// then writes it back out again, masquerading as the unknown chunk.
///
/// This means we have to manually do some of what DECLARE_CHUNK does, ew.
class UnknownChunk : public SimChunk
{
   typedef SimChunk Parent;

   U32  mChunkFourCC;
   U8   mChunkFourCCString[4];
   U32  mChunkVersion;
   U32  mChunkCRC;

   U32  mDataLength;
   U8  *mDataBuffer;

public:
   DECLARE_CONOBJECT(UnknownChunk);

   UnknownChunk();
   ~UnknownChunk();

   virtual const U32 getFourCC() const { return mChunkFourCC; }
   virtual const U8 *getFourCCString() const { return &mChunkFourCCString[0]; }
   virtual const U32 getChunkVersion() const { return mChunkVersion; }

   virtual void readChunk(BitStream &s, const U32 length, const U32 version, const U32 crc, const U32 fourCC);
   virtual void writeChunk(BitStream &s);
};

// Allow only SimChunk and derivatives to be added to a SimChunk (override addObject)
// write/readChunk must call Parent::'s versions

class ChunkFile : ResourceInstance
{
private:
   /// Root of the chunk hierarchy for this file.
   SimObjectPtr<SimChunk> mRoot;

   /// Helper function, saves out a chunk and its children...
   bool saveInner(Stream &s, SimChunk *c);

   /// Helper function, loads up a chunk and its children...
   SimChunk *loadInner(Stream &s, U32 childCount=1);

   static const U32 csmFileFourCC;
   static const U32 csmFileVersion;

public:

   /// Generic chunk file loader.
   static ResourceInstance *constructChunkFile(Stream &stream);

   /// Return a pointer to the root chunk.
   SimChunk * getRoot() const { return mRoot; };
   void setRoot( SimChunk * c) { mRoot = c; };

   /// Serialize!
   bool save(const char * filename);

   /// Deserialize!
   bool load(Stream &s);
};

#endif