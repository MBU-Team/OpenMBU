/**********************************************************************
 *<
	FILE: ICommandPanel

	DESCRIPTION: Command Panel API

	CREATED BY: Nikolai Sander

	HISTORY: created 7/11/00

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __ICOMMANDPANEL_H__
#define __ICOMMANDPANEL_H__

class ICommandPanel : public FPStaticInterface 
{
	public:
	
	// function IDs 
	enum { 
		   fnIdGetRollupThreshhold,
		   fnIdSetRollupThreshhold,
	};
	
	virtual int GetRollupThreshold()=0;
	virtual void SetRollupThreshold(int iThresh)=0;
};

#define COMMAND_PANEL_INTERFACE Interface_ID(0x411753f6, 0x69a93710)
inline ICommandPanel* GetICommandPanel() { return (ICommandPanel*)GetCOREInterface(COMMAND_PANEL_INTERFACE); }

#endif