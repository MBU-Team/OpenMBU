//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "core/fileStream.h"
#include "atlas/core/atlasDeferredIO.h"
#include "console/console.h"
#include "platform/profiler.h"

AtlasDeferredIO::AtlasDeferredIO(ActionTypes _action)
{
    action = _action;
    data = NULL;
    cbObj = NULL;
    flags.clear();
}

AtlasDeferredIO::~AtlasDeferredIO()
{
    //   Con::printf("deleted adio @ %x %s", this, gProfiler->getProfilePath());
}

void AtlasDeferredIO::complete()
{
    // Do completion actions - delete, callback, etc.

    // Deal with the possibility of getting nuked - in this case our data
    // will be annihilated and we might get a value (ie 0xcececece) that 
    // passes one or more flag checks.
    bool quitAfterCallback = !flags.test(DeleteDataOnComplete | DeleteStructOnComplete);

    // Note it's completed.
    flags.set(IsCompleted);

    // Issue our callback, if we've got one.
    if (flags.test(DoCallback))
        (cbObj->*callback)(callbackKeyA, this);

    if (quitAfterCallback)
        return;

    // Deal with data cleanup, if needed.
    if (flags.test(DeleteDataOnComplete))
    {
        if (flags.test(DataIsArray))
            delete[] data;
        else
            delete data;
    }

    // If we have to kill ourselves, do it last. :)
    if (flags.test(DeleteStructOnComplete))
        delete this;
}

void AtlasDeferredIO::doAction(Stream* s)
{
    switch (action)
    {
    case ImmediateRead:
    case DeferredRead:
        // Allocate memory for read if it's not present.
        if (!data)
        {
            AssertISV(length, "AtlasDeferredIO::doAction - zero length!");
            data = dMalloc(length);
        }

        // Read operation...
        s->setPosition(offset);
        s->read(length, data);
        break;

    case ImmediateWrite:
    case DeferredWrite:
        // Deal with flags...
        if (flags.test(WriteToEndOfStream))
            offset = s->getStreamSize();

        // Write operation...
        s->setPosition(offset);
        s->write(length, data);

        // If it's a filestream, issue a flush so we won't miss anything...
        if (FileStream* fs = dynamic_cast<FileStream*>(s))
            fs->flush();

        break;

    default:
        AssertISV(false, "AtlasDeferredIO::doAction - unknown action!");
    }
}