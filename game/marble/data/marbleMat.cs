//-----------------------------------------------------------------------------
// Shader defs
//-----------------------------------------------------------------------------
new ShaderData( BumpMarb )
{
   DXVertexShaderFile 	= "shaders/marbles/marbBumpCubeDiffuseV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbBumpCubeDiffuseP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarb )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicRefRefMarb )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicGlassV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicGlassP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( DiffMarb )
{
   DXVertexShaderFile 	= "shaders/marbles/marbDiffCubeV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbDiffCubeP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarb2 )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassic2V.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassic2P.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarb2PureSphere )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassic2PureSphereV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassic2PureSphereP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarbGlass )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicGlassPerturbV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicGlassPerturbP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarbGlassPureSphere )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicGlassPerturbPureSphereV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicGlassPerturbPureSphereP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarbMetal )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicMetalPerturbV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicMetalPerturbP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarbGlass2 )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassicGlassPerturb2V.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassicGlassPerturb2P.hlsl";
   pixVersion = 2.0;
};

new ShaderData( CrystalMarb )
{
   DXVertexShaderFile 	= "shaders/marbles/marbCrystalV.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbCrystalP.hlsl";
   pixVersion = 2.0;
};

new ShaderData( ClassicMarb3 )
{
   DXVertexShaderFile 	= "shaders/marbles/marbClassic3V.hlsl";
   DXPixelShaderFile 	= "shaders/marbles/marbClassic3P.hlsl";
   pixVersion = 2.0;
};

//-----------------------------------------------------------------------------
// Material defs
//-----------------------------------------------------------------------------
%mat = new Material(Material_Base_Marble)
{
   baseTex[0] = "marble/data/shapes/balls/base.marble";

   pixelSpecular[0] = true;
   specular[0] = "0.5 0.6 0.5 0.6";
   specularPower[0] = 12.0;
   //cubemap = sky_environment;
   renderBin = "Marble";
};

//function defineBasicMarbleMaterial(%id, %shader, %bump, %diff, %mapTo)
//{
//   %tmp = "new CustomMaterial(Material_Marble" @ %id @ ") " @
//        "{\n" @
//        "    mapTo = \"" @ %mapTo @ "\";\n" @
//        "    texture[0] = \"marble/data/shapes/balls/" @ %bump @ "\";\n" @
//        "    texture[1] = \"marble/data/shapes/balls/" @ %diff @ "\";\n" @
//        "    texture[3] = \"$dynamicCubemap\";\n" @
//        "    dynamicCubemap = true;\n" @ 
//        "    shader = \"" @ %shader @ "\";\n" @
//        "    version = 2.0;\n" @ 
//        "};\n";
        
//   echo(%tmp); // <-- I'm good for debugging.
//   eval(%tmp);
//}

function defineBasicMarbleMaterial(%id, %shader, %bump, %diff, %mapTo, %specular, %specPow)
{
   %tmp = "new CustomMaterial(Material_Marble" @ %id @ ") " @
        "{\n" @
        "    mapTo = \"" @ %mapTo @ "\";\n" @
        "    texture[0] = \"marble/data/shapes/balls/" @ %bump @ "\";\n" @
        "    texture[1] = \"marble/data/shapes/balls/" @ %diff @ "\";\n" @
        "    texture[3] = \"$dynamicCubemap\";\n" @
        "    baseTex[0] = \"marble/data/shapes/balls/" @ %diff @ "\";\n" @ 
        "    specular[0] = \"" @ %specular @ "\";\n" @
        "    specularPower[0] = " @ %specPow @ ";\n" @
        "    dynamicCubemap = true;\n" @ 
        "    shader = \"" @ %shader @ "\";\n" @
        "    version = 2.0;\n" @ 
        "    renderBin = \"Marble\";\n" @
        "};\n";
        
//   echo(%tmp); // <-- I'm good for debugging.
   eval(%tmp);
}


// Shaders with PureSphere after them do their own normal calculations and won't have seams.

defineBasicMarbleMaterial("01", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble01.skin", "marble01.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("02", "CrystalMarb", "marble02.normal",   "marble02.skin", "marble02.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("03", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble03.skin", "marble03.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("04", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble04.skin", "marble04.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("05", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble05.skin", "marble05.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("06", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble06.skin", "marble06.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("07", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble07.skin", "marble07.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("08", "ClassicMarb", "marblefx.normal", "marble08.skin", "marble08.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("09", "ClassicMarb3", "marble.normal",  "marble09.skin", "marble09.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("10", "ClassicMarb2", "marble.normal",   "marble10.skin", "marble10.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("11", "ClassicMarbMetal", "marble18.normal",   "marble11.skin", "marble11.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("12", "ClassicMarbGlassPureSphere", "marble01.normal",  "marble12.skin", "marble12.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("13", "ClassicMarb3", "marble.normal",   "marble13.skin", "marble13.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("14", "ClassicMarb3", "marble.normal",   "marble14.skin", "marble14.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("15", "ClassicMarbGlassPureSphere", "marble01.normal",  "marble15.skin", "marble15.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("16", "ClassicMarb3", "marble.normal",   "marble16.skin", "marble16.skin", "0.2 0.2 0.2 0.2", 6 );
defineBasicMarbleMaterial("17", "ClassicMarb3", "marble.normal",   "marble17.skin", "marble17.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("18", "ClassicMarbGlass", "marble18.normal",   "marble18.skin", "marble18.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("19", "ClassicMarb3", "marble.normal",   "marble19.skin", "marble19.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("20", "ClassicMarbGlass", "marble20.normal",   "marble20.skin", "marble20.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("21", "ClassicMarb3", "marble.normal",   "marble21.skin", "marble21.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("22", "ClassicMarb3", "marble.normal",   "marble22.skin", "marble22.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("23", "ClassicMarb3", "marble.normal",   "marble23.skin", "marble23.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("24", "ClassicMarb3", "marble.normal",   "marble24.skin", "marble24.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("25", "ClassicMarb3", "marble.normal",   "marble25.skin", "marble25.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("26", "CrystalMarb", "marble02.normal",   "marble26.skin", "marble26.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("27", "CrystalMarb", "marble02.normal",   "marble27.skin", "marble27.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("28", "CrystalMarb", "marble02.normal",   "marble28.skin", "marble28.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("29", "CrystalMarb", "marble02.normal",   "marble29.skin", "marble29.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("30", "CrystalMarb", "marble02.normal",   "marble30.skin", "marble30.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("31", "ClassicMarb3", "marble.normal",   "marble31.skin", "marble31.skin", "0.3 0.3 0.3 0.3", 24 );
defineBasicMarbleMaterial("32", "ClassicMarb3", "marble.normal",   "marble32.skin", "marble32.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("33", "ClassicMarbMetal", "marble18.normal",   "marble33.skin", "marble33.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("34", "ClassicMarb2", "marble.normal",   "marble34.skin", "marble34.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("35", "ClassicMarb3", "marble.normal",   "marble35.skin", "marble35.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("36", "ClassicMarb3", "marble36.normal",   "marble36.skin", "marble36.skin", "0.6 0.6 0.6 0.6", 12 );
defineBasicMarbleMaterial("37", "ClassicMarbGlassPureSphere", "marble01.normal",   "marble37.skin", "marble37.skin", "0.6 0.6 0.6 0.6", 12 );

//-----------------------------------------------------------------------------------------------------------------------
// CUSTOM MARBLES
//-----------------------------------------------------------------------------------------------------------------------

new CustomMaterial(ReflectMarble01)
{
//   mapTo = "marble01.skin";

   texture[0] = "~/data/shapes/balls/plainNormal";
   texture[1] = "~/data/shapes/balls/marble01.skin";
   texture[3] = "$dynamicCubemap";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.9 0.8 0.8";
   specularPower[0] = 12;

   dynamicCubemap = true;
   
   shader = BumpMarb; 
   version = 2.0;
   renderBin = "Marble";
};

new CustomMaterial(ReflectMarble02)
{
//   mapTo = "marble01.skin";

   texture[0] = "~/data/shapes/balls/plainNormal";
   texture[1] = "~/data/shapes/balls/marble01.skin";
   texture[3] = "$dynamicCubemap";

   pixelSpecular[0] = true;
   specular[0] = "0.8 0.9 0.8 0.8";
   specularPower[0] = 12;

   dynamicCubemap = true;
   
   shader = BumpMarb; 
   version = 2.0;
   renderBin = "Marble";
};

new CustomMaterial(ReflectMarble03)
{
//   mapTo = "marble01.skin";

   texture[0] = "~/data/shapes/balls/marble01.skin";
   texture[1] = "$dynamicCubemap";

   dynamicCubemap = true;
   
   shader = DiffMarb; 
   version = 2.0;
   renderBin = "Marble";
};
