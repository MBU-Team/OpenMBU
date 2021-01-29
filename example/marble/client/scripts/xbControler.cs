//-----------------------------------------------------------------------------
// These scripts handle the locking/unlocking and insert/removal handlers for
// the XBox controlers to fid the certification requirements
//-----------------------------------------------------------------------------

function onXBLockedControllerRemoved()
{
   checkLockedController();
}

function checkLockedController()
{
   if ($Input::LockedController == -1)
      return;
   
   if (!XBLiveIsControllerPresent($Input::LockedController))
   {
      if (PlayGui.isAwake() && !GamePauseGui.isAwake())
      {
         Canvas.pushDialog(GamePauseGui);
         //ControlerUnplugGui.controlerSomething();
         pauseGame();
      }
   }
   else
   {
      if( !$GamePauseGuiActive )
         resumeGame();
   }
}

//-----------------------------------------------------------------------------

function onControlerInsert( %controlerId )
{
   if( $LockedControlerRemoved && %controlerId == $LockedControlerId  )
   {
      echo( "Locked controler replaced! Port " @ %controlerId );
      $ControlerRemovedDuringLoad = false;
   }
}

//------------------------------------------------------------------------------

function XBCheckAllControlersRemoved()
{
//   %allGone = true;
//   for( %i = 0; %i < 4; %i++ )
//   {
//      if( XBLiveIsControllerPresent( %i ) )
//      {
//         %allGone = false;
//         break;
//      }
//   }
//   
//   if( %allGone )
//   {
//      ControlerUnplugGui.controlerSomething( true );
//   }
}
