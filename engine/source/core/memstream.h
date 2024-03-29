//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MEMSTREAM_H_
#define _MEMSTREAM_H_

//Includes
#ifndef _STREAM_H_
#include "core/stream.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif

#include "core/MemoryStream.h"

class MemStream : public Stream {
    typedef Stream Parent;

protected:
    U32 m_bufferSize;
    void* m_pBufferBase;
    bool mOwnsMemory;

    U32 m_instCaps;
    U32 m_currentPosition;

public:
    MemStream(const U32  in_bufferSize,
        void* io_pBuffer = NULL,
        const bool in_allowRead = true,
        const bool in_allowWrite = true);
    virtual ~MemStream();

    // Mandatory overrides from Stream
protected:
    bool _read(const U32 in_numBytes, void* out_pBuffer);
    bool _write(const U32 in_numBytes, const void* in_pBuffer);
public:
    bool hasCapability(const Capability) const;
    U32  getPosition() const;
    bool setPosition(const U32 in_newPosition);
    void* getBuffer() { return m_pBufferBase; }
    // Mandatory overrides from Stream
public:
    U32  getStreamSize();
    inline bool checkStatus(U32 bytes)
    {
        if (m_currentPosition + bytes > m_bufferSize)
        {
            m_currentPosition = m_bufferSize;
            setStatus(EOS);
            return false;
        }
        setStatus(Ok);
        return true;
    }

    inline bool read1(U8* dest)
    {
        if (!checkStatus(1))
            return false;

        *dest = ((U8*)m_pBufferBase)[m_currentPosition++];
        return true;
    }

    inline bool read2(U16* dest)
    {
        if (!checkStatus(2))
            return false;

        U8* bufPtr = ((U8*)m_pBufferBase) + m_currentPosition;

        *dest = bufPtr[0] | (U32(bufPtr[1]) << 8);
        m_currentPosition += 2;
        return true;
    }
    inline bool read4(U32* dest)
    {
        if (!checkStatus(4))
            return false;

        U8* bufPtr = ((U8*)m_pBufferBase) + m_currentPosition;

        *dest = bufPtr[0] | (U32(bufPtr[1]) << 8) | (U32(bufPtr[2]) << 16) | (U32(bufPtr[3]) << 24);
        m_currentPosition += 4;
        return true;
    }
    inline bool read8(U64* dest)
    {
        if (!checkStatus(8))
            return false;
        U8* bufPtr = ((U8*)m_pBufferBase) + m_currentPosition;

        U32 loWord = bufPtr[0] | (U32(bufPtr[1]) << 8) | (U32(bufPtr[2]) << 16) | (U32(bufPtr[3]) << 24);
        U32 hiWord = bufPtr[4] | (U32(bufPtr[5]) << 8) | (U32(bufPtr[6]) << 16) | (U32(bufPtr[7]) << 24);
        *dest = loWord | (((U64)hiWord) << 32);
        m_currentPosition += 8;
        return true;
    }
    inline bool read(const U32 in_numBytes, void* out_pBuffer)
    {
        if (!checkStatus(in_numBytes))
            return false;
        U8* bufPtr = ((U8*)m_pBufferBase) + m_currentPosition;
        dMemcpy(out_pBuffer, bufPtr, in_numBytes);
        m_currentPosition += in_numBytes;
        return true;
    }
    inline bool read(bool* dest)
    {
        U8 temp;
        bool ret = read1(&temp);
        *dest = temp != 0;
        return ret;
    }
    inline bool read(S8* dest)
    {
        return read1((U8*)dest);
    }
    inline bool read(U8* dest)
    {
        return read1(dest);
    }
    inline bool read(S16* dest)
    {
        return read2((U16*)dest);
    }
    inline bool read(U16* dest)
    {
        return read2(dest);
    }
    inline bool read(S32* dest)
    {
        return read4((U32*)dest);
    }
    inline bool read(U32* dest)
    {
        return read4(dest);
    }
    inline bool read(F32* dest)
    {
        return read4((U32*)dest);
    }
    inline bool read(F64* dest)
    {
        return read8((U64*)dest);
    }
    /// Read an integral color from the stream.
    bool read(ColorI* dest)
    {
        read(&dest->red);
        read(&dest->green);
        read(&dest->blue);
        return read(&dest->alpha);
    }
    /// Read a floating point color from the stream.
    bool read(ColorF* dest)
    {
        ColorI temp;
        bool ret = read(&temp);
        *dest = temp;
        return ret;
    }
};

class ResizableMemStream : public Stream {
    typedef Stream Parent;

    MemoryStream* inner;

public:
    ResizableMemStream(const U32 in_bufferSize = 0,
        void* io_pBuffer = NULL,
        const bool in_allowRead = true,
        const bool in_allowWrite = true);
    virtual ~ResizableMemStream();

    virtual U32 getStreamSize();
    virtual bool setPosition(const U32 in_newPosition);
    virtual bool _write(const U32 in_numBytes, const void* in_pBuffer);
    virtual bool _read(const U32 in_numBytes, void* out_pBuffer);

    bool hasCapability(const Capability) const;
    U32  getPosition() const;
};

class MemSubStream : public Stream {
    typedef Stream Parent;

    Stream* m_pStream;
    U32 m_currOffset;

public:
    MemSubStream();
    ~MemSubStream();

    bool attachStream(Stream* io_pSlaveStream);
    void detachStream();
    Stream* getStream();

protected:
    bool _read(const U32 in_numBytes, void* out_pBuffer);
    bool _write(const U32 in_numBytes, const void* in_pBuffer);

public:
    bool hasCapability(const Capability) const;

    U32 getPosition() const;
    bool setPosition(const U32 in_newPosition);

    U32 getStreamSize();
};

#endif //_MEMSTREAM_H_
