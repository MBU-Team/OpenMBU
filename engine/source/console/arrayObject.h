#pragma once
#include "simBase.h"
#include <string>
#include <vector>
#include <algorithm>

class ArrayObject : public SimObject {
    typedef SimObject Parent;
public:

    ArrayObject();
    virtual ~ArrayObject();
    DECLARE_CONOBJECT(ArrayObject);


public:
    typedef std::string Entry;
protected:
    std::vector<Entry> val;

public:
    //Managing data
    void addEntry(const Entry& entry);
    void replaceEntry(U32 index, const Entry& newEntry);
    void insertEntryBefore(U32 index, const Entry& entry);
    void removeEntry(U32 index);
    void removeMatching(const Entry& match);
    void clear();
    void swap(U32 index1, U32 index2);

    template<typename T>
    void sort(const T& compare);

    void sort(const char* scriptFunc);
    void sort();

    //Retrieving data
    const Entry& getEntry(U32 index) const;
    Entry& getEntry(U32 index);
    S32 getSize() const;

    //Checking if an entry exists
    bool contains(const Entry& match) const;
};

char* copyToReturnBuffer(const std::string& string);

template<typename T>
inline void ArrayObject::sort(const T& compare) {
    std::sort(val.begin(), val.end(), compare);
}
