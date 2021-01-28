//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _EDITINTERIORRES_H_
#define _EDITINTERIORRES_H_

#ifndef _INTERIORRES_H_
#include "interior/interiorRes.h"
#endif

class EditInteriorResource : public InteriorResource
{
  public:
   void sortDetailLevels();

   void addDetailLevel(Interior*);
   void setPreviewBitmap(GBitmap*);

   void insertTrigger(InteriorResTrigger* pTrigger);
   void insertPath(InteriorPath* pPath);
   void insertPathedChild(InteriorPathFollower * pPathFollower);
   U32  insertSubObject(Interior* pInterior);
   U32  insertField(ForceField*);
   U32  insertSpecialNode(AISpecialNode*);
   void insertGameEntity(ItrGameEntity*);
};

#endif  // _H_EDITINTERIORRES_
