//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_ELEM_MGR_H_
#define _RENDER_ELEM_MGR_H_

#include "renderInstMgr.h"
#include "materials/matInstance.h"

//**************************************************************************
// RenderElemManager - manages and renders lists of MainSortElem
//**************************************************************************
class RenderElemMgr
{
public:
    struct MainSortElem
    {
        RenderInst* inst;
        U32 key;
        U32 key2;
    };

protected:
    Vector< MainSortElem > mElementList;

    virtual void setupSGData( RenderInst *ri, SceneGraphData &data );
    bool newPassNeeded(MatInstance* currMatInst, RenderInst* ri);
public:
    RenderElemMgr();

    virtual void addElement(RenderInst* inst);
    virtual void sort();
    virtual void render() {};
    virtual void clear();

    static S32 FN_CDECL cmpKeyFunc(const void* p1, const void* p2);

};

// The bin is sorted by (see RenderElemMgr::cmpKeyFunc)
//    1.  MaterialInstance type (currently just MatInstance and sgMatInstance, sgMatInstance is last)
//    2.  Material
//    3.  Manager specific key (vertex buffer address by default)
// This function is called on each item of the bin and basically detects any changes in conditions 1 or 2
inline bool RenderElemMgr::newPassNeeded(MatInstance* currMatInst, RenderInst* ri)
{
    // We need a new pass if:
    //  1.  There's no Material Instance (old ff object?)
    //  2.  If the material differ
    //  3.  If the material instance types are different.
    return ((ri->matInst == NULL) || (ri->matInst->getMaterial() != currMatInst->getMaterial()) || (currMatInst->compare(ri->matInst) != 0));
}


#endif
