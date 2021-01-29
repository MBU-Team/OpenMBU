#define IN_HLSL
#include "shdrConsts.h"

//float4 view_position;
float3 eye_pos: register(VC_EYE_POS);
float4 light0: register(VC_LIGHT_POS1);
float4x4 view_proj_matrix: register(VC_WORLD_PROJ);
struct VS_OUTPUT
{
   float4 Pos   : POSITION;
   float3 eyePos: TEXCOORD0;
   float3 Normal: TEXCOORD1;
   float3 Light1: TEXCOORD2;
   float3 pos:    TEXCOORD3;
};
VS_OUTPUT main( float4 inPos    : POSITION, 
                float3 inNorm   : NORMAL )
{
   VS_OUTPUT Out = (VS_OUTPUT) 0;

   // Output transformed vertex position: 
   Out.Pos = mul( view_proj_matrix, inPos );

   Out.Normal = inNorm;

   // Compute the view vector: 
   //Out.View = normalize( view_position - inPos );
   Out.eyePos = eye_pos;
   Out.pos = inPos;

   // Compute vectors to three lights from the current vertex position: 
   Out.Light1 = normalize(light0 - inPos);   // Light 1

   return Out;
}