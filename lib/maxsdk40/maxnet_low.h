//-----------------------------------------------------------------------------
// ---------------------------
// File ....: maxnet_manager.h
// ---------------------------
// Author...: Gus J Grubba
// Date ....: August 2000
// O.S. ....: Windows 2000
//
// History .: Aug, 15 2000 - Created
//
// 3D Studio Max Network Rendering Classes - Low Level definitions
// 
//-----------------------------------------------------------------------------

#ifndef _MAXNET_LOW_H_
#define _MAXNET_LOW_H_

//---------------------------------------------------------
//-- Commands

typedef enum {		
	CMDM_ACK	= 0x46471000,	//-- Acknowledge                    
	CMDM_CLIENTLOGIN,			//-- Client Login (QueueManager, Job Assignment, etc.)
	CMDM_PING,					//-- Ping                           
	CMDM_DHCP,					//-- Broadcast (Who is the manager?)
	CMDM_KILLMANAGER,			//-- Kill Manager (It will bring the whole thing down)
	CMDM_ENABLEUPDATE,			//-- Toggle Manager Updates to Clients
	CMDM_GETMGRINFO,			//-- Get Manager Info
	CMDM_REG,					//-- Server Requests Registration
	CMDM_UNREG,					//-- Server Going Down
	CMDM_GLOG,					//-- Get Log (Job Log)
	CMDM_GJOB,					//-- Get Job
	CMDM_GJOBTEXT,				//-- Get Job Text
	CMDM_SJOB,					//-- Set Job
	CMDM_GETMLOG,				//-- Get MAX Log (Server Log as opposed to Job Log)
	CMDM_CLEARMLOG,				//-- Clear MAX Log
	CMDM_GETJOBCOUNT,			//-- Get Job Count
	CMDM_GETJOBSERVERS,			//-- Get Job Servers
	CMDM_LISTJOBS,				//-- List Jobs
	CMDM_DELJOB,				//-- Delete Job
	CMDM_ENDSESSION,			//-- End TCP Session
	CMDM_GETWKS,				//-- Get Server's Week Schedule
	CMDM_SETWKS,				//-- Set Server's Week Schedule
	CMDM_GETSRVCOUNT,			//-- Get Server Count
	CMDM_GETJOBSRVCOUNT,		//-- Get Job Server Count
	CMDM_LISTSERVERS,			//-- List Servers
	CMDM_DELSERVER,				//-- Delete Server (Must be absent)
	CMDM_GETJOBFRAMESCOUNT,		//-- Get Job Frames Count
	CMDM_LISTFRAMES,			//-- List Frames (Frame Status)
	CMDM_GETSRVGRPCOUNT,		//-- Get Server Group Count (Number of Groups)
	CMDM_GETSRVGRPXCOUNT,		//-- Get Server Group X Count (Number of Servers on Group X)
	CMDM_GETSRVGRPXSERVERS,		//-- Get Server Group X Servers (The server list)
	CMDM_NEWSRVGRP,				//-- New Server Group
	CMDM_DELSRVGROUP,			//-- Delete Server Group
	CMDM_NEWJOB,				//-- New Job
	CMDM_NEWORDER,				//-- New Job Order
	CMDM_SETJOBSTATE,			//-- Set Job State
	CMDM_CLEARSERVERSTATE,		//-- Clear [Job] Server State (From Failed to OK) - Adds to job if not already
	CMDM_REMOVEJOBSERVER,		//-- Remove Server from Job
	CMDM_NEWFRAME,				//-- New Frame Assignment
	CMDM_FRAMECOMPLETE,			//-- Frame Complete
	CMDM_FRAMEERROR,			//-- Frame Error
	CMDM_GETSERVERINFO,			//-- Get Server Info
	CMDM_BADSERVERINFO,			//-- Manager Did not accept Server
	CMDM_ENDJOB,				//-- End Job command
	CMDM_CANCELJOB,				//-- Cancel Job command (Stops everything)
	CMDM_MGRDOWN,				//-- Manager going down
	CMDM_UPDATE,				//-- Something new with Manager (new job, new server, new frame rendered, etc.)
	CMDM_QUERYCONTROL,			//-- Query Queue Control
	CMDM_TAKECONTROL,			//-- Take Control of the Job Queue
	CMDM_GRANTCONTROL,			//-- Grant Control of the Job Queue (Response to Query Queue Control)
	CMDM_LOCKCONTROL,			//-- Lock/Unlock Manager Control
	CMDM_GETCLIENTCOUNT,		//-- Get Client Count
	CMDM_GETCLIENTLIST,			//-- Get List of Clients
	CMDM_NETSTAT,				//-- Net Stats (Server Sends to Manager Every 10 minutes or so)
	CMDM_GETSRVNETSTAT,			//-- Get Server Net Stats
	CMDM_CHKOUTPUT,				//-- Asks manager to check output visibility
	CMDM_GETJOBSRVSTATUS,		//-- Get Job Server Status Message (Errors if any)
	CMDM_RESETSRVINDEX,			//-- Reset Server Index
	CMDM_GETJOBPRIORITY,		//-- Get Job Priority
	CMDM_SETJOBPRIORITY,		//-- Set Job Priority
	CMDM_GJOBL,					//-- Get Job (JobList)
	CMDM_GSRVL,					//-- Get Server (ServerList)
	CMDM_UNKNOWN
};

//-- Ack Types

#define ACKID_PING			0x40
#define ACKID_NEWFRAME		0x41
#define ACKID_BADFRAME		0x42
#define ACKID_MAXUP			0x43
#define ACKID_MAXDOWN		0x44
#define ACKID_FRAMECOMPLETE	0x45
#define ACKID_MAXERROR		0x46
#define ACKID_CANCELJOB		0x47
#define ACKID_ENDJOB		0x48

//---------------------------------------------------------
//-- Queries

typedef struct tag_RANGE {
	int			start,end;
} RANGE;

typedef struct _MAX_QUERY {
	HJOB	hJob;
	HSERVER	hServer;
	union {
		bool			bValue;
		int				iValue;
		DWORD			dwValue;
		RANGE			range;
		WeekSchedule	ws;
		NetworkStatus	net_stat;
    };
} MAX_QUERY;

typedef struct tag_FRAMECOMPLETE {
	int		frame;
	DWORD	pmemory;
	DWORD	vmemory;
} FRAMECOMPLETE;

#endif

//-- EOF: maxnet_low.h --------------------------------------------------------

