//*****************************************************************************
// Base interior vertex shader
//*****************************************************************************
#define IN_HLSL
#include "shdrConsts.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
	float4 baseTex    : TEXCOORD0;
	float4 lmTex      : TEXCOORD1;
	float3 T          : TEXCOORD2;
	float3 B          : TEXCOORD3;
	float3 N          : TEXCOORD4;
};


struct Conn
{
   float4 HPOS : POSITION;
	float4 TEX0 : TEXCOORD0;
	float4 TEX1 : TEXCOORD1;
	float4 TEX2 : TEXCOORD2;
	float3 lightDir : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(  Appdata In, 
            uniform float4x4 modelview : register(VC_WORLD_PROJ),
            uniform float3 lightDir : register(VC_LIGHT_DIR1)
)
{
   Conn Out;

	
   // compute the 3x3 tranform from tangent space to object space
	float3x3 objToTangentSpace;
	objToTangentSpace[0] = In.T;
	objToTangentSpace[1] = In.B;
	objToTangentSpace[2] = In.N;

   // rotate light dir into tangent space
   Out.lightDir = mul( objToTangentSpace, -lightDir );
   
   // ps 1.1 clamps from 0.0 to 1.0 in tex coords
   Out.lightDir = (Out.lightDir + 1.0) * 0.5;
   
   // send tex coords to pix shader
   Out.HPOS = mul(modelview, In.position);
   Out.TEX0 = In.baseTex;
   Out.TEX1 = In.lmTex;
   Out.TEX2 = In.baseTex;




   return Out;
}

