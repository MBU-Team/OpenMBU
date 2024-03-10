//newChatHud.cs

if($Pref::Chat::FontSize < 14)
	$Pref::Chat::FontSize = 14;

//server connection stuff, copied from chathud.cs
function onChatMessage(%message, %voice, %pitch)
{
	if(strLen(%message) > 0)
		newChatHud_AddLine(%message);
}

function onServerMessage(%message)
{
	if(strLen(%message) > 0)
		newChatHud_AddLine(%message);
}

function newChatHud_Init()
{
   echo("***********************");
   echo(" chat hud init");
   echo("***********************");
	//create script object with a buffer array 
	//size dictated by $prefs, but check min/max and provide defaults

	//defaults
	if($Pref::Chat::CacheLines $= "")
	{
		$Pref::Chat::CacheLines = 1000;
		$Pref::Chat::MaxDisplayLines = 10;
		$Pref::Chat::LineTime = 6000;
	}
	
	//min and max values
	if($Pref::Chat::CacheLines < 100)
		$Pref::Chat::CacheLines = 100;
	if($Pref::Chat::CacheLines > 50000)
		$Pref::Chat::CacheLines = 50000;
	
	if($Pref::Chat::MaxDisplayLines < 4)
		$Pref::Chat::MaxDisplayLines = 4;
	if($Pref::Chat::MaxDisplayLines > 100)
		$Pref::Chat::MaxDisplayLines = 100;

	//if($Pref::Chat::LineTime < 1000)
	//	$Pref::Chat::LineTime = 1000;
	if($Pref::Chat::LineTime > 30000)
		$Pref::Chat::LineTime = 30000;

	$NewChatSO = new ScriptObject()
	{
		class = NewChatSO;
		size = $Pref::Chat::CacheLines;	//total number of lines to store in history
		maxLines = $Pref::Chat::MaxDisplayLines;	//maximum lines to display
		lineTime = $Pref::Chat::LineTime; //max period of time a line of chat is shown (can be pushed off though)
		head = 0;
		tail = 0;
		textObj = newChatText.getID();	//the gui object that we display our text in
		pageUpEnd = -1;
	};
   RootGroup.add($NewChatSO);

	if($Pref::Chat::ShowAllLines == 1 && $Pref::Chat::LineTime > 0)
		$NewChatSO.pageUpEnd = 0;

	if($Pref::Gui::ChatSize $= "")
		$Pref::Gui::ChatSize = 1;
	
	OPT_SetChatSize($Pref::Gui::ChatSize);
}

function newChatHud_UpdateScrollDownIndicator()
{
	if($NewChatSO.pageUpEnd == -1 || $NewChatSO.pageUpEnd == $NewChatSO.head) 
		chatScrollDownIndicator.setVisible(false);
	else
		chatScrollDownIndicator.setVisible(true);

	newChatHud_UpdateIndicatorPosition();
}
function newChatHud_UpdateIndicatorPosition()
{
	%w = getWord(chatScrollDownIndicator.getExtent(), 0);
	%h = getWord(chatScrollDownIndicator.getExtent(), 1);
	%x = getWord(chatScrollDownIndicator.getPosition(), 0);
	%y = getWord($NewChatSO.textObj.getPosition(), 1) + getWord($NewChatSO.textObj.getExtent(),1) - %h;	
	chatScrollDownIndicator.resize(%x,%y,%w,%h);
}

function newChatHud_AddLine(%line)
{
	echo("CHAT: " @ stripChars(%line, "\c0\c1\c2\c3\c4\c5\c6\c7\c8\c9\cp\co"));

	if(!isObject($NewChatSO))
		newChatHud_Init();
	
	//echo(%line);
	//clientside filtering of chat
		//if we've got a url start and no url end, add a url end
		if(strStr(%line, "<a:") != -1)
			%line = %line @ "</a>";
		
		//filter out cursewords


	$NewChatSO.addLine(%line);
   
   if(newChatText.isAwake())
   	newChatText.forceReflow();
	newMessageHud.updatePosition();
	newChatHud_UpdateScrollDownIndicator();
}


//copies the appropriate lines into the text box
function NewChatSO::displayLatest(%this)
{
	//cancel any pending checks
	if(isEventPending(%this.displaySchedule))
		cancel(%this.displaySchedule);
		
	%text = %this.textObj;
	if(isObject(%text))
	{
		%buff = "";
		//%buff = "<7/14/2006\""@ %this.textObj.profile.fontType @"\" size:"@ $Pref::Chat::FontSize @">";
		%currTime = getSimTime();
		for(%i = 0; %i < %this.maxLines; %i++)
		{
			//look backwards from the head, stop when we find a line that is too old or hit the maxlines
			%pos = (%this.head - 1 - %i);
			if(%pos == -1)
				%pos = %this.size + %pos;
         
			//if((%currTime - %this.time[%pos]) > %this.lineTime || %i == (%this.maxLines-1) || ((%pos+1) % %this.size == %this.tail))
         if((%currTime - %this.time[%pos]) > $Pref::Chat::LineTime || %i == (%this.maxLines-1) || ((%pos+1) % %this.size == %this.tail))
			{
				if(%pos != (%this.head - 1))
					%this.displaySchedule = %this.schedule(500, displayLatest); 					//schedule the next check so our chat fades out nicely
				
				%showMouseTip = false;
				//echo("found old line - ", %this.line[%pos], " index = ", %pos);
				//we've reached an old line, display the ones we've passed over so far
				for(%i--; %i >= 0; %i--)
				{
					%pos = (%this.head - 1 - %i);
					if(%pos < 0)
						%pos = %this.size + %pos;

					%buff = %buff @ %this.line[%pos] @ "\n";

					//check for urls
					if(%showMouseTip == false)
						if(strstr(%this.line[%pos], "<a:") != -1)
							%showMouseTip = true;
				}
				break;
			}	
			else
			{
				//echo("found new line - ", %this.line[%pos], " index = ", %pos);
			}
		}
		%text.setValue(%buff);
		newChatTextShadow.settext(%buff);
	
		//kind of a hack to move the message box to the right place
      if(%text.isAwake())
   		%text.forceReflow();
		newMessageHud.updatePosition();
		newChatHud_UpdateScrollDownIndicator();
	}
	else
	{
		error("ERROR: NewChatSO::displayLatest() - %this.textObj not defined");
	}

	//show the mouse tool tip
//	if(%showMouseTip && $pref::HUD::showToolTips)
//	{
//		MouseToolTip.setVisible(true);
//		%key = strUpr(getWord(movemap.getBinding("toggleCursor"), 1));
//		MouseToolTip.setValue("\c6TIP: Press "@ %key @" to toggle mouse and click on links");
//	}
//	else
//	{	
//		if(!Canvas.isCursorOn())
//			MouseToolTip.setVisible(false);
//	}
//	if(MouseToolTip.isVisible())
//	{  
//      if($NewChatSO.textObj.isAwake())
//   		$NewChatSO.textObj.forceReflow();
//
//		%w = getWord(MouseToolTip.getExtent(), 0);
//		%h = getWord(MouseToolTip.getExtent(), 1);
//		%x = getWord(MouseToolTip.getPosition(), 0);
//		%y = getWord($NewChatSO.textObj.getPosition(), 1) + getWord($NewChatSO.textObj.getExtent(),1) + %h;	
//		MouseToolTip.resize(%x,%y,%w,%h);
//	}
}

function NewChatSO::AddLine(%this, %line)
{
	%this.line[%this.head] = %line;
	%this.time[%this.head] = getSimTime();

	%doPage = false;
	if(%this.pageUpEnd == %this.head)
		%doPage = true;

	%this.head += 1;

	//loop around when we get to the end of the buffer
	if(%this.head >= %this.size)
	{
		%this.head = 0; //%this.size;
	}
	
	//push the tail forward as we wrap around
	if(%this.head == %this.tail)
		%this.tail++;
	if(%this.tail >= %this.size)
		%this.tail = 0;

	

	//if we're not scrolling through old chat, display latest
	if(%this.pageUpEnd == -1)
		%this.displayLatest();
	else
	{
		//if we're scrolled all the way up to the top, the last line just got overwritten so move forward and update
		if((%this.pageUpEnd - %this.maxLines + 1) % %this.size == %this.tail)
		{
			%this.pageUpEnd = (%this.pageUpEnd + 1) % %this.size;
			%this.displayPage();
		}
	}
	
	if(%doPage)
	{
		%this.pageUpEnd = %this.head;
		%this.displayPage();
	}
}

function NewChatSO::PageUp(%this)
{
	if(isEventPending(%this.displaySchedule))
		cancel(%this.displaySchedule);

	if(%this.pageUpEnd == -1)
	{
		%this.pageUpEnd = %this.head; //initial value
		$Pref::Chat::ShowAllLines = 1;

		//echo("pageup starting at head ", %this.head);
	}
	else
	{
		if(%this.tail == 0)
		{
			if(%this.pageUpEnd <= (%this.tail + (%this.maxLines * 2)))
			{
				if(%this.head <= %this.tail + %this.maxLines)
				{
					//dont do anything
				}
				else
				{
					//we're within max lines of the end, we cant go any further back
					%this.pageUpEnd = %this.tail + %this.maxLines;
				}
			}
			else
			{
				%this.pageUpEnd -= %this.maxLines;
			}
		}
		else
		{
			//echo("chatSO: pageupend = ", %this.pageUpEnd);
			//the tail is 1 in front of the head

			for(%i = 0; %i <= (%this.maxLines * 2); %i++)
			{
				%pos = %this.pageUpEnd - %i;
				if(%pos < 0)
					%pos += %this.size;

				if(%pos == %this.tail)
				{
					//echo("breaking at tail ", %pos);
					break;
				}
			}
			%this.pageUpEnd = %pos + %this.maxLines;
			if(%this.pageUpEnd > %this.size)
				%this.pageUpEnd -= %this.size;
		
		}
	}
	newChatHud_UpdateScrollDownIndicator();

	%this.displayPage();
	
   if(%this.textObj.isAwake())
   	%this.textObj.forceReflow();
	newMessageHud.updatePosition();
}

function NewChatSO::PageDown(%this)
{
	//if we're on the head, go back to normal viewing
	if(%this.pageUpEnd == %this.head || %this.pageUpEnd == -1)
	{
		%this.pageUpEnd = -1;
		%this.displayLatest();
		$Pref::Chat::ShowAllLines = 0;
	}
	else
	{
		if(isEventPending(%this.displaySchedule))
			cancel(%this.displaySchedule);


		for(%i = 0; %i <= %this.maxLines; %i++)
		{
			%pos = %this.pageUpEnd + %i;
			if(%pos >= %this.size)
				%pos -= %this.size;
			
			if(%pos == %this.head)
				break;
		}

		%this.pageUpEnd = %pos;

		//if we're within maxlines of the head, jump to head
		//if(%this.head - %this.pageUpEnd <= %this.maxlines)
		//{
		//	%this.pageUpEnd = %this.head;
		//}
		//else	
		//{
		//	%this.pageUpEnd += %this.maxlines;
		//	if(%this.pageUpEnd >= %this.size)
		//		%this.pageUpEnd -= %this.size;
		//}
		%this.displayPage();
	}
	newChatHud_UpdateScrollDownIndicator();
}

function NewChatSO::DisplayPage(%this)
{
	%text = %this.textObj;
	
	if(!isObject(%text))
	{
		error("ERROR: NewChatSO::DisplayPage() - textObj is not defined for object " @ %this.getName() @ " ("@ %this @")");
		return;
	}
	
	%start = %this.pageUpEnd - %this.maxLines;

	if(%start < 0)
		%start += %this.size;
	//if(%start < %this.tail)
	//	%start = %this.tail;

	//echo("display page start = ", %start);

	%showMouseTip = false;

	%buff = "";
	//%buff = "<font:\""@ %this.textObj.profile.fontType @"\" size:"@ $Pref::Chat::FontSize @">";
	for(%i = 0; %i < %this.maxLines; %i++)
	{
		%pos = (%start + %i) % %this.size;
		if(%this.line[%pos] !$= "")
			%buff = %buff @ %this.line[%pos] @ "\n"; 

		if(%showMouseTip == false)
			if(strstr(%this.line[%pos], "<a:") != -1)
				%showMouseTip = true;
	}
	%text.setValue(%buff);
	
	//show the mouse tool tip
//	if(%showMouseTip && $pref::HUD::showToolTips && $pref::chat::lineTime > 0)
//	{
//      if(%text.isAwake())
//   		%text.forcereflow();
//		MouseToolTip.setVisible(true);
//		%key = strUpr(getWord(movemap.getBinding("toggleCursor"), 1));
//		MouseToolTip.setValue("\c6TIP: Press "@ %key @" to toggle mouse and click on links");
//	}
//	else
//	{	
//		if(!Canvas.isCursorOn())
//			MouseToolTip.setVisible(false);
//	}

//   if($pref::chat::lineTime <= 0)
//      MouseToolTip.setVisible(false);
      
//	if(MouseToolTip.isVisible())
//	{
//      if($NewChatSO.textObj.isAwake())
//   		$NewChatSO.textObj.forceReflow();
//
//		%w = getWord(MouseToolTip.getExtent(), 0);
//		%h = getWord(MouseToolTip.getExtent(), 1);
//		%x = getWord(MouseToolTip.getPosition(), 0);
//		%y = getWord($NewChatSO.textObj.getPosition(), 1) + getWord($NewChatSO.textObj.getExtent(),1) + %h;	
//		MouseToolTip.resize(%x,%y,%w,%h);
//	}
}


function NewChatSO::Update(%this)
{
	if(%this.pageUpEnd == -1)
		%this.displayLatest();
	else
		%this.displayPage();
}

function NewChatSO::dumpLines(%this)
{
	echo("head = ", %this.head);
	echo("tail = ", %this.tail);
	for(%i = 0; %i < %this.size; %i++)
	{
		if(%this.head == %i)
			echo("line " @ %i @ " : " @ %this.line[%i] @ "\c0<-HEAD");
		else if(%this.tail == %i)
			echo("line " @ %i @ " : " @ %this.line[%i] @ "\c0<-TAIL");
		else
			echo("line " @ %i @ " : " @ %this.line[%i] );
	}
}
