//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "core/idGenerator.h"
#include "core/bitStream.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "sim/netObject.h"
#include "game/gameConnection.h"
#include "game/net/serverQuery.h"

StringTableEntry gMasterAddress;

//----------------------------------------------------------------
// remote procedure call console functions
//----------------------------------------------------------------

class RemoteCommandEvent : public NetEvent
{
public:
    enum {
        MaxRemoteCommandArgs = 20,
        CommandArgsBits = 5
    };

private:
    S32 mArgc;
    char* mArgv[MaxRemoteCommandArgs + 1];
    StringHandle mTagv[MaxRemoteCommandArgs + 1];
    static char mBuf[1024];
public:
    RemoteCommandEvent(S32 argc = 0, const char** argv = NULL, NetConnection* conn = NULL)
    {
        mArgc = argc;
        for (S32 i = 0; i < argc; i++)
        {
            if (argv[i][0] == StringTagPrefixByte)
            {
                char buffer[256];
                mTagv[i + 1] = StringHandle(dAtoi(argv[i] + 1));
                if (conn)
                {
                    dSprintf(buffer + 1, sizeof(buffer) - 1, "%d", conn->getNetSendId(mTagv[i + 1]));
                    buffer[0] = StringTagPrefixByte;
                    mArgv[i + 1] = dStrdup(buffer);
                }
            }
            else
                mArgv[i + 1] = dStrdup(argv[i]);
        }
    }

#ifdef TORQUE_DEBUG_NET
    const char* getDebugName()
    {
        static char buffer[256];
        dSprintf(buffer, sizeof(buffer), "%s [%s]", getClassName(), gNetStringTable->lookupString(dAtoi(mArgv[1] + 1)));
        return buffer;
    }
#endif

    ~RemoteCommandEvent()
    {
        for (S32 i = 0; i < mArgc; i++)
            dFree(mArgv[i + 1]);
    }

    virtual void pack(NetConnection* conn, BitStream* bstream)
    {
        bstream->writeInt(mArgc, CommandArgsBits);
        // write it out reversed... why?
        // automatic string substitution with later arguments -
        // handled automatically by the system.

        for (S32 i = 0; i < mArgc; i++)
            conn->packString(bstream, mArgv[i + 1]);
    }

    virtual void write(NetConnection* conn, BitStream* bstream)
    {
        pack(conn, bstream);
    }

    virtual void unpack(NetConnection* conn, BitStream* bstream)
    {

        mArgc = bstream->readInt(CommandArgsBits);
        // read it out backwards
        for (S32 i = 0; i < mArgc; i++)
        {
            conn->unpackString(bstream, mBuf);
            mArgv[i + 1] = dStrdup(mBuf);
        }
    }

    virtual void process(NetConnection* conn)
    {
        static char idBuf[10];

        // de-tag the command name

        for (S32 i = mArgc - 1; i >= 0; i--)
        {
            char* arg = mArgv[i + 1];
            if (*arg == StringTagPrefixByte)
            {
                // it's a tag:
                U32 localTag = dAtoi(arg + 1);
                StringHandle tag = conn->translateRemoteStringId(localTag);
                NetStringTable::expandString(tag,
                    mBuf,
                    sizeof(mBuf),
                    (mArgc - 1) - i,
                    (const char**)(mArgv + i + 2));
                dFree(mArgv[i + 1]);
                mArgv[i + 1] = dStrdup(mBuf);
            }
        }
        const char* rmtCommandName = dStrchr(mArgv[1], ' ') + 1;
        if (conn->isConnectionToServer())
        {
            dStrcpy(mBuf, "clientCmd");
            dStrcat(mBuf, rmtCommandName);

            char* temp = mArgv[1];
            mArgv[1] = mBuf;

            Con::execute(mArgc, (const char**)mArgv + 1);
            mArgv[1] = temp;
        }
        else
        {
            dStrcpy(mBuf, "serverCmd");
            dStrcat(mBuf, rmtCommandName);
            char* temp = mArgv[1];

            dSprintf(idBuf, sizeof(idBuf), "%d", conn->getId());
            mArgv[0] = mBuf;
            mArgv[1] = idBuf;

            Con::execute(mArgc + 1, (const char**)mArgv);
            mArgv[1] = temp;
        }
    }

    DECLARE_CONOBJECT(RemoteCommandEvent);
};
char RemoteCommandEvent::mBuf[1024];

IMPLEMENT_CO_NETEVENT_V1(RemoteCommandEvent);

static void sendRemoteCommand(NetConnection* conn, S32 argc, const char** argv)
{
    if (U8(argv[0][0]) != StringTagPrefixByte)
    {
        Con::errorf(ConsoleLogEntry::Script, "Remote Command Error - command must be a tag.");
        return;
    }
    S32 i;
    for (i = argc - 1; i >= 0; i--)
    {
        if (argv[i][0] != 0)
            break;
        argc = i;
    }
    for (i = 0; i < argc; i++)
        conn->validateSendString(argv[i]);
    RemoteCommandEvent* cevt = new RemoteCommandEvent(argc, argv, conn);
    conn->postNetEvent(cevt);
}

ConsoleFunctionGroupBegin(Net, "Functions for use with the network; tagged strings and remote commands.");

ConsoleFunction(commandToServer, void, 2, RemoteCommandEvent::MaxRemoteCommandArgs + 1, "(string func, ...)"
    "Send a command to the server.")
{
    NetConnection* conn = NetConnection::getConnectionToServer();
    if (!conn)
        return;
    sendRemoteCommand(conn, argc - 1, argv + 1);
}

ConsoleFunction(commandToClient, void, 3, RemoteCommandEvent::MaxRemoteCommandArgs + 2, "(NetConnection client, string func, ...)")
{
    NetConnection* conn;
    if (!Sim::findObject(argv[1], conn))
        return;
    sendRemoteCommand(conn, argc - 2, argv + 2);
}

ConsoleFunction(removeTaggedString, void, 2, 2, "(int tag)")
{
    gNetStringTable->removeString(dAtoi(argv[1] + 1), true);
}

ConsoleFunction(addTaggedString, const char*, 2, 2, "(string str)")
{
    StringHandle s(argv[1]);
    gNetStringTable->incStringRefScript(s.getIndex());

    char* ret = Con::getReturnBuffer(10);
    ret[0] = StringTagPrefixByte;
    dSprintf(ret + 1, 9, "%d", s.getIndex());
    return ret;
}

ConsoleFunction(getTaggedString, const char*, 2, 2, "(int tag)")
{
    const char* indexPtr = argv[1];
    if (*indexPtr == StringTagPrefixByte)
        indexPtr++;
    return gNetStringTable->lookupString(dAtoi(indexPtr));
}

ConsoleFunction(buildTaggedString, const char*, 2, 11, "(string format, ...)")
{
    const char* indexPtr = argv[1];
    if (*indexPtr == StringTagPrefixByte)
        indexPtr++;
    const char* fmtString = gNetStringTable->lookupString(dAtoi(indexPtr));
    char* strBuffer = Con::getReturnBuffer(512);
    const char* fmtStrPtr = fmtString;
    char* strBufPtr = strBuffer;
    S32 strMaxLength = 511;
    if (!fmtString)
        goto done;

    //build the string
    while (*fmtStrPtr)
    {
        //look for an argument tag
        if (*fmtStrPtr == '%')
        {
            if (fmtStrPtr[1] >= '1' && fmtStrPtr[1] <= '9')
            {
                S32 argIndex = S32(fmtStrPtr[1] - '0') + 1;
                if (argIndex >= argc)
                    goto done;
                const char* argStr = argv[argIndex];
                if (!argStr)
                    goto done;
                S32 strLength = dStrlen(argStr);
                if (strLength > strMaxLength)
                    goto done;
                dStrcpy(strBufPtr, argStr);
                strBufPtr += strLength;
                strMaxLength -= strLength;
                fmtStrPtr += 2;
                continue;
            }
        }

        //if we don't continue, just copy the character
        if (strMaxLength <= 0)
            goto done;
        *strBufPtr++ = *fmtStrPtr++;
        strMaxLength--;
    }

done:
    *strBufPtr = '\0';
    return strBuffer;
}

ConsoleFunctionGroupEnd(Net);

void netInit()
{
    gMasterAddress = "";
    Con::addVariable("MasterServerAddress", TypeString, &gMasterAddress);
}
