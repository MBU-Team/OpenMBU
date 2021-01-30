//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShapeInstance.h"

//-------------------------------------------------------------------------------------
// Dump shape structure:
//-------------------------------------------------------------------------------------

#define dumpLine(buffer) {str = buffer; stream.write((int)dStrlen(str),str);}

void TSShapeInstance::dumpDecals(Stream& stream, S32 indent, MeshObjectInstance* mesh)
{
    // search for any decals
    const char* str;
    for (S32 i = 0; i < mDecalObjects.size(); i++)
    {
        DecalObjectInstance* decal = &mDecalObjects[i];
        if (decal->targetObject != mesh)
            continue;

        // we have a decal

        // get name
        const char* decalName = "";
        if (decal->decalObject->nameIndex != -1)
            decalName = mShape->getName(decal->decalObject->nameIndex);

        // indent...
        char buf[1024];
        dMemset(buf, ' ', indent);
        buf[indent] = '\0';
        dumpLine(buf);

        // dump object name
        dumpLine(avar("  Decal named \"%s\" on above object.\r\n", decalName));
    }
}

void TSShapeInstance::dumpNode(Stream& stream, S32 level, S32 nodeIndex, Vector<S32>& detailSizes)
{
    if (nodeIndex < 0)
        return;

    S32 i;
    const char* str;
    char space[256];
    for (i = 0; i < level * 3; i++)
        space[i] = ' ';
    space[level * 3] = '\0';

    const char* nodeName = "";
    const TSShape::Node& node = mShape->nodes[nodeIndex];
    if (node.nameIndex != -1)
        nodeName = mShape->getName(node.nameIndex);
    dumpLine(avar("%s%s", space, nodeName));

    // find all the objects that hang off this node...
    Vector<ObjectInstance*> objectList;
    for (i = 0; i < mMeshObjects.size(); i++)
        if (mMeshObjects[i].nodeIndex == nodeIndex)
            objectList.push_back(&mMeshObjects[i]);

    if (objectList.size() == 0)
        dumpLine("\r\n");

    S32 spaceCount = -1;
    for (S32 j = 0; j < objectList.size(); j++)
    {
        // should be a dynamic cast, but MSVC++ has problems with this...
        MeshObjectInstance* obj = (MeshObjectInstance*)(objectList[j]);
        if (!obj)
            continue;

        // object name
        const char* objectName = "";
        if (obj->object->nameIndex != -1)
            objectName = mShape->getName(obj->object->nameIndex);

        // more spaces if this is the second object on this node
        if (spaceCount > 0)
        {
            char buf[1024];
            dMemset(buf, ' ', spaceCount);
            buf[spaceCount] = '\0';
            dumpLine(buf);
        }

        // dump object name
        dumpLine(avar(" --> Object %s with following details: ", objectName));

        // dump object detail levels
        for (S32 k = 0; k < obj->object->numMeshes; k++)
        {
            S32 f = obj->object->startMeshIndex;
            if (mShape->meshes[f + k])
                dumpLine(avar(" %i", detailSizes[k]));
        }

        dumpLine("\r\n");

        // how many spaces should we prepend if we have another object on this node
        if (spaceCount < 0)
            spaceCount = (S32)(dStrlen(space) + dStrlen(nodeName));

        // dump any decals
        dumpDecals(stream, spaceCount + 3, obj);
    }

    // search for children
    for (S32 k = nodeIndex + 1; k < mShape->nodes.size(); k++)
    {
        if (mShape->nodes[k].parentIndex == nodeIndex)
            // this is our child
            dumpNode(stream, level + 1, k, detailSizes);
    }
}

void TSShapeInstance::dump(Stream& stream)
{
    S32 i, j, ss, od, sz;
    const char* name;
    const char* str;

    dumpLine("\r\nShape Hierarchy:\r\n");

    dumpLine("\r\n   Details:\r\n");

    for (i = 0; i < mShape->details.size(); i++)
    {
        const TSDetail& detail = mShape->details[i];
        name = mShape->getName(detail.nameIndex);
        ss = detail.subShapeNum;
        od = detail.objectDetailNum;
        sz = (S32)detail.size;
        dumpLine(avar("      %s, Subtree %i, objectDetail %i, size %i\r\n", name, ss, od, sz));
    }

    dumpLine("\r\n   Subtrees:\r\n");

    for (i = 0; i < mShape->subShapeFirstNode.size(); i++)
    {
        S32 a = mShape->subShapeFirstNode[i];
        S32 b = a + mShape->subShapeNumNodes[i];
        dumpLine(avar("      Subtree %i\r\n", i));

        // compute detail sizes for each subshape
        Vector<S32> detailSizes;
        for (S32 l = 0; l < mShape->details.size(); l++)
        {
            if (mShape->details[l].subShapeNum == i)
                detailSizes.push_back((S32)mShape->details[l].size);
        }

        for (j = a; j < b; j++)
        {
            const TSNode& node = mShape->nodes[j];
            // if the node has a parent, it'll get dumped via the parent
            if (node.parentIndex < 0)
                dumpNode(stream, 3, j, detailSizes);
        }
    }

    bool foundSkin = false;
    for (i = 0; i < mShape->objects.size(); i++)
    {
        if (mShape->objects[i].nodeIndex < 0) // must be a skin
        {
            if (!foundSkin)
                dumpLine("\r\n   Skins:\r\n");
            foundSkin = true;
            const char* skinName = "";
            S32 nameIndex = mShape->objects[i].nameIndex;
            if (nameIndex >= 0)
                skinName = mShape->getName(nameIndex);
            dumpLine(avar("      Skin %s with following details: ", skinName));
            for (S32 num = 0; num < mShape->objects[i].numMeshes; num++)
            {
                if (mShape->meshes[num])
                    dumpLine(avar(" %i", (S32)mShape->details[num].size));
            }
            dumpLine("\r\n");
        }
    }
    if (foundSkin)
        dumpLine("\r\n");

    dumpLine("\r\n   Sequences:\r\n");
    for (i = 0; i < mShape->sequences.size(); i++)
    {
        const char* name = "(none)";
        if (mShape->sequences[i].nameIndex != -1)
            name = mShape->getName(mShape->sequences[i].nameIndex);
        dumpLine(avar("      %3d: %s\r\n", i, name));
    }

    /*
       if (mShape->materialList)
       {
          TSMaterialList * ml = mShape->materialList;
          dumpLine("\r\n   Material list:\r\n");
          for (i=0; i<(S32)ml->getMaterialCount(); i++)
          {
             U32 flags = ml->getFlags(i);
             const char * name = ml->getMaterialName(i);
             dumpLine(avar("   material #%i: \"%s\"%s.",i,name ? ml->getMaterialName(i) : "",flags&(TSMaterialList::S_Wrap|TSMaterialList::T_Wrap) ? "" : " not tiled"));
             if (flags & TSMaterialList::IflMaterial)
                dumpLine("  Place holder for ifl.");
             if (flags & TSMaterialList::IflFrame)
                dumpLine("  Ifl frame.");
             if (flags & TSMaterialList::DetailMapOnly)
                dumpLine("  Used as a detail map.");
             if (flags & TSMaterialList::BumpMapOnly)
                dumpLine("  Used as a bump map.");
             if (flags & TSMaterialList::ReflectanceMapOnly)
                dumpLine("  Used as a reflectance map.");
             if (flags & TSMaterialList::Translucent)
             {
                if (flags & TSMaterialList::Additive)
                   dumpLine("  Additive-translucent.")
                else if (flags & TSMaterialList::Subtractive)
                   dumpLine("  Subtractive-translucent.")
                else
                   dumpLine("  Translucent.")
             }
             dumpLine("\r\n");
          }
       }
    */
}

