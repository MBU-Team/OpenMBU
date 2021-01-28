//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************
#define IN_HLSL
#include "..\shdrConsts.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
   float2 texCoord   : TEXCOORD0;
};


struct Conn
{
   float4 HPOS             : POSITION;
	float2 TEX0             : TEXCOORD0;
	float2 TEX1             : TEXCOORD1;
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main( Appdata IN, 
           uniform float4x4 modelview : register(VC_WORLD_PROJ),
           uniform float4   timeData        : register(C54)
)
{
   Conn OUT;

   OUT.HPOS = mul(modelview, IN.position);
   OUT.TEX0 = IN.texCoord;
   OUT.TEX1 = IN.texCoord + float2( timeData.x, 0.3 );

   return OUT;
}


