
// Stack used to track what gui thread we are viewing
new SimSet(menu_thread_stack);

// Define various topic threads

new SimSet(main_menu_thread);
   main_menu_thread.add(MainMenuDlg);

new SimSet(overview_thread);
   overview_thread.add(overview_main);

new SimSet(features_thread);
   features_thread.add(features_main);
   features_thread.add(features_script);
   features_thread.add(features_gui);
   features_thread.add(features_net);
   features_thread.add(features_render);
   features_thread.add(features_shader);
   features_thread.add(features_terrain);
   features_thread.add(features_interior);
   features_thread.add(features_mesh);
   features_thread.add(features_water);
   features_thread.add(features_sound);
  
new SimSet(tools_thread);
   tools_thread.add(tools_main);
   tools_thread.add(tools_gui);
   tools_thread.add(tools_world);
   tools_thread.add(tools_heightfield);
   tools_thread.add(tools_texture);
   tools_thread.add(tools_terrain);

new SimSet(products_thread);
   products_thread.add(product_main);
   products_thread.add(product_tribes2);
   products_thread.add(product_hunting);
   products_thread.add(product_marbleblast);
   products_thread.add(product_thinktanks);
   products_thread.add(product_tenniscritters);
   products_thread.add(product_orbz);

new SimSet(testimonials_thread);
   testimonials_thread.add(community);
   testimonials_thread.add(testimonials_main);

new SimSet(license_thread);
   license_thread.add(publishing);
   license_thread.add(license_main);
   license_thread.add(license_indie);
   license_thread.add(license_corp);

new SimSet(play_game_thread);
   play_game_thread.add(StartMissionGui);
   play_game_thread.add(JoinServerGui);

new SimSet(garagegames_thread);
   garagegames_thread.add(garageGames_main);


package AutoLoad_GuiMLTextCtrl_Contents
{
   function GuiMLTextCtrl::onWake(%this)
   {
      if (%this.filename !$= "")
      {
         %fo = new FileObject();
         %fo.openForRead(%this.filename);
         %text = "";
         while(!%fo.isEOF())
            %text = %text @ %fo.readLine() @ "\n";

         %fo.delete();
         %this.setText(%text);
      }
      //parent::onWake(%this);
   }

   function gotoWebPage(%url)
   {
      if(isFullScreen())
         toggleFullScreen();
      Parent::gotoWebPage(%url);
   }

   function GuiMLTextCtrl::onURL(%this, %url)
   {
      if (getSubStr(%url, 0, 9) $= "gamelink ")
         eval( getSubStr(%url, 9, 1024) );
      else
         gotoWebPage( %url );
   }   

};

activatePackage(AutoLoad_GuiMLTextCtrl_Contents);

