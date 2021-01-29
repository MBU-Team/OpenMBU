#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float3 eyePos          : TEXCOORD1;
   float3 normal          : TEXCOORD2;
   float3 pos             : TEXCOORD3;
   float4 hpos2           : TEXCOORD4;
   float3 screenNorm      : TEXCOORD5;
   float3 lightVec        : TEXCOORD6;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap   : register(S0),
              uniform sampler2D colorMap     : register(S1),
              uniform float4 specularColor   : register(PC_MAT_SPECCOLOR),
              uniform float  specularPower   : register(PC_MAT_SPECPOWER)
)
{
   Fragout OUT;

   float3 eyeVec = normalize( IN.eyePos - IN.pos );
   float2 texCoord2;
   texCoord2.x = saturate( dot( IN.normal, eyeVec ) ) - (0.5/128.0);
   texCoord2.y = 0.0;
   
   float4 grad = tex2D( colorMap, texCoord2 );
   
   
   
   
   float3 refractVec = refract( float3( 0.0, 0.0, 1.0 ), IN.screenNorm, 0.7 );
   
   float2 tc;
   tc = float2( (  IN.hpos2.x + refractVec.x ) / (IN.hpos2.w), 
                ( -IN.hpos2.y + refractVec.y ) / (IN.hpos2.w) );
   
   tc = saturate( (tc + 1.0) * 0.5 );
   
   OUT.col = tex2D( refractMap, tc ) + grad;

   
//   float3 lightVec = -normalize( float3( 0.0, 0.0, -1.0 ) );
   
//   float foo = saturate( dot( IN.lightVec, IN.normal ) );
   
   
   float3 halfAng = normalize(eyeVec + IN.lightVec);
   float specular = saturate( dot(IN.normal.xyz, halfAng) );
   specular = pow(specular, specularPower);
   
   
   OUT.col += specularColor * specular;




   return OUT;
}
