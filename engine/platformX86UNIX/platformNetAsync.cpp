//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
// can't include this until new mutex interface is merged
//#include "platform/platformMutex.h"
#include "platform/platformNetAsync.h"
#include "console/console.h"

#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static Mutex gNetAsyncMutex;
NetAsync gNetAsync;

// internal structure for storing information about a name lookup request
struct NameLookupRequest
{
      NetSocket sock;
      char remoteAddr[4096];
      char out_h_addr[4096];
      int out_h_length;
      bool complete;

      NameLookupRequest()
      {
         sock = InvalidSocket;
         remoteAddr[0] = 0;
         out_h_addr[0] = 0;
         out_h_length = -1;
         complete = false;
      }
};

void NetAsync::queueLookup(const char* remoteAddr, NetSocket socket)
{
   gNetAsyncMutex.lock();
   // do we have it already?
   unsigned int i = 0;
   for (i = 0; i < mLookupRequests.size(); ++i)
   {
      if (mLookupRequests[i]->sock == socket)
         // found it.  ignore more than one lookup at a time for a socket.
         break;
   }

   if (i == mLookupRequests.size())
   {
      // not found, so add it
      NameLookupRequest* lookupRequest = new NameLookupRequest();
      lookupRequest->sock = socket;
      dStrncpy(lookupRequest->remoteAddr, remoteAddr, 
               sizeof(lookupRequest->remoteAddr));
      mLookupRequests.push_back(lookupRequest);
   }
   gNetAsyncMutex.unlock();
}

void NetAsync::run()
{
   if (isRunning())
      return;

   mRunning = true;
   NameLookupRequest* lookupRequest = NULL;

   while (isRunning())
   {
      lookupRequest = NULL;

      // lock 
      gNetAsyncMutex.lock();
      // if there is a request...
      if (mLookupRequests.size() > 0)
      {
         // assign the first incomplete request
         for (unsigned int i = 0; i < mLookupRequests.size(); ++i)
            if (!mLookupRequests[i]->complete)
               lookupRequest = mLookupRequests[i];
      }

      // unlock so that more requests can be added
      gNetAsyncMutex.unlock();

      // if we have a lookup request
      if (lookupRequest != NULL)
      {
         // do it
         struct hostent* hostent = gethostbyname(lookupRequest->remoteAddr);
         if (hostent == NULL)
         {
            // oh well!  leave the lookup data unmodified (h_length) should
            // still be -1 from initialization
            lookupRequest->complete = true;
         }
         else
         {
            // copy the stuff we need from the hostent 
            dMemset(lookupRequest->out_h_addr, 0, 
                   sizeof(lookupRequest->out_h_addr));
            dStrncpy(lookupRequest->out_h_addr, hostent->h_addr,
                    hostent->h_length);
            lookupRequest->out_h_length = hostent->h_length;
            lookupRequest->complete = true;
         }
      }
      else
      {
         // no lookup request.  sleep for a bit
         usleep(500000);
      }
   };
}

bool NetAsync::checkLookup(NetSocket socket, char* out_h_addr, 
                           int* out_h_length, int out_h_addr_size)
{
   gNetAsyncMutex.lock();
   unsigned int i = 0;
   bool found = false;
   // search for the socket
   Vector<NameLookupRequest*>::iterator iter;
   for (iter = mLookupRequests.begin(); 
        iter != mLookupRequests.end(); 
        ++iter)
      // if we found it and it is complete...
      if (socket == (*iter)->sock && (*iter)->complete)
      {
         // copy the lookup data to the callers parameters
         dStrncpy(out_h_addr, (*iter)->out_h_addr, out_h_addr_size);
         *out_h_length = (*iter)->out_h_length;
         found = true;
         break;
      }

   // we found the socket, so we are done with it.  erase.
   if (found)
   {
      delete *iter;
      mLookupRequests.erase(iter);
   }
   gNetAsyncMutex.unlock();

   return found;
}

// this is called by the pthread module to start the thread
static void* StartThreadFunc(void* nothing)
{
   nothing;

   if (gNetAsync.isRunning())
      return NULL;

   gNetAsync.run();
   return NULL;
}


void NetAsync::startAsync()
{
  if (gNetAsync.isRunning())
     return;

  // create the thread...
  pthread_t thread;
  int ret = pthread_create(&thread, NULL, StartThreadFunc, NULL);
  if (ret != 0)
     Con::errorf("Error starting net async thread: %s", strerror(errno));
}

void NetAsync::stopAsync()
{
   if (gNetAsync.isRunning())
      gNetAsync.stop();
}
