//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2dtsExporter/SceneEnum.h"
#include "max2dtsExporter/Sequence.h"
#include "platform/platformAssert.h"
#include "core/filestream.h"
#include "max2dtsExporter/exporter.h"
#include "ts/TSShapeInstance.h"
#include <time.h>
#include "max2dtsExporter/maxUtil.h"

#pragma pack(push,8)
#include <ISTDPLUG.H>
#pragma pack(pop)

U32 dumpMask;

U32 allowEmptySubtrees;
U32 allowCrossedDetails;
U32 allowUnusedMeshes;
U32 transformCollapse;
U32 viconNeeded;
U32 enableSequences;
U32 allowOldSequences;
U32 enableTwoSidedMaterials;
F32 animationDelta;
F32 maxFrameRate;
S32 weightsPerVertex;
F32 weightThreshhold;
char baseTexturePath[256];
S32 t2AutoDetail;

void setBaseTexturePath(const char * str)
{
   dStrcpy(baseTexturePath,str);
   // if first character is '.', then chop everything else off
   if (*baseTexturePath=='.')
   {
      baseTexturePath[1]='\0';
      return;
   }
   // make sure path ends in '/'
   S32 len = dStrlen(baseTexturePath);
   if (baseTexturePath[len-1]!='\\')
   {
      baseTexturePath[len]   = '\\';
      baseTexturePath[len+1] = '\0';
   }
}

FileStream SceneEnumProc::file;

Vector<char *> SceneEnumProc::alwaysExport;
Vector<char *> SceneEnumProc::neverExport;
Vector<char *> SceneEnumProc::neverAnimate;

char SceneEnumProc::exportType = ' ';

//--------------------------------------------------------------

bool SceneEnumProc::isDummy(INode *pNode)
{
   return !dStrnicmp(pNode->GetName(), "dummy", 5);
}

bool SceneEnumProc::isBounds(INode *pNode)
{
   return !dStrnicmp(pNode->GetName(), "bounds", 6);
}

bool SceneEnumProc::isCamera(INode *pNode)
{
    return !dStrnicmp(pNode->GetName(), "camera", 6);
}

bool SceneEnumProc::isVICON(INode *pNode)
{
   return !dStrncmp(pNode->GetName(), "VICON", 5) || !dStricmp(pNode->GetName(), "Bip01");
}

bool SceneEnumProc::isSubtree(INode *pNode)
{
    return pNode->GetParentNode()->IsRootNode() && !hasMesh(pNode);
}

bool SceneEnumProc::isBillboard(INode * pNode)
{
   return !dStrncmp(pNode->GetName(),"BB::",4);
}

bool SceneEnumProc::isBillboardZAxis(INode * pNode)
{
   return !dStrncmp(pNode->GetName(),"BBZ::",5);
}

bool SceneEnumProc::isSortedObject(INode * pNode)
{
   return !dStrnicmp(pNode->GetName(),"SORT::",6);
}

bool SceneEnumProc::isDecal(INode * pNode)
{
   return !dStrnicmp(pNode->GetName(),"Decal::",7);
}

void SceneEnumProc::tweakName(const char ** name)
{
   if (!dStrncmp(*name,"BB::",4))
      *name += 4;
   else if (!dStrncmp(*name,"BBZ::",5))
      *name += 5;
   else if (!dStrncmp(*name,"SORT::",6))
      *name += 6;
}

//--------------------------------------------------------------

void SceneEnumProc::printDump(U32 mask, const char * buffer)
{
    if (mask & dumpMask && file.getStatus()==Stream::Ok)
        file.write(strlen(buffer),buffer);
}

//--------------------------------------------------------------

bool SceneEnumProc::isEmpty()
{
    return subTrees.empty() && !viconNode;
}

//--------------------------------------------------------------

SceneEnumProc::SceneEnumProc()
{
    // initialize variables
    pShape     = NULL;
    boundsNode = NULL;
    viconNode  = NULL;
    exportError = false;
    exportErrorStr = NULL;
}

//--------------------------------------------------------------

void SceneEnumProc::startDump(const TCHAR * filename, const TCHAR * maxFile)
{
    if (!dumpMask)
        return;

    // add dump file to same directory as shape
    char dumpFilename[256];
    dStrcpy(dumpFilename,filename);
    char * p  = dStrrchr(dumpFilename,'\\');
    char * p2 = dStrrchr(dumpFilename,':');
    if (p && *p=='\\')
       dStrcpy(p+1,"dump.dmp");
    else if (p2 && *p2==':')
       dStrcpy(p2+1,"dump.dmp");
    else
       dStrcpy(dumpFilename,"dump.dmp");

    // get rid of old dump file -- file stream's don't deal with this well
    File zap;
    zap.open(dumpFilename,File::Write);
    zap.close();

    // open debugging file:
    file.open(dumpFilename,FileStream::Write);
    if (file.getStatus() != File::Ok)
    {
       setExportError("Unable to open dump file.");
       return;
    }

    // add info to dump file so we know what file it is for
    printDump(PDAlways,avar("Max file %s exported to %s\r\n",maxFile,filename));

    // add time and date to dump file
    time_t timer;
    struct tm *tblock;
    timer = time(NULL);
    tblock = localtime(&timer);
    printDump(PDAlways,avar("Exported on %s\r\n",asctime(tblock)));
}

//--------------------------------------------------------------

void SceneEnumProc::enumScene(IScene *scene)
{
    printDump(PDPass1,"First pass: collect useful nodes...\r\n\r\n");

    scene->EnumTree(this);

    // if already had an error, exit now
    if (isExportError())
        return;

    // Make sure they have a bounding box
    if (!boundsNode)
    {
        setExportError("No bounding box found");
        return;
    }
    if (boundsNode->NumberOfChildren())
    {
        setExportError("Error:  Bounds node must not have children.");
        return;
    }

    // Make sure they have a vicon node if required
    if (viconNeeded && !viconNode)
    {
        setExportError("No vicon node found");
        return;
    }
}

//--------------------------------------------------------------

SceneEnumProc::~SceneEnumProc()
{
    // close debugging file:
    file.close();

    shapeMimic.destroyShape(pShape);
    pShape = NULL;

    dFree(exportErrorStr);

    clearExportConfig();
}

//--------------------------------------------------------------

// only used for testing ...
TSShape * SceneEnumProc::regenShape()
{
    delete pShape;
    pShape = NULL;

    // generate the shape
    pShape = shapeMimic.generateShape();

    // be sure to propagate any errors
    if (shapeMimic.isError())
        setExportError(shapeMimic.getError());

    return pShape;
}

//--------------------------------------------------------------
// callback for EnumTree:

S32 SceneEnumProc::callback(INode *pNode)
{
    // At this stage we do not need to collect all the nodes
    // because the tree structure will still be there when we
    // build the shape.  What we need to do right now is grab
    // the top of all the subtrees, the vicon node if it exists,
    // any meshes hanging on the root level (these will be lower
    // detail levels, we don't need to grab meshes on the sub-trees
    // because they will be found when we recurse into the sub-tree),
    // the bounds node, always nodes, and any sequences.

    // NOTE:  excessive return statements here -- the idea is to make
    //        it easier to follow flow...hopefully is doesn't backfire.

    const char *name  = pNode->GetName();
    const char *pname = pNode->GetParentNode()->GetName();

    printDump(PDPass1,avar("Processing Node %s with parent %s\r\n", name, pname));

    ObjectState os = pNode->EvalWorldState(0);

    // If it is a sequence, get the sequence
    if (os.obj->ClassID() == SEQUENCE_CLASS_ID || (allowOldSequences && os.obj->ClassID() == OLD_SEQUENCE_CLASS_ID))
        return getSequence(pNode);

    if (isVICON(pNode))
    {
        // We are the VICON...coo-coo ka-choo
        printDump(PDPass1,avar("Found VICON node (%s)\r\n",name));
        viconNode = pNode;
        return TREE_CONTINUE;
    }

    if (isCamera(pNode))
        // just ignore it
        return TREE_CONTINUE;

    if (isSubtree(pNode))
    {
        // Add this node to the subtree list...
        printDump(PDPass1,avar("Found subtree starting at Node \"%s\"\r\n",name));
        subTrees.push_back(pNode);
        return TREE_CONTINUE;
    }

    // See if it is a bounding box.  If so, save it as THE bounding
    // box for the scene
    if (isBounds(pNode))
    {
        if (boundsNode)
        {
            setExportError("More than one bounds node found.");
            return TREE_ABORT;
        }
        printDump(PDPass1,"Bounding box found\r\n");
        boundsNode = pNode;
    }
    else if (hasSkin(pNode))
    {
        if (!pNode->GetParentNode()->IsRootNode())
        {
            setExportError(avar("Skin found on linked node \"%s\" -- skin must be unlinked.",name));
            return TREE_ABORT;
        }
        // skin...
        printDump(PDPass1,avar("Skin Node \"%s\" with parent \"%s\" added to entry list\r\n",name,pname));
        skinNodes.push_back(pNode);
    }
    else if (hasMesh(pNode) && pNode->GetParentNode()->IsRootNode())
    {
        // A node with a mesh hanging on the root...
        // it's probably a lower detail version of a mesh in the shape
        printDump(PDPass1,avar("Mesh Node \"%s\" with parent \"%s\" added to entry list\r\n",name,pname));
        meshNodes.push_back(pNode);
    }

    return TREE_CONTINUE;
}

//--------------------------------------------------------------

S32 SceneEnumProc::getSequence(INode *pNode)
{
    // We could just push this onto the sequences list, but
    // we'll do some validity testing first.

    // Sequences must have "root" as parent:
    if (!pNode->GetParentNode()->IsRootNode())
    {
        setExportError("Sequence defined off the \"root\" node.");
        return TREE_ABORT;
    }

    Object *ob = pNode->GetObjectRef();

    // the following should never happen...
    if (ob->ClassID() != SEQUENCE_CLASS_ID && ob->ClassID() != OLD_SEQUENCE_CLASS_ID)
    {
       setExportError("Error in getSequencee: report to programmer");
       return TREE_ABORT;
    }

    // Do extra check to make sure sequence is valid
    IParamBlock *pblock = (IParamBlock *)ob->GetReference(0);
    Control *control = pblock->GetController(PB_SEQ_BEGIN_END);
    if (!control)
    {
        setExportError(avar("Sequence \"%s\" is missing keyframes to mark start and end",pNode->GetName()));
        return TREE_ABORT;
    }

    sequences.push_back(pNode);
    printDump(PDPass1,avar("Sequence %s found.\r\n",pNode->GetName()));

    return TREE_CONTINUE;
}

void SceneEnumProc::getPolyCountSubtree(INode * node, S32 & polyCount, F32 & minVolPerPoly, F32 & maxVolPerPoly)
{
   F32 vol;
   S32 pc;
   vol = findVolume(node,pc);
   if (pc>0)
   {
      F32 volPerPoly = vol/(F32)pc;
      if (volPerPoly<minVolPerPoly)
         minVolPerPoly = volPerPoly;
      else if (volPerPoly>maxVolPerPoly)
         maxVolPerPoly = volPerPoly;
   }
   polyCount += pc;
   for (S32 i=0; i<node->NumberOfChildren(); i++)
      getPolyCountSubtree(node->GetChildNode(i),polyCount,minVolPerPoly,maxVolPerPoly);
}

void SceneEnumProc::getPolyCount(S32 & polyCount, F32 & minVolPerPoly, F32 & maxVolPerPoly)
{
   polyCount = 0;
   minVolPerPoly = 10E30f;
   maxVolPerPoly = -10E30F;
   if (viconNode)
      getPolyCountSubtree(viconNode,polyCount,minVolPerPoly,maxVolPerPoly);

   for (S32 i=0; i<subTrees.size(); i++)
      getPolyCountSubtree(subTrees[i],polyCount,minVolPerPoly,maxVolPerPoly);
}

//--------------------------------------------------------------
// We've now scanned in the nodes (saving off what we need) via
// Max repeatedly calling callback(INode *) method...
// Now put the shape together.
void SceneEnumProc::processScene()
{
    S32 i;

    printDump(PDPass2,"\r\nSecond pass:  put shape structure together...\r\n\r\n");

    // get poly count -- this will be used by t2AutoDetail method
    S32 polyCount;
    F32 minVolPerPoly,maxVolPerPoly;
    getPolyCount(polyCount,minVolPerPoly,maxVolPerPoly);

    // eventually we should get a better way of doing this...
    shapeMimic.setGlobals(boundsNode,polyCount,minVolPerPoly,maxVolPerPoly);

    // set up bounds node
    shapeMimic.addBounds(boundsNode);

    // add the VICON subtree
    if (viconNode)
        shapeMimic.addSubtree(viconNode);

    // add other subtrees
    for (i=0; i<subTrees.size(); i++)
        shapeMimic.addSubtree(subTrees[i]);

    // add meshes
    for (i=0; i<meshNodes.size(); i++)
        shapeMimic.addMesh(meshNodes[i]);

    // add skin
    for (i=0; i<skinNodes.size(); i++)
        shapeMimic.addSkin(skinNodes[i]);

    // add sequences
    for (i=0; i<sequences.size(); i++)
        shapeMimic.addSequence(sequences[i]);

    shapeMimic.collapseTransforms();

    // generate the shape
    pShape = shapeMimic.generateShape();

    // clear lists for next time around
    shapeMimic.clearCollapseTransforms();

    // be sure to propagate any errors
    if (shapeMimic.isError())
        setExportError(shapeMimic.getError());
}

//--------------------------------------------------------------

bool checkDuplicateNodes(TSShape * pShape, S32 & dup1, S32 & dup2)
{
   S32 i,j;
   i = (dup1<0) ? 0 : dup1+1;
   for (; i < ((S32)pShape->nodes.size())-1; i++)
   {
      j = (dup2<0) ? i+1 : dup2+1;
      dup2=-1;
      for (; j<pShape->nodes.size(); j++)
      {
         if (pShape->nodes[i].nameIndex == pShape->nodes[j].nameIndex)
         {
            dup1 = i;
            dup2 = j;
            return true;
         }
      }
   }
   return false;
}

bool checkDuplicateObjects(TSShape * pShape, S32 & dup1, S32 & dup2)
{
   S32 i,j;
   i = (dup1<0) ? 0 : dup1+1;
   for (; i < ((S32)pShape->objects.size())-1; i++)
   {
      j = (dup2<0) ? i+1 : dup2+1;
      dup2=-1;
      for (; j<pShape->objects.size(); j++)
      {
         if (pShape->objects[i].nameIndex == pShape->objects[j].nameIndex)
         {
            dup1 = i;
            dup2 = j;
            return true;
         }
      }
   }
   return false;
}

//--------------------------------------------------------------

#define NumBoolParams 31

char * BoolParamNames[NumBoolParams] =
{
    "Dump::NodeCollection",
    "Dump::ShapeConstruction",
    "Dump::NodeCulling",
    "Dump::NodeStates",
    "Dump::NodeStateDetails",
    "Dump::ObjectStates",
    "Dump::ObjectStateDetails",
    "Dump::ObjectOffsets",
    "Dump::SequenceDetails",
    "Dump::ShapeHierarchy",
    "Error::AllowEmptySubtrees",
    "Error::AllowCrossedDetails",
    "Error::AllowUnusedMeshes",
    "Error::AllowOldSequences",
    "Error::RequireViconNode",
    "Param::EnableTwoSidedMaterials",
    "Param::CollapseTransforms",
    "Param::SequenceExport",
    "Sequence::defaultCyclic",
    "Sequence::defaultBlend",
    "Sequence::defaultFirstLastFrameSame",
    "Sequence::defaultUseFrameRate",
    "Sequence::defaultIgnoreGroundTransform",
    "Sequence::defaultUseGroundFrameRate",
    "Sequence::defaultEnableMorphAnimation",
    "Sequence::defaultEnableVisAnimation",
    "Sequence::defaultEnableTransformAnimation",
    "Sequence::defaultForceMorphAnimation",
    "Sequence::defaultForceVisAnimation",
    "Sequence::defaultForceTransformAnimation",
    "Sequence::defaultOverrideDuration"
};

U32 * BoolParams[NumBoolParams] =
{
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    &dumpMask,
    (U32*)&allowEmptySubtrees,
    (U32*)&allowCrossedDetails,
    (U32*)&allowUnusedMeshes,
    (U32*)&allowOldSequences,
    (U32*)&viconNeeded,
    (U32*)&enableTwoSidedMaterials,
    (U32*)&transformCollapse,
    (U32*)&enableSequences,
    (U32*)&SequenceObject::defaultCyclic,
    (U32*)&SequenceObject::defaultBlend,
    (U32*)&SequenceObject::defaultFirstLastFrameSame,
    (U32*)&SequenceObject::defaultUseFrameRate,
    (U32*)&SequenceObject::defaultIgnoreGroundTransform,
    (U32*)&SequenceObject::defaultUseGroundFrameRate,
    (U32*)&SequenceObject::defaultEnableMorphAnimation,
    (U32*)&SequenceObject::defaultEnableVisAnimation,
    (U32*)&SequenceObject::defaultEnableTransformAnimation,
    (U32*)&SequenceObject::defaultForceMorphAnimation,
    (U32*)&SequenceObject::defaultForceVisAnimation,
    (U32*)&SequenceObject::defaultForceTransformAnimation,
    (U32*)&SequenceObject::defaultOverrideDuration
};

U32 BoolParamBit[NumBoolParams] =
{
    PDPass1,
    PDPass2,
    PDPass3,
    PDNodeStates,
    PDNodeStateDetails,
    PDObjectStates,
    PDObjectStateDetails,
    PDObjectOffsets,
    PDSequences,
    PDShapeHierarchy,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

#define NumFloatParams 6

char * FloatParamNames[NumFloatParams] =
{
    "Params::AnimationDelta",
    "Params::MaxFrameRate",
    "Sequence::defaultFrameRate",
    "Sequence::defaultGroundFrameRate",
    "Params::SkinWeightThreshhold",
    "Sequence::defaultDuration"
};

F32 * FloatParams[NumFloatParams] =
{
    &animationDelta,
    &maxFrameRate,
    &SequenceObject::defaultFrameRate,
    &SequenceObject::defaultGroundFrameRate,
    &weightThreshhold,
    &SequenceObject::defaultDuration
};

#define NumIntParams 5

char * IntParamNames[NumIntParams] =
{
    "SequenceObject::defaultNumFrames",
    "SequenceObject::defaultNumGroundFrames",
    "SequenceObject::defaultDefaultSequencePriority",
    "Params::weightsPerVertex",
    "Params::T2AutoDetail"
};

S32 * IntParams[NumIntParams] =
{
    &SequenceObject::defaultNumFrames,
    &SequenceObject::defaultNumGroundFrames,
    &SequenceObject::defaultDefaultSequencePriority,
    &weightsPerVertex,
    &t2AutoDetail
};

#define NumStringParams 2

char testString[15] ="xxxxxxxxxxxxxx";

char * StringParamNames[NumStringParams] =
{
    "Test::test",
    "Params::baseTexturePath"
};

char * StringParams[NumStringParams] =
{
    testString,
    baseTexturePath
};

S32 StringParamMaxLen[NumStringParams] =
{
    5,
    200 // 256 really, but this should be enough
};

S32 getParamEntry(const char * name, char ** nameTable, S32 size)
{
    for (S32 i=0; i<size; i++)
    {
        if (!dStrcmp(name,nameTable[i]))
            return i;
    }
    return -1;
}

extern void findFiles(const char *, Vector<char*> &);
extern void clearFindFiles(Vector<char*>);

const char * SceneEnumProc::readConfigFile(const char * filename)
{
    clearExportConfig();

    Vector<char*> fnames;
    findFiles(filename,fnames);
    if (fnames.size()>1)
      // could potentially happen when not exporting, but act as if we are...
      return "Multiple config (*.cfg) files found in shape directory.";

    if (fnames.empty())
    {
        printDump(PDAlways,"\r\nConfig file not found.\r\n");
        return NULL;
    }

    // Build new file name out of passed file name and found file...
    char fileBuffer[256];
    dStrcpy(fileBuffer,filename);
    char * s = dStrrchr(fileBuffer,'\\');
    dStrcpy(s+1,fnames[0]);

    FileStream fs;
    fs.open(fileBuffer,FileStream::Read);
    if (fs.getStatus() != Stream::Ok)
    {
        printDump(PDAlways,avar("\r\nConfig file \"%s\" not found.\r\n",fileBuffer));
        return NULL;
    }

    printDump(PDAlways,avar("\r\nBegin reading config file \"%s\".\r\n",fileBuffer));

    S32 len = fs.getStreamSize();

    // read one line at a time
    char * buffer = new char[len+1];
    fs.read(len,buffer);
    fs.close();

    // make sure we're properly terminated
    buffer[len] = '\n';

    S32 mode = 0;
    char * pos = buffer;
    do
    {
        char * next = dStrchr(pos,'\n');
        *next='\0';

        // chop off trailing \r's
        char * ch = next-1;
        while (ch>pos && *ch=='\r')
        {
            *ch = '\0';
            ch--;
        }
        if (ch!=pos)
        {
            if (!dStricmp(pos,"AlwaysExport:"))
                mode = 0;
            else if (!dStricmp(pos,"NeverExport:"))
                mode = 1;
            else if (!dStricmp(pos,"NeverAnimate:"))
                mode = 2;
            else if (*pos=='+' || *pos=='-')
            {
                bool newVal = *pos=='+';
                S32 idx = getParamEntry(pos+1,BoolParamNames,NumBoolParams);
                if (idx>=0)
                {
                    if (BoolParamBit[idx])
                    {
                        if (newVal)
                            *BoolParams[idx] |= BoolParamBit[idx];
                        else
                            *BoolParams[idx] &= ~BoolParamBit[idx];
                    }
                    else
                        *BoolParams[idx] = newVal;
                    printDump(PDAlways,avar("%s %s.\r\n",BoolParamNames[idx],newVal ? "enabled" : "disabled"));
                }
                else
                    printDump(PDAlways,avar("Unknown bool parameter \"%s\"\r\n",pos+1));
            }
            else if (*pos=='=')
            {
                char * endName = strchr(pos+1,' ');
                if (endName)
                {
                    *endName = '\0';
                    S32 idx1 = getParamEntry(pos+1,FloatParamNames,NumFloatParams);
                    S32 idx2 = getParamEntry(pos+1,StringParamNames,NumStringParams);
                    S32 idx3 = getParamEntry(pos+1,IntParamNames,NumIntParams);
                    if (idx1>=0)
                    {
                        // Float
                        *FloatParams[idx1] = atof(endName+1);
                        printDump(PDAlways,avar("%s = %f\r\n",FloatParamNames[idx1],*FloatParams[idx1]));
                    }
                    if (idx2>=0)
                    {
                        // string
                        S32 maxLen = StringParamMaxLen[idx2];
                        dStrncpy(StringParams[idx2],endName+1,maxLen-1);
                        StringParams[idx2][maxLen]='\0';
                        printDump(PDAlways,avar("%s = \"%s\"\r\n",StringParamNames[idx2],StringParams[idx2]));
                    }
                    if (idx3>=0)
                    {
                        // S32
                        *IntParams[idx3] = atoi(endName+1);
                        printDump(PDAlways,avar("%s = %i\r\n",IntParamNames[idx3],*IntParams[idx3]));
                    }
                    if (idx1<0 && idx2<0 && idx3<0)
                        printDump(PDAlways,avar("Unknown parameter \"%\"\r\n",pos+1));
                }
            }
            else if (*pos!='\\' && *pos!='/' && *pos!=';')
            {
                if (mode == 0)
                {
                    alwaysExport.push_back(dStrdup(pos));
                    printDump(PDAlways,avar("Always export node: \"%s\"\r\n",pos));
                }
                else if (mode == 1)
                {
                    neverExport.push_back(dStrdup(pos));
                    printDump(PDAlways,avar("Never export node: \"%s\"\r\n",pos));
                }
                else if (mode == 2)
                {
                    neverAnimate.push_back(dStrdup(pos));
                    printDump(PDAlways,avar("Never animate transform on node: \"%s\"\r\n",pos));
                }
            }
        }

        pos = next+1;
    }
    while (pos < &buffer[len]);

    delete [] buffer;

    printDump(PDAlways,"End reading config file.\r\n");
    return NULL;
}

void SceneEnumProc::writeConfigFile(const char * filename)
{
    // make sure to get rid of old version first...
    // otherwise we might have a "collision"
    File zap;
    zap.open(filename,File::Write);
    zap.close();

    FileStream fs;
    fs.open(filename,FileStream::Write);
    if (fs.getStatus() != Stream::Ok)
        return;

    // just write the parameters...none of the config lists

    S32 i;
    char buffer[1024];

    for (i=0; i<NumBoolParams; i++)
    {
        bool enabled = BoolParamBit[i] ? BoolParamBit[i] & *BoolParams[i] : *BoolParams[i];
        dStrcpy(buffer,avar("%s%s\r\n",enabled ? "+" : "-",BoolParamNames[i]));
        fs.write(dStrlen(buffer),buffer);
    }

    for (i=0; i<NumFloatParams; i++)
    {
        dStrcpy(buffer,avar("=%s %f\r\n",FloatParamNames[i],*FloatParams[i]));
        fs.write(dStrlen(buffer),buffer);
    }

    for (i=0; i<NumIntParams; i++)
    {
        dStrcpy(buffer,avar("=%s %i\r\n",IntParamNames[i],*IntParams[i]));
        fs.write(dStrlen(buffer),buffer);
    }

    for (i=0; i<NumStringParams; i++)
    {
        dStrcpy(buffer,avar("=%s %s\r\n",StringParamNames[i],StringParams[i]));
        fs.write(dStrlen(buffer),buffer);
    }

    fs.close();
}

//--------------------------------------------------------------

void SceneEnumProc::clearExportConfig()
{
    S32 i;

    for (i=0; i<alwaysExport.size(); i++)
        dFree(alwaysExport[i]);
    alwaysExport.clear();

    for (i=0; i<neverExport.size(); i++)
        dFree(neverExport[i]);
    neverExport.clear();

    for (i=0; i<neverAnimate.size(); i++)
        dFree(neverAnimate[i]);
    neverAnimate.clear();
}

//--------------------------------------------------------------

#define dumpLine(buffer) { const char * str = buffer; file->write(dStrlen(str),str);}

void SceneEnumProc::exportTextFile(Stream * file)
{
    dumpLine("Nodes:\r\n");

    S32 i;
    for (i=0; i<pShape->nodes.size(); i++)
    {
        if (pShape->nodes[i].nameIndex<0)
        {
            dumpLine("<no name>");
        }
        else
            dumpLine(pShape->getName(pShape->nodes[i].nameIndex));
        dumpLine("\r\n");
    }

    dumpLine("\r\nObjects:\r\n");
    for (i=0; i<pShape->objects.size(); i++)
    {
        if (pShape->objects[i].nameIndex<0)
        {
            dumpLine("<no name>");
        }
        else
            dumpLine(pShape->getName(pShape->objects[i].nameIndex));
        dumpLine("\r\n");
    }
    dumpLine("\r\n");

   TSShapeInstance * pShapeInstance = new TSShapeInstance(pShape);
   pShapeInstance->dump(*file);
   delete pShapeInstance;
}

//--------------------------------------------------------------

bool SceneEnumProc::dumpShape(const char * filename)
{
   // now let's read the shape in, create a shape instance, and dump out its hierrarchy
   FileStream fs;
   fs.open(filename,FileStream::Read);
   if( fs.getStatus() != Stream::Ok )
      return false;

   TSShape * readShape = new TSShape;
   readShape->read(&fs);
   fs.close();

   TSShapeInstance * pShapeInstance = new TSShapeInstance(readShape);
   pShapeInstance->dump(file);

   // check for duplicate nodes
   printDump(PDShapeHierarchy,"\r\nChecking for duplicate nodes...\r\n");
   S32 dup1 = -1;
   S32 dup2 = -1;
   while (checkDuplicateNodes(readShape,dup1,dup2))
   {
      const char * str = readShape->getName(readShape->nodes[dup1].nameIndex);
      printDump(PDShapeHierarchy,avar("Node with name \"%s\" is duplicated.\r\n",str));
   }

   // check for duplicate objects
   printDump(PDShapeHierarchy,"\r\nChecking for duplicate objects...\r\n");
   dup1 = -1;
   dup2 = -1;
   while (checkDuplicateObjects(readShape,dup1,dup2))
   {
      const char * str = readShape->getName(readShape->objects[dup1].nameIndex);
      printDump(PDShapeHierarchy,avar("Object with name \"%s\" is duplicated.\r\n",str));
   }

   delete pShapeInstance;
   delete readShape;

   return true;
}

void SceneEnumProc::setInitialDefaults()
{
    dumpMask = PDPass1 | PDPass2 | PDPass3 |
               PDObjectOffsets             |
               PDNodeStates                |
               PDObjectStates              |
               PDNodeStateDetails          |
               PDObjectStateDetails        |
               PDSequences                 |
               PDShapeHierarchy;

    allowEmptySubtrees = true;
    allowCrossedDetails = false;
    allowUnusedMeshes = true;
    transformCollapse = true;
    viconNeeded = false;
    enableSequences = true;
    allowOldSequences = true;
    enableTwoSidedMaterials = true;
    animationDelta = 0.0001f;
    maxFrameRate = 30.0f;
    weightsPerVertex = 10;
    weightThreshhold = 0.001f;
    dStrcpy(baseTexturePath,".");

    resetSequenceDefaults();
}
