//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/profiler.h"

// An explanation of the following bit twiddling:
//----------------------------------------------------------------
//  UTF8 encodes characters with a varying # of bytes.
//  first byte of a UTF8 char is either or
//  0x(xxxxxx)
//  11(xxxxxx)
//
//  0xxx xxxx & 0xC0 = 0000 0000 or 0100 0000 = 0x00 or 0x40
//  11xx xxxx & 0xC0 = 1100 0000              = 0xC0
//
//  all middle bytes of UTF8 chars start with
//  10(xxxxxx)
//
//  10xx xxxx & 0xC0 = 1000 0000 = 0x80
//
//  so, a bitwise and against 0xC0 will detect whether we are in the middle of a UTF8 char or not.
//
//  see: http://en.wikipedia.org/wiki/UTF8 for a good clear explanation of UTF8
/*
UTF16 getFirstUTF8Char(const UTF8 *string)
{
    if(string == NULL || *string == 0)
        return 0;

    PROFILE_START(getFirstUTF8Char);

    if(*string & 0xc0 == 0x80)
    {
        // We're in the middle of a character, find the next one
        string = getNextUTF8Char(string);
    }

    UTF8 s[5];
    const UTF8 *ptr;

    // Get first UTF-8 char
    for(ptr = string+1;(*ptr & 0xc0) == 0x80 && *ptr;ptr++)
    {
    }

    dStrncpy((char *)s, (const char *)string, ptr - string);
    s[ptr - string] = 0;

    UTF16 buf[3];
    convertUTF8toUTF16(s, buf, sizeof(buf));

    PROFILE_END();

    return buf[0];
}

const UTF8 *getNextUTF8Char(const UTF8 *ptr)
{
    while(*ptr)
    {
        ptr++;
        if((*ptr & 0xc0) != 0x80)
            return ptr;
    }

    return NULL;
}

const UTF8 *getNextUTF8Char(const UTF8 *ptr, const U32 n)
{
   for(U32 i=0; i<n && ptr; i++)
      ptr = getNextUTF8Char(ptr);

   return ptr;
}
*/
//-------------------------------------------------------------------------

const char* getCharSetName(const U32 charSet)
{
    switch (charSet)
    {
    case TGE_ANSI_CHARSET:        return "ansi";
    case TGE_SYMBOL_CHARSET:      return "symbol";
    case TGE_SHIFTJIS_CHARSET:    return "shiftjis";
    case TGE_HANGEUL_CHARSET:     return "hangeul";
    case TGE_HANGUL_CHARSET:      return "hangul";
    case TGE_GB2312_CHARSET:      return "gb2312";
    case TGE_CHINESEBIG5_CHARSET: return "chinesebig5";
    case TGE_OEM_CHARSET:         return "oem";
    case TGE_JOHAB_CHARSET:       return "johab";
    case TGE_HEBREW_CHARSET:      return "hebrew";
    case TGE_ARABIC_CHARSET:      return "arabic";
    case TGE_GREEK_CHARSET:       return "greek";
    case TGE_TURKISH_CHARSET:     return "turkish";
    case TGE_VIETNAMESE_CHARSET:  return "vietnamese";
    case TGE_THAI_CHARSET:        return "thai";
    case TGE_EASTEUROPE_CHARSET:  return "easteurope";
    case TGE_RUSSIAN_CHARSET:     return "russian";
    case TGE_MAC_CHARSET:         return "mac";
    case TGE_BALTIC_CHARSET:      return "baltic";
    }

    AssertISV(false, "getCharSetName - unknown charset! Update table in platformString.cc!");
    return "";
}
