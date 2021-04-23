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
    float2 texCoord   : TEXCOORD0;
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
Conn main( Appdata IN, uniform float4x4 modelview : register( VC_WORLD_PROJ ) )
{
   Conn OUT;
   OUT.HPOS = mul( modelview, IN.position );
   OUT.color = IN.color;
   OUT.texCoord = IN.texCoord;
   
   return OUT;
}

