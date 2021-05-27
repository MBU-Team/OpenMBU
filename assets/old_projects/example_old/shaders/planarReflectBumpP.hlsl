//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 texCoord   : TEXCOORD0;
   float2 tex2       : TEXCOORD1;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Fade edges of axis for texcoord passed in
//-----------------------------------------------------------------------------
float fadeAxis( float val )
{
   // Fades from 1.0 to 0.0 when less than 0.1
   float fadeLow = saturate( val * 10.0 );
   
   // Fades from 1.0 to 0.0 when greater than 0.9
   float fadeHigh = 1.0 - saturate( (val - 0.9) * 10.0 );

   return fadeLow * fadeHigh;
}


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap      : register(S1),
              uniform sampler2D texMap          : register(S0),
              uniform sampler2D bumpMap         : register(S2)
)
{
   Fragout OUT;

   float3 bumpNorm = tex2D( bumpMap, IN.tex2 ) * 2.0 - 1.0;
   float2 offset = float2( bumpNorm.x, bumpNorm.y );
   float4 texIndex = IN.texCoord;

   // The fadeVal is used to "fade" the distortion at the edges of the screen.
   // This is done so it won't sample the reflection texture out-of-bounds and create artifacts
   // Note - this can be done more efficiently with a texture lookup
   float fadeVal = fadeAxis( texIndex.x / texIndex.w ) * fadeAxis( texIndex.y / texIndex.w );

   const float distortion = 0.2;
   texIndex.xy += offset * distortion * fadeVal;

   float4 reflectColor = tex2Dproj( refractMap, texIndex );
   float4 diffuseColor = tex2D( texMap, IN.tex2 );

   OUT.col = diffuseColor + reflectColor * diffuseColor.a;

   return OUT;
}
