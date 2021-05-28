function runPresentation()
{
   presentsGui.done = false;
   presentsGui.skip = false;
   Canvas.setContent( PresentsGui );
   schedule(100, 0, checkPresentsDone );
}
function checkPresentsDone()
{
   if (presentsGui.skip)
      runTitle();
   else if (presentsGui.done)
      runProduction();
   else
      schedule(100, 0, checkPresentsDone );
}
function runTitle()
{
   Canvas.setContent(MainMenuGui);
}
function runProduction()
{
   productionGui.done = false;
   productionGui.skip = false;
   Canvas.setContent( productionGui );
   schedule(100, 0, checkProductionDone );
}
//-------------------------------------
function checkProductionDone()
{
   if (productionGui.done || productionGui.skip)
      runTitle();
   else
      schedule(100, 0, checkProductionDone );
}