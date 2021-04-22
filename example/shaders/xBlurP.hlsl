//*****************************************************************************
// Glow Shader
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 tex0            : TEXCOORD0;
   float2 tex1            : TEXCOORD1;
   float2 tex2            : TEXCOORD2;
   float2 tex3            : TEXCOORD3;
};


struct Fragout
{
   half4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D diffuseMap0  : register(S0),
              uniform sampler2D diffuseMap1  : register(S1),
              uniform sampler2D diffuseMap2  : register(S2),
              uniform sampler2D diffuseMap3  : register(S3),
              uniform float4     kernel[1]  : register(C0),
              uniform float      divisor     : register(C2)
)
{
   Fragout OUT;

	
   float4 accumColor = {0.0,0.0,0.0,0.0};
	
   for( int i=0; i<1; i++ )
   {
		accumColor += tex2D(diffuseMap0, IN.tex0 ) * kernel[i].x;
		accumColor += tex2D(diffuseMap1, IN.tex1 ) * kernel[i].y;
		accumColor += tex2D(diffuseMap2, IN.tex2 ) * kernel[i].z;
      accumColor += tex2D(diffuseMap3, IN.tex3 ) * kernel[i].w;
   }

   OUT.col = accumColor / divisor;
   OUT.col.a = OUT.col.x + OUT.col.y + OUT.col.z / 3.0;
	OUT.col.xyz *= 1.25;

   return OUT;
}

