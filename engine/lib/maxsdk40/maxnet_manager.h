//-----------------------------------------------------------------------------
// ---------------------------
// File ....: maxnet_manager.h
// ---------------------------
// Author...: Gus J Grubba
// Date ....: February 2000
// O.S. ....: Windows 2000
//
// History .: Feb, 15 2000 - Created
//
// 3D Studio Max Network Rendering Classes
// 
//-----------------------------------------------------------------------------

#ifndef _MAXNET_MANAGER_H_
#define _MAXNET_MANAGER_H_

//---------------------------------------------------------
//-- Directories and Files

#define MAXEXE			_T("3dsmax.exe")
#define COMBEXE			_T("combustionQueue.exe")

#ifdef DESIGN_VER
#define VIZEXE			_T("3dsviz.exe")
#endif
#define SRVTOCOMB		_T("ToCombustion.txt")
#define SRVTOMAX		_T("ToMax.txt")
#define APPTOSRV		_T("ToServer.txt")
#define MAXNETINI		_T("maxnet.ini")
#define MGRHST			_T("hosts.ini")
#define JOBSTATEFILE	_T("jobs.ini")
#define NETDIR			_T("Network")
#define REGDIR			_T("Servers")
#define JOBDIR			_T("Jobs")
#define SRVWORKDIR		_T("ServerJob")
#define MAXEXTENSION	_T(".max")
#define COMBEXTENSION	_T(".cws")
#define JOBDESCRP		_T("Job.txt")
#ifndef DESIGN_VER
#define MAXLOG			_T("Max.log")
#else
#define MAXLOG			_T("Viz.log")
#endif
#define _JOB_DIR_FORMAT_ "%s%08X %s%s"


#define MGRSERVICENAME	TEXT("Max4Manager")
#define SRVSERVICENAME	TEXT("Max4Server")

#define INIT_CFG_CMD_LINE	"INITCONFIG"

//---------------------------------------------------------
//-- Special Types

typedef struct tag_HSERVER {
	BYTE addr[8];
} HSERVER;

#define HBSERVER	(BYTE *)(void *) 

//---------------------------------------------------------
//-- Server Work Schedule

#define HIGHPRIORITY		0
#define LOWPRIORITY 		1
#define IDLEPRIORITY		2

typedef struct tag_Schedule {
	DWORD hour;						//-- Bitmap (24 bits = 24 hours of the day)
} Schedule;							//   0 Allowed to work - 1 Not Allowed

typedef struct tag_WeekSchedule {
	Schedule	day[7];
	int			AttendedPriority;
	int			UnattendedPriority;
} WeekSchedule;

//---------------------------------------------------------
//-- Time Helpers (All times are kept in ms)

#define _MAX_SECONDS			1000
#define _MAX_MINUTES			60 * _MAX_SECONDS
#define _MAX_HOURS				_MAX_MINUTES * 60	

//---------------------------------------------------------
//-- 3DS Network Rendering Defaults

//-- General

#define DF_SRVPORT				3333					//-- Server Port
#define DF_MGRPORT				3334					//-- Manager Port
#define DF_READCHUNK			128000					//-- Max Block Size
#define DF_NETMASK				0xFFFFFF00				//-- Network Mask

//-- Manager

#define DF_MAXCONASSIGN			4						//-- How many jobs can manager assign concurrently
#define DF_RETRYFAILED			1						//-- Should we retry failed servers?
#define DF_RESTARTLIMIT			3						//-- How many times to retry a failed server
#define DF_TIMEBETWEENRETRY		30 * _MAX_SECONDS		//-- Time to Wait between retries
#define DF_MAXLOADTIMEOUT		12 * _MAX_MINUTES		//-- Time to wait for MAX to load
#define DF_MAXUNLOADTIMEOUT		30 * _MAX_SECONDS		//-- Time to wait for MAX to unload
#define DF_MAXRENDERTIMEOUT		601 * _MAX_MINUTES		//-- Time to wait for MAX to render
#define DF_SERVERCOOLOFF		30 * _MAX_SECONDS		//-- When a server fails a frame, it goes into a cool off period

//-- Server

#define DF_AUTOSEARCH			1						//-- Automatically searches for a manager
#define DF_MAXMANAGER			"maxmanager"			//-- Default manager name for direct connect (i.e. if above is 0)
#define DF_SRVMAXLOADTIMEOUT	10 * _MAX_MINUTES		//-- Time to wait for MAX to load (Server Side)
#define DF_SRVMAXRENDERTIMEOUT	10 * _MAX_HOURS			//-- Time to wait for MAX to render (Server Side)
#define DF_SRVMAXUNLOADTIMEOUT	20 * _MAX_SECONDS		//-- Time to wait for MAX to unload (Server Side)

//-- Timers

#define DF_ACK_RETRIES			6						//-- How many times to try a silent response
#define DF_ACK_TIMEOUT			20 * _MAX_SECONDS		//-- Time between pings
#define DF_ACK_FAST_TIMEOUT		4 * _MAX_SECONDS		//-- Time between pings after one is missed

//-- Log

#define DF_LOGDEBUG_FILE		0
#define DF_LOGDEBUG_SCREEN		0
#define DF_LOGDEBUGEX_FILE		0
#define DF_LOGDEBUGEX_SCREEN	0
#define DF_LOGERROR_FILE		1
#define DF_LOGERROR_SCREEN		1
#define DF_LOGINFO_FILE			0
#define DF_LOGINFO_SCREEN		1
#define DF_LOGWARNING_FILE		0
#define DF_LOGWARNING_SCREEN	1
#define DF_MAX_SCREEN_LINES		1024

//---------------------------------------------------------
//-- Log Flags

#define MAXLOG_ERR				(1<<0)
#define MAXLOG_WARN				(1<<1)
#define MAXLOG_INFO				(1<<2)
#define MAXLOG_DEBUG			(1<<3)
#define MAXLOG_DEBUGEX			(1<<4)
#define MAXLOG_PGRS				(1<<5)
#define MAXLOG_INIT				(1<<6)
#define MAXLOG_START_SESSION	(1<<7)
#define MAXLOG_END_SESSION		(1<<8)

#define LOG_INFO_TXT     _T("INF")
#define LOG_ERROR_TXT    _T("ERR")
#define LOG_DEBUG_TXT    _T("DBG")
#define LOG_DEBUGX_TXT   _T("DBX")
#define LOG_PROGRESS_TXT _T("PRG")
#define LOG_INIT_TXT     _T("INI")

//---------------------------------------------------------
//-- Network Status
//

typedef struct tag_NetworkStatus {
	DWORD		dropped_packets;		//-- Packets dropped due to buffer overflow
	DWORD		bad_packets;			//-- Bad formed packets
	DWORD		tcprequests;			//-- Total number of TCP requests (since boot)
	DWORD		udprequests;			//-- Total number of UDP requests (since boot)
	SYSTEMTIME	boot_time;
	char		reserved[32];
} NetworkStatus;

//---------------------------------------------------------
//-- Station Configuration Block
//

typedef struct tag_ConfigurationBlock {
	DWORD	dwTotalPhys;			//-- GlobalMemoryStatus();
	DWORD	dwNumberOfProcessors;	//-- GetSystemInfo();
	DWORD	dwMajorVersion;			//-- GetVersionEx();	
	DWORD	dwMinorVersion;			//-- GetVersionEx();
	DWORD	dwBuildNumber;			//-- GetVersionEx();
	DWORD	dwPlatformId;			//-- GetVersionEx();
	TCHAR	szCSDVersion[128];		//-- GetVersionEx();
	char	user[MAX_PATH];			//-- GetUserName();
	char	tempdir[MAX_PATH];		//-- ExpandEnvironmentStrings()
	char	name[MAX_PATH];			//-- GetComputerName()
	char	workDisk;				//-- Disk used for Server files (incomming jobs, etc. A = 0, B = 1, etc)
	DWORD	disks;					//-- Available disks (bitmap A=0x1, B=0x2, C=0x4, etc)
	DWORD	diskSpace[26];			//-- Space available on disks in MegaBytes (A=diskSpace[0], B=diskSpace[1], etc.)
	BYTE	mac[8];					//-- Computer NIC address (00:00:00:00:00:00) 6 bytes + 2 padding
	char	reserved[32];			//-- Space to grow
} ConfigurationBlock;

//---------------------------------------------------------
//-- Manager Info
//

#define _MANAGER_INFO_VERSION 400

typedef struct tag_ManagerInfo {
	DWORD				size;			//-- Structure Size ( size = sizeof(ManagerInfo) )
	DWORD				version;
	ConfigurationBlock	cfg;
	NetworkStatus		net_status;
	int			  		servers;		//-- Number of registered servers
	int			  		jobs;			//-- Number of jobs
	char				reserved[32];	//-- Space to grow
} ManagerInfo;

//---------------------------------------------------------
//-- Server Info
//

#define _SERVER_INFO_VERSION 400

typedef struct tag_ServerInfo {
	DWORD				size;			//-- Structure Size ( size = sizeof(ServerInfo) )
	DWORD				version;		
	DWORD				total_frames;	//-- Total number of frames rendered
	float				total_time;		//-- Total time spent rendering (in hours)
	float				index;			//-- Performance index
	ConfigurationBlock	cfg;
	NetworkStatus		net_status;
	char				reserved[32];	//-- Space to grow
} ServerInfo;

//---------------------------------------------------------
//-- Client Info
//

#define _CLIENT_INFO_VERSION 400

typedef struct tag_ClientInfo {
	DWORD				size;			//-- Structure Size ( size = sizeof(ClientInfo) )
	DWORD				version;
	ConfigurationBlock	cfg;
	bool				controller;
	short		  		udp_port;
	char				reserved[32];	//-- Space to grow
} ClientInfo;

//-------------------------------------------------------------------
//-- Global Server State
//

#define SERVER_STATE_ABSENT    0
#define SERVER_STATE_IDLE      1
#define SERVER_STATE_BUSY      2
#define SERVER_STATE_ERROR     3
#define SERVER_STATE_SUSPENDED 4

typedef struct tag_ServerList {
	HSERVER		hServer;
	WORD		state;
	ServerInfo	info;
	//-- Current Task
	HJOB  		hJob;			//-- It will be 0 if no current task is defined
	int			frame;			//-- It will be NO_FRAME if loading job (no frames yet assigned)
	SYSTEMTIME	frame_started;	//-- Time frame was assigned
} ServerList;

//---------------------------------------------------------
//-- Server Statistics
//

typedef struct tag_Statistics {
	float		tseconds;
	int			frames;
} Statistics;

//-------------------------------------------------------------------
//-- Servers in Job Queue -------------------------------------------
//
//   Server Information for a given Job

#define JOB_SRV_IDLE		0		//-- Idle
#define JOB_SRV_BUSY		1		//-- Busy
#define JOB_SRV_FAILED		2		//-- Render Error
#define JOB_SRV_ABSENT		3		//-- Absent
#define JOB_SRV_SUSPENDED	4		//-- Out of work schedule
#define JOB_SRV_BUSYOTHER	5		//-- Busy with another job
#define JOB_SRV_ERROR		6		//-- Connection Error
#define JOB_SRV_COOL_OFF	7		//-- In Error Recovery

typedef struct tagJobServer {
	HSERVER		hServer;			//-- Server Handle
	char	  	status;				//-- JOB_SRV_XXX Status Above
	bool		failed;				//-- Internal Use
	bool		active;				//-- This server is active in the job
	int			cur_frame;			//-- Current Rendering Frame
	float		thours;				//-- Total Hours spent rendering
	int			frames;				//-- Total Number of Frames Rendered
} JobServer;

//-----------------------------------------------------------------------------
//-- Job Frame State

#define NO_FRAME				0x0FFFFFFF

#define FRAME_WAITING   		0
#define FRAME_ASSIGNED  		1
#define FRAME_COMPLETE  		2

typedef struct tagJobFrames {
	char	state;				//-- FRAME_XXX above
	int		frame;				//-- Frame Number
	HSERVER	hServer;			//-- The server rendering this frame
	DWORD	elapsed;			//-- Time it took to render this frame (milliseconds)
} JOBFRAMES;

//-------------------------------------------------------------------
//-- Global Server State
//

#define JOB_STATE_COMPLETE	0
#define JOB_STATE_WAITING	1
#define JOB_STATE_BUSY		2
#define JOB_STATE_ERROR		3
#define JOB_STATE_SUSPENDED	4

typedef struct tag_JobList {
	Job			job;
	HJOB		hJob;
	WORD		state;
} JobList;

//-----------------------------------------------------------------------------
//-- MaxNetCallBack
//
//	Note: Return as soon as possible from these calls. They block the API thread
//  and nothing will happen until you return. If you have to do some processing,
//  post a message to your own code and return immediately. Also note that these
//  calls may come from a separate thread than your main process thread. 
//

class MAXNETEXPORT MaxNetCallBack {
	public:
		//-- Return "true" to cancel, "false" to continue
		virtual bool Progress		(int total, int current){return false;}
		virtual void ProgressMsg	(const TCHAR *message){;}
		//-- Notifies the Manager Went Down
		virtual void ManagerDown	( ){;}
		//-- Notifies something has changed (new job, new server, new frame, etc.)
		virtual void Update			( ){;}
		//-- Notifies someone wants control of the queue; Send grant control msg to manager;
		virtual void QueryControl	( TCHAR* station ){;}
		//-- Notifies someone has taken control of the queue (Another QueueManager for instance)
		virtual void QueueControl	( ){;}
};

//-----------------------------------------------------------------------------
//-- Manager Session Class
//

class MAXNETEXPORT MaxNetManager : public MaxNet {

	public:

		//-- Optional Call Back
		
		virtual void	SetCallBack				( MaxNetCallBack* cb )=0;

		//-- Session

		virtual bool	FindManager				( short port, char* manager, char* netmask = "255.255.255.0" )=0;
		virtual void	Connect					( short port, char* manager = NULL, bool enable_callback = false )=0;
		virtual void	Disconnect				( )=0;
		virtual void	GetManagerInfo			( ManagerInfo* info )=0;
		virtual bool	KillManager				( )=0;
		virtual void	EnableUpdate			( bool enable = true )=0;
		
		virtual bool	QueryManagerControl		( bool wait )=0;
		virtual bool	TakeManagerControl		( )=0;
		virtual void	GrantManagerControl		( bool grant )=0;
		virtual bool	LockControl				( bool lock  )=0;
		virtual int		GetClientCount			( )=0;
		virtual int		ListClients				( int start, int end, ClientInfo* clientList )=0;

		//-- Jobs
		
		virtual int		GetJobCount				( )=0;
		virtual int		ListJobs				( int start, int end, JobList* jobList )=0;
		virtual void	GetJob					( HJOB hJob, JobList* jobList )=0;
		virtual void	GetJob					( HJOB hJob, Job* job )=0;
		virtual void	GetJobText				( HJOB hJob, CJobText& jobText, int count )=0;
		virtual void	SetJob					( HJOB hJob, Job* job, CJobText& jobText, bool reset )=0;
		virtual int		GetJobPriority			( HJOB hJob )=0;
		virtual bool	SetJobPriority			( HJOB hJob, int priority )=0;
		virtual void	SetJobOrder				( HJOB* hJob, DWORD count )=0;
		virtual void	DeleteJob				( HJOB hJob )=0;
		virtual void	SuspendJob				( HJOB hJob )=0;
		virtual void	ActivateJob				( HJOB hJob )=0;
		virtual int		GetJobServersCount		( HJOB hJob )=0;
		virtual int		GetJobServers			( int start, int end, HJOB hJob, JobServer* servers )=0;
		virtual void	GetJobServerStatus		( HJOB hJob, HSERVER hServer, TCHAR* status_text )=0;
		virtual void	SuspendJobServer		( HJOB hJob, HSERVER hServer )=0;
		virtual void	AssignJobServer			( HJOB hJob, HSERVER hServer )=0;
		virtual int		GetJobFramesCount		( HJOB hJob )=0;
		virtual int		GetJobFrames			( int start, int end, HJOB hJob, JOBFRAMES* frames )=0;
		virtual int		GetJobLog				( int start, int count, HJOB hJob, TCHAR** buffer )=0;
		
		virtual bool	CheckOutputVisibility	( TCHAR* output, TCHAR* err )=0;
		virtual void	AssignJob				( Job* job, TCHAR* archive, HSERVER* servers, CJobText& jobtext, DWORD blocksize = 0 )=0;
		

		//-- Servers (Global)

		virtual int		GetServerCount			( )=0;
		virtual int		ListServers				( int start, int end, ServerList* serverList )=0;
		virtual void	GetServer				( HSERVER hServer, ServerList* serverList )=0;
		virtual bool	DeleteServer			( HSERVER hServer )=0;
		virtual bool	ResetServerIndex		( HSERVER hServer )=0;
		virtual void	GetWeekSchedule			( HSERVER hServer, WeekSchedule* schedule )=0;
		virtual void	SetWeekSchedule			( HSERVER hServer, WeekSchedule* schedule )=0;
		virtual void	GetServerNetStat		( HSERVER hServer, NetworkStatus* net_stat )=0;
		
		virtual int		GetServerGroupCount		( )=0;
		virtual int		GetServerGroupXCount	( int group )=0;
		virtual int		GetServerGroup			( int group, int count, HSERVER* grplist, TCHAR* name )=0;
		virtual void	NewServerGroup			( int count, HSERVER* grplist, TCHAR* name )=0;
		virtual void	DeleteServerGroup		( int group )=0;
		

};

#endif

//-- EOF: maxnet_manager.h ----------------------------------------------------
