//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 color    : COLOR;
   float2 texCoord : TEXCOORD0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN, uniform sampler2D diffuseMap : register(S0),
                              uniform sampler1D gammaRamp : register(S1),
                              uniform float gammaRampInvSize : register(C0))
{
    Fragout OUT;

    float4 color = tex2D(diffuseMap, IN.texCoord);

    // The center of the first texel of the LUT contains the value for 0, and the
    // center of the last texel contains the value for 1.

    color = color * (1.0f - gammaRampInvSize) + (0.5 * gammaRampInvSize);
	
    color.x = tex1D(gammaRamp, color.x);
    color.y = tex1D(gammaRamp, color.y);
    color.z = tex1D(gammaRamp, color.z);

    // Force alpha to 1 to make sure the surface won't be translucent.
    OUT.col = float4(color.xyz, 1.0f);

    return OUT;
}

