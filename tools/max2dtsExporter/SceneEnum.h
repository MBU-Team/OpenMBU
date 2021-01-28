//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef SCENEENUM_H_
#define SCENEENUM_H_

#pragma pack(push,8)
#include <max.h>
#pragma pack(pop)

#include "max2dtsExporter/shapeMimic.h"
#include "ts/tsShape.h"
#include "platform/platform.h"
#include "core/tvector.h"
#include "core/filestream.h"

class SceneEnumProc : public ITreeEnumProc
{
    static FileStream file;

    Vector<INode*> meshNodes;
    Vector<INode*> skinNodes;
    Vector<INode*> subTrees;
    Vector<INode*> sequences;
    INode *boundsNode;
    INode *viconNode;

    ShapeMimic shapeMimic;
    TSShape * pShape;

    bool exportError;
    char * exportErrorStr;

    S32 getSequence( INode *pNode );

    static void clearExportConfig();

    void getPolyCount(S32 & polyCount, F32 & minVolPerPoly, F32 & maxVolPerPoly);
    void getPolyCountSubtree(INode *, S32 &, F32 & minVolPerPoly, F32 & maxVolPerPoly);

public:
    SceneEnumProc();
    ~SceneEnumProc();

    static Vector<char *> alwaysExport;
    static Vector<char *> neverExport;
    static Vector<char *> neverAnimate;

    static char exportType;

    static const char * readConfigFile(const char * filename);
    static void writeConfigFile(const char * filename);
    static void setInitialDefaults();

    static bool dumpShape(const char * filename);

    void exportTextFile(Stream * file);

    static bool isDummy(INode *pNode);
    static bool isBounds(INode *pNode);
    static bool isVICON(INode *pNode);
    static bool isCamera(INode *pNode);
    static bool isSubtree(INode *pNode);
    static bool isBillboard(INode * pNode);
    static bool isBillboardZAxis(INode * pNode);
    static bool isSortedObject(INode * pNode);
    static bool isDecal(INode * pNode);
    static void tweakName(const char ** nodeName); // can only take from front at this point...

    static void printDump(U32 mask, const char *);
    void startDump(const TCHAR * filename, const TCHAR * maxFile);

    void setExportError(const char * errStr) { dFree(exportErrorStr); exportErrorStr = dStrdup(errStr); exportError = true; }
    void clearExportError() { exportError = false; dFree(exportErrorStr); exportErrorStr = NULL; }
    bool isExportError() { return exportError; }
    const char * getExportError() { return exportErrorStr; }

    TSShape * getShape() { return pShape; }

    TSShape * regenShape(); // for testing purposes only...

    bool isEmpty();
    S32 callback( INode *node );

    void processScene();
    void enumScene(IScene *);
};

// enum for printDump
enum
{
   PDPass1              = 1 << 0, // collect useful nodes
   PDPass2              = 1 << 1, // put together shape structure
   PDPass3              = 1 << 2, // cull un-needed nodes
   PDObjectOffsets      = 1 << 3, // display object offset transform during 2nd pass
   PDNodeStates         = 1 << 4, // display as added
   PDObjectStates       = 1 << 5, // ""
   PDNodeStateDetails   = 1 << 6, // details of above
   PDObjectStateDetails = 1 << 7, // ""
   PDSequences          = 1 << 8,
   PDShapeHierarchy     = 1 << 9,
   PDAlways             = 0xFFFFFFFF
};

// parameters of export...
extern U32 dumpMask;
extern U32 allowEmptySubtrees;
extern U32 allowCrossedDetails;
extern U32 allowUnusedMeshes;
extern U32 transformCollapse;
extern U32 viconNeeded;
extern U32 enableSequences;
extern U32 allowOldSequences;
extern U32 enableTwoSidedMaterials;
extern F32 animationDelta;
extern F32 maxFrameRate;
extern S32 weightsPerVertex;
extern F32 weightThreshhold;
extern char baseTexturePath[256];
extern S32 t2AutoDetail;

extern void setBaseTexturePath(const char *);
//--------------------------------------------------------------
#endif
