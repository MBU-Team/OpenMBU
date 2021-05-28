#ifndef MIXMSG_INCLUDED // -*- C++ -*-
#define MIXMSG_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  MixKit error reporting and message notification support.

  This module is meant to provide a small but reasonably flexible way of
  reporting errors and other interesting events to the user.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: mixmsg.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

//
// Message severity levels
//
enum MxSeverityLevel
{
    MXMSG_FATAL = 0,
    MXMSG_ASSERT,
    MXMSG_ERROR,
    MXMSG_WARN,
    MXMSG_NOTE,
    MXMSG_DEBUG,
    MXMSG_TRACE
};

class MxMsgInfo
{
public:
    MxSeverityLevel severity;
    const char *message;
    const char *context;
    const char *filename;
    int line;
};

typedef bool (*mxmsg_handler)(MxMsgInfo *);

extern void mxmsg_signal(MxSeverityLevel severity,
			 const char *msg, const char *context=NULL,
			 const char *filename=NULL,
			 int line=0);
extern void mxmsg_signalf(MxSeverityLevel severity, const char *format, ...);


extern bool mxmsg_default_handler(MxMsgInfo *);
extern void mxmsg_set_handler(mxmsg_handler h=mxmsg_default_handler);

extern MxSeverityLevel mxmsg_severity_level();
extern void mxmsg_severity_level(MxSeverityLevel);
extern const char *mxmsg_severity_name(MxSeverityLevel);

extern MxSeverityLevel mxmsg_lethality_level();
extern void mxmsg_lethality_level(MxSeverityLevel);

extern void mxmsg_indent(uint i=1);
extern void mxmsg_dedent(uint i=1);

// MIXMSG_INCLUDED
#endif
