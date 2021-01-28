//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMINPUT_H_
#define _PLATFORMINPUT_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif


//------------------------------------------------------------------------------
U8 TranslateOSKeyCode( U8 vcode );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class InputDevice : public SimObject
{
   public:
      struct ObjInfo
      {
         U16   mType;
         U16   mInst;
         S32   mMin, mMax;
      };

   protected:
      char mName[30];

   public:
      const char* getDeviceName();
      virtual bool process() = 0;
};


//------------------------------------------------------------------------------
inline const char* InputDevice::getDeviceName()
{
   return mName;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class InputManager : public SimGroup
{
   protected:
      bool  mEnabled;

   public:
      bool  isEnabled();

      virtual bool enable() = 0;
      virtual void disable() = 0;

      virtual void process() = 0;
};


//------------------------------------------------------------------------------
inline bool InputManager::isEnabled()
{
   return mEnabled;
}

enum KEY_STATE
{
   STATE_LOWER,
   STATE_UPPER,
   STATE_GOOFY
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class Input
{
   protected:
      static InputManager* smManager;
      static bool smActive;
      static bool smLastKeyboardActivated;
      static bool smLastMouseActivated;
      static bool smLastJoystickActivated;

   public:
      static void init();
      static void destroy();

      static bool enable();
      static void disable();

      static void activate();
      static void deactivate();
      static void reactivate();

      static U16  getAscii( U16 keyCode, KEY_STATE keyState );
      static U16  getKeyCode( U16 asciiCode );

      static bool isEnabled();
      static bool isActive();

      static void process();

      static InputManager* getManager();

#ifdef LOG_INPUT
      static void log( const char* format, ... );
#endif
};

#endif // _H_PLATFORMINPUT_
