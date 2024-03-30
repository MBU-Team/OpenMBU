//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "core/resManager.h"
#include "core/fileStream.h"
#include "console/compiler.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "platform/platformInput.h"
#include "core/units.h"
#include "console/arrayObject.h"
#include <regex>
#include <ctime>

// This is a temporary hack to get tools using the library to
// link in this module which contains no other references.
bool LinkConsoleFunctions = false;

// Buffer for expanding script filenames.
static char scriptFilenameBuffer[1024];

//----------------------------------------------------------------

ConsoleFunction(IsDirectory, bool, 2, 2, "( string: directory of form \"foo/bar\", do not include trailing /, case insensitive, directory must have files in it if you expect the directory to be in a zip )")
{
    bool isDir = Platform::isDirectory(argv[1]);
    if (!isDir)
    {
        // may be in a zip, see if the resource manager knows about it
        // resource manager does not currently add directories to its dictionary, so we
        // interate through all the resources looking for files that have the target directory
        // as the path
        // WARNING: read the doc string so that you understand the limitations of this implementation
        Con::printf("IsDirectory: traversing all resources to look for directory");

        ResourceManager->startResourceTraverse();

        ResourceObject* obj = NULL;
        while (!isDir && (obj = ResourceManager->getNextResource()) != NULL)
        {
            if (obj->path != NULL && dStricmp(obj->path, argv[1]) == 0)
                isDir = true;
        }
    }
    return isDir;
}
ConsoleFunction(expandFilename, const char*, 2, 2, "(string filename)")
{
    argc;
    char* ret = Con::getReturnBuffer(1024);
    Con::expandScriptFilename(ret, 1024, argv[1]);
    return ret;
}

ConsoleFunctionGroupBegin(StringFunctions, "General string manipulation functions.");

ConsoleFunction(strasc, int, 2, 2, "(char)")
{
    argc;
    return (int)(*argv[1]);
}

// -- Formats the given value as a string, using a printf-style format string --- jason_cahill -
ConsoleFunction(strformat, const char*, 3, 3, "(string format, value)"
    "Formats the given given value as a string, given the printf-style format string.")
{
    argc;
    char* pBuffer = Con::getReturnBuffer(64);
    const char* pch = argv[1];

    pBuffer[0] = '\0';
    while (*pch != '\0' && *pch != '%')
        pch++;
    while (*pch != '\0' && !dIsalpha(*pch))
        pch++;
    if (*pch == '\0')
    {
        Con::errorf("strFormat: Invalid format string!\n");
        return pBuffer;
    }

    switch (*pch)
    {
    case 'c':
    case 'C':
    case 'd':
    case 'i':
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        dSprintf(pBuffer, 64, argv[1], dAtoi(argv[2]));
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'g':
    case 'G':
        dSprintf(pBuffer, 64, argv[1], dAtof(argv[2]));
        break;

    default:
        Con::errorf("strFormat: Invalid format string!\n");
        break;
    }

    return pBuffer;
}

ConsoleFunction(strcmp, S32, 3, 3, "(string one, string two)"
    "Case sensitive string compare.")
{
    argc;
    return dStrcmp(argv[1], argv[2]);
}

ConsoleFunction(stricmp, S32, 3, 3, "(string one, string two)"
    "Case insensitive string compare.")
{
    argc;
    return dStricmp(argv[1], argv[2]);
}

ConsoleFunction(strlen, S32, 2, 2, "(string str)"
    "Calculate the length of a string in characters.")
{
    argc;
    return dStrlen(argv[1]);
}

ConsoleFunction(strstr, S32, 3, 3, "(string one, string two) "
    "Returns the start of the sub string two in one or"
    " -1 if not found.")
{
    argc;
    // returns the start of the sub string argv[2] in argv[1]
    // or -1 if not found.

    const char* retpos = dStrstr(argv[1], argv[2]);
    if (!retpos)
        return -1;
    return retpos - argv[1];
}

ConsoleFunction(strpos, S32, 3, 4, "(string hay, string needle, int offset=0) "
    "Find needle in hay, starting offset bytes in.")
{
    S32 ret = -1;
    S32 start = 0;
    if (argc == 4)
        start = dAtoi(argv[3]);
    U32 sublen = dStrlen(argv[2]);
    U32 strlen = dStrlen(argv[1]);
    if (start < 0)
        return -1;
    if (sublen + start > strlen)
        return -1;
    for (; start + sublen <= strlen; start++)
        if (!dStrncmp(argv[1] + start, argv[2], sublen))
            return start;
    return -1;
}

ConsoleFunction(ltrim, const char*, 2, 2, "(string value)")
{
    argc;
    const char* ret = argv[1];
    while (*ret == ' ' || *ret == '\n' || *ret == '\t')
        ret++;
    return ret;
}

ConsoleFunction(rtrim, const char*, 2, 2, "(string value)")
{
    argc;
    S32 firstWhitespace = 0;
    S32 pos = 0;
    const char* str = argv[1];
    while (str[pos])
    {
        if (str[pos] != ' ' && str[pos] != '\n' && str[pos] != '\t')
            firstWhitespace = pos + 1;
        pos++;
    }
    char* ret = Con::getReturnBuffer(firstWhitespace + 1);
    dStrncpy(ret, argv[1], firstWhitespace);
    ret[firstWhitespace] = 0;
    return ret;
}

ConsoleFunction(trim, const char*, 2, 2, "(string)")
{
    argc;
    const char* ptr = argv[1];
    while (*ptr == ' ' || *ptr == '\n' || *ptr == '\t')
        ptr++;
    S32 firstWhitespace = 0;
    S32 pos = 0;
    const char* str = ptr;
    while (str[pos])
    {
        if (str[pos] != ' ' && str[pos] != '\n' && str[pos] != '\t')
            firstWhitespace = pos + 1;
        pos++;
    }
    char* ret = Con::getReturnBuffer(firstWhitespace + 1);
    dStrncpy(ret, ptr, firstWhitespace);
    ret[firstWhitespace] = 0;
    return ret;
}

ConsoleFunction(stripChars, const char*, 3, 3, "(string value, string chars) "
    "Remove all the characters in chars from value.")
{
    argc;
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);
    dStrcpy(ret, argv[1]);
    U32 pos = dStrcspn(ret, argv[2]);
    while (pos < dStrlen(ret))
    {
        dStrcpy(ret + pos, ret + pos + 1);
        pos = dStrcspn(ret, argv[2]);
    }
    return(ret);
}

ConsoleFunction(stripColorCodes, const char*, 2, 2, "(stringtoStrip) - "
    "remove TorqueML color codes from the string.")
{
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);
    dStrcpy(ret, argv[1]);
    Con::stripColorChars(ret);
    return ret;
}

ConsoleFunction(strlwr, const char*, 2, 2, "(string) "
    "Convert string to lower case.")
{
    argc;
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);
    dStrcpy(ret, argv[1]);
    return dStrlwr(ret);
}

ConsoleFunction(strupr, const char*, 2, 2, "(string) "
    "Convert string to upper case.")
{
    argc;
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);
    dStrcpy(ret, argv[1]);
    return dStrupr(ret);
}

ConsoleFunction(strchr, const char*, 3, 3, "(string,char)")
{
    argc;
    const char* ret = dStrchr(argv[1], argv[2][0]);
    return ret ? ret : "";
}

ConsoleFunction(strreplace, const char*, 4, 4, "(string source, string from, string to)")
{
    argc;
    S32 fromLen = dStrlen(argv[2]);
    if (!fromLen)
        return argv[1];

    S32 toLen = dStrlen(argv[3]);
    S32 count = 0;
    const char* scan = argv[1];
    while (scan)
    {
        scan = dStrstr(scan, argv[2]);
        if (scan)
        {
            scan += fromLen;
            count++;
        }
    }
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1 + (toLen - fromLen) * count);
    U32 scanp = 0;
    U32 dstp = 0;
    for (;;)
    {
        const char* scan = dStrstr(argv[1] + scanp, argv[2]);
        if (!scan)
        {
            dStrcpy(ret + dstp, argv[1] + scanp);
            return ret;
        }
        U32 len = scan - (argv[1] + scanp);
        dStrncpy(ret + dstp, argv[1] + scanp, len);
        dstp += len;
        dStrcpy(ret + dstp, argv[3]);
        dstp += toLen;
        scanp += len + fromLen;
    }
    return ret;
}

ConsoleFunction(getSubStr, const char*, 4, 4, "getSubStr(string str, int start, int numChars) "
    "Returns the substring of str, starting at start, and continuing "
    "to either the end of the string, or numChars characters, whichever "
    "comes first.")
{
    argc;
    // Returns the substring of argv[1], starting at argv[2], and continuing
    //  to either the end of the string, or argv[3] characters, whichever
    //  comes first.
    //
    S32 startPos = dAtoi(argv[2]);
    S32 desiredLen = dAtoi(argv[3]);
    if (startPos < 0 || desiredLen < 0) {
        Con::errorf(ConsoleLogEntry::Script, "getSubStr(...): error, starting position and desired length must be >= 0: (%d, %d)", startPos, desiredLen);

        return "";
    }

    S32 baseLen = dStrlen(argv[1]);
    if (baseLen < startPos)
        return "";

    U32 actualLen = desiredLen;
    if (startPos + desiredLen > baseLen)
        actualLen = baseLen - startPos;

    char* ret = Con::getReturnBuffer(actualLen + 1);
    dStrncpy(ret, argv[1] + startPos, actualLen);
    ret[actualLen] = '\0';

    return ret;
}

// Used?
ConsoleFunction(stripTrailingSpaces, const char*, 2, 2, "stripTrailingSpaces( string )")
{
    argc;
    S32 temp = S32(dStrlen(argv[1]));
    if (temp)
    {
        while ((argv[1][temp - 1] == ' ' || argv[1][temp - 1] == '_') && temp >= 1)
            temp--;

        if (temp)
        {
            char* returnString = Con::getReturnBuffer(temp + 1);
            dStrncpy(returnString, argv[1], U32(temp));
            returnString[temp] = '\0';
            return(returnString);
        }
    }

    return("");
}

ConsoleFunctionGroupEnd(StringFunctions);

//--------------------------------------
ConsoleFunctionGroupBegin(FieldManipulators, "Functions to manipulate data returned in the form of \"x y z\".");

ConsoleFunction(getWord, const char*, 3, 3, "(string text, int index)")
{
    argc;
    return getUnit(argv[1], dAtoi(argv[2]), " \t\n");
}

ConsoleFunction(getWords, const char*, 3, 4, "(string text, int index, int endIndex=INF)")
{
    U32 endIndex;
    if (argc == 3)
        endIndex = 1000000;
    else
        endIndex = dAtoi(argv[3]);
    return getUnits(argv[1], dAtoi(argv[2]), endIndex, " \t\n");
}

ConsoleFunction(setWord, const char*, 4, 4, "newText = setWord(text, index, replace)")
{
    argc;
    return setUnit(argv[1], dAtoi(argv[2]), argv[3], " \t\n");
}

ConsoleFunction(removeWord, const char*, 3, 3, "newText = removeWord(text, index)")
{
    argc;
    return removeUnit(argv[1], dAtoi(argv[2]), " \t\n");
}

ConsoleFunction(getWordCount, S32, 2, 2, "getWordCount(text)")
{
    argc;
    return getUnitCount(argv[1], " \t\n");
}

ConsoleFunction(findWord, S32, 3, 3, "findWord(text, word)")
{
    argc;
    return findUnit(argv[1], argv[2], " \t\n");
}

//--------------------------------------
ConsoleFunction(getField, const char*, 3, 3, "getField(text, index)")
{
    argc;
    return getUnit(argv[1], dAtoi(argv[2]), "\t\n");
}

ConsoleFunction(getFields, const char*, 3, 4, "getFields(text, index [,endIndex])")
{
    U32 endIndex;
    if (argc == 3)
        endIndex = 1000000;
    else
        endIndex = dAtoi(argv[3]);
    return getUnits(argv[1], dAtoi(argv[2]), endIndex, "\t\n");
}

ConsoleFunction(setField, const char*, 4, 4, "newText = setField(text, index, replace)")
{
    argc;
    return setUnit(argv[1], dAtoi(argv[2]), argv[3], "\t\n");
}

ConsoleFunction(removeField, const char*, 3, 3, "newText = removeField(text, index)")
{
    argc;
    return removeUnit(argv[1], dAtoi(argv[2]), "\t\n");
}

ConsoleFunction(getFieldCount, S32, 2, 2, "getFieldCount(text)")
{
    argc;
    return getUnitCount(argv[1], "\t\n");
}

ConsoleFunction(findField, S32, 3, 3, "findField(text, field)")
{
    argc;
    return findUnit(argv[1], argv[2], "\t\n");
}

//--------------------------------------
ConsoleFunction(getRecord, const char*, 3, 3, "getRecord(text, index)")
{
    argc;
    return getUnit(argv[1], dAtoi(argv[2]), "\n");
}

ConsoleFunction(getRecords, const char*, 3, 4, "getRecords(text, index [,endIndex])")
{
    U32 endIndex;
    if (argc == 3)
        endIndex = 1000000;
    else
        endIndex = dAtoi(argv[3]);
    return getUnits(argv[1], dAtoi(argv[2]), endIndex, "\n");
}

ConsoleFunction(setRecord, const char*, 4, 4, "newText = setRecord(text, index, replace)")
{
    argc;
    return setUnit(argv[1], dAtoi(argv[2]), argv[3], "\n");
}

ConsoleFunction(removeRecord, const char*, 3, 3, "newText = removeRecord(text, index)")
{
    argc;
    return removeUnit(argv[1], dAtoi(argv[2]), "\n");
}

ConsoleFunction(getRecordCount, S32, 2, 2, "getRecordCount(text)")
{
    argc;
    return getUnitCount(argv[1], "\n");
}
//--------------------------------------
ConsoleFunction(firstWord, const char*, 2, 2, "firstWord(text)")
{
    argc;
    const char* word = dStrchr(argv[1], ' ');
    U32 len;
    if (word == NULL)
        len = dStrlen(argv[1]);
    else
        len = word - argv[1];
    char* ret = Con::getReturnBuffer(len + 1);
    dStrncpy(ret, argv[1], len);
    ret[len] = 0;
    return ret;
}

ConsoleFunction(restWords, const char*, 2, 2, "restWords(text)")
{
    argc;
    const char* word = dStrchr(argv[1], ' ');
    if (word == NULL)
        return "";
    char* ret = Con::getReturnBuffer(dStrlen(word + 1) + 1);
    dStrcpy(ret, word + 1);
    return ret;
}

static bool isInSet(char c, const char* set)
{
    if (set)
        while (*set)
            if (c == *set++)
                return true;

    return false;
}

ConsoleFunction(NextToken, const char*, 4, 4, "nextToken(str,token,delim)")
{
    argc;

    char* str = (char*)argv[1];
    const char* token = argv[2];
    const char* delim = argv[3];

    if (str)
    {
        // skip over any characters that are a member of delim
        // no need for special '\0' check since it can never be in delim
        while (isInSet(*str, delim))
            str++;

        // skip over any characters that are NOT a member of delim
        const char* tmp = str;

        while (*str && !isInSet(*str, delim))
            str++;

        // terminate the token
        if (*str)
            *str++ = 0;

        // set local variable if inside a function
        if (gEvalState.stack.size() &&
            gEvalState.stack.last()->scopeName)
            Con::setLocalVariable(token, tmp);
        else
            Con::setVariable(token, tmp);

        // advance str past the 'delim space'
        while (isInSet(*str, delim))
            str++;
    }

    return str;
}

ConsoleFunctionGroupEnd(FieldManipulators)
//----------------------------------------------------------------

ConsoleFunctionGroupBegin(TaggedStrings, "Functions dealing with tagging/detagging strings.");

ConsoleFunction(detag, const char*, 2, 2, "detag(textTagString)")
{
    argc;
    if (argv[1][0] == StringTagPrefixByte)
    {
        const char* word = dStrchr(argv[1], ' ');
        if (word == NULL)
            return "";
        char* ret = Con::getReturnBuffer(dStrlen(word + 1) + 1);
        dStrcpy(ret, word + 1);
        return ret;
    }
    else
        return argv[1];
}

ConsoleFunction(getTag, const char*, 2, 2, "getTag(textTagString)")
{
    argc;
    if (argv[1][0] == StringTagPrefixByte)
    {
        const char* space = dStrchr(argv[1], ' ');

        U32 len;
        if (space)
            len = space - argv[1];
        else
            len = dStrlen(argv[1]) + 1;

        char* ret = Con::getReturnBuffer(len);
        dStrncpy(ret, argv[1] + 1, len - 1);
        ret[len - 1] = 0;

        return(ret);
    }
    else
        return(argv[1]);
}

ConsoleFunctionGroupEnd(TaggedStrings);

//----------------------------------------------------------------

ConsoleFunctionGroupBegin(Output, "Functions to output to the console.");

ConsoleFunction(echo, void, 2, 0, "echo(text [, ... ])")
{
    U32 len = 0;
    S32 i;
    for (i = 1; i < argc; i++)
        len += dStrlen(argv[i]);

    char* ret = Con::getReturnBuffer(len + 1);
    ret[0] = 0;
    for (i = 1; i < argc; i++)
        dStrcat(ret, argv[i]);

    Con::printf("%s", ret);
    ret[0] = 0;
}

ConsoleFunction(warn, void, 2, 0, "warn(text [, ... ])")
{
    U32 len = 0;
    S32 i;
    for (i = 1; i < argc; i++)
        len += dStrlen(argv[i]);

    char* ret = Con::getReturnBuffer(len + 1);
    ret[0] = 0;
    for (i = 1; i < argc; i++)
        dStrcat(ret, argv[i]);

    Con::warnf(ConsoleLogEntry::General, "%s", ret);
    ret[0] = 0;
}

ConsoleFunction(error, void, 2, 0, "error(text [, ... ])")
{
    U32 len = 0;
    S32 i;
    for (i = 1; i < argc; i++)
        len += dStrlen(argv[i]);

    char* ret = Con::getReturnBuffer(len + 1);
    ret[0] = 0;
    for (i = 1; i < argc; i++)
        dStrcat(ret, argv[i]);

    Con::errorf(ConsoleLogEntry::General, "%s", ret);
    ret[0] = 0;
}

ConsoleFunction(expandEscape, const char*, 2, 2, "expandEscape(text)")
{
    argc;
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) * 2 + 1);  // worst case situation
    expandEscape(ret, argv[1]);
    return ret;
}

ConsoleFunction(collapseEscape, const char*, 2, 2, "collapseEscape(text)")
{
    argc;
    char* ret = Con::getReturnBuffer(dStrlen(argv[1]) + 1);  // worst case situation
    dStrcpy(ret, argv[1]);
    collapseEscape(ret);
    return ret;
}

ConsoleFunction(setLogMode, void, 2, 2, "setLogMode(mode);")
{
    argc;
    Con::setLogMode(dAtoi(argv[1]));
}

ConsoleFunction(setEchoFileLoads, void, 2, 2, "setEchoFileLoads(bool);")
{
    argc;
    ResourceManager->setFileNameEcho(dAtob(argv[1]));
}

ConsoleFunctionGroupEnd(Output);

//----------------------------------------------------------------

ConsoleFunction(quit, void, 1, 1, "quit() End execution of Torque.")
{
    argc; argv;
    Platform::postQuitMessage(0);
}

ConsoleFunction(quitWithErrorMessage, void, 2, 2, "quitWithErrorMessage(msg)"
    " - Quit, showing the provided error message. This is equivalent"
    " to an AssertISV.")
{
    AssertISV(false, argv[1]);
}

//----------------------------------------------------------------

ConsoleFunction(gotoWebPage, void, 2, 2, "( address ) - Open a web page in the user's favorite web browser.")
{
    argc;
    Platform::openWebBrowser(argv[1]);
}

//----------------------------------------------------------------

ConsoleFunctionGroupBegin(MetaScripting, "Functions that let you manipulate the scripting engine programmatically.");

ConsoleFunction(call, const char*, 2, 0, "call(funcName [,args ...])")
{
    return Con::execute(argc - 1, argv + 1);
}

static U32 execDepth = 0;
static U32 journalDepth = 1;

ConsoleFunction(compile, bool, 2, 2, "compile(fileName)")
{
    argc;
    char nameBuffer[512];
    char* script = NULL;
    U32 scriptSize = 0;

    Stream* compiledStream = NULL;
    FileTime comModifyTime, scrModifyTime;

    Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]);

    // If the script file extention is '.ed.cs' then compile it to a different compiled extention
    const char* ext = dStrchr(scriptFilenameBuffer, '.');

    if (ext != NULL && (dStricmp(ext, ".ed.cs") == 0) || (dStricmp(ext, ".ed.gui") == 0))
        dSprintf(nameBuffer, sizeof(nameBuffer), "%s.edso", scriptFilenameBuffer);
    else
        dSprintf(nameBuffer, sizeof(nameBuffer), "%s.dso", scriptFilenameBuffer);

    ResourceObject* rScr = ResourceManager->find(scriptFilenameBuffer);
    ResourceObject* rCom = ResourceManager->find(nameBuffer);

    if (rCom)
        rCom->getFileTimes(NULL, &comModifyTime);
    if (rScr)
        rScr->getFileTimes(NULL, &scrModifyTime);

    Stream* s = ResourceManager->openStream(scriptFilenameBuffer);
    if (s)
    {
        scriptSize = ResourceManager->getSize(scriptFilenameBuffer);
        script = new char[scriptSize + 1];
        s->read(scriptSize, script);
        ResourceManager->closeStream(s);
        script[scriptSize] = 0;
    }

    if (!scriptSize || !script)
    {
        delete[] script;
        Con::errorf(ConsoleLogEntry::Script, "compile: invalid script file %s.", scriptFilenameBuffer);
        return false;
    }
    // compile this baddie.
    Con::printf("Compiling %s...", scriptFilenameBuffer);
    CodeBlock* code = new CodeBlock();
    code->compile(nameBuffer, scriptFilenameBuffer, script);
    delete code;
    code = NULL;

    delete[] script;
    return true;
}

ConsoleFunction(exec, bool, 2, 4, "exec(fileName [, nocalls [,journalScript]])")
{
    bool journal = false;

    execDepth++;
    if (journalDepth >= execDepth)
        journalDepth = execDepth + 1;
    else
        journal = true;

    bool noCalls = false;
    bool ret = false;

    if (argc >= 3 && dAtoi(argv[2]))
        noCalls = true;

    if (argc >= 4 && dAtoi(argv[3]) && !journal)
    {
        journal = true;
        journalDepth = execDepth;
    }

    // Determine the filename we actually want...
    Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]);

    const char* ext = dStrrchr(scriptFilenameBuffer, '.');

    if (!ext)
    {
        // We need an extension!
        Con::errorf(ConsoleLogEntry::Script, "exec: invalid script file name %s.", scriptFilenameBuffer);
        execDepth--;
        return false;
    }

    // Check Editor Extensions
    bool isEditorScript = false;

    // If the script file extention is '.ed.cs' then compile it to a different compiled extention
    const char* edExt = dStrchr(scriptFilenameBuffer, '.');
    if (edExt != NULL && (dStricmp(edExt, ".ed.cs") == 0) || (dStricmp(edExt, ".ed.gui") == 0))
        isEditorScript = true;


    StringTableEntry scriptFileName = StringTable->insert(scriptFilenameBuffer);

    // Is this a file we should compile?
    bool compiled = dStricmp(ext, ".mis") && !journal && !Con::getBoolVariable("Pref::ignoreDSOs");

    // If we're in a journalling mode, then we will read the script
    // from the journal file.
    if (journal && Game->isJournalReading())
    {
        char fileNameBuf[256];
        bool fileRead;
        U32 fileSize;

        Game->getJournalStream()->readString(fileNameBuf);
        Game->getJournalStream()->read(&fileRead);
        if (!fileRead)
        {
            Con::errorf(ConsoleLogEntry::Script, "Journal script read (failed) for %s", fileNameBuf);
            execDepth--;
            return false;
        }
        Game->journalRead(&fileSize);
        char* script = new char[fileSize + 1];
        Game->journalRead(fileSize, script);
        script[fileSize] = 0;
        Con::printf("Executing (journal-read) %s.", scriptFileName);
        CodeBlock* newCodeBlock = new CodeBlock();
        newCodeBlock->compileExec(scriptFileName, script, noCalls, 0);
        delete[] script;

        execDepth--;
        return true;
    }

    // Ok, we let's try to load and compile the script.
    ResourceObject* rScr = ResourceManager->find(scriptFileName);
    ResourceObject* rCom = NULL;

    char nameBuffer[512];
    char* script = NULL;
    U32 scriptSize = 0;
    U32 version;

    Stream* compiledStream = NULL;
    FileTime comModifyTime, scrModifyTime;

    // Check here for .edso
    bool edso = false;
    if (dStricmp(ext, ".edso") == 0)
    {
        edso = true;
        rCom = rScr;
        rScr = NULL;

        rCom->getFileTimes(NULL, &comModifyTime);
        dStrcpy(nameBuffer, scriptFileName);
    }

    // If we're supposed to be compiling this file, check to see if there's a DSO
    if (compiled && !edso)
    {
        if (isEditorScript)
        {
            dStrcpyl(nameBuffer, sizeof(nameBuffer), scriptFileName, ".edso", NULL);
            rCom = ResourceManager->find(nameBuffer);
        }
        else
        {
            dStrcpyl(nameBuffer, sizeof(nameBuffer), scriptFileName, ".dso", NULL);
            rCom = ResourceManager->find(nameBuffer);
        }

        if (rCom)
            rCom->getFileTimes(NULL, &comModifyTime);
        if (rScr)
            rScr->getFileTimes(NULL, &scrModifyTime);
    }

    // Let's do a sanity check to complain about DSOs in the future.
    //
    // MM:	This doesn't seem to be working correctly for now so let's just not issue
    //		the warning until someone knows how to resolve it.
    //
    //if(compiled && rCom && rScr && Platform::compareFileTimes(comModifyTime, scrModifyTime) < 0)
    //{
       //Con::warnf("exec: Warning! Found a DSO from the future! (%s)", nameBuffer);
    //}

    // If we had a DSO, let's check to see if we should be reading from it.
    if (compiled && rCom && (!rScr || Platform::compareFileTimes(comModifyTime, scrModifyTime) >= 0))
    {
        compiledStream = ResourceManager->openStream(nameBuffer);

        // Check the version!
        compiledStream->read(&version);
        if (version != Con::DSOVersion)
        {
            Con::warnf("exec: Found an old DSO (%s, ver %d < %d), ignoring.", nameBuffer, version, Con::DSOVersion);
            ResourceManager->closeStream(compiledStream);
            compiledStream = NULL;
        }
    }

    // If we're journalling, let's write some info out.
    if (journal && Game->isJournalWriting())
        Game->getJournalStream()->writeString(scriptFileName);

    if (rScr && !compiledStream)
    {
        // If we have source but no compiled version, then we need to compile
        // (and journal as we do so, if that's required).

        Stream* s = ResourceManager->openStream(scriptFileName);
        if (journal && Game->isJournalWriting())
            Game->getJournalStream()->write(bool(s != NULL));

        if (s)
        {
            scriptSize = ResourceManager->getSize(scriptFileName);
            script = new char[scriptSize + 1];
            s->read(scriptSize, script);

            if (journal && Game->isJournalWriting())
            {
                Game->journalWrite(scriptSize);
                Game->journalWrite(scriptSize, script);
            }
            ResourceManager->closeStream(s);
            script[scriptSize] = 0;
        }

        if (!scriptSize || !script)
        {
            delete[] script;
            Con::errorf(ConsoleLogEntry::Script, "exec: invalid script file %s.", scriptFileName);
            execDepth--;
            return false;
        }

#ifndef TORQUE_NO_DSO_GENERATION
        if (compiled)
        {
            // compile this baddie.
            Con::printf("Compiling %s...", scriptFileName);
            CodeBlock* code = new CodeBlock();
            code->compile(nameBuffer, scriptFileName, script);
            delete code;
            code = NULL;

            compiledStream = ResourceManager->openStream(nameBuffer);
            if (compiledStream)
            {
                compiledStream->read(&version);
            }
            else
            {
                // We have to exit out here, as otherwise we get double error reports.
                delete[] script;
                execDepth--;
                return false;
            }
        }
#endif
    }
    else
    {
        if (journal && Game->isJournalWriting())
            Game->getJournalStream()->write(bool(false));
    }

    if (compiledStream)
    {
        // Delete the script object first to limit memory used
        // during recursive execs.
        delete[] script;
        script = 0;

        // We're all compiled, so let's run it.
        Con::printf("Loading compiled script %s.", scriptFileName);
        CodeBlock* code = new CodeBlock;
        code->read(scriptFileName, *compiledStream);
        ResourceManager->closeStream(compiledStream);
        code->exec(0, scriptFileName, NULL, 0, NULL, noCalls, NULL, 0);
        ret = true;
    }
    else
        if (rScr)
        {
            // No compiled script,  let's just try executing it
            // directly... this is either a mission file, or maybe
            // we're on a readonly volume.
            Con::printf("Executing %s.", scriptFileName);
            CodeBlock* newCodeBlock = new CodeBlock();
            StringTableEntry name = StringTable->insert(scriptFileName);

            newCodeBlock->compileExec(name, script, noCalls, 0);
            ret = true;
        }
        else
        {
            // Don't have anything.
            Con::warnf(ConsoleLogEntry::Script, "Missing file: %s!", scriptFileName);
            ret = false;
        }

    delete[] script;
    execDepth--;
    return ret;
}

ConsoleFunction(eval, const char*, 2, 2, "eval(consoleString)")
{
    argc;
    return Con::evaluate(argv[1], false, NULL);
}

ConsoleFunction(isDefined, bool, 2, 2, "isDefined(variable name [, value if not defined])")
{
    if (dStrlen(argv[1]) == 0)
    {
        Con::errorf("isDefined() - did you forget to put quotes around the variable name?");
        return false;
    }

    StringTableEntry name = StringTable->insert(argv[1]);

    // Deal with <var>.<value>
    if (dStrchr(name, '.'))
    {
        static char scratchBuffer[4096];

        S32 len = dStrlen(name);
        AssertFatal(len < sizeof(scratchBuffer) - 1, "isDefined() - name too long");
        dMemcpy(scratchBuffer, name, len + 1);

        char* token = dStrtok(scratchBuffer, ".");

        if (!token || token[0] == '\0')
            return false;

        StringTableEntry objName = StringTable->insert(token);

        // Attempt to find the object
        SimObject* obj = Sim::findObject(objName);

        // If we didn't find the object then we can safely
        // assume that the field variable doesn't exist
        if (!obj)
            return false;

        // Get the name of the field
        token = dStrtok(0, ".\0");
        if (!token)
            return false;

        while (token != NULL)
        {
            StringTableEntry valName = StringTable->insert(token);

            // Store these so we can restore them after we search for the variable
            bool saveModStatic = obj->canModStaticFields();
            bool saveModDyn = obj->canModDynamicFields();

            // Set this so that we can search both static and dynamic fields
            obj->setModStaticFields(true);
            obj->setModDynamicFields(true);

            const char* value = obj->getDataField(valName, 0);

            // Restore our mod flags to be safe
            obj->setModStaticFields(saveModStatic);
            obj->setModDynamicFields(saveModDyn);

            if (!value)
            {
                obj->setDataField(valName, 0, argv[2]);

                return false;
            }
            else
            {
                // See if we are field on a field
                token = dStrtok(0, ".\0");
                if (token)
                {
                    // The previous field must be an object
                    obj = Sim::findObject(value);
                    if (!obj)
                        return false;
                }
                else
                {
                    if (dStrlen(value) > 0)
                        return true;
                    else if (argc > 2)
                        obj->setDataField(valName, 0, argv[2]);
                }
            }
        }
    }
    else if (name[0] == '%')
    {
        // Look up a local variable
        if (gEvalState.stack.size())
        {
            Dictionary::Entry* ent = gEvalState.stack.last()->lookup(name);

            if (ent)
                return true;
            else if (argc > 2)
                gEvalState.stack.last()->setVariable(name, argv[2]);
        }
        else
            Con::errorf("%s() - no local variable frame.", __FUNCTION__);
    }
    else if (name[0] == '$')
    {
        // Look up a global value
        Dictionary::Entry* ent = gEvalState.globalVars.lookup(name);

        if (ent)
            return true;
        else if (argc > 2)
            gEvalState.globalVars.setVariable(name, argv[2]);
    }
    else
    {
        // Is it an object?
        if (dStrcmp(argv[1], "0") && dStrcmp(argv[1], "") && (Sim::findObject(argv[1]) != NULL))
            return true;
        else if (argc > 2)
            Con::errorf("%s() - can't assign a value to a variable of the form \"%s\"", __FUNCTION__, argv[1]);
    }

    return false;
}

//----------------------------------------------------------------

ConsoleFunction(getPrefsPath, const char*, 1, 2, "([relativeFileName])")
{
    const char* filename = Platform::getPrefsPath(argc > 1 ? argv[1] : NULL);
    if (filename == NULL || *filename == 0)
        return "";

    return filename;
}

ConsoleFunction(execPrefs, bool, 2, 4, "execPrefs(relativeFileName [, nocalls [,journalScript]])")
{
    const char* filename = Platform::getPrefsPath(argv[1]);
    if (filename == NULL || *filename == 0)
        return false;

    // Scripts do this a lot, so we may as well help them out
    // FIXME [tom, 11/17/2006] This should use the resource manager in addition to file system check
    if (!Platform::isFile(filename) && !ResourceManager->find(filename))
        return true;

    argv[0] = "exec";
    argv[1] = filename;
    return dAtob(Con::execute(argc, argv));
}

ConsoleFunction(export, void, 2, 4, "export(searchString [, relativeFileName [,append]])")
{
    const char* filename = NULL;
    bool append = (argc == 4) ? dAtob(argv[3]) : false;

    if (argc >= 3)
    {
#ifdef TORQUE_PLAYER
        if (Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[2]))
            filename = scriptFilenameBuffer;
#else
        filename = Platform::getPrefsPath(argv[2]);
        if (filename == NULL || *filename == 0)
            return;

        gAllowExternalWrite = true;
#endif
    }

    gEvalState.globalVars.exportVariables(argv[1], filename, append);

    gAllowExternalWrite = false;
}

ConsoleFunction(deletePrefs, void, 2, 2, "deletePrefs(relativeFileName)")
{
    const char* filename = Platform::getPrefsPath(argv[1]);
    if (filename == NULL || *filename == 0)
        return;

    gAllowExternalWrite = true;

    dFileDelete(filename);

    gAllowExternalWrite = false;
}

ConsoleFunction(deleteVariables, void, 2, 2, "deleteVariables(wildCard)")
{
    argc;
    gEvalState.globalVars.deleteVariables(argv[1]);
}

ConsoleFunction(deleteEmptyVariables, void, 2, 2, "deleteVariables(wildCard)")
{
    argc;
    gEvalState.globalVars.deleteVariables(argv[1], true);
}

ConsoleFunction(variablesExist, bool, 2, 2, "variablesExist(wildCard)")
{
    argc;
    return gEvalState.globalVars.variablesExist(argv[1]);
}

//----------------------------------------------------------------

ConsoleFunction(trace, void, 2, 2, "trace(bool)")
{
    argc;
    gEvalState.traceOn = dAtob(argv[1]);
    Con::printf("Console trace is %s", gEvalState.traceOn ? "on." : "off.");
}

//----------------------------------------------------------------

#if defined(TORQUE_DEBUG) || defined(INTERNAL_RELEASE)
ConsoleFunction(debug, void, 1, 1, "debug()")
{
    argv; argc;
    Platform::debugBreak();
}
#endif

ConsoleFunctionGroupEnd(MetaScripting);

//----------------------------------------------------------------

ConsoleFunctionGroupBegin(InputManagement, "Functions that let you deal with input from scripts");

ConsoleFunction(deactivateDirectInput, void, 1, 1, "Deactivate input. (ie, ungrab the mouse so the user can do other things.")
{
    argc; argv;
    if (Input::isActive())
        Input::deactivate();
}

//--------------------------------------------------------------------------
ConsoleFunction(activateDirectInput, void, 1, 1, "Activate input. (ie, grab the mouse again so the user can play our game.")
{
    argc; argv;
    if (!Input::isActive())
        Input::activate();
}

ConsoleFunctionGroupEnd(InputManagement);

//----------------------------------------------------------------

ConsoleFunctionGroupBegin(FileSystem, "Functions allowing you to search for files, read them, write them, and access their properties.");

static ResourceObject* firstMatch = NULL;

ConsoleFunction(findFirstFile, const char*, 2, 2, "(string pattern) Returns the first file in the directory system matching the given pattern.")
{
    argc;
    const char* fn;
    firstMatch = NULL;
    if (Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]))
        firstMatch = ResourceManager->findMatch(scriptFilenameBuffer, &fn, NULL);
    if (firstMatch)
        return fn;
    else
        return "";
}

ConsoleFunction(findNextFile, const char*, 2, 2, "(string pattern) Returns the next file matching a search begun in findFirstFile.")
{
    argc;
    const char* fn;
    if (Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]))
        firstMatch = ResourceManager->findMatch(scriptFilenameBuffer, &fn, firstMatch);
    else
        firstMatch = NULL;
    if (firstMatch)
        return fn;
    else
        return "";
}

ConsoleFunction(getFileCount, S32, 2, 2, "(string pattern)returns the number of files in the directory tree that match the given pattern")
{
    argc;
    const char* fn;
    U32 count = 0;
    firstMatch = ResourceManager->findMatch(argv[1], &fn, NULL);
    if (firstMatch)
    {
        count++;
        while (1)
        {
            firstMatch = ResourceManager->findMatch(argv[1], &fn, firstMatch);
            if (firstMatch)
                count++;
            else
                break;
        }
    }

    return(count);
}

ConsoleFunction(getFileCRC, S32, 2, 2, "getFileCRC(filename)")
{
    argc;
    U32 crcVal;
    Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]);

    if (!ResourceManager->getCrc(scriptFilenameBuffer, crcVal))
        return(-1);
    return(S32(crcVal));
}

ConsoleFunction(isFile, bool, 2, 2, "isFile(fileName)")
{
    argc;
    Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]);
    return bool(ResourceManager->find(scriptFilenameBuffer));
}

ConsoleFunction(isWriteableFileName, bool, 2, 2, "isWriteableFileName(fileName)")
{
    argc;
    // in a writeable directory?
    Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), argv[1]);
    if (!ResourceManager->isValidWriteFileName(scriptFilenameBuffer))
        return(false);

    // exists?
    FileStream fs;
    if (!fs.open(scriptFilenameBuffer, FileStream::Read))
        return(true);

    // writeable? (ReadWrite will create file if it does not exist)
    fs.close();
    if (!fs.open(scriptFilenameBuffer, FileStream::ReadWrite))
        return(false);

    return(true);
}

//----------------------------------------------------------------

ConsoleFunction(fileExt, const char*, 2, 2, "fileExt(fileName)")
{
    argc;
    const char* ret = dStrrchr(argv[1], '.');
    if (ret)
        return ret;
    return "";
}

ConsoleFunction(fileBase, const char*, 2, 2, "fileBase(fileName)")
{
    argc;
    const char* path = dStrrchr(argv[1], '/');
    if (!path)
        path = argv[1];
    else
        path++;
    char* ret = Con::getReturnBuffer(dStrlen(path) + 1);
    dStrcpy(ret, path);
    char* ext = dStrrchr(ret, '.');
    if (ext)
        *ext = 0;
    return ret;
}

ConsoleFunction(fileName, const char*, 2, 2, "fileName(filePathName)")
{
    argc;
    const char* name = dStrrchr(argv[1], '/');
    if (!name)
        name = argv[1];
    else
        name++;
    char* ret = Con::getReturnBuffer(dStrlen(name));
    dStrcpy(ret, name);
    return ret;
}

ConsoleFunction(filePath, const char*, 2, 2, "filePath(fileName)")
{
    argc;
    const char* path = dStrrchr(argv[1], '/');
    if (!path)
        return "";
    U32 len = path - argv[1];
    char* ret = Con::getReturnBuffer(len + 1);
    dStrncpy(ret, argv[1], len);
    ret[len] = 0;
    return ret;
}

ConsoleFunctionGroupEnd(FileSystem);
ConsoleFunction(isspace, bool, 3, 3, "(string, index): return true if character at specified index in string is whitespace")
{
    S32 idx = dAtoi(argv[2]);
    if (idx >= 0 && idx < dStrlen(argv[1]))
        return dIsspace(argv[1][idx]);
    else
        return false;
}

ConsoleFunction(isalnum, bool, 3, 3, "(string, index): return true if character at specified index in string is alnum")
{
    S32 idx = dAtoi(argv[2]);
    if (idx >= 0 && idx < dStrlen(argv[1]))
        return dIsalnum(argv[1][idx]);
    else
        return false;
}

ConsoleFunction(startswith, bool, 3, 3, "(src string, target string) case insensitive")
{
    const char* src = argv[1];
    const char* target = argv[2];

    // if the target string is empty, return true (all strings start with the empty string)
    S32 srcLen = dStrlen(src);
    S32 targetLen = dStrlen(target);
    if (targetLen == 0)
        return true;
    // else if the src string is empty, return false (empty src does not start with non-empty target)
    else if (srcLen == 0)
        return false;

    // both src and target are non empty, create temp buffers for lowercase operation
    char* srcBuf = new char[srcLen + 1];
    char* targetBuf = new char[targetLen + 1];

    // copy src and target into buffers
    dStrcpy(srcBuf, src);
    dStrcpy(targetBuf, target);

    // reassign src/target pointers to lowercase versions
    src = dStrlwr(srcBuf);
    target = dStrlwr(targetBuf);

    // do the comparison
    bool startsWith = dStrncmp(src, target, targetLen) == 0;

    // delete temp buffers
    delete[] srcBuf;
    delete[] targetBuf;

    return startsWith;
}

ConsoleFunction(endswith, bool, 3, 3, "(src string, target string) case insensitive")
{
    const char* src = argv[1];
    const char* target = argv[2];

    // if the target string is empty, return true (all strings end with the empty string)
    S32 srcLen = dStrlen(src);
    S32 targetLen = dStrlen(target);
    if (targetLen == 0)
        return true;
    // else if the src string is empty, return false (empty src does not end with non-empty target)
    else if (srcLen == 0)
        return false;

    // both src and target are non empty, create temp buffers for lowercase operation
    char* srcBuf = new char[srcLen + 1];
    char* targetBuf = new char[targetLen + 1];

    // copy src and target into buffers
    dStrcpy(srcBuf, src);
    dStrcpy(targetBuf, target);

    // reassign src/target pointers to lowercase versions
    src = dStrlwr(srcBuf);
    target = dStrlwr(targetBuf);

    // set the src pointer to the appropriate place to check the end of the string
    src += srcLen - targetLen;

    // do the comparison
    bool endsWith = dStrcmp(src, target) == 0;

    // delete temp buffers
    delete[] srcBuf;
    delete[] targetBuf;

    return endsWith;
}

ConsoleFunction(strrchrpos, S32, 3, 3, "strrchrpos(string,char)")
{
    argc;
    const char* ret = dStrrchr(argv[1], argv[2][0]);
    return ret ? ret - argv[1] : -1;
}

ConsoleFunction(strrchr, const char*, 3, 3, "strrchr(string,char)")
{
    argc;
    const char* ret = dStrrchr(argv[1], argv[2][0]);
    return ret ? ret : "";
}

ConsoleFunction(strswiz, const char*, 3, 3, "strswiz(string,len)")
{
    argc;
    S32 lenIn = dStrlen(argv[1]);
    S32 lenOut = getMin(dAtoi(argv[2]), lenIn);
    char* ret = Con::getReturnBuffer(lenOut + 1);
    for (S32 i = 0; i < lenOut; i++)
    {
        if (i & 1)
            ret[i] = argv[1][i >> 1];
        else
            ret[i] = argv[1][lenIn - (i >> 1) - 1];
    }
    ret[lenOut] = '\0';
    return ret;
}

ConsoleFunction(countBits, S32, 2, 2, "count the number of bits in the specified 32 bit integer")
{
    S32 c = 0;
    S32 v = dAtoi(argv[1]);

    // from 
    // http://graphics.stanford.edu/~seander/bithacks.html

    // for at most 32-bit values in v:
    c = ((v & 0xfff) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;
    c += (((v & 0xfff000) >> 12) * 0x1001001001001ULL & 0x84210842108421ULL) %
        0x1f;
    c += ((v >> 24) * 0x1001001001001ULL & 0x84210842108421ULL) % 0x1f;

#ifndef TORQUE_SHIPPING
    // since the above isn't very obvious, for debugging compute the count in a more 
    // traditional way and assert if it is different
    {
        S32 c2 = 0;
        S32 v2 = v;
        for (c2 = 0; v2; v2 >>= 1)
        {
            c2 += v2 & 1;
        }
        if (c2 != c)
            Con::errorf("countBits: Uh oh bit count mismatch");
        AssertFatal(c2 == c, "countBits: uh oh, bit count mismatch");
    }
#endif

    return c;
}

ConsoleFunction(isShippingBuild, bool, 1, 1, "Returns true if this is a shipping build, false otherwise")
{
#ifdef TORQUE_SHIPPING
    return true;
#else
    return false;
#endif
}

ConsoleFunction(regexMatch, bool, 3, 4, "regexMatch(testString, pattern [, matches]);") {
    std::string testString(argv[1]);
    try {
        std::regex regex(argv[2]);
        if (argc > 3) {
            SimObject* potentialArrayObject = Sim::findObject(argv[3]);
            ArrayObject* result = dynamic_cast<ArrayObject*>(potentialArrayObject);

            std::smatch matches;
            bool found = std::regex_match(testString, matches, regex);

            if (result != nullptr) {
                for (const std::string& match : matches) {
                    result->addEntry(match);
                }
            }

            return found;
        }
        else {
            return std::regex_match(testString, regex);
        }
    }
    catch (std::regex_error e) {
        Con::errorf("regexMatch: %s", e.what());
        return false;
    }
}

ConsoleFunction(regexReplace, const char*, 4, 4, "regexMatch(testString, pattern, replacement);") {
    std::string testString(argv[1]);
    std::string replacement(argv[3]);

    try {
        std::regex regex(argv[2]);
        std::string replaced = std::regex_replace(testString, regex, replacement);

        char* ret = Con::getReturnBuffer(replaced.length() + 1);
        dStrcpy(ret, replaced.data());

        return ret;
    }
    catch (std::regex_error e) {
        Con::errorf("regexReplace: %s", e.what());
        return argv[1];
    }
}

ConsoleFunction(getDayNum, const char*, 1, 1, "getDayNum();") {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char* ret = Con::getReturnBuffer(20);
    dSprintf(ret, 20, "%d", timeinfo->tm_mday);
    return ret;
}

ConsoleFunction(getMonthNum, const char*, 1, 1, "getMonthNum();") {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char* ret = Con::getReturnBuffer(20);
    dSprintf(ret, 20, "%d", (timeinfo->tm_mon) + 1);
    return ret;
}

ConsoleFunction(getYearNum, const char*, 1, 1, "getYearNum();") {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char* ret = Con::getReturnBuffer(20);
    dSprintf(ret, 20, "%d", (timeinfo->tm_year) + 1900);
    return ret;
}
