
/*******************************************************************
 *
 *    DESCRIPTION:      Basic string definitions header file
 *
 *    AUTHOR:           Tom Hudson
 *
 *    HISTORY:          File created 9/6/94
 *
 *******************************************************************/

#ifndef _STRBASICS_
#define _STRBASICS_

#define WIN95STUFF

// To set up Jaguar to use Unicode, define _UNICODE, and don't define _MBCS
// To set up Jaguar to use multi-byte character sets, define _MBCS and 
//              don't define _UNICODE
// To set up Jaguar to use single-byte ANSI character strings, don't define
//              either _UNICODE or _MBCS

// #define _UNICODE     // turn on Unicode support

#ifndef _MBCS
#define _MBCS   // if Unicode is off, turn on multi-byte character support
#endif


#ifdef _UNICODE

#ifdef _MBCS
#undef _MBCS    // can't have both Unicode and MBCS at once -- Unicode wins
#endif
#define UNICODE
#define STRCONST L
//#define RWSTR RWWString
//#define RWTOKENIZER RWWTokenizer
// Here's a macro to get a const char * from a RWWString object -- It
// temporarily constructs a RWCString object to hold the 1-byte wide
// character string output by the toAcsii() operator.  Don't store
// this pointer!  Copy it to a new allocation, because it might go
// away.
#define NARROW(s) ((const char *)((s).toAscii()))

#else

//#define RWSTR RWCString
//#define RWTOKENIZER RWCTokenizer
//#define NARROW(s) (s)

#endif

// Bring in the generic international text header file
#include <tchar.h>

#ifdef __cplusplus

// Bring in the Rogue Wave regular and wide string classes
// These classes will help us avoid problems in dealing with strings.
// Use the RWWString class to store all strings used in Jaguar -- The
// RWWString class allows unlimited length strings, so we will avoid
// the problems of string truncation or overwriting memory.
// NOTE: No RogueWave classes should be used in plugin API's !!
//#include <rw\cstring.h>
//#include <rw\wstring.h>
 
// Simple string class, can be used in plugin API's 
#endif // __cplusplus

#endif // _STRBASICS_
