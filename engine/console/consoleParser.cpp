//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleParser.h"

namespace Compiler
{

static ConsoleParser *gParserList = NULL;
static ConsoleParser *gDefaultParser = NULL;

void freeConsoleParserList(void)
{
	ConsoleParser *pParser;

	while(pParser = gParserList)
	{
		gParserList = pParser->next;
		delete pParser;
	}

	gDefaultParser = NULL;
}

bool addConsoleParser(char *ext, fnGetCurrentFile gcf, fnGetCurrentLine gcl, fnParse p, fnRestart r, fnSetScanBuffer ssb, bool def /* = false */)
{
	AssertFatal(ext && gcf && gcl && p && r, "AddConsoleParser called with one or more NULL arguments");

	ConsoleParser *pParser;
	if(pParser = new ConsoleParser)
	{
		pParser->ext = ext;
		pParser->getCurrentFile = gcf;
		pParser->getCurrentLine = gcl;
		pParser->parse = p;
		pParser->restart = r;
		pParser->setScanBuffer = ssb;

		if(def)
			gDefaultParser = pParser;

		pParser->next = gParserList;
		gParserList = pParser;

		return true;
	}
	return false;
}

ConsoleParser * getParserForFile(const char *filename)
{
	char *ptr;

	if(filename == NULL)
		return gDefaultParser;

	if(ptr = dStrrchr((char *)filename, '.'))
	{
		ptr++;

		ConsoleParser *p;
		for(p = gParserList;p;p = p->next)
		{
			if(dStricmp(ptr, p->ext) == 0)
				return p;
		}
	}

	return gDefaultParser;
}

} // end namespace Con
