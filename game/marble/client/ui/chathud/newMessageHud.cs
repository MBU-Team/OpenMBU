//newMessageHud.cs
//the thing we type in when chatting

function newMessageHud::onWake(%this)
{

}
function newMessageHud::onSleep(%this)
{
	commandtoserver('stopTalking');

	activateKeyboard();
	NMH_Type.makeFirstResponder(false);
}


function newMessageHud::open(%this, %channel)
{
   if (!$Client::connectedMultiplayer)   
      return;
   
	%this.channel = %channel;
	
	switch$(%channel)
	{
		case "SAY":
			NMH_Channel.setText("\c3" @ $Text::Chat @ ":");		
		case "TEAM":
			NMH_Channel.setText("\c6" @ %channel @ ":");		
		default:
			error("ERROR(): newMessageHud::open() - unknown channel \""@ %channel @"\"");
			return;
	}
	
	if(!%this.isAwake())
	{
		NMH_Type.setValue("");
		canvas.pushDialog(%this);		
		%this.updatePosition();

      deactivateKeyboard();
      NMH_Type.makeFirstResponder(true);

		//schedule a re-position of the type-in box
		%this.schedule(10, updateTypePosition);
	}
   
}

//moves the type-in box right next to the channel display
function newMessageHud::updateTypePosition(%this)
{
	%pixelWidth = 33; //NMH_Channel.getPixelWidth();
	if(%pixelWidth < 5) //must not have had enough time to draw the text
	{
		%this.schedule(100, updateTypePosition);
		return;
	}

	%x =  getWord(NMH_Channel.getPosition(),0);
	%x += %pixelWidth + 2;
	%y = 0;
	%w = getWord(NMH_Box.getExtent(),0) - %x - 2;
	%h = getWord(NMH_Type.getExtent(), 1);

	NMH_Type.resize(%x,%y,%w,%h);
}

function newMessageHud::updatePosition(%this)
{
	if(!%this.isAwake())
		return;

	//move to bottom of the chat box
	%x = getWord(NMH_Box.getPosition(), 0);
	%y = getWord(newChatText.getPosition(), 1);
	%y += getWord(newChatText.getExtent(), 1);
	%y -= 10;
	%w = getWord(NMH_Box.getExtent(), 0);
	%h = getWord(NMH_Type.getExtent(), 1);// getWord(NMH_Box.getExtent(), 1);

	NMH_Box.resize(%x,%y,%w,%h);
}


//called every time we type a character
function NMH_Type::Type(%this)
{
	//if this is the first character
	//and it is a "/", we're typing a command so dont tell people that we're typing
	//otherwise tell the server that we've started typing something
	%text = %this.getValue();
	if(strLen(%text) == 1)
		if(%text !$= "/")
			commandToServer('StartTalking');	
}


//called when we press enter
function NMH_Type::Send(%this)
{
	%text = %this.getText();

   if(strLen(trim(%text)) <= 0)
   {
      canvas.popDialog(newMessageHud);
      return;
   }

	%firstChar = getSubStr(%text, 0, 1);
	if(%firstChar $= "/")
	{
		%newText = getSubStr(%text, 1, 256);
		%command = getWord(%newText, 0);
		%par1	= getWord(%newText, 1);
		%par2	= getWord(%newText, 2);
		%par3	= getWord(%newText, 3);
		%par4	= getWord(%newText, 4);
		%par5	= getWord(%newText, 4);
		%par6	= getWord(%newText, 4);
		commandToServer(addTaggedString(%command), %par1, %par2, %par3, %par4, %par5, %par6);
	}
	else
	{	
		switch$(newMessageHud.channel)
		{
			case"SAY":
				commandToServer('messageSent', %text);
			case"TEAM":
				commandToServer('teamMessageSent', %text);
			default:
				error("ERROR: NMH_Type::Send() - unknown channel \"" @ newMessageHuf.channel @ "\"");
		}			
	}

	canvas.popDialog(newMessageHud);
}


