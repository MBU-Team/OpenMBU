//-----------------------------------------------------------------------------
// Collada-2-DTS
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/collada/colladaExtensions.h"
#include "ts/collada/colladaAppSequence.h"


ColladaAppSequence::ColladaAppSequence(const domAnimation_clip* clip)
   : pClip(clip), clipExt(new ColladaExtension_animation_clip(clip))
{
   seqStart = pClip->getStart();
   seqEnd = pClip->getEnd();
}

ColladaAppSequence::~ColladaAppSequence()
{
   delete clipExt;
}

const char* ColladaAppSequence::getName() const
{
   return _GetNameOrId(pClip);
}

S32 ColladaAppSequence::getNumTriggers()
{
   return clipExt->triggers.size();
}

void ColladaAppSequence::getTrigger(S32 index, TSShape::Trigger& trigger)
{
   trigger.pos = clipExt->triggers[index].time;
   trigger.state = clipExt->triggers[index].state;
}

U32 ColladaAppSequence::getFlags() const
{
   U32 flags = 0;
   if (clipExt->cyclic) flags |= TSShape::Cyclic;
   if (clipExt->blend)  flags |= TSShape::Blend;
   return flags;
}

F32 ColladaAppSequence::getPriority()
{
   return clipExt->priority;
}

F32 ColladaAppSequence::getBlendRefTime()
{
   return clipExt->blendReferenceTime;
}
