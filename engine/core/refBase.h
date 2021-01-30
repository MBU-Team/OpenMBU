//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2004 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _REFBASE_H_
#define _REFBASE_H_

class SafeObjectRef;

class RefBase
{
    SafeObjectRef* mFirstObjectRef; ///< the head of the linked list of safe object references
    friend class SafeObjectRef;
    friend class RefObjectRef;
public:
    U32 mRefCount; ///< reference counter for RefPtr objects
    RefBase() { mRefCount = 0; mFirstObjectRef = NULL; }
    virtual ~RefBase();

    /// object destroy self call (from RefPtr).  Override if this class has specially allocated memory.
    virtual void destroySelf() { delete this; }
};

/// Base class for RefBase reference counting
class RefObjectRef
{
protected:
    RefBase* mObject; ///< the object this RefObjectRef references

    /// increments the reference count on the referenced object
    void incRef()
    {
        if (mObject)
            mObject->mRefCount++;
    }

    /// decrements the reference count on the referenced object
    void decRef()
    {
        if (mObject)
        {
            if (!--mObject->mRefCount)
                mObject->destroySelf();
        }
    }
public:

    /// Constructor, assings from the object and increments its reference count if it's not NULL
    RefObjectRef(RefBase* object = NULL)
    {
        mObject = object;
        incRef();
    }

    /// Destructor, dereferences the object, if there is one
    ~RefObjectRef()
    {
        decRef();
    }

    /// Assigns this reference object from an existing RefBase instance
    void set(RefBase* object)
    {
        if (mObject != object)
        {
            decRef();
            mObject = object;
            incRef();
        }
    }
};

/// Reference counted object template pointer class
/// Instances of this template class can be used as pointers to
/// instances of RefBase and its subclasses.  The object will not
/// be deleted until all of the RefPtr instances pointing to it
/// have been destructed.
template <class T> class RefPtr : public RefObjectRef
{
public:
    RefPtr() : RefObjectRef() {}
    RefPtr(T* ptr) : RefObjectRef(ptr) {}
    RefPtr(const RefPtr<T>& ref) : RefObjectRef((T*)ref.mObject) {}

    RefPtr<T>& operator=(const RefPtr<T>& ref)
    {
        set((T*)ref.mObject);
        return *this;
    }
    RefPtr<T>& operator=(T* ptr)
    {
        set(ptr);
        return *this;
    }
    bool operator == (const RefBase* ptr) const { return mObject == ptr; }
    bool operator != (const RefBase* ptr) const { return mObject != ptr; }
    bool isNull()  const { return mObject == 0; }
    bool isValid() const { return mObject != 0; }
    T* operator->() const { return static_cast<T*>(mObject); }
    T& operator*() const { return *static_cast<T*>(mObject); }
    operator T* () { return static_cast<T*>(mObject); }
    //operator T*() const   { return static_cast<T*>(mObject); }
    T* getPointer() const { return static_cast<T*>(mObject); }
};

/// Base class for RefBase safe pointers
class SafeObjectRef
{
    friend class RefBase;
protected:
    RefBase* mObject; ///< The object this is a safe pointer to, or NULL if the object has been deleted
    SafeObjectRef* mPrevObjectRef; ///< The previous SafeObjectRef for mObject
    SafeObjectRef* mNextObjectRef; ///< The next SafeObjectRef for mObject
public:
    SafeObjectRef(RefBase* object);
    SafeObjectRef();
    void set(RefBase* object);
    ~SafeObjectRef();

    void registerReference(); ///< Links this SafeObjectRef into the doubly linked list of SafeObjectRef instances for mObject
    void unregisterReference(); ///< Unlinks this SafeObjectRef from the doubly linked list of SafeObjectRef instance for mObject
};

inline RefBase::~RefBase()
{
    SafeObjectRef* walk = mFirstObjectRef;
    while (walk)
    {
        // get walk's next pointer now because it will null out its list
        // pointers when it unregisters.
        SafeObjectRef* walkNext = walk->mNextObjectRef;
        walk->set(NULL);
        walk = walkNext;
    }
};

inline void SafeObjectRef::unregisterReference()
{
    if (mObject)
    {
        if (mPrevObjectRef)
            mPrevObjectRef->mNextObjectRef = mNextObjectRef;
        else
            mObject->mFirstObjectRef = mNextObjectRef;
        if (mNextObjectRef)
            mNextObjectRef->mPrevObjectRef = mPrevObjectRef;

        // we are no longer linked into the reference list, so clear our pointers
        mNextObjectRef = NULL;
        mPrevObjectRef = NULL;
        mObject = NULL;
    }
}

inline void SafeObjectRef::registerReference()
{
    if (mObject)
    {
        mNextObjectRef = mObject->mFirstObjectRef;
        if (mNextObjectRef)
            mNextObjectRef->mPrevObjectRef = this;
        mPrevObjectRef = NULL;
        mObject->mFirstObjectRef = this;
    }
}

inline void SafeObjectRef::set(RefBase* object)
{
    unregisterReference();
    mObject = object;
    registerReference();
}

inline SafeObjectRef::~SafeObjectRef()
{
    unregisterReference();
}

inline SafeObjectRef::SafeObjectRef(RefBase* object)
{
    mObject = object;
    registerReference();
}

inline SafeObjectRef::SafeObjectRef()
{
    mObject = NULL;
}

/// Safe object template pointer class.
/// Instances of this template class can be used as pointers to
/// instances of RefBase and its subclasses.
/// When the object referenced by a SafePtr instance is deleted,
/// the pointer to the object is set to NULL in the SafePtr instance.
template <class T> class SafePtr : public SafeObjectRef
{
public:
    SafePtr() : SafeObjectRef() {}
    SafePtr(T* ptr) : SafeObjectRef(ptr) {}
    SafePtr(const SafePtr<T>& ref) : SafeObjectRef((T*)ref.mObject) {}

    SafePtr<T>& operator=(const SafePtr<T>& ref)
    {
        set((T*)ref.mObject);
        return *this;
    }
    SafePtr<T>& operator=(T* ptr)
    {
        set(ptr);
        return *this;
    }
    bool operator == (const RefBase* ptr) { return mObject == ptr; }
    bool operator != (const RefBase* ptr) { return mObject != ptr; }
    bool isNull() const { return mObject == 0; }
    T* operator->() const { return static_cast<T*>(mObject); }
    T& operator*() const { return *static_cast<T*>(mObject); }
    operator T* () const { return static_cast<T*>(mObject); }
    operator T* () { return reinterpret_cast<T*>(mObject); }
    T* getPointer() { return static_cast<T*>(mObject); }
};

#endif
