//*****************************************************************************
// Cottage and Tower materials
//*****************************************************************************
function demoMaterial( %name, %dir )
{
   %com = "new Material(Demo_"@%name@") {"@
      "baseTex[0] = " @ %name @ ";" @
      "mapTo = " @ %name @ ";" @
   "};";
//   echo(%com);
   eval(%com);
}

demoMaterial("MtlPilrFront");
demoMaterial("MtlPilrBottom");
demoMaterial("Mtl" );
demoMaterial("F" );
demoMaterial("Wall_filler1_01" );
demoMaterial("Floor_tile01" );
demoMaterial("FlrFiller3_01" );
demoMaterial("WalMtlbase1_01" );
demoMaterial("Filler_tile1_01" );
demoMaterial("oak1" );
demoMaterial("oak2" );
demoMaterial("oak2_end" );
demoMaterial("thatch" );
demoMaterial("Floor_slate01" );
demoMaterial("WalNoGroove" );
demoMaterial("Wall_block01" );
demoMaterial("stonewall" );
demoMaterial("Wall_panel01" );
demoMaterial("MtlPilrTop" );
