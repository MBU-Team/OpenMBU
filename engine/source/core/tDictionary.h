//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) GarageGames, All Rights Reserved
//-----------------------------------------------------------------------------

#ifndef CORE_TDICTIONARY_H
#define CORE_TDICTIONARY_H

#include "platform/platform.h"
#include "core/stringTable.h"

namespace DictHash
{
    inline dsize_t hash(dsize_t data)
    {
        return data;
    }

    inline dsize_t hash(const char* data)
    {
        return StringTable->hashString(data);
    }

    inline dsize_t hash(const void* data)
    {
        return (dsize_t)data;
    }

    U32 nextPrime(U32);
};

namespace KeyCmp
{
    template<typename Key>
    inline bool equals(Key keya, Key keyb)
    {
        return (keya == keyb);
    }

    template<>
    inline bool equals<>(const char* keya, const char* keyb)
    {
        return (dStrcmp(keya, keyb) == 0);
    }
};

/// A HashTable template class.
///
/// The hash table class maps between a key and an associated value. Access
/// using the key is performed using a hash table.  The class provides
/// methods for both unique and equal keys. The global ::hash(Type) function
/// is used for hashing, see util/hash.h
/// @ingroup UtilContainers
template<typename Key, typename Value >
class HashTable
{
public:
    struct Pair {
        Key  key;
        Value value;
        Pair() {}
        Pair(Key k, Value v) : key(k), value(v) {}
    };

private:
    struct Node {
        Node* mNext;
        Pair mPair;
        Node() : mNext(0) {}
        Node(Pair p, Node* n) : mPair(p), mNext(n) {}
    };

    Node** mTable;                      ///< Hash table
    S32 mTableSize;                     ///< Hash table size
    U32 mSize;                          ///< Number of keys in the table

    dsize_t _hash(const Key& key) const;
    U32 _index(const Key& key) const;
    Node* _next(U32 index) const;
    void _resize(U32 size);
    void _destroy();

public:
    // Iterator support
    template<typename U, typename E, typename M>
    class _Iterator {
        friend class HashTable;
        E* mLink;
        M* mHashTable;
        operator E* ();
    public:
        typedef U  ValueType;
        typedef U* Pointer;
        typedef U& Reference;

        _Iterator()
        {
            mHashTable = 0;
            mLink = 0;
        }

        _Iterator(M* table, E* ptr)
        {
            mHashTable = table;
            mLink = ptr;
        }

        _Iterator& operator++()
        {
            mLink = mLink->mNext ? mLink->mNext :
                mHashTable->_next(mHashTable->_index(mLink->mPair.key) + 1);
            return *this;
        }

        _Iterator operator++(int)
        {
            _Iterator itr(*this);
            ++(*this);
            return itr;
        }

        bool operator==(const _Iterator& b) const
        {
            return mHashTable == b.mHashTable && mLink == b.mLink;
        }

        bool operator!=(const _Iterator& b) const
        {
            return !(*this == b);
        }

        U* operator->() const
        {
            return &mLink->mPair;
        }

        U& operator*() const
        {
            return mLink->mPair;
        }
    };

    // Types
    typedef Pair        ValueType;
    typedef Pair& Reference;
    typedef const Pair& ConstReference;

    typedef _Iterator<Pair, Node, HashTable>  Iterator;
    typedef _Iterator<const Pair, const Node, const HashTable>  ConstIterator;
    typedef S32         DifferenceType;
    typedef U32         SizeType;

    // Initialization
    HashTable();
    ~HashTable();
    HashTable(const HashTable& p);

    // Management
    U32  size() const;                  ///< Return the number of elements
    U32  tableSize() const;             ///< Return the size of the hash bucket table
    void clear();                       ///< Empty the HashTable
    void resize(U32 size);
    bool isEmpty() const;               ///< Returns true if the table is empty
    F32 collisions() const;             ///< Returns the average number of nodes per bucket

    // Insert & erase elements
    Iterator insertEqual(const Key& key, const Value&);
    Iterator insertUnique(const Key& key, const Value&);
    void erase(Iterator);               ///< Erase the given entry
    void erase(const Key& key);         ///< Erase all matching keys from the table

    // HashTable lookup
    Iterator findOrInsert(const Key& key);
    Iterator find(const Key&);          ///< Find the first entry for the given key
    ConstIterator find(const Key&) const;    ///< Find the first entry for the given key
    S32 count(const Key&);              ///< Count the number of matching keys in the table

    // Forward Iterator access
    Iterator       begin();             ///< Iterator to first element
    ConstIterator begin() const;        ///< Iterator to first element
    Iterator       end();               ///< Iterator to last element + 1
    ConstIterator end() const;          ///< Iterator to last element + 1

    void operator=(const HashTable& p);
};

template<typename Key, typename Value> HashTable<Key, Value>::HashTable()
{
    mTableSize = 0;
    mTable = 0;
    mSize = 0;
}

template<typename Key, typename Value> HashTable<Key, Value>::HashTable(const HashTable& p)
{
    mSize = 0;
    mTableSize = 0;
    mTable = 0;
    *this = p;
}

template<typename Key, typename Value> HashTable<Key, Value>::~HashTable()
{
    _destroy();
}


//-----------------------------------------------------------------------------

template<typename Key, typename Value>
inline dsize_t HashTable<Key, Value>::_hash(const Key& key) const
{
    return DictHash::hash(key);
}

template<typename Key, typename Value>
inline U32 HashTable<Key, Value>::_index(const Key& key) const
{
    return _hash(key) % mTableSize;
}

template<typename Key, typename Value>
typename HashTable<Key, Value>::Node* HashTable<Key, Value>::_next(U32 index) const
{
    for (; index < mTableSize; index++)
        if (Node* node = mTable[index])
            return node;
    return 0;
}

template<typename Key, typename Value>
void HashTable<Key, Value>::_resize(U32 size)
{
    S32 currentSize = mTableSize;
    mTableSize = DictHash::nextPrime(size);
    Node** table = new Node * [mTableSize];
    dMemset(table, 0, mTableSize * sizeof(Node*));

    for (S32 i = 0; i < currentSize; i++)
        for (Node* node = mTable[i]; node; )
        {
            // Get groups of matching keys
            Node* last = node;
            while (last->mNext && last->mNext->mPair.key == node->mPair.key)
                last = last->mNext;

            // Move the chain to the new table
            Node** link = &table[_index(node->mPair.key)];
            Node* tmp = last->mNext;
            last->mNext = *link;
            *link = node;
            node = tmp;
        }

    delete[] mTable;
    mTable = table;
}

template<typename Key, typename Value>
void HashTable<Key, Value>::_destroy()
{
    for (S32 i = 0; i < mTableSize; i++)
        for (Node* ptr = mTable[i]; ptr; )
        {
            Node* tmp = ptr;
            ptr = ptr->mNext;
            delete tmp;
        }
    delete[] mTable;
    mTable = NULL;
}


//-----------------------------------------------------------------------------
// management

template<typename Key, typename Value>
inline U32 HashTable<Key, Value>::size() const
{
    return mSize;
}

template<typename Key, typename Value>
inline U32 HashTable<Key, Value>::tableSize() const
{
    return mTableSize;
}

template<typename Key, typename Value>
inline void HashTable<Key, Value>::clear()
{
    _destroy();
    mTableSize = 0;
    mSize = 0;
}

/// Resize the bucket table for an estimated number of elements.
/// This method will optimize the hash bucket table size to hold the given
/// number of elements.  The size argument is a hint, and will not be the
/// exact size of the table.  If the table is sized down below it's optimal
/// size, the next insert will cause it to be resized again. Normally this
/// function is used to avoid resizes when the number of elements that will
/// be inserted is known in advance.
template<typename Key, typename Value>
inline void HashTable<Key, Value>::resize(U32 size)
{
    _resize(size);
}

template<typename Key, typename Value>
inline bool HashTable<Key, Value>::isEmpty() const
{
    return mSize == 0;
}

template<typename Key, typename Value>
inline F32 HashTable<Key, Value>::collisions() const
{
    S32 chains = 0;
    for (S32 i = 0; i < mTableSize; i++)
        if (mTable[i])
            chains++;
    return F32(mSize) / chains;
}


//-----------------------------------------------------------------------------
// add & remove elements

/// Insert the key value pair but don't insert duplicates.
/// This insert method does not insert duplicate keys. If the key already exists in
/// the table the function will fail and end() is returned.
template<typename Key, typename Value>
typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::insertUnique(const Key& key, const Value& x)
{
    if (mSize >= mTableSize)
        _resize(mSize + 1);
    Node** table = &mTable[_index(key)];
    for (Node* itr = *table; itr; itr = itr->mNext)
        if (KeyCmp::equals<Key>(itr->mPair.key, key))
            return end();

    mSize++;
    *table = new Node(Pair(key, x), *table);
    return Iterator(this, *table);
}

/// Insert the key value pair and allow duplicates.
/// This insert method allows duplicate keys.  Keys are grouped together but
/// are not sorted.
template<typename Key, typename Value>
typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::insertEqual(const Key& key, const Value& x)
{
    if (mSize >= mTableSize)
        _resize(mSize + 1);
    // The new key is inserted at the head of any group of matching keys.
    Node** prev = &mTable[_index(key)];
    for (Node* itr = *prev; itr; prev = &itr->mNext, itr = itr->mNext)
        if (KeyCmp::equals<Key>(itr->mPair.key, key))
            break;
    mSize++;
    *prev = new Node(Pair(key, x), *prev);
    return Iterator(this, *prev);
}

template<typename Key, typename Value>
void HashTable<Key, Value>::erase(const Key& key)
{
    Node** prev = &mTable[_index(key)];
    for (Node* itr = *prev; itr; prev = &itr->mNext, itr = itr->mNext)
        if (KeyCmp::equals<Key>(itr->mPair.key, key)) {
            // Delete matching keys, which should be grouped together.
            do {
                Node* tmp = itr;
                itr = itr->mNext;
                delete tmp;
                mSize--;
            } while (itr && KeyCmp::equals<Key>(itr->mPair.key, key));
            *prev = itr;
            return;
        }
}

template<typename Key, typename Value>
void HashTable<Key, Value>::erase(Iterator node)
{
    Node** prev = &mTable[_index(node->key)];
    for (Node* itr = *prev; itr; prev = &itr->mNext, itr = itr->mNext)
        if (itr == node.mLink) {
            *prev = itr->mNext;
            delete itr;
            mSize--;
            return;
        }
}


//-----------------------------------------------------------------------------

/// Find the key, or insert a one if it doesn't exist.
/// Returns the first key in the table that matches, or inserts one if there
/// are none.
template<typename Key, typename Value>
typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::findOrInsert(const Key& key)
{
    if (mSize >= mTableSize)
        _resize(mSize + 1);
    Node** table = &mTable[_index(key)];
    for (Node* itr = *table; itr; itr = itr->mNext)
        if (KeyCmp::equals<Key>(itr->mPair.key, key))
            return Iterator(this, itr);
    mSize++;
    *table = new Node(Pair(key, Value()), *table);
    return Iterator(this, *table);
}

template<typename Key, typename Value>
typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::find(const Key& key)
{
    if (mTableSize)
        for (Node* itr = mTable[_index(key)]; itr; itr = itr->mNext)
            if (KeyCmp::equals<Key>(itr->mPair.key, key))
                return Iterator(this, itr);
    return Iterator(this, 0);
}

template<typename Key, typename Value>
S32 HashTable<Key, Value>::count(const Key& key)
{
    S32 count = 0;
    if (mTableSize)
        for (Node* itr = mTable[_index(key)]; itr; itr = itr->mNext)
            if (KeyCmp::equals<Key>(itr->mPair.key, key)) {
                // Matching keys should be grouped together.
                do {
                    count++;
                    itr = itr->mNext;
                } while (itr && KeyCmp::equals<Key>(itr->mPair.key, key));
                break;
            }
    return count;
}


//-----------------------------------------------------------------------------
// Iterator access

template<typename Key, typename Value>
inline typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::begin()
{
    return Iterator(this, _next(0));
}

template<typename Key, typename Value>
inline typename HashTable<Key, Value>::ConstIterator HashTable<Key, Value>::begin() const
{
    return ConstIterator(this, _next(0));
}

template<typename Key, typename Value>
inline typename HashTable<Key, Value>::Iterator HashTable<Key, Value>::end()
{
    return Iterator(this, 0);
}

template<typename Key, typename Value>
inline typename HashTable<Key, Value>::ConstIterator HashTable<Key, Value>::end() const
{
    return ConstIterator(this, 0);
}


//-----------------------------------------------------------------------------
// operators

template<typename Key, typename Value>
void HashTable<Key, Value>::operator=(const HashTable& p)
{
    _destroy();
    mTableSize = p.mTableSize;
    mTable = new Node * [mTableSize];
    mSize = p.mSize;
    for (S32 i = 0; i < mTableSize; i++)
        if (Node* itr = p.mTable[i])
        {
            Node** head = &mTable[i];
            do
            {
                *head = new Node(itr->mPair, 0);
                head = &(*head)->mNext;
                itr = itr->mNext;
            } while (itr);
        }
        else
            mTable[i] = 0;
}

//-----------------------------------------------------------------------------
// Iterator class

/// A Map template class.
/// The map class maps between a key and an associated value. Keys
/// are unique.
/// The hash table class is used as the default implementation so the
/// the key must be hashable, see util/hash.h for details.
/// @ingroup UtilContainers
template<typename Key, typename Value, class Sequence = HashTable<Key, Value> >
class Map : private Sequence
{
    typedef HashTable<Key, Value> Parent;

private:
    Sequence mMap;

public:
    // types
    typedef typename Parent::Pair Pair;
    typedef Pair        ValueType;
    typedef Pair& Reference;
    typedef const Pair& ConstReference;

    typedef typename Parent::Iterator  Iterator;
    typedef typename Parent::ConstIterator ConstIterator;
    typedef S32         DifferenceType;
    typedef U32         SizeType;

    // initialization
    Map() {}
    ~Map() {}
    Map(const Map& p);

    // management
    U32  size() const;                  ///< Return the number of elements
    void clear();                       ///< Empty the Map
    bool isEmpty() const;               ///< Returns true if the map is empty

    // insert & erase elements
    Iterator insert(const Key& key, const Value&); // Documented below...
    void erase(Iterator);               ///< Erase the given entry
    void erase(const Key& key);         ///< Erase the key from the map

    // Map lookup
    Iterator find(const Key&);          ///< Find entry for the given key
    ConstIterator find(const Key&) const;    ///< Find entry for the given key
    bool contains(const Key& a)
    {
        return mMap.count(a) > 0;
    }

    // forward Iterator access
    Iterator       begin();             ///< Iterator to first element
    ConstIterator begin() const;       ///< Iterator to first element
    Iterator       end();               ///< IIterator to last element + 1
    ConstIterator end() const;         ///< Iterator to last element + 1

    // operators
    Value& operator[](const Key&);      ///< Index using the given key. If the key is not currently in the map it is added.
};

template<typename Key, typename Value, class Sequence> Map<Key, Value, Sequence>::Map(const Map& p)
{
    *this = p;
}


//-----------------------------------------------------------------------------
// management

template<typename Key, typename Value, class Sequence>
inline U32 Map<Key, Value, Sequence>::size() const
{
    return mMap.size();
}

template<typename Key, typename Value, class Sequence>
inline void Map<Key, Value, Sequence>::clear()
{
    mMap.clear();
}

template<typename Key, typename Value, class Sequence>
inline bool Map<Key, Value, Sequence>::isEmpty() const
{
    return mMap.isEmpty();
}


//-----------------------------------------------------------------------------
// add & remove elements

/// Insert the key value pair but don't allow duplicates.
/// The map class does not allow duplicates keys. If the key already exists in
/// the map the function will fail and return end().
template<typename Key, typename Value, class Sequence>
typename Map<Key, Value, Sequence>::Iterator Map<Key, Value, Sequence>::insert(const Key& key, const Value& x)
{
    return mMap.insertUnique(key, x);
}

template<typename Key, typename Value, class Sequence>
void Map<Key, Value, Sequence>::erase(const Key& key)
{
    mMap.erase(key);
}

template<typename Key, typename Value, class Sequence>
void Map<Key, Value, Sequence>::erase(Iterator node)
{
    mMap.erase(node);
}


//-----------------------------------------------------------------------------
// Searching

template<typename Key, typename Value, class Sequence>
typename Map<Key, Value, Sequence>::Iterator Map<Key, Value, Sequence>::find(const Key& key)
{
    return mMap.find(key);
}

//-----------------------------------------------------------------------------
// Iterator access

template<typename Key, typename Value, class Sequence>
inline typename Map<Key, Value, Sequence>::Iterator Map<Key, Value, Sequence>::begin()
{
    return mMap.begin();
}

template<typename Key, typename Value, class Sequence>
inline typename Map<Key, Value, Sequence>::ConstIterator Map<Key, Value, Sequence>::begin() const
{
    return mMap.begin();
}

template<typename Key, typename Value, class Sequence>
inline typename Map<Key, Value, Sequence>::Iterator Map<Key, Value, Sequence>::end()
{
    return mMap.end();
}

template<typename Key, typename Value, class Sequence>
inline typename Map<Key, Value, Sequence>::ConstIterator Map<Key, Value, Sequence>::end() const
{
    return mMap.end();
}


//-----------------------------------------------------------------------------
// operators

template<typename Key, typename Value, class Sequence>
inline Value& Map<Key, Value, Sequence>::operator[](const Key& key)
{
    return mMap.findOrInsert(key)->value;
}

#endif

