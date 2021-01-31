//**********   *******************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct FragIn
{
   float4 HPOS       : POSITION;
	float4 tex        : TEXCOORD0;
};

struct FragOut
{
   float4 HPOS       : POSITION;
   float2 tex        : TEXCOORD0;
   float2 tex3       : TEXCOORD1;
   float2 tex4       : TEXCOORD2;
   float2 texLM      : TEXCOORD3;
};

FragOut main(FragIn input)
{
   // Infer the tex coords for the four "real" textures from the
   // ones for the opacity map...
   FragOut OUT;
   
   OUT.HPOS = input.HPOS;
   OUT.tex  = input.tex;
   OUT.tex3 = input.tex * 64;
   OUT.tex4 = input.tex * 64;
   OUT.texLM = input.tex;
   
   return OUT;
}

