//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platformWin32/platformWin32.h"
#include "core/fileio.h"
#include "core/tVector.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "core/resManager.h"
#include "core/unicode.h"

// Microsoft VC++ has this POSIX header in the wrong directory
#if defined(TORQUE_COMPILER_VISUALC)
#include <sys/utime.h>
#elif defined (TORQUE_COMPILER_GCC)
#include <time.h>
#include <sys/utime.h>
#else
#include <utime.h>
#endif


//-------------------------------------- Helper Functions
static void forwardslash(char* str)
{
    while (*str)
    {
        if (*str == '\\')
            *str = '/';
        str++;
    }
}

static void backslash(char* str)
{
    while (*str)
    {
        if (*str == '/')
            *str = '\\';
        str++;
    }
}

StringTableEntry Platform::createPlatformFriendlyFilename( const char *filename )
{
    return StringTable->insert( filename );
}

//-----------------------------------------------------------------------------
bool dFileDelete(const char* name)
{
    if (!name || (dStrlen(name) >= MAX_PATH))
        return(false);
    //return(::DeleteFile(name));
    return(remove(name) == 0);
}

bool dFileTouch(const char* name)
{
    // change the modified time to the current time (0byte WriteFile fails!)
    return(utime(name, 0) != -1);
}

//-----------------------------------------------------------------------------
// Constructors & Destructor
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// After construction, the currentStatus will be Closed and the capabilities
// will be 0.
//-----------------------------------------------------------------------------
File::File()
    : currentStatus(Closed), capability(0)
{
    AssertFatal(sizeof(HANDLE) == sizeof(void*), "File::File: cannot cast void* to HANDLE");

    handle = (void*)INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// insert a copy constructor here... (currently disabled)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
File::~File()
{
    close();
    handle = (void*)INVALID_HANDLE_VALUE;
}


//-----------------------------------------------------------------------------
// Open a file in the mode specified by openMode (Read, Write, or ReadWrite).
// Truncate the file if the mode is either Write or ReadWrite and truncate is
// true.
//
// Sets capability appropriate to the openMode.
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::open(const char* filename, const AccessMode openMode)
{
    static char filebuf[2048];
    dStrcpy(filebuf, filename);
    backslash(filebuf);
#ifdef UNICODE
    UTF16 fname[2048];
    convertUTF8toUTF16((UTF8*)filebuf, fname, sizeof(fname));
#else
    char* fname;
    fname = filebuf;
#endif

    AssertFatal(NULL != fname, "File::open: NULL fname");
    AssertWarn(INVALID_HANDLE_VALUE == (HANDLE)handle, "File::open: handle already valid");

    // Close the file if it was already open...
    if (Closed != currentStatus)
        close();

    // create the appropriate type of file...
    switch (openMode)
    {
    case Read:
        handle = (void*)CreateFile(fname,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
        break;
    case Write:
        handle = (void*)CreateFile(fname,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
        break;
    case ReadWrite:
        handle = (void*)CreateFile(fname,
            GENERIC_WRITE | GENERIC_READ,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
        break;
    case WriteAppend:
        handle = (void*)CreateFile(fname,
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
        break;

    default:
        AssertFatal(false, "File::open: bad access mode");    // impossible
    }

    if (INVALID_HANDLE_VALUE == (HANDLE)handle)                // handle not created successfully
        return setStatus();
    else
    {
        // successfully created file, so set the file capabilities...
        switch (openMode)
        {
        case Read:
            capability = U32(FileRead);
            break;
        case Write:
        case WriteAppend:
            capability = U32(FileWrite);
            break;
        case ReadWrite:
            capability = U32(FileRead) |
                U32(FileWrite);
            break;
        default:
            AssertFatal(false, "File::open: bad access mode");
        }
        return currentStatus = Ok;                                // success!
    }
}

//-----------------------------------------------------------------------------
// Get the current position of the file pointer.
//-----------------------------------------------------------------------------
U32 File::getPosition() const
{
    AssertFatal(Closed != currentStatus, "File::getPosition: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getPosition: invalid file handle");

    LARGE_INTEGER in, out;
    in.QuadPart = 0;


    SetFilePointerEx((HANDLE)handle,
        in,                                    // how far to move
        &out,                         // pointer to high word
        FILE_CURRENT);                        // from what point

    return out.QuadPart;
}

//-----------------------------------------------------------------------------
// Set the position of the file pointer.
// Absolute and relative positioning is supported via the absolutePos
// parameter.
//
// If positioning absolutely, position MUST be positive - an IOError results if
// position is negative.
// Position can be negative if positioning relatively, however positioning
// before the start of the file is an IOError.
//
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::setPosition(S64 position, bool absolutePos)
{
    AssertFatal(Closed != currentStatus, "File::setPosition: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::setPosition: invalid file handle");

    if (Ok != currentStatus && EOS != currentStatus)
        return currentStatus;

    LARGE_INTEGER li;
    li.QuadPart = position;

    switch (absolutePos)
    {
    case true:                                                    // absolute position
        AssertFatal(0 <= position, "File::setPosition: negative absolute position");

        // position beyond EOS is OK
        li.LowPart = SetFilePointer((HANDLE)handle,
            li.LowPart,
            &li.HighPart,
            FILE_BEGIN);
        break;
    case false:                                                    // relative position
        //AssertFatal((getPosition() >= (U32)abs(position) && 0 > position) || 0 <= position, "File::setPosition: negative relative position");


        // position beyond EOS is OK
        li.LowPart = SetFilePointer((HANDLE)handle,
            li.LowPart,
            &li.HighPart,
            FILE_CURRENT);

    }

    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
        return setStatus();                                        // unsuccessful
    else if (li.QuadPart >= getSize())
        return currentStatus = EOS;                                // success, at end of file
    else
        return currentStatus = Ok;                                // success!
}

//-----------------------------------------------------------------------------
// Get the size of the file in bytes.
// It is an error to query the file size for a Closed file, or for one with an
// error status.
//-----------------------------------------------------------------------------
U32 File::getSize() const
{
    AssertWarn(Closed != currentStatus, "File::getSize: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getSize: invalid file handle");

    if (Ok == currentStatus || EOS == currentStatus)
    {
        LARGE_INTEGER li;
        GetFileSizeEx((HANDLE)handle, &li);  // success!
        return li.QuadPart;
    }
    else
        return 0;                                                // unsuccessful
}

//-----------------------------------------------------------------------------
// Flush the file.
// It is an error to flush a read-only file.
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::flush()
{
    AssertFatal(Closed != currentStatus, "File::flush: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::flush: invalid file handle");
    AssertFatal(true == hasCapability(FileWrite), "File::flush: cannot flush a read-only file");

    if (0 != FlushFileBuffers((HANDLE)handle))
        return setStatus();                                        // unsuccessful
    else
        return currentStatus = Ok;                                // success!
}

//-----------------------------------------------------------------------------
// Close the File.
//
// Returns the currentStatus
//-----------------------------------------------------------------------------
File::Status File::close()
{
    // check if it's already closed...
    if (Closed == currentStatus)
        return currentStatus;

    // it's not, so close it...
    if (INVALID_HANDLE_VALUE != (HANDLE)handle)
    {
        if (0 == CloseHandle((HANDLE)handle))
            return setStatus();                                    // unsuccessful
    }
    handle = (void*)INVALID_HANDLE_VALUE;
    return currentStatus = Closed;
}

//-----------------------------------------------------------------------------
// Self-explanatory.
//-----------------------------------------------------------------------------
File::Status File::getStatus() const
{
    return currentStatus;
}

//-----------------------------------------------------------------------------
// Sets and returns the currentStatus when an error has been encountered.
//-----------------------------------------------------------------------------
File::Status File::setStatus()
{
    switch (GetLastError())
    {
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_ACCESS:
    case ERROR_TOO_MANY_OPEN_FILES:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_SHARING_VIOLATION:
    case ERROR_HANDLE_DISK_FULL:
        return currentStatus = IOError;

    default:
        return currentStatus = UnknownError;
    }
}

//-----------------------------------------------------------------------------
// Sets and returns the currentStatus to status.
//-----------------------------------------------------------------------------
File::Status File::setStatus(File::Status status)
{
    return currentStatus = status;
}

//-----------------------------------------------------------------------------
// Read from a file.
// The number of bytes to read is passed in size, the data is returned in src.
// The number of bytes read is available in bytesRead if a non-Null pointer is
// provided.
//-----------------------------------------------------------------------------
File::Status File::read(U32 size, char* dst, U32* bytesRead)
{
    AssertFatal(Closed != currentStatus, "File::read: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::read: invalid file handle");
    AssertFatal(NULL != dst, "File::read: NULL destination pointer");
    AssertFatal(true == hasCapability(FileRead), "File::read: file lacks capability");
    AssertWarn(0 != size, "File::read: size of zero");

    if (Ok != currentStatus || 0 == size)
        return currentStatus;
    else
    {
        DWORD lastBytes;
        DWORD* bytes = (NULL == bytesRead) ? &lastBytes : (DWORD*)bytesRead;
        if (0 != ReadFile((HANDLE)handle, dst, size, bytes, NULL))
        {
            if (*((U32*)bytes) != size)
                return currentStatus = EOS;                        // end of stream
        }
        else
            return setStatus();                                    // unsuccessful
    }
    return currentStatus = Ok;                                    // successfully read size bytes
}

//-----------------------------------------------------------------------------
// Write to a file.
// The number of bytes to write is passed in size, the data is passed in src.
// The number of bytes written is available in bytesWritten if a non-Null
// pointer is provided.
//-----------------------------------------------------------------------------
File::Status File::write(U32 size, const char* src, U32* bytesWritten)
{
    AssertFatal(Closed != currentStatus, "File::write: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::write: invalid file handle");
    AssertFatal(NULL != src, "File::write: NULL source pointer");
    AssertFatal(true == hasCapability(FileWrite), "File::write: file lacks capability");
    AssertWarn(0 != size, "File::write: size of zero");

    if ((Ok != currentStatus && EOS != currentStatus) || 0 == size)
        return currentStatus;
    else
    {
        DWORD lastBytes;
        DWORD* bytes = (NULL == bytesWritten) ? &lastBytes : (DWORD*)bytesWritten;
        if (0 != WriteFile((HANDLE)handle, src, size, bytes, NULL))
            return currentStatus = Ok;                            // success!
        else
            return setStatus();                                    // unsuccessful
    }
}

//-----------------------------------------------------------------------------
// Self-explanatory.
//-----------------------------------------------------------------------------
bool File::hasCapability(Capability cap) const
{
    return (0 != (U32(cap) & capability));
}

S32 Platform::compareFileTimes(const FileTime& a, const FileTime& b)
{
    if (a.v2 > b.v2)
        return 1;
    if (a.v2 < b.v2)
        return -1;
    if (a.v1 > b.v1)
        return 1;
    if (a.v1 < b.v1)
        return -1;
    return 0;
}

static bool recurseDumpPath(const char* path, const char* pattern, Vector<Platform::FileInfo>& fileVector, S32 recurseDepth)
{
    char buf[1024];
    WIN32_FIND_DATA findData;

    dSprintf(buf, sizeof(buf), "%s/%s", path, pattern);

#ifdef UNICODE
    UTF16 search[1024];
    convertUTF8toUTF16((UTF8*)buf, search, sizeof(search));
#else
    char* search = buf;
#endif

    HANDLE handle = FindFirstFile(search, &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    do
    {
#ifdef UNICODE
        char fnbuf[1024];
        convertUTF16toUTF8(findData.cFileName, (UTF8*)fnbuf, sizeof(fnbuf));
#else
        char* fnbuf = findData.cFileName;
#endif

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // make sure it is a directory
            if (findData.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM))
                continue;

            // skip . and .. directories
            if (dStrcmp(fnbuf, ".") == 0 || dStrcmp(fnbuf, "..") == 0)
                continue;

            // Skip excluded directores
            if (Platform::isExcludedDirectory(fnbuf))
                continue;

            char child[1024];
            dSprintf(child, sizeof(child), "%s/%s", path, fnbuf);
            if (recurseDepth > 0)
                recurseDumpPath(child, pattern, fileVector, recurseDepth - 1);
            else if (recurseDepth == -1)
                recurseDumpPath(child, pattern, fileVector, -1);
        }
        else
        {
            // make sure it is the kind of file we're looking for
            if (findData.dwFileAttributes &
                (FILE_ATTRIBUTE_DIRECTORY |
                    FILE_ATTRIBUTE_OFFLINE |
                    FILE_ATTRIBUTE_SYSTEM |
                    FILE_ATTRIBUTE_TEMPORARY))
                continue;

            // add it to the list
            fileVector.increment();
            Platform::FileInfo& rInfo = fileVector.last();

            rInfo.pFullPath = StringTable->insert(path);
            rInfo.pFileName = StringTable->insert(fnbuf);
            rInfo.fileSize = findData.nFileSizeLow;
        }

    } while (FindNextFile(handle, &findData));

    FindClose(handle);
    return true;
}


//--------------------------------------

bool Platform::getFileTimes(const char* filePath, FileTime* createTime, FileTime* modifyTime)
{
    WIN32_FIND_DATA findData;
#ifdef UNICODE
    UTF16 fp[512];
    convertUTF8toUTF16((UTF8*)filePath, fp, sizeof(fp));
#else
    const char* fp = filePath;
#endif

    HANDLE h = FindFirstFile(fp, &findData);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    if (createTime)
    {
        createTime->v1 = findData.ftCreationTime.dwLowDateTime;
        createTime->v2 = findData.ftCreationTime.dwHighDateTime;
    }
    if (modifyTime)
    {
        modifyTime->v1 = findData.ftLastWriteTime.dwLowDateTime;
        modifyTime->v2 = findData.ftLastWriteTime.dwHighDateTime;
    }
    FindClose(h);
    return true;
}

//--------------------------------------
bool Platform::createPath(const char* file)
{
    char pathbuf[1024];
    const char* dir;
    pathbuf[0] = 0;
    U32 pathLen = 0;

    while ((dir = dStrchr(file, '/')) != NULL)
    {
        dStrncpy(pathbuf + pathLen, file, dir - file);
        pathbuf[pathLen + dir - file] = 0;
#ifdef UNICODE
        UTF16 b[1024];
        convertUTF8toUTF16((UTF8*)pathbuf, b, sizeof(b));
        bool ret = CreateDirectory(b, NULL);
#else
        bool ret = CreateDirectory(pathbuf, NULL);
#endif
        pathLen += dir - file;
        pathbuf[pathLen++] = '/';
        file = dir + 1;
    }
    return true;
}

// [tom, 7/12/2005] Rather then converting this to unicode, just using the ANSI
// versions of the Win32 API as its quicker for testing.
bool Platform::cdFileExists(const char* filePath, const char* volumeName, S32 serialNum)
{
    if (!filePath || !filePath[0])
        return true;

    //first find the CD device...
    char fileBuf[256];
    char drivesBuf[256];
    S32 length = GetLogicalDriveStringsA(256, drivesBuf);
    char* drivePtr = drivesBuf;
    while (S32(drivePtr - drivesBuf) < length)
    {
        char driveVolume[256], driveFileSystem[256];
        U32 driveSerial, driveFNLength, driveFlags;
        if ((dStricmp(drivePtr, "A:\\") != 0 && dStricmp(drivePtr, "B:\\") != 0) &&
            GetVolumeInformationA((const char*)drivePtr, &driveVolume[0], (unsigned long)255,
                (unsigned long*)&driveSerial, (unsigned long*)&driveFNLength,
                (unsigned long*)&driveFlags, &driveFileSystem[0], (unsigned long)255))
        {
#if defined (TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
            Con::printf("Found Drive: %s, vol: %s, serial: %d", drivePtr, driveVolume, driveSerial);
#endif
            //see if the volume and serial number match
            if (!dStricmp(volumeName, driveVolume) && (!serialNum || (serialNum == driveSerial)))
            {
                //see if the file exists on this volume
                if (dStrlen(drivePtr) == 3 && drivePtr[2] == '\\' && filePath[0] == '\\')
                    dSprintf(fileBuf, sizeof(fileBuf), "%s%s", drivePtr, filePath + 1);
                else
                    dSprintf(fileBuf, sizeof(fileBuf), "%s%s", drivePtr, filePath);
#if defined (TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
                Con::printf("Looking for file: %s on %s", fileBuf, driveVolume);
#endif
                WIN32_FIND_DATAA findData;
                HANDLE h = FindFirstFileA(fileBuf, &findData);
                if (h != INVALID_HANDLE_VALUE)
                {
                    FindClose(h);
                    return true;
                }
                FindClose(h);
            }
        }

        //check the next drive
        drivePtr += dStrlen(drivePtr) + 1;
    }

    return false;
}

//--------------------------------------
bool Platform::dumpPath(const char* path, Vector<Platform::FileInfo>& fileVector, S32 recurseDepth)
{
    return recurseDumpPath(path, "*", fileVector, recurseDepth);
}


//--------------------------------------
StringTableEntry Platform::getWorkingDirectory()
{
    static StringTableEntry cwd = NULL;
    if (!cwd)
    {
        char cwd_buf[2048];
        GetCurrentDirectoryA(2047, cwd_buf);
        forwardslash(cwd_buf);
        cwd = StringTable->insert(cwd_buf);
    }
    return cwd;
}

/*bool Platform::setWorkingDirectory(const char *in_pPath)
{
   return (bool)SetCurrentDirectoryA(in_pPath);
}*/

//--------------------------------------
bool Platform::isFile(const char* pFilePath)
{
    if (!pFilePath || !*pFilePath)
        return false;

    // Get file info
    WIN32_FIND_DATA findData;
#ifdef UNICODE
    UTF16 b[4096];
    S32 len = dStrlen(pFilePath);
    AssertFatal(dStrlen(pFilePath) * 2 < sizeof(b), "doh!");
    convertUTF8toUTF16((UTF8*)pFilePath, b, sizeof(b));
    HANDLE handle = FindFirstFile(b, &findData);
#else
    HANDLE handle = FindFirstFile(pFilePath, &findData);
#endif
    FindClose(handle);

    if (handle == INVALID_HANDLE_VALUE)
        return false;

    // if the file is a Directory, Offline, System or Temporary then FALSE
    if (findData.dwFileAttributes &
        (FILE_ATTRIBUTE_DIRECTORY |
            FILE_ATTRIBUTE_OFFLINE |
            FILE_ATTRIBUTE_SYSTEM |
            FILE_ATTRIBUTE_TEMPORARY))
        return false;

    // must be a real file then
    return true;
}

//--------------------------------------
S32 Platform::getFileSize(const char* pFilePath)
{
    if (!pFilePath || !*pFilePath)
        return -1;

    // Get file info
    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA(pFilePath, &findData);
    FindClose(handle);

    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    // if the file is a Directory, Offline, System or Temporary then FALSE
    if (findData.dwFileAttributes &
        (FILE_ATTRIBUTE_DIRECTORY |
            FILE_ATTRIBUTE_OFFLINE |
            FILE_ATTRIBUTE_SYSTEM |
            FILE_ATTRIBUTE_TEMPORARY))
        return -1;

    // must be a real file then
    return findData.nFileSizeLow;;
}


//--------------------------------------
bool Platform::isDirectory(const char* pDirPath)
{
    if (!pDirPath || !*pDirPath)
        return false;

    // Get file info
    WIN32_FIND_DATA findData;
#ifdef UNICODE
    UTF16 b[512];
    convertUTF8toUTF16((UTF8*)pDirPath, b, sizeof(b));
    HANDLE handle = FindFirstFile(b, &findData);
#else
    HANDLE handle = FindFirstFile(pDirPath, &findData);
#endif

    FindClose(handle);

    if (handle == INVALID_HANDLE_VALUE)
        return false;

    // if the file is a Directory, Offline, System or Temporary then FALSE
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        // make sure it's a valid game directory
        if (findData.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_SYSTEM))
            return false;

        // must be a directory
        return true;
    }

    return false;
}



//--------------------------------------
bool Platform::isSubDirectory(const char* pParent, const char* pDir)
{
    if (!pParent || !*pDir)
        return false;

    // this is somewhat of a brute force method but we need to be 100% sure
    // that the user cannot enter things like ../dir or /dir etc,...
    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA(avar("%s/*", pParent), &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        // if it is a directory...
        if (findData.dwFileAttributes &
            (FILE_ATTRIBUTE_DIRECTORY |
                FILE_ATTRIBUTE_OFFLINE |
                FILE_ATTRIBUTE_SYSTEM |
                FILE_ATTRIBUTE_TEMPORARY))
        {
            // and the names match
            if (dStrcmp(pDir, findData.cFileName) == 0)
            {
                // then we have a real sub directory
                FindClose(handle);
                return true;
            }
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);
    return false;
}

//------------------------------------------------------------------------------

bool Platform::fileTimeToString(FileTime* time, char* string, U32 strLen)
{
    if (!time || !string)
        return(false);

    dSprintf(string, strLen, "%d:%d", time->v2, time->v1);
    return(true);
}

bool Platform::stringToFileTime(const char* string, FileTime* time)
{
    if (!time || !string)
        return(false);

    char buf[80];
    dSprintf(buf, sizeof(buf), (char*)string);

    char* sep = (char*)dStrstr((const char*)buf, (const char*)":");
    if (!sep)
        return(false);

    *sep = 0;
    sep++;

    time->v2 = dAtoi(buf);
    time->v1 = dAtoi(sep);

    return(true);
}

// Volume Functions

void Platform::getVolumeNamesList(Vector<const char*>& out_rNameVector, bool bOnlyFixedDrives)
{
    DWORD dwDrives = GetLogicalDrives();
    DWORD dwMask = 1;
    char driveLetter[12];

    out_rNameVector.clear();

    for (int i = 0; i < 32; i++)
    {
        dMemset(driveLetter, 0, 12);
        if (dwDrives & dwMask)
        {
            dSprintf(driveLetter, 12, "%c:", (i + 'A'));

            if (bOnlyFixedDrives && GetDriveTypeA(driveLetter) == DRIVE_FIXED)
                out_rNameVector.push_back(StringTable->insert(driveLetter));
            else if (!bOnlyFixedDrives)
                out_rNameVector.push_back(StringTable->insert(driveLetter));
        }
        dwMask <<= 1;
    }
}

void Platform::getVolumeInformationList(Vector<VolumeInformation>& out_rVolumeInfoVector, bool bOnlyFixedDrives)
{
    Vector<const char*> drives;

    getVolumeNamesList(drives, bOnlyFixedDrives);

    if (!drives.empty())
    {
        Vector<StringTableEntry>::iterator i;
        for (i = drives.begin(); i != drives.end(); i++)
        {
            VolumeInformation info;
            char lpszVolumeName[256];
            char lpszFileSystem[256];
            DWORD dwSerial = 0;
            DWORD dwMaxComponentLength = 0;
            DWORD dwFileSystemFlags = 0;

            dMemset(lpszVolumeName, 0, 256);
            dMemset(lpszFileSystem, 0, 256);
            dMemset(&info, 0, sizeof(VolumeInformation));


            // More volume information
            UINT uDriveType = GetDriveTypeA((*i));
            if (uDriveType == DRIVE_UNKNOWN)
                info.Type = DRIVETYPE_UNKNOWN;
            else if (uDriveType == DRIVE_REMOVABLE)
                info.Type = DRIVETYPE_REMOVABLE;
            else if (uDriveType == DRIVE_FIXED)
                info.Type = DRIVETYPE_FIXED;
            else if (uDriveType == DRIVE_CDROM)
                info.Type = DRIVETYPE_CDROM;
            else if (uDriveType == DRIVE_RAMDISK)
                info.Type = DRIVETYPE_RAMDISK;
            else if (uDriveType == DRIVE_REMOTE)
                info.Type = DRIVETYPE_REMOTE;

            info.RootPath = StringTable->insert((*i));

            // We don't retrieve drive volume info for removable drives, because it's loud :(
            if (info.Type != DRIVETYPE_REMOVABLE)
            {
                // Standard volume information
                GetVolumeInformationA((*i), lpszVolumeName, 256, &dwSerial, &dwMaxComponentLength, &dwFileSystemFlags, lpszFileSystem, 256);

                info.FileSystem = StringTable->insert(lpszFileSystem);
                info.Name = StringTable->insert(lpszVolumeName);
                info.SerialNumber = dwSerial;
                info.ReadOnly = false;  // not detected yet - to implement later
            }
            out_rVolumeInfoVector.push_back(info);

            // I opted not to get free disk space because of the overhead of the calculations required for it

        }
    }
}


bool Platform::hasSubDirectory(const char* pPath)
{
    if (!pPath)
        return false;

    ResourceManager->initExcludedDirectories();

    char search[1024];
    WIN32_FIND_DATAA findData;

    // Compose our search string - Format : ([path]/[subpath]/*)
    char trail = pPath[dStrlen(pPath) - 1];
    if (trail == '/')
        dStrcpy(search, pPath);
    else
        dSprintf(search, 1024, "%s/*", pPath);

    // See if we get any hits
    HANDLE handle = FindFirstFileA(search, &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // skip . and .. directories
            if (dStrcmp(findData.cFileName, ".") == 0 || dStrcmp(findData.cFileName, "..") == 0)
                continue;

            if (Platform::isExcludedDirectory(findData.cFileName))
                continue;

            Platform::clearExcludedDirectories();

            return true;
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);

    Platform::clearExcludedDirectories();

    return false;
}


static bool recurseDumpDirectories(const char* basePath, const char* subPath, Vector<StringTableEntry>& directoryVector, S32 currentDepth, S32 recurseDepth, bool noBasePath)
{
    char search[1024];
    WIN32_FIND_DATAA findData;

    //////////////////////////////////////////////////////////////////////////
    // Compose our search string - Format : ([path]/[subpath]/*)
    //////////////////////////////////////////////////////////////////////////

    char trail = basePath[dStrlen(basePath) - 1];
    char subTrail;
    if (subPath)
        subTrail = subPath[dStrlen(subPath) - 1];

    if (trail == '/')
    {
        // we have a sub path and it's not an empty string
        if (subPath && (dStrncmp(subPath, "", 1) != 0))
        {
            if (subTrail == '/')
                dSprintf(search, 1024, "%s%s*", basePath, subPath);
            else
                dSprintf(search, 1024, "%s%s/*", basePath, subPath);
        }
        else
            dSprintf(search, 1024, "%s*", basePath);
    }
    else
    {
        if (subPath && (dStrncmp(subPath, "", 1) != 0))
            if (subTrail == '/')
                dSprintf(search, 1024, "%s%s*", basePath, subPath);
            else
                dSprintf(search, 1024, "%s%s/*", basePath, subPath);
        else
            dSprintf(search, 1024, "%s/*", basePath);
    }

    //////////////////////////////////////////////////////////////////////////
    // See if we get any hits
    //////////////////////////////////////////////////////////////////////////

    HANDLE handle = FindFirstFileA(search, &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    //////////////////////////////////////////////////////////////////////////
    // add path to our return list ( provided it is valid )
    //////////////////////////////////////////////////////////////////////////
    if (!Platform::isExcludedDirectory(subPath))
    {

        if (noBasePath)
        {
            // We have a path and it's not an empty string or an excluded directory
            if ((subPath && (dStrncmp(subPath, "", 1) != 0)))
                directoryVector.push_back(StringTable->insert(subPath));
        }
        else
        {
            if ((subPath && (dStrncmp(subPath, "", 1) != 0)))
            {
                char szPath[1024];
                dMemset(szPath, 0, 1024);
                if (trail != '/')
                    dSprintf(szPath, 1024, "%s%s", basePath, subPath);
                else
                    dSprintf(szPath, 1024, "%s%s", basePath, &subPath[1]);
                directoryVector.push_back(StringTable->insert(szPath));
            }
            else
                directoryVector.push_back(StringTable->insert(basePath));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Iterate through and grab valid directories
    //////////////////////////////////////////////////////////////////////////

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // skip . and .. directories
            if (dStrcmp(findData.cFileName, ".") == 0 || dStrcmp(findData.cFileName, "..") == 0)
                continue;

            // skip excluded directories
            if (Platform::isExcludedDirectory(findData.cFileName))
                continue;

            if ((subPath && (dStrncmp(subPath, "", 1) != 0)))
            {
                char child[1024];

                if (subTrail == '/')
                    dSprintf(child, sizeof(child), "%s%s", subPath, findData.cFileName);
                else
                    dSprintf(child, sizeof(child), "%s/%s", subPath, findData.cFileName);

                if (currentDepth < recurseDepth || recurseDepth == -1)
                    recurseDumpDirectories(basePath, child, directoryVector, currentDepth + 1, recurseDepth, noBasePath);

            }
            else
            {
                char child[1024];

                if (trail == '/')
                    dStrcpy(child, findData.cFileName);
                else
                    dSprintf(child, sizeof(child), "/%s", findData.cFileName);

                if (currentDepth < recurseDepth || recurseDepth == -1)
                    recurseDumpDirectories(basePath, child, directoryVector, currentDepth + 1, recurseDepth, noBasePath);
            }
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);
    return true;
}

bool Platform::dumpDirectories(const char* path, Vector<StringTableEntry>& directoryVector, S32 depth, bool noBasePath)
{
    ResourceManager->initExcludedDirectories();

    bool retVal = recurseDumpDirectories(path, "", directoryVector, 0, depth, noBasePath);

    clearExcludedDirectories();

    return retVal;
}



