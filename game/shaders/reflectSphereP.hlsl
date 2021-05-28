//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

struct ConnectData
{
   float3 cubeNormal      : TEXCOORD0;
   float3 cubeEyePos      : TEXCOORD1;
   float3 pos             : TEXCOORD2;
};

/*
struct ConnectData
{
   float3 reflectVec      : TEXCOORD0;
};
*/

struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform samplerCUBE cubeMap         : register(S0)
)
{
   Fragout OUT;


   float3 eyeToPix = IN.pos - IN.cubeEyePos;
   float3 reflectVec = reflect(eyeToPix, IN.cubeNormal);
   OUT.col = texCUBE(cubeMap, reflectVec);
   OUT.col = OUT.col + float4( 0.0, 0.0, 0.1, 0.0 );


/*   
   OUT.col = texCUBE(cubeMap, IN.reflectVec);
   OUT.col = OUT.col + float4( 0.0, 0.0, 0.2, 0.0 );
*/

   return OUT;
}
