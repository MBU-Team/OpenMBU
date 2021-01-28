//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformMacCarb/platformMacCarb.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include "console/console.h"
#include "platform/profiler.h"

char *dStrdup_r(const char *src, const char *file, dsize_t line)
{
   char *buffer = (char *) dMalloc_r(dStrlen(src) + 1, file, line);
   dStrcpy(buffer, src);
   return buffer;
}

char *dStrnew(const char *src)
{
   char *buffer = new char[dStrlen(src) + 1];
   dStrcpy(buffer, src);
   return buffer;
}

char* dStrcat(char *dst, const char *src)
{
   return strcat(dst,src);
}   

char* dStrncat(char *dst, const char *src, dsize_t len)
{
   return strncat(dst,src,len);
}

// concatenates a list of src's onto the end of dst
// the list of src's MUST be terminated by a NULL parameter
// dStrcatl(dst, sizeof(dst), src1, src2, NULL);
char* dStrcatl(char *dst, dsize_t dstSize, ...)
{
   const char* src;
   char *p = dst;

   AssertFatal(dstSize > 0, "dStrcatl: destination size is set zero");
   dstSize--;  // leave room for string termination

   // find end of dst
   while (dstSize && *p++)                    
      dstSize--;   
   
   va_list args;
   va_start(args, dstSize);

   // concatenate each src to end of dst
   while ( (src = va_arg(args, const char*)) != NULL )
      while( dstSize && *src )
      {
         *p++ = *src++;
         dstSize--;   
      }

   va_end(args);

   // make sure the string is terminated 
   *p = 0;

   return dst;
}   


// copy a list of src's into dst
// the list of src's MUST be terminated by a NULL parameter
// dStrccpyl(dst, sizeof(dst), src1, src2, NULL);
char* dStrcpyl(char *dst, dsize_t dstSize, ...)
{
   const char* src;
   char *p = dst;

   AssertFatal(dstSize > 0, "dStrcpyl: destination size is set zero");
   dstSize--;  // leave room for string termination

   va_list args;
   va_start(args, dstSize);

   // concatenate each src to end of dst
   while ( (src = va_arg(args, const char*)) != NULL )
      while( dstSize && *src )
      {
         *p++ = *src++;
         dstSize--;   
      }

   va_end(args);

   // make sure the string is terminated 
   *p = 0;

   return dst;
}   


int dStrcmp(const char *str1, const char *str2)
{
   return strcmp(str1, str2);   
}

int dStrcmp( const UTF16 *str1, const UTF16 *str2)
{
   int ret;
   const UTF16 *a, *b;
   a = str1;
   b = str2;

   while( *a && *b && (ret = *a - *b) == 0)
      a++, b++;

   return ret;
}  

 
int dStricmp(const char *str1, const char *str2)
{
   char c1, c2;
   while (1)
   {
      c1 = tolower(*str1++);
      c2 = tolower(*str2++);
      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
      if (c1 == 0) return 0;
   }
}  

int dStrncmp(const char *str1, const char *str2, dsize_t len)
{
   return strncmp(str1, str2, len);   
}  
 
int dStrnicmp(const char *str1, const char *str2, dsize_t len)
{
   S32 i;
   char c1, c2;
   for (i=0; i<len; i++)
   {
      c1 = tolower(*str1++);
      c2 = tolower(*str2++);
      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
      if (!c1) return 0;
     }
   return 0;
}   


char* dStrcpy(char *dst, const char *src)
{
   return strcpy(dst,src);
}   

char* dStrncpy(char *dst, const char *src, dsize_t len)
{
   return strncpy(dst,src,len);
}   

dsize_t dStrlen(const char *str)
{
   return strlen(str);
}   

/*dsize_t dStrlenUTF8_fast(const UTF8 *str)
{
   // we do not run this through the operating system, because this method may be faster.
   int c = 0;
   for(; str; str = getNextUTF8Char(str))
      c++;

   return c;
}
*/
/*dsize_t dStrlenUTF8(const UTF8 *str)
{
   CFStringRef cfstr;
   U32         cfstrlen;
   
   if(!str)
      return 0;
      
   cfstr = CFStringCreateWithCString( kCFAllocatorDefault, (char*)str, kCFStringEncodingUTF8);
   if(!cfstr)
      return 0;
   cfstrlen = CFStringGetLength(cfstr);
   CFRelease(cfstr);
   
   return cfstrlen;

}
*/
/*dsize_t dStrlen(const UTF16 *str)
{
   int i=0;
   while(str[i] != 0x0000) i++;
   
   return i;

   // slower than just counting wchars, but makes sure surrogate pairs are dealt with correctly.
   CFStringRef cfstr;
   U32         cfstrlen;
   
   cfstr = CFStringCreateWithCString(kCFAllocatorDefault, (char*)str, kCFStringEncodingUnicode);
   cfstrlen = CFStringGetLength(cfstr);
   CFRelease(cfstr);
   
   return cfstrlen;
}
*/

char* dStrupr(char *str)
{
   char* saveStr = str;
   while (*str)
   {
      *str = toupper(*str);
      str++;
   }
   return saveStr;
}   


char* dStrlwr(char *str)
{
   char* saveStr = str;
   while (*str)
   {
      *str = tolower(*str);
      str++;
   }
   return saveStr;
}   


char* dStrchr(char *str, int c)
{
   return strchr(str,c);
}   


const char* dStrchr(const char *str, int c)
{
   return strchr(str,c);
}   

const char* dStrrchr(const char *str, int c)
{
   return strrchr(str,c);
}   


char* dStrrchr(char *str, int c)
{
   return strrchr(str,c);
}   

dsize_t dStrspn(const char *str, const char *set)
{
   return(strspn(str, set));
}

dsize_t dStrcspn(const char *str, const char *set)
{
   return strcspn(str, set);
}   


char* dStrstr(char *str1, char *str2)
{
   return strstr(str1,str2);
}

char* dStrstr(const char *str1, const char *str2)
{
   return strstr(str1,str2);
}   

char* dStrtok(char *str, const char *sep)
{
   return strtok(str, sep);
}


int dAtoi(const char *str)
{
   return atoi(str);   
}  

 
float dAtof(const char *str)
{
   return atof(str);   
}   

bool dAtob(const char *str)
{
   return !dStricmp(str, "true") || !dStricmp(str, "1") || (0!=dAtoi(str));
}   

bool dIsalnum(const char c)
{
   return isalnum(c);
}

bool dIsalpha(const char c)
{
   return(isalpha(c));
}

bool dIsspace(const char c)
{
   return(isspace(c));
}

bool dIsdigit(const char c)
{
   return(isdigit(c));
}

void dPrintf(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vprintf(format, args);
}   

int dVprintf(const char *format, void *arglist)
{
   S32 len = vprintf(format, (char*)arglist);

   // to-do
   // The intended behavior is to zero-terminate and not allow the overflow
   return (len);
}   

int dSprintf(char *buffer, dsize_t /*bufferSize*/, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   S32 len = vsprintf(buffer, format, args);

#warning "need to added zero-term + overflow handling"
   // to-do
   // The intended behavior is to zero-terminate and not allow the overflow
   return (len);
}   


int dVsprintf(char *buffer, dsize_t /*bufferSize*/, const char *format, void *arglist)
{
   S32 len = vsprintf(buffer, format, (char*)arglist);

#warning "need to added zero-term + overflow handling"
   // to-do
   // The intended behavior is to zero-terminate and not allow the overflow
   return (len);
}   


int dSscanf(const char *buffer, const char *format, ...)
{
   va_list args;
   va_start(args, format);
//   return __vsscanf(buffer, format, args);   
   return vsscanf(buffer, format, args);   
}   

int dFflushStdout()
{
   return fflush(stdout);
}

int dFflushStderr()
{
   return fflush(stderr);
}

void dQsort(void *base, U32 nelem, U32 width, int (QSORT_CALLBACK *fcmp)(const void *, const void *))
{
   qsort(base, nelem, width, fcmp);
}   


//---------------------------------------------------------------------------
// Mac Strinng conversion routines
U8* str2p(const char *str)
{
   static U8 buffer[256];
   str2p(str, buffer);
   return buffer;
}


U8* str2p(const char *str, U8 *p)
{
   AssertFatal(dStrlen(str) <= 255, "str2p:: Max Pascal String length exceeded (max=255).");
   U8 *dst = p+1;
   U8 *src = (U8*)str;
   *p = 0;
   while(*src != '\0')
   {
      *dst++ = *src++;
      (*p) += 1;
   }
   return p;
}


char* p2str(U8 *p)
{
   static char buffer[256];
   p2str(p, buffer);
   return buffer;
}


char* p2str(U8 *p, char *dst_str)
{
   U8 len = *p++;
   char *src = (char*)p;
   char *dst = dst_str;
   while (len--)
   {
      *dst++ = *src++;
   }
   *dst = '\0';
   return dst_str;
}

//------------------------------------------------------------------------------
// Unicode string conversions
//------------------------------------------------------------------------------
/*UTF8 * convertUTF16toUTF8(const UTF16 *string, UTF8 *buffer, U32 bufsize)
{
   PROFILE_START(convertUTF16toUTF8);
   // use a CFString to convert the unicode string.
   // CFStrings use a UTF16 based backing store.
   // the last kCFAllocatorNull parameter is a deallocator for the string passed in.
   //  we pass the Null allocator so that string will not be klobbered.
   // note: on 10.4+, another constant, kCFStringEncodingUTF16 is available, and
   //  it is an alias for kCFStringEncodingUnicode.
   //  see: http://developer.apple.com/documentation/CoreFoundation/Reference/CFStringRef/Reference/chapter_2.2_section_1.html 
   bool        ok;
   CFStringRef cfstr;
   
   U32 len = dStrlen(string);
   cfstr = CFStringCreateWithCharacters( kCFAllocatorDefault, string, len);
   //cfstr = CFStringCreateWithCString(kCFAllocatorDefault, (const char*)string, kCFStringEncodingUnicode);
   AssertFatal(cfstr != NULL, "Unicode string conversion failed - couldn't make the CFString!");

   ok = CFStringGetCString( cfstr, (char*)buffer, bufsize, kCFStringEncodingUTF8);
   CFRelease(cfstr);
   
   AssertWarn( ok, "Unicode string conversion failed in convertUTF16toUTF8()");
   if( !ok ) {
      PROFILE_END();
      return NULL;
   }
      
   buffer[bufsize-1] = 0x00;
   
   PROFILE_END();
   return buffer;
}


//------------------------------------------------------------------------------
UTF16 * convertUTF8toUTF16(const UTF8 *string, UTF16 *buffer, U32 bufsize)
{
   // see above notes in convertUTF16toUTF8() for an explanation of the following.
   // what we do here is exactly the same process as above, but in reverse.
   PROFILE_START(convertUTF8toUTF16);
   bool        ok;
   CFStringRef cfstr;   
   if( !string || string[0] == '\0' )
      goto returnNull;
      
   cfstr = CFStringCreateWithCString(kCFAllocatorDefault, (char*)string, kCFStringEncodingUTF8);
   if(!cfstr)
      goto returnNull;
   
   ok = CFStringGetCString( cfstr, (char*)buffer, bufsize*sizeof(UTF16), kCFStringEncodingUnicode);
   CFRelease(cfstr);
   
   AssertWarn( ok, "Unicode string conversion failed or buffer was too small in convertUTF8toUTF16()");
   if( !ok )
      goto returnNull;
         
   buffer[bufsize-1] = 0x000;

   PROFILE_END();
   return buffer;

returnNull:
   PROFILE_END();
   return NULL;
}
*/
