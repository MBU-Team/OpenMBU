//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/consoleObject.h"
#include "core/stringTable.h"
#include "core/crc.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/typeValidators.h"

AbstractClassRep* AbstractClassRep::classLinkList = NULL;
static AbstractClassRep::FieldList sg_tempFieldList;
U32                                AbstractClassRep::NetClassCount[NetClassGroupsCount][NetClassTypesCount] = { {0, }, };
U32                                AbstractClassRep::NetClassBitSize[NetClassGroupsCount][NetClassTypesCount] = { {0, }, };

AbstractClassRep** AbstractClassRep::classTable[NetClassGroupsCount][NetClassTypesCount];

U32                                AbstractClassRep::classCRC[NetClassGroupsCount] = { INITIAL_CRC_VALUE, };
bool                               AbstractClassRep::initialized = false;

//--------------------------------------
const AbstractClassRep::Field* AbstractClassRep::findField(StringTableEntry name) const
{
    for (U32 i = 0; i < mFieldList.size(); i++)
        if (mFieldList[i].pFieldname == name)
            return &mFieldList[i];

    return NULL;
}

//--------------------------------------
void AbstractClassRep::registerClassRep(AbstractClassRep* in_pRep)
{
    AssertFatal(in_pRep != NULL, "AbstractClassRep::registerClassRep was passed a NULL pointer!");

#ifdef TORQUE_DEBUG  // assert if this class is already registered.
    for (AbstractClassRep* walk = classLinkList; walk; walk = walk->nextClass)
    {
        AssertFatal(dStrcmp(in_pRep->mClassName, walk->mClassName),
            "Duplicate class name registered in AbstractClassRep::registerClassRep()");
    }
#endif

    in_pRep->nextClass = classLinkList;
    classLinkList = in_pRep;
}

//--------------------------------------

ConsoleObject* AbstractClassRep::create(const char* in_pClassName)
{
    AssertFatal(initialized,
        "AbstractClassRep::create() - Tried to create an object before AbstractClassRep::initialize().");

    for (AbstractClassRep* walk = classLinkList; walk; walk = walk->nextClass)
        if (!dStrcmp(walk->getClassName(), in_pClassName))
            return walk->create();

    AssertWarn(0, avar("Couldn't find class rep for dynamic class: %s", in_pClassName));
    return NULL;
}

//--------------------------------------
ConsoleObject* AbstractClassRep::create(const U32 groupId, const U32 typeId, const U32 in_classId)
{
    AssertFatal(initialized,
        "AbstractClassRep::create() - Tried to create an object before AbstractClassRep::initialize().");
    AssertFatal(in_classId < NetClassCount[groupId][typeId],
        "AbstractClassRep::create() - Class id out of range.");
    AssertFatal(classTable[groupId][typeId][in_classId] != NULL,
        "AbstractClassRep::create() - No class with requested ID type.");

    // Look up the specified class and create it.
    if (classTable[groupId][typeId][in_classId])
        return classTable[groupId][typeId][in_classId]->create();

    return NULL;
}

//--------------------------------------

static S32 QSORT_CALLBACK ACRCompare(const void* aptr, const void* bptr)
{
    const AbstractClassRep* a = *((const AbstractClassRep**)aptr);
    const AbstractClassRep* b = *((const AbstractClassRep**)bptr);

    if (a->mClassType != b->mClassType)
        return a->mClassType - b->mClassType;
    return dStrcmp(a->getClassName(), b->getClassName());
}

void AbstractClassRep::initialize()
{
    AssertFatal(!initialized, "Duplicate call to AbstractClassRep::initialize()!");
    Vector<AbstractClassRep*> dynamicTable(__FILE__, __LINE__);

    AbstractClassRep* walk;

    // Initialize namespace references...
    for (walk = classLinkList; walk; walk = walk->nextClass)
    {
        walk->mNamespace = Con::lookupNamespace(StringTable->insert(walk->getClassName()));
        walk->mNamespace->mClassRep = walk;
    }

    // Initialize field lists... (and perform other console registration).
    for (walk = classLinkList; walk; walk = walk->nextClass)
    {
        // sg_tempFieldList is used as a staging area for field lists
        // (see addField, addGroup, etc.)
        sg_tempFieldList.setSize(0);

        walk->init();

        // So if we have things in it, copy it over...
        if (sg_tempFieldList.size() != 0)
            walk->mFieldList = sg_tempFieldList;

        // And of course delete it every round.
        sg_tempFieldList.clear();
    }

    // Calculate counts and bit sizes for the various NetClasses.
    for (U32 group = 0; group < NetClassGroupsCount; group++)
    {
        U32 groupMask = 1 << group;

        // Specifically, for each NetClass of each NetGroup...
        for (U32 type = 0; type < NetClassTypesCount; type++)
        {
            // Go through all the classes and find matches...
            for (walk = classLinkList; walk; walk = walk->nextClass)
            {
                if (walk->mClassType == type && walk->mClassGroupMask & groupMask)
                    dynamicTable.push_back(walk);
            }

            // Set the count for this NetGroup and NetClass
            NetClassCount[group][type] = dynamicTable.size();
            if (!NetClassCount[group][type])
                continue; // If no classes matched, skip to next.

             // Sort by type and then by name.
            dQsort((void*)&dynamicTable[0], dynamicTable.size(), sizeof(AbstractClassRep*), ACRCompare);

            // Allocate storage in the classTable
            classTable[group][type] = new AbstractClassRep * [NetClassCount[group][type]];

            // Fill this in and assign class ids for this group.
            for (U32 i = 0; i < NetClassCount[group][type]; i++)
            {
                classTable[group][type][i] = dynamicTable[i];
                dynamicTable[i]->mClassId[group] = i;
            }

            // And calculate the size of bitfields for this group and type.
            NetClassBitSize[group][type] =
                getBinLog2(getNextPow2(NetClassCount[group][type] + 1));

            dynamicTable.clear();
        }
    }

    // Ok, we're golden!
    initialized = true;

}

//------------------------------------------------------------------------------
//-------------------------------------- ConsoleObject

char replacebuf[1024];
char* suppressSpaces(const char* in_pname)
{
    U32 i = 0;
    char chr;
    do
    {
        chr = in_pname[i];
        replacebuf[i++] = (chr != 32) ? chr : '_';
    } while (chr);

    return replacebuf;
}

void ConsoleObject::addGroup(const char* in_pGroupname, const char* in_pGroupDocs)
{
    // Remove spaces.
    char* pFieldNameBuf = suppressSpaces(in_pGroupname);

    // Append group type to fieldname.
    dStrcat(pFieldNameBuf, "_begingroup");

    // Create Field.
    AbstractClassRep::Field f;
    f.pFieldname = StringTable->insert(pFieldNameBuf);
    f.pGroupname = StringTable->insert(in_pGroupname);

    if (in_pGroupDocs)
        f.pFieldDocs = StringTable->insert(in_pGroupDocs);
    else
        f.pFieldDocs = NULL;

    f.type = AbstractClassRep::StartGroupFieldType;
    f.elementCount = 0;
    f.groupExpand = false;
    f.validator = NULL;

    // Add to field list.
    sg_tempFieldList.push_back(f);
}

void ConsoleObject::endGroup(const char* in_pGroupname)
{
    // Remove spaces.
    char* pFieldNameBuf = suppressSpaces(in_pGroupname);

    // Append group type to fieldname.
    dStrcat(pFieldNameBuf, "_endgroup");

    // Create Field.
    AbstractClassRep::Field f;
    f.pFieldname = StringTable->insert(pFieldNameBuf);
    f.pGroupname = StringTable->insert(in_pGroupname);
    f.pFieldDocs = NULL;
    f.type = AbstractClassRep::EndGroupFieldType;
    f.groupExpand = false;
    f.validator = NULL;
    f.elementCount = 0;

    // Add to field list.
    sg_tempFieldList.push_back(f);
}

void ConsoleObject::addField(const char* in_pFieldname,
    const U32 in_fieldType,
    const dsize_t in_fieldOffset,
    const char* in_pFieldDocs)
{
    addField(
        in_pFieldname,
        in_fieldType,
        in_fieldOffset,
        1,
        NULL,
        in_pFieldDocs);
}

void ConsoleObject::addField(const char* in_pFieldname,
    const U32 in_fieldType,
    const dsize_t in_fieldOffset,
    const U32 in_elementCount,
    EnumTable* in_table,
    const char* in_pFieldDocs)
{
    AbstractClassRep::Field f;
    f.pFieldname = StringTable->insert(in_pFieldname);
    f.pGroupname = NULL;

    if (in_pFieldDocs)
        f.pFieldDocs = StringTable->insert(in_pFieldDocs);
    else
        f.pFieldDocs = NULL;

    f.type = in_fieldType;
    f.offset = in_fieldOffset;
    f.elementCount = in_elementCount;
    f.table = in_table;
    f.validator = NULL;

    sg_tempFieldList.push_back(f);
}

void ConsoleObject::addFieldV(const char* in_pFieldname,
    const U32 in_fieldType,
    const dsize_t in_fieldOffset,
    TypeValidator* v,
    const char* in_pFieldDocs)
{
    AbstractClassRep::Field f;
    f.pFieldname = StringTable->insert(in_pFieldname);
    f.pGroupname = NULL;
    if (in_pFieldDocs)
        f.pFieldDocs = StringTable->insert(in_pFieldDocs);
    else
        f.pFieldDocs = NULL;
    f.type = in_fieldType;
    f.offset = in_fieldOffset;
    f.elementCount = 1;
    f.table = NULL;
    f.validator = v;
    v->fieldIndex = sg_tempFieldList.size();

    sg_tempFieldList.push_back(f);
}

void ConsoleObject::addDepricatedField(const char* fieldName)
{
    AbstractClassRep::Field f;
    f.pFieldname = StringTable->insert(fieldName);
    f.pGroupname = NULL;
    f.pFieldDocs = NULL;
    f.type = AbstractClassRep::DepricatedFieldType;
    f.offset = 0;
    f.elementCount = 0;
    f.table = NULL;
    f.validator = NULL;

    sg_tempFieldList.push_back(f);
}


bool ConsoleObject::removeField(const char* in_pFieldname)
{
    for (U32 i = 0; i < sg_tempFieldList.size(); i++) {
        if (dStricmp(in_pFieldname, sg_tempFieldList[i].pFieldname) == 0) {
            sg_tempFieldList.erase(i);
            return true;
        }
    }

    return false;
}

//--------------------------------------
void ConsoleObject::initPersistFields()
{
}

//--------------------------------------
void ConsoleObject::consoleInit()
{
}

ConsoleObject::~ConsoleObject()
{
}

//--------------------------------------
AbstractClassRep* ConsoleObject::getClassRep() const
{
    return NULL;
}


ConsoleFunction(dumpNetStats, void, 1, 1, "")
{
#ifdef TORQUE_NET_STATS
    for (AbstractClassRep* rep = AbstractClassRep::getClassList(); rep; rep = rep->getNextClass())
    {
        if (rep->mNetStatPack.numEvents || rep->mNetStatUnpack.numEvents || rep->mNetStatWrite.numEvents || rep->mNetStatRead.numEvents)
        {
            Con::errorf("class %s net info", rep->getClassName());
            if (rep->mNetStatPack.numEvents)
                Con::errorf("   packUpdate: avg (%f), min (%i), max (%i), num (%i)",
                    F32(rep->mNetStatPack.total) / F32(rep->mNetStatPack.numEvents),
                    rep->mNetStatPack.min,
                    rep->mNetStatPack.max,
                    rep->mNetStatPack.numEvents);
            if (rep->mNetStatUnpack.numEvents)
                Con::errorf("   unpackUpdate: avg (%f), min (%i), max (%i), num (%i)",
                    F32(rep->mNetStatUnpack.total) / F32(rep->mNetStatUnpack.numEvents),
                    rep->mNetStatUnpack.min,
                    rep->mNetStatUnpack.max,
                    rep->mNetStatUnpack.numEvents);
            if (rep->mNetStatWrite.numEvents)
                Con::errorf("   write: avg (%f), min (%i), max (%i), num (%i)",
                    F32(rep->mNetStatWrite.total) / F32(rep->mNetStatWrite.numEvents),
                    rep->mNetStatWrite.min,
                    rep->mNetStatWrite.max,
                    rep->mNetStatWrite.numEvents);
            if (rep->mNetStatRead.numEvents)
                Con::errorf("   read: avg (%f), min (%i), max (%i), num (%i)",
                    F32(rep->mNetStatRead.total) / F32(rep->mNetStatRead.numEvents),
                    rep->mNetStatRead.min,
                    rep->mNetStatRead.max,
                    rep->mNetStatRead.numEvents);
            S32 sum = 0;
            for (S32 i = 0; i < 32; i++)
                sum += rep->mDirtyMaskFrequency[i];
            if (sum)
            {
                Con::errorf("   Mask bits:");
                for (S32 i = 0; i < 8; i++)
                {
                    F32 avg0 = rep->mDirtyMaskFrequency[i] ? F32(rep->mDirtyMaskTotal[i]) / F32(rep->mDirtyMaskFrequency[i]) : 0.0f;
                    F32 avg8 = rep->mDirtyMaskFrequency[i + 8] ? F32(rep->mDirtyMaskTotal[i + 8]) / F32(rep->mDirtyMaskFrequency[i + 8]) : 0.0f;
                    F32 avg16 = rep->mDirtyMaskFrequency[i + 16] ? F32(rep->mDirtyMaskTotal[i + 16]) / F32(rep->mDirtyMaskFrequency[i + 16]) : 0.0f;
                    F32 avg24 = rep->mDirtyMaskFrequency[i + 24] ? F32(rep->mDirtyMaskTotal[i + 24]) / F32(rep->mDirtyMaskFrequency[i + 24]) : 0.0f;
                    Con::errorf("      %2i - %4i (%6.2f)     %2i - %4i (%6.2f)     %2i - %4i (%6.2f)     %2i - %4i, (%6.2f)",
                        i, rep->mDirtyMaskFrequency[i], avg0,
                        i + 8, rep->mDirtyMaskFrequency[i + 8], avg8,
                        i + 16, rep->mDirtyMaskFrequency[i + 16], avg16,
                        i + 24, rep->mDirtyMaskFrequency[i + 24], avg24);
                }
            }
        }
        rep->resetNetStats();
    }
#endif
}

