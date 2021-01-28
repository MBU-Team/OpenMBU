/**********************************************************************
 *<
	FILE: appio.h

	DESCRIPTION:  General chunk-ifying code: useful for writing 
	   hierarchical data structures to a linear stream, such as
	   an AppData block.

	CREATED BY: Dan Silva

	HISTORY: created 3/24/97

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __APPIO__H
#define __APPIO__H

//------------------------------------------------------------------------
// AppSave will write hierarchical chunks into a private buffer, enlarging 
//  it as needed.  When completed, use the methods BufferPtr() and 
// NBytesWritten() to get at this buffer. ( AppSave will delete the buffer in 
// its DeleteThis() method , so you need to copy the buffer to save the data.)

// The chunk hierarchy should always have a single highest level chunk.
// Chunks can be nested to any depth.
// A Chunk can contain either sub-chunks, or data, but not both.

//   For example:
//
//	AppSave *asave = NewAppSave(1000);
//	asave->BeginChunk(MAIN_CHUNK);
//	   asave->BeginChunk(CHUNK1);
//      .. write data 
//	   asave->EndChunk();
//
//	   asave->BeginChunk(CHUNK2);
//      .. write data 
//	   asave->EndChunk();
//
//	   asave->BeginChunk(CHUNK3);
//      .. write data 
//	   asave->EndChunk();
//	asave->EndChunk();  // end MAIN_CHUNK


class AppSave {
	protected:
		~AppSave() {}
	public:
		virtual void DeleteThis()=0;
		
		// After saving, use this to get pointer to the buffer created.
		virtual BYTE *BufferPtr()=0;

		// This tells how many bytes were written in the buffer.
		virtual int NBytesWritten()=0;
		
		// Begin a chunk.
		virtual void BeginChunk(USHORT id)=0;

		// End a chunk, and back-patch the length.
		virtual void EndChunk()=0;

		virtual int CurChunkDepth()=0;  // for checking balanced BeginChunk/EndChunk

		// write a block of bytes to the output stream.
		virtual IOResult Write(const void  *buf, ULONG nbytes, ULONG *nwrit)=0;

		// Write character strings
		virtual IOResult WriteWString(const char *str)=0;
		virtual IOResult WriteWString(const wchar_t *str)=0;
		virtual IOResult WriteCString(const char *str)=0;
		virtual IOResult WriteCString(const wchar_t *str)=0;

		};

//------------------------------------------------------------------------
// AppLoad takes a chunk-ified data stream, and provides routines for 
// decoding it. 

class AppLoad {
	protected:
		~AppLoad() {};
	public:
		virtual void DeleteThis()=0;
			
		// if OpenChunk returns IO_OK, use following 3 function to get the 
		// info about the chunk. IO_END indicates no more chunks at this level
		virtual IOResult OpenChunk()=0;

		// These give info about the most recently opened chunk
		virtual USHORT CurChunkID()=0;
		virtual ChunkType CurChunkType()=0;
		virtual	ULONG CurChunkLength()=0;  // chunk length NOT including header
		virtual	int CurChunkDepth()=0;  // for checking balanced OpenChunk/CloseChunk

		// close the currently opened chunk, and position at the next chunk
		//  return of IO_ERROR indicates there is no open chunk to close
		virtual IOResult CloseChunk()=0;

		// Look at the next chunk ID without opening it.
		// returns 0 if no more chunks
		virtual	USHORT PeekNextChunkID()=0;

		// Read a block of bytes from the output stream.
		virtual IOResult Read(void  *buf, ULONG nbytes, ULONG *nread )=0;

		// Read a string from a string chunk assumes chunk is already open, 
		// it will NOT close the chunk. Sets buf to point
		// to a char string.  Don't delete buf: ILoad will take care of it.

		//   Read a string that was stored as Wide chars. 
		virtual IOResult ReadWStringChunk(char** buf)=0;
		virtual IOResult ReadWStringChunk(wchar_t** buf)=0;

		//   Read a string that was stored as single byte chars
		virtual IOResult ReadCStringChunk(char** buf)=0;
		virtual IOResult ReadCStringChunk(wchar_t** buf)=0;


	};


// Create a new AppLoad for reading chunks out of buf:
// bufSize specifies the number of bytes that are valid in
// buf.. 
CoreExport AppLoad *NewAppLoad(BYTE *buf, int bufSize);

// Create a new AppSave for writing chunks
// InitbufSize is the initial size the internal buffer is allocated to.
// It will be enlarged if necessary.
CoreExport AppSave *NewAppSave(int initBufSize);

#endif