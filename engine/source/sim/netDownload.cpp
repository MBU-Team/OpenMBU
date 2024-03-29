//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (c) GarageGames.Com
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/simBase.h"
#include "sim/netConnection.h"
#include "core/bitStream.h"
#include "sim/netObject.h"
#include "core/resManager.h"
#include "console/consoleTypes.h"
#include "sim/netInterface.h"

class FileDownloadRequestEvent : public NetEvent
{
public:
    enum
    {
        MaxFileNames = 31,
    };

    U32 nameCount;
    char mFileNames[MaxFileNames][256];

    FileDownloadRequestEvent(Vector<char*>* nameList = NULL)
    {
        nameCount = 0;
        if (nameList)
        {
            nameCount = nameList->size();

            if (nameCount > MaxFileNames)
                nameCount = MaxFileNames;

            for (U32 i = 0; i < nameCount; i++)
            {
                dStrcpy(mFileNames[i], (*nameList)[i]);
                Con::printf("Sending request for file %s", mFileNames[i]);
            }
        }
    }

    virtual void pack(NetConnection*, BitStream* bstream)
    {
        bstream->writeRangedU32(nameCount, 0, MaxFileNames);
        for (U32 i = 0; i < nameCount; i++)
            bstream->writeString(mFileNames[i]);
    }

    virtual void write(NetConnection*, BitStream* bstream)
    {
        bstream->writeRangedU32(nameCount, 0, MaxFileNames);
        for (U32 i = 0; i < nameCount; i++)
            bstream->writeString(mFileNames[i]);
    }

    virtual void unpack(NetConnection*, BitStream* bstream)
    {
        nameCount = bstream->readRangedU32(0, MaxFileNames);
        for (U32 i = 0; i < nameCount; i++)
            bstream->readString(mFileNames[i]);
    }

    virtual void process(NetConnection* connection)
    {
        U32 i;
        for (i = 0; i < nameCount; i++)
            if (connection->startSendingFile(mFileNames[i]))
                break;
        if (i == nameCount)
            connection->startSendingFile(NULL);  // none of the files were sent
    }

    DECLARE_CONOBJECT(FileDownloadRequestEvent);

};

IMPLEMENT_CO_NETEVENT_V1(FileDownloadRequestEvent);

class FileChunkEvent : public NetEvent
{
public:
    enum
    {
        ChunkSize = 63,
    };

    U8 chunkData[ChunkSize];
    U32 chunkLen;

    FileChunkEvent(U8* data = NULL, U32 len = 0)
    {
        if (data)
            dMemcpy(chunkData, data, len);
        chunkLen = len;
    }

    virtual void pack(NetConnection*, BitStream* bstream)
    {
        bstream->writeRangedU32(chunkLen, 0, ChunkSize);
        bstream->write(chunkLen, chunkData);
    }

    virtual void write(NetConnection*, BitStream* bstream)
    {
        bstream->writeRangedU32(chunkLen, 0, ChunkSize);
        bstream->write(chunkLen, chunkData);
    }

    virtual void unpack(NetConnection*, BitStream* bstream)
    {
        chunkLen = bstream->readRangedU32(0, ChunkSize);
        bstream->read(chunkLen, chunkData);
    }

    virtual void process(NetConnection* connection)
    {
        connection->chunkReceived(chunkData, chunkLen);
    }

    virtual void notifyDelivered(NetConnection* nc, bool madeIt)
    {
        if (!nc->isRemoved())
            nc->sendFileChunk();
    }

    DECLARE_CONOBJECT(FileChunkEvent);
};

IMPLEMENT_CO_NETEVENT_V1(FileChunkEvent);

void NetConnection::sendFileChunk()
{
    U8 buffer[FileChunkEvent::ChunkSize];
    U32 len = FileChunkEvent::ChunkSize;
    if (len + mCurrentFileBufferOffset > mCurrentFileBufferSize)
        len = mCurrentFileBufferSize - mCurrentFileBufferOffset;

    if (!len)
    {
        ResourceManager->closeStream(mCurrentDownloadingFile);
        mCurrentDownloadingFile = NULL;
        return;
    }

    mCurrentFileBufferOffset += len;
    mCurrentDownloadingFile->read(len, buffer);
    postNetEvent(new FileChunkEvent(buffer, len));

    Con::executef(this, 4, "onFileChunkSent", mCurrentFileName, Con::getIntArg(mCurrentFileBufferOffset), Con::getIntArg(mCurrentFileBufferSize));
}

void failedToFindFile(NetConnection* conn);

bool NetConnection::startSendingFile(const char* fileName)
{
    if (!fileName || Con::getBoolVariable("$NetConnection::neverUploadFiles"))
    {
        sendConnectionMessage(SendNextDownloadRequest);
        return false;
    }

    mCurrentDownloadingFile = ResourceManager->openStream(fileName);

    if (!mCurrentDownloadingFile)
    {
        // the server didn't have the file, so send a 0 byte chunk:
        Con::printf("No such file '%s'.", fileName);
        // postNetEvent(new FileChunkEvent(NULL, 0));
        failedToFindFile(this);
        return false;
    }

    if (mCurrentFileName)
    {
        dFree(mCurrentFileName);
    }
    mCurrentFileName = dStrdup(fileName);

    Con::printf("Sending file '%s'.", fileName);
    mCurrentFileBufferSize = mCurrentDownloadingFile->getStreamSize();
    mCurrentFileBufferOffset = 0;

    Con::executef(this, 4, "onFileChunkSent", fileName, Con::getIntArg(0), Con::getIntArg(mCurrentFileBufferSize));

#ifdef TORQUE_FAST_FILE_TRANSFER

    sendFastFile();

#else // TORQUE_FAST_FILE_TRANSFER

    // always have 32 file chunks (64 bytes each) in transit
    sendConnectionMessage(FileDownloadSizeMessage, mCurrentFileBufferSize);
    for (U32 i = 0; i < 32; i++)
        sendFileChunk();

#endif // TORQUE_FAST_FILE_TRANSFER
    return true;
}

void NetConnection::sendNextFileDownloadRequest()
{
    // see if we've already downloaded this file...
    while (mMissingFileList.size() && (ResourceManager->find(mMissingFileList[0]) || Con::getBoolVariable("$NetConnection::neverDownloadFiles")))
    {
        dFree(mMissingFileList[0]);
        mMissingFileList.pop_front();
    }

    if (mMissingFileList.size())
    {
        postNetEvent(new FileDownloadRequestEvent(&mMissingFileList));
    }
    else
    {
        fileDownloadSegmentComplete();
    }
}

#ifdef TORQUE_FAST_FILE_TRANSFER

const U32 MaxFilePacketSize = 1320;
static U32 FastFilePacketSize = 1280; // ~= 1450 UDP MTU - headers
static U32 PacketsAtATime = 512; // Realistically not sure how high this can go before it gets bad

static BitStream gFastFileStream(NULL, 0);
static U8 gFastFileBuffer[MaxFilePacketSize];

BitStream *buildFastFilePacket()
{
    // Header size guesstimate
    U32 headerSize = 40;
    gFastFileStream.setBuffer(
        gFastFileBuffer,
        MaxFilePacketSize + headerSize,
        MaxFilePacketSize + headerSize
    );
    gFastFileStream.setPosition(0);

    gFastFileStream.write(U8(NetInterface::FileTransferPacket));
    gFastFileStream.write(U8(0));  // flags
    gFastFileStream.write(U32(0)); // key

    return &gFastFileStream;
}

void sendFastFilePacket(const NetAddress *addr)
{
    Net::sendto(addr, gFastFileStream.getBuffer(), gFastFileStream.getPosition());
}

// File transfer steps:
// 1. Server sends client WriteRequest with chunk count

// 2. Server sends the next X unacknowledged packets of the file
//    Can use exponential backoff to influence value of X
// 3. Client receives file packets until it has been Y ms since last recv
//    Can use connection ping to influence value of Y
// 4. If client has all packets, transfer is complete
// 5. Client sends list of acknowledged packets to Server
// 6. Goto 2

// O(N) where N is strictly less than a small constant (11200) is basically O(1)

// Originally there were more of these and then they all gradually got replaced
// Left in for potential future expansion
enum Opcode
{
    Data = 3, // Packet contains data chunk
};

struct FileChunk
{
    U32 index;
    U32 offset;
    Vector<U8> bytes;
};

/// Class managing file transfer state (both sender and receiver)
/// It's one big class defined here so we don't pollute netConnection.h
struct FastFileState
{
    NetConnection* mConnection;

    /// State for connection side sending data
    struct
    {
        U32 transferID;
        U32 totalSize; // bytes
        Vector<FileChunk> fileChunks;
        Vector<bool> acknowledgedChunks; // 1 per file chunk

        // Temp state for ack events
        U32 lastAckStart;
        Vector<bool> lastAckAcks;
    } mSend;

    /// State for connection side receiving data
    struct
    {
        U32 transferID;
        U32 lastRecvTime; // platform real milliseconds
        U32 totalSize; // bytes
        Vector<FileChunk> fileChunks;
        Vector<bool> acknowledgedChunks; // 1 per file chunk
        U32 nextNonRecvChunk;
    } mRecv;

    FastFileState(NetConnection* connection)
    {
        mConnection = connection;
        mSend.transferID = 0;
        mRecv.transferID = 0;
        mRecv.lastRecvTime = 0;
    }

    /// [Sender] Write packet for start of transfer
    void writeWriteRequestPacket(BitStream* out)
    {
        // Sequence number
        out->write(U32(mSend.transferID));
        // U16 gives us ~=87.5MB max file size
        // And means we don't have to really care about memory overflow from in case the packet is read wrong
        out->write(U16(mSend.fileChunks.size()));
        // Just for progress indicators
        out->write(U32(mSend.totalSize));

#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d",
            __func__,
            __LINE__,
            mSend.transferID,
            mSend.fileChunks.size(),
            mSend.totalSize
        );
#endif
    }

    /// [Receiver] Read packet for start of transfer
    bool readWriteRequestPacket(BitStream* stream)
    {
        U32 transferID;
        U16 chunkCount;
        U32 totalSize;

        stream->read(&transferID);
        stream->read(&chunkCount);
        stream->read(&totalSize);

#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d",
            __func__,
            __LINE__,
            transferID,
            chunkCount,
            totalSize
        );
#endif

        // This is the start of receiving a file, so init recv state here

        mRecv.transferID = transferID;
        mRecv.totalSize = totalSize;
        mRecv.fileChunks.clear();
        mRecv.fileChunks.setSize(chunkCount);
        mRecv.acknowledgedChunks.clear();
        mRecv.acknowledgedChunks.setSize(chunkCount);
        mRecv.nextNonRecvChunk = 0;

        for (U16 i = 0; i < chunkCount; i++)
        {
            mRecv.acknowledgedChunks[i] = false;
        }
        return true;
    }

    /// [Sender] Write packet for data chunk
    void writeDataPacket(BitStream* out, U32 index)
    {
        // Header
        out->write(U16(Data));
        out->write(U32(mSend.transferID));
        // Packet metadata
        out->write(U32(index));
        out->write(U32(mSend.fileChunks[index].offset));
        out->write(U16(mSend.fileChunks[index].bytes.size()));
        // Raw data
        for (U32 i = 0; i < mSend.fileChunks[index].bytes.size(); i++)
        {
            out->write(U8(mSend.fileChunks[index].bytes[i]));
        }

#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d %d %d",
            __func__,
            __LINE__,
            Data,
            mSend.transferID,
            index,
            mSend.fileChunks[index].offset,
            mSend.fileChunks[index].bytes.size()
        );
#endif
    }

    /// [Receiver] Read packet for data chunk
    bool readDataPacket(BitStream* stream)
    {
        U32 transferID;
        U32 index;
        U32 offset;
        U16 size;
        Vector<U8> bytes;

        stream->read(&transferID);
        stream->read(&index);
        stream->read(&offset);
        stream->read(&size);

#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d %d",
            __func__,
            __LINE__,
            transferID,
            index,
            offset,
            size
        );
#endif

        // Basic checks to make sure nothing fishy is happening
        if (transferID == 0 || transferID != mRecv.transferID)
        {
            Con::errorf(
                "%s @%d Bad transfer id",
                __func__,
                __LINE__
            );
            return false;
        }

        // They need to have actually sent the number of bytes they claim
        for (U32 i = 0; i < size; i++)
        {
            U8 byte;
            if (!stream->read(&byte))
            {
                Con::errorf(
                    "%s @%d EOS",
                    __func__,
                    __LINE__
                );
                return false;
            }
            bytes.push_back(byte);
        }

        // Prevent overflow if index is wrong
        if (index >= mRecv.fileChunks.size())
        {
            Con::errorf(
                "%s @%d Index >= chunks size",
                __func__,
                __LINE__
            );
            return false;
        }

        // Store chunk in state (gets pulled out later)
        mRecv.fileChunks[index].index = index;
        mRecv.fileChunks[index].offset = offset;
        mRecv.fileChunks[index].bytes = bytes;
        mRecv.acknowledgedChunks[index] = true;

        // Update ping timer so we know when to ack
        mRecv.lastRecvTime = Platform::getRealMilliseconds();
        return true;
    }

    /// [Sender] Write data packets for the next `count` unacknowledged chunks to receiver
    void writeNextDataPackets(U32 count)
    {
        U32 index = 0;
        for (U32 i = 0; i < count; i ++)
        {
            // Get index of next chunk
            for (; index < mSend.fileChunks.size(); index ++)
            {
                if (!mSend.acknowledgedChunks[index])
                {
                    break;
                }
            }

            // EOF is handled by the receiver
            if (index == mSend.fileChunks.size())
            {
                return;
            }

            // Send next unacknowledged chunk to receiver
            BitStream *out = buildFastFilePacket();
            writeDataPacket(out, index);
            sendFastFilePacket(mConnection->getNetAddress());

            index++;
        }
    }

    /// [Receiver] Determine if it has been long enough since the last chunk packet and we should send an ack
    /// According to the state machine above, this is when it has been more than 1RTT since the last packet
    bool shouldSendAcknowledgement()
    {
        // If it has been Ping ms since last chunk packet, send ack
        if (mRecv.transferID != 0)
        {
            // This is entirely a heuristic but it works well in practice
            U32 now = Platform::getRealMilliseconds();
            if (now - mRecv.lastRecvTime > mConnection->getRoundTripTime())
            {
                return true;
            }
        }
        return false;
    }

    /// [Receiver] Determine if the transfer is finished
    bool isFinished()
    {
        if (mRecv.transferID != 0)
        {
            // Any unack means no
            for (U32 index = 0; index < mRecv.acknowledgedChunks.size(); index++)
            {
                if (!mRecv.acknowledgedChunks[index])
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// [Receiver] Write the packet telling the sender which chunks we have received
    void writeAcknowledgeEvent(BitStream* out)
    {
        // Bump the recv time so we don't send duplicate ack events
        mRecv.lastRecvTime = Platform::getRealMilliseconds();

        // Header
        out->write(mRecv.transferID);

        // Index of first non-acknowledged chunk, so the server can ez ignore everything before this
        // Mostly just an optimization
        U32 minNonAcknowledged = 0;
        for (; minNonAcknowledged < mRecv.acknowledgedChunks.size(); minNonAcknowledged++)
        {
            if (!mRecv.acknowledgedChunks[minNonAcknowledged])
            {
                break;
            }
        }
        out->writeInt(minNonAcknowledged, 32);

        // Find index of last acknowledged chunk so we know how many bits of ack we will need to cover
        // everything from the first non-ack to the last ack
        U32 lastAcknowledged = 0;
        for (U32 index = 0; index < mRecv.acknowledgedChunks.size(); index++)
        {
            if (mRecv.acknowledgedChunks[index])
            {
                lastAcknowledged = index;
                break;
            }
        }

        // Build debug string for the Con::warnf
        char debugStr[512+1] = {0};

        // Number of chunks between first nonack and last ack. Since we tell the server the lower bound
        // we only need to send ack bits for everything between these two.
        U32 ackCount = minNonAcknowledged - lastAcknowledged;

        // Upper bound for sanity and packet size limits
        // If we exceed this we will get retransmissions for the later chunks we have acked,
        // but that's probably unlike and also not that bad.
        if (ackCount > 512)
        {
            ackCount = 512;
        }
        out->writeInt(ackCount, 16);

        // For every chunk in the [first nonack, last ack] range, tell the server if we have it
        for (U32 i = 0; i < ackCount; i ++)
        {
            U32 index = minNonAcknowledged + i;
            if (index >= mRecv.acknowledgedChunks.size())
            {
                // This probably never happens, but just in case it does, we definitely don't have it
                out->writeFlag(false);
                debugStr[i] = '0';
            }
            else
            {
                out->writeFlag(mRecv.acknowledgedChunks[index]);
                debugStr[i] = mRecv.acknowledgedChunks[index] ? '1' : '0';
            }
        }
        // Null terminate
        debugStr[ackCount] = 0;
#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d %s",
            __func__,
            __LINE__,
            mRecv.transferID,
            minNonAcknowledged,
            ackCount,
            debugStr
        );
#endif
    }

    /// [Sender] Read the packet saying which chunks the receiver has received
    bool readAcknowledgementPacket(BitStream* stream)
    {
        U32 transferID;
        U32 minNonAcknowledged;
        U32 ackCount;
        Vector<bool> acknowledged;

        // Build debug string for the Con::warnf
        char debugStr[512+1] = {0};

        stream->read(&transferID);
        minNonAcknowledged = stream->readInt(32);
        ackCount = stream->readInt(16);

        // Basic sanity checks on packet
        if (transferID == 0 || transferID != mSend.transferID)
        {
            Con::errorf(
                "%s @%d Bad transfer id",
                __func__,
                __LINE__
            );
            return false;
        }
        if (ackCount > 512)
        {
            return false;
        }
        for (U32 i = 0; i < ackCount; i++)
        {
            bool flag = stream->readFlag();
            acknowledged.push_back(flag);
            debugStr[i] = flag ? '1' : '0';
        }
        debugStr[ackCount] = 0;
        // Make sure the stream is actually long enough to have the data
        if (!stream->isValid())
        {
            return false;
        }
#if TORQUE_DEBUG
        Con::warnf(
            "%s @%d %d %d %d %s",
            __func__,
            __LINE__,
            transferID,
            minNonAcknowledged,
            ackCount,
            debugStr
        );
#endif

        // Save this state for handleAcknowledgement because NetEvent does this in 2 steps
        mSend.lastAckStart = minNonAcknowledged;
        mSend.lastAckAcks = acknowledged;

        return true;
    }

    /// [Sender] Handle the ack packet from the receiver and send it the next packets
    bool handleAcknowledgement()
    {
        // For everything up to the receiver's first nonack chunk, we can assume they have received it
        for (U32 index = 0; index < mSend.lastAckStart; index++)
        {
            // Bounds check for sanity
            if (index >= mSend.acknowledgedChunks.size())
            {
#if TORQUE_DEBUG
                Con::errorf(
                    "%s @%d Index >= ackChunks.size()",
                    __func__,
                    __LINE__
                );
#endif
                return false;
            }
            mSend.acknowledgedChunks[index] = true;
        }
        // Then all of the ack bits start at the first nonack index and go from there
        for (U32 i = 0; i < mSend.lastAckAcks.size(); i++)
        {
            U32 index = mSend.lastAckStart + i;
            // Bounds check for sanity, but don't bother erroring on this because they might send
            // a couple past the end (todo: do they actually? this is safe even if they do, so idc)
            if (index < mSend.acknowledgedChunks.size())
            {
                mSend.acknowledgedChunks[index] = mSend.lastAckAcks[i];
            }
        }

        // Send them the next set of packets now since we know they're waiting
        writeNextDataPackets(PacketsAtATime);
        return true;
    }
};

/// Event the receiver sends when they want to download a file
/// It's all just punting over to NetConnection who punts to FastFileState
class FastFileRequestEvent : public NetEvent
{
    bool mFoundFile;
public:
    FastFileRequestEvent(bool foundFile = true)
    {
        mFoundFile = foundFile;
    }

    virtual void pack(NetConnection* connection, BitStream *bstream)
    {
        // Sender: Write packet
        if (bstream->writeFlag(mFoundFile))
            connection->sendFastFileRequest(bstream);
    }

    virtual void write(NetConnection* connection, BitStream *bstream)
    {
        // Sender: Write packet (but for demos)
        if (bstream->writeFlag(mFoundFile))
            connection->sendFastFileRequest(bstream);
    }

    virtual void unpack(NetConnection* connection, BitStream *bstream)
    {
        // Receiver: Read packet
        mFoundFile = bstream->readFlag();
        if (mFoundFile)
            connection->handleFastFileRequest(bstream);
        else
            connection->popMissingFile();
    }

    virtual void process(NetConnection* connection)
    {
        // Receiver: Handle packet
    }

    virtual void notifyDelivered(NetConnection* connection, bool madeit)
    {
        // Sender: It's done
        connection->processFastFileRequest();
    }

    DECLARE_CONOBJECT(FastFileRequestEvent);
};

// Client only for now (todo: clients sending files to the server?? that is too spicy an idea for now)
IMPLEMENT_CO_CLIENTEVENT_V1(FastFileRequestEvent);

/// Event the receiver sends when it has been a bit since they last received a chunk
/// It's all just punting over to NetConnection who punts to FastFileState
class FastFileAcknowledgeEvent : public NetEvent
{
public:
    FastFileAcknowledgeEvent()
    {
    }

    virtual void pack(NetConnection* connection, BitStream *bstream)
    {
        // Receiver: Write ack packet
        connection->sendFastFileAcknowledgement(bstream);
    }

    virtual void write(NetConnection* connection, BitStream *bstream)
    {
        // Receiver: Write ack packet (but for demos)
        connection->sendFastFileAcknowledgement(bstream);
    }

    virtual void unpack(NetConnection* connection, BitStream *bstream)
    {
        // Sender: Read ack packet
        connection->handleFastFileAcknowledgement(bstream);
    }

    virtual void process(NetConnection *connection)
    {
        // Sender: Handle ack packet
        connection->processFastFileAcknowledgement();
    }

    DECLARE_CONOBJECT(FastFileAcknowledgeEvent);
};

// Server only for now (todo: clients sending files to the server?? that is too spicy an idea for now)
IMPLEMENT_CO_SERVEREVENT_V1(FastFileAcknowledgeEvent);

/// Init console variables for tuning the speed of transfer
void NetConnection::fastFileTransferInit()
{
    /// Size of chunks. Should be <= MaxFilePacketSize (1400)
    /// Testing in practice has found that values > ~1380 cause packet issues in some cases
    /// (could not send packets on unruly routers with anything over than 1392)
    /// So 1380 is chosen as it should be "safe enough" for most UDP/IP stacks to accept it
    Con::addVariable("$pref::Net::FastFileChunkSize", TypeS32, &FastFilePacketSize);
    /// Number of packets to send in each burst of data
    /// Really not sure what the upper bound on this is, as increasing it will just increase loss
    /// but you were going to get loss anyway.
    Con::addVariable("$pref::Net::FastFilePacketsAtATime", TypeS32, &PacketsAtATime);
}

/// Init connection state (NetConnection::NetConnection())
void NetConnection::initFastFile()
{
    mFastFileState = new FastFileState(this);
}

/// Destroy connection state (NetConnection::~NetConnection())
void NetConnection::destroyFastFile()
{
    delete mFastFileState;
}

/// Actually send a file, specifically the currently downloading file as requested
/// using a FileDownloadRequestEvent from above
void NetConnection::sendFastFile()
{
    // Init sender state
    mFastFileState->mSend.transferID ++;
    mFastFileState->mSend.totalSize = mCurrentFileBufferSize;
    mFastFileState->mSend.fileChunks.clear();
    mFastFileState->mSend.acknowledgedChunks.clear();

    if (FastFilePacketSize > 1280)
        FastFilePacketSize = 1280;

    // Load all chunks of file to send
    U32 index = 0;
    for (U32 offset = 0; offset < mCurrentFileBufferSize; offset += FastFilePacketSize)
    {
        U32 length = FastFilePacketSize;
        // Last packet may not be full size
        if (offset + length > mCurrentFileBufferSize)
        {
            length = mCurrentFileBufferSize - offset;
        }
        Vector<U8> bytes(length);
        bytes.setSize(length);
        mCurrentDownloadingFile->read(length, bytes.address());

        // Queue up the packets in our state so we don't have try seeking when reading the file
        FileChunk chunk;
        chunk.index = index;
        chunk.offset = offset;
        chunk.bytes = bytes;

        mFastFileState->mSend.fileChunks.push_back(chunk);
        mFastFileState->mSend.acknowledgedChunks.push_back(false);

        index ++;
    }

    // And tell the receiver we're going to be sending it
    // We will send them chunks once they ack this
    postNetEvent(new FastFileRequestEvent());
}

/// Every frame when receiving a file, check to see if we're done or if we need to ack the packets
void NetConnection::checkFastFile()
{
    // Process all the chunks we have
    for (U32 index = mFastFileState->mRecv.nextNonRecvChunk;
        index < mFastFileState->mRecv.fileChunks.size();
        index++)
    {
        // When we reach one we don't, stop so the stream output is contiguous
        if (!mFastFileState->mRecv.acknowledgedChunks[index])
        {
            mFastFileState->mRecv.nextNonRecvChunk = index;
            break;
        }
        // Conventional torque file download output function
        chunkReceived(
            mFastFileState->mRecv.fileChunks[index].bytes.address(),
            mFastFileState->mRecv.fileChunks[index].bytes.size()
        );
    }
    // If we're done, chunkReceived will have finished the transfer. So just exit
    if (mFastFileState->isFinished())
    {
        return;
    }

    // If it has been Ping ms since last packet, send ack
    if (mFastFileState->shouldSendAcknowledgement())
    {
        // Send ack event
        postNetEvent(new FastFileAcknowledgeEvent());
    }
}

/// [Sender/Receiver] Handle the packets used by the fast file send protocol
void NetConnection::handleFastFilePacket(BitStream *stream)
{
    // Simulate packet loss for testing
    if (mSimulatedPacketLoss && Platform::getRandom() < mSimulatedPacketLoss)
    {
        //Con::printf("NET  %d: SENDDROP - %d", getId(), mLastSendSeq);
        return;
    }

    // Packet header
    U16 opcode;
    stream->read(&opcode);

    switch (opcode)
    {
    case Data:
    {
        // Punt to state manager
        if (!mFastFileState->readDataPacket(stream))
        {
            connectionError("Bad fast file packet.");
        }
        break;
    }
    // There used to be more opcodes here, I promise
    default:
        // Hmm
        connectionError("Some connection error in fast file transfer");
        break;
    }
}

/// [Sender] Write the packet for starting a file transfer
void NetConnection::sendFastFileRequest(BitStream *stream)
{
    mFastFileState->writeWriteRequestPacket(stream);
}

/// [Receiver] Read the packet for starting a file transfer
void NetConnection::handleFastFileRequest(BitStream *stream)
{
    // Init fast file receiving state
    if (!mFastFileState->readWriteRequestPacket(stream))
    {
        setLastError("Bad fast file packet.");
        return;
    }

    // Init conventional packet receiving state
    mCurrentFileBufferSize = mFastFileState->mRecv.totalSize;
    mCurrentFileBuffer = dRealloc(mCurrentFileBuffer, mCurrentFileBufferSize);
    mCurrentFileBufferOffset = 0;
    Con::executef(4, "onFileChunkReceived", mMissingFileList[0], Con::getIntArg(0), Con::getIntArg(mCurrentFileBufferSize));
}

/// [Sender] After sending packet for starting a file transfer, start sending data
void NetConnection::processFastFileRequest()
{
    mFastFileState->writeNextDataPackets(PacketsAtATime);
}

/// [Receiver] Write the packet to acknowledge sent chunks
void NetConnection::sendFastFileAcknowledgement(BitStream *stream)
{
    mFastFileState->writeAcknowledgeEvent(stream);
}

/// [Sender] Read the packet acknowledging sent chunks
void NetConnection::handleFastFileAcknowledgement(BitStream *stream)
{
    if (!mFastFileState->readAcknowledgementPacket(stream))
    {
        setLastError("Bad fast file packet.");
        return;
    }
}

/// [Sender] After reading the packet acknowledging sent chunks, update state
void NetConnection::processFastFileAcknowledgement()
{
    if (!mFastFileState->handleAcknowledgement())
    {
        setLastError("Bad fast file packet.");
        return;
    }

    // Calculate size and progress for reporting to script
    U32 totalSent = 0;
    for (U32 index = 0; index < mFastFileState->mSend.acknowledgedChunks.size(); index++)
    {
        // Lazy: Don't bother checking unacked chunks after the first one
        // We haven't actually written that to disk yet either
        if (!mFastFileState->mSend.acknowledgedChunks[index])
        {
            break;
        }
        totalSent += mFastFileState->mSend.fileChunks[index].bytes.size();
    }
    Con::executef(this, 4, "onFileChunkSent", mCurrentFileName, Con::getIntArg(totalSent), Con::getIntArg(mCurrentFileBufferSize));
}

#endif // TORQUE_FAST_FILE_TRANSFER

void NetConnection::chunkReceived(U8* chunkData, U32 chunkLen)
{
    if (chunkLen == 0)
    {
        // the server didn't have the file... apparently it's one we don't need...
        dFree(mCurrentFileBuffer);
        mCurrentFileBuffer = NULL;
        dFree(mMissingFileList[0]);
        mMissingFileList.pop_front();
        return;
    }
    if (chunkLen + mCurrentFileBufferOffset > mCurrentFileBufferSize)
    {
        setLastError("Invalid file chunk from server.");
        return;
    }
    dMemcpy(((U8*)mCurrentFileBuffer) + mCurrentFileBufferOffset, chunkData, chunkLen);
    mCurrentFileBufferOffset += chunkLen;
    if (mCurrentFileBufferOffset == mCurrentFileBufferSize)
    {
        // this file's done...
        // save it to disk:
        Stream* stream;

        Con::printf("Saving file %s.", mMissingFileList[0]);
        if (!ResourceManager->openFileForWrite(stream, mMissingFileList[0], 1, true))
        {
            setLastError("Couldn't open file downloaded by server.");
            return;
        }
        stream->write(mCurrentFileBufferSize, mCurrentFileBuffer);
        ResourceManager->closeStream(stream);
        Con::executef(2, "onFileDownloaded", mMissingFileList[0]);
        dFree(mMissingFileList[0]);
        mMissingFileList.pop_front();
        mNumDownloadedFiles++;
        dFree(mCurrentFileBuffer);
        mCurrentFileBuffer = NULL;
        sendNextFileDownloadRequest();
    }
    else
    {
        Con::executef(4, "onFileChunkReceived", mMissingFileList[0], Con::getIntArg(mCurrentFileBufferOffset), Con::getIntArg(mCurrentFileBufferSize));
    }
}

void NetConnection::addMissingFile(const char* path)
{
    int slen = dStrlen(path);
    char* buf = new char[slen + 1];
    dStrcpy(buf, path);
    buf[slen] = '\0';
    mMissingFileList.push_back(buf);
}

void NetConnection::popMissingFile()
{
    if (mMissingFileList.size() > 0)
    {
        dFree(mMissingFileList[0]);
        mMissingFileList.pop_front();
    }
}

void failedToFindFile(NetConnection* conn)
{
    conn->postNetEvent(new FastFileRequestEvent(false));
}

ConsoleMethod(NetConnection, requestFileDownload, bool, 3, 3, "(path)")
{
    Vector<char*> filePath;
    filePath.push_back(const_cast<char*>(argv[2]));
    object->addMissingFile(argv[2]);
    return object->postNetEvent(new FileDownloadRequestEvent(&filePath));
}