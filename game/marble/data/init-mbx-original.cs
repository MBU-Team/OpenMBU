new MaterialProperty(DefaultMaterial) {
   friction = 1;
   restitution = 1;
   force = 0;
};


// Will need to play with these three friction values to balance game play
new MaterialProperty(NoFrictionMaterial) {
   friction = 0.01;
   restitution = 0.5;
};

new MaterialProperty(LowFrictionMaterial) {
   friction = 0.20;
   restitution = 0.5;
   bumpMapName = "marble/data/missions/Interior Textures/friction_low.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(HighFrictionMaterial) {
   friction = 1.50;
   restitution = 0.5;
};

new MaterialProperty(VeryHighFrictionMaterial) {
   friction = 2;
   restitution = 1;
   bumpMapName = "marble/data/missions/Interior Textures/friction_ramp_yellow.bump";
   shaderName = "BumpIntSurfData";
};



new MaterialProperty(RubberFloorMaterial) {
   friction = 1;
   restitution = 1;
};

new MaterialProperty(IceMaterial) {
   friction = 0.05;
   restitution = 0.5;
};

new MaterialProperty(BumperMaterial) {
   friction = 0.5;
   restitution = 0;
   force = 15;
};

new MaterialProperty(ButtonMaterial) {
   friction = 1;
   restitution = 1;
};


new MaterialProperty(GridSwirl){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/grid_circle.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(Grid9Box){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/grid_square.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(GridTris){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/grid_triangle.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(Grid4Box){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/grid_4square.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(PatternCool1){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/pattern_cool1.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(PatternCool2){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/pattern_cool2.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(PatternWarm24){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/pattern_warm24.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(EdgeWhite){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/edge_white.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(EdgeWhite2){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/edge_white2.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(Wall){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/wall.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(Chevron){
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/chevron.bump";
   shaderName = "BumpIntSurfData";
};

new MaterialProperty(Trim) {
   friction = 1;
   restitution = 1;
   force = 0;
   bumpMapName = "marble/data/missions/Interior Textures/trim_warm2.bump";
   shaderName = "BumpIntSurfData";
};

//
addMaterialMapping( "", DefaultMaterial);

addMaterialMapping( "trim_warm2", Trim );

addMaterialMapping( "edge_white", EdgeWhite);
addMaterialMapping( "edge_white2", EdgeWhite2 );

addMaterialMapping( "edge_warm1", EdgeWhite);
addMaterialMapping( "edge_warm2", EdgeWhite );
addMaterialMapping( "edge_cool1", EdgeWhite);
addMaterialMapping( "edge_cool2", EdgeWhite );
addMaterialMapping( "edge_neutral1", EdgeWhite);
addMaterialMapping( "edge_neutral2", EdgeWhite );

// Textures listed in BrianH texture document
addMaterialMapping( "chevron_cool", Chevron );
addMaterialMapping( "chevron_cool2", Chevron );
addMaterialMapping( "chevron_warm", Chevron );
addMaterialMapping( "chevron_warm2", Chevron );
addMaterialMapping( "chevron_neutral", Chevron );
addMaterialMapping( "chevron_neutral2", Chevron );

addMaterialMapping( "wall_cool1", Wall );
addMaterialMapping( "wall_cool2", Wall );
addMaterialMapping( "wall_warm1", Wall );
addMaterialMapping( "wall_warm2", Wall );
addMaterialMapping( "wall_neutral1", Wall );
addMaterialMapping( "wall_neutral2", Wall );

addMaterialMapping( "grid_warm" ,    GridSwirl);
addMaterialMapping( "grid_warm1" ,    Grid4Box);
addMaterialMapping( "grid_warm2" ,    Grid9Box);
addMaterialMapping( "grid_warm3" ,    Grid9Box);
addMaterialMapping( "grid_warm4" ,    Grid9Box);

addMaterialMapping( "grid_neutral" , Grid9Box);
addMaterialMapping( "grid_neutral1" , Grid9Box);
addMaterialMapping( "grid_neutral2" , GridTris);
addMaterialMapping( "grid_neutral3" , Grid9Box);
addMaterialMapping( "grid_neutral4" , GridTris);

addMaterialMapping( "grid_cool" ,    GridSwirl);
addMaterialMapping( "grid_cool2" ,    GridSwirl);
addMaterialMapping( "grid_cool1" ,   GridSwirl);
addMaterialMapping( "grid_cool3" ,   Grid9Box);
addMaterialMapping( "grid_cool4" ,   GridSwirl);


addMaterialMapping( "stripe_cool" ,    DefaultMaterial);
addMaterialMapping( "stripe_neutral" , DefaultMaterial);
addMaterialMapping( "stripe_warm" ,    DefaultMaterial);
addMaterialMapping( "tube_cool" ,      DefaultMaterial);
addMaterialMapping( "tube_neutral" ,   DefaultMaterial);
addMaterialMapping( "tube_warm" ,      DefaultMaterial);

addMaterialMapping( "solid_cool1" ,      DefaultMaterial);
addMaterialMapping( "solid_cool2" ,      DefaultMaterial);
addMaterialMapping( "solid_neutral1" ,   DefaultMaterial);
addMaterialMapping( "solid_neutral2" ,   DefaultMaterial);
addMaterialMapping( "solid_warm1" ,      DefaultMaterial);
addMaterialMapping( "solid_warm2" ,      DefaultMaterial);

addMaterialMapping( "pattern_cool1" ,      PatternCool1);
addMaterialMapping( "pattern_cool2" ,      PatternCool2);
addMaterialMapping( "pattern_neutral1" ,   DefaultMaterial);
addMaterialMapping( "pattern_neutral2" ,   DefaultMaterial);
addMaterialMapping( "pattern_warm1" ,      DefaultMaterial);
addMaterialMapping( "pattern_warm2" ,      DefaultMaterial);
addMaterialMapping( "pattern_warm2" ,      PatternWarm24);

addMaterialMapping( "friction_none" ,    NoFrictionMaterial);
addMaterialMapping( "friction_low" ,     LowFrictionMaterial);
addMaterialMapping( "friction_low2" ,     LowFrictionMaterial);
addMaterialMapping( "friction_high" ,    HighFrictionMaterial);
// used for ramps in escher level
addMaterialMapping( "friction_ramp_yellow" ,    VeryHighFrictionMaterial);

// old textures (to be removed?)
addMaterialMapping( "grid1" , RubberFloorMaterial);
addMaterialMapping( "grid2" , RubberFloorMaterial);
addMaterialMapping( "grid3" , RubberFloorMaterial);
addMaterialMapping( "grid4" , RubberFloorMaterial);

// some part textures
addMaterialMapping( "base.slick" , IceMaterial);
addMaterialMapping( "ice.slick" , IceMaterial);
addMaterialMapping( "bumper-rubber" ,    BumperMaterial);
addMaterialMapping( "pball-round-side" , BumperMaterial);
addMaterialMapping( "pball-round-top" , BumperMaterial);
addMaterialMapping( "pball-round-bottm" , BumperMaterial);
addMaterialMapping( "button" , ButtonMaterial);

