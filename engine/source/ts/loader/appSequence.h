//-----------------------------------------------------------------------------
// TS Shape Loader
// Copyright (C) 2008 GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _APPSEQUENCE_H_
#define _APPSEQUENCE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif

class AppSequence
{
public:
   F32 fps;

public:
   AppSequence(): fps(30.0) { }
   virtual ~AppSequence() { }

   virtual S32 getNumTriggers() const { return 0; }
   virtual void getTrigger(S32 index, TSShape::Trigger& trigger) const { trigger.state = 0;}

   virtual const char* getName() const { return "ambient"; }

   virtual F32 getStart() const { return 0.0f; }
   virtual F32 getEnd() const { return 0.0f; }

   virtual U32 getFlags() const { return 0; }
   virtual F32 getPriority() const { return 5; }
   virtual F32 getBlendRefTime() const { return 0.0f; }
};

#endif // _APPSEQUENCE_H_
