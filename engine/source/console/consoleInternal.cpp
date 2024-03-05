//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"

#include "console/ast.h"
#include "core/tAlgorithm.h"
#include "core/resManager.h"

#include "core/findMatch.h"
#include "console/consoleInternal.h"
#include "core/fileStream.h"
#include "console/compiler.h"

#define ST_INIT_SIZE 15

static char scratchBuffer[1024];
U32 Namespace::mCacheSequence = 0;
DataChunker Namespace::mCacheAllocator;
DataChunker Namespace::mAllocator;
Namespace* Namespace::mNamespaceList = NULL;
Namespace* Namespace::mGlobalNamespace = NULL;

bool canTabComplete(const char* prevText, const char* bestMatch,
    const char* newText, S32 baseLen, bool fForward)
{
    // test if it matches the first baseLen chars:
    if (dStrnicmp(newText, prevText, baseLen))
        return false;

    if (fForward)
    {
        if (!bestMatch)
            return dStricmp(newText, prevText) > 0;
        else
            return (dStricmp(newText, prevText) > 0) &&
            (dStricmp(newText, bestMatch) < 0);
    }
    else
    {
        if (dStrlen(prevText) == (U32)baseLen)
        {
            // look for the 'worst match'
            if (!bestMatch)
                return dStricmp(newText, prevText) > 0;
            else
                return dStricmp(newText, bestMatch) > 0;
        }
        else
        {
            if (!bestMatch)
                return (dStricmp(newText, prevText) < 0);
            else
                return (dStricmp(newText, prevText) < 0) &&
                (dStricmp(newText, bestMatch) > 0);
        }
    }
}

//---------------------------------------------------------------
//
// Dictionary functions
//
//---------------------------------------------------------------
struct StringValue
{
    S32 size;
    char* val;

    operator char* () { return val; }
    StringValue& operator=(const char* string);

    StringValue() { size = 0; val = NULL; }
    ~StringValue() { dFree(val); }
};


StringValue& StringValue::operator=(const char* string)
{
    if (!val)
    {
        val = dStrdup(string);
        size = dStrlen(val);
    }
    else
    {
        S32 len = dStrlen(string);
        if (len < size)
            dStrcpy(val, string);
        else
        {
            size = len;
            dFree(val);
            val = dStrdup(string);
        }
    }
    return *this;
}

static S32 QSORT_CALLBACK varCompare(const void* a, const void* b)
{
    return dStricmp((*((Dictionary::Entry**)a))->name, (*((Dictionary::Entry**)b))->name);
}

void Dictionary::exportVariables(const char* varString, const char* fileName, bool append)
{
    const char* searchStr = varString;
    Vector<Entry*> sortList(__FILE__, __LINE__);

    for (S32 i = 0; i < hashTable->size; i++)
    {
        Entry* walk = hashTable->data[i];
        while (walk)
        {
            if (FindMatch::isMatch((char*)searchStr, (char*)walk->name))
                sortList.push_back(walk);

            walk = walk->nextEntry;
        }
    }

    if (!sortList.size())
        return;

    dQsort((void*)&sortList[0], sortList.size(), sizeof(Entry*), varCompare);

    Vector<Entry*>::iterator s;
    char expandBuffer[1024];
    Stream* strm;

    if (fileName)
    {
        if (!ResourceManager->openFileForWrite(strm, fileName, append ? FileStream::ReadWrite : FileStream::Write))
        {
            Con::errorf(ConsoleLogEntry::General, "Unable to open file '%s for writing.", fileName);
            return;
        }
        if (append)
            strm->setPosition(strm->getStreamSize());
    }

    char buffer[1024];
    const char* cat = fileName ? "\r\n" : "";

    for (s = sortList.begin(); s != sortList.end(); s++)
    {
        // -----------------------------------------------------------
        // Make sure that we don't have any invalid variable names!
        // -----------------------------------------------------------
        // TODO: This code needs to be optimized!
        bool needsFix = false;
        S32 cIndex;
        for (cIndex = 1; cIndex < dStrlen((*s)->name); cIndex++)
        {
            if (dIsalnum((*s)->name[cIndex]))
                continue;

            if ((*s)->name[cIndex] != ':' && (*s)->name[cIndex] != '_')
            {
                needsFix = true;
                break;
            }
        }

        char varName[1024];
        if (needsFix)
        {
            const char *strInvalidStart = &(*s)->name[cIndex];
            char expandBufferName[1024];
            expandEscape(expandBufferName, strInvalidStart);

            char orgStringStart[1024];
            dStrncpy(orgStringStart, (*s)->name, cIndex);
            orgStringStart[cIndex] = '\0';

            dSprintf(varName, sizeof(varName), "%s[\"%s\"]", orgStringStart, strInvalidStart);
        } else {
            dStrcpy(varName, (*s)->name);
        }
        // -----------------------------------------------------------

        switch ((*s)->type)
        {
        case Entry::TypeInternalInt:
            dSprintf(buffer, sizeof(buffer), "%s = %d;%s", varName, (*s)->ival, cat);
            break;
        case Entry::TypeInternalFloat:
            dSprintf(buffer, sizeof(buffer), "%s = %g;%s", varName, (*s)->fval, cat);
            break;
        default:
            expandEscape(expandBuffer, (*s)->getStringValue());
            dSprintf(buffer, sizeof(buffer), "%s = \"%s\";%s", varName, expandBuffer, cat);
            break;
        }
        if (fileName)
            strm->write(dStrlen(buffer), buffer);
        else
            Con::printf("%s", buffer);
    }
    if (fileName)
        delete strm;
}

void Dictionary::deleteVariables(const char* varString, bool emptyOnly)
{
    const char* searchStr = varString;

    for (S32 i = 0; i < hashTable->size; i++)
    {
        Entry* walk = hashTable->data[i];
        while (walk)
        {
            Entry* matchedEntry = (FindMatch::isMatch((char*)searchStr, (char*)walk->name)) ? walk : NULL;
            walk = walk->nextEntry;
            if (matchedEntry)
            {
                bool empty = false;
                const char* val = matchedEntry->getStringValue();
                if (!val || (val && val[0] == '\0'))
                    empty = true;
                if (!emptyOnly || empty)
                    remove(matchedEntry); // assumes remove() is a stable remove (will not reorder entries on remove)
            }
        }
    }
}

bool Dictionary::variablesExist(const char* varString)
{
    const char* searchStr = varString;

    for (S32 i = 0; i < hashTable->size; i++)
    {
        Entry* walk = hashTable->data[i];
        while (walk)
        {
            Entry* matchedEntry = (FindMatch::isMatch((char*)searchStr, (char*)walk->name)) ? walk : NULL;
            walk = walk->nextEntry;
            if (matchedEntry)
                return true;
        }
    }
    return false;
}

U32 HashPointer(StringTableEntry ptr)
{
    return (U32)(((dsize_t)ptr) >> 2);
}

Dictionary::Entry* Dictionary::lookup(StringTableEntry name)
{
    Entry* walk = hashTable->data[HashPointer(name) % hashTable->size];
    while (walk)
    {
        if (walk->name == name)
            return walk;
        else
            walk = walk->nextEntry;
    }

    return NULL;
}

Dictionary::Entry* Dictionary::add(StringTableEntry name)
{
    Entry* walk = hashTable->data[HashPointer(name) % hashTable->size];
    while (walk)
    {
        if (walk->name == name)
            return walk;
        else
            walk = walk->nextEntry;
    }
    Entry* ret;
    hashTable->count++;

    if (hashTable->count > hashTable->size * 2)
    {
        Entry head(NULL), * walk;
        S32 i;
        walk = &head;
        walk->nextEntry = 0;
        for (i = 0; i < hashTable->size; i++) {
            while (walk->nextEntry) {
                walk = walk->nextEntry;
            }
            walk->nextEntry = hashTable->data[i];
        }
        delete[] hashTable->data;
        hashTable->size = hashTable->size * 4 - 1;
        hashTable->data = new Entry * [hashTable->size];
        for (i = 0; i < hashTable->size; i++)
            hashTable->data[i] = NULL;
        walk = head.nextEntry;
        while (walk)
        {
            Entry* temp = walk->nextEntry;
            S32 idx = HashPointer(walk->name) % hashTable->size;
            walk->nextEntry = hashTable->data[idx];
            hashTable->data[idx] = walk;
            walk = temp;
        }
    }

    ret = new Entry(name);
    S32 idx = HashPointer(name) % hashTable->size;
    ret->nextEntry = hashTable->data[idx];
    hashTable->data[idx] = ret;
    return ret;
}

// deleteVariables() assumes remove() is a stable remove (will not reorder entries on remove)
void Dictionary::remove(Dictionary::Entry* ent)
{
    Entry** walk = &hashTable->data[HashPointer(ent->name) % hashTable->size];
    while (*walk != ent)
        walk = &((*walk)->nextEntry);

    *walk = (ent->nextEntry);
    delete ent;
    hashTable->count--;
}

Dictionary::Dictionary()
    : hashTable(NULL),
    exprState(NULL),
    scopeName(NULL),
    scopeNamespace(NULL),
    code(NULL),
    ip(0)
{
}

Dictionary::Dictionary(ExprEvalState* state, Dictionary* ref)
    : hashTable(NULL),
    exprState(NULL),
    scopeName(NULL),
    scopeNamespace(NULL),
    code(NULL),
    ip(0)
{
    setState(state, ref);
}

void Dictionary::setState(ExprEvalState* state, Dictionary* ref)
{
    exprState = state;

    if (ref)
        hashTable = ref->hashTable;
    else
    {
        hashTable = new HashTableData;
        hashTable->owner = this;
        hashTable->count = 0;
        hashTable->size = ST_INIT_SIZE;
        hashTable->data = new Entry * [hashTable->size];

        for (S32 i = 0; i < hashTable->size; i++)
            hashTable->data[i] = NULL;
    }
}

Dictionary::~Dictionary()
{
    if (hashTable->owner == this)
    {
        reset();
        delete[] hashTable->data;
        delete hashTable;
    }
}

void Dictionary::reset()
{
    S32 i;
    Entry* walk, * temp;

    for (i = 0; i < hashTable->size; i++)
    {
        walk = hashTable->data[i];
        while (walk)
        {
            temp = walk->nextEntry;
            delete walk;
            walk = temp;
        }
        hashTable->data[i] = NULL;
    }
    hashTable->size = ST_INIT_SIZE;
    hashTable->count = 0;
}


const char* Dictionary::tabComplete(const char* prevText, S32 baseLen, bool fForward)
{
    S32 i;

    const char* bestMatch = NULL;
    for (i = 0; i < hashTable->size; i++)
    {
        Entry* walk = hashTable->data[i];
        while (walk)
        {
            if (canTabComplete(prevText, bestMatch, walk->name, baseLen, fForward))
                bestMatch = walk->name;
            walk = walk->nextEntry;
        }
    }
    return bestMatch;
}


char* typeValueEmpty = "";

Dictionary::Entry::Entry(StringTableEntry in_name)
{
    dataPtr = NULL;
    name = in_name;
    type = -1;
    ival = 0;
    fval = 0;
    sval = typeValueEmpty;
}

Dictionary::Entry::~Entry()
{
    if (sval != typeValueEmpty)
        dFree(sval);
}

const char* Dictionary::getVariable(StringTableEntry name, bool* entValid)
{
    Entry* ent = lookup(name);
    if (ent)
    {
        if (entValid)
            *entValid = true;
        return ent->getStringValue();
    }
    if (entValid)
        *entValid = false;

    // Warn users when they access a variable that isn't defined.
    if (gWarnUndefinedScriptVariables)
        Con::warnf(" *** Accessed undefined variable '%s'", name);

    return "";
}

void Dictionary::Entry::setStringValue(const char* value)
{
    if (type <= TypeInternalString)
    {
        // Let's not remove empty-string-valued global vars from the dict.
        // If we remove them, then they won't be exported, and sometimes
        // it could be necessary to export such a global.  There are very
        // few empty-string global vars so there's no performance-related
        // need to remove them from the dict.
  /*
        if(!value[0] && name[0] == '$')
        {
           gEvalState.globalVars.remove(this);
           return;
        }
  */

        U32 stringLen = dStrlen(value);

        // If it's longer than 256 bytes, it's certainly not a number.
        //
        // (This decision may come back to haunt you. Shame on you if it
        // does.)
        if (stringLen < 256)
        {
            fval = dAtof(value);
            ival = dAtoi(value);
        }
        else
        {
            fval = 0.f;
            ival = 0;
        }

        type = TypeInternalString;

        // may as well pad to the next cache line
        U32 newLen = ((stringLen + 1) + 15) & ~15;

        if (sval == typeValueEmpty)
            sval = (char*)dMalloc(newLen);
        else if (newLen > bufferLen)
            sval = (char*)dRealloc(sval, newLen);

        bufferLen = newLen;
        dStrcpy(sval, value);
    }
    else
        Con::setData(type, dataPtr, 0, 1, &value);
}

void Dictionary::setVariable(StringTableEntry name, const char* value)
{
    Entry* ent = add(name);
    if (!value)
        value = "";
    ent->setStringValue(value);
}

void Dictionary::addVariable(const char* name, S32 type, void* dataPtr)
{
    if (name[0] != '$')
    {
        scratchBuffer[0] = '$';
        dStrcpy(scratchBuffer + 1, name);
        name = scratchBuffer;
    }
    Entry* ent = add(StringTable->insert(name));
    ent->type = type;
    if (ent->sval != typeValueEmpty)
    {
        dFree(ent->sval);
        ent->sval = typeValueEmpty;
    }
    ent->dataPtr = dataPtr;
}

bool Dictionary::removeVariable(StringTableEntry name)
{
    if (Entry* ent = lookup(name)) {
        remove(ent);
        return true;
    }
    return false;
}

void ExprEvalState::pushFrame(StringTableEntry frameName, Namespace* ns)
{
    Dictionary* newFrame = new Dictionary(this);
    newFrame->scopeName = frameName;
    newFrame->scopeNamespace = ns;
    stack.push_back(newFrame);
}

void ExprEvalState::popFrame()
{
    Dictionary* last = stack.last();
    stack.pop_back();
    delete last;
}

void ExprEvalState::pushFrameRef(S32 stackIndex)
{
    AssertFatal(stackIndex >= 0 && stackIndex < stack.size(), "You must be asking for a valid frame!");
    Dictionary* newFrame = new Dictionary(this, stack[stackIndex]);
    stack.push_back(newFrame);
}

ExprEvalState::ExprEvalState()
{
    VECTOR_SET_ASSOCIATION(stack);
    globalVars.setState(this);
    thisObject = NULL;
    traceOn = false;
}

ExprEvalState::~ExprEvalState()
{
    while (stack.size())
        popFrame();
}

ConsoleFunction(backtrace, void, 1, 1, "Print the call stack.")
{
    argc; argv;
    U32 totalSize = 1;

    for (U32 i = 0; i < gEvalState.stack.size(); i++)
    {
        totalSize += dStrlen(gEvalState.stack[i]->scopeName) + 3;
        if (gEvalState.stack[i]->scopeNamespace && gEvalState.stack[i]->scopeNamespace->mName)
            totalSize += dStrlen(gEvalState.stack[i]->scopeNamespace->mName) + 2;
    }

    char* buf = Con::getReturnBuffer(totalSize);
    buf[0] = 0;
    for (U32 i = 0; i < gEvalState.stack.size(); i++)
    {
        dStrcat(buf, "->");
        if (gEvalState.stack[i]->scopeNamespace && gEvalState.stack[i]->scopeNamespace->mName)
        {
            dStrcat(buf, gEvalState.stack[i]->scopeNamespace->mName);
            dStrcat(buf, "::");
        }
        dStrcat(buf, gEvalState.stack[i]->scopeName);
    }
    Con::printf("BackTrace: %s", buf);

}

Namespace::Entry::Entry()
{
    mCode = NULL;
    mType = InvalidFunctionType;
}

void Namespace::Entry::clear()
{
    if (mCode)
    {
        mCode->decRefCount();
        mCode = NULL;
    }
}

Namespace::Namespace()
{
    mPackage = NULL;
    mName = NULL;
    mParent = NULL;
    mNext = NULL;
    mEntryList = NULL;
    mHashSize = 0;
    mHashTable = 0;
    mHashSequence = 0;
    mRefCountToParent = 0;
    mClassRep = 0;
}

void Namespace::clearEntries()
{
    for (Entry* walk = mEntryList; walk; walk = walk->mNext)
        walk->clear();
}

Namespace* Namespace::find(StringTableEntry name, StringTableEntry package)
{
    for (Namespace* walk = mNamespaceList; walk; walk = walk->mNext)
        if (walk->mName == name && walk->mPackage == package)
            return walk;

    Namespace* ret = (Namespace*)mAllocator.alloc(sizeof(Namespace));
    constructInPlace(ret);
    ret->mPackage = package;
    ret->mName = name;
    ret->mNext = mNamespaceList;
    mNamespaceList = ret;
    return ret;
}

bool Namespace::classLinkTo(Namespace* parent)
{
    Namespace* walk = this;
    while (walk->mParent && walk->mParent->mName == mName)
        walk = walk->mParent;

    if (walk->mParent && walk->mParent != parent)
    {
        Con::errorf(ConsoleLogEntry::General, "Error: cannot change namespace parent linkage for %s from %s to %s.",
            walk->mName, walk->mParent->mName, parent->mName);
        return false;
    }
    mRefCountToParent++;
    walk->mParent = parent;
    return true;
}

void Namespace::buildHashTable()
{
    if (mHashSequence == mCacheSequence)
        return;

    if (!mEntryList && mParent)
    {
        mParent->buildHashTable();
        mHashTable = mParent->mHashTable;
        mHashSize = mParent->mHashSize;
        mHashSequence = mCacheSequence;
        return;
    }

    U32 entryCount = 0;
    Namespace* ns;
    for (ns = this; ns; ns = ns->mParent)
        for (Entry* walk = ns->mEntryList; walk; walk = walk->mNext)
            if (lookupRecursive(walk->mFunctionName) == walk)
                entryCount++;

    mHashSize = entryCount + (entryCount >> 1) + 1;

    if (!(mHashSize & 1))
        mHashSize++;

    mHashTable = (Entry**)mCacheAllocator.alloc(sizeof(Entry*) * mHashSize);
    for (U32 i = 0; i < mHashSize; i++)
        mHashTable[i] = NULL;

    for (ns = this; ns; ns = ns->mParent)
    {
        for (Entry* walk = ns->mEntryList; walk; walk = walk->mNext)
        {
            U32 index = HashPointer(walk->mFunctionName) % mHashSize;
            while (mHashTable[index] && mHashTable[index]->mFunctionName != walk->mFunctionName)
            {
                index++;
                if (index >= mHashSize)
                    index = 0;
            }

            if (!mHashTable[index])
                mHashTable[index] = walk;
        }
    }

    mHashSequence = mCacheSequence;
}

void Namespace::init()
{
    // create the global namespace
    mGlobalNamespace = find(NULL);
}

Namespace* Namespace::global()
{
    return mGlobalNamespace;
}

void Namespace::shutdown()
{
    for (Namespace* walk = mNamespaceList; walk; walk = walk->mNext)
        walk->clearEntries();
}

void Namespace::trashCache()
{
    mCacheSequence++;
    mCacheAllocator.freeBlocks();
}

const char* Namespace::tabComplete(const char* prevText, S32 baseLen, bool fForward)
{
    if (mHashSequence != mCacheSequence)
        buildHashTable();

    const char* bestMatch = NULL;
    for (U32 i = 0; i < mHashSize; i++)
        if (mHashTable[i] && canTabComplete(prevText, bestMatch, mHashTable[i]->mFunctionName, baseLen, fForward))
            bestMatch = mHashTable[i]->mFunctionName;
    return bestMatch;
}

Namespace::Entry* Namespace::lookupRecursive(StringTableEntry name)
{
    for (Namespace* ns = this; ns; ns = ns->mParent)
        for (Entry* walk = ns->mEntryList; walk; walk = walk->mNext)
            if (walk->mFunctionName == name)
                return walk;

    return NULL;
}

Namespace::Entry* Namespace::lookup(StringTableEntry name)
{
    if (mHashSequence != mCacheSequence)
        buildHashTable();

    U32 index = HashPointer(name) % mHashSize;
    while (mHashTable[index] && mHashTable[index]->mFunctionName != name)
    {
        index++;
        if (index >= mHashSize)
            index = 0;
    }
    return mHashTable[index];
}

static S32 QSORT_CALLBACK compareEntries(const void* a, const void* b)
{
    const Namespace::Entry* fa = *((Namespace::Entry**)a);
    const Namespace::Entry* fb = *((Namespace::Entry**)b);

    return dStricmp(fa->mFunctionName, fb->mFunctionName);
}

void Namespace::getEntryList(Vector<Entry*>* vec)
{
    if (mHashSequence != mCacheSequence)
        buildHashTable();

    for (U32 i = 0; i < mHashSize; i++)
        if (mHashTable[i])
            vec->push_back(mHashTable[i]);

    dQsort(vec->address(), vec->size(), sizeof(Namespace::Entry*), compareEntries);
}

Namespace::Entry* Namespace::createLocalEntry(StringTableEntry name)
{
    for (Entry* walk = mEntryList; walk; walk = walk->mNext)
    {
        if (walk->mFunctionName == name)
        {
            walk->clear();
            return walk;
        }
    }

    Entry* ent = (Entry*)mAllocator.alloc(sizeof(Entry));
    constructInPlace(ent);

    ent->mNamespace = this;
    ent->mFunctionName = name;
    ent->mNext = mEntryList;
    ent->mPackage = mPackage;
    mEntryList = ent;
    return ent;
}

void Namespace::addFunction(StringTableEntry name, CodeBlock* cb, U32 functionOffset)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = NULL;
    ent->mCode = cb;
    ent->mFunctionOffset = functionOffset;
    ent->mCode->incRefCount();
    ent->mType = Entry::ScriptFunctionType;
}

void Namespace::addCommand(StringTableEntry name, StringCallback cb, const char* usage, S32 minArgs, S32 maxArgs)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = usage;
    ent->mMinArgs = minArgs;
    ent->mMaxArgs = maxArgs;

    ent->mType = Entry::StringCallbackType;
    ent->cb.mStringCallbackFunc = cb;
}

void Namespace::addCommand(StringTableEntry name, IntCallback cb, const char* usage, S32 minArgs, S32 maxArgs)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = usage;
    ent->mMinArgs = minArgs;
    ent->mMaxArgs = maxArgs;

    ent->mType = Entry::IntCallbackType;
    ent->cb.mIntCallbackFunc = cb;
}

void Namespace::addCommand(StringTableEntry name, VoidCallback cb, const char* usage, S32 minArgs, S32 maxArgs)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = usage;
    ent->mMinArgs = minArgs;
    ent->mMaxArgs = maxArgs;

    ent->mType = Entry::VoidCallbackType;
    ent->cb.mVoidCallbackFunc = cb;
}

void Namespace::addCommand(StringTableEntry name, FloatCallback cb, const char* usage, S32 minArgs, S32 maxArgs)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = usage;
    ent->mMinArgs = minArgs;
    ent->mMaxArgs = maxArgs;

    ent->mType = Entry::FloatCallbackType;
    ent->cb.mFloatCallbackFunc = cb;
}

void Namespace::addCommand(StringTableEntry name, BoolCallback cb, const char* usage, S32 minArgs, S32 maxArgs)
{
    Entry* ent = createLocalEntry(name);
    trashCache();

    ent->mUsage = usage;
    ent->mMinArgs = minArgs;
    ent->mMaxArgs = maxArgs;

    ent->mType = Entry::BoolCallbackType;
    ent->cb.mBoolCallbackFunc = cb;
}

void Namespace::addOverload(const char* name, const char* altUsage)
{
    static U32 uid = 0;
    char buffer[1024];
    char lilBuffer[32];
    dStrcpy(buffer, name);
    dSprintf(lilBuffer, 32, "_%d", uid++);
    dStrcat(buffer, lilBuffer);

    Entry* ent = createLocalEntry(StringTable->insert(buffer));
    trashCache();

    ent->mUsage = altUsage;
    ent->mMinArgs = -1;
    ent->mMaxArgs = -2;

    ent->mType = Entry::OverloadMarker;
    ent->cb.mGroupName = name;
}

void Namespace::markGroup(const char* name, const char* usage)
{
    static U32 uid = 0;
    char buffer[1024];
    char lilBuffer[32];
    dStrcpy(buffer, name);
    dSprintf(lilBuffer, 32, "_%d", uid++);
    dStrcat(buffer, lilBuffer);

    Entry* ent = createLocalEntry(StringTable->insert(buffer));
    trashCache();

    if (usage != NULL)
        lastUsage = (char*)(ent->mUsage = usage);
    else
        ent->mUsage = lastUsage;

    ent->mMinArgs = -1; // Make sure it explodes if somehow we run this entry.
    ent->mMaxArgs = -2;

    ent->mType = Entry::GroupMarker;
    ent->cb.mGroupName = name;
}

extern S32 executeBlock(StmtNode* block, ExprEvalState* state);

const char* Namespace::Entry::execute(S32 argc, const char** argv, ExprEvalState* state)
{
    if (mType == ScriptFunctionType)
    {
        if (mFunctionOffset)
            return mCode->exec(mFunctionOffset, argv[0], mNamespace, argc, argv, false, mPackage);
        else
            return "";
    }

    if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs))
    {
        Con::warnf(ConsoleLogEntry::Script, "%s::%s - wrong number of arguments.", mNamespace->mName, mFunctionName);
        Con::warnf(ConsoleLogEntry::Script, "usage: %s", mUsage);
        return "";
    }

    static char returnBuffer[32];
    switch (mType)
    {
    case StringCallbackType:
        return cb.mStringCallbackFunc(state->thisObject, argc, argv);
    case IntCallbackType:
        dSprintf(returnBuffer, sizeof(returnBuffer), "%d",
            cb.mIntCallbackFunc(state->thisObject, argc, argv));
        return returnBuffer;
    case FloatCallbackType:
        dSprintf(returnBuffer, sizeof(returnBuffer), "%g",
            cb.mFloatCallbackFunc(state->thisObject, argc, argv));
        return returnBuffer;
    case VoidCallbackType:
        cb.mVoidCallbackFunc(state->thisObject, argc, argv);
        return "";
    case BoolCallbackType:
        dSprintf(returnBuffer, sizeof(returnBuffer), "%d",
            (U32)cb.mBoolCallbackFunc(state->thisObject, argc, argv));
        return returnBuffer;
    }

    return "";
}

StringTableEntry Namespace::mActivePackages[Namespace::MaxActivePackages];
U32 Namespace::mNumActivePackages = 0;
U32 Namespace::mOldNumActivePackages = 0;

bool Namespace::isPackage(StringTableEntry name)
{
    for (Namespace* walk = mNamespaceList; walk; walk = walk->mNext)
        if (walk->mPackage == name)
            return true;
    return false;
}

void Namespace::activatePackage(StringTableEntry name)
{
    if (mNumActivePackages == MaxActivePackages)
    {
        Con::printf("ActivatePackage(%s) failed - Max package limit reached: %d", name, MaxActivePackages);
        return;
    }
    if (!name)
        return;

    // see if this one's already active
    for (U32 i = 0; i < mNumActivePackages; i++)
        if (mActivePackages[i] == name)
            return;

    // kill the cache
    trashCache();

    // find all the package namespaces...
    for (Namespace* walk = mNamespaceList; walk; walk = walk->mNext)
    {
        if (walk->mPackage == name)
        {
            Namespace* parent = Namespace::find(walk->mName);
            // hook the parent
            walk->mParent = parent->mParent;
            parent->mParent = walk;

            // now swap the entries:
            Entry* ew;
            for (ew = parent->mEntryList; ew; ew = ew->mNext)
                ew->mNamespace = walk;

            for (ew = walk->mEntryList; ew; ew = ew->mNext)
                ew->mNamespace = parent;

            ew = walk->mEntryList;
            walk->mEntryList = parent->mEntryList;
            parent->mEntryList = ew;
        }
    }
    mActivePackages[mNumActivePackages++] = name;
}

void Namespace::deactivatePackage(StringTableEntry name)
{
    S32 i, j;
    for (i = 0; i < mNumActivePackages; i++)
        if (mActivePackages[i] == name)
            break;
    if (i == mNumActivePackages)
        return;

    trashCache();

    for (j = mNumActivePackages - 1; j >= i; j--)
    {
        // gotta unlink em in reverse order...
        for (Namespace* walk = mNamespaceList; walk; walk = walk->mNext)
        {
            if (walk->mPackage == mActivePackages[j])
            {
                Namespace* parent = Namespace::find(walk->mName);
                // hook the parent
                parent->mParent = walk->mParent;
                walk->mParent = NULL;

                // now swap the entries:
                Entry* ew;
                for (ew = parent->mEntryList; ew; ew = ew->mNext)
                    ew->mNamespace = walk;

                for (ew = walk->mEntryList; ew; ew = ew->mNext)
                    ew->mNamespace = parent;

                ew = walk->mEntryList;
                walk->mEntryList = parent->mEntryList;
                parent->mEntryList = ew;
            }
        }
    }
    mNumActivePackages = i;
}

void Namespace::unlinkPackages()
{
    mOldNumActivePackages = mNumActivePackages;
    if (!mNumActivePackages)
        return;
    deactivatePackage(mActivePackages[0]);
}

void Namespace::relinkPackages()
{
    if (!mOldNumActivePackages)
        return;
    for (U32 i = 0; i < mOldNumActivePackages; i++)
        activatePackage(mActivePackages[i]);
}

ConsoleFunctionGroupBegin(Packages, "Functions relating to the control of packages.");

ConsoleFunction(isPackage, bool, 2, 2, "isPackage(packageName)")
{
    argc;
    StringTableEntry packageName = StringTable->insert(argv[1]);
    return Namespace::isPackage(packageName);
}

ConsoleFunction(activatePackage, void, 2, 2, "activatePackage(packageName)")
{
    argc;
    StringTableEntry packageName = StringTable->insert(argv[1]);
    Namespace::activatePackage(packageName);
}

ConsoleFunction(deactivatePackage, void, 2, 2, "deactivatePackage(packageName)")
{
    argc;
    StringTableEntry packageName = StringTable->insert(argv[1]);
    Namespace::deactivatePackage(packageName);
}

ConsoleFunctionGroupEnd(Packages);
