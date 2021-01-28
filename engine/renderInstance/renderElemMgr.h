//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _RENDER_ELEM_MGR_H_
#define _RENDER_ELEM_MGR_H_

#include "renderInstMgr.h"

//**************************************************************************
// RenderElemManager - manages and renders lists of MainSortElem
//**************************************************************************
class RenderElemMgr
{
 public:
   struct MainSortElem
   {
      RenderInst *inst;
      U32 key;
      U32 key2;
   };

 protected:
   Vector< MainSortElem > mElementList;

 public:
   RenderElemMgr();

   virtual void addElement( RenderInst *inst );
   virtual void sort();
   virtual void render(){};
   virtual void clear();

   virtual void setupLights(RenderInst *inst, SceneGraphData &data);

   static S32 FN_CDECL cmpKeyFunc(const void* p1, const void* p2);

};




#endif
