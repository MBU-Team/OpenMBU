#define IN_HLSL
#include "shdrConsts.h"

struct a2v 
{
	float4 texcoord        : TEXCOORD0;
	float2 lmCoord         : TEXCOORD1;
	float3 tangent         : TEXCOORD2;
	float3 binormal        : TEXCOORD3;
	float3 N               : TEXCOORD4;
	float3 normal          : NORMAL;
	float4 pos             : POSITION;
};

struct v2f
{
	float4 hpos     : POSITION;
	float3 eye      : TEXCOORD0;
	float3 light    : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
	float2 lmcoord  : TEXCOORD3;
};

v2f main(a2v IN,
         uniform float4x4 modelview       : register(VC_WORLD_PROJ),
         uniform float4x4 texMat          : register(VC_TEX_TRANS1),
         uniform float3   lightpos        : register(VC_LIGHT_DIR1),
         uniform float3   eyePos          : register(VC_EYE_POS),
         uniform float4x4 objTrans        : register(VC_OBJ_TRANS))
{
	v2f OUT;

	// vertex position in object space
	float4 pos=float4(IN.pos.x,IN.pos.y,IN.pos.z,1.0);

	// vertex position in clip space
	OUT.hpos=mul(modelview, pos);

	// copy color and texture coordinates
	OUT.texcoord = mul(texMat, IN.texcoord);
	
	// copy light map texture coordinates
	OUT.lmcoord = mul(texMat, IN.lmCoord);

	// tangent vectors in view space
	float3x3 tangentspace=float3x3(IN.tangent,IN.binormal,IN.normal);

	// vertex position in view space (with model transformations)
	float3 vpos=mul(modelview, pos).xyz;

	// view in tangent space
	float3 objectspace_view_vector = mul( texMat, eyePos ) - IN.pos;
    OUT.eye = mul( tangentspace, objectspace_view_vector );
    OUT.eye = - OUT.eye;
	
	OUT.light.xyz = -lightpos;
	OUT.light.xyz = mul(tangentspace, OUT.light);
	OUT.light = OUT.light / 2.0 + 0.5;

	return OUT;
}
