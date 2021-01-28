//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// MainMenuGui is the main Control through which the game shell is viewed.
//-----------------------------------------------------------------------------

function MainMenuGui::onWake(%this)
{
   %this.pushThread(main_menu_thread);
   OverlayBuyNow.setVisible(true);
}

//-------------------------------------
function MainMenuGui::getThread(%this)
{
   %top = menu_thread_stack.getCount();
   if (%top > 0)
      return menu_thread_stack.getObject(%top-1);
   else
      return 0;
}


//-------------------------------------
function MainMenuGui::popCurrent(%this)
{
   if (%thread = %this.getThread())
   {
      Canvas.popDialog(%thread.getObject(%thread.page));
      echo ("POP Current: " @ %thread.getName() @ " page: " @ %thread.page);
   }
}


//-------------------------------------
function MainMenuGui::pushThread(%this, %thread)
{
   echo ("PushThread: " @ %thread.getname());

   %this.popCurrent();
   %thread.page = 0;
   menu_thread_stack.add(%thread);
   %this.update();
}


//-------------------------------------
function MainMenuGui::popThread(%this)
{
   if (%thread = %this.getThread())
   {
      echo ("PopThread: " @ %thread.getname());

      %this.popCurrent();
      menu_thread_stack.remove(%thread);
      %this.update();
   }
}


//-------------------------------------
function MainMenuGui::isNextPage(%this)
{
   if ( !(%thread = %this.getThread()) )
      return false;
   echo ("isNext: " @ %thread.page @ " - " @ %thread.getCount());
   return %thread.page < (%thread.getCount() - 1);
}


//-------------------------------------
function MainMenuGui::isPrevPage(%this)
{
   if ( !(%thread = %this.getThread()) )
      return false;
   return %thread.page > 0;
}


//-------------------------------------
function MainMenuGui::isPrevThread(%this)
{
   return menu_thread_stack.getCount() > 1;
}


//-------------------------------------
function MainMenuGui::nextPage(%this)
{
   if (%this.isNextPage())
   {
      %this.popCurrent();
      %thread = %this.getThread();
      %thread.page = (%thread.page+1);
      %this.update();
   }
   else
      %this.popThread();
}


//-------------------------------------
function MainMenuGui::prevPage(%this)
{
   if (%this.isPrevPage())
   {
      %this.popCurrent();
      %thread = %this.getThread();
      %thread.page = (%thread.page-1);
      %this.update();
   }
}


//-------------------------------------
function MainMenuGui::update(%this)
{
   if (Canvas.getContent().getName() $= "GuiEditorGui")
   {
	   %this.popCurrent();
	   Canvas.popdialog(OverlayDlg);
	   return;
   }
	   
   if (%thread = %this.getThread())
   {
      echo ("Update: " @ %thread.getName() @ " page: " @ %thread.page);
      %page = %thread.getObject(%thread.page);
      
      // Setup default overlay buttons
      OverlayTitle.setText("<just:center><shadowcolor:000000><shadow:1:1>" @ %page.title);
      OverlayDesc.setText("<font:Arial Bold:16><color:ffffff><just:left>" @
         "Thank you for considering the Torque Shader Engine\nEarly Adopter (EA1) Demo\n--GarageGames");

//      OverlayNextPage.setVisible(%this.isNextPage());
      OverlayNextPage.setVisible(%this.isNextPage() || (%this.isPrevPage() && %this.isPrevThread()));
      OverlayNextPage.command = "MainMenuGui.nextPage();";

      OverlayPrevPage.setVisible(%this.isPrevPage());
      OverlayPrevPage.command = "MainMenuGui.prevPage();";

      OverlayQuitPage.setVisible(false);
      OverlayQuitPage.command = "quit();";

      if (!%this.isPrevPage())
      {
         if (%this.isPrevThread())
         {
            OverlayPrevPage.setVisible(true);
            OverlayPrevPage.command = "MainMenuGui.popThread();";
         }
         else
            OverlayQuitPage.setVisible(true);
      }
      
      // Push the current page
      Canvas.pushDialog(%page);

      // make sure The Overlay is on top
      Canvas.popdialog(OverlayDlg);
      Canvas.pushdialog(OverlayDlg);
   }
}


//-------------------------------------
function MainMenuGui::onSleep(%this)
{
}

//-------------------------------------
function MainMenuGui::goHome(%this)
{
   disconnect();
   if (Canvas.getContent() != %this.getId())
      Canvas.pushDialog(%this);
   else {
      while (%this.isPrevThread())
         %this.popThread();
   }
}



