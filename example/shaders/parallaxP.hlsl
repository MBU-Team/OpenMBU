#define IN_HLSL
#include "shdrConsts.h"

struct v2f
{
	float4 hpos     : POSITION;
	float3 eye      : TEXCOORD0;
	float3 light    : TEXCOORD1;
	float2 texcoord : TEXCOORD2;
	float2 lmcoord  : TEXCOORD3;
};

float4 main(
	v2f IN,
	uniform sampler2D texmap    : register(S0),
	uniform sampler2D reliefmap : register(S1),
	uniform sampler2D lightmap  : register(S2),
	uniform float4    ambient   : register(PC_AMBIENT_COLOR),
	uniform float4    specular  : register(PC_MAT_SPECCOLOR),
	uniform float     shine     : register(PC_MAT_SPECPOWER)
	) : COLOR
{
	const float3 diffuse  = {1,1,1};
	
   	// view and light directions
	float3 v = normalize(IN.eye);
	float3 l = normalize(IN.light);

	float2 uv = IN.texcoord;

	// parallax code
	float height = tex2D(reliefmap,uv).w * 0.06 - 0.03;
	uv += height * v.xy;

	// normal map
	float4 normal=tex2D(reliefmap,uv);
	normal.xyz=normalize(normal.xyz-0.5);
	normal.y=-normal.y;
	
	// color map
	float4 color=tex2D(texmap,uv);
	
	// light map
	float4 lm = tex2D(lightmap, IN.lmcoord);

	// compute diffuse and specular terms
	float diff=saturate(dot(l,normal.xyz));
	float spec=saturate(dot(normalize(l-v),normal.xyz));
	
	// attenuation factor
	float att=1.0-max(0,l.z); att=1.0-att*att;

	// compute final color
	float4 finalcolor;
	finalcolor.xyz = ambient*color.xyz +
		att*(color.xyz*diffuse*diff + 
		specular*pow(spec,shine)) *
		lm;
	finalcolor.w=1.0;

	return finalcolor;
}
