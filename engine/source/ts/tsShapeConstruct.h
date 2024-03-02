//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TSSHAPECONSTRUCT_H_
#define _TSSHAPECONSTRUCT_H_

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

/// This class allows an artist to export their animations for the model
/// into the .dsq format.  This class in particular matches up the model
/// with the .dsqs to create a nice animated model.
class TSShapeConstructor : public SimDataBlock
{
    typedef SimDataBlock Parent;

    enum {
        NumSequenceBits = 7,
        MaxSequences = (1 << NumSequenceBits) - 1
    };

    StringTableEntry mShape;
    StringTableEntry mSequence[MaxSequences];

    Resource<TSShape> hShape;

public:

    TSShapeConstructor();
    ~TSShapeConstructor();
    bool onAdd();
    bool preload(bool server, char errorBuffer[256]);
    void packData(BitStream* stream);
    void unpackData(BitStream* stream);

    DECLARE_CONOBJECT(TSShapeConstructor);
    static void initPersistFields();
    static TSShapeConstructor* findShapeConstructor(const char* path);
};

#endif
