//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DATACHUNKER_H_
#define _DATACHUNKER_H_


//----------------------------------------------------------------------------
/// Implements a chunked data allocater.
///
/// Calling new/malloc all the time is a time consuming operation. Therefore,
/// we provide the DataChunker, which allocates memory in blockss of
/// chunkSize (by default 16k, see ChunkSize, though it can be set in
/// the constructor), then doles it out as requested, in chunks of up to
/// chunkSize in size.
///
/// It will assert if you try to get more than ChunkSize bytes at a time,
/// and it deals with the logic of allocating new blocks and giving out
/// word-aligned chunks.
///
/// Note that new/free/realloc WILL NOT WORK on memory gotten from the
/// DataChunker. This also only grows (you can call freeBlocks to deallocate
/// and reset things).
class DataChunker
{
  public:
   enum {
      ChunkSize = 16376 ///< Default size for chunks.
   };

  private:
   /// Block of allocated memory.
   ///
   /// <b>This has nothing to do with datablocks as used in the rest of Torque.</b>
   struct DataBlock
   {
      DataBlock *next;
      U8 *data;
      S32 curIndex;
      DataBlock(S32 size);
      ~DataBlock();
   };
   DataBlock *curBlock;
   S32 chunkSize;

  public:

   /// Return a pointer to a chunk of memory from a pre-allocated block.
   ///
   /// This memory goes away when you call freeBlocks.
   ///
   /// This memory is word-aligned.
   /// @param   size    Size of chunk to return. This must be less than chunkSize or else
   ///                  an assertion will occur.
   void *alloc(S32 size);

   /// Free all allocated memory blocks.
   ///
   /// This invalidates all pointers returned from alloc().
   void freeBlocks();

   /// Initialize using blocks of a given size.
   ///
   /// One new block is allocated at constructor-time.
   ///
   /// @param   size    Size in bytes of the space to allocate for each block.
   DataChunker(S32 size=ChunkSize);
   ~DataChunker();
};


//----------------------------------------------------------------------------

template<class T>
class Chunker: private DataChunker
{
public:
   Chunker(S32 size = DataChunker::ChunkSize) : DataChunker(size) {};
   T* alloc()  { return reinterpret_cast<T*>(DataChunker::alloc(S32(sizeof(T)))); }
   void clear()  { freeBlocks(); };
};

template<class T>
class FreeListChunker: private DataChunker
{
   S32 numAllocated;
   S32 elementSize;
   T *freeListHead;
public:
   FreeListChunker(S32 size = DataChunker::ChunkSize) : DataChunker(size)
   {
      numAllocated = 0;
      elementSize = getMax(U32(sizeof(T)), U32(sizeof(T *)));
      freeListHead = NULL;
   }

   T *alloc()
   {
      numAllocated++;
      if(freeListHead == NULL)
         return reinterpret_cast<T*>(DataChunker::alloc(elementSize));
      T* ret = freeListHead;
      freeListHead = *(reinterpret_cast<T**>(freeListHead));
      return ret;
   }

   void free(T* elem)
   {
      numAllocated--;
      *(reinterpret_cast<T**>(elem)) = freeListHead;
      freeListHead = elem;

      // If nothing's allocated, clean up!
      if(!numAllocated)
      {
         freeBlocks();
         freeListHead = NULL;
      }
   }

   // Allow people to free all their memory if they want.
   void freeBlocks()
   {
      DataChunker::freeBlocks();

      // We have to terminate the freelist as well or else we'll run
      // into crazy unused memory.
      freeListHead = NULL;
   }
};


#endif
