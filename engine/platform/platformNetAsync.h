//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef PLATFORM_NET_ASYNC_H
#define PLATFORM_NET_ASYNC_H

#include "platform/platform.h"
#include "core/tVector.h"

// JMQ: new mutex interface here for now, until it gets merged into 
// platformMutex
#include <pthread.h>

class Mutex
{
   pthread_mutex_t mutex;
   bool valid;
public:
   Mutex()
   {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init( &attr );
      //pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );

      valid = pthread_mutex_init( &mutex, &attr );
   }
   ~Mutex()
   {
      if(valid)
         pthread_mutex_destroy(&mutex);
   }
   void lock()
   {
      if(valid)
         pthread_mutex_lock(&mutex);
   }
   void unlock()
   {
      if(valid)
         pthread_mutex_unlock(&mutex);
   }
};

struct NameLookupRequest;

// class for doing asynchronous network operations on unix (linux and 
// hopefully osx) platforms.  right now it only implements dns lookups
class NetAsync
{
   private:
      Vector<NameLookupRequest*> mLookupRequests;
      bool mRunning;

   public:
      NetAsync()
      {
         mRunning = false;
      }

      // queue a DNS lookup.  only one dns lookup can be queued per socket at
      // a time.  subsequent queue request for the socket are ignored.  use
      // checkLookup() to check the status of a request.
      void queueLookup(const char* remoteAddr, NetSocket socket);

      // check on the status of a dns lookup for a socket.  if the lookup is 
      // not yet complete, the function will return false.  if it is 
      // complete, the function will return true, and out_h_addr and 
      // out_h_length will be set appropriately.  if out_h_length is -1, then
      // name could not be resolved.  otherwise, it provides the number of
      // address bytes copied into out_h_addr.
      bool checkLookup(NetSocket socket, char* out_h_addr, int* out_h_length, int out_h_addr_size);

      // returns true if the async thread is running, false otherwise
      bool isRunning() { return mRunning; };

      // these functions are used by the static start/stop functions
      void run();
      void stop() { mRunning = false; };

      // used to start and stop the thread
      static void startAsync();
      static void stopAsync();
};

// the global net async object
extern NetAsync gNetAsync;

#endif
