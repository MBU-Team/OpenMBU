//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/memstream.h"

MemStream::MemStream(const U32 in_bufferSize,
    void* io_pBuffer,
    const bool   in_allowRead,
    const bool   in_allowWrite)
    : cm_bufferSize(in_bufferSize),
    m_pBufferBase(io_pBuffer),
    m_instCaps(0),
    m_currentPosition(0)
{
    AssertFatal(io_pBuffer != NULL, "Invalid buffer pointer");
    AssertFatal(in_bufferSize > 0, "Invalid buffer size");
    AssertFatal(in_allowRead || in_allowWrite, "Either write or read must be allowed");

    if (in_allowRead)
        m_instCaps |= Stream::StreamRead;
    if (in_allowWrite)
        m_instCaps |= Stream::StreamWrite;
    mOwnsMemory = false;
    if (!io_pBuffer)
    {
        mOwnsMemory = true;
        m_pBufferBase = dMalloc(in_bufferSize);
    }
    setStatus(Ok);
}

MemStream::~MemStream()
{
    if (mOwnsMemory)
        dFree(m_pBufferBase);
    m_pBufferBase = NULL;
    m_currentPosition = 0;

    setStatus(Closed);
}

U32 MemStream::getStreamSize()
{
    AssertFatal(getStatus() != Closed, "Stream not open, size undefined");

    return cm_bufferSize;
}

bool MemStream::hasCapability(const Capability in_cap) const
{
    // Closed streams can't do anything
    //
    if (getStatus() == Closed)
        return false;

    U32 totalCaps = U32(Stream::StreamPosition) | m_instCaps;

    return (U32(in_cap) & totalCaps) != 0;
}

U32 MemStream::getPosition() const
{
    AssertFatal(getStatus() != Closed, "Position of a closed stream is undefined");

    return m_currentPosition;
}

bool MemStream::setPosition(const U32 in_newPosition)
{
    AssertFatal(getStatus() != Closed, "SetPosition of a closed stream is not allowed");
    AssertFatal(in_newPosition <= cm_bufferSize, "Invalid position");

    m_currentPosition = in_newPosition;
    if (m_currentPosition > cm_bufferSize) {
        // Never gets here in debug version, this is for the release builds...
        //
        setStatus(UnknownError);
        return false;
    }
    else if (m_currentPosition == cm_bufferSize) {
        setStatus(EOS);
    }
    else {
        setStatus(Ok);
    }

    return true;
}

bool MemStream::_read(const U32 in_numBytes, void* out_pBuffer)
{
    AssertFatal(getStatus() != Closed, "Attempted read from a closed stream");

    if (in_numBytes == 0)
        return true;

    AssertFatal(out_pBuffer != NULL, "Invalid output buffer");

    if (hasCapability(StreamRead) == false) {
        AssertWarn(false, "Reading is disallowed on this stream");
        setStatus(IllegalCall);
        return false;
    }

    bool success = true;
    U32  actualBytes = in_numBytes;
    if ((m_currentPosition + in_numBytes) > cm_bufferSize) {
        success = false;
        actualBytes = cm_bufferSize - m_currentPosition;
    }

    // Obtain a current pointer, and do the copy
    const void* pCurrent = (const void*)((const U8*)m_pBufferBase + m_currentPosition);
    dMemcpy(out_pBuffer, pCurrent, actualBytes);

    // Advance the stream position
    m_currentPosition += actualBytes;

    if (!success)
        setStatus(EOS);
    else
        setStatus(Ok);

    return success;
}

bool MemStream::_write(const U32 in_numBytes, const void* in_pBuffer)
{
    AssertFatal(getStatus() != Closed, "Attempted write to a closed stream");

    if (in_numBytes == 0)
        return true;

    AssertFatal(in_pBuffer != NULL, "Invalid input buffer");

    if (hasCapability(StreamWrite) == false) {
        AssertWarn(0, "Writing is disallowed on this stream");
        setStatus(IllegalCall);
        return false;
    }

    bool success = true;
    U32  actualBytes = in_numBytes;
    if ((m_currentPosition + in_numBytes) > cm_bufferSize) {
        success = false;
        actualBytes = cm_bufferSize - m_currentPosition;
    }

    // Obtain a current pointer, and do the copy
    void* pCurrent = (void*)((U8*)m_pBufferBase + m_currentPosition);
    dMemcpy(pCurrent, in_pBuffer, actualBytes);

    // Advance the stream position
    m_currentPosition += actualBytes;

    if (m_currentPosition == cm_bufferSize)
        setStatus(EOS);
    else
        setStatus(Ok);

    return success;
}

