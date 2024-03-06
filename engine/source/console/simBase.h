//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SIMBASE_H_
#define _SIMBASE_H_

#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _TALGORITHM_H_
#include "core/tAlgorithm.h"
#endif
#ifndef _BITSET_H_
#include "core/bitSet.h"
#endif

#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif
#ifndef _SIMDICTIONARY_H_
#include "console/simDictionary.h"
#endif

#ifndef _PLATFORMMUTEX_H_
#include "platform/platformMutex.h"
#endif

#ifndef _PLATFORMSEMAPHORE_H_
#include "platform/platformSemaphore.h"
#endif

//---------------------------------------------------------------------------

/// Definition of some basic Sim system constants.
///
/// These constants define the range of ids assigned to datablocks
/// (DataBlockObjectIdFirst - DataBlockObjectIdLast), and the number
/// of bits used to store datablock IDs.
///
/// Normal Sim objects are given the range of IDs starting at
/// DynamicObjectIdFirst and going to infinity. Sim objects use
/// a SimObjectId to represent their ID; this is currently a U32.
///
/// The RootGroupId is assigned to gRootGroup, in which most SimObjects
/// are addded as child members. See simManager.cc for details, particularly
/// Sim::initRoot() and following.
enum SimObjectsConstants
{
    DataBlockObjectIdFirst = 3,
    DataBlockObjectIdBitSize = 10,
    DataBlockObjectIdLast = DataBlockObjectIdFirst + (1 << DataBlockObjectIdBitSize) - 1,

    DynamicObjectIdFirst = DataBlockObjectIdLast + 1,
    InvalidEventId = 0,
    RootGroupId = 0xFFFFFFFF,
};

class SimEvent;
class SimObject;
class SimGroup;
class SimManager;
class Namespace;
class BitStream;
class Stream;
class LightManager;

typedef U32 SimTime;
typedef U32 SimObjectId;

/// A vector of SimObjects.
///
/// As this inherits from VectorPtr, it has the full range of vector methods.
class SimObjectList : public VectorPtr<SimObject*>
{
    static S32 QSORT_CALLBACK compareId(const void* a, const void* b);
public:
    void pushBack(SimObject*);       ///< Add the SimObject* to the end of the list, unless it's already in the list.
    void pushBackForce(SimObject*);  ///< Add the SimObject* to the end of the list, moving it there if it's already present in the list.
    void pushFront(SimObject*);      ///< Add the SimObject* to the start of the list.
    void remove(SimObject*);         ///< Remove the SimObject* from the list; may disrupt order of the list.

    SimObject* at(S32 index) const { if (index >= 0 && index < size()) return (*this)[index]; return NULL; }

    /// Remove the SimObject* from the list; guaranteed to preserve list order.
    void removeStable(SimObject* pObject);

    void sortId();                   ///< Sort the list by object ID.
};

//---------------------------------------------------------------------------

/// Represents a queued event in the sim.
///
/// Sim provides an event queue for your convenience, which
/// can be used to schedule events. A few things which use
/// this event queue:
///
///     - The scene lighting system. In order to keep the game
///       responsive while scene lighting occurs, the lighting
///       process is divided into little chunks. In implementation
///       terms, there is a subclass of SimEvent called
///       SceneLightingProcessEvent. The process method of this
///       subclass calls into the lighting code, telling it to
///       perform the next chunk of lighting calculations.
///     - The schedule() console function uses a subclass of
///       SimEvent called SimConsoleEvent to keep track of
///       scheduled events.
class SimEvent
{
public:
    SimEvent* nextEvent;     ///< Linked list details - pointer to next item in the list.
    SimTime startTime;       ///< When the event was posted.
    SimTime time;            ///< When the event is scheduled to occur.
    U32 sequenceCount;       ///< Unique ID. These are assigned sequentially based on order
                             ///  of addition to the list.
    SimObject* destObject;   ///< Object on which this event will be applied.

    SimEvent() { destObject = NULL; }
    virtual ~SimEvent() {}   ///< Destructor
                             ///
                             /// A dummy virtual destructor is required
                             /// so that subclasses can be deleted properly

    /// Function called when event occurs.
    ///
    /// This is where the meat of your event's implementation goes.
    ///
    /// See any of the subclasses for ideas of what goes in here.
    ///
    /// The event is deleted immediately after processing. If the
    /// object referenced in destObject is deleted, then the event
    /// is not called. The even will be executed unconditionally if
    /// the object referenced is NULL.
    ///
    /// @param   object  Object stored in destObject.
    virtual void process(SimObject* object) = 0;
};

/// Implementation of schedule() function.
///
/// This allows you to set a console function to be
/// called at some point in the future.
class SimConsoleEvent : public SimEvent
{
protected:
    S32 mArgc;
    char** mArgv;
    bool mOnObject;
public:

    /// Constructor
    ///
    /// Pass the arguments of a function call, optionally on an object.
    ///
    /// The object for the call to be executed on is specified by setting
    /// onObject and storing a reference to the object in destObject. If
    /// onObject is false, you don't need to store anything into destObject.
    ///
    /// The parameters here are passed unmodified to Con::execute() at the
    /// time of the event.
    ///
    /// @see Con::execute(S32 argc, const char *argv[])
    /// @see Con::execute(SimObject *object, S32 argc, const char *argv[])
    SimConsoleEvent(S32 argc, const char** argv, bool onObject);

    ~SimConsoleEvent();
    virtual void process(SimObject* object);
};

/// Used by Con::threadSafeExecute()
struct SimConsoleThreadExecCallback
{
    void* sem;
    const char* retVal;

    SimConsoleThreadExecCallback();
    ~SimConsoleThreadExecCallback();

    void handleCallback(const char* ret);
    const char* waitForResult();
};

class SimConsoleThreadExecEvent : public SimConsoleEvent
{
    SimConsoleThreadExecCallback* cb;

public:
    SimConsoleThreadExecEvent(S32 argc, const char** argv, bool onObject, SimConsoleThreadExecCallback* callback);

    virtual void process(SimObject* object);
};


//---------------------------------------------------------------------------
/// Dictionary to keep track of dynamic fields on SimObject.
class SimFieldDictionary
{
    friend class SimFieldDictionaryIterator;

public:
    struct Entry
    {
        StringTableEntry slotName;
        char* value;
        Entry* next;
    };
    enum
    {
        HashTableSize = 19
    };
private:
    Entry* mHashTable[HashTableSize];

    static Entry* mFreeList;
    static void freeEntry(Entry* entry);
    static Entry* allocEntry();

    /// In order to efficiently detect when a dynamic field has been
    /// added or deleted, we increment this every time we add or
    /// remove a field.
    U32 mVersion;

public:
    const U32 getVersion() const { return mVersion; }

    SimFieldDictionary();
    ~SimFieldDictionary();
    void setFieldValue(StringTableEntry slotName, const char* value);
    const char* getFieldValue(StringTableEntry slotName);
    void writeFields(SimObject* obj, Stream& strem, U32 tabStop);
    void printFields(SimObject* obj);
    void assignFrom(SimFieldDictionary* dict);
};

class SimFieldDictionaryIterator
{
    SimFieldDictionary* mDictionary;
    S32                           mHashIndex;
    SimFieldDictionary::Entry* mEntry;

public:
    SimFieldDictionaryIterator(SimFieldDictionary*);
    SimFieldDictionary::Entry* operator++();
    SimFieldDictionary::Entry* operator*();
};

//---------------------------------------------------------------------------
/// Base class for objects involved in the simulation.
///
/// @section simobject_intro Introduction
///
/// SimObject is a base class for most of the classes you'll encounter
/// working in Torque. It provides fundamental services allowing "smart"
/// object referencing, creation, destruction, organization, and location.
/// Along with SimEvent, it gives you a flexible event-scheduling system,
/// as well as laying the foundation for the in-game editors, GUI system,
/// and other vital subsystems.
///
/// @section simobject_subclassing Subclassing
///
/// You will spend a lot of your time in Torque subclassing, or working
/// with subclasses of, SimObject. SimObject is designed to be easy to
/// subclass.
///
/// You should not need to override anything in a subclass except:
///     - The constructor/destructor.
///     - processArguments()
///     - onAdd()/onRemove()
///     - onGroupAdd()/onGroupRemove()
///     - onNameChange()
///     - onStaticModified()
///     - onDeleteNotify()
///     - onEditorEnable()/onEditorDisable()
///     - inspectPreApply()/inspectPostApply()
///     - things from ConsoleObject (see ConsoleObject docs for specifics)
///
/// Of course, if you know what you're doing, go nuts! But in most cases, you
/// shouldn't need to touch things not on that list.
///
/// When you subclass, you should define a typedef in the class, called Parent,
/// that references the class you're inheriting from.
///
/// @code
/// class mySubClass : public SimObject {
///     typedef SimObject Parent;
///     ...
/// @endcode
///
/// Then, when you override a method, put in:
///
/// @code
/// bool mySubClass::onAdd()
/// {
///     if(!Parent::onAdd())
///         return false;
///
///     // ... do other things ...
/// }
/// @endcode
///
/// Of course, you want to replace onAdd with the appropriate method call.
///
/// @section simobject_lifecycle A SimObject's Life Cycle
///
/// SimObjects do not live apart. One of the primary benefits of using a
/// SimObject is that you can uniquely identify it and easily find it (using
/// its ID). Torque does this by keeping a global hierarchy of SimGroups -
/// a tree - containing every registered SimObject. You can then query
/// for a given object using Sim::findObject() (or SimSet::findObject() if
/// you want to search only a specific set).
///
/// @code
///        // Three examples of registering an object.
///
///        // Method 1:
///        AIClient *aiPlayer = new AIClient();
///        aiPlayer->registerObject();
///
///        // Method 2:
///        ActionMap* globalMap = new ActionMap;
///        globalMap->registerObject("GlobalActionMap");
///
///        // Method 3:
///        bool reg = mObj->registerObject(id);
/// @endcode
///
/// Registering a SimObject performs these tasks:
///     - Marks the object as not cleared and not removed.
///     - Assigns the object a unique SimObjectID if it does not have one already.
///     - Adds the object to the global name and ID dictionaries so it can be found
///       again.
///     - Calls the object's onAdd() method. <b>Note:</b> SimObject::onAdd() performs
///       some important initialization steps. See @ref simobject_subclassing "here
///       for details" on how to properly subclass SimObject.
///     - If onAdd() fails (returns false), it calls unregisterObject().
///     - Checks to make sure that the SimObject was properly initialized (and asserts
///       if not).
///
/// Calling registerObject() and passing an ID or a name will cause the object to be
/// assigned that name and/or ID before it is registered.
///
/// Congratulations, you have now registered your object! What now?
///
/// Well, hopefully, the SimObject will have a long, useful life. But eventually,
/// it must die.
///
/// There are a two ways a SimObject can die.
///         - First, the game can be shut down. This causes the root SimGroup
///           to be unregistered and deleted. When a SimGroup is unregistered,
///           it unregisters all of its member SimObjects; this results in everything
///           that has been registered with Sim being unregistered, as everything
///           registered with Sim is in the root group.
///         - Second, you can manually kill it off, either by calling unregisterObject()
///           or by calling deleteObject().
///
/// When you unregister a SimObject, the following tasks are performed:
///     - The object is flagged as removed.
///     - Notifications are cleaned up.
///     - If the object is in a group, then it removes itself from the group.
///     - Delete notifications are sent out.
///     - Finally, the object removes itself from the Sim globals, and tells
///       Sim to get rid of any pending events for it.
///
/// If you call deleteObject(), all of the above tasks are performed, in addition
/// to some sanity checking to make sure the object was previously added properly,
/// and isn't in the process of being deleted. After the object is unregistered, it
/// deallocates itself.
///
/// @section simobject_editor Torque Editors
///
/// SimObjects are one of the building blocks for the in-game editors. They
/// provide a basic interface for the editor to be able to list the fields
/// of the object, update them safely and reliably, and inform the object
/// things have changed.
///
/// This interface is implemented in the following areas:
///     - onNameChange() is called when the object is renamed.
///     - onStaticModified() is called whenever a static field is modified.
///     - inspectPreApply() is called before the object's fields are updated,
///                     when changes are being applied.
///     - inspectPostApply() is called after the object's fields are updated.
///     - onEditorEnable() is called whenever an editor is enabled (for instance,
///                     when you hit F11 to bring up the world editor).
///     - onEditorDisable() is called whenever the editor is disabled (for instance,
///                     when you hit F11 again to close the world editor).
///
/// (Note: you can check the variable gEditingMission to see if the mission editor
/// is running; if so, you may want to render special indicators. For instance, the
/// fxFoliageReplicator renders inner and outer radii when the mission editor is
/// runnning.)
///
/// @section simobject_console The Console
///
/// SimObject extends ConsoleObject by allowing you to
/// to set arbitrary dynamic fields on the object, as well as
/// statically defined fields. This is done through two methods,
/// setDataField and getDataField, which deal with the complexities of
/// allowing access to two different types of object fields.
///
/// Static fields take priority over dynamic fields. This is to be
/// expected, as the role of dynamic fields is to allow data to be
/// stored in addition to the predefined fields.
///
/// The fields in a SimObject are like properties (or fields) in a class.
///
/// Some fields may be arrays, which is what the array parameter is for; if it's non-null,
/// then it is parsed with dAtoI and used as an index into the array. If you access something
/// as an array which isn't, then you get an empty string.
///
/// <b>You don't need to read any further than this.</b> Right now,
/// set/getDataField are called a total of 6 times through the entire
/// Torque codebase. Therefore, you probably don't need to be familiar
/// with the details of accessing them. You may want to look at Con::setData
/// instead. Most of the time you will probably be accessing fields directly,
/// or using the scripting language, which in either case means you don't
/// need to do anything special.
///
/// The functions to get/set these fields are very straightforward:
///
/// @code
///  setDataField(StringTable->insert("locked", false), NULL, b ? "true" : "false" );
///  curObject->setDataField(curField, curFieldArray, STR.getStringValue());
///  setDataField(slotName, array, value);
/// @endcode
///
/// <i>For advanced users:</i> There are two flags which control the behavior
/// of these functions. The first is ModStaticFields, which controls whether
/// or not the DataField functions look through the static fields (defined
/// with addField; see ConsoleObject for details) of the class. The second
/// is ModDynamicFields, which controls dynamically defined fields. They are
/// set automatically by the console constructor code.
///
/// @nosubgrouping
class SimObject : public ConsoleObject
{
    typedef ConsoleObject Parent;

    friend class SimManager;
    friend class SimGroup;
    friend class SimNameDictionary;
    friend class SimManagerNameDictionary;
    friend class SimIdDictionary;

    //-------------------------------------- Structures and enumerations
private:

    /// Flags for use in mFlags
    enum {
        Deleted = BIT(0),   ///< This object is marked for deletion.
        Removed = BIT(1),   ///< This object has been unregistered from the object system.
        Added = BIT(3),   ///< This object has been registered with the object system.
        Selected = BIT(4),   ///< This object has been marked as selected. (in editor)
        Expanded = BIT(5),   ///< This object has been marked as expanded. (in editor)
        ModStaticFields = BIT(6),    ///< The object allows you to read/modify static fields
        ModDynamicFields = BIT(7)     ///< The object allows you to read/modify dynamic fields
    };
public:
    /// @name Notification
    /// @{
    struct Notify {
        enum Type {
            ClearNotify,   ///< Notified when the object is cleared.
            DeleteNotify,  ///< Notified when the object is deleted.
            ObjectRef,     ///< Cleverness to allow tracking of references.
            Invalid        ///< Mark this notification as unused (used in freeNotify).
        } type;
        void* ptr;        ///< Data (typically referencing or interested object).
        Notify* next;     ///< Next notification in the linked list.
    };

    /// @}

    enum WriteFlags {
        SelectedOnly = BIT(0) ///< Passed to SimObject::write to indicate that only objects
                              ///  marked as selected should be outputted. Used in SimSet.
    };

private:
    // dictionary information stored on the object
    StringTableEntry objectName;
    SimObject* nextNameObject;
    SimObject* nextManagerNameObject;
    SimObject* nextIdObject;

    SimGroup* mGroup;  ///< SimGroup we're contained in, if any.
    BitSet32    mFlags;

    /// @name Notification
    /// @{
    Notify* mNotifyList;
    /// @}

protected:
    SimObjectId mId;         ///< Id number for this object.
    Namespace* mNameSpace;
    U32         mTypeMask;

protected:
    /// @name Notification
    /// Helper functions for notification code.
    /// @{

    static SimObject::Notify* mNotifyFreeList;
    static SimObject::Notify* allocNotify();     ///< Get a free Notify structure.
    static void freeNotify(SimObject::Notify*);  ///< Mark a Notify structure as free.

    /// @}

private:
    SimFieldDictionary* mFieldDictionary;    ///< Storage for dynamic fields.

public:
    /// @name Accessors
    /// @{

    /// Get the value of a field on the object.
    ///
    /// See @ref simobject_console "here" for a detailed discussion of what this
    /// function does.
    ///
    /// @param   slotName    Field to access.
    /// @param   array       String containing index into array
    ///                      (if field is an array); if NULL, it is ignored.
    const char* getDataField(StringTableEntry slotName, const char* array);

    /// Set the value of a field on the object.
    ///
    /// See @ref simobject_console "here" for a detailed discussion of what this
    /// function does.
    ///
    /// @param   slotName    Field to access.
    /// @param   array       String containing index into array; if NULL, it is ignored.
    /// @param   value       Value to store.
    void setDataField(StringTableEntry slotName, const char* array, const char* value);

    /// Get reference to the dictionary containing dynamic fields.
    ///
    /// See @ref simobject_console "here" for a detailed discussion of what this
    /// function does.
    ///
    /// This dictionary can be iterated over using a SimFieldDictionaryIterator.
    SimFieldDictionary* getFieldDictionary() { return(mFieldDictionary); }

    /// Check if a method exists in the objects current namespace.
    bool isMethod(const char* methodName);
    /// @}

    /// @name Initialization
    /// @{

    ///
    SimObject();
    virtual ~SimObject();

    virtual bool processArguments(S32 argc, const char** argv);  ///< Process constructor options. (ie, new SimObject(1,2,3))

    /// @}

    /// @name Events
    /// @{
    virtual bool onAdd();                                ///< Called when the object is added to the sim.
    virtual void onRemove();                             ///< Called when the object is removed from the sim.
    virtual void onGroupAdd();                           ///< Called when the object is added to a SimGroup.
    virtual void onGroupRemove();                        ///< Called when the object is removed from a SimGroup.
    virtual void onNameChange(const char* name);         ///< Called when the object's name is changed.
    virtual void onStaticModified(const char* slotName); ///< Called when a static field is modified.
                                                         ///
                                                         ///  Specifically, this is called by setDataField
                                                         ///  when a static field is modified, see
                                                         ///  @ref simobject_console "the console details".

    /// Called before any property of the object is changed in the world editor.
    ///
    /// The calling order here is:
    ///  - inspectPreApply()
    ///  - ...
    ///  - calls to setDataField()
    ///  - ...
    ///  - inspectPostApply()
    virtual void inspectPreApply();

    /// Called after any property of the object is changed in the world editor.
    ///
    /// @see inspectPreApply
    virtual void inspectPostApply();

    /// Called when a SimObject is deleted.
    ///
    /// When you are on the notification list for another object
    /// and it is deleted, this method is called.
    virtual void onDeleteNotify(SimObject* object);

    /// Called when the editor is activated.
    virtual void onEditorEnable() {};

    /// Called when the editor is deactivated.
    virtual void onEditorDisable() {};

    /// @}

    /// Find a named sub-object of this object.
    ///
    /// This is subclassed in the SimGroup and SimSet classes.
    ///
    /// For a single object, it just returns NULL, as normal objects cannot have children.
    virtual SimObject* findObject(const char* name);

    /// @name Notification
    /// @{
    Notify* removeNotify(void* ptr, Notify::Type);   ///< Remove a notification from the list.
    void deleteNotify(SimObject* obj);               ///< Notify an object when we are deleted.
    void clearNotify(SimObject* obj);                ///< Notify an object when we are cleared.
    void clearAllNotifications();                    ///< Remove all notifications for this object.
    void processDeleteNotifies();                    ///< Send out deletion notifications.

    /// Register a reference to this object.
    ///
    /// You pass a pointer to your reference to this object.
    ///
    /// When the object is deleted, it will null your
    /// pointer, ensuring you don't access old memory.
    ///
    /// @param obj   Pointer to your reference to the object.
    void registerReference(SimObject** obj);

    /// Unregister a reference to this object.
    ///
    /// Remove a reference from the list, so that it won't
    /// get nulled inappropriately.
    ///
    /// Call this when you're done with your reference to
    /// the object, especially if you're going to free the
    /// memory. Otherwise, you may erroneously get something
    /// overwritten.
    ///
    /// @see registerReference
    void unregisterReference(SimObject** obj);

    /// @}

    /// @name Registration
    ///
    /// SimObjects must be registered with the object system.
    /// @{


    /// Register an object with the object system.
    ///
    /// This must be called if you want to keep the object around.
    /// In the rare case that you will delete the object immediately, or
    /// don't want to be able to use Sim::findObject to locate it, then
    /// you don't need to register it.
    ///
    /// registerObject adds the object to the global ID and name dictionaries,
    /// after first assigning it a new ID number. It calls onAdd(). If onAdd fails,
    /// it unregisters the object and returns false.
    ///
    /// If a subclass's onAdd doesn't eventually call SimObject::onAdd(), it will
    /// cause an assertion.
    bool registerObject();

    /// Register the object, forcing the id.
    ///
    /// @see registerObject()
    /// @param   id  ID to assign to the object.
    bool registerObject(U32 id);

    /// Register the object, assigning the name.
    ///
    /// @see registerObject()
    /// @param   name  Name to assign to the object.
    bool registerObject(const char* name);

    /// Register the object, assigning a name and ID.
    ///
    /// @see registerObject()
    /// @param   name  Name to assign to the object.
    /// @param   id  ID to assign to the object.
    bool registerObject(const char* name, U32 id);

    /// Unregister the object from Sim.
    ///
    /// This performs several operations:
    ///  - Sets the removed flag.
    ///  - Call onRemove()
    ///  - Clear out notifications.
    ///  - Remove the object from...
    ///      - its group, if any. (via getGroup)
    ///      - Sim::gNameDictionary
    ///      - Sim::gIDDictionary
    ///  - Finally, cancel any pending events for this object (as it can't receive them now).
    void unregisterObject();

    void deleteObject();     ///< Unregister, mark as deleted, and free the object.
                             ///
                             /// This helper function can be used when you're done with the object
                             /// and don't want to be bothered with the details of cleaning it up.

    /// @}

    /// @name Accessors
    /// @{
    SimObjectId getId() const { return mId; }
    const char* getIdString();
    U32         getType() const { return mTypeMask; }
    const char* getName() const { return objectName; };

    void setId(SimObjectId id);
    void assignName(const char* name);
    SimGroup* getGroup() const { return mGroup; }
    bool isProperlyAdded() const { return mFlags.test(Added); }
    bool isDeleted() const { return mFlags.test(Deleted); }
    bool isRemoved() const { return mFlags.test(Deleted | Removed); }
    bool isLocked();
    void setLocked(bool b);
    virtual bool isHidden();
    virtual void setHidden(bool b);

    /// @}

    /// @name Sets
    ///
    /// The object must be properly registered before you can add/remove it to/from a set.
    ///
    /// All these functions accept either a name or ID to identify the set you wish
    /// to operate on. Then they call addObject or removeObject on the set, which
    /// sets up appropriate notifications.
    ///
    /// An object may be in multiple sets at a time.
    /// @{
    bool addToSet(SimObjectId);
    bool addToSet(const char*);
    bool removeFromSet(SimObjectId);
    bool removeFromSet(const char*);

    /// @}

    /// @name Serialization
    /// @{

    /// Output the TorqueScript to recreate this object.
    ///
    /// This calls writeFields internally.
    /// @param   stream  Stream to output to.
    /// @param   tabStop Indentation level for this object.
    /// @param   flags   If SelectedOnly is passed here, then
    ///                  only objects marked as selected (using setSelected)
    ///                  will output themselves.
    virtual void write(Stream& stream, U32 tabStop, U32 flags = 0);

    /// Write the fields of this object in TorqueScript.
    ///
    /// @param   stream  Stream for output.
    /// @param   tabStop Indentation level for the fields.
    void writeFields(Stream& stream, U32 tabStop);

    /// Copy fields from another object onto this one.
    ///
    /// Objects must be of same type. Everything from obj
    /// will overwrite what's in this object; extra fields
    /// in this object will remain.
    ///
    /// @param   obj Object to copy from.
    void assignFieldsFrom(SimObject* obj);

    /// @}

    /// Return the object's namespace.
    Namespace* getNamespace() { return mNameSpace; }

    /// Get next matching item in namespace.
    ///
    /// This wraps a call to Namespace::tabComplete; it gets the
    /// next thing in the namespace, given a starting value
    /// and a base length of the string. See
    /// Namespace::tabComplete for details.
    const char* tabComplete(const char* prevText, S32 baseLen, bool);

    /// @name Accessors
    /// @{
    bool isSelected() const { return mFlags.test(Selected); }
    bool isExpanded() const { return mFlags.test(Expanded); }
    void setSelected(bool sel) { if (sel) mFlags.set(Selected); else mFlags.clear(Selected); }
    void setExpanded(bool exp) { if (exp) mFlags.set(Expanded); else mFlags.clear(Expanded); }
    void setModDynamicFields(bool dyn) { if (dyn) mFlags.set(ModDynamicFields); else mFlags.clear(ModDynamicFields); }
    void setModStaticFields(bool sta) { if (sta) mFlags.set(ModStaticFields); else mFlags.clear(ModStaticFields); }
    bool canModDynamicFields() { return mFlags.test(ModDynamicFields); }
    bool canModStaticFields() { return mFlags.test(ModStaticFields); }

    /// @}

    /// @name Initialization
    /// @{

    /// Called to register the object's lights, if any, with the LightManager.
    ///
    /// @param   lm               LightManager to put lights into.
    /// @param   lightingScene   True if we're currently calculating lighting information.
    virtual void registerLights(LightManager* lm, bool lightingScene) {};

    /// @}

    /// Dump the contents of this object to the console.  Use the Torque Script dump() and dumpF() functions to 
    /// call this.  
    void dumpToConsole(bool includeFunctions = true);

    static void initPersistFields();
    DECLARE_CONOBJECT(SimObject);
};

//---------------------------------------------------------------------------
/// Smart SimObject pointer.
///
/// This class keeps track of the book-keeping necessary
/// to keep a registered reference to a SimObject or subclass
/// thereof.
///
/// Normally, if you want the SimObject to be aware that you
/// have a reference to it, you must call SimObject::registerReference()
/// when you create the reference, and SimObject::unregisterReference() when
/// you're done. If you change the reference, you must also register/unregister
/// it. This is a big headache, so this class exists to automatically
/// keep track of things for you.
///
/// @code
///     // Assign an object to the
///     SimObjectPtr<GameBase> mOrbitObject = Sim::findObject("anObject");
///
///     // Use it as a GameBase*.
///     mOrbitObject->getWorldBox().getCenter(&mPosition);
///
///     // And reassign it - it will automatically update the references.
///     mOrbitObject = Sim::findObject("anotherObject");
/// @endcode
template <class T> class SimObjectPtr
{
private:
    SimObject* mObj;

public:
    SimObjectPtr() { mObj = 0; }
    SimObjectPtr(T* ptr)
    {
        mObj = ptr;
        if (mObj)
            mObj->registerReference(&mObj);
    }
    SimObjectPtr(const SimObjectPtr<T>& rhs)
    {
        mObj = const_cast<T*>(static_cast<const T*>(rhs));
        if (mObj)
            mObj->registerReference(&mObj);
    }
    SimObjectPtr<T>& operator=(const SimObjectPtr<T>& rhs)
    {
        if (this == &rhs)
            return(*this);
        if (mObj)
            mObj->unregisterReference(&mObj);
        mObj = const_cast<T*>(static_cast<const T*>(rhs));
        if (mObj)
            mObj->registerReference(&mObj);
        return(*this);
    }
    ~SimObjectPtr()
    {
        if (mObj)
            mObj->unregisterReference(&mObj);
    }
    SimObjectPtr<T>& operator= (T* ptr)
    {
        if (mObj != (SimObject*)ptr)
        {
            if (mObj)
                mObj->unregisterReference(&mObj);
            mObj = (SimObject*)ptr;
            if (mObj)
                mObj->registerReference(&mObj);
        }
        return *this;
    }
#if defined(__MWERKS__) && (__MWERKS__ < 0x2400)
    // CW 5.3 seems to get confused comparing SimObjectPtrs...
    bool operator == (const SimObject* ptr) { return mObj == ptr; }
    bool operator != (const SimObject* ptr) { return mObj != ptr; }
#endif
    bool isNull() const { return mObj == 0; }
    T* operator->() const { return static_cast<T*>(mObj); }
    T& operator*() const { return *static_cast<T*>(mObj); }
    operator T* () const { return static_cast<T*>(mObj) ? static_cast<T*>(mObj) : 0; }
};

//---------------------------------------------------------------------------
/// Root DataBlock class.
///
/// @section SimDataBlock_intro Introduction
///
/// Another powerful aspect of Torque's networking is the datablock. Datablocks
/// are used to provide relatively static information about entities; for instance,
/// what model a weapon should use to display itself, or how heavy a player class is.
///
/// This gives significant gains in network efficiency, because it means that all
/// the datablocks on a server can be transferred over the network at client
/// connect time, instead of being intertwined with the update code for NetObjects.
///
/// This makes the network code much simpler overall, as one-time initialization
/// code is segregated from the standard object update code, as well as providing
/// several powerful features, which we will discuss momentarily.
///
/// @section SimDataBlock_preload preload() and File Downloading
///
/// Because datablocks are sent over the wire, using SimDataBlockEvent, before
/// gameplay starts in earnest, we gain in several areas. First, we don't have to
/// try to keep up with the game state while working with incomplete information.
/// Second, we can provide the user with a nice loading screen, instead of the more
/// traditional "Connecting..." message. Finally, and most usefully, we can request
/// missing files from the server as we become aware of them, since we are under
/// no obligation to render anything for the user.
///
/// The mechanism for this is fairly basic. After a datablock is unpacked, the
/// preload() method is called. If it returns false and sets an error, then the
/// network code checks to see if a file (or files) failed to be located by the
/// ResManager; if so, then it requests those files from the server. If preload
/// returns true, then the datablock is considered loaded. If preload returns
/// false and sets no error, then the connection is aborted.
///
/// Once the file(s) is downloaded, the datablock's preload() method is called again.
/// If it fails with the same error, the connection is aborted. If a new error is
/// returned, then the download-retry process is repeated until the preload works.
///
/// @section SimDataBlock_guide Guide To Datablock Code
///
/// To make a datablock subclass, you need to extend three functions:
///      - preload()
///      - packData()
///      - unpackData()
///
/// packData() and unpackData() simply read or write data to a network stream. If you
/// add any fields, you need to add appropriate calls to read or write. Make sure that
/// the order of reads and writes is the same in both functions. Make sure to call
/// the Parent's version of these methods in every subclass.
///
/// preload() is a bit more complex; it is responsible for taking the raw data read by
/// unpackData() and processing it into a form useful by the datablock's owning object. For
/// instance, the Player class' datablock, PlayerData, gets handles to commonly used
/// nodes in the player model, as well as resolving handles to textures and other
/// resources. <b>Any</b> code which loads files or performs other actions beyond simply
/// reading the data from the packet, such as validation, must reside in preload().
///
/// To write your own preload() methods, see any of the existing methods in the codebase; for instance,
/// PlayerData::preload() is an excellent example of error-reporting, data validation, and so forth.
///
/// @note A useful trick, which is used in several places in the engine, is that of temporarily
///       storing SimObjectIds in the variable which will eventually hold the "real" handle. ShapeImage
///       uses this trick in several pllaces; so do the vehicle classes. See GameBaseData for more on
///       using this trick.
///
/// @see GameBaseData for some more information on the datablocks used throughout
///      most of the engine.
/// @see http://hosted.tribalwar.com/t2faq/datablocks.shtml for an excellent
///      explanation of the basics of datablocks from script. Note that these comments
///      mostly apply to GameBaseData and its children.
/// @nosubgrouping
class SimDataBlock : public SimObject
{
    typedef SimObject Parent;
public:

    SimDataBlock();
    DECLARE_CONOBJECT(SimDataBlock);

    /// @name Datablock Internals
    /// @{

protected:
    S32  modifiedKey;

public:
    static SimObjectId sNextObjectId;
    static S32         sNextModifiedKey;

    /// Assign a new modified key.
    ///
    /// Datablocks are assigned a modified key which is updated every time
    /// a static field of the datablock is changed. These are gotten from
    /// a global store.
    static S32 getNextModifiedKey() { return sNextModifiedKey; }

    /// Get the modified key for this particular datablock.
    S32 getModifiedKey() const { return modifiedKey; }

    bool onAdd();
    void onStaticModified(const char* slotName);
    //void setLastError(const char*);
    void assignId();

    /// @}

    /// @name Datablock Interface
    /// @{

    virtual void packData(BitStream* stream);
    virtual void unpackData(BitStream* stream);

    /// Called to prepare the datablock for use, after it has been unpacked.
    ///
    /// @param  server      Set if we're running on the server (and therefore don't need to load
    ///                     things like textures or sounds).
    /// @param  errorBuffer If an error occurs in loading, this is set to a short string describing
    ///                     the error.
    /// @returns True if all went well; false if something failed.
    ///
    /// @see @ref SimDataBlock_preload
    virtual bool preload(bool server, char errorBuffer[256]);
    /// @}
};

//---------------------------------------------------------------------------
/// A set of SimObjects.
///
/// It is often necessary to keep track of an arbitrary set of SimObjects.
/// For instance, Torque's networking code needs to not only keep track of
/// the set of objects which need to be ghosted, but also the set of objects
/// which must <i>always</i> be ghosted. It does this by working with two
/// sets. The first of these is the RootGroup (which is actually a SimGroup)
/// and the second is the GhostAlwaysSet, which contains objects which must
/// always be ghosted to the client.
///
/// Some general notes on SimSets:
///     - Membership is not exclusive. A SimObject may be a member of multiple
///       SimSets.
///     - A SimSet does not destroy subobjects when it is destroyed.
///     - A SimSet may hold an arbitrary number of objects.
///
/// Using SimSets, the code to work with these two sets becomes
/// relatively straightforward:
///
/// @code
///        // (Example from netObject.cc)
///        // To iterate over all the objects in the Sim:
///        for (SimSetIterator obj(Sim::getRootGroup()); *obj; ++obj)
///        {
///                  NetObject* nobj = dynamic_cast<NetObject*>(*obj);
///
///                 if (nobj)
///                   {
///                     // ... do things ...
///                 }
///         }
///
///         // (Example from netGhost.cc)
///         // To iterate over the ghostAlways set.
///         SimSet* ghostAlwaysSet = Sim::getGhostAlwaysSet();
///         SimSet::iterator i;
///
///         U32 sz = ghostAlwaysSet->size();
///         S32 j;
///
///         for(i = ghostAlwaysSet->begin(); i != ghostAlwaysSet->end(); i++)
///         {
///             NetObject *obj = (NetObject *)(*i);
///
///             /// ... do things with obj...
///         }
/// @endcode
///

class SimSet : public SimObject
{
    typedef SimObject Parent;
protected:
    SimObjectList objectList;
    void* mMutex;

public:
    SimSet() {
        VECTOR_SET_ASSOCIATION(objectList);

        mMutex = Mutex::createMutex();
    }

    ~SimSet()
    {
        lock();
        unlock();
        Mutex::destroyMutex(mMutex);
        mMutex = NULL;
    }

    /// @name STL Interface
    /// @{

    ///
    typedef SimObjectList::iterator iterator;
    typedef SimObjectList::value_type value;
    SimObject* front() { return objectList.front(); }
    SimObject* first() { return objectList.first(); }
    SimObject* last() { return objectList.last(); }
    bool       empty() { return objectList.empty(); }
    S32        size() { return objectList.size(); }
    iterator   begin() { return objectList.begin(); }
    iterator   end() { return objectList.end(); }
    value operator[] (S32 index) { return objectList[U32(index)]; }

    iterator find(iterator first, iterator last, SimObject* obj)
    {
        return ::find(first, last, obj);
    }

    bool reOrder(SimObject* obj, SimObject* target = 0);

    /// @}

    virtual void onRemove();
    virtual void onDeleteNotify(SimObject* object);

    /// @name Set Management
    /// @{

    virtual void addObject(SimObject*);      ///< Add an object to the set.
    virtual void removeObject(SimObject*);   ///< Remove an object from the set.

    virtual void pushObject(SimObject*);     ///< Add object to end of list.
                                             ///
                                             /// It will force the object to the end of the list if it already exists
                                             /// in the list.

    virtual void popObject();                ///< Remove an object from the end of the list.

    void bringObjectToFront(SimObject* obj) { reOrder(obj, front()); }
    void pushObjectToBack(SimObject* obj) { reOrder(obj, NULL); }

    /// @}

    void write(Stream& stream, U32 tabStop, U32 flags = 0);

    virtual SimObject* findObject(const char* name);

    //bool isHidden();
    void setHidden(bool b);

    inline void lock()
    {
#ifdef TORQUE_MULTITHREAD
        Mutex::lockMutex(mMutex);
#endif
    }

    inline void unlock()
    {
#ifdef TORQUE_MULTITHREAD
        Mutex::unlockMutex(mMutex);
#endif
    }

    DECLARE_CONOBJECT(SimSet);
};

/// Iterator for use with SimSets
///
/// @see SimSet
class SimSetIterator
{
protected:
    struct Entry {
        SimSet* set;
        SimSet::iterator itr;
    };
    class Stack : public Vector<Entry> {
    public:
        void push_back(SimSet*);
    };
    Stack stack;

public:
    SimSetIterator(SimSet*);
    SimObject* operator++();
    SimObject* operator*() {
        return stack.empty() ? 0 : *stack.last().itr;
    }
};

//---------------------------------------------------------------------------
/// A group of SimObjects.
///
/// A SimGroup is a stricter form of SimSet. SimObjects may only be a member
/// of a single SimGroup at a time.
///
/// The SimGroup will automatically enforce the single-group-membership rule.
///
/// @code
///      // From engine/sim/simPath.cc - getting a pointer to a SimGroup
///      SimGroup* pMissionGroup = dynamic_cast<SimGroup*>(Sim::findObject("MissionGroup"));
///
///      // From game/trigger.cc:46 - iterating over a SimObject's group.
///      SimObject* trigger = ...;
///      SimGroup* pGroup = trigger->getGroup();
///      for (SimGroup::iterator itr = pGroup->begin(); itr != pGroup->end(); itr++)
///      {
///         // do something with *itr
///      }
/// @endcode
class SimGroup : public SimSet
{
private:
    friend class SimManager;
    friend class SimObject;

    typedef SimSet Parent;
    SimNameDictionary nameDictionary;

public:
    ~SimGroup();

    /// Add an object to the group.
    void addObject(SimObject*);
    void addObject(SimObject*, SimObjectId);
    void addObject(SimObject*, const char* name);

    /// Remove an object from the group.
    void removeObject(SimObject*);

    void onRemove();

    /// Find an object in the group.
    virtual SimObject* findObject(const char* name);

    bool processArguments(S32 argc, const char** argv);

    DECLARE_CONOBJECT(SimGroup);
};

inline void SimGroup::addObject(SimObject* obj, SimObjectId id)
{
    obj->mId = id;
    addObject(obj);
}

inline void SimGroup::addObject(SimObject* obj, const char* name)
{
    addObject(obj);
    obj->assignName(name);
}

class SimGroupIterator : public SimSetIterator
{
public:
    SimGroupIterator(SimGroup* grp) : SimSetIterator(grp) {}
    SimObject* operator++();
};

//---------------------------------------------------------------------------

class SimDataBlockGroup : public SimGroup
{
private:
    S32 mLastModifiedKey;

public:
    static S32 QSORT_CALLBACK compareModifiedKey(const void* a, const void* b);
    void sort();
    SimDataBlockGroup();
};

//---------------------------------------------------------------------------

/// @defgroup simbase_helpermacros Helper Macros
///
/// These are used for named sets and groups in the manager.
/// @{
#define DeclareNamedSet(set) extern SimSet *g##set;inline SimSet *get##set() { return g##set; }
#define DeclareNamedGroup(set) extern SimGroup *g##set;inline SimGroup *get##set() { return g##set; }
#define ImplementNamedSet(set) SimSet *g##set;
#define ImplementNamedGroup(set) SimGroup *g##set;
/// @}
//---------------------------------------------------------------------------

namespace Sim
{
    DeclareNamedSet(ActiveActionMapSet)
        DeclareNamedSet(GhostAlwaysSet)
        DeclareNamedSet(LightSet)
        DeclareNamedSet(WayPointSet)
        DeclareNamedSet(fxReplicatorSet)
        DeclareNamedSet(fxFoliageSet)
        DeclareNamedSet(reflectiveSet)
        DeclareNamedSet(MaterialSet)
        DeclareNamedSet(SFXSourceSet);

        DeclareNamedGroup(ActionMapGroup)
        DeclareNamedGroup(ClientGroup)
        DeclareNamedGroup(GuiGroup)
        DeclareNamedGroup(GuiDataGroup)
        DeclareNamedGroup(TCPGroup)
        DeclareNamedGroup(ClientConnectionGroup)
        DeclareNamedGroup(ChunkFileGroup);

    DeclareNamedSet(sgMissionLightingFilterSet);

    void init();
    void shutdown();

    SimDataBlockGroup* getDataBlockGroup();
    SimGroup* getRootGroup();

    SimObject* findObject(SimObjectId);
    SimObject* findObject(const char* name);
    template<class T> inline bool findObject(SimObjectId id, T*& t)
    {
        t = dynamic_cast<T*>(findObject(id));
        return t != NULL;
    }
    template<class T> inline bool findObject(const char* objectName, T*& t)
    {
        t = dynamic_cast<T*>(findObject(objectName));
        return t != NULL;
    }

    void advanceToTime(SimTime time);
    void advanceTime(SimTime delta);
    SimTime getCurrentTime();
    SimTime getTargetTime();

    /// a target time of 0 on an event means current event
    U32 postEvent(SimObject*, SimEvent*, U32 targetTime);

    inline U32 postEvent(SimObjectId id, SimEvent* evt, U32 targetTime)
    {
        return postEvent(findObject(id), evt, targetTime);
    }
    inline U32 postEvent(const char* objectName, SimEvent* evt, U32 targetTime)
    {
        return postEvent(findObject(objectName), evt, targetTime);
    }
    inline U32 postCurrentEvent(SimObject* obj, SimEvent* evt)
    {
        return postEvent(obj, evt, getCurrentTime());
    }
    inline U32 postCurrentEvent(SimObjectId obj, SimEvent* evt)
    {
        return postEvent(obj, evt, getCurrentTime());
    }
    inline U32 postCurrentEvent(const char* obj, SimEvent* evt)
    {
        return postEvent(obj, evt, getCurrentTime());
    }

    void cancelPendingEvents(SimObject* object);
    void cancelEvent(U32 eventId);
    bool isEventPending(U32 eventId);
    U32  getEventTimeLeft(U32 eventId);
    U32  getTimeSinceStart(U32 eventId);
    U32  getScheduleDuration(U32 eventId);
}

//----------------------------------------------------------------------------
#define DECLARE_CONSOLETYPE(T) \
   DefineConsoleType( Type##T##Ptr )

#define IMPLEMENT_CONSOLETYPE(T) \
   DatablockConsoleType( T##Ptr, Type##T##Ptr, sizeof(T*), T )

#define IMPLEMENT_SETDATATYPE(T) \
   ConsoleSetType( Type##T##Ptr ) \
   {                                                                                                 \
      volatile SimDataBlock* pConstraint = static_cast<SimDataBlock*>((T*)NULL);                     \
                                                                                                     \
      if (argc == 1) {                                                                               \
         *reinterpret_cast<T**>(dptr) = NULL;                                                        \
         if (argv[0] && argv[0][0] && !Sim::findObject(argv[0],*reinterpret_cast<T**>(dptr)))        \
            Con::printf("Object '%s' is not a member of the '%s' data block class", argv[0], #T);    \
      }                                                                                              \
      else                                                                                           \
         Con::printf("Cannot set multiple args to a single pointer.");                               \
   }

#define IMPLEMENT_GETDATATYPE(T) \
   ConsoleGetType( Type##T##Ptr ) \
   {                                                                                   \
      volatile SimDataBlock* pConstraint = static_cast<SimDataBlock*>((T*)NULL);       \
      T** obj = reinterpret_cast<T**>(dptr);                                           \
      return *obj ? (*obj)->getName() : "";                                            \
   }

//---------------------------------------------------------------------------

#endif
