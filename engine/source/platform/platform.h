//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifndef _TORQUECONFIG_H_
#  include "core/torqueConfig.h"
#endif

#ifndef _TORQUE_TYPES_H_
#  include "platform/types.h"
#endif

#ifndef _PLATFORMASSERT_H_
#  include "platform/platformAssert.h"
#endif

#include <new>
#include <cstdarg>

class GFXWindowTarget;

//------------------------------------------------------------------------------
// Endian conversions
#ifdef TORQUE_LITTLE_ENDIAN

inline U16 convertHostToLEndian(U16 i) { return i; }
inline U16 convertLEndianToHost(U16 i) { return i; }
inline U32 convertHostToLEndian(U32 i) { return i; }
inline U32 convertLEndianToHost(U32 i) { return i; }
inline S16 convertHostToLEndian(S16 i) { return i; }
inline S16 convertLEndianToHost(S16 i) { return i; }
inline S32 convertHostToLEndian(S32 i) { return i; }
inline S32 convertLEndianToHost(S32 i) { return i; }

inline F32 convertHostToLEndian(F32 i) { return i; }
inline F32 convertLEndianToHost(F32 i) { return i; }

inline F64 convertHostToLEndian(F64 i) { return i; }
inline F64 convertLEndianToHost(F64 i) { return i; }

inline U16 convertHostToBEndian(U16 i)
{
    return U16((i << 8) | (i >> 8));
}

inline U16 convertBEndianToHost(U16 i)
{
    return U16((i << 8) | (i >> 8));
}

inline S16 convertHostToBEndian(S16 i)
{
    return S16(convertHostToBEndian(U16(i)));
}

inline S16 convertBEndianToHost(S16 i)
{
    return S16(convertBEndianToHost(S16(i)));
}

inline U32 convertHostToBEndian(U32 i)
{
    return ((i << 24) & 0xff000000) |
        ((i << 8) & 0x00ff0000) |
        ((i >> 8) & 0x0000ff00) |
        ((i >> 24) & 0x000000ff);
}

inline U32 convertBEndianToHost(U32 i)
{
    return ((i << 24) & 0xff000000) |
        ((i << 8) & 0x00ff0000) |
        ((i >> 8) & 0x0000ff00) |
        ((i >> 24) & 0x000000ff);
}

inline S32 convertHostToBEndian(S32 i)
{
    return S32(convertHostToBEndian(U32(i)));
}

inline S32 convertBEndianToHost(S32 i)
{
    return S32(convertBEndianToHost(U32(i)));
}

inline U64 convertBEndianToHost(U64 i)
{
    U32* inp = (U32*)&i;
    U64 ret;
    U32* outp = (U32*)&ret;
    outp[0] = convertBEndianToHost(inp[1]);
    outp[1] = convertBEndianToHost(inp[0]);
    return ret;
}

inline U64 convertHostToBEndian(U64 i)
{
    U32* inp = (U32*)&i;
    U64 ret;
    U32* outp = (U32*)&ret;
    outp[0] = convertHostToBEndian(inp[1]);
    outp[1] = convertHostToBEndian(inp[0]);
    return ret;
}

inline F64 convertBEndianToHost(F64 in_swap)
{
    U64 result = convertBEndianToHost(*((U64*)&in_swap));
    return *((F64*)&result);
}

inline F64 convertHostToBEndian(F64 in_swap)
{
    U64 result = convertHostToBEndian(*((U64*)&in_swap));
    return *((F64*)&result);
}

#elif defined(TORQUE_BIG_ENDIAN)

inline U16 convertHostToBEndian(U16 i) { return i; }
inline U16 convertBEndianToHost(U16 i) { return i; }
inline U32 convertHostToBEndian(U32 i) { return i; }
inline U32 convertBEndianToHost(U32 i) { return i; }
inline S16 convertHostToBEndian(S16 i) { return i; }
inline S16 convertBEndianToHost(S16 i) { return i; }
inline S32 convertHostToBEndian(S32 i) { return i; }
inline S32 convertBEndianToHost(S32 i) { return i; }

inline U16 convertHostToLEndian(U16 i)
{
    return (i << 8) | (i >> 8);
}
inline U16 convertLEndianToHost(U16 i)
{
    return (i << 8) | (i >> 8);
}
inline U32 convertHostToLEndian(U32 i)
{
    return ((i << 24) & 0xff000000) |
        ((i << 8) & 0x00ff0000) |
        ((i >> 8) & 0x0000ff00) |
        ((i >> 24) & 0x000000ff);
}
inline U32 convertLEndianToHost(U32 i)
{
    return ((i << 24) & 0xff000000) |
        ((i << 8) & 0x00ff0000) |
        ((i >> 8) & 0x0000ff00) |
        ((i >> 24) & 0x000000ff);
}

inline U64 convertLEndianToHost(U64 i)
{
    U32* inp = (U32*)&i;
    U64 ret;
    U32* outp = (U32*)&ret;
    outp[0] = convertLEndianToHost(inp[1]);
    outp[1] = convertLEndianToHost(inp[0]);
    return ret;
}

inline U64 convertHostToLEndian(U64 i)
{
    U32* inp = (U32*)&i;
    U64 ret;
    U32* outp = (U32*)&ret;
    outp[0] = convertHostToLEndian(inp[1]);
    outp[1] = convertHostToLEndian(inp[0]);
    return ret;
}

inline F64 convertLEndianToHost(F64 in_swap)
{
    U64 result = convertLEndianToHost(*((U64*)&in_swap));
    return *((F64*)&result);
}

inline F64 convertHostToLEndian(F64 in_swap)
{
    U64 result = convertHostToLEndian(*((U64*)&in_swap));
    return *((F64*)&result);
}

inline F32 convertHostToLEndian(F32 i)
{
    U32 result = convertHostToLEndian(*reinterpret_cast<U32*>(&i));
    return *reinterpret_cast<F32*>(&result);
}

inline F32 convertLEndianToHost(F32 i)
{
    U32 result = convertLEndianToHost(*reinterpret_cast<U32*>(&i));
    return *reinterpret_cast<F32*>(&result);
}

inline S16 convertHostToLEndian(S16 i) { return S16(convertHostToLEndian(U16(i))); }
inline S16 convertLEndianToHost(S16 i) { return S16(convertLEndianToHost(U16(i))); }
inline S32 convertHostToLEndian(S32 i) { return S32(convertHostToLEndian(U32(i))); }
inline S32 convertLEndianToHost(S32 i) { return S32(convertLEndianToHost(U32(i))); }

#else
#error "Endian define not set"
#endif


//------------------------------------------------------------------------------
// Input structures and functions - all input is pushed into the input event queue
template <class T> class Vector;
class Point2I;


// Theese emuns must be globally scoped so that they work
// with the inline assembly
enum ProcessorType
{
    // x86
    CPU_X86Compatible,
    CPU_Intel_Unknown,
    CPU_Intel_486,
    CPU_Intel_Pentium,
    CPU_Intel_PentiumMMX,
    CPU_Intel_PentiumPro,
    CPU_Intel_PentiumII,
    CPU_Intel_PentiumCeleron,
    CPU_Intel_PentiumIII,
    CPU_Intel_Pentium4,
    CPU_AMD_K6,
    CPU_AMD_K6_2,
    CPU_AMD_K6_3,
    CPU_AMD_Athlon,
    CPU_AMD_Unknown,
    CPU_Cyrix_6x86,
    CPU_Cyrix_MediaGX,
    CPU_Cyrix_6x86MX,
    CPU_Cyrix_GXm,        // Media GX w/ MMX
    CPU_Cyrix_Unknown,
    // PowerPC
    CPU_PowerPC_Unknown,
    CPU_PowerPC_601,
    CPU_PowerPC_603e,
    CPU_PowerPC_603,
    CPU_PowerPC_604e,
    CPU_PowerPC_604,
    CPU_PowerPC_G3,
    CPU_PowerPC_G4,
    CPU_PowerPC_G4u, // PPC7450 ultra g4
    // Xenon
    CPU_Xenon,
};

enum x86Properties
{  // x86 properties
    CPU_PROP_C = (1 << 0),
    CPU_PROP_FPU = (1 << 1),
    CPU_PROP_MMX = (1 << 2),     // Integer-SIMD
    CPU_PROP_3DNOW = (1 << 3),     // AMD Float-SIMD
    CPU_PROP_SSE = (1 << 4),     // PentiumIII SIMD
    CPU_PROP_RDTSC = (1 << 5)     // Read Time Stamp Counter
 //   CPU_PROP_SSE2      = (1<<6),   // Pentium4 SIMD
 //   CPU_PROP_MP        = (1<<7)      // Multi-processor system
};

enum PPCProperties
{  // PowerPC properties
    CPU_PROP_PPCMIN = (1 << 0),
    CPU_PROP_ALTIVEC = (1 << 1),     // Float-SIMD
    CPU_PROP_PPCMP = (1 << 7)      // Multi-processor system
};

#if defined (TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
#define TORQUE_DEBUGBREAK() { __asm { int 3 }; }
#elif defined(TORQUE_SUPPORTS_GCC_INLINE_X86_ASM)
#define TORQUE_DEBUGBREAK() { asm ( "int 3"); }
#else
/// Macro to do in-line debug breaks, used for asserts.  Does inline assembly where appropriate
#define TORQUE_DEBUGBREAK() Platform::debugBreak();
#endif
enum DriveType
{
    DRIVETYPE_FIXED = 0,
    DRIVETYPE_REMOVABLE = 1,
    DRIVETYPE_REMOTE = 2,
    DRIVETYPE_CDROM = 3,
    DRIVETYPE_RAMDISK = 4,
    DRIVETYPE_UNKNOWN = 5
};

#ifdef XB_LIVE
enum LangType
{
    LANGTYPE_ENGLISH,
    LANGTYPE_FRENCH,
    LANGTYPE_SPANISH,
    LANGTYPE_GERMAN,
    LANGTYPE_ITALIAN,
    LANGTYPE_POLISH,
    LANGTYPE_PORTUGUESE,
    LANGTYPE_JAPANESE,
    LANGTYPE_CHINESE,
    LANGTYPE_KOREAN,
    LANGTYPE_UNSUPPORTED
};
#endif

enum AlertResponse
{
    ALERT_RESPONSE_OK,
    ALERT_RESPONSE_CANCEL,
    ALERT_RESPONSE_ABORT,
    ALERT_RESPONSE_RETRY,
    ALERT_RESPONSE_IGNORE
};

struct Platform
{
    static void sleep(U32 ms);
    static void init();
    static void initConsole();
    static void shutdown();
    static void process();
    static bool doCDCheck();
    static StringTableEntry createPlatformFriendlyFilename(const char *filename);
    static GFXWindowTarget* getWindowGFXTarget();
    static void initWindow(const Point2I& initialSize, const char* name);
    static void enableKeyboardTranslation(void);
    static void disableKeyboardTranslation(void);
    static void setWindowLocked(bool locked);
    static void minimizeWindow();
    static bool excludeOtherInstances(const char* string);

    static const Point2I& getWindowSize();
    static void setWindowSize(U32 newWidth, U32 newHeight, bool fullScreen, bool borderless);
    static float getRandom();
    static void AlertOK(const char* windowTitle, const char* message);
    static bool AlertOKCancel(const char* windowTitle, const char* message);
    static bool AlertRetry(const char* windowTitle, const char* message);
    static int AlertAbortRetryIgnore(const char* windowTitle, const char* message);

    struct LocalTime
    {
        U8  sec;        // seconds after minute (0-59)
        U8  min;        // Minutes after hour (0-59)
        U8  hour;       // Hours after midnight (0-23)
        U8  month;      // Month (0-11; 0=january)
        U8  monthday;   // Day of the month (1-31)
        U8  weekday;    // Day of the week (0-6, 6=sunday)
        U16 year;       // current year minus 1900
        U16 yearday;    // Day of year (0-365)
        bool isdst;     // true if daylight savings time is active
    };

    static void getLocalTime(LocalTime&);

    struct FileInfo
    {
        const char* pFullPath;
        const char* pFileName;
        U32 fileSize;
    };
    static bool cdFileExists(const char* filePath, const char* volumeName, S32 serialNum);
    static void fileToLocalTime(const FileTime& ft, LocalTime* lt);
    // compare file times returns < 0 if a is earlier than b, >0 if b is earlier than a
    static S32 compareFileTimes(const FileTime& a, const FileTime& b);
    // Process control
public:
    static void postQuitMessage(const U32 in_quitVal);
    static void debugBreak();
    static void forceShutdown(S32 returnValue);

    static U32  getTime();
    static U32  getVirtualMilliseconds();
    static U32  getRealMilliseconds();
    static void advanceTime(U32 delta);

    static S32 getBackgroundSleepTime();

    // Directory functions.  Dump path returns false iff the directory cannot be
    //  opened.
public:
    static StringTableEntry getPrefsPath(const char* file = NULL);

    static StringTableEntry getWorkingDirectory();
    static bool dumpPath(const char* in_pBasePath, Vector<FileInfo>& out_rFileVector, S32 recurseDepth = -1);
    static bool dumpDirectories(const char* path, Vector<StringTableEntry>& directoryVector, S32 depth = 1, bool noBasePath = false);
    static bool hasSubDirectory(const char* pPath);
    static bool getFileTimes(const char* filePath, FileTime* createTime, FileTime* modifyTime);
    static bool isFile(const char* pFilePath);
    static S32  getFileSize(const char* pFilePath);
    static bool isDirectory(const char* pDirPath);
    static bool isSubDirectory(const char* pParent, const char* pDir);

    static void addExcludedDirectory(const char* pDir);
    static void clearExcludedDirectories();
    static bool isExcludedDirectory(const char* pDir);

    struct VolumeInformation
    {
        StringTableEntry  RootPath;
        StringTableEntry  Name;
        StringTableEntry  FileSystem;
        U32               SerialNumber;
        U32               Type;
        bool              ReadOnly;
    }*PVolumeInformation;

    // Volume functions.
    static void getVolumeNamesList(Vector<const char*>& out_rNameVector, bool bOnlyFixedDrives = false);
    static void getVolumeInformationList(Vector<VolumeInformation>& out_rVolumeInfoVector, bool bOnlyFixedDrives = false);

    static bool createPath(const char* path); // create a directory path
    static struct SystemInfo_struct
    {
        struct Processor
        {
            ProcessorType type;
            const char* name;
            U32         mhz;
            U32         properties;      // CPU type specific enum
        } processor;
    } SystemInfo;

    // Web page launch function:
    static bool openWebBrowser(const char* webAddress);

    static const char* getLoginPassword();
    static bool setLoginPassword(const char* password);

    static const char* getClipboard();
    static bool setClipboard(const char* text);

    // User Specific Functions
    static StringTableEntry getUserName(int charLimit = 1023);
    static StringTableEntry getUserHomeDirectory();
    static StringTableEntry getUserDataDirectory();
    static bool getUserIsAdministrator();

    static bool stringToFileTime(const char* string, FileTime* time);
    static bool fileTimeToString(FileTime* time, char* string, U32 strLen);

#ifdef XB_LIVE
    static LangType getSystemLanguage();
#endif
};


struct Processor
{
    static void init();
};


//------------------------------------------------------------------------------
// time manager generates a ServerTimeEvent / ClientTimeEvent, FrameEvent combo
// every other time its process is called.
extern S32 sgTimeManagerProcessInterval;
struct TimeManager
{
    static void process();
};

// the entry point of the app is in the platform code...
// it calls out into game code at GameMain

// all input goes through the game input event queue
// whether or not it is used (replaying a journal, etc)
// is up to the game code.  The game must copy the event data out.

struct Event;

//------------------------------------------------------------------------------
// String functions

extern char* dStrcat(char* dst, const char* src);
extern UTF8* dStrcat(UTF8* dst, const UTF8* src);

extern char* dStrncat(char* dst, const char* src, dsize_t len);
extern char* dStrcatl(char* dst, dsize_t dstSize, ...);

extern int         dStrcmp(const char* str1, const char* str2);
extern int         dStrcmp(const UTF16* str1, const UTF16* str2);
extern int         dStrcmp(const UTF8* str1, const UTF8* str2);

extern int         dStricmp(const char* str1, const char* str2);
extern int         dStrncmp(const char* str1, const char* str2, dsize_t len);
extern int         dStrnicmp(const char* str1, const char* str2, dsize_t len);
extern char* dStrcpy(char* dst, const char* src);

extern char* dStrcpyl(char* dst, dsize_t dstSize, ...);
extern char* dStrncpy(char* dst, const char* src, dsize_t len);
extern char* dStrncpy(UTF8* dst, const UTF8* src, dsize_t len);

extern dsize_t      dStrlen(const char* str);

extern char* dStrupr(char* str);
extern char* dStrlwr(char* str);
extern char* dStrchr(char* str, int c);
extern const char* dStrchr(const char* str, int c);
extern char* dStrrchr(char* str, int c);
extern const char* dStrrchr(const char* str, int c);
extern dsize_t     dStrspn(const char* str, const char* set);
extern dsize_t     dStrcspn(const char* str, const char* set);

extern char* dStrstr(char* str1, char* str2);
extern char* dStrstr(const char* str1, const char* str2);


extern bool dStrEqual(const char* str1, const char* str2);
extern bool dStrStartsWith(const char* str1, const char* str2);
extern bool dStrEndsWith(const char* str1, const char* str2);
extern char* dStripPath(const char* filename);
extern char* dChopTrailingNumber(const char* name, S32& number);

/// Like ChopTrailingNumber but doesn't initialize the passed number 
/// to a default value of 2.
extern char* dGetTrailingNumber(const char* name, S32& number);


extern char* dStrtok(char* str, const char* sep);

extern int         dAtoi(const char* str);
extern float       dAtof(const char* str);
extern bool        dAtob(const char* str);

extern void   dPrintf(const char* format, ...);
extern int    dVprintf(const char* format, void* arglist);
extern int    dSprintf(char* buffer, dsize_t bufferSize, const char* format, ...);
extern int    dVsprintf(char* buffer, dsize_t bufferSize, const char* format, va_list arglist);
extern int    dSscanf(const char* buffer, const char* format, ...);
extern int    dFflushStdout();
extern int    dFflushStderr();

inline char dToupper(const char c) { if (c >= char('a') && c <= char('z')) return char(c + 'A' - 'a'); else return c; }
inline char dTolower(const char c) { if (c >= char('A') && c <= char('Z')) return char(c - 'A' + 'a'); else return c; }

extern bool dIsalnum(const char c);
extern bool dIsalpha(const char c);
extern bool dIsdigit(const char c);
extern bool dIsspace(const char c);
//------------------------------------------------------------------------------
// Unicode string conversions

//extern UTF8 * convertUTF16toUTF8(const UTF16 *string, UTF8 *buffer, U32 bufsize);
//extern UTF16 * convertUTF8toUTF16(const UTF8 *string, UTF16 *buffer, U32 bufsize);

//////////////////////////////////////////////////////////////////////////
/// \brief Get the first character of a UTF-8 string as a UTF-16 character
///
/// \param string UTF-8 string
/// \return First character of string as a UTF-16 character
//////////////////////////////////////////////////////////////////////////
//extern UTF16 getFirstUTF8Char(const UTF8 *string);

//////////////////////////////////////////////////////////////////////////
/// \brief Obtain a pointer to the start of the next UTF-8 character in the string
///
/// As UTF-8 characters can span multiple bytes, you can't scan the string
/// in the same way you would an ANSI string. getNextUTF8Char() is intended
///
/// This still works if ptr points to the middle of a character.
///
/// \param ptr Pointer to last character or start of string
/// \return Pointer to next UTF-8 character in the string or NULL for end of string.
//////////////////////////////////////////////////////////////////////////
//extern const UTF8 *getNextUTF8Char(const UTF8 *ptr);

//////////////////////////////////////////////////////////////////////////
/// \brief Obtain a pointer to the start of the Nth UTF-8 character in the string
///
/// As UTF-8 characters can span multiple bytes, you can't scan the string
/// in the same way you would an ANSI string. This allows you to quickly
/// skip to the appropriate place in the string.
///
/// This still works if ptr points to the middle of a character.
///
/// \param ptr Pointer to last character or start of string
/// \return Pointer to Nth UTF-8 character in the string or NULL for end of string.
//////////////////////////////////////////////////////////////////////////
//extern const UTF8 *getNextUTF8Char(const UTF8 *ptr, const U32 n);

// UNICODE is a windows platform API switching flag. Don't define it on other platforms.
#ifdef UNICODE
#define dT(s)    L##s
#else
#define dT(s)    s
#endif

//------------------------------------------------------------------------------
// Misc StdLib functions


#define QSORT_CALLBACK FN_CDECL
void dQsort(void* base, U32 nelem, U32 width, int (QSORT_CALLBACK* fcmp)(const void*, const void*));


//------------------------------------------------------------------------------
// ConsoleObject GetClassNameFn


//------------------------------------------------------------------------------
/// Memory functions
namespace Memory
{
    void flagCurrentAllocs();
    void dumpUnflaggedAllocs(const char* file);
    dsize_t getMemoryUsed();
    dsize_t getMemoryAllocated();
    void validate();
} // namespace Memory

template <class T>
inline T* constructInPlace(T* p)
{
    return new(p) T;
}

template <class T>
inline T* constructInPlace(T* p, const T* copy)
{
    return new(p) T(*copy);
}

template <class T>
inline void destructInPlace(T* p)
{
    p->~T();
}

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
extern void* FN_CDECL operator new(dsize_t size, const char*, const U32);
extern void* FN_CDECL operator new[](dsize_t size, const char*, const U32);
extern void  FN_CDECL operator delete(void* ptr);
extern void  FN_CDECL operator delete[](void* ptr);
#define new new(__FILE__, __LINE__)
#endif

#define placenew(x) new(x)
#define dMalloc(x) dMalloc_r(x, __FILE__, __LINE__)
#define dStrdup(x) dStrdup_r(x, __FILE__, __LINE__)

extern char* dStrdup_r(const char* src, const char*, dsize_t);

extern void  setBreakAlloc(dsize_t);
extern void  setMinimumAllocUnit(U32);
extern void* dMalloc_r(dsize_t in_size, const char*, const dsize_t);
extern void  dFree(void* in_pFree);
extern void* dRealloc(void* in_pResize, dsize_t in_size);
extern void* dRealMalloc(dsize_t);
extern void  dRealFree(void*);

extern void* dMemcpy(void* dst, const void* src, dsize_t size);
extern void* dMemmove(void* dst, const void* src, dsize_t size);
extern void* dMemset(void* dst, int c, dsize_t size);
extern int   dMemcmp(const void* ptr1, const void* ptr2, dsize_t size);

// Helper function to copy one array into another of different type
template<class T, class S> void dCopyArray(T* dst, const S* src, dsize_t size)
{
    for (dsize_t i = 0; i < size; i++)
        dst[i] = (T)src[i];
}

// Special case of the above function when the arrays are the same type (use memcpy)
template<class T> void dCopyArray(T* dst, const T* src, dsize_t size)
{
    dMemcpy(dst, src, size * sizeof(T));
}

//------------------------------------------------------------------------------
// Graphics functions

// Charsets for fonts

// [tom, 7/27/2005] These are intended to map to their Win32 equivalents. This
// enumeration may require changes to accommodate other platforms.
enum FontCharset
{
    TGE_ANSI_CHARSET = 0,
    TGE_SYMBOL_CHARSET,
    TGE_SHIFTJIS_CHARSET,
    TGE_HANGEUL_CHARSET,
    TGE_HANGUL_CHARSET,
    TGE_GB2312_CHARSET,
    TGE_CHINESEBIG5_CHARSET,
    TGE_OEM_CHARSET,
    TGE_JOHAB_CHARSET,
    TGE_HEBREW_CHARSET,
    TGE_ARABIC_CHARSET,
    TGE_GREEK_CHARSET,
    TGE_TURKISH_CHARSET,
    TGE_VIETNAMESE_CHARSET,
    TGE_THAI_CHARSET,
    TGE_EASTEUROPE_CHARSET,
    TGE_RUSSIAN_CHARSET,
    TGE_MAC_CHARSET,
    TGE_BALTIC_CHARSET
};

class GOldFont;
extern GOldFont* createFont(const char* name, dsize_t size, U32 charset = TGE_ANSI_CHARSET);
const char* getCharSetName(const U32 charSet);

//------------------------------------------------------------------------------
// FileIO functions
typedef void* FILE_HANDLE;
enum DFILE_STATUS
{
    DFILE_OK = 1
};

extern bool dFileDelete(const char* name);
extern bool dFileTouch(const char* name);

extern FILE_HANDLE dOpenFileRead(const char* name, DFILE_STATUS& error);
extern FILE_HANDLE dOpenFileReadWrite(const char* name, bool append, DFILE_STATUS& error);
extern int dFileRead(FILE_HANDLE handle, U32 bytes, char* dst, DFILE_STATUS& error);
extern int dFileWrite(FILE_HANDLE handle, U32 bytes, const char* dst, DFILE_STATUS& error);
extern void dFileClose(FILE_HANDLE handle);


//------------------------------------------------------------------------------
// Math
struct Math
{
    static void init(U32 properties = 0);   // 0 == detect available hardware
};


//------------------------------------------------------------------------------
// Networking
struct NetAddress;

typedef S32 NetSocket;
const NetSocket InvalidSocket = -1;

struct Net
{
    enum Error
    {
        NoError,
        WrongProtocolType,
        InvalidPacketProtocol,
        WouldBlock,
        NotASocket,
        UnknownError
    };

    enum Protocol
    {
        UDPProtocol,
        IPXProtocol,
        TCPProtocol
    };

    static bool init();
    static void shutdown();

    // Unreliable net functions (UDP)
    // sendto is for sending data
    // all incoming data comes in on packetReceiveEventType
    // App can only open one unreliable port... who needs more? ;)
    static bool openPort(S32 connectPort);
    static void closePort();
    static Error sendto(const NetAddress* address, const U8* buffer, S32 bufferSize);

    // Reliable net functions (TCP)
    // all incoming messages come in on the Connected* events
    static NetSocket openListenPort(U16 port);
    static NetSocket openConnectTo(const char* stringAddress); // does the DNS resolve etc.
    static void closeConnectTo(NetSocket socket);
    static Error sendtoSocket(NetSocket socket, const U8* buffer, S32 bufferSize);

    static void process();

    static bool compareAddresses(const NetAddress* a1, const NetAddress* a2);
    static bool stringToAddress(const char* addressString, NetAddress* address);
    static void addressToString(const NetAddress* address, char addressString[256]);

    // lower level socked based network functions
    static NetSocket openSocket();
    static Error closeSocket(NetSocket socket);

    static Error connect(NetSocket socket, const NetAddress* address);
    static Error listen(NetSocket socket, S32 maxConcurrentListens);
    static NetSocket accept(NetSocket acceptSocket, NetAddress* remoteAddress);

    static Error bind(NetSocket socket, U16    port);
    static Error setBufferSize(NetSocket socket, S32 bufferSize);
    static Error setBroadcast(NetSocket socket, bool broadcastEnable);
    static Error setBlocking(NetSocket socket, bool blockingIO);

    static Error send(NetSocket socket, const U8* buffer, S32 bufferSize);
    static Error recv(NetSocket socket, U8* buffer, S32 bufferSize, S32* bytesRead);
};

#endif


