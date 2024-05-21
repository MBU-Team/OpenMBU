datablock ItemData(EasterEggItem)
{
   // Mission editor category
   category = "Easter";
   className = "EasterEgg";

   // Basic Item properties
   shapeFile = "~/data/shapes/items/egg.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupName = $Text::AnEasterEgg;
   maxInventory = 1;
   noRespawn = true;
   gemType = 1;
   noPickupMessage = false;

   renderGemAura = false;
   noRenderTranslucents = true;
};

function EasterEggItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"glow");
}

function EasterEgg::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this,%obj,%user,%amount);
   commandToClient(%user.client,'onEasterEggPickup',%obj.easterEggIndex);
   return true;
}

function EasterEgg::saveState(%this,%obj,%state)
{
   %state.object[%obj.getId()] = %obj.isHidden();
}

function EasterEgg::restoreState(%this,%obj,%state)
{
   %obj.setHidden(%state.object[%obj.getId()]);
}

function hasFoundEgg( %index )
{
   %bit = BIT( %index );
   return ( $UserAchievements::easterEggs & %bit );
}

function clientCmdOnEasterEggPickup( %index )
{
   if( isDemoLaunch() )
   {
      serverPlay2d(easterNotNewSfx);
      addHelpLine( $Text::DemoEgg, false );
      return;
   }
   
   if ($testLevel || GameMissionInfo.getCurrentMission().difficultySet $= "custom" || GameMissionInfo.getMode() $= GameMissionInfo.CustomMode)
   {
      serverPlay2d(easterNewSfx);
      addHelpLine( $Text::FoundNewEgg, false );
      return;
   }
   
   autosplitterSetEggFound(true);
   sendAutosplitterData("egg" SPC %index);
   
   if( hasFoundEgg( %index ) )
   {
      serverPlay2d(easterNotNewSfx);
      addHelpLine( $Text::AlreadyFoundEgg, false );
   }
   else
   {
      // Dean Scream...we remember you!
      
      // achievement function
      gotEasterEgg(%index);
      
      serverPlay2d(easterNewSfx);
      addHelpLine( $Text::FoundNewEgg, false );
   }
}
