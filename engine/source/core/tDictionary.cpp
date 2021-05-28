//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) GarageGames, All Rights Reserved
//-----------------------------------------------------------------------------

#include "core/tDictionary.h"

namespace DictHash
{

    static U32 Primes[] = {
      53ul,         97ul,         193ul,       389ul,       769ul,
      1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
      49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
      1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
      50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
      1610612741ul, 3221225473ul, 4294967291ul
    };

    U32 nextPrime(U32 size)
    {
        U32 len = sizeof(Primes) / sizeof(U32);
        for (U32 i = 0; i < len; i++)
            if (Primes[i] > size)
                return Primes[i];

        return Primes[len - 1];
    }

};
