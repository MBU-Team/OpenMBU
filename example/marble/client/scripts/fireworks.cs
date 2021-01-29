//-----------------------------------------------------------------------------
// FireWorks, client side control.

//-----------------------------------------------------------------------------

function clientCmdLaunchWave(%wave, %position, %rotation, %explosion1, %explosion2)
{
   if (!isObject(ServerConnection))
      // this happens when we exit server mid-wave
      return;

   if (!isObject(FireWorks) && isObject(ServerConnection))
   {
      new SimGroup(FireWorks);
      ServerConnection.add(FireWorks);
   }
   
   // Create the explosions
   for (%i = 0; %i < 2; %i++) 
   {
      %exp = %i==0 ? %explosion1 : %explosion2;
      %obj = new Explosion() 
      {
         datablock = %exp;
         position = %position;
         rotation = %rotation;
      };
      FireWorks.add(%obj);
   }
   
   // Schedule next wave
   if (%wave < 3) 
   {
      %delay = 500 + 1000 * getRandom();
      $Game::FireWorkSchedule = schedule(%delay,0,"clientCmdLaunchWave",%wave + 1,%position, %rotation,%explosion1,%explosion2);
   }
}
