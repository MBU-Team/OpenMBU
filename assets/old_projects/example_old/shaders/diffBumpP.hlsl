//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
   float2 bumpCoord       : TEXCOORD1;
   float3 lightVec        : TEXCOORD2;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D diffuseMap      : register(S0),
              uniform sampler2D bumpMap         : register(S1),
              uniform float4    ambient         : register(C2)
)
{
   Fragout OUT;

   OUT.col = tex2D(diffuseMap, IN.texCoord);
   float4 bumpNormal = tex2D(bumpMap, IN.bumpCoord);

   IN.lightVec = IN.lightVec * 2.0 - 1.0;
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz) );
   OUT.col *= bumpDot + ambient;

   return OUT;
}
