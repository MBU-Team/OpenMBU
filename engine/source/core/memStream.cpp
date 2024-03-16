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
    : m_bufferSize(in_bufferSize),
    m_pBufferBase(io_pBuffer),
    m_instCaps(0),
    m_currentPosition(0)
{
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

    return m_bufferSize;
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
    AssertFatal(in_newPosition <= m_bufferSize, "Invalid position");

    m_currentPosition = in_newPosition;
    if (m_currentPosition > m_bufferSize) {
        // Never gets here in debug version, this is for the release builds...
        //
        setStatus(UnknownError);
        return false;
    }
    else if (m_currentPosition == m_bufferSize) {
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
    if ((m_currentPosition + in_numBytes) > m_bufferSize) {
        success = false;
        actualBytes = m_bufferSize - m_currentPosition;
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
    if ((m_currentPosition + in_numBytes) > m_bufferSize) {
        success = false;
        actualBytes = m_bufferSize - m_currentPosition;
    }

    // Obtain a current pointer, and do the copy
    void* pCurrent = (void*)((U8*)m_pBufferBase + m_currentPosition);
    dMemcpy(pCurrent, in_pBuffer, actualBytes);

    // Advance the stream position
    m_currentPosition += actualBytes;

    if (m_currentPosition == m_bufferSize)
        setStatus(EOS);
    else
        setStatus(Ok);

    return success;
}


ResizableMemStream::ResizableMemStream(
    const U32 in_bufferSize,
    void* io_pBuffer,
    const bool in_allowRead,
    const bool in_allowWrite
)
{
    inner = new MemoryStream();
    if (io_pBuffer != NULL)
    {
        inner->useBuffer(static_cast<U8*>(io_pBuffer), in_bufferSize);
    }
}

ResizableMemStream::~ResizableMemStream()
{
    delete inner;
}

U32 ResizableMemStream::getStreamSize()
{
    return inner->length();
}

bool ResizableMemStream::setPosition(const U32 in_newPosition)
{
    return inner->seek(in_newPosition);
}

bool ResizableMemStream::_write(const U32 in_numBytes, const void* in_pBuffer)
{
    inner->writeBuffer(static_cast<const char*>(in_pBuffer), in_numBytes);
    return true;
}

bool ResizableMemStream::_read(const U32 in_numBytes, void* out_pBuffer)
{
    U32 bRead;

    if (inner->tell() + in_numBytes > inner->length())
    {
        bRead = (inner->length() - inner->tell());
        setStatus(EOS);
    }
    else
    {
        bRead = in_numBytes;
        setStatus(Ok);
    }
    U8* buffer = inner->getBuffer();
    dMemcpy(out_pBuffer, (buffer + inner->tell()), bRead);
    try
    {
        inner->seek(inner->tell() + bRead);
    }
    catch (...)
    {
        setStatus(EOS);
    }
    return getStatus() != EOS;
}

bool ResizableMemStream::hasCapability(const Capability) const
{
    return true;
}

U32 ResizableMemStream::getPosition() const
{
    return inner->tell();
}


MemSubStream::MemSubStream()
    : m_pStream(NULL),
    m_currOffset(0)
{

}

MemSubStream::~MemSubStream()
{
    detachStream();
}


bool MemSubStream::attachStream(Stream* io_pSlaveStream)
{
    AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");
    AssertFatal(m_pStream == NULL, "Already attached!");

    m_pStream = io_pSlaveStream;
    m_currOffset = 0;

    setStatus(Ok);
    return true;
}

void MemSubStream::detachStream()
{
    m_pStream = NULL;
    m_currOffset = 0;
    setStatus(Closed);
}

Stream* MemSubStream::getStream()
{
    return m_pStream;
}

bool MemSubStream::_read(const U32 in_numBytes, void* out_pBuffer)
{
    if (in_numBytes == 0)
        return true;

    AssertFatal(out_pBuffer != NULL, "NULL output buffer");
    if (getStatus() == Closed) {
        AssertFatal(false, "Attempted read from closed stream");
        return false;
    }

    if (Ok != getStatus())
        return false;

    U32 oldPos = m_pStream->getPosition();
    
    m_pStream->setPosition(m_currOffset);
    bool result = m_pStream->read(in_numBytes, out_pBuffer);
    m_pStream->setPosition(oldPos);

    if (result)
        m_currOffset = m_currOffset + in_numBytes;
    else
        setStatus(EOS);

    return result;
}

bool MemSubStream::_write(const U32 in_numBytes, const void* in_pBuffer)
{
    if (in_numBytes == 0)
        return true;

    AssertFatal(in_pBuffer != NULL, "NULL output buffer");
    if (getStatus() == Closed) {
        AssertFatal(false, "Attempted read from closed stream");
        return false;
    }

    if (Ok != getStatus())
        return false;

    U32 oldPos = m_pStream->getPosition();
    
    m_pStream->setPosition(m_currOffset);
    bool result = m_pStream->write(in_numBytes, in_pBuffer);
    m_pStream->setPosition(oldPos);

    if (result)
        m_currOffset = m_currOffset + in_numBytes;
    else
        setStatus(EOS);

    return result;
}

bool MemSubStream::hasCapability(const Capability cap) const
{
    if (!m_pStream)
        return false;
    return m_pStream->hasCapability(cap);
}

U32 MemSubStream::getPosition() const
{
    return m_currOffset;
}

bool MemSubStream::setPosition(const U32 in_newPosition)
{
    if (!m_pStream)
        return false;

    U32 oldPos = m_pStream->getPosition();
    bool result = m_pStream->setPosition(in_newPosition);
    m_pStream->setPosition(oldPos);

    if (result) {
        m_currOffset = in_newPosition;
        setStatus(Ok);
    }
    
    return result;
}

U32 MemSubStream::getStreamSize()
{
    if (!m_pStream)
        return 0;
    return m_pStream->getStreamSize();
}
