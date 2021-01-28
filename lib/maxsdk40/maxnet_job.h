//-----------------------------------------------------------------------------
// -----------------------
// File ....: maxnet_job.h
// -----------------------
// Author...: Gus J Grubba
// Date ....: March 2000
// O.S. ....: Windows 2000
//
// History .: March, 11 2000 - Created
//
// 3D Studio Max Network Rendering Classes - Job
// 
//-----------------------------------------------------------------------------

#ifndef _MAXNET_JOB_H_
#define _MAXNET_JOB_H_

typedef DWORD HJOB;

#ifdef WIN32
#ifndef MAXNETEXPORT
#define MAXNETEXPORT __declspec( dllimport )
#endif
#else
#define MAXNETEXPORT
#endif

//-----------------------------------------------------------------------------
//-- Forward Reference

class MaxNetManagerImp;

//-----------------------------------------------------------------------------
//-- Scene Info

//#define SCENE_SHADOWMAPPED	(1<<0)	Obsolete
//#define SCENE_RAYTRACED		(1<<1)	Obsolete
#define SCENE_VIDEOCOLORCHECK	(1<<2)
#define SCENE_TWOSIDED			(1<<3)
#define SCENE_RENDERHIDEN		(1<<4)
#define SCENE_RENDERATMOSPHER	(1<<5)
#define SCENE_SUPERBLACK		(1<<6)
//#define SCENE_RENDERALPHA		(1<<7)	Obsolete
#define SCENE_SERIALNUMBERING	(1<<8)
#define SCENE_DITHER256			(1<<9)
#define SCENE_DITHERTRUE		(1<<10)
#define SCENE_RENDERFIELDS		(1<<11)
#define SCENE_DISPLACEMENT		(1<<12)
#define SCENE_EFFECTS			(1<<13)
#define SCENE_FIELDORDER		(1<<14)		//-- 0 Even / 1 Odd

typedef struct tagSceneInfo {
	int		objects;			//-- Number of Objects in scene
	int		faces;				//-- Total number of faces in scene
	int		lights;				//-- Total number of lights in scene
	int		start,end;			//-- Scene Start and End times
	DWORD	flags;				//-- SCENE_XXX above
} SceneInfo;

//-----------------------------------------------------------------------------
//-- Text Info
//

typedef enum {
	JOB_TEXT_USER=64,				//-- User Name
	JOB_TEXT_COMPUTER,				//-- Computer Name (Job Submission)
	JOB_TEXT_MANAGER_SHARE,			//-- Manager's share (where to find job - filled by Manager)
	JOB_TEXT_FRAMES,				//-- Frames for those "1,2,4,5-40" types. Otherwise frames are defined in Job
	JOB_TEXT_MAX_OUTPUT,			//-- Output image file name (MAX)
	JOB_TEXT_CMB_OUTPUT,			//-- Output image file name (Combustion)
	JOB_TEXT_RENDER_ELEMENT,		//-- Render Elements
	JOB_TEXT_CAMERA,				//-- Camera List
	JOB_TEXT_RESERVED = 9999		//-- Unknown Type
} JOB_TEXT_TYPE;

typedef struct tag_TextBufferOutput {
	bool	device;
	float	gamma;
	TCHAR	data[256];
}TextBufferOutput;

typedef struct tag_JobRenderElement {
	bool	enabled;
	bool	filterenabled;
	bool	atmosphere_applied;
	bool	shadows_applied;
	TCHAR	name[128];
	TCHAR	output[MAX_PATH];
}JobRenderElement;

typedef struct tag_TextBuffer {
	JOB_TEXT_TYPE type;
	union {
		TCHAR				text[256];
		TextBufferOutput	output;
		JobRenderElement	re;
	};
} JobText;

class MAXNETEXPORT CJobText {
	friend class MaxNetManagerImp;
	protected:
		void*	list;
	public:
				CJobText		( );
				~CJobText		( );
		
		int			Count		( );
		int			Add			(JobText* jt);
		void		Delete		(int idx, int count = 1);
		void		Reset		( );
		JobText*	Buffer		( );
		int			BufferSize	( );
		
		JobText& operator[](const int i);

		int		FindJobText		(JOB_TEXT_TYPE tp, int start = 0);
		bool	GetTextItem		(TCHAR* text, JOB_TEXT_TYPE type, int start = 0, int* idx = 0);
		bool	GetUser			(TCHAR* user);
		bool	GetComputer		(TCHAR* computer);
		bool	GetFrames		(TCHAR* frames);
		bool	GetShare		(TCHAR* share);

};

//---------------------------------------------------------
//-- Alert Notification
//

#define NOTIFY_FAILURE		(1<<0)
#define NOTIFY_PROGRESS		(1<<1)
#define NOTIFY_COMPLETION	(1<<2)

typedef struct tag_AlertData {
	bool		alertEnabled;			//-- Are alerts enabled?
	DWORD		notifications;			//-- Bitmap of alerts enabled
	int			nthFrames;				//-- Every n frames for progress reports
} AlertData;

//---------------------------------------------------------
//-- Job Flags

#define JOB_VP				(1<<0)		//-- Video Post (otherwise is Render Scene)
#define JOB_NONC			(1<<1)		//-- Non concurrent driver (Accom, AVI, etc.)
#define JOB_MAPS			(1<<2)		//-- Include Maps
#define JOB_NONSTOP			(1<<3)		//-- Uninterruptible Driver (AVI, FCL, etc.)
#define JOB_SKIPEXST		(1<<4)		//-- Skip Existing Frames
#define JOB_ALLSERVERS		(1<<5)		//-- Use All Available Servers
#define JOB_INACTIVE		(1<<6)		//-- Job is Suspended
#define JOB_COMPLETE		(1<<7)		//-- Job is Complete (Read Only)
#define JOB_IGNORESHARE		(1<<8)		//-- Ignore Manager's Job Share - Always request archives
#define JOB_SKIPOUTPUTTST	(1<<9)		//-- Skip output test (Server won't test the output path)
#define JOB_NONSEQFRAMES	(1<<10)		//-- Non sequential frames (1,3,5-10, etc.)
#define JOB_COMBUSTIONJOB	(1<<11)		//-- Combustion Job
#define JOB_NOTARCHIVED		(1<<12)		//-- Uncompressed File (not an archive)
#define JOB_VFB				(1<<13)		//-- Should the VFB be up?
#define JOB_RENDER_CROP		(1<<14)		//-- Partial Render (Crop / Zoom / Region / etc.)

#define JOB_ASSIGN_VP		JOB_VP		//-- Make compatible with legacy flag
#define JOB_ASSIGN_RND		0			//-- Make compatible with legacy flag

//---------------------------------------------------------
//-- Priority Level

#define _JOB_PRIORITY_CRITICAL	0
#define _JOB_DEFAULT_PRIORITY	50

//---------------------------------------------------------
//-- Job

#define _JOB_VERSION 399

typedef struct tag_MaxJobRenderElements {
	bool		enabled;
}MaxJobRenderElements;

typedef struct tag_MaxJob {
	bool					init;					//-- Is structure valid?
	bool					gammacorrection;		//-- Use gamma correction?
	float					gammavaluein;			//-- Input Gamma (Maps)
	float					gammavalueout;			//-- Output Gamma (Output Image)
	float					pixelaspect;			//-- Pixel Aspect Ratio
	char					camera[128];			//-- Camera
	SceneInfo				sceneInfo;				//-- Scene Info 
	MaxJobRenderElements	re;						//-- Render Elements
	char					reserved[64];
}MaxJob;

typedef struct tag_CombustionJob {
	bool		init;					//-- Is structure valid?
	char		reserved[128];
}CombustionJob;

typedef struct tag_Job {
	DWORD		size;					//-- Structure Size ( size = sizeof(Job) )
	DWORD		version;				//-- Structure Version
	DWORD		server_pid;				//-- Server Process ID (Used by MAX to check server's health)
	DWORD		flags;					//-- JOB_XXX flags
	HJOB		hJob;					//-- Assigned by Manager when job is created (Submitted) - Read Only afterwards
	char		name[MAX_PATH];			//-- Job Name
	DWORD		filesize;				//-- Used internally when transferring archive (size of archive file)
	DWORD		filesizeextracted;		//-- Used internally when transferring archive (size of expanded archive)
	SYSTEMTIME	submission;				//-- Set when job is submitted
	SYSTEMTIME	startjob;				//-- Set when job starts
	SYSTEMTIME	endjob;					//-- Set when job is completed
	int			servercount;			//-- Number of servers defined for this job (can be 0 if JOB_ALLSERVERS is set)
	AlertData	alerts;					//-- Alerts
	int			jobtextcount;			//-- Number of JobTextInfo records
	int			firstframe;				//-- First frame in range
	int			lastframe;				//-- Last frame in range
	int			step;					//-- Every nth frame (step)
	int			width,height;			//-- Output Dimensions
	int			frames_completed;		//-- Number of completed frames
	char		priority;				//-- Priority Level
	char		reserved[32];			//-- Future stuff
	union {
		MaxJob			maxJob;			//-- MAX Specific
		CombustionJob	combustionJob;	//-- Combustion Specific
	};
} Job;

#endif

//-- EOF: maxnet_job.h --------------------------------------------------------

										