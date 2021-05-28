//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ACTIONMAP_H_
#define _ACTIONMAP_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

struct InputEvent;

struct EventDescriptor
{
    U8  flags;      ///< Combination of any modifier flags.
    U8  eventType;  ///< SI_KEY, etc.
    U16 eventCode;  ///< From event.h
};

/// Map raw inputs to a variety of actions.  This is used for all keymapping
/// in the engine.
/// @see ActionMap::Node
class ActionMap : public SimObject
{
    typedef SimObject Parent;

protected:
    bool onAdd();

    struct Node {
        U32 modifiers;
        U32 action;

        enum Flags {
            Ranged = BIT(0),   ///< Ranged input.
            HasScale = BIT(1),   ///< Scaled input.
            HasDeadZone = BIT(2),   ///< Dead zone is present.
            Inverted = BIT(3),   ///< Input is inverted.
            NonLinear = BIT(4),   ///< Input should be re-fit to a non-linear scale
            BindCmd = BIT(5)    ///< Bind a console command to this.
        };

        U32 flags;           /// @see Node::Flags
        F32 deadZoneBegin;
        F32 deadZoneEnd;
        F32 scaleFactor;

        StringTableEntry consoleFunction; ///< Console function to call with new values.

        char* makeConsoleCommand;         ///< Console command to execute when we make this command.
        char* breakConsoleCommand;        ///< Console command to execute when we break this command.
    };

    /// Used to represent a devices.
    struct DeviceMap
    {
        U32 deviceType;
        U32 deviceInst;

        Vector<Node> nodeMap;
        DeviceMap() {
            VECTOR_SET_ASSOCIATION(nodeMap);
        }
        ~DeviceMap();
    };
    struct BreakEntry
    {
        U32 deviceType;
        U32 deviceInst;
        U32 objInst;
        StringTableEntry consoleFunction;
        char* breakConsoleCommand;

        // It's possible that the node could be deleted (unlikely, but possible,
        //  so we replicate the node flags here...
        //
        U32 flags;
        F32 deadZoneBegin;
        F32 deadZoneEnd;
        F32 scaleFactor;
    };


    Vector<DeviceMap*>        mDeviceMaps;
    static Vector<BreakEntry> smBreakTable;

    // Find: return NULL if not found in current map, Get: create if not
    //  found.
    const Node* findNode(const U32 inDeviceType, const U32 inDeviceInst,
        const U32 inModifiers, const U32 inAction);
    bool findBoundNode(const char* function, U32& devMapIndex, U32& nodeIndex);
    Node* getNode(const U32 inDeviceType, const U32 inDeviceInst,
        const U32 inModifiers, const U32 inAction);

    void removeNode(const U32 inDeviceType, const U32 inDeviceInst,
        const U32 inModifiers, const U32 inAction);

    void enterBreakEvent(const InputEvent* pEvent, const Node* pNode);

    static const char* getModifierString(const U32 modifiers);

public:
    ActionMap();
    ~ActionMap();

    void dumpActionMap(const char* fileName, const bool append) const;

    static bool createEventDescriptor(const char* pEventString, EventDescriptor* pDescriptor);

    bool processBind(const U32 argc, const char** argv);
    bool processBindCmd(const char* device, const char* action, const char* makeCmd, const char* breakCmd);
    bool processUnbind(const char* device, const char* action);

    /// @name Console Interface Functions
    /// @{
    const char* getBinding(const char* command);                    ///< Find what the given command is bound to.
    const char* getCommand(const char* device, const char* action); ///< Find what command is bound to the given event descriptor .
    bool    isInverted(const char* device, const char* action);
    F32   getScale(const char* device, const char* action);
    const char* getDeadZone(const char* device, const char* action);
    /// @}


    static bool        getKeyString(const U32 action, char* buffer);
    static bool        getDeviceName(const U32 deviceType, const U32 deviceInstance, char* buffer);
    static const char* buildActionString(const InputEvent* event);

    bool processAction(const InputEvent*);

    static bool checkBreakTable(const InputEvent*);
    static bool handleEvent(const InputEvent*);
    static bool handleEventGlobal(const InputEvent*);

    static bool getDeviceTypeAndInstance(const char* device, U32& deviceType, U32& deviceInstance);

    DECLARE_CONOBJECT(ActionMap);
};

#endif // _ACTIONMAP_H_
