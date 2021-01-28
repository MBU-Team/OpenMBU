//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CONSOLEINTERNAL_H_
#define _CONSOLEINTERNAL_H_

#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

class ExprEvalState;
struct FunctionDecl;
class CodeBlock;
class AbstractClassRep;

class Namespace
{
    enum {
        MaxActivePackages = 512,
    };

    static U32 mNumActivePackages;
    static U32 mOldNumActivePackages;
    static StringTableEntry mActivePackages[MaxActivePackages];
public:
    StringTableEntry mName;
    StringTableEntry mPackage;

    Namespace *mParent;
    Namespace *mNext;
    AbstractClassRep *mClassRep;
    U32 mRefCountToParent;

    struct Entry
    {
        enum {
            GroupMarker                  = -3,
            OverloadMarker               = -2,
            InvalidFunctionType          = -1,
            ScriptFunctionType,
            StringCallbackType,
            IntCallbackType,
            FloatCallbackType,
            VoidCallbackType,
            BoolCallbackType
        };

        Namespace *mNamespace;
        Entry *mNext;
        StringTableEntry mFunctionName;
        S32 mType;
        S32 mMinArgs;
        S32 mMaxArgs;
        const char *mUsage;
        StringTableEntry mPackage;

        CodeBlock *mCode;
        U32 mFunctionOffset;
        union {
            StringCallback mStringCallbackFunc;
            IntCallback mIntCallbackFunc;
            VoidCallback mVoidCallbackFunc;
            FloatCallback mFloatCallbackFunc;
            BoolCallback mBoolCallbackFunc;
            const char* mGroupName;
        } cb;
        Entry();
        void clear();

        const char *execute(S32 argc, const char **argv, ExprEvalState *state);

    };
    Entry *mEntryList;

    Entry **mHashTable;
    U32 mHashSize;
    U32 mHashSequence;  ///< @note The hash sequence is used by the autodoc console facility
                        ///        as a means of testing reference state.

    Namespace();
    void addFunction(StringTableEntry name, CodeBlock *cb, U32 functionOffset);
    void addCommand(StringTableEntry name,StringCallback, const char *usage, S32 minArgs, S32 maxArgs);
    void addCommand(StringTableEntry name,IntCallback, const char *usage, S32 minArgs, S32 maxArgs);
    void addCommand(StringTableEntry name,FloatCallback, const char *usage, S32 minArgs, S32 maxArgs);
    void addCommand(StringTableEntry name,VoidCallback, const char *usage, S32 minArgs, S32 maxArgs);
    void addCommand(StringTableEntry name,BoolCallback, const char *usage, S32 minArgs, S32 maxArgs);

    void addOverload(const char *name, const char* altUsage);

    void markGroup(const char* name, const char* usage);
    char * lastUsage;

    void getEntryList(Vector<Entry *> *);

    Entry *lookup(StringTableEntry name);
    Entry *lookupRecursive(StringTableEntry name);
    Entry *createLocalEntry(StringTableEntry name);
    void buildHashTable();
    void clearEntries();
    bool classLinkTo(Namespace *parent);

    const char *tabComplete(const char *prevText, S32 baseLen, bool fForward);

    static U32 mCacheSequence;
    static DataChunker mCacheAllocator;
    static DataChunker mAllocator;
    static void trashCache();
    static Namespace *mNamespaceList;
    static Namespace *mGlobalNamespace;

    static void init();
    static void shutdown();
    static Namespace *global();

    static Namespace *find(StringTableEntry name, StringTableEntry package=NULL);

    static void activatePackage(StringTableEntry name);
    static void deactivatePackage(StringTableEntry name);
    static void dumpClasses();
    static void dumpFunctions();
    static void printNamespaceEntries(Namespace * g);
    static void unlinkPackages();
    static void relinkPackages();
    static bool isPackage(StringTableEntry name);
};

extern char *typeValueEmpty;

class Dictionary
{
public:
    struct Entry
    {
        enum
        {
            TypeInternalInt = -3,
            TypeInternalFloat = -2,
            TypeInternalString = -1,
        };

        StringTableEntry name;
        Entry *nextEntry;
        S32 type;
        char *sval;
        U32 ival;  // doubles as strlen when type = -1
        F32 fval;
        U32 bufferLen;
        void *dataPtr;

        Entry(StringTableEntry name);
        ~Entry();

        U32 getIntValue()
        {
            if(type <= TypeInternalString)
                return ival;
            else
                return dAtoi(Con::getData(type, dataPtr, 0));
        }
        F32 getFloatValue()
        {
            if(type <= TypeInternalString)
                return fval;
            else
                return dAtof(Con::getData(type, dataPtr, 0));
        }
        const char *getStringValue()
        {
            if(type == TypeInternalString)
                return sval;
            if(type == TypeInternalFloat)
                return Con::getData(TypeF32, &fval, 0);
            else if(type == TypeInternalInt)
                return Con::getData(TypeS32, &ival, 0);
            else
                return Con::getData(type, dataPtr, 0);
        }
        void setIntValue(U32 val)
        {
            if(type <= TypeInternalString)
            {
                fval = (F32)val;
                ival = val;
                if(sval != typeValueEmpty)
                {
                    dFree(sval);
                    sval = typeValueEmpty;
                }
                type = TypeInternalInt;
                return;
            }
            else
            {
                const char *dptr = Con::getData(TypeS32, &val, 0);
                Con::setData(type, dataPtr, 0, 1, &dptr);
            }
        }
        void setFloatValue(F32 val)
        {
            if(type <= TypeInternalString)
            {
                fval = val;
                ival = static_cast<U32>(val);
                if(sval != typeValueEmpty)
                {
                    dFree(sval);
                    sval = typeValueEmpty;
                }
                type = TypeInternalFloat;
                return;
            }
            else
            {
                const char *dptr = Con::getData(TypeF32, &val, 0);
                Con::setData(type, dataPtr, 0, 1, &dptr);
            }
        }
        void setStringValue(const char *value);
    };

private:
    struct HashTableData
    {
        Dictionary* owner;
        S32 size;
        S32 count;
        Entry **data;
    };

    HashTableData *hashTable;
    ExprEvalState *exprState;
public:
    StringTableEntry scopeName;
    Namespace *scopeNamespace;
    CodeBlock *code;
    U32 ip;

    Dictionary();
    Dictionary(ExprEvalState *state, Dictionary* ref=NULL);
    ~Dictionary();
    Entry *lookup(StringTableEntry name);
    Entry *add(StringTableEntry name);
    void setState(ExprEvalState *state, Dictionary* ref=NULL);
    void remove(Entry *);
    void reset();

    void exportVariables(const char *varString, const char *fileName, bool append);
    void deleteVariables(const char *varString);

    void setVariable(StringTableEntry name, const char *value);
    const char *getVariable(StringTableEntry name, bool *valid = NULL);

    void addVariable(const char *name, S32 type, void *dataPtr);
    bool removeVariable(StringTableEntry name);

    /// Return the best tab completion for prevText, with the length
    /// of the pre-tab string in baseLen.
    const char *tabComplete(const char *prevText, S32 baseLen, bool);
};

class ExprEvalState
{
public:
    /// @name Expression Evaluation
    /// @{

    ///
    SimObject *thisObject;
    Dictionary::Entry *currentVariable;
    bool traceOn;

    ExprEvalState();
    ~ExprEvalState();

    /// @}

    /// @name Stack Management
    /// @{

    ///
    Dictionary globalVars;
    Vector<Dictionary *> stack;
    void setCurVarName(StringTableEntry name);
    void setCurVarNameCreate(StringTableEntry name);
    S32 getIntVariable();
    F64 getFloatVariable();
    const char *getStringVariable();
    void setIntVariable(S32 val);
    void setFloatVariable(F64 val);
    void setStringVariable(const char *str);

    void pushFrame(StringTableEntry frameName, Namespace *ns);
    void popFrame();

    /// Puts a reference to an existing stack frame
    /// on the top of the stack.
    void pushFrameRef(S32 stackIndex);

    /// @}
};

#endif
