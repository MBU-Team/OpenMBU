// LG_loadingtext2

function loadingGui::updateText(%this)
{
   %status = %this.status;
   %task = %this.task;
   %progress = %this.progress;   
   
   //%text = "";
      //
   //if (%this.status $= "")
      //%text = %text @ $Text::Loading;
   //else
      //%text = %text @ %this.status;
      //
   //if (%this.task !$= "")
      //%text = %text @ " - " @ %this.task;
      //
   //if (%this.progress !$= "")
      //%text = %text @ " - " @ %this.progress @ "%";
   //
   //%this.setText(%text);
   
   LG_loadingtextStatus.setVisible(%status !$= "");
   LG_loadingtextTask.setVisible(%task !$= "");
   LG_loadingtextProgress.setVisible(%progress !$= "");
   
   LG_loadingtextStatus.setText(%status);
   LG_loadingtextTask.setText(%task);
   LG_loadingtextProgress.setText(mCeil(%progress) @ "%");
}

function loaderStatusCallback(%status)
{
   if (loadingGui.status $= %status)
      return;
   
   loadingGui.status = %status;
   
   //loadingGui.task = "";
   //loadingGui.progress = "";   
   
   loadingGui.updateText();
}

function loaderTaskCallback(%task)
{
   if (loadingGui.task $= %task)
      return;
   
   loadingGui.task = %task;
   loadingGui.updateText();
}

function loaderProgressCallback(%progress)
{
   if (loadingGui.progress $= %progress)
      return;
   
   loadingGui.progress = %progress;
   loadingGui.updateText();
}

function loaderCallback(%status, %task, %progress)
{
   loadingGui.status = %status;
   loadingGui.task = %task;
   loadingGui.progress = %progress;
   loadingGui.updateText();
}
