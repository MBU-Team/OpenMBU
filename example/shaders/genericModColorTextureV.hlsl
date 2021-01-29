//*****************************************************************************
// GUI Shader
//*****************************************************************************
#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
	float4 color      : COLOR;
	float2 texCoord   : TEXCOORD;
};


struct Conn
{
   float4 HPOS             : POSITION;
   float4 color            : COLOR;
   float2 texCoord         : TEXCOORD0;
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main( Appdata In, uniform float4x4 modelview : register( VC_WORLD_PROJ ) )
{
   Conn Out;
   Out.HPOS = mul( modelview, In.position );
   Out.color = In.color;
   Out.texCoord = In.texCoord;
   
   return Out;
}

