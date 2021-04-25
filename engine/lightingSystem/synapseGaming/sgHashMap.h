//---------------------------------------------------------------
// Synapse Gaming - Hash Map
// Copyright © Synapse Gaming 2004 - 2005
// Written by John Kabus
//---------------------------------------------------------------

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include "platform/types.h"
#include "core/tVector.h"


template<class T, class Tinfo> class hash_multimap
{
public:
    U32 hashCode;
    // entry level info or single object per-entry...
    Tinfo info;
    // for storing multiple objects per-entry...
    Vector<T> object;
    hash_multimap* mapHigh;
    hash_multimap* mapLow;
    hash_multimap* linkHigh;
    hash_multimap* linkLow;
    hash_multimap()
    {
        // theoretical mean for hash code (balance the tree)...
        hashCode = 0x7fffffff;
        // this unfortunately clears objects after the constructor
        // setups up values, but is necessary for clearing pointers...
        dMemset(&info, 0, sizeof(Tinfo));
        mapHigh = mapLow = linkHigh = linkLow = NULL;
    }
    hash_multimap(U32 hashcode)
    {
        hashCode = hashcode;
        // this unfortunately clears objects after the constructor
        // setups up values, but is necessary for clearing pointers...
        dMemset(&info, 0, sizeof(Tinfo));
        mapHigh = mapLow = linkHigh = linkLow = NULL;
    }
    virtual ~hash_multimap()
    {
        clear();
    }
    void clear()
    {
        if (mapHigh)
            delete mapHigh;
        if (mapLow)
            delete mapLow;
        mapHigh = mapLow = linkHigh = linkLow = NULL;
    }
    void relink(hash_multimap* high, hash_multimap* low, hash_multimap* insert)
    {
        if (high)
            high->linkLow = insert;
        if (low)
            low->linkHigh = insert;
        insert->linkHigh = high;
        insert->linkLow = low;
    }
    hash_multimap* find(U32 hashcode)
    {
        if (hashcode == hashCode)
            return this;
        else if (hashcode < hashCode)
        {
            if (!mapLow)
            {
                mapLow = new hash_multimap(hashcode);
                relink(this, this->linkLow, mapLow);
                return mapLow;
            }
            return mapLow->find(hashcode);
        }
        else
        {
            if (!mapHigh)
            {
                mapHigh = new hash_multimap(hashcode);
                relink(this->linkHigh, this, mapHigh);
                return mapHigh;
            }
            return mapHigh->find(hashcode);
        }
    }
};


#endif//HASHMAP_H_
