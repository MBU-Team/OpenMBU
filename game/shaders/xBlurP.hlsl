//*****************************************************************************
// Glow Shader
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 tex0            : TEXCOORD0;
   float2 vpos            : VPOS;
};


struct Fragout
{
   half4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D  diffuseMap0       : register(S0),
              uniform float3     kernel[13]         : register(C0),
              uniform float      divisor           : register(C30)
)
{
   Fragout OUT;


   float4 accumColor = {0.0,0.0,0.0,0.0};

   for( int i=0; i<13; i++ )
   {
      accumColor += tex2D( diffuseMap0, IN.tex0 + kernel[i].xy ) * kernel[i].z;
   }



   OUT.col = accumColor / divisor;
   OUT.col.a  = OUT.col.x + OUT.col.y + OUT.col.z / 3.0;
   OUT.col.xyz *= 2.0;

   return OUT;
}

