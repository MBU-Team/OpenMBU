//-----------------------------------------------------------------------------
// ------------------------
// File ....: maxnet_file.h
// ------------------------
// Author...: Gus J Grubba
// Date ....: March 2000
// O.S. ....: Windows 2000
//
// History .: March, 11 2000 - Created
//
// 3D Studio Max Network Rendering Classes - Argument Files
// 
//-----------------------------------------------------------------------------

#ifndef _MAXNET_FILE_H_
#define _MAXNET_FILE_H_

//-----------------------------------------------------------------------------
//-- Network Rendering Configuration File
//

typedef struct tagGeneralSettings {
	short		managerport;
	short		serverport;
	DWORD		maxblocksize;
	DWORD		networkmask;
} GeneralSettings;

typedef struct tagClientSettings {
	int			autoconnect;
	int			includemaps;
	int			ignorescene;
	int			skipoutput;
	int			useallsrvs;
	int			alerts;
	int			alert_failure;
	int			alert_prgs;
	int			alert_comp;
	int			alert_nth;
	DWORD		server_tabs;
	int			use_vfb;
} ClientSettings;

typedef struct tagManagerSettings {
	int			retryfailedservers;
	int			retrycount;
	DWORD		timebetweenretries;
	DWORD		waitmaxtoload;
	DWORD		waitmaxtounload;
	DWORD		waitmaxtorender;
	DWORD		servercooloff;
	int			maxconcurrentassignments;
	DWORD		clientnotifydelay;
} ManagerSettings;

typedef struct tagManagerApp {
	int			init;
	int			window_x;
	int			window_y;
	int			window_w;
	int			window_h;
} ManagerApp;

typedef struct tagServerApp {
	int			init;
	int			window_x;
	int			window_y;
	int			window_w;
	int			window_h;
} ServerApp;

typedef struct tagQueueManager {
	int			init;
	int			window_x;
	int			window_y;
	int			window_w;
	int			window_h;
	int			main_splitter_row;
	int			job_splitter_col;
	int			autoconnect;
	int			autorefresh;
	DWORD		server_tabs;
} QueueManager;

typedef struct tagServerSettings {
	TCHAR		managername[MAX_PATH];
	int			autosearch;
	DWORD		waitmaxtoload;
	DWORD		waitmaxtounload;
	DWORD		waitmaxtorender;
} ServerSettings;

typedef struct tagLogSettings {
	int			maxscreenlines;
	int			logerror_screen;
	int			logwarning_screen;
	int			loginfo_screen;
	int			logdebug_screen;
	int			logdebugex_screen;
	int			logerror_file;
	int			logwarning_file;
	int			loginfo_file;
	int			logdebug_file;
	int			logdebugex_file;
} LogSettings;

typedef struct tagTimeSettings {
	int			acktimeout;
	int			ackretries;
	int			fastacktimeout;
} TimeSettings;

//-----------------------------------------------------------------------------
//-- Sections
//

#ifdef	_MXNTF_DEFINE_

#define MXNTF_SERVERINFO			"ServerInfo"
#define MXNTF_VERSION				"Version"
#define MXNTF_SRV_PID				"ServerPID"
#define MXNTF_SRV_TOTAL_FRAMES		"TotalFrames"
#define MXNTF_SRV_TOTAL_TIME		"TotalTime"
#define MXNTF_SRV_INDEX				"Index"
#define MXNTF_CONFIG_BLOCK			"Configuration"
#define MXNTF_TOTAL_MEMORY			"TotalMemory"
#define MXNTF_TOTAL_CPU				"NumberCpus"
#define MXNTF_OS_MAJOR				"OSMajor"
#define MXNTF_OS_MINOR				"OSMinor"
#define MXNTF_OS_BUILD				"OSBuild"
#define MXNTF_OS_PLATFORM_ID		"OSPlatformID"
#define MXNTF_OS_CSDV				"OSCSDV"
#define MXNTF_TEMP_DIR				"TempDir"
#define MXNTF_WORK_DISK				"WorkDisk"
#define MXNTF_AVAILABLE_DISKS		"AvailableDisks"
#define MXNTF_DISK_SPACE			"DiskSpace"
#define MXNTF_MAC					"MAC"
#define MXNTF_NETSTATUS				"NetworkStatus"
#define MXNTF_DROPPED_PACKETS		"DroppedPackets"
#define MXNTF_BAD_PACKETS			"BadPackets"
#define MXNTF_TCP_REQUESTS			"TCPRequests"
#define MXNTF_UDP_REQUESTS			"UDPRequests"
#define MXNTF_BOOT_TIME				"BootTime"
#define MXNTF_JOB					"Job"
#define MXNTF_FLAGS					"Flags"
#define MXNTF_HJOB					"HJOB"
#define MXNTF_JOB_NAME				"JobName"
#define MXNTF_JOB_FIRST_FRAME		"FirstFrame"
#define MXNTF_JOB_LAST_FRAME		"LastFrame"
#define MXNTF_STEP					"Step"
#define MXNTF_JOB_SUBMISSION		"Submission"
#define MXNTF_JOB_START				"Start"
#define MXNTF_JOB_END				"End"
#define MXNTF_JOB_FRAMES_COMPLETE	"FramesComplete"
#define MXNTF_JOB_SERVER_COUNT		"ServerCount"
#define MXNTF_JOB_PRIORITY			"JobPriority"
#define MXNTF_JOB_ARCHSIZE			"ArchivedSize"
#define MXNTF_JOB_UNARCHSIZE		"UnarchivedSize"
#define MXNTF_ALERTS				"Alerts"
#define MXNTF_NOTIFICATIONS			"Notifications"
#define MXNTF_ENABLED				"Enabled"
#define MXNTF_USER_NAME				"UserName"
#define MXNTF_COMPUTER_NAME			"ComputerName"
#define MXNTF_JOB_TEXT				"JobText"
#define MXNTF_COUNT					"Count"
#define MXNTF_JOB_TEXTITEM			"JobTextItem"
#define MXNTF_JOB_TEXT_TEXT			"JobTextText"
#define MXNTF_JOB_TEXT_OUTPUT		"JobTextOutput"
#define MXNTF_JOB_TEXT_OUTPUT_FILE	"JobTextOutputData"
#define MXNTF_R_ELEMENT				"RenderElement"
#define MXNTF_RE_ENABLED			"RenderElementEnabled"
#define MXNTF_RE_ACTIVE				"RenderElementsActive"
#define MXNTF_RE_FLT_ENABLED		"RenderElementFilterEnabled"
#define MXNTF_RE_ATM_ENABLED		"RenderElementAtmEnabled"
#define MXNTF_RE_SHD_ENABLED		"RenderElementShadowEnabled"
#define MXNTF_RE_NAME				"RenderElementName"
#define MXNTF_RE_OUTPUT				"RenderElementOutput"
#define MXNTF_JOB_FRAMES			"JobFrames"
#define MXNTF_JOB_FRAME_NO			"Frame"
#define MXNTF_JOB_SERVERS			"JobServers"
#define MXNTF_JOB_SERVER_NO			"Server"
#define MXNTF_WIDTH					"Width"
#define MXNTF_HEIGHT				"Height"
#define MXNTF_PIXELASPECT			"PixelAspect"
#define MXNTF_INIT					"Init"
#define MXNTF_GAMMACORRECTION		"UseGammaCorrection"
#define MXNTF_INPUTGAMMA			"InputGamma"
#define MXNTF_OUTPUTGAMMA			"OutputGamma"
#define MXNTF_CAMERA				"Camera"
#define MXNTF_SCENE_INFO			"SceneInfo"
#define MXNTF_NO_OBJECTS			"NumberOfObjects"
#define MXNTF_NO_FACES				"NumberOfFaces"
#define MXNTF_NO_LIGHTS				"NumberOfLights"
#define MXNTF_SCENE_FLAGS			"SceneFlags"
#define MXNTF_SCENE_START			"SceneStart"
#define MXNTF_SCENE_END				"SceneEnd"
#define MXNTF_JOB_STATE				"JobState"
#define MXNTF_NO_JOBS				"NumberOfJobs"
#define MXNTF_JOB_STATE_N			"Job"
#define MXNTF_INI_GENERAL			"General"
#define MXNTF_INI_MGRPORT			"ManagerPort"
#define MXNTF_INI_SRVPORT			"ServerPort"
#define MXNTF_INI_MAXBLOCK			"MaxBlockSize"
#define MXNTF_INI_NETMASK			"NetworkMask"
#define MXNTF_INI_MANAGER			"Manager"
#define MXNTF_INI_QM				"QueueManager"
#define MXNTF_INI_MGRAPP			"ManagerApp"
#define MXNTF_INI_SRVAPP			"ServerApp"
#define MXNTF_INI_INIT				"Init"
#define MXNTF_INI_WX				"WindowX"
#define MXNTF_INI_WY				"WindowY"
#define MXNTF_INI_WW				"WindowW"
#define MXNTF_INI_WH				"WindowH"
#define MXNTF_INI_SR				"SplitterRoll"
#define MXNTF_INI_SC				"SplitterColumn"
#define MXNTF_INI_CLI_VFB			"UseVFB"
#define MXNTF_INI_SRV_TABS			"ServerTabs"
#define MXNTF_INI_AUTO_CONN			"AutoConnect"
#define MXNTF_INI_AUTO_REFRESH		"AutoRefresh"
#define MXNTF_INI_RETRYFSERVERS		"RetryFailedServers"
#define MXNTF_INI_SRVCOOLOFF		"ServerCoolOffTime"
#define MXNTF_INI_CLIENTNOTIFYDELAY	"ClientNotificationDelay"
#define MXNTF_INI_RETRYCOUNT		"RetryCount"
#define MXNTF_INI_TIMEBRETRY		"TimeBetweenRetries"
#define MXNTF_INI_MAXLOADTIME		"MAXLoadTimeout"
#define MXNTF_INI_MAXRENDERTIME		"MAXRenderTimeout"
#define MXNTF_INI_MAXUNLOADTIME		"MAXUnloadTimeout"
#define MXNTF_INI_MAXCONCASSIGN		"MaxConcurrentAssignments"
#define MXNTF_INI_TIME				"Timers"
#define MXNTF_INI_ACKTIMEOUT		"AckTimeout"
#define MXNTF_INI_ACKRETRY			"AckRetries"
#define MXNTF_INI_FASTACKTIMEOUT	"FastAckTimeout"
#define MXNTF_INI_LOG				"Log"
#define MXNTF_INI_MAXSCRRENLN		"MaxLogScreenLines"
#define MXNTF_INI_ERRORSCREEN		"LogErrorsToScreen"
#define MXNTF_INI_WARNSCREEN		"LogWarningsToScreen"
#define MXNTF_INI_INFOSCREEN		"LogInfoToScreen"
#define MXNTF_INI_DEBUGSCREEN		"LogDebugToScreen"
#define MXNTF_INI_DEBUGEXSCREEN		"LogDebugExToScreen"
#define MXNTF_INI_ERRORFILE			"LogErrorsToFile"
#define MXNTF_INI_WARNFILE			"LogWarningsToFile"
#define MXNTF_INI_INFOFILE			"LogInfoToFile"
#define MXNTF_INI_DEBUGFILE			"LogDebugToFile"
#define MXNTF_INI_DEBUGEXFILE		"LogDebugExToFile"
#define MXNTF_INI_SERVER			"Server"
#define MXNTF_INI_AUTOSEARCH		"AutoSearchManager"
#define MXNTF_INI_MGRNAME			"ManagerName"
#define MXNTF_WEEK_SCHEDULE			"WeekSchedule"
#define MXNTF_ATT_PRIORITY			"AttendedPriority"
#define MXNTF_UTT_PRIORITY			"UnattendedPriority"
#define MXNTF_WEEK_DAY				"Day"
#define MXNTF_INI_CLIENT			"ClientSettings"
#define MXNTF_INI_ALERT_COMPLETION	"AlertCompletion"
#define MXNTF_INI_ALERT_FAILURE		"AlertFailure"
#define MXNTF_INI_ALERT_NTH			"AlertEveryNth"
#define MXNTF_INI_ALERT_PROGRESS	"AlertProgress"
#define MXNTF_INI_ALERTS			"Alerts"
#define MXNTF_INI_AUTOCONNECT		"AutoConnect"
#define MXNTF_INI_IGNORESCENE		"IgnoreScenePath"
#define MXNTF_INI_INCLUDEMAPS		"IncludeMaps"
#define MXNTF_INI_SKIPOUTPUT		"SkipOutputTest"
#define MXNTF_INI_USEALLSERVERS		"UseAllServers"

#endif

//---------------------------------------------------------
//-- Messages from MAX to Server

#define	FROMMAX_READY			"ready"
#define	FROMMAX_FRAME_COMPLETE	"complete"
#define	FROMMAX_FRAME_ERROR		"error"
#define	FROMMAX_BUSY			"busy"
#define	FROMMAX_GOING_DOWN		"end"

//---------------------------------------------------------
//-- Messages from Server to Max

#define	FROMSRV_NEWFRAME		"frame"
#define	FROMSRV_CANCEL			"cancel"

//-----------------------------------------------------------------------------
//-- File Class
//

class MAXNETEXPORT MaxNetFile {

	protected:
		
		TCHAR	line[1024];
		FILE*	f;
		bool	read;

	public:

				MaxNetFile		( );
		virtual	~MaxNetFile		( );

		virtual bool	OpenRead	(const TCHAR* name);
		virtual bool	OpenWrite	(const TCHAR* name);
		virtual void	Close		( );					//-- Optional (Destructor calls it)
		void			Reset		( );					//-- fseek 0 seek_set
		
		TCHAR*	FirstBlank		(TCHAR* line);
		TCHAR*	FirstNonblank	(TCHAR* line);
		TCHAR*	NextValue		(TCHAR* ln);
		bool	LocateSection	(TCHAR* section);
		TCHAR*	LocateValue		(TCHAR* section, TCHAR* var, bool rescan = true);
		TCHAR*	GetLine			(TCHAR* destination = 0 );
		
		bool	ReadFilename	(TCHAR* ptr, TCHAR* filename);

		bool	Write			(TCHAR* line);
		bool	WriteHeader		(TCHAR* hdr);
		bool	WritePair		(TCHAR* var, float value);
		bool	WritePair		(TCHAR* var, int value);
		bool	WritePair		(TCHAR* var, bool value);
		bool	WritePair		(TCHAR* var, short value);
		bool	WritePair		(TCHAR* var, DWORD value);
		bool	WritePair		(TCHAR* var, TCHAR* value);
		bool	WritePair		(TCHAR* var, SYSTEMTIME* value);
		bool	WritePairHex	(TCHAR* var, DWORD value);

		bool	ReadPair		(TCHAR* section, TCHAR* var, float* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, int* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, bool* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, short* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, DWORD* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, TCHAR* value);
		bool	ReadPair		(TCHAR* section, TCHAR* var, SYSTEMTIME* value);
		bool	ReadPairHex		(TCHAR* section, TCHAR* var, DWORD* value);

};

//-----------------------------------------------------------------------------
//-- Network Engine Config
//

class MAXNETEXPORT MaxNetEngine : public MaxNetFile {

	public:

		bool	WriteNetStatus		(NetworkStatus* status);
		bool	WriteCfg			(ConfigurationBlock *cfg);
		bool	WriteAlertData		(AlertData *alerts);
		bool	WriteJobText		(CJobText& jobText);
		bool	WriteSceneInfo		(SceneInfo* info);
		bool	WriteWeekSchedule	(WeekSchedule* ws);

		bool	ReadNetStatus		(NetworkStatus* status);
		bool	ReadCfg				(ConfigurationBlock *cfg);
		bool	ReadAlertData		(AlertData *alerts);
		int		ReadJobTextCount	( );
		bool	ReadJobText			(CJobText& jobText);
		bool	ReadSceneInfo		(SceneInfo* info);
		bool	ReadWeekSchedule	(WeekSchedule* ws);

};

//-----------------------------------------------------------------------------
//-- Network Server Registration Data
//

class MAXNETEXPORT MaxNetServerReg : public MaxNetEngine {
	public:
		bool	WriteSrvInfo	(ServerInfo *info);
		bool	ReadSrvInfo		(ServerInfo* info);
};

//-----------------------------------------------------------------------------
//-- Network Job
//

class MAXNETEXPORT MaxNetJob : public MaxNetEngine {
	public:
		bool	WriteJob		(Job* job, CJobText& jobText);
		bool	ReadJob			(Job* job, CJobText& jobText);
		bool	WriteServers	(int count, const JobServer* servers);
		bool	ReadServers		(int count, JobServer* servers);
		bool	WriteFrames		(int count, const JOBFRAMES* frames);
		bool	ReadFrames		(int count, JOBFRAMES* frames);
};

//-----------------------------------------------------------------------------
//-- Message File From Max to Server
//

typedef enum max_msg{
	MAX_MESSAGE_NOTHING=0,
	MAX_MESSAGE_READY,
	MAX_MESSAGE_BUSY,
	MAX_MESSAGE_FRAME_COMPLETE,
	MAX_MESSAGE_FRAME_ERROR,
	MAX_MESSAGE_GOING_DOWN
};

class MAXNETEXPORT MsgFromMax : public MaxNetFile {
	public:
		//-- Server uses this to peek/read messages
		max_msg	Message			(TCHAR* file, TCHAR* err = 0);
		//-- MAX uses this to write out messages
		void	WriteMessage	(max_msg message, TCHAR* err = 0);
};

//-----------------------------------------------------------------------------
//-- Message File From Server to Max
//

typedef enum srv_msg{
	SRV_MESSAGE_NOTHING=0,
	SRV_MESSAGE_NEWFRAME,
	SRV_MESSAGE_CANCEL
};

class MAXNETEXPORT MsgFromSrv : public MaxNetFile {
	public:
		//-- MAX uses this to peek/read messages
		srv_msg	Message			(TCHAR* file, int* arg);
		//-- Server uses this to write out messages
		void	WriteMessage	(srv_msg message, int arg = 0);
};

//-----------------------------------------------------------------------------
//-- 3DS MAX Configuration File
//

class MAXNETEXPORT MaxNetIni : public MaxNetFile {

	protected:

		ManagerApp			default_mgrapp;
		ServerApp			default_srvapp;
		GeneralSettings		default_general;
		ManagerSettings		default_manager;
		ServerSettings		default_server;
		LogSettings			default_log;
		TimeSettings		default_time;
		QueueManager		default_queuemanager;
		ClientSettings		default_client;

		bool				have_mgrapp;
		bool				have_srvapp;
		bool				have_general;
		bool				have_manager;
		bool				have_server;
		bool				have_log;
		bool				have_time;
		bool				have_queuemanager;
		bool				have_client;

		void	InitMgrApp		(ManagerApp* ma);
		void	InitSrvApp		(ServerApp* sa);
		void	InitGeneral		(GeneralSettings* gn);
		void	InitManager		(ManagerSettings* mg);
		void	InitServer		(ServerSettings* sv);
		void	InitLog			(LogSettings* lg);
		void	InitTime		(TimeSettings* tm);
		void	InitQueueManager(QueueManager* qm);
		void	InitClient		(ClientSettings *cl);

	public:

				~MaxNetIni		( );
		void	Close			( );

		bool	OpenWrite		(const TCHAR* name);
		bool	OpenRead		(const TCHAR* name);

		bool	WriteQM			(QueueManager* ma);
		bool	WriteMgrApp		(ManagerApp* ma);
		bool	WriteSrvApp		(ServerApp* sa);
		bool	WriteGeneral	(GeneralSettings* gn);
		bool	WriteManager	(ManagerSettings* mg);
		bool	WriteServer		(ServerSettings* sv);
		bool	WriteLog		(LogSettings* lg);
		bool	WriteTime		(TimeSettings* tm);
		bool	WriteClient		(ClientSettings* cl);

		bool	ReadQM			(QueueManager* ma);
		bool	ReadMgrApp		(ManagerApp* ma);
		bool	ReadSrvApp		(ServerApp* sa);
		bool	ReadGeneral		(GeneralSettings* gn);
		bool	ReadManager		(ManagerSettings* mg);
		bool	ReadServer		(ServerSettings* sv);
		bool	ReadLog			(LogSettings* lg);
		bool	ReadTime		(TimeSettings* tm);
		bool	ReadClient		(ClientSettings* cl);

};

#endif

//-- EOF: maxnet_file.h -------------------------------------------------------
