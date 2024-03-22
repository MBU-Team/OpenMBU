// maps a texture name to the specified material object.  You should not need to call this function,
// unless you want to override a mapping for a texture (normally the mapping is set up by the baseTex
// property of the Material or the mapTo field of the material).
function mapMaterial(%name,%material)
{
   addMaterialMapping(%name, "material: " @ %material);
}

function loadMaterials()
{
   // load base materials
   exec("./baseMaterials.cs");
   
   for( %file = findFirstFile( "*/materials.cs" ); %file !$= ""; %file = findNextFile( "*/materials.cs" ))
   {
      exec( %file );
   }

   // load custom material files
   loadMaterialJson("./interiorMaterials.mat.json");
   loadMaterialJson("./shapeMaterials.mat.json");
   loadMaterialJson("./sizeMaterials.mat.json");
   //exec("./interiorMaterials.cs");
   //exec("./shapeMaterials.cs");
   //exec("./sizeMaterials.cs"); //Blue Sizing Materials
   exec("./marbleMat.cs");
}

function reloadMaterials()
{
   reloadTextures();
   loadMaterials();
   reInitMaterials();
}

// load all material.cs files
loadMaterials();   
