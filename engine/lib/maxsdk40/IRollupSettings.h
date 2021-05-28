/**********************************************************************
 *<
	FILE: IRollupSettings

	DESCRIPTION: Rollup Window Settings Interface

	CREATED BY: Nikolai Sander

	HISTORY: created 8/8/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __IROLLUP_H__
#define __IROLLUP_H__

class ICatRegistry;

#define EFFECTS_ROLLUPCFG_CLASSID Class_ID(0x559c705d, 0x32573fe3)
#define ATMOSPHERICS_ROLLUPCFG_CLASSID Class_ID(0x40ef5775, 0x5b5606f)
#define DISPLAY_PANEL_ROLLUPCFG_CLASSID Class_ID(0x12d45445, 0x3a5779a2)
#define MOTION_PANEL_ROLLUPCFG_CLASSID Class_ID(0xbea1816, 0x50de0291)
#define HIERARCHY_PANEL_ROLLUPCFG_CLASSID Class_ID(0x5d6c08d4, 0x7e5e2c2b)
#define	UTILITY_PANEL_ROLLUPCFG_CLASSID   Class_ID(0x2e256000, 0x6a5b2b34)

class IRollupSettings : public FPStaticInterface 
{
	public:
	virtual ICatRegistry *GetCatReg()=0;
};

class ICatRegistry
{
public:
	// This method gets the category (order field) for the given SuperClass ID, ClassID and Rollup Title
	virtual int  GetCat(SClass_ID sid, Class_ID cid, TCHAR *title,int category)=0;
	// This method updates (sets) the category (order field) for the given SuperClass ID, ClassID and Rollup Title
	virtual void UpdateCat(SClass_ID sid, Class_ID cid, TCHAR *title,int category)=0;
	// This method Saves the category settings in File "UI\RollupOrder.cfg"
	virtual void Save()=0;
	// This method Loads the category settings from File "UI\RollupOrder.cfg"
	virtual void Load()=0;
	// This method Erases all category settings (in memory only)
	virtual void EmptyRegistry()=0;
	// This method deletes a category list for a given superclass ID and class ID
	virtual void DeleteList(SClass_ID sid, Class_ID cid)=0;
};

#define ROLLUP_SETTINGS_INTERFACE Interface_ID(0x281a65e8, 0x12db025d)
inline IRollupSettings* GetIRollupSettings() { return (IRollupSettings*)GetCOREInterface(ROLLUP_SETTINGS_INTERFACE); }

#endif