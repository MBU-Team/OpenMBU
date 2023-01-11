//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _UTIL_DELEGATE_H_
#define _UTIL_DELEGATE_H_

/// Function and object method callbacks.
/// Delegates are essentially the same as function pointers, except that they
/// support both static functions and pointers to methods. A delegate can
/// reference the method of any class or static function as long as the
/// argument and return signature match.
/// @ingroup UtilEvent
template <class R, class A = EmptyType, class B = EmptyType, class C = EmptyType,
   class D = EmptyType, class E = EmptyType>
class Delegate
{
public:
   Delegate()
   {
      // BJGNOTE - this is here to prevent Vector<> from doing bad stuff.
      _delegate = (DelegateBase<R,A,B,C,D,E>*)new DelegateRef();
   }
   Delegate(const Delegate& d) {
      _delegate = d._delegate;
      _delegate->refcount++;
   }
   template <class T,class U>
   Delegate(T obj,U func) {
      _delegate = new DelegateMethod<T,U>(obj,func);
   }
   template <class T>
   Delegate(T func) {
      _delegate = new DelegateFunction<T>(func);
   }
   ~Delegate() {
      if (!--_delegate->refcount)
         delete _delegate;
   }
   Delegate& operator = (const Delegate& d) {
      d._delegate->refcount++;
      if (!--_delegate->refcount)
         delete _delegate;
      _delegate = d._delegate;
      return *this;
   }
   bool operator == (const Delegate& obj) const {
      return *_delegate == *obj._delegate;
   }
   bool operator != (const Delegate& obj) const {
      return !operator==(obj);
   }

   //
   R operator()(A a,B b,C c,D d,E e) const { return (*_delegate)(a,b,c,d,e); }
   R operator()(A a,B b,C c,D d) const { return (*_delegate)(a,b,c,d); }
   R operator()(A a,B b,C c) const { return (*_delegate)(a,b,c); }
   R operator()(A a,B b) const { return (*_delegate)(a,b); }
   R operator()(A a) const { return (*_delegate)(a); }
   R operator()() const { return (*_delegate)(); }

private:
   // Delegate base class
   struct DelegateRef {
      int refcount;
      DelegateRef() { refcount = 1; }
      virtual ~DelegateRef() {}
      virtual bool operator == (const DelegateRef&) const { return false; };
   };

   // Base class with virtual operator(), no other way to deal with
   // multiple arguments except to specialize.
   template <class RR, class AA, class BB, class CC, class DD, class EE>
   struct DelegateBase: public DelegateRef {
      virtual RR operator()(AA,BB,CC,DD,EE) = 0;
   };
   template <class RR, class AA, class BB, class CC, class DD>
   struct DelegateBase<RR,AA,BB,CC,DD,EmptyType>: public DelegateRef {
      virtual RR operator()(AA,BB,CC,DD) = 0;
   };
   template <class RR, class AA, class BB, class CC>
   struct DelegateBase<RR,AA,BB,CC,EmptyType,EmptyType>: public DelegateRef {
      virtual RR operator()(AA,BB,CC) = 0;
   };
   template <class RR, class AA, class BB>
   struct DelegateBase<RR,AA,BB,EmptyType,EmptyType,EmptyType>: public DelegateRef {
      virtual RR operator()(AA,BB) = 0;
   };
   template <class RR, class AA>
   struct DelegateBase<RR,AA,EmptyType,EmptyType,EmptyType,EmptyType>: public DelegateRef {
      virtual RR operator()(AA) = 0;
   };
   template <class RR>
   struct DelegateBase<RR,EmptyType,EmptyType,EmptyType,EmptyType,EmptyType>: public DelegateRef {
      virtual RR operator()() = 0;
   };

   // Member function pointers
   template <class T, class U>
   struct DelegateMethod: public DelegateBase<R,A,B,C,D,E> {
      T object;
      U func;
      DelegateMethod(T ptr,U ff): object(ptr),func(ff) {}
      bool operator == (const DelegateRef& ref) const {
         if (const DelegateMethod* obj = dynamic_cast<const DelegateMethod*>(&ref))
            return object == obj->object && func == obj->func;
         return false;
      }
      //bool operator == (const DelegateMethod& obj) const {
      //   return object == obj.object && func == obj.func;
      //}
      R operator()(A a,B b,C c,D d,E e) { return (object->*func)(a,b,c,d,e); }
      R operator()(A a,B b,C c,D d) { return (object->*func)(a,b,c,d); }
      R operator()(A a,B b,C c) { return (object->*func)(a,b,c); }
      R operator()(A a,B b) { return (object->*func)(a,b); }
      R operator()(A a) { return (object->*func)(a); }
      R operator()() { return (object->*func)(); }
   };

   // Static function pointers
   template <class T>
   struct DelegateFunction: public DelegateBase<R,A,B,C,D,E> {
      T func;
      DelegateFunction(T ff): func(ff) {}
      bool operator == (const DelegateRef& ref) const {
         if (const DelegateFunction* obj = dynamic_cast<const DelegateFunction*>(&ref))
            return func == obj->func;
         return false;
      }
      //bool operator == (const DelegateFunction& obj) const {
      //   return func == obj.func;
      //}
      R operator()(A a,B b,C c,D d,E e) { return (*func)(a,b,c,d,e); }
      R operator()(A a,B b,C c,D d) { return (*func)(a,b,c,d); }
      R operator()(A a,B b,C c) { return (*func)(a,b,c); }
      R operator()(A a,B b) { return (*func)(a,b); }
      R operator()(A a) { return (*func)(a); }
      R operator()() { return (*func)(); }
   };

   DelegateBase<R,A,B,C,D,E>* _delegate;
};

#endif