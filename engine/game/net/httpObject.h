//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _HTTPOBJECT_H_
#define _HTTPOBJECT_H_

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _TCPOBJECT_H_
#include "game/net/tcpObject.h"
#endif

class HTTPObject : public TCPObject
{
private:
   typedef TCPObject Parent;
protected:
   enum ParseState {
      ParsingStatusLine,
      ParsingHeader,
      ParsingChunkHeader,
      ProcessingBody,
      ProcessingDone,
   };
   ParseState mParseState;
   U32 mTotalBytes;
   U32 mBytesRemaining;
 public:
   U32 mStatus;
   F32 mVersion;
   U32 mContentLength;
   bool mChunkedEncoding;
   U32 mChunkSize;
   const char *mContentType;
   char *mHostName;
   char *mPath;
   char *mQuery;
   char *mPost;
   U8 *mBufferSave;
   U32 mBufferSaveSize;
public:
   static void expandPath(char *dest, const char *path, U32 destSize);
   void get(const char *hostName, const char *urlName, const char *query);
   void post(const char *host, const char *path, const char *query, const char *post);
   HTTPObject();
   ~HTTPObject();

   //static HTTPObject *find(U32 tag);

   virtual U32 onDataReceive(U8 *buffer, U32 bufferLen);
   virtual U32 onReceive(U8 *buffer, U32 bufferLen); // process a buffer of raw packet data
   virtual void onConnected();
   virtual void onConnectFailed();
   virtual void onDisconnect();
   bool processLine(U8 *line);

   DECLARE_CONOBJECT(HTTPObject);
};


#endif  // _H_HTTPOBJECT_
