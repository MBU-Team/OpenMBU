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

// use linear and binary search
float3 ray_intersect_rm(
      in sampler2D reliefmap,
      in float3 s, 
      in float3 ds)
{
   const int linear_search_steps=10;
   const int binary_search_steps=5;
  
   ds/=linear_search_steps;

   for( int i=0;i<linear_search_steps-1;i++ )
   {
		float4 t=tex2D(reliefmap,s);

		if (s.z<t.w)
			s+=ds;
   }

   for( int i=0;i<binary_search_steps;i++ )
   {
		ds*=0.5;
		float4 t=tex2D(reliefmap,s);
		if (s.z<t.w)
			s+=2*ds;
		s-=ds;
   }

   return s;
}

// only linear search for shadows
float3 ray_intersect_rm_lin(
      in sampler2D reliefmap,
      in float3 s, 
      in float3 ds)
{
   const int linear_search_steps=10;
   ds/=linear_search_steps;

   for( int i=0;i<linear_search_steps-1;i++ )
   {
		float4 t=tex2D(reliefmap,s);
		if (s.z<t.w)
			s+=ds;
   }
   
   return s;
}

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
	const float  depth    = 0.05;
	const float3 diffuse  = {1,1,1};
	
	// view direction
	float3 v = normalize(IN.eye);
	
	// serach start point and serach vector with depth bias
	float3 s = float3(IN.texcoord,0);
	v.xy *= depth*(2*v.z-v.z*v.z);
	v /= v.z;

	// ray intersect depth map
	float3 tx = ray_intersect_rm(reliefmap,s,v);

	// displace start position to intersetcion point
	s.xy += v.xy*tx.z;

	// get normal, lightmap and color at intersection point
	float4 t = tex2D(reliefmap,tx.xy);
	float4 c = tex2D(texmap,tx.xy);
	float4 lm = tex2D(lightmap, IN.lmcoord);

	// expand normal
	t.xyz = t.xyz*2-1;
	t.y = -t.y;

	// light direction
	float3 l = normalize(IN.light);
	
	// view direction
	v = normalize(IN.eye);
	v.z = -v.z;

	// compute diffuse and specular terms
	float diff = saturate(dot(l,t.xyz));
	float spec = saturate(dot(normalize(l-v),t.xyz));

	// attenuation factor
	float att=1.0-max(0,l.z); att=1.0-att*att;

	// light serch vector with depth bias
	l.xy *= depth*(2*l.z-l.z*l.z);
	l.z = -l.z;
	l /= l.z;
	
	// displace start position to light entry point
	s.xy -= l.xy*tx.z;
	
	// ray intersect from light
	float3 tx2 = ray_intersect_rm_lin(reliefmap,s,l);
	
	// if pixel in shadow zero attenuation
	if (tx2.z<tx.z-0.05) 
		att=0;

	// compute final color
	float4 finalcolor;
	finalcolor.xyz = ambient*c.xyz + 
		att*(c.xyz*diffuse*diff + 
		specular*pow(spec,shine)) *
		lm;
	finalcolor.w=1.0;

	return finalcolor;
}
