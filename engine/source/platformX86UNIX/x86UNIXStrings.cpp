//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformX86UNIX.h"
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef HAS_VSSCANF
#  undef HAS_VSSCANF
#endif

/* this routine turns a string to all uppercase - rjp */
char *__strtoup(char *str)
{
   char *newStr = str;
   if (newStr == NULL)
      return(NULL);
   while (*newStr)
   {
      *newStr = toupper(*newStr);
      *newStr++;
   }
   return(str);
}
/* this routine turns a string to all lowercase - rjp */
char *__strtolwr(char *str)
{
   char *newStr = str;
   if (newStr == NULL)
      return(NULL);
   while (*newStr)
   {
      *newStr = tolower(*newStr);
      *newStr++;
   }
   return(str);
}

char *strtolwr(char* str)
{
   return __strtolwr(str);
}

const char *stristr(const char *szStringToBeSearched, const char *szSubstringToSearchFor)
{
   const char *pPos = NULL;
   char *szCopy1 = NULL;
   char *szCopy2 = NULL;

   // verify parameters
   if ( szStringToBeSearched == NULL ||
        szSubstringToSearchFor == NULL )
   {
      return szStringToBeSearched;
   }

   // empty substring - return input (consistent with strstr)
   if (strlen(szSubstringToSearchFor) == 0 ) {
      return szStringToBeSearched;
   }

   szCopy1 = __strtolwr(strdup(szStringToBeSearched));
   szCopy2 = __strtolwr(strdup(szSubstringToSearchFor));

   if ( szCopy1 == NULL || szCopy2 == NULL  ) {
      // another option is to raise an exception here
      free((void*)szCopy1);
      free((void*)szCopy2);
      return NULL;
   }

   pPos = strstr(szCopy1, szCopy2);

   if ( pPos != NULL ) {
      // map to the original string
      pPos = szStringToBeSearched + (pPos - szCopy1);
   }

   free((void*)szCopy1);
   free((void*)szCopy2);

   return pPos;
} // stristr(...)

char *dStrdup_r(const char *src, const char *fileName, dsize_t lineNumber)
{
   char *buffer = (char *) dMalloc_r(dStrlen(src) + 1, fileName, lineNumber);
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


S32 dStrcmp(const char *str1, const char *str2)
{
   return strcmp(str1, str2);
}

S32 dStricmp(const char *str1, const char *str2)
{
   return strcasecmp(str1, str2);
}

S32 dStrncmp(const char *str1, const char *str2, dsize_t len)
{
   return strncmp(str1, str2, len);
}

S32 dStrnicmp(const char *str1, const char *str2, dsize_t len)
{
   return strncasecmp(str1, str2, len);
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


char* dStrupr(char *str)
{
#ifdef __MWERKS__ // metrowerks strupr is broken
   _strupr(str);
   return str;
#else
   return __strtoup(str);
#endif
}


char* dStrlwr(char *str)
{
   return __strtolwr(str);
}


char* dStrchr(char *str, S32 c)
{
   return strchr(str,c);
}


const char* dStrchr(const char *str, S32 c)
{
   return strchr(str,c);
}


const char* dStrrchr(const char *str, S32 c)
{
   return strrchr(str,c);
}

char* dStrrchr(char *str, S32 c)
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

char* dStrstr(const char* str1, const char* str2)
{
    return strstr((char*)str1, str2);
}


char* dStrtok(char *str, const char *sep)
{
   return strtok(str, sep);
}


S32 dAtoi(const char *str)
{
   return atoi(str);
}

F32 dAtof(const char *str)
{
   return atof(str);
}

bool dAtob(const char *str)
{
   return !dStricmp(str, "true") || dAtof(str);
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

/*S32 dVprintf(const char *format, void *arglist)
{
   S32 len = vprintf(format, (char*)arglist);
   return (len);
}*/

S32 dSprintf(char *buffer, dsize_t bufferSize, const char *format, ...)
{
   va_list args;
   va_start(args, format);

   S32 len = vsnprintf(buffer, bufferSize, format, args);
   return (len);
}


S32 dVsprintf(char *buffer, dsize_t bufferSize, const char *format, va_list arglist)
{
   S32 len = vsnprintf(buffer, bufferSize, format, arglist);
   return (len);
}


S32 dSscanf(const char *buffer, const char *format, ...)
{
   va_list args;
#if defined(HAS_VSSCANF)
   va_start(args, format);
   return __vsscanf(buffer, format, args);
#else
   va_start(args, format);

   // Boy is this lame.  We have to scan through the format string, and find out how many
   //  arguments there are.  We'll store them off as void*, and pass them to the sscanf
   //  function through specialized calls.  We're going to have to put a cap on the number of args that
   //  can be passed, 8 for the moment.  Sigh.
   static void* sVarArgs[20];
   U32 numArgs = 0;

   for (const char* search = format; *search != '\0'; search++) {
      if (search[0] == '%' && search[1] != '%')
         numArgs++;
   }
   AssertFatal(numArgs <= 20, "Error, too many arguments to lame implementation of dSscanf.  Fix implmentation");

   // Ok, we have the number of arguments...
   for (U32 i = 0; i < numArgs; i++)
      sVarArgs[i] = va_arg(args, void*);
   va_end(args);

   switch (numArgs) {
     case 0: return 0;
     case 1:  return sscanf(buffer, format, sVarArgs[0]);
     case 2:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1]);
     case 3:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2]);
     case 4:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3]);
     case 5:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4]);
     case 6:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5]);
     case 7:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6]);
     case 8:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7]);
     case 9:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8]);
     case 10: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9]);
     case 11: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10]);
     case 12: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11]);
     case 13: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12]);
     case 14: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13]);
     case 15: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14]);
     case 16: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15]);
     case 17: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16]);
     case 18: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17]);
     case 19: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17], sVarArgs[18]);
     case 20: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17], sVarArgs[18], sVarArgs[19]);
   }
   return 0;
#endif
}

S32 dFflushStdout()
{
   return fflush(stdout);
}

S32 dFflushStderr()
{
   return fflush(stderr);
}

void dQsort(void *base, U32 nelem, U32 width, S32 (QSORT_CALLBACK *fcmp)(const void *, const void *))
{
   qsort(base, nelem, width, fcmp);
}

/// Safe form of dStrcmp: checks both strings for NULL before comparing
bool dStrEqual(const char* str1, const char* str2)
{
    if (!str1 || !str2)
        return false;
    else
        return (dStrcmp(str1, str2) == 0);
}

/// Check if one string starts with another
bool dStrStartsWith(const char* str1, const char* str2)
{
    return !dStrnicmp(str1, str2, dStrlen(str2));
}

/// Check if one string ends with another
bool dStrEndsWith(const char* str1, const char* str2)
{
    const char* p = str1 + dStrlen(str1) - dStrlen(str2);
    return ((p >= str1) && !dStricmp(p, str2));
}

/// Strip the path from the input filename
char* dStripPath(const char* filename)
{
    const char* itr = filename + dStrlen(filename);
    while (--itr != filename) {
        if (*itr == '/' || *itr == '\\') {
            itr++;
            break;
        }
    }
    return dStrdup(itr);
}

char* dChopTrailingNumber(const char* name, S32& number)
{
    // Set default return value
    number = 2;

    // Check for trivial strings
    if (!name || !name[0])
        return 0;

    // Find the number at the end of the string
    char* buffer = dStrdup(name);
    char* p = buffer + strlen(buffer) - 1;

    // Ignore trailing whitespace
    while ((p != buffer) && dIsspace(*p))
        p--;

    // Need at least one digit!
    if (!isdigit(*p))
        return buffer;

    // Back up to the first non-digit character
    while ((p != buffer) && isdigit(*p))
        p--;

    // Convert number => allow negative numbers, treat '_' as '-' for Maya
    if ((*p == '-') || (*p == '_'))
        number = -dAtoi(p + 1);
    else
        number = ((p == buffer) ? dAtoi(p) : dAtoi(++p));

    // Remove space between the name and the number
    while ((p > buffer) && dIsspace(*(p - 1)))
        p--;
    *p = '\0';

    return buffer;
}

char* dGetTrailingNumber(const char* name, S32& number)
{
    // Check for trivial strings
    if (!name || !name[0])
        return 0;

    // Find the number at the end of the string
    char* buffer = dStrdup(name);
    char* p = buffer + strlen(buffer) - 1;

    // Ignore trailing whitespace
    while ((p != buffer) && dIsspace(*p))
        p--;

    // Need at least one digit!
    if (!isdigit(*p))
        return buffer;

    // Back up to the first non-digit character
    while ((p != buffer) && isdigit(*p))
        p--;

    // Convert number => allow negative numbers, treat '_' as '-' for Maya
    if ((*p == '-') || (*p == '_'))
        number = -dAtoi(p + 1);
    else
        number = ((p == buffer) ? dAtoi(p) : dAtoi(++p));

    // Remove space between the name and the number
    while ((p > buffer) && dIsspace(*(p - 1)))
        p--;
    *p = '\0';

    return buffer;
}
