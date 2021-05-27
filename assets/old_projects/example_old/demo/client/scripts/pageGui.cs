//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PageGui is the main TSControl through which the game is viewed.
// The PageGui also contains the hud controls.
//-----------------------------------------------------------------------------

function PageGui::onWake(%this)
{
   %this.pageNumber = 0;
}

function PageGui::onSleep(%this)
{
}

function PageGui::pageName(%this, %index)
{
   if (%index $= "")
   {
      %index = %this.pageNumber;
     echo ("PAGE: " @ %this.pageNumber);
   }
   return %this.thread @ "_page" @ %index;
}

function PageGui::isNextPage(%this)
{
   for (%num = %this.pageNumber + 1; !isObject(%this.pageName(%num)); %num += 1)
      if (%num > 100)
         return false;
   return true;
}

function PageGui::isPrevPage(%this)
{
   for (%num = %this.pageNumber - 1; !isObject(%this.pageName(%num)); %num -= 1)
      if (%num < 0)
         return false;
   return true;
}

function PageGui::pushThread(%this, %thread)
{
   if (%this.thread !$= "" )
      %this.threadStack = %this.thread @ " " @ %this.pageNUmber @ " " @ %this.threadStack;
   %this.popPage();
   %this.thread = %thread;
   %this.pushPage(1);
   %this.status();
}

function PageGui::popThread(%this)
{
   if (%this.threadStack !$= "" )
   {
      %this.popPage();
      %this.thread = firstWord(%this.threadStack);
      %this.threadStack = removeWord(%this.threadStack, 0);

      %this.pageNumber = firstWord(%this.threadStack);
      %this.threadStack = removeWord(%this.threadStack, 0);
   }
   else
      %this.thread = "";
   %this.status();
}

function PageGui::status(%this)
{
   echo ("----------------------");
   echo ("THREADSTACK: " @ %this.threadStack);
   echo ("THREAD: " @ %this.thread);
   echo ("PAGE: " @ %this.pageNumber);
}

function PageGui::popPage(%this)
{
   echo ("POP: " @ %this.pageName());
   if (%this.pageNumber > 0)
      Canvas.popDialog(%this.pageName());
}

function PageGui::pushPage(%this, %pageNumber)
{
   // Load up the new page   
   %this.pageNumber = %pageNumber;
   %page = %this.pageName();
   Canvas.pushDialog(%page);
  
   // Update the arrows to reflect next/prev page availabilty
   PageGuiNextPage.setVisible(%this.isNextPage());
   //PageGuiPrevPage.setVisible(%this.isPrevPage());
   
   // Extract demo text from page object
   PageGuiTitle.setText(
      "<font:Arial Bold:32><color:ffffff><just:right>" @
      %page.title);
   %this.status();
}

function PageGui::setPageNumber(%this,%pageNumber)
{
   // Pop off any current page...
   %this.popPage();

   // Load up the new page   
   %this.pushPage(%pageNumber);
}

function PageGui::nextPage(%this)
{
   if (%this.isNextPage())
      %this.setPageNumber(%this.pageNumber + 1);
}

function PageGui::prevPage(%this)
{
   if (%this.isPrevPage())
      %this.setPageNumber(%this.pageNumber - 1);
   else
   {
      %this.popThread();
      if (%this.thread !$= "")
         %this.pushPage(%this.pageNumber);
      else
         Canvas.setContent(mainMenuGui);
   }
}


