//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS             : POSITION;
	float2 TEX0             : TEXCOORD1;
   float3 reflectVec       : TEXCOORD2;
   float2 fogCoord         : TEXCOORD3;
};



struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(  v2f IN,
   				uniform sampler      texMap   : register(S1),
               uniform samplerCUBE  cubeMap  : register(S2),
               uniform sampler       fogMap  : register(S3)
)
{
	Fragout OUT;

   float4 fresCol = tex1D( texMap, IN.TEX0 );
   float4 skyCol = texCUBE( cubeMap, IN.reflectVec );
   skyCol.a = 1.0 - fresCol.a;
   
   
   OUT.col = lerp( skyCol, fresCol, fresCol.a );
   OUT.col.rgb *= 0.5;

   float4 fogColor = tex2D( fogMap, IN.fogCoord );
   OUT.col = lerp( OUT.col, fogColor, fogColor.a );


	return OUT;
}

