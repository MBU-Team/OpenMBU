/**********************************************************************
 *<
	FILE: cmdmode.h

	DESCRIPTION: Command mode class definition

	CREATED BY:	Rolf Berteig

	HISTORY: Created 13 January 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __CMDMODE__
#define __CMDMODE__

// This file can be included in plug-in modules so
// it shouldn't reference/include private classes or functions.

class MouseCallBack;
class ChangeForegroundCallback;

class CommandMode {
	public:
		virtual int Class()=0;
		virtual int SuperClass() { return 0; }
		virtual int ID()=0;
		virtual MouseCallBack *MouseProc(int *numPoints)=0;
		virtual ChangeForegroundCallback *ChangeFGProc()=0;
		virtual BOOL ChangeFG( CommandMode *oldMode )=0;
		virtual void EnterMode()=0;
		virtual void ExitMode()=0;		
	};

// This is just a collection of modes that make up the xform modes.
// Plug-ins can specify these for thier sub-object types.
class XFormModes {
	public:
		CommandMode *move;
		CommandMode *rotate;
		CommandMode *scale;
		CommandMode *uscale;
		CommandMode *squash;
		CommandMode *select;
		XFormModes( 
			CommandMode *move,
			CommandMode *rotate,
			CommandMode *scale,
			CommandMode *uscale,
			CommandMode *squash,
			CommandMode *select )
			{
			this->move   = move;
			this->rotate = rotate;
			this->scale  = scale;
			this->uscale = uscale;
			this->squash = squash;
			this->select = select;
			}
		XFormModes() { move = rotate = scale = uscale = squash = select = NULL; } 
	};


// These can be returned from ChangeFGProc() instead of an actual FG proc
// to use predefined FG sets.
#define CHANGE_FG_SELECTED		((ChangeForegroundCallback *)1)
#define CHANGE_FG_ANIMATED		((ChangeForegroundCallback *)2)

// command super classes:
#define TRANSFORM_CMD_SUPER 	1

// command classes
#define VIEWPORT_COMMAND		1
#define MOVE_COMMAND			2
#define ROTATE_COMMAND			3
#define SCALE_COMMAND			4
#define USCALE_COMMAND			5
#define SQUASH_COMMAND			6
#define SELECT_COMMAND			7
#define HIERARCHY_COMMAND		8
#define CREATE_COMMAND			9
#define MODIFY_COMMAND			10
#define MOTION_COMMAND			11
#define ANIMATION_COMMAND		12
#define CAMERA_COMMAND			13
#define NULL_COMMAND			14
#define DISPLAY_COMMAND			15
#define SPOTLIGHT_COMMAND		16
#define PICK_COMMAND			17


// command IDs
#define CID_USER				0x0000ffff

// XFORM_COMMAND
#define CID_OBJMOVE				1
#define CID_OBJROTATE			2
#define CID_OBJSCALE			3
#define CID_OBJUSCALE			4
#define CID_OBJSQUASH			5
#define CID_OBJSELECT			6

#define CID_SUBOBJMOVE			7
#define CID_SUBOBJROTATE		8
#define CID_SUBOBJSCALE			9
#define CID_SUBOBJUSCALE		10
#define CID_SUBOBJSQUASH		11
#define CID_SUBOBJSELECT		12

// display branch command modes
#define CID_UNFREEZE			13
#define CID_UNHIDE				14


// HEIRARCHY_COMMAND
#define CID_LINK				100
#define CID_BINDWSM				110		// I guess this is a heirarchy command... sort of

// VIEWPORT_COMMAND
#define CID_ZOOMVIEW			200
#define CID_ZOOMREGION			201
#define CID_PANVIEW				202
#define CID_ROTATEVIEW			203
#define CID_ZOOMALL				204
#define CID_RNDREGION			205

// CAMERA COMMANDS
#define CID_CAMFOV				210
#define CID_CAMDOLLY			211
#define CID_CAMPERSP			212
#define CID_CAMTRUCK			213
#define CID_CAMROTATE			214
#define CID_CAMROLL				215

//ANIMATION_COMMAND
#define CID_PLAYANIMATION		300

//CREATE_COMMAND		
#define CID_SIMPLECREATE		400

//MODIFIY_COMMAND
#define CID_MODIFYPARAM			500

//MOTION_COMMAND	

#define CID_NULL				600

// Pick modes
#define CID_STDPICK				710
#define CID_PICKAXISOBJECT		700

// ATTACH To GROUP COMMAND
#define CID_GRP_ATTACH			800



#endif // __CMDMODE

