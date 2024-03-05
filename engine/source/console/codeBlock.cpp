//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/console.h"
#include "console/compiler.h"
#include "console/codeBlock.h"
#include "console/telnetDebugger.h"
#include "core/resManager.h"
#include "core/unicode.h"

using namespace Compiler;

bool           CodeBlock::smInFunction = false;
U32            CodeBlock::smBreakLineCount = 0;
CodeBlock* CodeBlock::smCodeBlockList = NULL;
CodeBlock* CodeBlock::smCurrentCodeBlock = NULL;
ConsoleParser* CodeBlock::smCurrentParser = NULL;

//-------------------------------------------------------------------------

CodeBlock::CodeBlock()
{
    globalStrings = NULL;
    functionStrings = NULL;
    globalFloats = NULL;
    functionFloats = NULL;
    lineBreakPairs = NULL;
    breakList = NULL;
    breakListSize = 0;

    refCount = 0;
    code = NULL;
    name = NULL;
    mRoot = StringTable->insert("");
}

CodeBlock::~CodeBlock()
{
    // Make sure we aren't lingering in the current code block...
    AssertFatal(smCurrentCodeBlock != this, "CodeBlock::~CodeBlock - Caught lingering in smCurrentCodeBlock!")

        if (name)
            removeFromCodeList();
    delete[] const_cast<char*>(globalStrings);
    delete[] const_cast<char*>(functionStrings);
    delete[] globalFloats;
    delete[] functionFloats;
    delete[] code;
    delete[] breakList;
}

//-------------------------------------------------------------------------

StringTableEntry CodeBlock::getCurrentCodeBlockName()
{
    if (CodeBlock::getCurrentBlock())
        return CodeBlock::getCurrentBlock()->name;
    else
        return NULL;
}


CodeBlock* CodeBlock::find(StringTableEntry name)
{
    for (CodeBlock* walk = CodeBlock::getCodeBlockList(); walk; walk = walk->nextFile)
        if (walk->name == name)
            return walk;
    return NULL;
}

//-------------------------------------------------------------------------

void CodeBlock::addToCodeList()
{
    // remove any code blocks with my name
    for (CodeBlock** walk = &smCodeBlockList; *walk; walk = &((*walk)->nextFile))
    {
        if ((*walk)->name == name)
        {
            *walk = (*walk)->nextFile;
            break;
        }
    }
    nextFile = smCodeBlockList;
    smCodeBlockList = this;
}

void CodeBlock::clearAllBreaks()
{
    if (!lineBreakPairs)
        return;
    for (U32 i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        code[p[1]] = p[0] & 0xFF;
    }
}

void CodeBlock::clearBreakpoint(U32 lineNumber)
{
    if (!lineBreakPairs)
        return;
    for (U32 i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        if ((p[0] >> 8) == lineNumber)
        {
            code[p[1]] = p[0] & 0xFF;
            return;
        }
    }
}

void CodeBlock::setAllBreaks()
{
    if (!lineBreakPairs)
        return;
    for (U32 i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        code[p[1]] = OP_BREAK;
    }
}

bool CodeBlock::setBreakpoint(U32 lineNumber)
{
    if (!lineBreakPairs)
        return false;

    for (U32 i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        if ((p[0] >> 8) == lineNumber)
        {
            code[p[1]] = OP_BREAK;
            return true;
        }
    }

    return false;
}

U32 CodeBlock::findFirstBreakLine(U32 lineNumber)
{
    if (!lineBreakPairs)
        return 0;

    for (U32 i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        U32 line = (p[0] >> 8);

        if (lineNumber <= line)
            return line;
    }

    return 0;
}

struct LinePair
{
    U32 instLine;
    U32 ip;
};

void CodeBlock::findBreakLine(U32 ip, U32& line, U32& instruction)
{
    U32 min = 0;
    U32 max = lineBreakPairCount - 1;
    LinePair* p = (LinePair*)lineBreakPairs;

    U32 found;
    if (!lineBreakPairCount || p[min].ip > ip || p[max].ip < ip)
    {
        line = 0;
        instruction = OP_INVALID;
        return;
    }
    else if (p[min].ip == ip)
        found = min;
    else if (p[max].ip == ip)
        found = max;
    else
    {
        for (;;)
        {
            if (min == max - 1)
            {
                found = min;
                break;
            }
            U32 mid = (min + max) >> 1;
            if (p[mid].ip == ip)
            {
                found = mid;
                break;
            }
            else if (p[mid].ip > ip)
                max = mid;
            else
                min = mid;
        }
    }
    instruction = p[found].instLine & 0xFF;
    line = p[found].instLine >> 8;
}

const char* CodeBlock::getFileLine(U32 ip)
{
    static char nameBuffer[256];
    U32 line, inst;
    findBreakLine(ip, line, inst);

    dSprintf(nameBuffer, sizeof(nameBuffer), "%s (%d)", name ? name : "<input>", line);
    return nameBuffer;
}

void CodeBlock::removeFromCodeList()
{
    for (CodeBlock** walk = &smCodeBlockList; *walk; walk = &((*walk)->nextFile))
    {
        if (*walk == this)
        {
            *walk = nextFile;

            // clear out all breakpoints
            clearAllBreaks();
            return;
        }
    }
}

void CodeBlock::calcBreakList()
{
    U32 size = 0;
    S32 line = -1;
    U32 seqCount = 0;
    U32 i;
    for (i = 0; i < lineBreakPairCount; i++)
    {
        U32 lineNumber = lineBreakPairs[i * 2];
        if (lineNumber == U32(line + 1))
            seqCount++;
        else
        {
            if (seqCount)
                size++;
            size++;
            seqCount = 1;
        }
        line = lineNumber;
    }
    if (seqCount)
        size++;

    breakList = new U32[size];
    breakListSize = size;
    line = -1;
    seqCount = 0;
    size = 0;

    for (i = 0; i < lineBreakPairCount; i++)
    {
        U32 lineNumber = lineBreakPairs[i * 2];

        if (lineNumber == U32(line + 1))
            seqCount++;
        else
        {
            if (seqCount)
                breakList[size++] = seqCount;
            breakList[size++] = lineNumber - getMax(0, line) - 1;
            seqCount = 1;
        }

        line = lineNumber;
    }

    if (seqCount)
        breakList[size++] = seqCount;

    for (i = 0; i < lineBreakPairCount; i++)
    {
        dsize_t* p = lineBreakPairs + i * 2;
        p[0] = (p[0] << 8) | code[p[1]];
    }

    // Let the telnet debugger know that this code
    // block has been loaded and that it can add break
    // points it has for it.
    if (TelDebugger)
        TelDebugger->addAllBreakpoints(this);
}

bool CodeBlock::read(StringTableEntry fileName, Stream& st)
{
    name = fileName;

    //
    if (name)
    {
        if (const char* slash = dStrchr(this->name, '/'))
        {
            char root[512];
            dStrncpy(root, this->name, slash - this->name);
            root[slash - this->name] = 0;
            mRoot = StringTable->insert(root);
        }
    }

    //
    addToCodeList();

    U32 globalSize, size, i;
    st.read(&size);
    if (size)
    {
        globalSize = size;
        globalStrings = new char[size];
        st.read(size, globalStrings);
    }
    st.read(&size);
    if (size)
    {
        functionStrings = new char[size];
        st.read(size, functionStrings);
    }
    st.read(&size);
    if (size)
    {
        globalFloats = new F64[size];
        for (U32 i = 0; i < size; i++)
            st.read(&globalFloats[i]);
    }
    st.read(&size);
    if (size)
    {
        functionFloats = new F64[size];
        for (U32 i = 0; i < size; i++)
            st.read(&functionFloats[i]);
    }
    U32 codeSize;
    st.read(&codeSize);
    st.read(&lineBreakPairCount);

    U32 totSize = codeSize + lineBreakPairCount * 2;
    code = new dsize_t[totSize];

    for (i = 0; i < codeSize; i++)
    {
        U8 b;
        st.read(&b);
        if (b == 0xFF)
        {
            code[i] = 0;
            st.read((U32 *) &code[i]);
        }
        else
            code[i] = b;
    }

    for (i = codeSize; i < totSize; i++)
    {
        code[i] = 0;
        st.read((U32 *) &code[i]);
    }

    lineBreakPairs = code + codeSize;

    // StringTable-ize our identifiers.
    U32 identCount;
    st.read(&identCount);
    while (identCount--)
    {
        U32 offset;
        st.read(&offset);
        StringTableEntry ste;
        if (offset < globalSize)
            ste = StringTable->insert(globalStrings + offset);
        else
            ste = StringTable->insert("");
        U32 count;
        st.read(&count);
        while (count--)
        {
            U32 ip;
            st.read(&ip);
            code[ip] = *((dsize_t*)&ste);
        }
    }

    if (lineBreakPairCount)
        calcBreakList();

    return true;
}


bool CodeBlock::compile(const char* codeFileName, StringTableEntry fileName, const char* inScript)
{
#if defined(TORQUE_NO_DSO_GENERATION)
    // There is probably a better way to do this.  
    return false;
#endif

    // This will return true, but return value is ignored
    char* script;
    chompUTF8BOM(inScript, &script);

    gSyntaxError = false;

    consoleAllocReset();

    STEtoU32 = compileSTEtoU32;

    statementList = NULL;

    // Set up the parser.
    smCurrentParser = getParserForFile(fileName);
    AssertISV(smCurrentParser, avar("CodeBlock::compile - no parser available for '%s'!", fileName));

    // Now do some parsing.
    smCurrentParser->setScanBuffer(script, fileName);
    smCurrentParser->restart(NULL);
    smCurrentParser->parse();

    if (gSyntaxError)
    {
        consoleAllocReset();
        return false;
    }

    Stream* st;
    if (!ResourceManager->openFileForWrite(st, codeFileName))
        return false;
    st->write(U32(Con::DSOVersion));

    // Reset all our value tables...
    resetTables();

    smInFunction = false;
    smBreakLineCount = 0;
    setBreakCodeBlock(this);

    if (statementList)
        codeSize = precompileBlock(statementList, 0) + 1;
    else
        codeSize = 1;

    lineBreakPairCount = smBreakLineCount;
    code = new dsize_t[codeSize + smBreakLineCount * 2];

    lineBreakPairs = code + codeSize;

    // Write string table data...
    getGlobalStringTable().write(*st);
    getFunctionStringTable().write(*st);

    // Write float table data...
    getGlobalFloatTable().write(*st);
    getFunctionFloatTable().write(*st);

    smBreakLineCount = 0;
    U32 lastIp;
    if (statementList)
        lastIp = compileBlock(statementList, code, 0, 0, 0);
    else
        lastIp = 0;

    if (lastIp != codeSize - 1)
        Con::errorf(ConsoleLogEntry::General, "CodeBlock::compile - precompile size mismatch, a precompile/compile function pair is probably mismatched.");

    code[lastIp++] = OP_RETURN;
    U32 totSize = codeSize + smBreakLineCount * 2;
    st->write(codeSize);
    st->write(lineBreakPairCount);

    // Write out our bytecode, doing a bit of compression for low numbers.
    U32 i;
    for (i = 0; i < codeSize; i++)
    {
        if (code[i] < 0xFF)
            st->write(U8(code[i]));
        else
        {
            st->write(U8(0xFF));
            st->write((U32)code[i]);
        }
    }

    // Write the break info...
    for (i = codeSize; i < totSize; i++)
        st->write((U32)code[i]);

    getIdentTable().write(*st);

    consoleAllocReset();
    delete st;

    return true;
}

const char* CodeBlock::compileExec(StringTableEntry fileName, const char* inString, bool noCalls, int setFrame)
{
    // Check for a UTF8 script file
    char* string;
    chompUTF8BOM(inString, &string);

    STEtoU32 = evalSTEtoU32;
    consoleAllocReset();

    name = fileName;

    if (name)
        addToCodeList();

    statementList = NULL;

    // Set up the parser.
    smCurrentParser = getParserForFile(fileName);
    AssertISV(smCurrentParser, avar("CodeBlock::compile - no parser available for '%s'!", fileName));

    // Now do some parsing.
    smCurrentParser->setScanBuffer(string, fileName);
    smCurrentParser->restart(NULL);
    smCurrentParser->parse();

    if (!statementList)
    {
        delete this;
        return "";
    }

    resetTables();

    smInFunction = false;
    smBreakLineCount = 0;
    setBreakCodeBlock(this);

    codeSize = precompileBlock(statementList, 0) + 1;

    lineBreakPairCount = smBreakLineCount;

    globalStrings = getGlobalStringTable().build();
    functionStrings = getFunctionStringTable().build();
    globalFloats = getGlobalFloatTable().build();
    functionFloats = getFunctionFloatTable().build();

    code = new dsize_t[codeSize + lineBreakPairCount * 2];

    lineBreakPairs = code + codeSize;

    smBreakLineCount = 0;
    U32 lastIp = compileBlock(statementList, code, 0, 0, 0);
    code[lastIp++] = OP_RETURN;

    consoleAllocReset();

    if (lineBreakPairCount && fileName)
        calcBreakList();

    if (lastIp != codeSize)
        Con::warnf(ConsoleLogEntry::General, "precompile size mismatch");

    return exec(0, fileName, NULL, 0, 0, noCalls, NULL, setFrame);
}

//-------------------------------------------------------------------------

void CodeBlock::incRefCount()
{
    refCount++;
}

void CodeBlock::decRefCount()
{
    refCount--;
    if (!refCount)
        delete this;
}
