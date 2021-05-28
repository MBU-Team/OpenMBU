new Material(sizeMaterial) {
   friction = 1;
   restitution = 1;
   force = 0;
   
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 12.0;
};

%mat = new Material(Material_sizer_circle : sizeMaterial)
{
   baseTex[0] = "./textures/sizer_circle";
   
};

%mat = new Material(Material_sizer_32 : sizeMaterial)
{
   baseTex[0] = "./textures/sizer_32";
   
};

%mat = new Material(Material_sizer_64 : sizeMaterial)
{
   baseTex[0] = "./textures/sizer_64";
   
};

%mat = new Material(Material_sizer_128 : sizeMaterial)
{
   mapTo = "sizer_128";
   baseTex[0] = "./textures/edge_white";
   bumpTex[0] = "./textures/edge.normal";
   
   friction = 2;
   restitution = 1;
   force = 0;
};

//%mat = new Material(Material_sizer_512 : sizeMaterial)
//{
//   baseTex[0] = "./textures/sizer_512";
//   
//};

%mat = new Material(Material_sizer_1024 : sizeMaterial)
{
   baseTex[0] = "./textures/sizer_1024";
};

//-----------------------------------------------------------------------------
// Noise tile
//-----------------------------------------------------------------------------
new ShaderData( NoiseTile )
{
   DXVertexShaderFile   = "shaders/noiseTileV.hlsl";
   DXPixelShaderFile    = "shaders/noiseTileP.hlsl";
   pixVersion = 2.0;
};

%mat = new CustomMaterial( Material_sizer_256  )
{
   mapTo = sizer_256;
   texture[0] = "./textures/sizer_256";
   texture[1] = "./textures/sizer_512.bump";
   texture[2] = "./textures/noise";
   
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 12.0;
   pixelSpecular[0] = true;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Tile_Intermediate  )
{
   mapTo = sizer_512;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "0.75 0.8 0.8 1.0";
   specularPower[0] = 12.0;
   pixelSpecular[0] = true;

   shader = NoiseTile;
   version = 2.0;
};

//-----------------------------------------------------------------------------
// Reflect tile
//-----------------------------------------------------------------------------
new ShaderData( ReflectTile )
{
   DXVertexShaderFile   = "shaders/planarReflectV.hlsl";
   DXPixelShaderFile    = "shaders/planarReflectP.hlsl";
   pixVersion = 2.0;
};

%mat = new CustomMaterial( reflect )
{
   mapTo = reflect;
   texture[0] = "./textures/reflect";
     
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 12.0;

   shader = ReflectTile;
   version = 2.0;
};
