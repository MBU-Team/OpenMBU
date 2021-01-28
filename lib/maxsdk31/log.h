//-----------------------------------------------------------------------------
// ----------------
// File ....: log.h
// ----------------
// Author...: Gus Grubba
// Date ....: November 1996
//
// History .: Nov, 27 1996 - Started
//
//-----------------------------------------------------------------------------
		
#ifndef ERRORLOG_H_DEFINED
#define ERRORLOG_H_DEFINED

#define NO_DIALOG		FALSE
#define DISPLAY_DIALOG	TRUE

#define	SYSLOG_ERROR	0x00000001
#define	SYSLOG_WARN		0x00000002
#define	SYSLOG_INFO		0x00000004
#define	SYSLOG_DEBUG	0x00000008

#define	SYSLOG_LIFE_EVER	0
#define	SYSLOG_LIFE_DAYS	1
#define	SYSLOG_LIFE_SIZE	2

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Frame   Range
//
	
class LogSys {

		DWORD	valTypes;
		int		logLife;
		DWORD	logDays;
		DWORD	logSize;

	 public:

		//-- Maintenance methods -----------------------------------------------
		//
		//	 Methods used internally

		//-- Queries what log types are enabled

		virtual		DWORD	LogTypes ( ) { return valTypes; }

		//-- Sets what log types are enabled

		virtual		void	SetLogTypes ( DWORD types ) { valTypes = types; }

		//-- Logging methods ---------------------------------------------------
		//
		//	 "type"	defines the type of log entry based on LogTypes above.
		//
		//   "dialogue" is DISPLAY_DIALOGUE if you want the message to be displayed
		//   in a dialogue. The system will determine if displaying a dialogue is
		//   appropriate based on network rendering mode. If this is just some
		//   information you don't want a dialogue for, or if you are handling
		//   the dialogue yourself, just set dialogue to NO_DIALOGUE.
		//
		//
		//   "title" is optional. If non NULL, it will be used to define the module
		//   that originated the log entry (and the title bar in the dialogue).
		//
		//
	 
		virtual		void	LogEntry		( DWORD type, BOOL dialogue, TCHAR *title, TCHAR *text,... ) = 0;

		//-- By turning on quiet mode the log system will not display any dialogues
		//-- even if it is not noetwork rendering.
		//-- The error will only be written to the log file.
		virtual		void	SetQuietMode( bool quiet ) = 0;
		virtual		bool	GetQuietMode( ) = 0;

		//-- Log File Longevity ------------------------------------------------

		virtual		int		Longevity		( )				{ return logLife; }
		virtual		void	SetLongevity	( int type )	{ logLife = type; }
		virtual		DWORD	LogDays			( )				{ return logDays; }
		virtual		DWORD	LogSize			( )				{ return logSize; }
		virtual		void	SetLogDays		( DWORD days ) 	{ logDays = days; }
		virtual		void	SetLogSize		( DWORD size ) 	{ logSize = size; }

		//-- State -------------------------------------------------------------

		virtual		void	SaveState		( void ) = 0;
		virtual		void	LoadState		( void ) = 0;

};

#endif

//-- EOF: log.h ---------------------------------------------------------------
