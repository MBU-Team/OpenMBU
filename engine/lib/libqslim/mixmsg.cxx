/************************************************************************

  Implementation of error reporting and notification facilities.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: mixmsg.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "stdmix.h"
//#include "mixio.h"
#include "mixmsg.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//static ostream& error_stream = cerr;

static MxSeverityLevel current_severity =
#if (SAFETY >= 0)
	MXMSG_NOTE;
#else
	MXMSG_WARN;
#endif

static MxSeverityLevel current_lethality = MXMSG_ASSERT;

static uint current_indent = 0;

static mxmsg_handler current_handler = mxmsg_default_handler;


static void prefix()
{
//    for(uint i=0; i<current_indent; i++)
//	error_stream << "    ";
}

void mxmsg_signal(MxSeverityLevel severity,
		  const char *msg, const char *context,
		  const char *filename, int line)
{
    if( severity <= current_severity )
    {
	MxMsgInfo info;

	info.severity = severity;
	info.message = msg;
	info.context = context;
	info.filename = filename;
	info.line = line;

	bool result = (*current_handler)(&info);

	if( !result )
	{
//	    cerr << "MXMSG PANIC: Error while reporting signal!" << endl;
	    exit(1);
	}

	if( severity <= current_lethality )
	{
	    if( severity == MXMSG_ASSERT )  abort();
	    else                            exit(1);
	}
    }
}

void mxmsg_signalf(MxSeverityLevel severity, const char *format, ...)
{
    va_list args;

    // !!BUG: It would be nice not to rely on a fixed-size buffer here.
    char msg[512];

    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    mxmsg_signal(severity, msg);
}


bool mxmsg_default_handler(MxMsgInfo *info)
{
/*    prefix();
    error_stream << mxmsg_severity_name(info->severity) << ": ";
    error_stream << info->message << endl;
    if( info->context )
    {
	prefix();
	error_stream << "  [Location: " << info->context << "]" << endl;
    }
    if( info->filename )
    {
	prefix();
	error_stream << "  [File: " << info->filename
		     << " at line " << info->line << "]" << endl;
    }
*/
    return true;
}

void mxmsg_set_handler(mxmsg_handler h)
{
    current_handler = h;
}

MxSeverityLevel mxmsg_lethality_level() { return current_lethality; }
void mxmsg_lethality_level(MxSeverityLevel l) { current_lethality = l; }

MxSeverityLevel mxmsg_severity_level() { return current_severity; }
void mxmsg_severity_level(MxSeverityLevel l) { current_severity = l; }

static char *severity_names[] =
{
    "FATAL ERROR",
    "ASSERT",
    "ERROR",
    "WARNING",
    "NOTE",
    "DEBUG",
    "TRACE"
};

const char *mxmsg_severity_name(MxSeverityLevel l)
{
    if( l <= MXMSG_TRACE )
	return severity_names[l];
    else
	return "USER";
}

void mxmsg_indent(uint i) { current_indent += i; }
void mxmsg_dedent(uint i) { current_indent -= i; }
