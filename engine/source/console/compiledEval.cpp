//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"

#include "console/ast.h"
#include "core/tAlgorithm.h"
#include "core/resManager.h"

#include "core/findMatch.h"
#include "console/consoleInternal.h"
#include "core/fileStream.h"
#include "console/compiler.h"

#include "console/simBase.h"
#include "console/telnetDebugger.h"
#include "sim/netStringTable.h"
#include "console/stringStack.h"

#include "materials/material.h"
using namespace Compiler;

enum EvalConstants {
    MaxStackSize = 1024
};

namespace Con
{
    // Current script file name and root, these are registered as
    // console variables.
    extern StringTableEntry gCurrentFile;
    extern StringTableEntry gCurrentRoot;
}

F64 floatStack[MaxStackSize];
U32 intStack[MaxStackSize];

StringStack STR;

U32 FLT = 0;
U32 UINT = 0;

static const char* getNamespaceList(Namespace* ns)
{
    U32 size = 1;
    Namespace* walk;
    for (walk = ns; walk; walk = walk->mParent)
        size += dStrlen(walk->mName) + 4;
    char* ret = Con::getReturnBuffer(size);
    ret[0] = 0;
    for (walk = ns; walk; walk = walk->mParent)
    {
        dStrcat(ret, walk->mName);
        if (walk->mParent)
            dStrcat(ret, " -> ");
    }
    return ret;
}

//------------------------------------------------------------

F64 consoleStringToNumber(const char* str, StringTableEntry file, U32 line)
{
    F64 val = dAtof(str);
    if (val != 0)
        return val;
    else if (!dStricmp(str, "true"))
        return 1;
    else if (!dStricmp(str, "false"))
        return 0;
    else if (file)
    {
        Con::warnf(ConsoleLogEntry::General, "%s (%d): string always evaluates to 0.", file, line);
        return 0;
    }
    return 0;
}

//------------------------------------------------------------

namespace Con
{

    char* getReturnBuffer(U32 bufferSize)

    {
        return STR.getReturnBuffer(bufferSize);
    }

    char* getReturnBuffer( const char *stringToCopy )
    {
        char *ret = STR.getReturnBuffer( dStrlen( stringToCopy ) + 1 );
        dStrcpy( ret, stringToCopy );
        ret[dStrlen( stringToCopy )] = '\0';
        return ret;
    }

    char* getArgBuffer(U32 bufferSize)
    {
        return STR.getArgBuffer(bufferSize);
    }

    char* getFloatArg(F64 arg)
    {

        char* ret = STR.getArgBuffer(32);
        dSprintf(ret, 32, "%g", arg);
        return ret;
    }

    char* getIntArg(S32 arg)
    {

        char* ret = STR.getArgBuffer(32);
        dSprintf(ret, 32, "%d", arg);
        return ret;
    }
}

//------------------------------------------------------------

inline void ExprEvalState::setCurVarName(StringTableEntry name)
{
    if (name[0] == '$')
        currentVariable = globalVars.lookup(name);
    else if (stack.size())
        currentVariable = stack.last()->lookup(name);
    if (!currentVariable && gWarnUndefinedScriptVariables)
        Con::warnf(ConsoleLogEntry::Script, "Variable referenced before assignment: %s", name);
}

inline void ExprEvalState::setCurVarNameCreate(StringTableEntry name)
{
    if (name[0] == '$')
        currentVariable = globalVars.add(name);
    else if (stack.size())
        currentVariable = stack.last()->add(name);
    else
    {
        currentVariable = NULL;
        Con::warnf(ConsoleLogEntry::Script, "Accessing local variable in global scope... failed: %s", name);
    }
}

//------------------------------------------------------------

inline S32 ExprEvalState::getIntVariable()
{
    return currentVariable ? currentVariable->getIntValue() : 0;
}

inline F64 ExprEvalState::getFloatVariable()
{
    return currentVariable ? currentVariable->getFloatValue() : 0;
}

inline const char* ExprEvalState::getStringVariable()
{
    return currentVariable ? currentVariable->getStringValue() : "";
}

//------------------------------------------------------------

inline void ExprEvalState::setIntVariable(S32 val)
{
    AssertFatal(currentVariable != NULL, "Invalid evaluator state - trying to set null variable!");
    currentVariable->setIntValue(val);
}

inline void ExprEvalState::setFloatVariable(F64 val)
{
    AssertFatal(currentVariable != NULL, "Invalid evaluator state - trying to set null variable!");
    currentVariable->setFloatValue(val);
}

inline void ExprEvalState::setStringVariable(const char* val)
{
    AssertFatal(currentVariable != NULL, "Invalid evaluator state - trying to set null variable!");
    currentVariable->setStringValue(val);
}

//------------------------------------------------------------

void CodeBlock::getFunctionArgs(char buffer[1024], U32 ip)
{
    U32 fnArgc = code[ip + 5];
    buffer[0] = 0;
    for (U32 i = 0; i < fnArgc; i++)
    {
        StringTableEntry var = U32toSTE(code[ip + i + 6]);

        // Add a comma so it looks nice!
        if (i != 0)
            dStrcat(buffer, ", ");

        dStrcat(buffer, "var ");

        // Try to capture junked parameters
        if (var[0])
            dStrcat(buffer, var + 1);
        else
            dStrcat(buffer, "JUNK");
    }
}

const char* CodeBlock::exec(U32 ip, const char* functionName, Namespace* thisNamespace, U32 argc, const char** argv, bool noCalls, StringTableEntry packageName, S32 setFrame)
{
    static char traceBuffer[1024];
    U32 i;

    incRefCount();
    F64* curFloatTable;
    char* curStringTable;
    STR.clearFunctionOffset();
    StringTableEntry thisFunctionName = NULL;
    bool popFrame = false;
    if (argv)
    {
        // assume this points into a function decl:
        U32 fnArgc = code[ip + 5];
        thisFunctionName = U32toSTE(code[ip]);
        argc = getMin(argc - 1, fnArgc); // argv[0] is func name
        if (gEvalState.traceOn)
        {
            traceBuffer[0] = 0;
            dStrcat(traceBuffer, "Entering ");
            if (packageName)
            {
                dStrcat(traceBuffer, "[");
                dStrcat(traceBuffer, packageName);
                dStrcat(traceBuffer, "]");
            }
            if (thisNamespace && thisNamespace->mName)
            {
                dSprintf(traceBuffer + dStrlen(traceBuffer), sizeof(traceBuffer) - dStrlen(traceBuffer),
                    "%s::%s(", thisNamespace->mName, thisFunctionName);
            }
            else
            {
                dSprintf(traceBuffer + dStrlen(traceBuffer), sizeof(traceBuffer) - dStrlen(traceBuffer),
                    "%s(", thisFunctionName);
            }
            for (i = 0; i < argc; i++)
            {
                dStrcat(traceBuffer, argv[i + 1]);
                if (i != argc - 1)
                    dStrcat(traceBuffer, ", ");
            }
            dStrcat(traceBuffer, ")");
            Con::printf("%s", traceBuffer);
        }
        gEvalState.pushFrame(thisFunctionName, thisNamespace);
        popFrame = true;
        for (i = 0; i < argc; i++)
        {
            StringTableEntry var = U32toSTE(code[ip + i + 6]);
            gEvalState.setCurVarNameCreate(var);
            gEvalState.setStringVariable(argv[i + 1]);
        }
        ip = ip + fnArgc + 6;
        curFloatTable = functionFloats;
        curStringTable = functionStrings;
    }
    else
    {
        curFloatTable = globalFloats;
        curStringTable = globalStrings;

        // Do we want this code to execute using a new stack frame?
        if (setFrame < 0)
        {
            gEvalState.pushFrame(NULL, NULL);
            popFrame = true;
        }
        else if (!gEvalState.stack.empty())
        {
            // We want to copy a reference to an existing stack frame
            // on to the top of the stack.  Any change that occurs to 
            // the locals during this new frame will also occur in the 
            // original frame.
            S32 stackIndex = gEvalState.stack.size() - setFrame - 1;
            gEvalState.pushFrameRef(stackIndex);
            popFrame = true;
        }
    }

    if (TelDebugger && setFrame < 0)
        TelDebugger->pushStackFrame();

    StringTableEntry var, objParent;
    U32 failJump;
    StringTableEntry fnName;
    StringTableEntry fnNamespace, fnPackage;
    SimObject* currentNewObject = 0;
    StringTableEntry curField;
    SimObject* curObject;
    SimObject* saveObject = NULL;
    Namespace::Entry* nsEntry;
    Namespace* ns;

    U32 callArgc;
    const char** callArgv;

    static char curFieldArray[256];

    CodeBlock* saveCodeBlock = smCurrentCodeBlock;
    smCurrentCodeBlock = this;
    if (this->name)
    {
        Con::gCurrentFile = this->name;
        Con::gCurrentRoot = mRoot;
    }
    const char* val;
    for (;;)
    {
        U32 instruction = code[ip++];
    breakContinue:
        switch (instruction)
        {
        case OP_FUNC_DECL:
            if (!noCalls)
            {
                fnName = U32toSTE(code[ip]);
                fnNamespace = U32toSTE(code[ip + 1]);
                fnPackage = U32toSTE(code[ip + 2]);
                bool hasBody = bool(code[ip + 3]);

                Namespace::unlinkPackages();
                ns = Namespace::find(fnNamespace, fnPackage);
                ns->addFunction(fnName, this, hasBody ? ip : 0);// if no body, set the IP to 0
                Namespace::relinkPackages();

                //Con::printf("Adding function %s::%s (%d)", fnNamespace, fnName, ip);
            }
            ip = code[ip + 4];
            break;

        case OP_CREATE_OBJECT:
        {
            // If we don't allow calls, we certainly don't allow creating objects!
            if (noCalls)
            {
                ip = failJump;
                break;
            }

            // Read some useful info.
            objParent = U32toSTE(code[ip]);
            bool isDataBlock = code[ip + 1];
            failJump = code[ip + 2];

            // Get the constructor information off the stack.
            STR.getArgcArgv(NULL, &callArgc, &callArgv);

            // Con::printf("Creating object...");

            // objectName = argv[1]...
            currentNewObject = NULL;

            // Are we creating a datablock? If so, deal with case where we override
            // an old one.
            if (isDataBlock)
            {
                // Con::printf("  - is a datablock");

                // Find the old one if any.
                SimObject* db = Sim::getDataBlockGroup()->findObject(callArgv[2]);

                // Make sure we're not changing types on ourselves...
                if (db && dStricmp(db->getClassName(), callArgv[1]))
                {
                    Con::errorf(ConsoleLogEntry::General, "Cannot re-declare data block %s with a different class.", callArgv[2]);
                    ip = failJump;
                    break;
                }

                // If there was one, set the currentNewObject and move on.
                if (db)
                    currentNewObject = db;
            }

            // If object is an existing Material, use the existing Material and overwrite its properties.
            // Compare class names to see if we are creating a Material
            // this is a string compare, but it should exit quickly under normal conditions
            if (dStrcmp(callArgv[1], Material::getStaticClassRep()->getClassName()) == 0)
            {
                // we're creating a material, so see if we already have a Material with this
                // name
                SimObject* obj = Material::getMaterialSet()->findObject(callArgv[2]);
                if (obj)
                {
                    AssertFatal(dynamic_cast<Material*>(obj) != NULL, "doh, not a material!");
                    currentNewObject = obj;
                }
            }


            if (!currentNewObject)
            {
                // Well, looks like we have to create a new object.
                ConsoleObject* object = ConsoleObject::create(callArgv[1]);

                // Deal with failure!
                if (!object)
                {
                    Con::errorf(ConsoleLogEntry::General, "%s: Unable to instantiate non-conobject class %s.", getFileLine(ip - 1), callArgv[1]);
                    ip = failJump;
                    break;
                }

                // Do special datablock init if appropros
                if (isDataBlock)
                {
                    SimDataBlock* dataBlock = dynamic_cast<SimDataBlock*>(object);
                    if (dataBlock)
                    {
                        dataBlock->assignId();
                    }
                    else
                    {
                        // They tried to make a non-datablock with a datablock keyword!
                        Con::errorf(ConsoleLogEntry::General, "%s: Unable to instantiate non-datablock class %s.", getFileLine(ip - 1), callArgv[1]);

                        // Clean up...
                        delete object;
                        ip = failJump;
                        break;
                    }
                }

                // Finally, set currentNewObject to point to the new one.
                currentNewObject = dynamic_cast<SimObject*>(object);

                // Deal with the case of a non-SimObject.
                if (!currentNewObject)
                {
                    Con::errorf(ConsoleLogEntry::General, "%s: Unable to instantiate non-SimObject class %s.", getFileLine(ip - 1), callArgv[1]);
                    delete object;
                    ip = failJump;
                    break;
                }

                // Does it have a parent object? (ie, the copy constructor : syntax, not inheriance)
                if (*objParent)
                {
                    // Find it!
                    SimObject* parent;
                    if (Sim::findObject(objParent, parent))
                    {
                        // Con::printf(" - Parent object found: %s", parent->getClassName());

                        // and suck the juices from it!
                        currentNewObject->assignFieldsFrom(parent);
                    }
                    else
                        Con::errorf(ConsoleLogEntry::General, "%s: Unable to find parent object %s for %s.", getFileLine(ip - 1), objParent, callArgv[1]);

                    // Mm! Juices!
                }

                // If a name was passed, assign it.
                if (callArgv[2][0])
                    currentNewObject->assignName(callArgv[2]);

                // Do the constructor parameters.
                if (!currentNewObject->processArguments(callArgc - 3, callArgv + 3))
                {
                    delete currentNewObject;
                    currentNewObject = NULL;
                    ip = failJump;
                    break;
                }

                // If it's not a datablock, allow people to modify bits of it.
                if (!isDataBlock)
                {
                    currentNewObject->setModStaticFields(true);
                    currentNewObject->setModDynamicFields(true);
                }
            }

            // Advance the IP past the create info...
            ip += 3;
            break;
        }

        case OP_ADD_OBJECT:
        {
            // Do we place this object at the root?
            bool placeAtRoot = code[ip++];

            // Con::printf("Adding object %s", currentNewObject->getName());

            // Make sure it wasn't already added, then add it.
            if (currentNewObject->isProperlyAdded() == false && !currentNewObject->registerObject())
            {
                // This error is usually caused by failing to call Parent::initPersistFields in the class' initPersistFields().
                Con::warnf(ConsoleLogEntry::General, "%s: Register object failed for object %s of class %s.", getFileLine(ip - 2), currentNewObject->getName(), currentNewObject->getClassName());
                delete currentNewObject;
                ip = failJump;
                break;
            }

            // Are we dealing with a datablock?
            SimDataBlock* dataBlock = dynamic_cast<SimDataBlock*>(currentNewObject);
            static char errorBuffer[256];

            // If so, preload it.
            if (dataBlock && !dataBlock->preload(true, errorBuffer))
            {
                Con::errorf(ConsoleLogEntry::General, "%s: preload failed for %s: %s.", getFileLine(ip - 2),
                    currentNewObject->getName(), errorBuffer);
                dataBlock->deleteObject();
                ip = failJump;
                break;
            }

            // What group will we be added to, if any?
            U32 groupAddId = intStack[UINT];
            SimGroup* grp = NULL;
            SimSet* set = NULL;

            if (!placeAtRoot || !currentNewObject->getGroup())
            {
                if (placeAtRoot)
                {
                    // Deal with the instantGroup if we're being put at the root.
                    const char* addGroupName = Con::getVariable("instantGroup");
                    if (!Sim::findObject(addGroupName, grp))
                        Sim::findObject(RootGroupId, grp);
                }
                else
                {
                    // Otherwise just add to the requested group or set.
                    if (!Sim::findObject(groupAddId, grp))
                        Sim::findObject(groupAddId, set);
                }

                // If we didn't get a group, then make sure we have a pointer to
                // the rootgroup.
                if (!grp)
                    Sim::findObject(RootGroupId, grp);

                // add to the parent group
                grp->addObject(currentNewObject);

                // add to any set we might be in
                if (set)
                    set->addObject(currentNewObject);
            }

            // store the new object's ID on the stack (overwriting the group/set
            // id, if one was given, otherwise getting pushed)
            if (placeAtRoot)
                intStack[UINT] = currentNewObject->getId();
            else
                intStack[++UINT] = currentNewObject->getId();

            break;
        }

        case OP_END_OBJECT:
        {
            // If we're not to be placed at the root, make sure we clean up
            // our group reference.
            bool placeAtRoot = code[ip++];
            if (!placeAtRoot)
                UINT--;
            break;
        }

        case OP_JMPIFFNOT:
            if (floatStack[FLT--])
            {
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMPIFNOT:
            if (intStack[UINT--])
            {
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMPIFF:
            if (!floatStack[FLT--])
            {
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMPIF:
            if (!intStack[UINT--])
            {
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMPIFNOT_NP:
            if (intStack[UINT])
            {
                UINT--;
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMPIF_NP:
            if (!intStack[UINT])
            {
                UINT--;
                ip++;
                break;
            }
            ip = code[ip];
            break;
        case OP_JMP:
            ip = code[ip];
            break;
        case OP_RETURN:
            goto execFinished;
        case OP_CMPEQ:
            intStack[UINT + 1] = bool(floatStack[FLT] == floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_CMPGR:
            intStack[UINT + 1] = bool(floatStack[FLT] > floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_CMPGE:
            intStack[UINT + 1] = bool(floatStack[FLT] >= floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_CMPLT:
            intStack[UINT + 1] = bool(floatStack[FLT] < floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_CMPLE:
            intStack[UINT + 1] = bool(floatStack[FLT] <= floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_CMPNE:
            intStack[UINT + 1] = bool(floatStack[FLT] != floatStack[FLT - 1]);
            UINT++;
            FLT -= 2;
            break;

        case OP_XOR:
            intStack[UINT - 1] = intStack[UINT] ^ intStack[UINT - 1];
            UINT--;
            break;

        case OP_MOD:
            intStack[UINT - 1] = intStack[UINT] % intStack[UINT - 1];
            UINT--;
            break;

        case OP_BITAND:
            intStack[UINT - 1] = intStack[UINT] & intStack[UINT - 1];
            UINT--;
            break;

        case OP_BITOR:
            intStack[UINT - 1] = intStack[UINT] | intStack[UINT - 1];
            UINT--;
            break;

        case OP_NOT:
            intStack[UINT] = !intStack[UINT];
            break;

        case OP_NOTF:
            intStack[UINT + 1] = !floatStack[FLT];
            FLT--;
            UINT++;
            break;

        case OP_ONESCOMPLEMENT:
            intStack[UINT] = ~intStack[UINT];
            break;

        case OP_SHR:
            intStack[UINT - 1] = intStack[UINT] >> intStack[UINT - 1];
            UINT--;
            break;

        case OP_SHL:
            intStack[UINT - 1] = intStack[UINT] << intStack[UINT - 1];
            UINT--;
            break;

        case OP_AND:
            intStack[UINT - 1] = intStack[UINT] && intStack[UINT - 1];
            UINT--;
            break;

        case OP_OR:
            intStack[UINT - 1] = intStack[UINT] || intStack[UINT - 1];
            UINT--;
            break;

        case OP_ADD:
            floatStack[FLT - 1] = floatStack[FLT] + floatStack[FLT - 1];
            FLT--;
            break;

        case OP_SUB:
            floatStack[FLT - 1] = floatStack[FLT] - floatStack[FLT - 1];
            FLT--;
            break;

        case OP_MUL:
            floatStack[FLT - 1] = floatStack[FLT] * floatStack[FLT - 1];
            FLT--;
            break;
        case OP_DIV:
            floatStack[FLT - 1] = floatStack[FLT] / floatStack[FLT - 1];
            FLT--;
            break;
        case OP_NEG:
            floatStack[FLT] = -floatStack[FLT];
            break;

        case OP_SETCURVAR:
            var = U32toSTE(code[ip]);
            ip++;
            gEvalState.setCurVarName(var);
            break;

        case OP_SETCURVAR_CREATE:
            var = U32toSTE(code[ip]);
            ip++;
            gEvalState.setCurVarNameCreate(var);
            break;

        case OP_SETCURVAR_ARRAY:
            var = STR.getSTValue();
            gEvalState.setCurVarName(var);
            break;

        case OP_SETCURVAR_ARRAY_CREATE:
            var = STR.getSTValue();
            gEvalState.setCurVarNameCreate(var);
            break;

        case OP_LOADVAR_UINT:
            intStack[UINT + 1] = gEvalState.getIntVariable();
            UINT++;
            break;

        case OP_LOADVAR_FLT:
            floatStack[FLT + 1] = gEvalState.getFloatVariable();
            FLT++;
            break;

        case OP_LOADVAR_STR:
            val = gEvalState.getStringVariable();
            STR.setStringValue(val);
            break;

        case OP_SAVEVAR_UINT:
            gEvalState.setIntVariable(intStack[UINT]);
            break;

        case OP_SAVEVAR_FLT:
            gEvalState.setFloatVariable(floatStack[FLT]);
            break;

        case OP_SAVEVAR_STR:
            gEvalState.setStringVariable(STR.getStringValue());
            break;

        case OP_SETCUROBJECT:
            curObject = Sim::findObject(STR.getStringValue());
            break;

        case OP_SETCUROBJECT_NEW:
            curObject = currentNewObject;
            break;

        case OP_SETCURFIELD:
            curField = U32toSTE(code[ip]);
            curFieldArray[0] = 0;
            ip++;
            break;

        case OP_SETCURFIELD_ARRAY:
            dStrcpy(curFieldArray, STR.getStringValue());
            break;

        case OP_LOADFIELD_UINT:
            if (curObject)
                intStack[UINT + 1] = U32(dAtoi(curObject->getDataField(curField, curFieldArray)));
            else
                intStack[UINT + 1] = 0;
            UINT++;
            break;

        case OP_LOADFIELD_FLT:
            if (curObject)
                floatStack[FLT + 1] = dAtof(curObject->getDataField(curField, curFieldArray));
            else
                floatStack[FLT + 1] = 0;
            FLT++;
            break;

        case OP_LOADFIELD_STR:
            if (curObject)
                val = curObject->getDataField(curField, curFieldArray);
            else
                val = "";
            STR.setStringValue(val);
            break;

        case OP_SAVEFIELD_UINT:
            STR.setIntValue(intStack[UINT]);
            if (curObject)
                curObject->setDataField(curField, curFieldArray, STR.getStringValue());
            break;

        case OP_SAVEFIELD_FLT:
            STR.setFloatValue(floatStack[FLT]);
            if (curObject)
                curObject->setDataField(curField, curFieldArray, STR.getStringValue());
            break;

        case OP_SAVEFIELD_STR:
            if (curObject)
                curObject->setDataField(curField, curFieldArray, STR.getStringValue());
            break;

        case OP_STR_TO_UINT:
            intStack[UINT + 1] = STR.getIntValue();
            UINT++;
            break;

        case OP_STR_TO_FLT:
            floatStack[FLT + 1] = STR.getFloatValue();
            FLT++;
            break;

        case OP_STR_TO_NONE:
            // This exists simply to deal with certain typecast situations.
            break;

        case OP_FLT_TO_UINT:
            intStack[UINT + 1] = (unsigned int)floatStack[FLT];
            FLT--;
            UINT++;
            break;

        case OP_FLT_TO_STR:
            STR.setFloatValue(floatStack[FLT]);
            FLT--;
            break;

        case OP_FLT_TO_NONE:
            FLT--;
            break;

        case OP_UINT_TO_FLT:
            floatStack[FLT + 1] = intStack[UINT];
            UINT--;
            FLT++;
            break;

        case OP_UINT_TO_STR:
            STR.setIntValue(intStack[UINT]);
            UINT--;
            break;

        case OP_UINT_TO_NONE:
            UINT--;
            break;

        case OP_LOADIMMED_UINT:
            intStack[UINT + 1] = code[ip++];
            UINT++;
            break;

        case OP_LOADIMMED_FLT:
            floatStack[FLT + 1] = curFloatTable[code[ip]];
            ip++;
            FLT++;
            break;
        case OP_TAG_TO_STR:
            code[ip - 1] = OP_LOADIMMED_STR;
            // it's possible the string has already been converted
            if (U8(curStringTable[code[ip]]) != StringTagPrefixByte)
            {
                U32 id = GameAddTaggedString(curStringTable + code[ip]);
                dSprintf(curStringTable + code[ip] + 1, 7, "%d", id);
                *(curStringTable + code[ip]) = StringTagPrefixByte;
            }
        case OP_LOADIMMED_STR:
            STR.setStringValue(curStringTable + code[ip++]);
            break;

        case OP_LOADIMMED_IDENT:
            STR.setStringValue(U32toSTE(code[ip++]));
            break;

        case OP_CALLFUNC_RESOLVE:
            // This deals with a function that is potentially living in a namespace.
            fnNamespace = U32toSTE(code[ip + 1]);
            fnName = U32toSTE(code[ip]);

            // Try to look it up.
            ns = Namespace::find(fnNamespace);
            nsEntry = ns->lookup(fnName);
            if (!nsEntry)
            {
                ip += 3;
                Con::warnf(ConsoleLogEntry::General,
                    "%s: Unable to find function %s%s%s",
                    getFileLine(ip - 4), fnNamespace ? fnNamespace : "",
                    fnNamespace ? "::" : "", fnName);
                STR.getArgcArgv(fnName, &callArgc, &callArgv);
                break;
            }
            // Now, rewrite our code a bit (ie, avoid future lookups) and fall
            // through to OP_CALLFUNC
            code[ip + 1] = *((dsize_t*)&nsEntry);
            code[ip - 1] = OP_CALLFUNC;

        case OP_CALLFUNC:
        {
            fnName = U32toSTE(code[ip]);

            //if this is called from inside a function, append the ip and codeptr
            if (!gEvalState.stack.empty())
            {
                gEvalState.stack.last()->code = this;
                gEvalState.stack.last()->ip = ip - 1;
            }

            U32 callType = code[ip + 2];

            ip += 3;
            STR.getArgcArgv(fnName, &callArgc, &callArgv);

            if (callType == FuncCallExprNode::FunctionCall) {
                nsEntry = *((Namespace::Entry**)&code[ip - 2]);
                ns = NULL;
            }
            else if (callType == FuncCallExprNode::MethodCall)
            {
                saveObject = gEvalState.thisObject;
                gEvalState.thisObject = Sim::findObject(callArgv[1]);
                if (!gEvalState.thisObject)
                {
                    gEvalState.thisObject = 0;
                    Con::warnf(ConsoleLogEntry::General, "%s: Unable to find object: '%s' attempting to call function '%s'", getFileLine(ip - 4), callArgv[1], fnName);
                    break;
                }
                ns = gEvalState.thisObject->getNamespace();
                if (ns)
                    nsEntry = ns->lookup(fnName);
                else
                    nsEntry = NULL;
            }
            else // it's a ParentCall
            {
                if (thisNamespace)
                {
                    ns = thisNamespace->mParent;
                    if (ns)
                        nsEntry = ns->lookup(fnName);
                    else
                        nsEntry = NULL;
                }
                else
                {
                    ns = NULL;
                    nsEntry = NULL;
                }
            }

            if (!nsEntry || noCalls)
            {
                if (!noCalls)
                {
                    Con::warnf(ConsoleLogEntry::General, "%s: Unknown command %s.", getFileLine(ip - 4), fnName);
                    if (callType == FuncCallExprNode::MethodCall)
                    {
                        Con::warnf(ConsoleLogEntry::General, "  Object %s(%d) %s",
                            gEvalState.thisObject->getName() ? gEvalState.thisObject->getName() : "",
                            gEvalState.thisObject->getId(), getNamespaceList(ns));
                    }
                }
                STR.setStringValue("");
                break;
            }
            if (nsEntry->mType == Namespace::Entry::ScriptFunctionType)
            {
                if (nsEntry->mFunctionOffset)
                    nsEntry->mCode->exec(nsEntry->mFunctionOffset, fnName, nsEntry->mNamespace, callArgc, callArgv, false, nsEntry->mPackage);
                else // no body
                    STR.setStringValue("");
            }
            else
            {
                if ((nsEntry->mMinArgs && S32(callArgc) < nsEntry->mMinArgs) || (nsEntry->mMaxArgs && S32(callArgc) > nsEntry->mMaxArgs))
                {
                    const char* nsName = ns ? ns->mName : "";
                    Con::warnf(ConsoleLogEntry::Script, "%s: %s::%s - wrong number of arguments.", getFileLine(ip - 4), nsName, fnName);
                    Con::warnf(ConsoleLogEntry::Script, "%s: usage: %s", getFileLine(ip - 4), nsEntry->mUsage);
                }
                else
                {
                    switch (nsEntry->mType)
                    {
                    case Namespace::Entry::StringCallbackType:
                    {
                        const char* ret = nsEntry->cb.mStringCallbackFunc(gEvalState.thisObject, callArgc, callArgv);
                        if (ret != STR.getStringValue())
                            STR.setStringValue(ret);
                        else
                            STR.setLen(dStrlen(ret));
                        break;
                    }
                    case Namespace::Entry::IntCallbackType:
                    {
                        S32 result = nsEntry->cb.mIntCallbackFunc(gEvalState.thisObject, callArgc, callArgv);
                        if (code[ip] == OP_STR_TO_UINT)
                        {
                            ip++;
                            intStack[++UINT] = result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_FLT)
                        {
                            ip++;
                            floatStack[++FLT] = result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_NONE)
                            ip++;
                        else
                            STR.setIntValue(result);
                        break;
                    }
                    case Namespace::Entry::FloatCallbackType:
                    {
                        F64 result = nsEntry->cb.mFloatCallbackFunc(gEvalState.thisObject, callArgc, callArgv);
                        if (code[ip] == OP_STR_TO_UINT)
                        {
                            ip++;
                            intStack[++UINT] = (unsigned int)result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_FLT)
                        {
                            ip++;
                            floatStack[++FLT] = result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_NONE)
                            ip++;
                        else
                            STR.setFloatValue(result);
                        break;
                    }
                    case Namespace::Entry::VoidCallbackType:
                        nsEntry->cb.mVoidCallbackFunc(gEvalState.thisObject, callArgc, callArgv);
#ifdef CONSOLE_WARN_VOID_ASSIGNMENT
                        if (code[ip] != OP_STR_TO_NONE && Con::getBoolVariable("$Con::warnVoidAssignment", true))
                            Con::warnf(ConsoleLogEntry::General, "%s: Call to %s in %s uses result of void function call.", getFileLine(ip - 4), fnName, functionName);
#endif
                        STR.setStringValue("");
                        break;
                    case Namespace::Entry::BoolCallbackType:
                    {
                        bool result = nsEntry->cb.mBoolCallbackFunc(gEvalState.thisObject, callArgc, callArgv);
                        if (code[ip] == OP_STR_TO_UINT)
                        {
                            ip++;
                            intStack[++UINT] = result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_FLT)
                        {
                            ip++;
                            floatStack[++FLT] = result;
                            break;
                        }
                        else if (code[ip] == OP_STR_TO_NONE)
                            ip++;
                        else
                            STR.setIntValue(result);
                        break;
                    }
                    }
                }
            }

            if (callType == FuncCallExprNode::MethodCall)
                gEvalState.thisObject = saveObject;
            break;
        }
        case OP_ADVANCE_STR:
            STR.advance();
            break;
        case OP_ADVANCE_STR_APPENDCHAR:
            STR.advanceChar(code[ip++]);
            break;

        case OP_ADVANCE_STR_COMMA:
            STR.advanceChar('_');
            break;

        case OP_ADVANCE_STR_NUL:
            STR.advanceChar(0);
            break;

        case OP_REWIND_STR:
            STR.rewind();
            break;

        case OP_TERMINATE_REWIND_STR:
            STR.rewindTerminate();
            break;

        case OP_COMPARE_STR:
            intStack[++UINT] = STR.compare();
            break;
        case OP_PUSH:
            STR.push();
            break;

        case OP_PUSH_FRAME:
            STR.pushFrame();
            break;
        case OP_BREAK:
        {
            //append the ip and codeptr before managing the breakpoint!
            AssertFatal(!gEvalState.stack.empty(), "Empty eval stack on break!");
            gEvalState.stack.last()->code = this;
            gEvalState.stack.last()->ip = ip - 1;

            U32 breakLine;
            findBreakLine(ip - 1, breakLine, instruction);
            if (!breakLine)
                goto breakContinue;
            TelDebugger->executionStopped(this, breakLine);
            goto breakContinue;
        }
        case OP_INVALID:

        default:
            // error!
            goto execFinished;
        }
    }
execFinished:

    if (TelDebugger && setFrame < 0)
        TelDebugger->popStackFrame();

    if (popFrame)
        gEvalState.popFrame();

    if (argv)
    {
        if (gEvalState.traceOn)
        {
            traceBuffer[0] = 0;
            dStrcat(traceBuffer, "Leaving ");

            if (packageName)
            {
                dStrcat(traceBuffer, "[");
                dStrcat(traceBuffer, packageName);
                dStrcat(traceBuffer, "]");
            }
            if (thisNamespace && thisNamespace->mName)
            {
                dSprintf(traceBuffer + dStrlen(traceBuffer), sizeof(traceBuffer) - dStrlen(traceBuffer),
                    "%s::%s() - return %s", thisNamespace->mName, thisFunctionName, STR.getStringValue());
            }
            else
            {
                dSprintf(traceBuffer + dStrlen(traceBuffer), sizeof(traceBuffer) - dStrlen(traceBuffer),
                    "%s() - return %s", thisFunctionName, STR.getStringValue());
            }
            Con::printf("%s", traceBuffer);
        }
    }
    else
    {
        delete[] const_cast<char*>(globalStrings);
        delete[] globalFloats;
        globalStrings = NULL;
        globalFloats = NULL;
    }
    smCurrentCodeBlock = saveCodeBlock;
    if (saveCodeBlock && saveCodeBlock->name)
    {
        Con::gCurrentFile = saveCodeBlock->name;
        Con::gCurrentRoot = saveCodeBlock->mRoot;
    }

    decRefCount();
    return STR.getStringValue();
}

//------------------------------------------------------------
