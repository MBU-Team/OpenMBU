//*****************************************************************************
// Glow Buffer Data
//*****************************************************************************


//-----------------------------------------------------------------------------
// Blur shader for 1.1 pixel shaders
//-----------------------------------------------------------------------------
new ShaderData( Blur )
{
   DXVertexShaderFile 	= "shaders/blurV.hlsl";
   DXPixelShaderFile 	= "shaders/blurP.hlsl";
   pixVersion = 1.1;
};


//-----------------------------------------------------------------------------
// Glow Buffer Datablock
//-----------------------------------------------------------------------------
new GlowBuffer(GlowBufferData)
{
   shader = Blur;
};
