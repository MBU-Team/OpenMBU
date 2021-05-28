sampler Outline: register(s0);
float4 main( float3 eyePos: TEXCOORD0,
             float3 Normal: TEXCOORD1,
             float3 Light1: TEXCOORD2,
             float3 pos:    TEXCOORD3 ) : COLOR
{
   float4 Material = float4(0.56, 0.8, 0.54, 1.0);
   
   float3 viewVec = normalize( eyePos - pos );
   
   // Normalize input normal vector:
   float3 norm = normalize (Normal);
  
   float4 outline = tex1D(Outline, 1 - dot (norm, normalize(viewVec)));

   float lighting = (dot (normalize (Light1), norm) * 0.5 + 0.5);

   return outline * lighting * Material; 
}