//*****************************************************************************
// Precipitation vertex shader
//*****************************************************************************
#define IN_HLSL
#include "shdrConsts.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Vert
{
	float4 position	: POSITION;
	float4 texCoord	: TEXCOORD0;
};

struct Conn
{
	float4 position : POSITION;
	float4 texCoord	: TEXCOORD0;
	float4 color : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(  Vert In, 
            uniform float4x4 modelview : register(VC_WORLD_PROJ),
	    uniform float2 fadeStartEnd : register(C4),
	    uniform float3 cameraPos : register(C5),
	    uniform float3 ambient : register(C6)
)
{
   Conn Out;

   Out.position = mul(modelview, In.position);
   Out.texCoord = In.texCoord;
   Out.color = float4( ambient.r, ambient.g, ambient.b, 1 );

   // Do we need to do a distance fade?
   if ( fadeStartEnd.x < fadeStartEnd.y ) {

      float distance = length( cameraPos - In.position );
      Out.color.a = abs( clamp( ( distance - fadeStartEnd.x ) / ( fadeStartEnd.y - fadeStartEnd.x ), 0, 1 ) - 1 );
   }

   return Out;
}

