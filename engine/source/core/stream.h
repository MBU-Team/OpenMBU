//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _STREAM_H_
#define _STREAM_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif


// This should ideally be done with templates...
//
/// @defgroup stream_overload Primitive Type Stream Operation Overloads
/// These macros define the read and write functions for all primitive types.
/// @{

#define DECLARE_OVERLOADED_READ(type)      \
   bool read(type* out_read) {             \
      return read(sizeof(type), out_read); \
   }
#define DECLARE_OVERLOADED_WRITE(type)       \
   bool write(type in_write) {               \
      return write(sizeof(type), &in_write); \
   }

#define DECLARE_ENDIAN_OVERLOADED_READ(type)       \
   bool read(type* out_read) {                     \
      type temp;                                   \
      bool success = read(sizeof(type), &temp);    \
      *out_read = convertLEndianToHost(temp);      \
      return success;                              \
   }
#define DECLARE_ENDIAN_OVERLOADED_WRITE(type)      \
   bool write(type in_write) {                     \
      type temp = convertHostToLEndian(in_write);  \
      return write(sizeof(type), &temp);           \
   }
/// @}

class ColorI;
class ColorF;

//------------------------------------------------------------------------------
//-------------------------------------- Base Stream class
//
/// Base stream class for streaming data across a specific media
class Stream {
    // Public structs and enumerations...
public:
    /// Status constantants for the stream
    enum Status {
        Ok = 0,           ///< Ok!
        IOError,          ///< Read or Write error
        EOS,              ///< End of Stream reached (mostly for reads)
        IllegalCall,      ///< An unsupported operation used.  Always w/ accompanied by AssertWarn
        Closed,           ///< Tried to operate on a closed stream (or detached filter)
        UnknownError      ///< Catchall
    };

    enum Capability {
        StreamWrite = BIT(0), ///< Can this stream write?
        StreamRead = BIT(1), ///< Can this stream read?
        StreamPosition = BIT(2)  ///< Can this stream position?
    };

    // Accessible only through inline accessors
private:
    Status m_streamStatus;

    // Derived accessible data modifiers...
protected:
    void setStatus(const Status in_newStatus) { m_streamStatus = in_newStatus; }

public:
    Stream();
    virtual ~Stream();

    /// Gets the status of the stream
    Stream::Status getStatus() const { return m_streamStatus; }
    /// Gets a printable string form of the status
    static const char* getStatusString(const Status in_status);

    // Derived classes must override these...
protected:
    virtual bool _read(const U32 in_numBytes, void* out_pBuffer) = 0;
    virtual bool _write(const U32 in_numBytes, const void* in_pBuffer) = 0;
public:
    /// Checks to see if this stream has the capability of a given function
    virtual bool hasCapability(const Capability) const = 0;

    /// Gets the position in the stream
    virtual U32  getPosition() const = 0;
    /// Sets the position of the stream.  Returns if the new position is valid or not
    virtual bool setPosition(const U32 in_newPosition) = 0;
    /// Gets the size of the stream
    virtual U32  getStreamSize() = 0;

    /// Reads a line from the stream.
    /// @param buffer buffer to be read into
    /// @param bufferSize max size of the buffer.  Will not read more than the "bufferSize"
    void readLine(U8* buffer, U32 bufferSize);
    /// writes a line to the stream
    void writeLine(U8* buffer);

    /// Reads a string and inserts it into the StringTable
    /// @see StringTable
    const char* readSTString(bool casesens = false);
    /// Reads a string of maximum 255 characters long
    virtual void readString(char stringBuf[256]);
    /// Reads a string that could potentially be more than 255 characters long.
    /// @param maxStringLen Maximum length to read.  If the string is longer than maxStringLen, only maxStringLen bytes will be read.
    /// @param stringBuf buffer where data is read into
    void readLongString(U32 maxStringLen, char* stringBuf);
    /// Writes a string to the stream.  This function is slightly unstable.
    /// Only use this if you have a valid string that is not empty.
    /// writeString is safer.
    void writeLongString(U32 maxStringLen, const char* string);

    /// Writes a string to the stream.
    virtual void writeString(const char* stringBuf, S32 maxLen = 255);

    /// Write an integral color to the stream.
    bool write(const ColorI&);
    /// Write a floating point color to the stream.
    bool write(const ColorF&);
    /// Read an integral color from the stream.
    bool read(ColorI*);
    /// Read a floating point color from the stream.
    bool read(ColorF*);


    // Overloaded write and read ops..
public:
    bool read(const U32 in_numBytes, void* out_pBuffer) {
        return _read(in_numBytes, out_pBuffer);
    }
    bool write(const U32 in_numBytes, const void* in_pBuffer) {
        return _write(in_numBytes, in_pBuffer);
    }
    DECLARE_OVERLOADED_WRITE(S8)
    DECLARE_OVERLOADED_WRITE(U8)

    DECLARE_ENDIAN_OVERLOADED_WRITE(S16)
    DECLARE_ENDIAN_OVERLOADED_WRITE(S32)
    DECLARE_ENDIAN_OVERLOADED_WRITE(U16)
    DECLARE_ENDIAN_OVERLOADED_WRITE(U32)
    DECLARE_ENDIAN_OVERLOADED_WRITE(F32)
    DECLARE_ENDIAN_OVERLOADED_WRITE(F64)

    DECLARE_OVERLOADED_READ(S8)
    DECLARE_OVERLOADED_READ(U8)

    DECLARE_ENDIAN_OVERLOADED_READ(S16)
    DECLARE_ENDIAN_OVERLOADED_READ(S32)
    DECLARE_ENDIAN_OVERLOADED_READ(U16)
    DECLARE_ENDIAN_OVERLOADED_READ(U32)
    DECLARE_ENDIAN_OVERLOADED_READ(F32)
    DECLARE_ENDIAN_OVERLOADED_READ(F64)

    // We have to do the bool's by hand, since they are different sizes
    //  on different compilers...
    //
    bool read(bool* out_pRead) {
        U8 translate;
        bool success = read(&translate);
        if (success == false)
            return false;

        *out_pRead = translate != 0;
        return true;
    }
    bool write(const bool& in_rWrite) {
        U8 translate = in_rWrite ? U8(1) : U8(0);
        return write(translate);
    }
};

#endif //_STREAM_H_
