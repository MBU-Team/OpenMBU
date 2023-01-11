//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_JOURNAL_JOURNALEDSIGNAL_H_
#define _UTIL_JOURNAL_JOURNALEDSIGNAL_H_

#include "util/journal/journal.h"
#include "util/tSignal.h"

/// Journaled version of the Signal class.
///
/// All events going through this class are journaled with the kernel's
/// journaling system. In order for journals to be played back correctly,
/// this class must be constructed deterministically either before the
/// journal is started, or directly from some other journaled event.
/// The internal journaling functions are declared when the class
/// is constructed.
template <class A = EmptyType, class B = EmptyType, class C = EmptyType,
          class D = EmptyType, class E = EmptyType, class F = EmptyType,
          class G = EmptyType, class H = EmptyType>
class JournaledSignal
{
   Signal<A,B,C,D,E,F,G,H> _signal;

   // Callback methods invoked by the journaling system
   void journal8(A a,B b,C c,D d,E e, F f, G g, H h)  { _signal.trigger(a,b,c,d,e,f,g,h); }
   void journal7(A a,B b,C c,D d,E e, F f, G g)       { _signal.trigger(a,b,c,d,e,f,g); }
   void journal6(A a,B b,C c,D d,E e, F f)            { _signal.trigger(a,b,c,d,e,f); }
   void journal5(A a,B b,C c,D d,E e)                 { _signal.trigger(a,b,c,d,e); }
   void journal4(A a,B b,C c,D d)                     { _signal.trigger(a,b,c,d); }
   void journal3(A a,B b,C c)                         { _signal.trigger(a,b,c); }
   void journal2(A a,B b)                             { _signal.trigger(a,b); }
   void journal1(A a)                                 { _signal.trigger(a); }
   void journal0()                                    { _signal.trigger(); }

   /// Helper typename used to declare the appropriate callback method with
   /// the journaling system
   template<typename ES,typename AA,typename BB,typename CC,typename DD,typename EE,typename FF,typename GG,typename HH>
   struct Declare
   { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal8, destroy); } };

   template<typename ES,typename AA,typename BB,typename CC,typename DD,typename EE,typename FF,typename GG>
   struct Declare<ES,AA,BB,CC,DD,EE,FF,GG,EmptyType>
   { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal7, destroy); } };

   template<typename ES,typename AA,typename BB,typename CC,typename DD,typename EE,typename FF>
   struct Declare<ES,AA,BB,CC,DD,EE,FF,EmptyType,EmptyType>
   { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal6, destroy); } };

   template<typename ES,typename AA,typename BB,typename CC,typename DD,typename EE>
   struct Declare<ES,AA,BB,CC,DD,EE,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal5, destroy); } };

   template<typename ES,typename AA,typename BB,typename CC,typename DD>
   struct Declare<ES,AA,BB,CC,DD,EmptyType,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal4, destroy); } };

   template<typename ES,typename AA,typename BB,typename CC>
   struct Declare<ES,AA,BB,CC,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal3, destroy); } };

   template<typename ES,typename AA,typename BB>
   struct Declare<ES,AA,BB,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal2, destroy); } };

   template<typename ES,typename AA>
   struct Declare<ES,AA,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal1, destroy); } };

   template<typename ES>
   struct Declare<ES,::EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType>
      { static inline void declare(ES* ptr, bool destroy) { Journal::DeclareFunction(ptr,&ES::journal0, destroy); } };

public:
   JournaledSignal() {
      Declare<JournaledSignal,A,B,C,D,E,F,G,H>::declare(this, false);
   }

   ~JournaledSignal() {
      // Unregister ourselves...
      Declare<JournaledSignal,A,B,C,D,E,F,G,H>::declare(this, true);
   }

   template <class T,class U>
   void notify(T obj,U func, F32 order = 0.5f) {
      _signal.notify(obj,func,order);
   }

   template <class T>
   void notify(T func, F32 order = 0.5f) {
      _signal.notify(func,order);
   }

   template <class T>
   void remove(T func) {
      _signal.remove(func);
   }

   template <class T,class U>
   void remove(T obj,U func) {
      _signal.remove(obj,func);
   }

   inline void trigger(A a,B b,C c,D d,E e, F f, G g, H h) {
      if (!_signal.isEmpty())
         Journal::Call((JournaledSignal*)this,&JournaledSignal::journal8,a,b,c,d,e,f,g,h);
   }
   inline void trigger(A a,B b,C c,D d,E e, F f, G g) {
      if (!_signal.isEmpty())
         Journal::Call((JournaledSignal*)this,&JournaledSignal::journal7,a,b,c,d,e,f,g);
   }
   inline void trigger(A a,B b,C c,D d,E e, F f) {
      if (!_signal.isEmpty())
         Journal::Call((JournaledSignal*)this,&JournaledSignal::journal6,a,b,c,d,e,f);
   }
   inline void trigger(A a,B b,C c,D d,E e) {
      if (!_signal.isEmpty())
         Journal::Call((JournaledSignal*)this,&JournaledSignal::journal5,a,b,c,d,e);
   }
   inline void trigger(A a,B b,C c,D d) {
      if (!_signal.isEmpty())
         Journal::Call(this,&JournaledSignal::journal4,a,b,c,d);
   }
   inline void trigger(A a,B b,C c) {
      if (!_signal.isEmpty())
         Journal::Call(this,&JournaledSignal::journal3,a,b,c);
   }
   inline void trigger(A a,B b) {
      if (!_signal.isEmpty())
         Journal::Call(this,&JournaledSignal::journal2,a,b);
   }
   inline void trigger(A a) {
      if (!_signal.isEmpty())
         Journal::Call(this,&JournaledSignal::journal1,a);
   }
   inline void trigger() {
      if (!_signal.isEmpty())
         Journal::Call(this,&JournaledSignal::journal0);
   }
};


//-----------------------------------------------------------------------------
// Common event callbacks definitions
enum InputModifier {
   IM_LALT     = (1 << 1),
   IM_RALT     = (1 << 2),
   IM_LSHIFT   = (1 << 3),
   IM_RSHIFT   = (1 << 4),
   IM_LCTRL    = (1 << 5),
   IM_RCTRL    = (1 << 6),
   IM_LOPT     = (1 << 7),
   IM_ROPT     = (1 << 8),
   IM_ALT      = IM_LALT | IM_RALT,
   IM_SHIFT    = IM_LSHIFT | IM_RSHIFT,
   IM_CTRL     = IM_LCTRL | IM_RCTRL,
   IM_OPT      = IM_LOPT | IM_ROPT,
};

enum InputAction {
   IA_MAKE     = (1 << 0),
   IA_BREAK    = (1 << 1),
   IA_REPEAT   = (1 << 2),
   IA_MOVE     = (1 << 3),
   IA_DELTA    = (1 << 4),
   IA_BUTTON   = (1 << 5),
};

enum ApplicationMessage {
   Quit,
   WindowOpen,          ///< Window opened
   WindowClose,         ///< Window closed.
   WindowDestroy,       ///< Window was destroyed.
   GainFocus,           ///< Application gains focus
   LoseFocus,           ///< Application loses focus
   Timer,
};

typedef U32 DeviceId;

/// void event()
typedef JournaledSignal<> IdleEvent;

/// void event(DeviceId,U32 modifier,S32 x,S32 y, bool isRelative)
typedef JournaledSignal<DeviceId,U32,S32,S32,bool> MouseEvent;

/// void event(DeviceId,U32 modifier,S32 wheelDelta)
typedef JournaledSignal<DeviceId,U32,S32> MouseWheelEvent;

/// void event(DeviceId,U32 modifier,U32 action,U16 key)
typedef JournaledSignal<DeviceId,U32,U32,U16> KeyEvent;

/// void event(DeviceId,U32 modifier,U16 key)
typedef JournaledSignal<DeviceId,U32,U16> CharEvent;

/// void event(DeviceId,U32 modifier,U32 action,U16 button)
typedef JournaledSignal<DeviceId,U32,U32,U16> ButtonEvent;

/// void event(DeviceId,U32 modifier,U32 action,U32 axis,F32 value)
typedef JournaledSignal<DeviceId,U32,U32,U32,F32> LinearEvent;

/// void event(DeviceId,U32 modifier,F32 value)
typedef JournaledSignal<DeviceId,U32,F32> PovEvent;

/// void event(DeviceId,InputAppMessage)
typedef JournaledSignal<DeviceId,U16> AppEvent;

/// void event(DeviceId)
typedef JournaledSignal<DeviceId> DisplayEvent;

/// void event(DeviceId, S32 width, S32 height)
typedef JournaledSignal<DeviceId, S32, S32> ResizeEvent;

/// void event(S32 timeDelta)
typedef JournaledSignal<S32> TimeManagerEvent;

// void event(U32 deviceInst,F32 fValue, U16 deviceType, U16 objType, U16 ascii, U16 objInst, U8 action, U8 modifier)
//typedef JournaledSignal<U32,F32,U16,U16,U16,U16,U8,U8> InputEvent;

#endif