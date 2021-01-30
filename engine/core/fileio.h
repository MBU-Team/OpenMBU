//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FILEIO_H_
#define _FILEIO_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

class File
{
public:
    /// What is the status of our file handle?
    enum Status
    {
        Ok = 0,           ///< Ok!
        IOError,          ///< Read or Write error
        EOS,              ///< End of Stream reached (mostly for reads)
        IllegalCall,      ///< An unsupported operation used.  Always accompanied by AssertWarn
        Closed,           ///< Tried to operate on a closed stream (or detached filter)
        UnknownError      ///< Catchall
    };

    /// How are we accessing the file?
    enum AccessMode
    {
        Read = 0,  ///< Open for read only, starting at beginning of file.
        Write = 1,  ///< Open for write only, starting at beginning of file; will blast old contents of file.
        ReadWrite = 2,  ///< Open for read-write.
        WriteAppend = 3   ///< Write-only, starting at end of file.
    };

    /// Flags used to indicate what we can do to the file.
    enum Capability
    {
        FileRead = BIT(0),
        FileWrite = BIT(1)
    };

#ifdef TORQUE_OS_XBOX
    enum FileType
    {
        NormalFile = 0,
        XBESection = 1,
    };
#endif

private:
    void* handle;           ///< Pointer to the file handle.
    Status currentStatus;   ///< Current status of the file (Ok, IOError, etc.).
    U32 capability;         ///< Keeps track of file capabilities.

#ifdef TORQUE_OS_XBOX
    FileType type;
    U32 filePosition;
    U32 fileSize;
#endif
    File(const File&);              ///< This is here to disable the copy constructor.
    File& operator=(const File&);   ///< This is here to disable assignment.

public:
    File();                     ///< Default constructor
    virtual ~File();            ///< Destructor

    /// Opens a file for access using the specified AccessMode
    ///
    /// @returns The status of the file
    Status open(const char* filename, const AccessMode openMode);

    /// Gets the current position in the file
    ///
    /// This is in bytes from the beginning of the file.
    U32 getPosition() const;

    /// Sets the current position in the file.
    ///
    /// You can set either a relative or absolute position to go to in the file.
    ///
    /// @code
    /// File *foo;
    ///
    /// ... set up file ...
    ///
    /// // Go to byte 32 in the file...
    /// foo->setPosition(32);
    ///
    /// // Now skip back 20 bytes...
    /// foo->setPosition(-20, false);
    ///
    /// // And forward 17...
    /// foo->setPosition(17, false);
    /// @endcode
    ///
    /// @returns The status of the file
    Status setPosition(S64 position, bool absolutePos = true);

    /// Returns the size of the file
    U32 getSize() const;

    /// Make sure everything that's supposed to be written to the file gets written.
    ///
    /// @returns The status of the file.
    Status flush();

    /// Closes the file
    ///
    /// @returns The status of the file.
    Status close();

    /// Gets the status of the file
    Status getStatus() const;

    /// Reads "size" bytes from the file, and dumps data into "dst".
    /// The number of actual bytes read is returned in bytesRead
    /// @returns The status of the file
    Status read(U32 size, char* dst, U32* bytesRead = NULL);

    /// Writes "size" bytes into the file from the pointer "src".
    /// The number of actual bytes written is returned in bytesWritten
    /// @returns The status of the file
    Status write(U32 size, const char* src, U32* bytesWritten = NULL);

    /// Returns whether or not this file is capable of the given function.
    bool hasCapability(Capability cap) const;

protected:
    Status setStatus();                 ///< Called after error encountered.
    Status setStatus(Status status);    ///< Setter for the current status.
};

#endif // _FILE_IO_H_
