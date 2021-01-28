//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2dtsExporter/Sequence.h"
#include "max2dtsExporter/exporter.h"
#include "Platform/platformAssert.h"

#pragma pack(push,8)
#include <MAX.H>
#include <dummy.h>
#include <iparamm.h>
#pragma pack(pop)

// conversion from 1.x to 2.x 3ds Max?
//#define TYPE_BOOL TYPE_BOOLEAN

extern HINSTANCE hInstance;
extern TCHAR *GetString(S32);

//------------------------------------------------------
// max data structures used to map between max
// user-interface and max parameter blocks...
//------------------------------------------------------
S32 useFrameRateControls1[] = { IDC_SEQ_USE_FRAME_RATE, IDC_SEQ_USE_N_FRAMES };
S32 useFrameRateControls2[] = { IDC_SEQ_USE_GROUND_FRAME_RATE, IDC_SEQ_USE_N_GROUND_FRAMES };
S32 tfArray[] = { true, false };

static ParamUIDesc descParam1[] =
{
   ParamUIDesc(
      PB_SEQ_CYCLIC,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_CYCLIC ),
   ParamUIDesc(
      PB_SEQ_BLEND,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_BLEND ),
   ParamUIDesc(
      PB_SEQ_LAST_FIRST_FRAME_SAME,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_LAST_FIRST_FRAME_SAME ),

   ParamUIDesc(
      PB_SEQ_USE_FRAME_RATE,
      TYPE_RADIO,
      useFrameRateControls1,
      2,
      tfArray),

   ParamUIDesc(
      PB_SEQ_FRAME_RATE,
      EDITTYPE_FLOAT,
      IDC_SEQ_FRAME_RATE,IDC_SEQ_FRAME_RATE_SPINNER,
      0.0001f,100.0f,1.0f),
   ParamUIDesc(
      PB_SEQ_NUM_FRAMES,
      EDITTYPE_INT,
      IDC_SEQ_NUM_FRAMES,IDC_SEQ_NUM_FRAMES_SPINNER,
      2.0f,100.0f,1.0f),

   ParamUIDesc(
      PB_SEQ_OVERRIDE_DURATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_DURATION_OVERRIDE ),
   ParamUIDesc(
      PB_SEQ_DURATION,
      EDITTYPE_FLOAT,
      IDC_SEQ_DURATION,IDC_SEQ_DURATION_SPINNER,
      0.0001f,1000.0f,1.0f),

    ParamUIDesc(
      PB_SEQ_DEFAULT_PRIORITY,
      EDITTYPE_INT,
      IDC_SEQ_PRIORITY,IDC_SEQ_PRIORITY_SPINNER,
      0.0f, 1000.0f, 1.0f)
};

static ParamUIDesc descParam2[] =
{
   ParamUIDesc(
      PB_SEQ_IGNORE_GROUND_TRANSFORM,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_IGNORE_GROUND_TRANSFORM ),
   ParamUIDesc(
      PB_SEQ_USE_GROUND_FRAME_RATE,
      TYPE_RADIO,
      useFrameRateControls2,
      2,
      tfArray),

   ParamUIDesc(
      PB_SEQ_GROUND_FRAME_RATE,
      EDITTYPE_FLOAT,
      IDC_SEQ_GROUND_FRAME_RATE,
      IDC_SEQ_GROUND_FRAME_RATE_SPINNER,
      0.0001f,100.0f,1.0f),
   ParamUIDesc(
      PB_SEQ_NUM_GROUND_FRAMES,
      EDITTYPE_INT,
      IDC_SEQ_NUM_GROUND_FRAMES,
      IDC_SEQ_NUM_GROUND_FRAMES_SPINNER,
      2.0f,100.0f,1.0f),
};

static ParamUIDesc descParam3[] =
{
   ParamUIDesc(
      PB_SEQ_ENABLE_MORPH_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_MORPH_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_VIS_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_VIS_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_TRANSFORM_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_TRANSFORM_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_TEXTURE_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_TEXTURE_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_IFL_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_IFL_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_DECAL_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_DECAL_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_ENABLE_DECAL_FRAME_ANIMATION ),

   ParamUIDesc(
      PB_SEQ_FORCE_MORPH_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_MORPH_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_FORCE_VIS_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_VIS_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_FORCE_TRANSFORM_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_TRANSFORM_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_FORCE_SCALE_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_SCALE_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_FORCE_TEXTURE_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_TEXTURE_ANIMATION ),
   ParamUIDesc(
      PB_SEQ_FORCE_DECAL_ANIMATION,
      TYPE_SINGLECHEKBOX,
      IDC_SEQ_FORCE_DECAL_ANIMATION )
};

#define PARAMDESC1_LENGTH  (sizeof( descParam1 ) / sizeof( ParamUIDesc ))
#define PARAMDESC2_LENGTH  (sizeof( descParam2 ) / sizeof( ParamUIDesc ))
#define PARAMDESC3_LENGTH  (sizeof( descParam3 ) / sizeof( ParamUIDesc ))

// This is the current version.
static ParamBlockDescID curVer[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 },  // blend sequence reference time (keyframe time determines it, not param value)

    { TYPE_BOOL,  NULL, FALSE, 19 },  // enable texture animation
    { TYPE_BOOL,  NULL, FALSE, 20 },  // enable ifl animation
    { TYPE_BOOL,  NULL, FALSE, 21 },  // force texture animation

    { TYPE_FLOAT, NULL, TRUE,  22 },  // triggers...keyframes are read to determine trigger times and values

    { TYPE_BOOL,  NULL, FALSE, 23 },  // enable decal animation
    { TYPE_BOOL,  NULL, FALSE, 24 },  // enable decal frame animation
    { TYPE_BOOL,  NULL, FALSE, 25 },  // force decal animation

    { TYPE_BOOL,  NULL, FALSE, 26 },  // override sequence duration
    { TYPE_FLOAT, NULL, FALSE, 27 },  // sequence duration (if override true)

    { TYPE_BOOL,  NULL, FALSE, 28 },  // enable uniform scale animation
    { TYPE_BOOL,  NULL, FALSE, 29 },  // enable arbitrary scale animation
    { TYPE_BOOL,  NULL, FALSE, 30 },  // force scale animation
};

static ParamBlockDescID oldVer6[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 },  // blend sequence reference time (keyframe time determines it, not param value)

    { TYPE_BOOL,  NULL, FALSE, 19 },  // enable texture animation
    { TYPE_BOOL,  NULL, FALSE, 20 },  // enable ifl animation
    { TYPE_BOOL,  NULL, FALSE, 21 },  // force texture animation

    { TYPE_FLOAT, NULL, TRUE,  22 },  // triggers...keyframes are read to determine trigger times and values

    { TYPE_BOOL,  NULL, FALSE, 23 },  // enable decal animation
    { TYPE_BOOL,  NULL, FALSE, 24 },  // enable decal frame animation
    { TYPE_BOOL,  NULL, FALSE, 25 },  // force decal animation

    { TYPE_BOOL,  NULL, FALSE, 26 },  // override sequence duration
    { TYPE_FLOAT, NULL, FALSE, 27 },  // sequence duration (if override true)
};

static ParamBlockDescID oldVer5[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 },  // blend sequence reference time (keyframe time determines it, not param value)

    { TYPE_BOOL,  NULL, FALSE, 19 },  // enable texture animation
    { TYPE_BOOL,  NULL, FALSE, 20 },  // enable ifl animation
    { TYPE_BOOL,  NULL, FALSE, 21 },  // force texture animation

    { TYPE_FLOAT, NULL, TRUE,  22 },  // triggers...keyframes are read to determine trigger times and values

    { TYPE_BOOL,  NULL, FALSE, 23 },  // enable decal animation
    { TYPE_BOOL,  NULL, FALSE, 24 },  // enable decal frame animation
    { TYPE_BOOL,  NULL, FALSE, 25 }   // force decal animation
};

static ParamBlockDescID oldVer4[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 },  // blend sequence reference time (keyframe time determines it, not param value)

    { TYPE_BOOL,  NULL, FALSE, 19 },  // enable texture animation
    { TYPE_BOOL,  NULL, FALSE, 20 },  // enable ifl animation
    { TYPE_BOOL,  NULL, FALSE, 21 },  // force texture animation

    { TYPE_FLOAT, NULL, TRUE,  22 },  // triggers...keyframes are read to determine trigger times and values
};

// old param block versions
static ParamBlockDescID oldVer3[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 },  // blend sequence reference time (keyframe time determines it, not param value)

    { TYPE_BOOL,  NULL, FALSE, 19 },  // enable texture animation
    { TYPE_BOOL,  NULL, FALSE, 20 },  // enable ifl animation
    { TYPE_BOOL,  NULL, FALSE, 21 }   // force texture animation
};

static ParamBlockDescID oldVer2[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 },  // sequence default priority

    { TYPE_FLOAT, NULL, TRUE,  18 }   // blend sequence reference time (keyframe time determines it, not param value)
};

static ParamBlockDescID oldVer1[] =
{
    { TYPE_BOOL,  NULL, TRUE , 0  },   // begin/end

    { TYPE_BOOL,  NULL, FALSE, 1  },   // cyclic
    { TYPE_BOOL,  NULL, FALSE, 2  },   // blend

    { TYPE_BOOL,  NULL, FALSE, 3  },   // first and last frame same (cyclic seq only)

    { TYPE_BOOL,  NULL, FALSE, 4  },  // use frame rate?
    { TYPE_FLOAT, NULL, FALSE, 5  },  // Frame Rate
    { TYPE_INT,   NULL, FALSE, 6  },  // number of frames

    { TYPE_BOOL,  NULL, FALSE, 7  },  // ignore ground transform altogether?
    { TYPE_BOOL,  NULL, FALSE, 8  },  // use ground frame rate?
    { TYPE_FLOAT, NULL, FALSE, 9  },  // ground frame Rate
    { TYPE_INT,   NULL, FALSE, 10 },  // number of ground frames

    { TYPE_BOOL,  NULL, FALSE, 11 },  // enable morph animation
    { TYPE_BOOL,  NULL, FALSE, 12 },  // enable vis animation
    { TYPE_BOOL,  NULL, FALSE, 13 },  // enable transform animation

    { TYPE_BOOL,  NULL, FALSE, 14 },  // force morph animation
    { TYPE_BOOL,  NULL, FALSE, 15 },  // force vis animation
    { TYPE_BOOL,  NULL, FALSE, 16 },  // force transform animation

    { TYPE_INT,   NULL, FALSE, 17 }   // sequence default priority
};

// Array of old versions
static ParamVersionDesc versions[] =
{
   ParamVersionDesc( oldVer1, sizeof(oldVer1)/sizeof(ParamBlockDescID), 0),
   ParamVersionDesc( oldVer2, sizeof(oldVer2)/sizeof(ParamBlockDescID), 1),
   ParamVersionDesc( oldVer3, sizeof(oldVer3)/sizeof(ParamBlockDescID), 2),
   ParamVersionDesc( oldVer4, sizeof(oldVer4)/sizeof(ParamBlockDescID), 3),
   ParamVersionDesc( oldVer5, sizeof(oldVer5)/sizeof(ParamBlockDescID), 4),
   ParamVersionDesc( oldVer6, sizeof(oldVer6)/sizeof(ParamBlockDescID), 5)
};

#define NUM_OLDVERSIONS (sizeof(versions) / sizeof(ParamVersionDesc))
#define CURRENT_VERSION NUM_OLDVERSIONS

// Current version
#define PBLOCK_LENGTH   (sizeof(curVer) / sizeof(ParamBlockDescID))
static ParamVersionDesc curVersion( curVer, PBLOCK_LENGTH, CURRENT_VERSION );

//------------------------------------------------------
// sequence class defaults:
//------------------------------------------------------
U32   SequenceObject::defaultCyclic                     = true;
U32   SequenceObject::defaultBlend                      = false;
U32   SequenceObject::defaultFirstLastFrameSame         = false;

U32   SequenceObject::defaultUseFrameRate               = true;
F32   SequenceObject::defaultFrameRate                  = 15.0f;
S32   SequenceObject::defaultNumFrames                  = 2;

U32   SequenceObject::defaultIgnoreGroundTransform      = false;
U32   SequenceObject::defaultUseGroundFrameRate         = false;
F32   SequenceObject::defaultGroundFrameRate            = 15.0f;
S32   SequenceObject::defaultNumGroundFrames            = 2;

U32   SequenceObject::defaultEnableMorphAnimation       = false;
U32   SequenceObject::defaultEnableVisAnimation         = false;
U32   SequenceObject::defaultEnableTransformAnimation   = true;
U32   SequenceObject::defaultEnableTextureAnimation     = false;
U32   SequenceObject::defaultEnableIflAnimation         = false;
U32   SequenceObject::defaultEnableDecalAnimation       = false;
U32   SequenceObject::defaultEnableDecalFrameAnimation  = false;

U32   SequenceObject::defaultForceMorphAnimation        = false;
U32   SequenceObject::defaultForceVisAnimation          = false;
U32   SequenceObject::defaultForceTransformAnimation    = false;
U32   SequenceObject::defaultForceTextureAnimation      = false;
U32   SequenceObject::defaultForceDecalAnimation        = false;

S32   SequenceObject::defaultDefaultSequencePriority    = 0;

S32   SequenceObject::defaultBlendReferenceTime         = 0; // value not important

U32   SequenceObject::defaultOverrideDuration           = false;
F32   SequenceObject::defaultDuration                   = 1.0f;

U32   SequenceObject::defaultEnableUniformScaleAnimation = false;
U32   SequenceObject::defaultEnableArbitraryScaleAnimation = false;
U32   SequenceObject::defaultForceScaleAnimation        = false;

//------------------------------------------------------
// Some more statics:
//------------------------------------------------------
SequenceObject * SequenceObject::editOb = NULL;
IParamMap      * SequenceObject::pmapParam1  = NULL;
IParamMap      * SequenceObject::pmapParam2  = NULL;
IParamMap      * SequenceObject::pmapParam3  = NULL;

//------------------------------------------------------
// Sequence object methods...
//------------------------------------------------------

void SequenceObject::InitNodeName(TSTR& s)
{
    s = GetString(IDS_DB_SEQUENCE);
}

void SequenceObject::GetClassName(TSTR& s)
{
    s = TSTR(GetString(IDS_DB_SEQUENCE));
}

void resetSequenceDefaults()
{
   SequenceObject::defaultCyclic                     = true;
   SequenceObject::defaultBlend                      = false;
   SequenceObject::defaultFirstLastFrameSame         = false;

   SequenceObject::defaultUseFrameRate               = true;
   SequenceObject::defaultFrameRate                  = 15.0f;
   SequenceObject::defaultNumFrames                  = 2;

   SequenceObject::defaultIgnoreGroundTransform      = false;
   SequenceObject::defaultUseGroundFrameRate         = false;
   SequenceObject::defaultGroundFrameRate            = 15.0f;
   SequenceObject::defaultNumGroundFrames            = 2;

   SequenceObject::defaultEnableMorphAnimation       = false;
   SequenceObject::defaultEnableVisAnimation         = false;
   SequenceObject::defaultEnableTransformAnimation   = true;
   SequenceObject::defaultEnableTextureAnimation     = false;
   SequenceObject::defaultEnableIflAnimation         = false;
   SequenceObject::defaultEnableDecalAnimation       = false;
   SequenceObject::defaultEnableDecalFrameAnimation  = false;

   SequenceObject::defaultForceMorphAnimation        = false;
   SequenceObject::defaultForceVisAnimation          = false;
   SequenceObject::defaultForceTransformAnimation    = false;
   SequenceObject::defaultForceTextureAnimation      = false;
   SequenceObject::defaultForceDecalAnimation        = false;

   SequenceObject::defaultDefaultSequencePriority    = 0;

   SequenceObject::defaultBlendReferenceTime         = 0; // value not important

   SequenceObject::defaultOverrideDuration           = false;
   SequenceObject::defaultDuration                   = 1.0f;

   SequenceObject::defaultEnableUniformScaleAnimation   = false;
   SequenceObject::defaultEnableArbitraryScaleAnimation = false;
   SequenceObject::defaultForceScaleAnimation           = false;
}

bool getBoolSequenceDefault(S32 index)
{
   switch (index)
   {
      case PB_SEQ_CYCLIC: return SequenceObject::defaultCyclic;
      case PB_SEQ_BLEND: return SequenceObject::defaultBlend;
      case PB_SEQ_LAST_FIRST_FRAME_SAME: return SequenceObject::defaultFirstLastFrameSame;
      case PB_SEQ_USE_FRAME_RATE: return SequenceObject::defaultUseFrameRate;

      case PB_SEQ_IGNORE_GROUND_TRANSFORM: return SequenceObject::defaultIgnoreGroundTransform;
      case PB_SEQ_USE_GROUND_FRAME_RATE: return SequenceObject::defaultUseGroundFrameRate;

      case PB_SEQ_ENABLE_MORPH_ANIMATION: return SequenceObject::defaultEnableMorphAnimation;
      case PB_SEQ_ENABLE_VIS_ANIMATION: return SequenceObject::defaultEnableVisAnimation;
      case PB_SEQ_ENABLE_TRANSFORM_ANIMATION: return SequenceObject::defaultEnableTransformAnimation;
      case PB_SEQ_ENABLE_TEXTURE_ANIMATION: return SequenceObject::defaultEnableTextureAnimation;
      case PB_SEQ_ENABLE_IFL_ANIMATION: return SequenceObject::defaultEnableIflAnimation;
      case PB_SEQ_ENABLE_DECAL_ANIMATION: return SequenceObject::defaultEnableDecalAnimation;
      case PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION: return SequenceObject::defaultEnableDecalFrameAnimation;

      case PB_SEQ_FORCE_MORPH_ANIMATION: return SequenceObject::defaultForceMorphAnimation;
      case PB_SEQ_FORCE_VIS_ANIMATION: return SequenceObject::defaultForceVisAnimation;
      case PB_SEQ_FORCE_TRANSFORM_ANIMATION: return SequenceObject::defaultForceTransformAnimation;
      case PB_SEQ_FORCE_TEXTURE_ANIMATION: return SequenceObject::defaultForceTextureAnimation;
      case PB_SEQ_FORCE_DECAL_ANIMATION: return SequenceObject::defaultForceDecalAnimation;

      case PB_SEQ_OVERRIDE_DURATION: return SequenceObject::defaultOverrideDuration;

      case PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION: return SequenceObject::defaultEnableUniformScaleAnimation;
      case PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION: return SequenceObject::defaultEnableArbitraryScaleAnimation;
      case PB_SEQ_FORCE_SCALE_ANIMATION: return SequenceObject::defaultForceScaleAnimation;

//      default : AssertFatal(0,"Invalid default when getting sequence default value");
//         return false;
   }
   return false;
}

F32 getFloatSequenceDefault(S32 index)
{
    switch (index)
    {
        case PB_SEQ_FRAME_RATE: return SequenceObject::defaultFrameRate;
        case PB_SEQ_GROUND_FRAME_RATE: return SequenceObject::defaultGroundFrameRate;
        case PB_SEQ_DURATION: return SequenceObject::defaultDuration;

//        default : AssertFatal(0,"Invalid default when getting sequence default value");
//            return 0.0f;
    }
    return 0.0f;
}

S32 getIntSequenceDefault(S32 index)
{
    switch (index)
    {
        case PB_SEQ_NUM_FRAMES: return SequenceObject::defaultNumFrames;
        case PB_SEQ_NUM_GROUND_FRAMES: return SequenceObject::defaultNumGroundFrames;
        case PB_SEQ_DEFAULT_PRIORITY: return SequenceObject::defaultDefaultSequencePriority;

//        default : AssertFatal(0,"Invalid default when getting sequence default value");
//            return 0;
    }
    return 0;
}

//------------------------------------------------------
// Sequence object implementation -- not much here
//------------------------------------------------------
IParamArray *SequenceObject::GetParamBlock()
{
    return pblock;
}

S32 SequenceObject::GetParamBlockIndex(S32 id)
{
    if (pblock && id>=0 && id<pblock->NumParams())
        return id;
    return -1;
}

void SequenceObject::FreeCaches()
{
    ivalid.SetEmpty();
}

RefResult SequenceObject::NotifyRefChanged( Interval changeInt,
                                            RefTargetHandle hTarget,
                                            PartID& partID,
                                            RefMessage message )
{
    switch (message)
    {
        case REFMSG_CHANGE:
        {
            if (editOb==this)
                InvalidateUI();
                break;
        }
        case REFMSG_GET_PARAM_DIM:
        {
            GetParamDim *gpd = (GetParamDim*)partID;
            gpd->dim = GetParameterDim(gpd->index);
            return REF_STOP;
        }
        case REFMSG_GET_PARAM_NAME:
        {
            GetParamName *gpn = (GetParamName*)partID;
            gpn->name = GetParameterName(gpn->index);
            return REF_STOP;
        }
    }
    return(REF_SUCCEED);
}

Interval SequenceObject::ObjectValidity(TimeValue time)
{
    return ivalid;
}

TSTR SequenceObject::SubAnimName(S32 i)
{
    return _T("Parameters");
}

void SequenceObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
    // Save the current sequence object
    editOb = this;

    // we have mutliple parameter maps...make sure one of each is created:

    if (pmapParam1)
    {
        // Left over from last sequence ceated
        pmapParam1->SetParamBlock(pblock);
    }
    else
    {
        pmapParam1 = CreateCPParamMap(
                descParam1,
                PARAMDESC1_LENGTH,
                pblock,
                ip,
                hInstance,
                MAKEINTRESOURCE(IDD_SEQUENCE_PARAMS),
                _T("General"),
                0);
    }

    if (pmapParam2)
    {
        // Left over from last sequence ceated
        pmapParam2->SetParamBlock(pblock);
    }
    else
    {
        pmapParam2 = CreateCPParamMap(
                descParam2,
                PARAMDESC2_LENGTH,
                pblock,
                ip,
                hInstance,
                MAKEINTRESOURCE(IDD_SEQUENCE_PARAMS2),
                _T("Ground Transform"),
                APPENDROLL_CLOSED);
    }

    if (pmapParam3)
    {
        // Left over from last sequence ceated
        pmapParam3->SetParamBlock(pblock);
    }
    else
    {
        pmapParam3 = CreateCPParamMap(
                descParam3,
                PARAMDESC3_LENGTH,
                pblock,
                ip,
                hInstance,
                MAKEINTRESOURCE(IDD_SEQUENCE_PARAMS3),
                _T("Export Control"),
                APPENDROLL_CLOSED);
    }

}

void SequenceObject::EndEditParams(IObjParam *ip, ULONG flags,Animatable *next)
{
    editOb = NULL;

    ClearAFlag(A_OBJ_CREATING);

    if ( (flags&END_EDIT_REMOVEUI) && pmapParam1 )
    {
        // Remove the rollup pages from the command panel.
        DestroyCPParamMap(pmapParam1);
        pmapParam1  = NULL;
    }

    if ( (flags&END_EDIT_REMOVEUI) && pmapParam2 )
    {
        // Remove the rollup pages from the command panel.
        DestroyCPParamMap(pmapParam2);
        pmapParam2  = NULL;
    }

    if ( (flags&END_EDIT_REMOVEUI) && pmapParam3 )
    {
        // Remove the rollup pages from the command panel.
        DestroyCPParamMap(pmapParam3);
        pmapParam3  = NULL;
    }
}


// This method is called when the user interface controls need to be
// updated to reflect new values because of the user moving the time
// slider.  Here we simply call a method of the parameter map to
// handle this for us.
void SequenceObject::InvalidateUI()
{
    if (pmapParam1)
        pmapParam1->Invalidate();

    if (pmapParam2)
        pmapParam2->Invalidate();

    if (pmapParam3)
        pmapParam3->Invalidate();
}

// This method returns the dimension of the parameter requested.
// This dimension describes the type and magnitude of the value
// stored by the parameter.
ParamDimension *SequenceObject::GetParameterDim(S32)
{
   return defaultDim; // just store the parameter...ask no questions
}

// This method returns the name of the parameter requested.
TSTR SequenceObject::GetParameterName(S32 pbIndex)
{
    switch (pbIndex)
    {
        case PB_SEQ_BEGIN_END:
            return TSTR(_T("Sequence Begin/End"));

        case PB_SEQ_CYCLIC:
            return TSTR(_T("Cyclic sequence"));
        case PB_SEQ_BLEND:
            return TSTR(_T("Blend sequence"));

        case PB_SEQ_LAST_FIRST_FRAME_SAME:
            return TSTR(_T("Last frame matches first frame"));

        case PB_SEQ_USE_FRAME_RATE:
            return TSTR(_T("Use frame rate"));
        case PB_SEQ_FRAME_RATE:
            return TSTR(_T("Frame rate"));
        case PB_SEQ_NUM_FRAMES:
            return TSTR(_T("Number of frames"));

        case PB_SEQ_IGNORE_GROUND_TRANSFORM:
            return TSTR(_T("Ignore ground transform"));
        case PB_SEQ_USE_GROUND_FRAME_RATE:
            return TSTR(_T("Use ground transform frame rate"));
        case PB_SEQ_GROUND_FRAME_RATE:
            return TSTR(_T("Ground transform frame rate"));
        case PB_SEQ_NUM_GROUND_FRAMES:
            return TSTR(_T("Number of ground transform frames"));

        case PB_SEQ_ENABLE_MORPH_ANIMATION:
            return TSTR(_T("Enable morph animation"));
        case PB_SEQ_ENABLE_VIS_ANIMATION:
            return TSTR(_T("Enable visibility animation"));
        case PB_SEQ_ENABLE_TRANSFORM_ANIMATION:
            return TSTR(_T("Enable transform animation"));
        case PB_SEQ_ENABLE_TEXTURE_ANIMATION:
            return TSTR(_T("Enable texture animation"));
        case PB_SEQ_ENABLE_IFL_ANIMATION:
            return TSTR(_T("Enable IFL animation"));
        case PB_SEQ_ENABLE_DECAL_ANIMATION:
            return TSTR(_T("Enable decal animation"));
        case PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION:
            return TSTR(_T("Enable decal frame animation"));

        case PB_SEQ_FORCE_MORPH_ANIMATION:
            return TSTR(_T("Force morph animation"));
        case PB_SEQ_FORCE_VIS_ANIMATION:
            return TSTR(_T("Force visibility animation"));
        case PB_SEQ_FORCE_TRANSFORM_ANIMATION:
            return TSTR(_T("Force transform animation"));
        case PB_SEQ_FORCE_TEXTURE_ANIMATION:
            return TSTR(_T("Force texture animation"));
        case PB_SEQ_FORCE_DECAL_ANIMATION:
            return TSTR(_T("Force decal animation"));

        case PB_SEQ_DEFAULT_PRIORITY:
            return TSTR(_T("Default sequence priority"));

        case PB_SEQ_BLEND_REFERENCE_TIME:
            return TSTR(_T("Blend Reference Time"));

        case PB_SEQ_TRIGGERS:
            return TSTR(_T("Triggers"));

        case PB_SEQ_OVERRIDE_DURATION:
            return TSTR(_T("Override sequence duration"));
        case PB_SEQ_DURATION:
            return TSTR(_T("Sequence duration"));

        case PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION:
            return TSTR(_T("Enable uniform scale animation"));
        case PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION:
            return TSTR(_T("Enable arbitrary scale animation"));
        case PB_SEQ_FORCE_SCALE_ANIMATION:
            return TSTR(_T("Force scale animation"));

        default:
            return TSTR(_T(""));
    }
}

RefTargetHandle SequenceObject::Clone(RemapDir& remap)
{
    SequenceObject* newob = new SequenceObject();
    newob->ReplaceReference(0,pblock->Clone(remap));
    newob->ivalid.SetEmpty();
    return newob;
}

SequenceObject::SequenceObject()
{
    ivalid.SetEmpty();
    pblock = NULL;
    SetAFlag(A_OBJ_CREATING);

    // Create the parameter block and make a reference to it.
    MakeRefByID(FOREVER, 0,
                CreateParameterBlock( curVersion.desc, PBLOCK_LENGTH, CURRENT_VERSION));

//    assert(pblock);

//    if (!pblock)
//        // need a way to signal an error
//        return;

    // Initialize the default values.
    pblock->SetValue(PB_SEQ_BEGIN_END, 0, false);
    pblock->SetValue(PB_SEQ_CYCLIC, 0, (bool)defaultCyclic);
    pblock->SetValue(PB_SEQ_BLEND, 0, (bool)defaultBlend);
    pblock->SetValue(PB_SEQ_LAST_FIRST_FRAME_SAME, 0, (bool)defaultFirstLastFrameSame);
    pblock->SetValue(PB_SEQ_USE_FRAME_RATE, 0, (bool)defaultUseFrameRate);
    pblock->SetValue(PB_SEQ_FRAME_RATE, 0, defaultFrameRate);
    pblock->SetValue(PB_SEQ_NUM_FRAMES, 0, defaultNumFrames);
    pblock->SetValue(PB_SEQ_IGNORE_GROUND_TRANSFORM, 0, (bool)defaultIgnoreGroundTransform);
    pblock->SetValue(PB_SEQ_USE_GROUND_FRAME_RATE, 0, (bool)defaultUseGroundFrameRate);
    pblock->SetValue(PB_SEQ_GROUND_FRAME_RATE, 0, defaultGroundFrameRate);
    pblock->SetValue(PB_SEQ_NUM_GROUND_FRAMES, 0, defaultNumGroundFrames);
    pblock->SetValue(PB_SEQ_ENABLE_MORPH_ANIMATION, 0, (bool)defaultEnableMorphAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_VIS_ANIMATION, 0, (bool)defaultEnableVisAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_TRANSFORM_ANIMATION, 0, (bool)defaultEnableTransformAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_TEXTURE_ANIMATION, 0, (bool)defaultEnableTextureAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_IFL_ANIMATION, 0, (bool)defaultEnableIflAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_DECAL_ANIMATION, 0, (bool)defaultEnableDecalAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_DECAL_FRAME_ANIMATION, 0, (bool)defaultEnableDecalFrameAnimation);
    pblock->SetValue(PB_SEQ_FORCE_MORPH_ANIMATION, 0, (bool)defaultForceMorphAnimation);
    pblock->SetValue(PB_SEQ_FORCE_VIS_ANIMATION, 0, (bool)defaultForceVisAnimation);
    pblock->SetValue(PB_SEQ_FORCE_TRANSFORM_ANIMATION, 0, (bool)defaultForceTransformAnimation);
    pblock->SetValue(PB_SEQ_FORCE_TEXTURE_ANIMATION, 0, (bool)defaultForceTextureAnimation);
    pblock->SetValue(PB_SEQ_FORCE_DECAL_ANIMATION, 0, (bool)defaultForceDecalAnimation);
    pblock->SetValue(PB_SEQ_DEFAULT_PRIORITY, 0, defaultDefaultSequencePriority);
    pblock->SetValue(PB_SEQ_BLEND_REFERENCE_TIME, 0, defaultBlendReferenceTime);
    pblock->SetValue(PB_SEQ_OVERRIDE_DURATION, 0, (bool)defaultOverrideDuration);
    pblock->SetValue(PB_SEQ_DURATION, 0, defaultDuration);
    pblock->SetValue(PB_SEQ_ENABLE_UNIFORM_SCALE_ANIMATION, 0, (bool)defaultEnableUniformScaleAnimation);
    pblock->SetValue(PB_SEQ_ENABLE_ARBITRARY_SCALE_ANIMATION, 0, (bool)defaultEnableArbitraryScaleAnimation);
    pblock->SetValue(PB_SEQ_FORCE_SCALE_ANIMATION, 0, (bool)defaultForceScaleAnimation);
}

SequenceObject::~SequenceObject()
{
    DeleteAllRefsFromMe();
}

//
// Reference Managment:
//

IOResult SequenceObject::Load(ILoad *iload)
{
    // This is the callback that corrects for any older versions
    // of the parameter block structure found in the MAX file
    // being loaded.
    iload->RegisterPostLoadCallback(new ParamBlockPLCB(versions,NUM_OLDVERSIONS,&curVersion,this,0));
    return IO_OK;
}


//------------------------------------------------------
// Sequence object descriptor declaration & implementation
//------------------------------------------------------
class SequenceObjClassDesc:public ClassDesc
{
public:
    S32            IsPublic() { return 1; }
    void *         Create(BOOL loading = FALSE) { return new SequenceObject; }
    const TCHAR *  ClassName() { return GetString(IDS_DB_SEQUENCE); }
    SClass_ID      SuperClassID() { return HELPER_CLASS_ID; }
    Class_ID       ClassID() { return SEQUENCE_CLASS_ID; }
    const TCHAR *  Category() { return GetString(IDS_DB_GENERAL);  }

    // following functions allow setting of parameter defaults...
    // worthless unless all 3 are implemented
//     void           ResetClassParams(BOOL fileReset)
//    {
//       if(fileReset)
//          resetSequenceDefaults();
//    }
    // HasClassParams?
    // EditClassParams?
};

static SequenceObjClassDesc sequenceObjDesc;

ClassDesc * GetSequenceDesc() { return &sequenceObjDesc; }

