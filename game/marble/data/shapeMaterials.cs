new CubemapData( sky_environment )
{
   cubeFace[0] = "marble/data/skies/env_SO";
   cubeFace[1] = "marble/data/skies/env_NO";
   cubeFace[2] = "marble/data/skies/env_EA";
   cubeFace[3] = "marble/data/skies/env_WE";
   cubeFace[4] = "marble/data/skies/env_UP";
   cubeFace[5] = "marble/data/skies/env_DN";
};

new CubemapData( marbleCubemap3 )
{
   cubeFace[0] = "marble/data/skies/marbleCubemap3_SO";
   cubeFace[1] = "marble/data/skies/marbleCubemap3_NO";
   cubeFace[2] = "marble/data/skies/marbleCubemap3_EA";
   cubeFace[3] = "marble/data/skies/marbleCubemap3_WE";
   cubeFace[4] = "marble/data/skies/marbleCubemap3_UP";
   cubeFace[5] = "marble/data/skies/marbleCubemap3_DN";
};

new CubemapData( gemCubemap )
{
   cubeFace[0] = "marble/data/skies/gemCubemapUp";
   cubeFace[1] = "marble/data/skies/gemCubemapUp";
   cubeFace[2] = "marble/data/skies/gemCubemapUp";
   cubeFace[3] = "marble/data/skies/gemCubemapUp";
   cubeFace[4] = "marble/data/skies/gemCubemapUp";
   cubeFace[5] = "marble/data/skies/gemCubemapUp";
};

new CubemapData( gemCubemap2 )
{
   cubeFace[0] = "marble/data/skies/gemCubemapUp2";
   cubeFace[1] = "marble/data/skies/gemCubemapUp2";
   cubeFace[2] = "marble/data/skies/gemCubemapUp2";
   cubeFace[3] = "marble/data/skies/gemCubemapUp2";
   cubeFace[4] = "marble/data/skies/gemCubemapUp2";
   cubeFace[5] = "marble/data/skies/gemCubemapUp2";
};

new CubemapData( gemCubemap3 )
{
   cubeFace[0] = "marble/data/skies/gemCubemapUp3";
   cubeFace[1] = "marble/data/skies/gemCubemapUp3";
   cubeFace[2] = "marble/data/skies/gemCubemapUp3";
   cubeFace[3] = "marble/data/skies/gemCubemapUp3";
   cubeFace[4] = "marble/data/skies/gemCubemapUp3";
   cubeFace[5] = "marble/data/skies/gemCubemapUp3";
};


//-----------------------------------------------------------------------------
// ShaderData
//-----------------------------------------------------------------------------
new ShaderData( RefractPix )
{
   DXVertexShaderFile 	= "shaders/refractV.hlsl";
   DXPixelShaderFile 	= "shaders/refractP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( StdTex )
{
   DXVertexShaderFile 	= "shaders/standardTexV.hlsl";
   DXPixelShaderFile 	= "shaders/standardTexP.hlsl";
   pixVersion = 2.0;
};

new CustomMaterial(Material_Marble_BB)
{
   mapTo = "marble.BB.skin";

   texture[0] = "~/data/shapes/balls/marble.BB.bump";
   texture[1] = "$backbuff";
   texture[2] = "~/data/shapes/balls/marble.BB.skin";

   specular[0] = "1 1 1 1.0";
   specularPower[0] = 12.0;

   version = 2.0;
   refract = true;
   shader = RefractPix;

   pass[0] = Mat_Glass_NoRefract;
   
};


%mat = new Material(Material_cap)
{
   baseTex[0] = "marble/data/shapes/balls/cap";
   bumpTex[0] = "marble/data/shapes/balls/cap_normal";
   cubemap = Lobby;
   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.8 1.0";
   specularPower[0] = 12.0;
};

//pball_round.dts
%mat = new Material(Material_Bumper)
{
   mapTo = bumper;
   
   friction = 0.5;
   restitution = 0;
   force = 15;

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.8 1.0";
   specularPower[0] = 12.0;
   
   baseTex[0] = "~/data/shapes/bumpers/bumper";
};

//ductfan.dts
%mat = new Material(Material_HazardFan)
{
   mapTo = fan;
   
   baseTex[0] = "~/data/shapes/hazards/fan";
   //bumpTex[0] = "~/data/shapes/signs/arrowsign_post_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 12.0;
};

//trapdoor.dts
%mat = new Material(Material_Trapdoor)
{
   mapTo = "trapdoor";

   baseTex[0] = "~/data/shapes/hazards/trapdoor";
};



//copter

%mat = new Material(Material_Helicopter)
{
   mapTo = copter_skin;

   // stage 0
   baseTex[0] = "~/data/shapes/images/copter_skin";
   //bumpTex[0] = "~/data/shapes/images/copter_bump";
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
};



//blast.dts

%mat = new Material(Material_blastOrbit)
{
   mapTo = blast_orbit_skin;
   
   baseTex[0] = "~/data/shapes/images/blast_orbit_skin";
   bumpTex[0] = "~/data/shapes/images/blast_orbit_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 32.0;
};


%mat = new Material(Material_item_glow)
{
   mapTo = item_glow;

   // stage 0
   baseTex[0] = "~/data/shapes/items/item_glow";
   
   glow[0] = true;
   emissive[0] = true;
   
	specular[0] = "1 1 1 1";
	specularPower[0] = 8.0;
};


%mat = new Material(Material_blast_glow)
{
   mapTo = blast_glow;

   // stage 0
   baseTex[0] = "~/data/shapes/images/blast_glow";
   

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 32.0;
   glow[0]=true;
   
};


//grow.dts
%mat = new Material(Material_grow)
{
   mapTo = grow;
   
   baseTex[0] = "~/data/shapes/images/grow";
   bumpTex[0] = "~/data/shapes/images/grow_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 32.0;
};

%mat = new Material(Material_Grow_Glow)
{
   mapTo = grow_glow;

   // stage 0
   baseTex[0] = "~/data/shapes/images/grow_glow";
   emissive[0] = true;
   glow[0]=true;
   
   //pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 8.0;//32.0;
};


//antiGravity.dts

%mat = new Material(Material_AntiGravSkin)
{
   mapTo = antigrav_skin;
   
   baseTex[0] = "~/data/shapes/items/antigrav_skin";
   bumpTex[0] = "~/data/shapes/items/antigrav_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 32.0;
};

%mat = new Material(Material_AntiGravGlow)
{
   mapTo = antigrav_glow;

   // stage 0
   baseTex[0] = "~/data/shapes/items/antigrav_glow";
   glow[0]=true;
   emissive[0] = true;
};


//egg.dts
%mat = new Material(Material_EasterEgg)
{
   mapTo = "egg_skin";

   baseTex[0] = "~/data/shapes/items/egg_skin";
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;

   cubemap = gemCubemap;
};

//gem.dts
%mat = new Material(Material_BaseGem)
{
   mapTo = "base.gem";
   baseTex[0] = "~/data/shapes/items/red.gem";
   cubemap = gemCubemap;
};

%mat = new Material(Material_RedGem)
{
   baseTex[0] = "~/data/shapes/items/red.gem";
   cubemap = gemCubemap;
};

%mat = new Material(Material_BlueGem)
{
   baseTex[0] = "~/data/shapes/items/blue.gem";
   cubemap = gemCubemap3;
};

%mat = new Material(Material_YellowGem)
{
   baseTex[0] = "~/data/shapes/items/yellow.gem";
   cubemap = gemCubemap2;
};


%mat = new Material(Material_GemShine)
{
   baseTex[0] = "~/data/shapes/items/gemshine";
   translucentBlendOp = add;
   translucent = true;
};

//superJump.dts

%mat = new Material(Material_SuperJump)
{
   mapTo = superJump_skin;

   // stage 0
   baseTex[0] = "~/data/shapes/items/superJump_skin";
   bumpTex[0] = "~/data/shapes/items/superJump_bump";
   
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
};


//itemArrow used in several powerups
%mat = new Material(Material_ItemArrow)
{
   baseTex[0] = "~/data/shapes/items/itemArrow";
};

//superSpeed.dts

%mat = new Material(Material_SuperSpeed)
{
   mapTo = superSpeed_skin;

   // stage 0
   baseTex[0] = "~/data/shapes/items/superSpeed_skin";
   //bumpTex[0] = "~/data/shapes/items/superSpeed_bump";
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
};


%mat = new Material(Material_SuperSpeedStar)
{
   mapTo = superSpeed_star;

   baseTex[0] = "~/data/shapes/items/superSpeed_star";
   emissive[0] = true;
   glow[0] = true;
   

};


//timetravel.dts
%mat = new Material(Material_TimeTravelSkin)
{
   mapto = timeTravel_skin;	
   
   baseTex[0] = "~/data/shapes/items/timeTravel_skin";
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
};


//endarea.dts

%mat = new Material(Material_endpad_glow)
{
   mapTo = endpad_glow;
   
   baseTex[0] = "~/data/shapes/pads/endpad_glow";
   glow[0] = true;
   emissive[0] = true;
   translucent[0] = true;
   //cubemap = sky_environment;
};

%mat = new Material(Material_checkpad)
{
   mapTo = checkpad;
   baseTex[0] = "~/data/shapes/pads/checkpad";
   pixelSpecular[0] = true;
   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 32.0;
};

//startarea.dts

%mat = new Material(Material_ringglass)
{
   mapTo = ringglass;

   baseTex[0] = "~/data/shapes/pads/ringglass";
   bumpTex[0] = "~/data/shapes/pads/ringnormal";
   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.8 1.0";
   specularPower[0] = 12.0;
   emissive[0] = true;
   translucent[0] = true;
   cubemap = sky_environment;
   doubleSided = true;
};


%mat = new Material(Material_ringtex)
{
   mapTo = ringtex;

   //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
   baseTex[0] = "~/data/shapes/pads/ringtex";
   pixelSpecular[0] = true;
   specular[0] = "0.3 0.3 0.3 0.7";
   specularPower[0] = 14.0;
   
 };
 
//Unused atm - Tim
// %mat = new Material(Material_center)
//{
//   mapTo = center;
 //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
//   baseTex[0] = "~/data/shapes/pads/center";
//   pixelSpecular[0] = true;
//   specular[0] = "0.3 0.3 0.3 1.0";
//   specularPower[0] = 12.0;
//   emissive[0] = true;
// };

%mat = new Material(Material_abyss)
{
   mapTo = abyss;

   //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
   baseTex[0] = "~/data/shapes/pads/abyss";
   emissive[0] = true;
   //glow[0] = true;
   animFlags[0] = $rotate;
   rotPivotOffset[0] = "-0.5 -0.5";
   rotSpeed[0] = 1.0;
   

 };
 
 %mat = new Material(Material_abyss2)
{
   mapTo = abyss2;

   baseTex[0] = "~/data/shapes/pads/abyss2";
  // emissive[0] = true;
   glow[0] = true;
   animFlags[0] = $rotate;
   rotPivotOffset[0] = "-0.5 -0.5";
   rotSpeed[0] = 1.0;
   
 };
 
 

%mat = new Material(Material_misty)
{
   mapTo = misty;

   //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
   baseTex[0] = "~/data/shapes/pads/misty";

   translucent[0] = true;
   translucentBlendOp = LerpAlpha;
   
   animFlags[0] = $scroll;
   scrollDir[0] = "0.0 1.0";
   scrollSpeed[0] = 0.5;
   emissive[0] = true;
   glow[0] = true;


 };
 
 %mat = new Material(Material_mistyglow_ff)
{
	mapTo = "mistyglow";
	
	baseTex[0] = "marble/data/shapes/pads/mistyglow";
	
	translucent = true;
	specular[0] = "1 1 1 1";
	specularPower[0] = 8.0;
	translucent = true;
	
	translucentBlendOp = AddAlpha;
	
	texCompression[0] = DXT5;
};
 
 %mat = new Material(Material_mistyglow)
{
   mapTo = mistyglow;

   baseTex[0] = "~/data/shapes/pads/mistyglow";
	
	fallback = "Material_mistyglow_ff";
	
   glow[0] = true;
   emissive[0] = true;
   translucent[0] = true;
   //cubemap = sky_environment;
};

%mat = new Material(Material_corona)
{
   mapTo = corona;

   //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
   baseTex[0] = "~/data/shapes/images/corona";
   glow[0] = true;
   emissive[0] = true;
   translucent[0] = true;
   translucentBlendOp = AddAlpha;
   
	specular[0] = "1 1 1 1";
	specularPower[0] = 8.0;

   animFlags[0] = $rotate;
   rotPivotOffset[0] = "-0.5 -0.5";
   rotSpeed[0] = 3.0;
 };

//cautionsign.dts
%mat = new Material(Material_BaseCautionSign)
{
   baseTex[0] = "~/data/shapes/signs/base.cautionsign";
};

%mat = new Material(Material_CautionCautionSign)
{
   baseTex[0] = "~/data/shapes/signs/caution.cautionsign";
};

%mat = new Material(Material_DangerCautionSign)
{
   baseTex[0] = "~/data/shapes/signs/danger.cautionsign";
};

%mat = new Material(Material_CautionSignWood)
{
   baseTex[0] = "~/data/shapes/signs/cautionsignwood";
};

%mat = new Material(Material_CautionSignPole)
{
   baseTex[0] = "~/data/shapes/signs/cautionsign_pole";
};

//plainsign.dts
%mat = new Material(Material_PlainSignWood)
{
   baseTex[0] = "~/data/shapes/signs/plainsignwood";
};

%mat = new Material(Material_BasePlainSign)
{
   baseTex[0] = "~/data/shapes/signs/base.plainSign";
};

%mat = new Material(Material_DownPlainSign)
{
   baseTex[0] = "~/data/shapes/signs/down.plainSign";
};

%mat = new Material(Material_LeftPlainSign)
{
   baseTex[0] = "~/data/shapes/signs/left.plainSign";
};

%mat = new Material(Material_RightPlainSign)
{
   baseTex[0] = "~/data/shapes/signs/right.plainSign";
};

%mat = new Material(Material_UpPlainSign)
{
   baseTex[0] = "~/data/shapes/signs/up.plainSign";
};

%mat = new Material(Material_PlainSignWood2)
{
   baseTex[0] = "~/data/shapes/signs/signwood2";
};

%mat = new Material(Material_SignWood)
{
   baseTex[0] = "~/data/shapes/signs/signwood";
};

%mat = new Material(Material_astrolabe)
{
   mapTo = "astrolabe_glow";

   baseTex[0] = "~/data/shapes/astrolabe/astrolabe_glow";

   translucent[0] = true;

   emissive[0] = true;
   renderBin = "SkyShape";
};

%mat = new Material(Material_astrolabe_solid)
{
   mapTo = "astrolabe_solid_glow";

   baseTex[0] = "~/data/shapes/astrolabe/astrolabe_solid_glow";

   translucent[0] = true;

   emissive[0] = true;
   renderBin = "SkyShape";
};

%mat = new Material(Material_clouds_beginner)
{
   mapTo = "clouds_beginner";

   baseTex[0] = "~/data/shapes/astrolabe/clouds_beginner";

   translucent[0] = true;

   emissive[0] = true;
   renderBin = "SkyShape";
};

%mat = new Material(Material_clouds_intermediate)
{
   mapTo = "clouds_intermediate";

   baseTex[0] = "~/data/shapes/astrolabe/clouds_intermediate";

   translucent[0] = true;

   emissive[0] = true;
   renderBin = "SkyShape";
};

%mat = new Material(Material_clouds_advanced)
{
   mapTo = "clouds_advanced";

   baseTex[0] = "~/data/shapes/astrolabe/clouds_advanced";

   translucent[0] = true;

   emissive[0] = true;
   renderBin = "SkyShape";
};

new CustomMaterial(Mat_Glass_NoRefract)
{
   
   texture[0] = "~/data/shapes/structures/glass2";
    baseTex[0] = "~/data/shapes/structures/glass";
   
   friction = 1;
   restitution = 1;
   force = 0;

   version = 2.0;
   translucent = true;
   //translucentZWrite = false;
   blendOp = LerpAlpha;
   shader = StdTex;
};

%mat = new Material(Material_glass_fallback)
{
	mapTo = "glass2";
	
	baseTex[0] = "marble/data/shapes/structures/glass2";
	
	emissive[0] = true;
	specular[0] = "1 1 1 1";
	specularPower[0] = 8.0;
	translucent = true;
	
	texCompression[0] = DXT5;
};

new CustomMaterial(Material_glass)
{
   mapTo = "glass";

   texture[0] = "~/data/shapes/structures/glass.normal";
   texture[1] = "$backbuff";
   texture[2] = "~/data/shapes/structures/glass";
   
	fallback = "Material_glass_fallback";
   
   friction = 1;
   restitution = 1;
   force = 0;

   specular[0] = "1.0 1.0 1.0 1.0";
   specularPower[0] = 12.0;

   version = 2.0;
   refract = true;
   shader = RefractPix;

   pass[0] = Mat_Glass_NoRefract;
   renderBin = "TranslucentPreGlow";
};



//%mat = new Material(Material_glass)
//{
//eventually we want to use refraction here
//   mapTo = "glass.png";
   
//   baseTex[0] = "~/data/shapes/structures/glass";
//   translucent[0] = true;
   //translucentZwrite = true;
   //bumpTex[0] = "~/data/shapes/structures/glass.normal";

//   pixelSpecular[0] = true;
//   specular[0] = "1 1 1 1.0";
//   specularPower[0] = 10.0;
//};

%mat = new Material(Material_GemBeam)
{
   baseTex[0] = "marble/data/shapes/items/gembeam";
   translucent[0] = true;
   //translucentZwrite = true;
   emissive[0] = true;

   pixelSpecular[0] = true;
   specular[0] = "0.5 0.6 0.5 0.6";
   specularPower[0] = 12.0;
};

%mat = new Material(Material_ArrowSignArrow)
{
   mapTo = arrowsign_arrow;
	
   baseTex[0] = "marble/data/shapes/signs/arrowsign_arrow";
   bumpTex[0] = "marble/data/shapes/items/arrow_bump";
   //emissive[0] = true;
   //glow[0]=true;

   pixelSpecular[0] = true;
   specular[0] = "1 1 1 1";
   specularPower[0] = 32.0;
};

%mat = new Material(Material_ArrowSignArrowGlow)
{
   mapTo = arrowsign_arrow_glow;
	
   baseTex[0] = "marble/data/shapes/signs/arrowsign_arrow";

      pixelSpecular[0] = true;
   specular[0] = ".3 .3 .3 .3";
   specularPower[0] = 32.0;

   glow[0]=true;

};

%mat = new Material(Material_ArrowSignPost)
{
   mapTo = arrowpostUVW;
   
   baseTex[0] = "~/data/shapes/signs/arrowpostUVW";
   //bumpTex[0] = "~/data/shapes/signs/arrowsign_post_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 32.0;
};

%mat = new Material(Material_ArrowSignChain)
{
   mapTo = arrowsign_chain;
   
   baseTex[0] = "~/data/shapes/signs/arrowsign_chain";
   
   emissive[0] = true;
   glow[0] = true;
   translucent[0] = true;
   //translucentblendop[0] = add;
   
};

%mat = new Material(Material_ArrowSignPost)
{
   mapTo = arrowsign_post;
   
   baseTex[0] = "~/data/shapes/signs/arrowsign_post";
   bumpTex[0] = "~/data/shapes/signs/arrowsign_post_bump";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.8 0.6 1.0";
   specularPower[0] = 12.0;
};

%mat = new Material(TimeTravelGlass_Fallback)
{
	mapTo = "timeTravel_glass_fallback";
	
	baseTex[0] = "marble/data/shapes/structures/glass";
	
	emissive[0] = true;
	specular[0] = "1 1 1 1";
	specularPower[0] = 8.0;
	translucent = true;
	
	texCompression[0] = DXT5;
};

new CustomMaterial(Material_TimeTravelGlass)
{
   mapto = timeTravel_glass;

   texture[0] = "~/data/shapes/structures/time.normal";
   texture[1] = "$backbuff";
   texture[2] = "~/data/shapes/structures/glass";

   friction = 1;
   restitution = 1;
   force = 0;

   specular[0] = "1 1 1 1.0";
   specularPower[0] = 10.0;
   
	fallback = "TimeTravelGlass_Fallback";

   version = 2.0;
   refract = true;
   shader = RefractPix;

   // TODO: This shouldn't be needed but, without this line it renders all black.
   // translucent = true;
};

new CustomMaterial(Material_distort_d)
{
   mapto = distort_d;

   texture[0] = "~/data/shapes/Particles/distort_n";
   texture[1] = "$backbuff";
   texture[2] = "~/data/shapes/Particles/distort_d";
   //specular[0] = "1 1 1 1.0";
   //specularPower[0] = 10.0;
   
   specular[0] = "1 1 1 1.0";
   specularPower[0] = 10.0;

   version = 2.0;
   refract = true;
   shader = RefractPix;
};



new CustomMaterial(Material_cube_glass)
{
   mapTo = "cube_glass";

   texture[0] = "~/data/shapes/structures/cube_glass.normal";
   texture[1] = "$backbuff";
   texture[2] = "~/data/shapes/structures/cube_glass";

   friction = 0.8;
   restitution = 0.1;
   force = 0;

   specular[0] = "1 1 1 1.0";
   specularPower[0] = 10.0;

   version = 2.0;
   refract = true;
   shader = RefractPix;

   pass[0] = Mat_Glass_NoRefract;
};

//new CustomMaterial(Material_refract)
//{
   //mapto = refract;
//
   //texture[0] = "~/data/shapes/structures/time.normal";
   //texture[1] = "$backbuff";
   //texture[2] = "~/data/shapes/pads/refract";
//
   //friction = 1;
   //restitution = 1;
   //force = 0;
//
   //specular[0] = "1 1 1 1.0";
   //specularPower[0] = 10.0;
//
   //version = 2.0;
   //refract = true;
   //shader = RefractPix;
//};

%mat = new Material(Material_refract)
{
	mapTo = "refract";
	
	baseTex[0] = "marble/data/shapes/images/blast_glow";
	
	glow[0] = true;
	pixelSpecular[0] = true;
	specular[0] = "0.8 0.8 0.6 1";
	specularPower[0] = 32.0;
	
	
	texCompression[0] = DXT3;
};

%mat = new Material(Material_blastwave)
{
   mapTo = blastwave;
   //bumpTex[0] = "~/data/shapes/pads/pad_base2.normal";
   baseTex[0] = "~/data/shapes/images/blastwave";
   glow[0] = true;
   emissive[0] = true;
   translucent[0] = true;
   translucentBlendOp = AddAlpha;

 };
 
 %mat = new Material(Material_sigil)
{
   mapTo = sigil;

   // stage 0
   baseTex[0] = "~/data/shapes/pads/sigil";
   //emissive[0] = true;
   glow[0]=true;
   emissive[0] = true;
   translucent[0] = true;
   translucentBlendOp = AddAlpha;
};

 %mat = new Material(Material_sigil_glow)
{
   mapTo = sigil_glow;

   // stage 0
   baseTex[0] = "~/data/shapes/pads/sigil_glow";
   //emissive[0] = true;
   glow[0]=true;
   emissive[0] = true;
   translucent[0] = true;
   translucentBlendOp = Add;
   animFlags[0] = $scroll;
   scrollDir[0] = "1.0 0.0";
   scrollSpeed[0] = 0.3;
};

 %mat = new Material(Material_sigiloff)
{
   mapTo = sigiloff;

   // stage 0
   baseTex[0] = "~/data/shapes/pads/sigiloff";
   //emissive[0] = true;
//   glow[0]=true;
//   emissive[0] = true;
   translucent[0] = true;
//   translucentBlendOp = Add;
};

 %mat = new Material(lightning1frame1)
{
   mapTo = lightning1frame1;
   baseTex[0] = "~/data/shapes/bumpers/lightning1frame1";

	emmisive[0] = true;
	glow[0] = true;
	animFlags[0] = $sequence;
	sequenceFramePerSec[0] = 4.0;
	sequenceSegmentSize[0] = 0.25;
    translucent[0] = true;
};

%mat = new Material (Material_landmine_grs)
{
	mapTo = landmine_grs;
	baseTex = "~/data/shapes/hazards/landmine_grs";
};

%mat = new Material (Material_landmine_spikes)
{
	mapTo = landmine_spikes;
	baseTex = "~/data/shapes/hazards/landmine_spikes";
};
