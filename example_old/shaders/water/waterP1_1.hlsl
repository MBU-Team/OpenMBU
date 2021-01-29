//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS             : POSITION;
	float2 TEX0             : TEXCOORD0;
	float4 tangentToCube0   : TEXCOORD1;
	float4 tangentToCube1   : TEXCOORD2;
	float4 tangentToCube2   : TEXCOORD3;
};



struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
				uniform sampler      bumpMap  : register(S0),
            uniform samplerCUBE  cubeMap  : register(S3)
)
{
	Fragout OUT;

   float3 bumpNorm = tex2D( bumpMap, IN.TEX0 ) * 2.0 - 1.0;
   
   float3 eye = float3( IN.tangentToCube0.w, 
                        IN.tangentToCube1.w, 
                        IN.tangentToCube2.w );

   float3x3 cubeMat = float3x3(  IN.tangentToCube0.xyz,
                                 IN.tangentToCube1.xyz,
                                 IN.tangentToCube2.xyz );

   float3 norm = mul( cubeMat, bumpNorm );

   float3 reflectVec = 2.0 * dot( eye, norm ) * norm - eye * dot( norm, norm );
                       
   OUT.col = texCUBE(cubeMap, reflectVec );

   OUT.col.a = 0.5;

	return OUT;
}

