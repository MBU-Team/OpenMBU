//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------

float4 getDynamicLighting(sampler3D dynlightmap,
						  float3 texcoord,
						  float4 lightcolor)
{
	return tex3D(dynlightmap, texcoord) * lightcolor;
}

float3 getDynamicLightingCoord(float4 vertpos,
							   float4 lightpos,
							   float4x4 objtrans,
							   float4x4 lighttrans)
{
	float3 coord = ((mul(objtrans, vertpos.xyz) - mul(objtrans, lightpos.xyz)) * lightpos.w);
	return mul(lighttrans, coord) + 0.5;
}

float3 getDynamicLightingDir(float4 vertpos,
							 float4 lightpos)
{
	return normalize(lightpos.xyz - vertpos.xyz);
}

