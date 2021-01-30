//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#ifndef _SGLIGHTING_H_
#define _SGLIGHTING_H_

//----------------------------------------------
// lighting preferences...
// comment out line to disable option...

/// harsh or blurred lighting...
#define SG_STATIC_LIGHT_SHADOWS_BLUR

/// avoid washed out dynamic lighting...
#define SG_ADVANCED_INTERIOR_LIGHTING

/// detail maps on interiors...
#define SG_INTERIOR_DETAILMAPPING

/// enable advanced lighting options
#define SG_ADVANCED_DYNAMIC_SHADOWS
#define SG_ADVANCED_DTS_DYNAMIC_LIGHTING
#define SG_ADVANCED_PARTICLE_LIGHTING
#define SG_OVEREXPOSED_SELFILLUMINATION

/// tune advanced lighting options
#define SG_PARTICLESYSTEMLIGHT_FIXED_INTENSITY	0.5f
#define SG_DYNAMIC_SHADOW_INTENSITY		0.5
#define SG_DYNAMIC_SHADOW_STEPS			5.0f
#define SG_DYNAMIC_SHADOW_TIME			20


//----------------------------------------------
// lightmap hint defaults...
// these are NOT used by every implementation of
// the lightmap subclasses...

// these are self-explanatory...
// all used by sgPlanarLightMap...
// SG_CALCULATE_SHADOWS also used by sgTerrainLightMap...
#define SG_CHOOSE_SPEED_OVER_QUALITY	false
#define SG_SELF_SHADOWING				true
#define SG_CALCULATE_SHADOWS			true
#define SG_MIN_LEXEL_INTENSITY			0.0039215f
#define SG_MIN_LEXEL_INTENSITY_SPEED_OVER_QUALITY	0.1f


//----------------------------------------------
// don't touch...

#define SG_STATIC_SPOT_VECTOR_NORMALIZED		Point3F(0, 0, 1)


//----------------------------------------------
// vibrant lighting amounts...

/// how much glare do we want (must be 1.0, 2.0, or 4.0)...
#define SG_LIGHTING_OVERBRIGHT_AMOUNT			2.0

// do not change...
/// this value resets Torque to a normal lighting value...
#define SG_LIGHTING_NORMAL_AMOUNT				1.0


//----------------------------------------------
// static object 'tailored' lighting...

/// these define the number of universal static
/// lights that can be manually assigned to a static...
#define SG_TSSTATIC_MAX_LIGHT_SHIFT	3
#define SG_TSSTATIC_MAX_LIGHTS			((1 << SG_TSSTATIC_MAX_LIGHT_SHIFT) - 1)


//----------------------------------------------
// prioritized dts lighting

/// these values change the priority of the light objects...
#define SG_LIGHTMANAGER_SUN_PRIORITY			6.0f
#define SG_LIGHTMANAGER_DYNAMIC_PRIORITY		1.0f
#define SG_LIGHTMANAGER_STATIC_PRIORITY			4.0f
#define SG_LIGHTMANAGER_ASSIGNED_PRIORITY		6.0f


//----------------------------------------------
// version info

#define SG_LIGHTINGPACK_CORE_VERSION	"2.1"
#define SG_TGE_VERSION					"1.0"
#define SG_LIGHTINGPACK_SUB_VERSION		"5.1"

#define SG_LIGHTINGPACK_SHOW_ALL_INFORMATION	true

char* sgGetLightingPackInformation(bool fullinfo = false);


#endif//_SGLIGHTING_H_
