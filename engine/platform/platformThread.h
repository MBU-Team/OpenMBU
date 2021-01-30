//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMTHREAD_H_
#define _PLATFORMTHREAD_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif

typedef void (*ThreadRunFunction)(void*);

class Thread
{
protected:
    void* mData;

public:
    Thread(ThreadRunFunction func = 0, void* arg = 0, bool start_thread = true);
    virtual ~Thread();

    virtual void start();
    virtual bool join();

    virtual void run(void* arg = 0);

    virtual bool isAlive();

    static U32 getCurrentThreadId();
};

#endif
