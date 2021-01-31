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
   float4 tex        : TEXCOORD0;
   float4 tex1       : TEXCOORD1;
   float4 tex2       : TEXCOORD2;
   float4 tex3       : TEXCOORD3;
   float4 tex4       : TEXCOORD4;
};

FragOut main(FragIn input)
{
   // Infer the tex coords for the four "real" textures from the
   // ones for the opacity map...
   FragOut OUT;
   
   OUT.HPOS = input.HPOS;
   OUT.tex  = input.tex;
   OUT.tex1 = input.tex * 64;
   OUT.tex2 = input.tex * 64;
   OUT.tex3 = input.tex * 64;
   OUT.tex4 = input.tex * 64;
   
   return OUT;
}

