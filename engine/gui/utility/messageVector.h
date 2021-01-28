//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MESSAGEVECTOR_H_
#define _MESSAGEVECTOR_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

/// Store a list of chat messages.
///
/// This is responsible for managing messages which appear in the chat HUD.
///
/// @see GuiMessageVectorCtrl for more details on how this is used.
class MessageVector : public SimObject
{
   typedef SimObject Parent;

   //-------------------------------------- Public interface...
  public:
   MessageVector();
   ~MessageVector();

  public:
   struct MessageLine {
      char* message;
      S32   messageTag;
   };


   // Spectator registration...
  public:
   enum MessageCode {
      LineInserted   = 0,
      LineDeleted    = 1,

      VectorDeletion = 2
   };

   typedef void (*SpectatorCallback)(void *            spectatorKey,
                                     const MessageCode code,
                                     const U32         argument);

   void registerSpectator(SpectatorCallback, void * spectatorKey);
   void unregisterSpectator(void *spectatorKey);

   // Query functions
  public:
   U32                getNumLines() const;
   const MessageLine& getLine(const U32 line) const;

   // Mutators
  public:
   void pushBackLine(const char*, const S32);
   void popBackLine();
   void pushFrontLine(const char*, const S32);
   void popFrontLine();
   void clear();

   virtual void insertLine(const U32 position, const char*, const S32);
   virtual void deleteLine(const U32);

   bool dump( const char* filename, const char* header = NULL );


   //-------------------------------------- Internal interface
  protected:
   bool onAdd();
   void onRemove();

  private:
   struct SpectatorRef {
      SpectatorCallback callback;
      void *               key;
   };

   Vector<MessageLine>  mMessageLines;

   Vector<SpectatorRef> mSpectators;
   void spectatorMessage(MessageCode, const U32 arg);

  public:
   DECLARE_CONOBJECT(MessageVector);
   static void initPersistFields();
};


//--------------------------------------------------------------------------
inline U32 MessageVector::getNumLines() const
{
   return mMessageLines.size();
}

//--------------------------------------------------------------------------
inline const MessageVector::MessageLine& MessageVector::getLine(const U32 line) const
{
   AssertFatal(line < mMessageLines.size(), "MessageVector::getLine: out of bounds line index");
   return mMessageLines[line];
}

#endif  // _H_GUICHATVECTOR_
