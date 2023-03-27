function NMH_Type::onWake(%this)
{
   // deactivateKeyboard is supposed to do this but it doesn't work
   %this.reactivateMM = false;
   if (getCurrentActionMap() == moveMap.getId())
   {
      moveMap.pop();
      %this.reactivateMM = true;
   }
}

function NMH_Type::onSleep(%this)
{
   if (PlayGui.isAwake() && %this.reactivateMM)
   {
      moveMap.push();
   }
}