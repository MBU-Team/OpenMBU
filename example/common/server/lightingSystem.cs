//-----------------------------------------------
// Copyright © Synapse Gaming 2004
// Written by John Kabus
//-----------------------------------------------

$sgLightEditor::lightDBPath = "common/lighting/lights/";
$sgLightEditor::filterDBPath = "common/lighting/filters/";

function sgLoadDataBlocks(%basepath)
{
   //load precompiled...
   %path = %basepath @ "*.dso";
   echo("");
   echo("//-----------------------------------------------");
   //echo("Synapse Gaming Lighting Pack");
   echo("");
   echo("Loading light datablocks from: " @ %path);
   %file = findFirstFile(%path);
  
  while(%file !$= "")
  {
     %file = filePath(%file) @ "/" @ fileBase(%file);
     exec(%file);
      %file = findNextFile(%path);
  }

   //load uncompiled...
   %path = %basepath @ "*.cs";
   echo("");
   echo("Loading light datablocks from: " @ %path);
   %file = findFirstFile(%path);

   while(%file !$= "")
   {
      exec(%file);
      %file = findNextFile(%path);
   }
   
   echo("Loading complete.");
   echo("//-----------------------------------------------");
   echo("");
}

sgLoadDataBlocks($sgLightEditor::lightDBPath);
sgLoadDataBlocks($sgLightEditor::filterDBPath);

function serverCmdsgGetLightDBId(%client, %db)
{
   %id = nameToId(%db);
   commandToClient(%client, 'sgGetLightDBIdCallback', %id);
}



