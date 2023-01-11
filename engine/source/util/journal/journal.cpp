//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include "core/fileStream.h"
#include "util/journal/journal.h"
#include "util/safeDelete.h"

//-----------------------------------------------------------------------------

Journal::FuncDecl* Journal::_FunctionList;
Stream *Journal::mFile;
Journal::Mode Journal::_State;
U32 Journal::_Count;
bool Journal::_Dispatching = false;

//-----------------------------------------------------------------------------

static bool _AtExitHooked = false;
extern "C" {
   static void atExit() {
      Journal::Stop();
   }
}

//-----------------------------------------------------------------------------

Journal::Functor* Journal::_create(Id id)
{
   for (FuncDecl* ptr = _FunctionList; ptr; ptr = ptr->next)
      if (ptr->id == id)
         return ptr->create();
   return 0;
}

Journal::Id Journal::_getFunctionId(VoidPtr ptr,VoidMethod method)
{
   for (FuncDecl* itr = _FunctionList; itr; itr = itr->next)
      if (itr->match(ptr,method))
         return itr->id;
   return 0;
}

void Journal::_removeFunctionId(VoidPtr ptr,VoidMethod method)
{
   FuncDecl ** itr = &_FunctionList;

   do 
   {
      if((*itr)->match(ptr, method))
      {
         // Unlink and break.
         idPool().free((*itr)->id);
         *itr = (*itr)->next;
         return;
      }

      // Advance to next...
      itr = &((*itr)->next);
   }
   while(*itr);
}

void Journal::_start()
{
}

void Journal::_finish()
{
   if (_State == PlayState)
      --_Count;
   else {
      U32 pos = mFile->getPosition();
      mFile->setPosition(0);
      mFile->write(++_Count);
      mFile->setPosition(pos);
   }
}

void Journal::Record(const char * file)
{
   if (_State == StopState)
   {
      _Count = 0;
      mFile = new FileStream();
      ((FileStream*)mFile)->open(file, FileStream::Write);

      AssertFatal(mFile,"Journal: Could not create journal file");

      mFile->write(_Count);

      _State = RecordState;

      if (!_AtExitHooked)
      {
         atexit(atExit);
      }
   }
}

void Journal::Play(const char * file)
{
   if (_State == StopState)
   {
      SAFE_DELETE(mFile);
      mFile = new FileStream();
      ((FileStream*)mFile)->open(file, FileStream::Read);

      AssertFatal(mFile,"Journal: Could not open journal file");

      mFile->read(&_Count);
      _State = PlayState;
   }
}

void Journal::Stop()
{
   AssertFatal(mFile, "Journal::Stop - no file stream open!");

   ((FileStream*)mFile)->close();
   _State = StopState;
}

bool Journal::PlayNext()
{
   if (_State == PlayState) {
      _start();
      Id id;

      mFile->read(&id);

      Functor* jrn = _create(id);
      AssertFatal(jrn,"Journal: Undefined function found in journal");
      jrn->read(mFile);
      _finish();

      _Dispatching = true;
      jrn->dispatch();
      _Dispatching = false;

      delete jrn;
      if (_Count)
         return true;
      Stop();

      //debugBreak();
   }
   return false;
}