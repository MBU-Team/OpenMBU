//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/chunkFile.h"

InfiniteBitStream gChunkBuffer;
InfiniteBitStream gHeaderBuffer;

const U32 ChunkFile::csmFileFourCC = makeFourCCTag('T', 'C', 'H', 'K');
const U32 ChunkFile::csmFileVersion = 2;

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

bool ChunkFile::save(const char * filename)
{
   // Just to be sure, wipe stuff.
   gChunkBuffer.reset();

   // Get a stream...
   FileStream s;
   if(!ResourceManager->openFileForWrite(s, filename))
   {
      Con::errorf("ChunkFile::save - cannot open '%s' for write.", filename);
      return false;
   }

   // write preamble!
   s.write(csmFileFourCC);
   s.write(csmFileVersion);

   // Now save out the chunks!
   saveInner(s, getRoot());

   // Free any memory we had to use for buffering.
   gChunkBuffer.reset();
   gChunkBuffer.compact();

   // All done!
   return true;
}

bool ChunkFile::saveInner(Stream &s, SimChunk *c)
{
   // First, write the chunk to a buffer so we can meditate on it
   gChunkBuffer.reset();
   c->writeChunk(gChunkBuffer);

   // Buffer the header...
   gHeaderBuffer.reset();

   // Prepare the header
   gHeaderBuffer.writeCussedU32(c->getChunkVersion());
   gHeaderBuffer.writeCussedU32(gChunkBuffer.getPosition());
   gHeaderBuffer.writeString((const char *)c->getFourCCString(), 4);
   gHeaderBuffer.writeCussedU32(c->size()); // Child count.
   gHeaderBuffer.write(gChunkBuffer.getCRC());

   // Write the header (first size, then data)!
   AssertFatal(gHeaderBuffer.getPosition() < 256,
                                 "ChunkFile::saveInner - got too big header!");
   s.write(U8(gHeaderBuffer.getPosition())); 
   gHeaderBuffer.writeToStream(s);
   gHeaderBuffer.reset();

   // Write the chunk!
   gChunkBuffer.writeToStream(s);
   gChunkBuffer.reset();

   // Recurse!
   for(U32 i=0; i<c->size(); i++)
   {
      // We have to assume people will stuff all sorts of crap into this thing.
      SimChunk *sc = dynamic_cast<SimChunk*>((*c)[i]);

      if(sc)
         saveInner(s, sc);
      else
      {
         Con::errorf("ChunkFile::saveInner - got unexpected class '%s' as child of object %d!", 
            (c ? (*c)[i]->getClassName() : "NULL OBJECT"), c->getId());

         // Write a null header, eww gross.
         gHeaderBuffer.writeCussedU32(0);
         gHeaderBuffer.writeCussedU32(0);
         gHeaderBuffer.writeString("", 4);
         gHeaderBuffer.writeCussedU32(0); // Child count.
         gHeaderBuffer.write(0);

         // That should allow us to recover gracefully when we load...
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool ChunkFile::load(Stream &s)
{
   // check preamble!
   U32 fourcc, ver;
   s.read(&fourcc);
   s.read(&ver);

   if(fourcc != csmFileFourCC)
   {
      Con::errorf("ChunkFile::load - unexpected header value!");
      return false;
   }

   if(ver != csmFileVersion)
   {
      // Be clever
      if(ver > csmFileVersion)
      {
         Con::errorf("ChunkFile::load - encountered file header version from the future!");
      }
      else
      {
         Con::errorf("ChunkFile::load - encountered file header version from the past!");
      }

      // Error out.
      return false;
   }

   // Now recursively load all our children!
   mRoot = loadInner(s);

   // Add us to the ChunkFileGroup
   Sim::getChunkFileGroup()->addObject(mRoot);

   return true;
}

SimChunk *ChunkFile::loadInner(Stream &s, U32 childCount)
{
   U8 headerSize;
   U32 fourCC, ver, crc, size;
   S32 children;

   // Read the header size and prepare to read the header bitstream...
   s.read(&headerSize);

   U8 *headerBuff = new U8[headerSize];
   s.read(headerSize, headerBuff);

   BitStream hData(headerBuff, headerSize);

   // Read actual header values.
   ver      = hData.readCussedU32();
   size     = hData.readCussedU32();

   // Get the fourcc...
   char fcctmp[256]; // For safety we make it 256, longest size string we can read.
   hData.readString(fcctmp);
   fourCC = makeFourCCTag(fcctmp[0], fcctmp[1], fcctmp[2], fcctmp[3]);

   children = hData.readCussedU32();
   hData.read(&crc);

   delete[] headerBuff;

   // Create the chunk...
   SimChunk *sc = SimChunk::createChunkFromFourCC(fourCC);

   // Do some version sanity checking.
   if(ver > sc->getChunkVersion())
   {
      // uh oh, it's a chunk from the future!
      Con::warnf("ChunkFile::loadInner - found a '%s' chunk with version %d; highest supported version is %d. Treating as unknown chunk...",
                       fcctmp, ver, sc->getChunkVersion());

      // Let's use an unknown chunk instead.
      delete sc;
      sc = new UnknownChunk();
   }

   // Read the chunk data into a buffer...
   U8 *buff = new U8[size];
   s.read(size, buff);

   BitStream cData(buff, size);
   sc->readChunk(cData, size, ver, crc, fourCC);

   delete[] buff;

   // Register it!
   sc->registerObject();

   // Recurse on children, adding them to our SimChunk.
   for(S32 i=0; i<children; i++)
      sc->addObject(loadInner(s, childCount));

   // All done!
   return sc;
}

//------------------------------------------------------------------------------

ResourceInstance *ChunkFile::constructChunkFile(Stream &stream)
{
   ChunkFile *cf = new ChunkFile();
   if(cf->load(stream))
   {
      return cf;
   }
   else
   {
      delete cf;
      return NULL;
   }
}

//------------------------------------------------------------------------------
Vector<SimChunk::FourCCToAcr*> SimChunk::smFourCCList;

void SimChunk::initChunkMappings()
{
   Con::printf("Initializing chunk mappings...");

   for(AbstractClassRep *rep = AbstractClassRep::getClassList(); rep; rep = rep->getNextClass())
   {
      ConsoleObject *obj = rep->create();

      SimChunk *chunk;

      if(obj && (chunk = dynamic_cast<SimChunk *>(obj)))
      {
         Con::printf("   o '%s' maps to %s", chunk->getFourCCString(), chunk->getClassName());

         FourCCToAcr * fcta = new FourCCToAcr();

         fcta->fourCC = chunk->getFourCC();
         fcta->acr    = rep;       

         smFourCCList.push_back(fcta);
      }

      delete obj;
   }

   // Also register the .chunk file format.
   ResourceManager->registerExtension(".chunk", ChunkFile::constructChunkFile);
}

SimChunk *SimChunk::createChunkFromFourCC(U32 fourCC)
{
   // Try to find a match.
   for(U32 i=0; i<smFourCCList.size(); i++)
   {
      if(smFourCCList[i]->fourCC == fourCC)
         return (SimChunk*)smFourCCList[i]->acr->create();
   }

   // Unknown 4cc, let's use the UnknownChunk
   U8 c[4];
   c[0] = fourCC >> 24;
   c[1] = fourCC >> 16;
   c[2] = fourCC >> 8;
   c[3] = fourCC >> 0;
   Con::warnf("SimChunk::createChunkFromFourCC - encountered unknown fourCC '%c%c%c%c'", c[0], c[1], c[2], c[3]);

   return new UnknownChunk();
}

//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(SimChunk);

void SimChunk::writeChunk(BitStream &b)
{

}

void SimChunk::readChunk(BitStream &s, const U32 length, const U32 version, const U32 crc, const U32 fourCC)
{

}

//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(UnknownChunk);

UnknownChunk::UnknownChunk()
{
   mChunkFourCC = mChunkVersion = mChunkCRC = mDataLength = 0;
   mDataBuffer = NULL;
}

UnknownChunk::~UnknownChunk()
{
   if(mDataBuffer)
      dFree(mDataBuffer);
   mDataBuffer = NULL;
}

void UnknownChunk::writeChunk(BitStream &b)
{
   b.write(mDataLength, mDataBuffer);
}

void UnknownChunk::readChunk(BitStream &s, const U32 length, const U32 version, const U32 crc, const U32 fourCC)
{
   mDataBuffer = (U8*)dMalloc(length);
   s.read(length, mDataBuffer);
}

//------------------------------------------------------------------------------

class TextChunk : public SimChunk
{
   typedef SimChunk Parent;

protected:
   const char * mText;

public:
   DECLARE_CHUNK(TextChunk, ('T','E','X','T'), 2);

   TextChunk()
   {
      mText = StringTable->insert("");
   }

   ~TextChunk()
   {
      // No deletion, it's a string table entry.
   }

   static void initPersistFields()
   {
      Parent::initPersistFields();

      addField("textData", TypeString, Offset(mText, TextChunk));
   }

   void readChunk(BitStream &s, const U32 length, const U32 version, const U32 crc, const U32 fourCC)
   {
      switch(version)
      {
         // Deal with older stuff.
      case 1:
         mText = s.readSTString();
         break;

         // Shiny new format!
      case 2:
         // Resize the buffer..
         U32 newSize;
         s.read(&newSize);

         char *tmpBuff = new char[newSize+1];
         s.readLongString(newSize, (char*)tmpBuff);

         mText = StringTable->insert(tmpBuff);

         delete[] tmpBuff;
         break;
      }
   }

   void writeChunk(BitStream &s)
   {
      s.write((U32)dStrlen(mText));
      s.writeLongString(dStrlen(mText), mText);
   }
};

IMPLEMENT_CONOBJECT(TextChunk);

//------------------------------------------------------------------------------

ConsoleFunction(saveChunkFile, bool, 3, 3, "(SimChunk chunk, Filename file)"
                "Write a chunk hierarchy to a file.")
{
   SimChunk *rootChunk = NULL;
   const char *file = argv[2];

   if(!Sim::findObject(argv[1], rootChunk))
   {
      Con::errorf("writeChunkFile - Unable to locate root chunk '%s'", argv[1]);
      return false;
   }

   ChunkFile *cf = new ChunkFile();

   cf->setRoot(rootChunk);

   bool res = true;

   if(!cf->save(file))
   {
      Con::errorf("writeChunkFile - Failed to save '%s' to '%s'", argv[1],  file);
      res = false;
   }

   delete cf;

   return res;
}

ConsoleFunction(loadChunkFile, S32, 2, 2, "(Filename file)"
                "Read a chunk hierarchy from a file.")
{
   Resource<ChunkFile> ri = ResourceManager->load(argv[1]);

   if(bool(ri) == false)
   {
      Con::errorf("loadChunkFile - failed to open '%s'", argv[1]);
      return 0;
   }

   // Otherwise we're ok.
   return ri->getRoot()->getId();
}