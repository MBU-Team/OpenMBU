%{
#define YYLMAX 4096

#include <stdio.h>
#include "platform/platform.h"
#include "core/stringTable.h"
#include "console/console.h"
#define _CMDGRAM_H_
#include "console/compiler.h"
#include "console/basgram.h"

using namespace Compiler;

#define YY_NEVER_INTERACTIVE 1

// Some basic parsing primitives...
static int Sc_ScanString(int ret);
static int Sc_ScanNum();
static int Sc_ScanVar();
static int Sc_ScanHex();

// Deal with debuggability of FLEX.
#ifdef TORQUE_DEBUG
#define FLEX_DEBUG 1
#else
#define FLEX_DEBUG 0
#endif

//#undef input
//#undef unput
#undef BASgetc
int BASgetc();

#define YY_INPUT(buf,result,max_size) \
{ \
   int c = '*', n; \
   for ( n = 0; n < max_size && \
           (c = BASgetc()) != EOF && c != '\n'; ++n ) \
      buf[n] = (char) c; \
   if ( c == '\n' ) \
      buf[n++] = (char) c; \
   result = n; \
}

static int lineIndex;

// Prototypes
void BASSetScanBuffer(const char *sb, const char *fn);
void BASerror(char * s, ...);

// Error reporting
void CMDerror(char * s, ...);

// Reset the parser.
void CMDrestart(FILE *in);

%}

DIGIT	[0-9]
INTEGER	{DIGIT}+
FLOAT	({INTEGER}\.{INTEGER})|({INTEGER}(\.{INTEGER})?[eE][+-]?{INTEGER})
LETTER	[A-Za-z_]
FILECHAR [A-Za-z_\.]
VARMID	[:A-Za-z0-9_]
IDTAIL	[A-Za-z0-9_]
VARTAIL  {VARMID}*{IDTAIL}
VAR	[$%]{LETTER}{VARTAIL}*
ID	{LETTER}{IDTAIL}*
ILID [$%]{DIGIT}+{LETTER}{VARTAIL}*
FILENAME {FILECHAR}+
SPACE	[ \t\v\f]
HEXDIGIT [a-fA-F0-9]

%%
			;
{SPACE}+	{ }
"//"[^\n\r]*	;
"rem"{SPACE}+[^\n\r]*	;
[\r]        ;
[\n]        { lineIndex++; }
\"(\\.|[^\\"\n\r])*\"		{ return(Sc_ScanString(STRATOM)); }
\'(\\.|[^\\'\n\r])*\'		{ return(Sc_ScanString(TAGATOM)); }
"=="        return(BASlval.i = opEQ);
"!="        return(BASlval.i = opNE);
">="        return(BASlval.i = opGE);
"<="        return(BASlval.i = opLE);
"&&"        return(BASlval.i = opAND);
"||"        return(BASlval.i = opOR);
"::"        return(BASlval.i = opCOLONCOLON);
"--"        return(BASlval.i = opMINUSMINUS);
"++"        return(BASlval.i = opPLUSPLUS);
"$="        return(BASlval.i = opSTREQ);
"!$="       return(BASlval.i = opSTRNE);
"<<"        return(BASlval.i = opSHL);
">>"        return(BASlval.i = opSHR);
"+="        return(BASlval.i = opPLASN);
"-="        return(BASlval.i = opMIASN);
"*="        return(BASlval.i = opMLASN);
"/="        return(BASlval.i = opDVASN);
"%="        return(BASlval.i = opMODASN);
"&="        return(BASlval.i = opANDASN);
"^="        return(BASlval.i = opXORASN);
"|="        return(BASlval.i = opORASN);
"<<="       return(BASlval.i = opSLASN);
">>="       return(BASlval.i = opSRASN);
"NL"        {BASlval.i = '\n'; return '@'; }
"TAB"       {BASlval.i = '\t'; return '@'; }
"SPC"       {BASlval.i = ' '; return '@'; }
"@"         {BASlval.i = 0; return '@'; }
"?" |
"[" |
"]" |
"(" |
")" |
"+" |
"-" |
"*" |
"/" |
"<" |
">" |
"|" |
"." |
"!" |
":" |
";" |
"{" |
"}" |
"," |
"&" |
"%" |
"^" |
"~" |
"=" {       return(BASlval.i = BAStext[0]); }
"or"        { BASlval.i = lineIndex; return(rwCASEOR); }
"break"     { BASlval.i = lineIndex; return(rwBREAK); }
"return"    { BASlval.i = lineIndex; return(rwRETURN); }
"else"      { BASlval.i = lineIndex; return(rwELSE); }
"while"     { BASlval.i = lineIndex; return(rwWHILE); }
"if"        { BASlval.i = lineIndex; return(rwIF); }
"then"      { BASlval.i = lineIndex; return(rwTHEN); }
"do"        { BASlval.i = lineIndex; return(rwBEGIN); }
"begin"     { BASlval.i = lineIndex; return(rwBEGIN); }
"end"       { BASlval.i = lineIndex; return(rwEND); }
"for"       { BASlval.i = lineIndex; return(rwFOR); }
"cfor"      { BASlval.i = lineIndex; return(rwCFOR); }
"to"        { BASlval.i = lineIndex; return(rwTO); }
"step"      { BASlval.i = lineIndex; return(rwSTEP); }
"continue"  { BASlval.i = lineIndex; return(rwCONTINUE); }
"function"  { BASlval.i = lineIndex; return(rwDEFINE); }
"sub"       { BASlval.i = lineIndex; return(rwDEFINE); }
"new"       { BASlval.i = lineIndex; return(rwDECLARE); }
"datablock" { BASlval.i = lineIndex; return(rwDATABLOCK); }
"case"      { BASlval.i = lineIndex; return(rwCASE); }
"switch$"   { BASlval.i = lineIndex; return(rwSWITCHSTR); }
"switch"    { BASlval.i = lineIndex; return(rwSWITCH); }
"default"   { BASlval.i = lineIndex; return(rwDEFAULT); }
"package"   { BASlval.i = lineIndex; return(rwPACKAGE); }
"true"      { BASlval.i = 1; return INTCONST; }
"false"      { BASlval.i = 0; return INTCONST; }
{VAR}       return(Sc_ScanVar());
{ID}        { BAStext[BASleng] = 0; BASlval.s = StringTable->insert(BAStext); return(IDENT); }
0[xX]{HEXDIGIT}+ return(Sc_ScanHex());
{INTEGER}   { BAStext[BASleng] = 0; BASlval.i = atoi(BAStext); return INTCONST; }
{FLOAT}     return Sc_ScanNum();
{ILID}      return(ILLEGAL_TOKEN);
.           return(ILLEGAL_TOKEN);
%%

/*
 * Scan character constant.
 */

/*
 * Scan identifier.
 */

static const char *scanBuffer;
static const char *fileName;
static int scanIndex;

const char * BASGetCurrentFile()
{
   return fileName;
}

int BASGetCurrentLine()
{
   return lineIndex;
}

void BASerror(char *, ...)
{
   gSyntaxError = true;
   if(fileName)
      Con::errorf(ConsoleLogEntry::Script, "%s Line: %d - Syntax error.",
         fileName, lineIndex);
   else
      Con::errorf(ConsoleLogEntry::Script, "Syntax error in input.");
}

void BASSetScanBuffer(const char *sb, const char *fn)
{
   scanBuffer = sb;
   fileName = fn;
   scanIndex = 0;
   lineIndex = 1;
}

int BASgetc()
{
   int ret = scanBuffer[scanIndex];
   if(ret)
      scanIndex++;
   else
      ret = -1;
   return ret;
}

int BASwrap()
{
   return 1;
}

static int Sc_ScanVar()
{
   BAStext[BASleng] = 0;
	BASlval.s = StringTable->insert(BAStext);
	return(VAR);
}
/*
 * Scan string constant.
 */

// Defined in CS_CMD.l
extern void expandEscape(char *dest, const char *src);
extern bool collapseEscape(char *buf);

static int charConv(int in)
{
   switch(in)
   {
      case 'r':
         return '\r';
      case 'n':
         return '\n';
      case 't':
         return '\t';
      default:
         return in;
   }
}

static int getHexDigit(char c)
{
   if(c >= '0' && c <= '9')
      return c - '0';
   if(c >= 'A' && c <= 'F')
      return c - 'A' + 10;
   if(c >= 'a' && c <= 'f')
      return c - 'a' + 10;
   return -1;
}

static int Sc_ScanString(int ret)
{
	BAStext[BASleng - 1] = 0;
   if(!collapseEscape(BAStext+1))
      return -1;
	BASlval.str = (char *) consoleAlloc(dStrlen(BAStext));
   dStrcpy(BASlval.str, BAStext + 1);
	return(ret);
}


static int Sc_ScanNum()
{
   BAStext[BASleng] = 0;
	BASlval.f = atof(BAStext);
	return(FLTCONST);
}

static int Sc_ScanHex()
{
   int val = 0;
   dSscanf(BAStext, "%x", &val);
   BASlval.i = val;
   return INTCONST;
}

