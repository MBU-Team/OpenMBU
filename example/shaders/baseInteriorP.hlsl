//*****************************************************************************
// Base interior pixel shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data 
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS : POSITION;
	float4 TEX0 : TEXCOORD0_centroid;
	float4 TEX1 : TEXCOORD1_centroid;
	float4 TEX2 : TEXCOORD2_centroid;
	float3 lightDir : TEXCOORD3;
};

struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
				uniform sampler2D diffuseMap     : register(S0),
				uniform sampler2D lightMap       : register(S1),
            uniform sampler2D bumpMap        : register(S2)
)
{
	Fragout OUT;

   float4 bumpNormal = tex2D( bumpMap, IN.TEX2.xy ) * 2.0 - 1.0;
   float4 base = tex2D(diffuseMap, IN.TEX0) * tex2D(lightMap, IN.TEX1);

   // expand light dir
   float3 lightDir = IN.lightDir * 2.0 - 1.0;
   
   float bump = dot( bumpNormal.xyz, lightDir );
   
   OUT.col = base * bump;


	return OUT;
}
