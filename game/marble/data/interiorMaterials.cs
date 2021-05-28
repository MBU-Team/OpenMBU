new CubemapData( iceCubemap )
{
   cubeFace[0] = "~/data/textures/acubexpos2";
   cubeFace[1] = "~/data/textures/acubexneg2";
   cubeFace[2] = "~/data/textures/acubezneg2";
   cubeFace[3] = "~/data/textures/acubezpos2";
   cubeFace[4] = "~/data/textures/acubeypos2";
   cubeFace[5] = "~/data/textures/acubeyneg2";
};

new Material(DefaultMaterial) {

   translucent[0] = false;
   friction = 1;

   restitution = 1;
   force = 0;

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 12.0;
};

new Material(CementMaterial) {
   translucent[0] = false;
   friction = 1;
   restitution = 1;
   force = 0;

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 12.0;
};

//-----------------------------------------------------------------------------
// Set Dressing Textures
//-----------------------------------------------------------------------------

new ShaderData( NoiseTile )
{
   DXVertexShaderFile   = "shaders/noiseTileV.hlsl";
   DXPixelShaderFile    = "shaders/noiseTileP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( HalfTile )
{
   DXVertexShaderFile   = "shaders/halfTileV.hlsl";
   DXPixelShaderFile    = "shaders/halfTileP.hlsl";
   pixVersion = 2.0;
};


// Metal Plate random tile texture

%mat = new CustomMaterial( Material_Plate )
{
   mapTo = plate_1;
   texture[0] = "./textures/plate.randomize";
   texture[1] = "./textures/plate.normal";

   friction = 1;
   restitution = 1;
   force = 0;   

   specular[0] = "1.0 1.0 0.8 1.0";
   specularPower[0] = 8.0;

   shader = HalfTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Beginner )
{
   mapTo = tile_beginner;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Beginner_Red  )
{
   mapTo = tile_beginner_red;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_red";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};


%mat = new CustomMaterial( Material_Tile_Beginner_Blue  )
{
   mapTo = tile_beginner_blue;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_blue";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};


%mat = new CustomMaterial( Material_Tile_Intermediate  )
{
   mapTo = tile_intermediate;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
      force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Intermediate_green  )
{
   mapTo = tile_intermediate_green;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_green";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Intermediate_red  )
{
   mapTo = tile_intermediate_red;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_red";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced  )
{
   mapTo = tile_advanced;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced_Blue  )
{
   mapTo = tile_advanced_blue;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_blue";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;
   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced_Green  )
{
   mapTo = tile_advanced_green;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_green";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Beginner_shadow )
{
   mapTo = tile_beginner_shadow;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Beginner_Red_shadow  )
{
   mapTo = tile_beginner_red_shadow;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_red_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};


%mat = new CustomMaterial( Material_Tile_Beginner_Blue_shadow  )
{
   mapTo = tile_beginner_blue_shadow;
   texture[0] = "./textures/tile_beginner";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_blue_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};


%mat = new CustomMaterial( Material_Tile_Intermediate_shadow  )
{
   mapTo = tile_intermediate_shadow;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
      force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Intermediate_green_shadow  )
{
   mapTo = tile_intermediate_green_shadow;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_green_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Intermediate_red_shadow  )
{
   mapTo = tile_intermediate_red_shadow;
   texture[0] = "./textures/tile_intermediate";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_red_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced_shadow  )
{
   mapTo = tile_advanced_shadow;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced_Blue_shadow  )
{
   mapTo = tile_advanced_blue_shadow;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_blue_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;
   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Advanced_Green_shadow  )
{
   mapTo = tile_advanced_green_shadow;
   texture[0] = "./textures/tile_advanced";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise_green_shadow";
   
   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 40.0;

   friction = 1;
   restitution = 1;
   force = 0;

   shader = NoiseTile;
   version = 2.0;
};

%mat = new CustomMaterial( Material_Tile_Underside  )
{
   mapTo = tile_underside;
   texture[0] = "./textures/tile_underside";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "1 1 1 1";
   specularPower[0] = 40.0;

   shader = NoiseTile;
   version = 2.0;
};


%mat = new Material(Material_Wall_Beginner : DefaultMaterial) {
   mapTo="wall_beginner";

   baseTex[0] = "./textures/wall_beginner";
   //bumpTex[0] = "./textures/plate.normal";
};

%mat = new Material(Material_Edge_White : DefaultMaterial)
{
   mapTo = "edge_white";
   baseTex[0] = "./textures/edge_white";
   bumpTex[0] = "./textures/edge.normal";

   specular[0] = "0.8 0.8 0.8 1.0";
   specularPower[0] = 50.0;
   
};

%mat = new Material(Material_Edge_White_shadow : DefaultMaterial)
{
   mapTo = "edge_white_shadow";
   baseTex[0] = "./textures/edge_white_shadow";
   bumpTex[0] = "./textures/edge.normal";

   specular[0] = "0.2 0.2 0.2 0.2";
   specularPower[0] = 50.0;
   
};

%mat = new Material(Material_beam : DefaultMaterial) {
   baseTex[0] = "./textures/beam";
   bumpTex[0] = "./textures/beam.normal";
};

%mat = new Material(Material_BeamSide : DefaultMaterial) {
   baseTex[0] = "./textures/beam_side";
   bumpTex[0] = "./textures/beam_side.normal";
};

%mat = new Material(Tube_beginner : DefaultMaterial)
{
   mapto = "tube_neutral";
   baseTex[0] = "./textures/tube_beginner";
};

%mat = new Material(Tube_intermediate : DefaultMaterial)
{
   mapto="tube_cool";
   baseTex[0] = "./textures/tube_intermediate";
};

%mat = new Material(Tube_Advanced : DefaultMaterial)
{
   mapto="tube_warm";
   baseTex[0] = "./textures/tube_advanced";
};

// ---------------------------------------------------------------------------
// Friction Materials
// ---------------------------------------------------------------------------

%mat = new Material(Material_LowFriction) {
   baseTex[0] = "./textures/friction_low";
   bumpTex[0] = "./textures/friction_low.normal";
   friction = 0.20;
   restitution = 1.0;
   force = 0;
   
   mapTo = friction_low;
   
   cubemap[0] = iceCubemap;
   
   //FUCK ICE
   //baseTex[1] = "./textures/friction_low";
   //bumpTex[1] = "./textures/friction_low.normal";
   //translucent[1] = true;
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 0.8";
   specularPower[0] = 128.0;
};

//updated
%mat = new Material(Material_HighFriction : DefaultMaterial) {
   friction = 4.5;
   restitution = 0.5;
   force = 0;
  
   specular[0] = "0.3 0.3 0.35 1.0";
   specularPower[0] = 10.0;
   
   MAPTO = "friction_high";
   baseTex[0] = "./textures/friction_high";
   bumpTex[0] = "./textures/friction_high.normal";
};

%mat = new Material(Material_HighFriction_Shadow : DefaultMaterial) {
   friction = 4.5;
   restitution = 0.5;
   force = 0;
  
   specular[0] = "0.15 0.15 0.16 1.0";
   specularPower[0] = 10.0;
   
   MAPTO = "friction_high_shadow";
   baseTex[0] = "./textures/friction_high_shadow";
   bumpTex[0] = "./textures/friction_high.normal";
};

%mat = new Material(Material_LowFriction_Shadow) {
   baseTex[0] = "./textures/friction_low_shadow";
   bumpTex[0] = "./textures/friction_low.normal";
   friction = 0.20;
   restitution = 1.0;
   force = 0;
   
   mapTo = friction_low_shadow;
   
   cubemap[0] = iceCubemap;
   
   pixelSpecular[0] = true;
   specular[0] = "0.3 0.3 0.35 1.0";
   specularPower[0] = 128.0;
};

// Adding this back in
%mat = new Material(Material_stripe_caution : DefaultMaterial)
{
   mapTo = stripe_caution;
   baseTex[0] = "./textures/stripe_caution";
};