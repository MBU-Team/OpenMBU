//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMFONT_H_
#include "platform/platformFont.h"
#endif

#ifndef _WINFONT_H_
#define _WINFONT_H_

class WinFont : public PlatformFont
{
private:
    HFONT mFont;
    TEXTMETRIC mTextMetric;
    
public:
    WinFont();
    virtual ~WinFont();
    
    // PlatformFont virtual methods
    virtual bool isValidChar(const UTF16 ch) const;
    virtual bool isValidChar(const UTF8 *str) const;

    inline virtual U32 getFontHeight() const
    {
        return mTextMetric.tmHeight;
    }

    inline virtual U32 getFontBaseLine() const
    {
        return mTextMetric.tmAscent;
    }

    virtual PlatformFont::CharInfo &getCharInfo(const UTF16 ch) const;
    virtual PlatformFont::CharInfo &getCharInfo(const UTF8 *str) const;

    virtual bool create(const char *name, dsize_t size, U32 charset = TGE_ANSI_CHARSET);
};

#endif // _WINFONT_H_
