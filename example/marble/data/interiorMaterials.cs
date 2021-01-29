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

%mat = new Material(cement : CementMaterial)
{
   mapto = "cement";
   baseTex[0] = "./textures/cement";
};

// ---------------------------------------------------------------------------
// Bounce Materials
// ---------------------------------------------------------------------------

%mat = new CustomMaterial( Material_Tile_Bounce  )
{
   mapTo = tile_bouncy;
   texture[0] = "./textures/tile_bouncy";
   texture[1] = "./textures/tile_intermediate.normal";
   texture[2] = "./textures/noise";
   
   specular[0] = "0.8 0.8 0.8 1.0";
   specularPower[0] = 30.0;

   shader = NoiseTile;
   version = 2.0;
   
   friction = 0.01;
   restitution = 3.5;
   force = 0;
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

%mat = new Material(Material_VeryHighFriction : DefaultMaterial) {
   friction = 2;
   restitution = 1;
   force = 0;
   
   baseTex[0] = "./textures/friction_ramp_yellow";
   bumpTex[0] = "./textures/friction_ramp_yellow.bump";
};


//new Material(RubberFloorMaterial) {
//   friction = 1;
//   restitution = 1;
//   force = 0;
//};
//
//new Material(IceMaterial) {
//   friction = 0.05;
//   restitution = 0.5;
//   force = 0;
//};
//
//new Material(BumperMaterial) {
//   friction = 0.5;
//   restitution = 0;
//   force = 15;
//};
//
//new Material(ButtonMaterial) {
//   friction = 1;
//   restitution = 1;
//   force = 0;
//};

// ---------------------------------------------------------------------------
// Grid materials
// ---------------------------------------------------------------------------

%mat = new Material(Material_GridWarm1 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_warm1";
   bumpTex[0] = "./textures/grid_4square.bump";
};

%mat = new Material(Material_GridWarm2 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_warm2";
   bumpTex[0] = "./textures/grid_square.bump";
};

%mat = new Material(Material_GridWarm3 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_warm3";
   bumpTex[0] = "./textures/grid_square.bump";
};

%mat = new Material(Material_grid_neutral3 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_neutral3";
   bumpTex[0] = "./textures/grid_square.bump";
};

// ---------------------------------------------------------------------------
// Edge, Wall, Stripe materials
// ---------------------------------------------------------------------------
%mat = new Material(Material_edge_white : DefaultMaterial)
{
   baseTex[0] = "./textures/edge_white";
   bumpTex[0] = "./textures/edge_white.bump";
};

%mat = new Material(Material_edge_white2 : DefaultMaterial)
{
   baseTex[0] = "./textures/edge_white2";
   bumpTex[0] = "./textures/edge_white2.bump";
};

%mat = new Material(Material_wall_white : DefaultMaterial)
{
   baseTex[0] = "./textures/wall_white";
};

%mat = new Material(Material_wall_warm2 : DefaultMaterial)
{
   baseTex[0] = "./textures/wall_warm2";
   bumpTex[0] = "./textures/wall.bump";
};

%mat = new Material(Material_stripe_caution : DefaultMaterial)
{
   baseTex[0] = "./textures/stripe_caution";
};

%mat = new Material(Material_stripe_warm2 : DefaultMaterial)
{
   baseTex[0] = "./textures/stripe_warm2";
};


// ---------------------------------------------------------------------------
// Pattern materials
// ---------------------------------------------------------------------------
%mat = new Material(Material_pattern_cool2 : DefaultMaterial)
{
   baseTex[0] = "./textures/pattern_cool2";
   bumpTex[0] = "./textures/pattern_cool2.bump";
};

%mat = new Material(Material_pattern_warm3 : DefaultMaterial)
{
   baseTex[0] = "./textures/pattern_warm3";
};

%mat = new Material(Material_pattern_warm4 : DefaultMaterial)
{
   baseTex[0] = "./textures/pattern_warm4";
};

%mat = new Material(Material_chevron_warm : DefaultMaterial)
{
   baseTex[0] = "./textures/chevron_warm";
   bumpTex[0] = "./textures/chevron_warm.bump";
};

%mat = new Material(Material_trim_warm2 : DefaultMaterial)
{
   baseTex[0] = "./textures/trim_warm2";
   bumpTex[0] = "./textures/trim_warm2.bump";
};

%mat = new Material(Material_wall_neutral2 : DefaultMaterial)
{
   baseTex[0] = "./textures/wall_neutral2";
   bumpTex[0] = "./textures/wall_neutral2.bump";
};

%mat = new Material(grid_neutral : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_neutral";
   bumpTex[0] = "./textures/grid_neutral.bump";
};

//new ShaderData( ReflectBump )
//{
//   DXVertexShaderFile 	= "shaders/planarReflectBumpV.hlsl";
//   DXPixelShaderFile 	= "shaders/planarReflectBumpP.hlsl";
//   pixVersion = 2.0;
//};
//
//%mat = new CustomMaterial(Material_Grey)
//{
//   friction = 1;
//   restitution = 1;
//   force = 0;
//
//   texture[1] = "$backbuff";
//   texture[0] = "marble/data/interiors/grey";
//   texture[2] = "marble/data/interiors/noise.bump";
//   shader = ReflectBump;
//   version = 2.0;
//   planarReflection = true;
//};
//

%mat = new Material(Material_wall_neutral3 : DefaultMaterial)
{
   baseTex[0] = "./textures/wall_neutral3";
   bumpTex[0] = "./textures/wall.bump";
};

%mat = new Material(Material_grid_neutral1 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_neutral1";
   bumpTex[0] = "./textures/grid_square.bump";
};

%mat = new Material(Pattern_Cool1 : DefaultMaterial)
{
   baseTex[0] = "./textures/pattern_cool1";
};

%mat = new Material(Pattern_Warm1 : DefaultMaterial)
{
   baseTex[0] = "./textures/pattern_warm1";
};

%mat = new Material(Grid_Cool2 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_cool2";
};

%mat = new Material(Trim_Cool1 : DefaultMaterial)
{
   baseTex[0] = "./textures/trim_cool1";
};

%mat = new Material(Solid_Neutral2 : DefaultMaterial)
{
   baseTex[0] = "./textures/solid_neutral2";
};

%mat = new Material(Wall_Cool1 : DefaultMaterial)
{
   baseTex[0] = "./textures/Wall_Cool1";
};

%mat = new Material(Solid_Warm2 : DefaultMaterial)
{
   baseTex[0] = "./textures/solid_warm2";
};

%mat = new Material(Grid_Warm : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_warm";
};

%mat = new Material(Grid_Neutral4 : DefaultMaterial)
{
   baseTex[0] = "./textures/grid_neutral4";
};

%mat = new Material(Trim_Cool3 : DefaultMaterial)
{
   baseTex[0] = "./textures/trim_cool3";
};
