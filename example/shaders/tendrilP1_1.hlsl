#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 texCoord        : TEXCOORD0;
   float4 specular        : COLOR0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap      : register(S0)
)
{
   Fragout OUT;

   
   OUT.col = tex2D( refractMap, IN.texCoord ) + float4( 0.05, 0.05, 0.1, 1.0 );
   OUT.col += IN.specular.xxxx;

   return OUT;
}

