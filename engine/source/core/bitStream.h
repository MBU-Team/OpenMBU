//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STREAM_H_
#include "core/stream.h"
#endif

#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

#include "core/crc.h"

//-------------------------------------- Some caveats when using this class:
//                                        - Get/setPosition semantics are changed
//                                         to indicate bit position rather than
//                                         byte position.
//

class Point3F;
class MatrixF;
class HuffmanProcessor;

class BitStream : public Stream
{
protected:
    U8* dataPtr;
    S32  bitNum;
    S32  bufSize;
    bool error;
    S32  maxReadBitNum;
    S32  maxWriteBitNum;
    char* stringBuffer;
    Point3F mCompressPoint;

    friend class HuffmanProcessor;
public:
    static BitStream* getPacketStream(U32 writeSize = 0);
    static void sendPacketStream(const NetAddress* addr);

    void setBuffer(void* bufPtr, S32 bufSize, S32 maxSize = 0);
    U8* getBuffer() { return dataPtr; }
    U8* getBytePtr();

    U32 getReadByteSize();

    S32  getCurPos() const;
    void setCurPos(const U32);

    BitStream(void* bufPtr, S32 bufSize, S32 maxWriteSize = -1) { setBuffer(bufPtr, bufSize, maxWriteSize); stringBuffer = NULL; }
    void clear();

    void setStringBuffer(char buffer[256]);
    void writeInt(S32 value, S32 bitCount);
    S32  readInt(S32 bitCount);

    /// Use this method to write out values in a concise but ass backwards way...
    /// Good for values you expect to be frequently zero, often small. Worst case
    /// this will bloat values by nearly 20% (5 extra bits!) Best case you'll get
    /// one bit (if it's zero).
    ///
    /// This is not so much for efficiency's sake, as to make life painful for
    /// people that want to reverse engineer our network or file formats.
    void writeCussedU32(U32 val)
    {
        // Is it zero?
        if (writeFlag(val == 0))
            return;

        if (writeFlag(val <= 0xF)) // 4 bit
            writeRangedU32(val, 0, 0xF);
        else if (writeFlag(val <= 0xFF)) // 8 bit
            writeRangedU32(val, 0, 0xFF);
        else if (writeFlag(val <= 0xFFFF)) // 16 bit
            writeRangedU32(val, 0, 0xFFFF);
        else if (writeFlag(val <= 0xFFFFFF)) // 24 bit
            writeRangedU32(val, 0, 0xFFFFFF);
        else
            writeRangedU32(val, 0, 0xFFFFFFFF);
    }

    U32 readCussedU32()
    {
        if (readFlag())
            return 0;

        if (readFlag())
            return readRangedU32(0, 0xF);
        else if (readFlag())
            return readRangedU32(0, 0xFF);
        else if (readFlag())
            return readRangedU32(0, 0xFFFF);
        else if (readFlag())
            return readRangedU32(0, 0xFFFFFF);
        else
            return readRangedU32(0, 0xFFFFFFFF);
    }

    void writeSignedInt(S32 value, S32 bitCount);
    S32  readSignedInt(S32 bitCount);

    void writeRangedU32(U32 value, U32 rangeStart, U32 rangeEnd);
    U32  readRangedU32(U32 rangeStart, U32 rangeEnd);

    // read and write floats... floats are 0 to 1 inclusive, signed floats are -1 to 1 inclusive

    F32  readFloat(S32 bitCount);
    F32  readSignedFloat(S32 bitCount);

    void writeFloat(F32 f, S32 bitCount);
    void writeSignedFloat(F32 f, S32 bitCount);

    void writeClassId(U32 classId, U32 classType, U32 classGroup);
    S32 readClassId(U32 classType, U32 classGroup); // returns -1 if the class type is out of range

    // writes a normalized vector
    void writeNormalVector(const Point3F& vec, S32 bitCount);
    void readNormalVector(Point3F* vec, S32 bitCount);

    void clearCompressionPoint();
    void setCompressionPoint(const Point3F& p);

    // Matching calls to these compression methods must, of course,
    // have matching scale values.
    void writeCompressedPoint(const Point3F& p, F32 scale = 0.01f);
    void readCompressedPoint(Point3F* p, F32 scale = 0.01f);

#ifdef MB_ULTRA
    U32 writeCompressedPointRP(const Point3F& p, U32 numDists, const F32* dists, F32 err);
    U32 readCompressedPointRP(Point3F* p, U32 numDists, const F32* dists, F32 err);
#endif

    // Uses the above method to reduce the precision of a normal vector so the server can
    //  determine exactly what is on the client.  (Pre-dumbing the vector before sending
    //  to the client can result in precision errors...)
    static Point3F dumbDownNormal(const Point3F& vec, S32 bitCount);


    // writes a normalized vector using alternate method
    void writeNormalVector(const Point3F& vec, S32 angleBitCount, S32 zBitCount);
    void readNormalVector(Point3F* vec, S32 angleBitCount, S32 zBitCount);

    void readVector(Point3F* vec, F32 minMag, F32 maxMag, S32 magBits, S32 angleBits, S32 zBits);
    void writeVector(Point3F vec, F32 minMag, F32 maxMag, S32 magBits, S32 angleBits, S32 zBits);
    // writes an affine transform (full precision version)
    void writeAffineTransform(const MatrixF&);
    void readAffineTransform(MatrixF*);

    virtual void writeBits(S32 bitCount, const void* bitPtr);
    virtual void readBits(S32 bitCount, void* bitPtr);
    virtual bool writeFlag(bool val);
    virtual bool readFlag();

    void setBit(S32 bitCount, bool set);
    bool testBit(S32 bitCount);

    bool isFull() { return bitNum > (bufSize << 3); }
    bool isValid() { return !error; }

    bool _read(const U32 size, void* d);
    bool _write(const U32 size, const void* d);

    void readString(char stringBuf[256]);
    void writeString(const char* stringBuf, S32 maxLen = 255);

    bool hasCapability(const Capability) const { return true; }
    U32  getPosition() const;
    bool setPosition(const U32 in_newPosition);
    U32  getStreamSize();
};

class ResizeBitStream : public BitStream
{
protected:
    U32 mMinSpace;
public:
    ResizeBitStream(U32 minSpace = 1500, U32 initialSize = 0);
    void validate();
    ~ResizeBitStream();
};

/// This class acts to provide an "infinitely extending" stream.
///
/// Basically, it does what ResizeBitStream does, but it validates
/// on every write op, so that you never have to worry about overwriting
/// the buffer.
class InfiniteBitStream : public ResizeBitStream
{
public:
    InfiniteBitStream();
    ~InfiniteBitStream();

    /// Ensure we have space for at least upcomingBytes more bytes in the stream.
    void validate(U32 upcomingBytes);

    /// Reset the stream to zero length (but don't clean memory).
    void reset();

    /// Shrink the buffer down to match the actual size of the data.
    void compact();

    /// Write us out to a stream... Results in last byte getting padded!
    void writeToStream(Stream& s);

    virtual void writeBits(S32 bitCount, const void* bitPtr)
    {
        validate((bitCount >> 3) + 1); // Add a little safety.
        BitStream::writeBits(bitCount, bitPtr);
    }

    virtual bool writeFlag(bool val)
    {
        validate(1); // One bit will at most grow our buffer by a byte.
        return BitStream::writeFlag(val);
    }

    const U32 getCRC()
    {
        // This could be kinda inefficient - BJG
        return calculateCRC(getBuffer(), getStreamSize());
    }
};

//------------------------------------------------------------------------------
//-------------------------------------- INLINES
//
inline S32 BitStream::getCurPos() const
{
    return bitNum;
}

inline void BitStream::setCurPos(const U32 in_position)
{
    AssertFatal(in_position < (U32)(bufSize << 3), "Out of range bitposition");
    bitNum = S32(in_position);
}

inline bool BitStream::readFlag()
{
    if (bitNum > maxReadBitNum)
    {
        error = true;
        AssertFatal(false, "Out of range read");
        return false;
    }
    S32 mask = 1 << (bitNum & 0x7);
    bool ret = (*(dataPtr + (bitNum >> 3)) & mask) != 0;
    bitNum++;
    return ret;
}

inline void BitStream::writeRangedU32(U32 value, U32 rangeStart, U32 rangeEnd)
{
    AssertFatal(value >= rangeStart && value <= rangeEnd, "BitStream::writeRangedU32: Out of bounds value!");
    AssertFatal(rangeEnd >= rangeStart, "error, end of range less than start");

    U32 rangeSize = rangeEnd - rangeStart + 1;
    U32 rangeBits = getBinLog2(getNextPow2(rangeSize));

    writeInt(S32(value - rangeStart), S32(rangeBits));
}

inline U32 BitStream::readRangedU32(U32 rangeStart, U32 rangeEnd)
{
    AssertFatal(rangeEnd >= rangeStart, "error, end of range less than start");

    U32 rangeSize = rangeEnd - rangeStart + 1;
    U32 rangeBits = getBinLog2(getNextPow2(rangeSize));

    U32 val = U32(readInt(S32(rangeBits)));

    U32 result = val + rangeStart;

    AssertFatal(result >= rangeStart && result <= rangeEnd, "BitStream::readRangedU32: Out of bounds value!");

    return result;
}

#endif //_BITSTREAM_H_
