//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------

//-----------------------------------------------------------------------------
// Data 
//-----------------------------------------------------------------------------
struct v2f
{
	float4 HPOS : POSITION;
	float4 TEX0 : TEXCOORD0;
};

struct Fragout
{
	float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
			 uniform sampler2D intensitymap : register(S0),
			 uniform sampler2D bloommap1    : register(S1),
			 //uniform sampler2D bloommap2    : register(S2),
			 uniform sampler2D frame        : register(S3),
			 //uniform sampler2D tonearea     : register(S4),
			 uniform sampler2D tonemap      : register(S5),
			 uniform float4 drlinfo         : register(C0))
{
	Fragout OUT;
	
	float2 halfway;
	halfway = 0.5;
	
	float4 framecol = tex2D(frame, IN.TEX0);
	OUT.col = framecol;
	
	float1 intensity = tex2D(intensitymap, halfway).w;
	float1 fullrange = 1.0 / (intensity + 0.0001);
	
	intensity = drlinfo.z * fullrange;
	intensity = sqrt(intensity);
	intensity = min(intensity, drlinfo.x);
	intensity = max(intensity, drlinfo.y);
	
	//OUT.col.xyz = intensity.x;
	OUT.col.x *= intensity;
	OUT.col.y *= intensity;
	OUT.col.z *= intensity;
	
	framecol.w = max(max(framecol.x, framecol.y), framecol.z);
	
	float2 uv;
	uv.x = framecol.w * fullrange * 0.5;
	uv.y = 0.5;
	
	OUT.col.xyz *= drlinfo.w;
	OUT.col.xyz += tex2D(bloommap1, IN.TEX0).xyz * 2.0 * intensity.x;
	OUT.col.xyz *= tex2D(tonemap, uv).xyz * 2.0;
	
	/*float2 halfway;
	halfway = 0.5;
	
	OUT.col = tex2D(frame, IN.TEX0);
	
	float1 intensity = tex2D(intensitymap, halfway).w;
	float1 fullrange = 1.0 / (intensity + 0.0001);
	
	intensity = drlinfo.z * fullrange;
	intensity = sqrt(intensity);
	
	//float1 fullrange = intensity;
	intensity = min(intensity, drlinfo.x);
	intensity = max(intensity, drlinfo.y);
	
	//OUT.col.xyz = intensity.x;
	
	OUT.col.x *= intensity;
	OUT.col.y *= intensity;
	OUT.col.z *= intensity;
	
	float2 uv;
	float4 tonecol = tex2D(tonearea, IN.TEX0);
	tonecol.w = max(max(tonecol.x, tonecol.y), tonecol.z);
	uv.x = (tonecol.w * fullrange) * 0.5;
	//uv.x = ((tex2D(tonearea, IN.TEX0).w * fullrange) - drlinfo.z) + 0.5;
	//uv.x = (tex2D(tonearea, IN.TEX0).w * fullrange) * 0.5;
	uv.y = 0.5;
	OUT.col.xyz *= drlinfo.w;
	//OUT.col.xyz *= tex2D(tonemap, uv).xyz * 2.0;
	
	OUT.col.xyz += tex2D(bloommap1, IN.TEX0).xyz * 2.0 * intensity.x;
	//OUT.col.xyz += tex2D(bloommap2, IN.TEX0).xyz;
	
	OUT.col.xyz = tex2D(tonemap, uv).xyz * 2.0;
	
	//OUT.col.xyz = uv.x;
*/
	return OUT;
}
