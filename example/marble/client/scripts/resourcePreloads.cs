//$preload_skip["game/client/ui/GameSplash.jpg"] = 1;
//$preload_skip["game/client/ui/Missions/bgSpooky.jpg"] = 1;
//$preload_skip["game/client/ui/Missions/bgFrantic.jpg"] = 1;
//$preload_skip["game/client/ui/Missions/bgLush.jpg"] = 1;
//$preload_skip["game/client/ui/Missions/bgDefault.jpg"] = 1;

// immediately preload all textures specified by %spec
function preloadTextures(%spec)
{
   echo("Preloading Textures:" SPC %spec);
   %absStart = getRealTime();
   for (%file = findFirstFile(%spec); %file !$= ""; %file = findNextFile(%spec))
   {
      if ($preload_skip[%file])
         continue;
      %start = getRealTime();
      preloadTexture(%file);
      echo(%file SPC getRealTime() - %start);
   }
   echo("Preload elapsed:" SPC getRealTime() - %absStart);
}

$preload_idx = 0;
$preload_delay = 40;
$preload_list_len = 0;

// add the specified textures to the preload list for incremental preloading
// (gives main thread time to do other things)
function addToPreloadList(%spec)
{
   for (%file = findFirstFile(%spec); %file !$= ""; %file = findNextFile(%spec))
   {
      if ($preload_skip[%file])
         continue;
      $preload_list[$preload_list_len] = %file;
      $preload_list_len++;
   }
}

function preloadNext()
{
   // preload the next file then reschedule
   if ($preload_idx < $preload_list_len)
   {
      %start = getRealTime();
      preloadTexture($preload_list[$preload_idx]);
      echo($preload_list[$preload_idx] SPC getRealTime() - %start);
   }
   $preload_idx++;
   
   if ($preload_idx < $preload_list_len)
      schedule($preload_delay, 0, "preloadNext");
}

function clientPreloadTextures()
{
   // gui stuff
   for( %i = 0; %i < 10; %i++ )
   {
      preloadTexture( "~/client/ui/game/numbers/" @ %i @ ".png" );
   }
   preloadTexture( "~/client/ui/game/numbers/colon.png" );
   preloadTexture( "~/client/ui/game/numbers/dash.png" );
   preloadTexture( "~/client/ui/game/numbers/point.png" );
   preloadTexture( "~/client/ui/game/numbers/slash.png" );
   preloadTexture( "~/client/ui/game/fade_black.png" );
   
   preloadTextures("~/client/ui/game/*.png");
   preloadTextures("~/data/skies/*.jpg");
   preloadTexture("marble/client/ui/xbox/roundedBG.png");
   //addToPreloadList("game/client/ui/xbox/controls*.png");
   
   // start first incremental preload
   preloadNext();
}


// Preload some resources
//preloadResource( "~/data/shapes/balls/ball-superball.dts" );
//preloadTexture( "~/data/particles/star.png" );
//preloadTexture( "~/data/particles/smoke.png" );
//preloadTexture( "~/data/particles/twirl.png" );
//preloadTexture( "~/data/particles/spark.png");
