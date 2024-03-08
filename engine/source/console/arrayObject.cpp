#include "arrayObject.h"


//------------------------------------------------------------------------------
// Array struct methods
//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ArrayObject);

ArrayObject::ArrayObject() : Parent()
{

}

ArrayObject::~ArrayObject()
{
}

void ArrayObject::addEntry(const Entry& entry) {
    val.push_back(entry);
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d add %s", mObjectId, entry.c_str());
#endif
}
void ArrayObject::replaceEntry(U32 index, const Entry& newEntry) {
    if (index >= val.size())
        return;
    val[index] = newEntry;
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d replace index %d with %s", mObjectId, index, newEntry.c_str());
#endif
}
void ArrayObject::insertEntryBefore(U32 index, const Entry& entry) {
    if (index > val.size())
        return;
    val.insert(val.begin() + index, entry);
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d insert before %d with %s", mObjectId, index, entry.c_str());
#endif
}
void ArrayObject::removeEntry(U32 index) {
    if (index >= val.size())
        return;
    val.erase(val.begin() + index);
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d remove %d", mObjectId, index);
#endif
}
void ArrayObject::removeMatching(const Entry& match) {
    std::vector<Entry>::iterator it = std::find(val.begin(), val.end(), match);
    if (it != val.end()) {
        val.erase(it);
#if DEBUG_ARRAY
        TGE::Con::printf("Array %d remove matching %s", mObjectId, match.c_str());
#endif
    }
}
void ArrayObject::clear() {
    val.clear();
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d clear");
#endif

}
void ArrayObject::swap(U32 index1, U32 index2) {
    std::swap(val[index1], val[index2]);
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d swap %d and %d", mObjectId, index1, index2);
#endif
}

struct ScriptCompare {
    std::string func;
    ScriptCompare(const char* func) : func(func) {}

    bool operator()(const std::string& a, const std::string& b) const {
        bool result = dAtob(Con::executef(3, func.c_str(), a.c_str(), b.c_str()));
#if DEBUG_ARRAY
        TGE::Con::printf("Array sort %s and %s returned %s", a.c_str(), b.c_str(), (result ? "true" : "false"));
#endif
        return result;
    }
};

void ArrayObject::sort(const char* scriptFunc) {
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d sorting...", mObjectId);
#endif
    sort(ScriptCompare(scriptFunc));
}

struct NumericCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        bool result = dAtof(a.c_str()) < dAtof(b.c_str());
#if DEBUG_ARRAY
        TGE::Con::printf("Array sort %s and %s returned %s", a.c_str(), b.c_str(), (result ? "true" : "false"));
#endif
        return result;
    }
};
//Default sort is numerical
void ArrayObject::sort() {
#if DEBUG_ARRAY
    TGE::Con::printf("Array %d sorting...", mObjectId);
#endif
    sort(NumericCompare());
}

const ArrayObject::Entry& ArrayObject::getEntry(U32 index) const {
    return val[index];
}
ArrayObject::Entry& ArrayObject::getEntry(U32 index) {
    return val[index];
}
S32 ArrayObject::getSize() const {
    return val.size();
}

bool ArrayObject::contains(const Entry& match) const {
    std::vector<Entry>::const_iterator it = std::find(val.begin(), val.end(), match);
    return it != val.end();
}

//------------------------------------------------------------------------------
// Console functions
//------------------------------------------------------------------------------

ConsoleFunction(Array, S32, 1, 32, "Array([name, obj1, ...])") {
    //Something about ArrayObject::create clobbers the first couple arguments'
    // memory. So this is a cheap, dirty solution for that.
    std::vector<std::string> args;
    for (S32 i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    ArrayObject* array = new ArrayObject();
    array->registerObject();

    for (U32 i = 0; i < args.size(); i++) {
        array->addEntry(args[i]);
    }

    return array->getId();
}

ConsoleMethod(ArrayObject, addEntry, S32, 3, 3, "(%entry)") {
    object->addEntry(argv[2]);
    return object->getId();
}
ConsoleMethod(ArrayObject, replaceEntryByIndex, S32, 4, 4, "(%index, %entry)") {
    object->replaceEntry(dAtoi(argv[2]), argv[3]);
    return object->getId();
}
ConsoleMethod(ArrayObject, insertEntryBefore, S32, 4, 4, "(%index, %entry)") {
    object->insertEntryBefore(dAtoi(argv[2]), argv[3]);
    return object->getId();
}
ConsoleMethod(ArrayObject, removeEntryByIndex, S32, 3, 3, "(%index)") {
    object->removeEntry(dAtoi(argv[2]));
    return object->getId();
}
ConsoleMethod(ArrayObject, removeEntry, S32, 3, 3, "(%index)") {
    object->removeEntry(dAtoi(argv[2]));
    return object->getId();
}
ConsoleMethod(ArrayObject, removeEntriesByContents, S32, 3, 3, "(%match)") {
    object->removeMatching(argv[2]);
    return object->getId();
}
ConsoleMethod(ArrayObject, removeMatching, S32, 3, 3, "(%match)") {
    object->removeMatching(argv[2]);
    return object->getId();
}
ConsoleMethod(ArrayObject, clear, S32, 2, 2, "()") {
    object->clear();
    return object->getId();
}
ConsoleMethod(ArrayObject, swap, S32, 4, 4, "(%index1, %index2)") {
    object->swap(dAtoi(argv[2]), dAtoi(argv[3]));
    return object->getId();
}
ConsoleMethod(ArrayObject, sort, S32, 2, 3, "(%compareFn)") {
    if (argc == 2) {
        object->sort();
    }
    else {
        object->sort(argv[2]);
    }
    return object->getId();
}

ConsoleMethod(ArrayObject, getEntryByIndex, const char*, 3, 3, "(%index)") {
    S32 index = dAtoi(argv[2]);
    if (index >= object->getSize()) {
        return "";
    }
    if (index < 0) {
        return "";
    }
    return copyToReturnBuffer(object->getEntry(dAtoi(argv[2])));
}
ConsoleMethod(ArrayObject, getEntry, const char*, 3, 3, "(%index)") {
    S32 index = dAtoi(argv[2]);
    if (index >= object->getSize()) {
        return "";
    }
    if (index < 0) {
        return "";
    }
    return copyToReturnBuffer(object->getEntry(index));
}
ConsoleMethod(ArrayObject, getSize, S32, 2, 2, "()") {
    return object->getSize();
}

ConsoleMethod(ArrayObject, containsEntry, bool, 3, 3, "(%entry)") {
    return object->contains(argv[2]);
}
ConsoleMethod(ArrayObject, contains, bool, 3, 3, "(%entry)") {
    return object->contains(argv[2]);
}

//------------------------------------------------------------------------------
// Debugging
//------------------------------------------------------------------------------

ConsoleMethod(ArrayObject, dumpEntries, void, 2, 2, "()") {
    Con::printf("Array %s (%s) has %d entries:", object->getIdString(), object->getName(), object->getSize());
    for (S32 i = 0; i < object->getSize(); i++) {
        Con::printf("%d: %s", i, object->getEntry(i).c_str());
    }
}

char* copyToReturnBuffer(const std::string& string) {
    char* buffer = Con::getReturnBuffer(string.size() + 1);
    dStrcpy(buffer, string.data());
    return buffer;
}
