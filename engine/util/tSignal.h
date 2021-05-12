//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_SIGNAL_H_
#define _UTIL_SIGNAL_H_

#include "platform/types.h"

/// Support for broadcasted callbacks.
///
/// When a signal is triggered, all registered callbacks are invoked
/// in the order that they were registered. Both static functions and method
/// pointers are supported as long as they match the Signal's argument and
/// return signature.
/// @ingroup UtilEvent
template <class A = EmptyType, class B = EmptyType, class C = EmptyType, class D = EmptyType, 
          class E = EmptyType, class F = EmptyType, class G = EmptyType, class H = EmptyType>
class Signal
{
public:
   
   Signal() 
   {
      _list.next = _list.prev = &_list;
      _list.order = 0.0f;  // cosmetic only
   }

   ~Signal() 
   {
      while (_list.next != &_list) 
      {
         DelegateLink* ptr = _list.next;
         ptr->unlink();
         delete ptr;
      }
   }

   inline bool isEmpty() 
   {
      return _list.next == &_list;
   }

   template <class T,class U>
   void notify(T obj,U func, F32 order = 0.5f) 
   {
      _list.insert(new DelegateMethod<T,U>(obj,func), order);
   }

   template <class T>
   void notify(T func, F32 order = 0.5f) 
   {
      _list.insert(new DelegateFunction<T>(func), order);
   }

   template <class T>
   void remove(T func) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         if (DelegateFunction<T>* fd = dynamic_cast<DelegateFunction<T>*>(ptr))
            if (fd->func == func) 
            {
               fd->unlink();
               delete fd;
               return;
            }
   }

   template <class T,class U>
   void remove(T obj,U func) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         if (DelegateMethod<T,U>* fd = dynamic_cast<DelegateMethod<T,U>*>(ptr))
            if (fd->object == obj && fd->func == func) 
            {
               fd->unlink();
               delete fd;
               return;
            }
   }

   void trigger(A a,B b,C c,D d,E e,F f, G g, H h) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c,d,e,f,g,h);
   }

   void trigger(A a,B b,C c,D d,E e,F f, G g) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c,d,e,f,g);
   }

   void trigger(A a,B b,C c,D d,E e,F f) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c,d,e,f);
   }

   void trigger(A a,B b,C c,D d,E e) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c,d,e);
   }

   void trigger(A a,B b,C c,D d) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c,d);
   }

   void trigger(A a,B b,C c) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b,c);
   }

   void trigger(A a,B b) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a,b);
   }

   void trigger(A a) 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)(a);
   }

   void trigger() 
   {
      for (DelegateLink* ptr = _list.next; ptr != &_list; ptr = ptr->next)
         (*(DelegateBase<void,A,B,C,D,E,F,G,H>*)ptr)();
   }

private:
   // Delegate base class
   struct DelegateLink 
   {
      DelegateLink *next,*prev;
      F32 order;

      virtual ~DelegateLink() {}
      
      void insert(DelegateLink* node, F32 order) 
      {
         // Note: can only legitimately be called on list head
         DelegateLink * walk = next;
         while (order >= walk->order && walk->next != this)
            walk = walk->next;
         if (order >= walk->order)
         {
            // insert after walk
            node->prev = walk;
            node->next = walk->next;
            walk->next->prev = node;
            walk->next = node;
         }
         else
         {
            // insert before walk
            node->prev = walk->prev;
            node->next = walk;
            walk->prev->next = node;
            walk->prev = node;
         }
         node->order = order;
      }
      
      void unlink() 
      {
         // Unlink this node
         next->prev = prev;
         prev->next = next;
      }
   };

   // Base class with virtual operator(), no other way to deal with
   // multiple arguments except to specialize. Don't really need the
   // class RR return type, but specializing the class without it
   // was causing problems with gcc 3.3.1
   template <class RR, class AA, class BB, class CC, class DD,
                       class EE, class FF, class GG, class HH>
   struct DelegateBase: public DelegateLink {
      virtual RR operator()(AA,BB,CC,DD,EE,FF,GG,HH) = 0;
   };

   template <class RR, class AA, class BB, class CC, class DD,
                       class EE, class FF, class GG>
   struct DelegateBase<RR,AA,BB,CC,DD,EE,FF,GG,
                        EmptyType>: public DelegateLink {
      virtual RR operator()(AA,BB,CC,DD,EE,FF,GG) = 0;
   };

   template <class RR, class AA, class BB, class CC, class DD, 
                       class EE, class FF>
   struct DelegateBase<RR,AA,BB,CC,DD,EE,FF,
                       EmptyType,EmptyType>: public DelegateLink {
      virtual RR operator()(AA,BB,CC,DD,EE,FF) = 0;
   };

   template <class RR, class AA, class BB, class CC, class DD,
                       class EE>
   struct DelegateBase<RR,AA,BB,CC,DD,EE,
                       EmptyType,EmptyType,EmptyType> : public DelegateLink {
      virtual RR operator()(AA,BB,CC,DD,EE) = 0;
   };

   template <class RR, class AA, class BB, class CC, class DD>
   struct DelegateBase<RR,AA,BB,CC,DD,EmptyType,EmptyType,
                       EmptyType,EmptyType>: public DelegateLink {
      virtual RR operator()(AA,BB,CC,DD) = 0;
   };

   template <class RR, class AA, class BB, class CC>
   struct DelegateBase<RR,AA,BB,CC,EmptyType,EmptyType,
                       EmptyType,EmptyType,EmptyType>: public DelegateLink 
   {
      virtual RR operator()(AA,BB,CC) = 0;
   };

   template <class RR, class AA, class BB>
   struct DelegateBase<RR,AA,BB,EmptyType,EmptyType,EmptyType,EmptyType,
                       EmptyType,EmptyType>: public DelegateLink 
   {
      virtual RR operator()(AA,BB) = 0;
   };

   template <class RR, class AA>
   struct DelegateBase<RR,AA,EmptyType,EmptyType,EmptyType,
                       EmptyType,EmptyType,EmptyType,EmptyType>: public DelegateLink 
   {
      virtual RR operator()(AA) = 0;
   };

   template <class RR>
   struct DelegateBase<RR,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType,
                       EmptyType,EmptyType,EmptyType> : public DelegateLink {
      virtual RR operator()() = 0;
   };

   // Member function pointers
   template <class T, class U>
   struct DelegateMethod: public DelegateBase<void,A,B,C,D,E,F,G,H> {
      T object;
      U func;
      DelegateMethod(T ptr,U ff): object(ptr),func(ff) {}
      void operator()(A a,B b,C c,D d,E e,F f,G g,H h) { return (object->*func)(a,b,c,d,e,f,g,h); }
      void operator()(A a,B b,C c,D d,E e,F f,G g) { return (object->*func)(a,b,c,d,e,f,g); }
      void operator()(A a,B b,C c,D d,E e,F f) { return (object->*func)(a,b,c,d,e,f); }
      void operator()(A a,B b,C c,D d,E e) { return (object->*func)(a,b,c,d,e); }
      void operator()(A a,B b,C c,D d) { return (object->*func)(a,b,c,d); }
      void operator()(A a,B b,C c) { return (object->*func)(a,b,c); }
      void operator()(A a,B b) { return (object->*func)(a,b); }
      void operator()(A a) { return (object->*func)(a); }
      void operator()() { return (object->*func)(); }
   };

   // Static function pointers
   template <class T>
   struct DelegateFunction: public DelegateBase<void,A,B,C,D,E,F,G,H> {
      T func;
      DelegateFunction(T ff): func(ff) {}
      void operator()(A a,B b,C c,D d,E e,F f,G g,H h) { return (*func)(a,b,c,d,e,f,g,h); }
      void operator()(A a,B b,C c,D d,E e,F f,G g) { return (*func)(a,b,c,d,e,f,g); }
      void operator()(A a,B b,C c,D d,E e,F f) { return (*func)(a,b,c,d,e,f); }
      void operator()(A a,B b,C c,D d,E e) { return (*func)(a,b,c,d,e); }
      void operator()(A a,B b,C c,D d) { return (*func)(a,b,c,d); }
      void operator()(A a,B b,C c) { return (*func)(a,b,c); }
      void operator()(A a,B b) { return (*func)(a,b); }
      void operator()(A a) { return (*func)(a); }
      void operator()() { return (*func)(); }
   };

   //
   DelegateLink _list;
};

#endif