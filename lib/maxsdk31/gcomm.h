//-----------------------------------------------------------------------------
// ------------------
// File ....: gcomm.h
// ------------------
// Author...: Gus J Grubba
// Date ....: September 1995
// O.S. ....: Windows NT 3.51
//
// Note ....: Copyright 1991, 1995 Gus J Grubba
//
// History .: Sep, 03 1995 - Ported to C++ / WinNT
//
//-----------------------------------------------------------------------------

#ifndef _GCOMMINCLUDE_
#define _GCOMMINCLUDE_

//-----------------------------------------------------------------------------
//-- Common Error Codes

typedef unsigned int GCRES;

#define GCRES_SUCCESS           0x0000

//-----------------------------------------------------------------------------
//-- Error Handler

typedef void (WINAPI *PERROR_HANDLER)(
     int          ErrorCode,
     const TCHAR *ErrorMessage
    );

//-----------------------------------------------------------------------------
//-- Client Types

#define gcTCP         1    //-- 0x1000 ... 0x1FFF
#define gcUART        2    //-- 0x2000 ... 0x2FFF
#define gcSCSI        3    //-- 0x3000 ... 0x3FFF
#define gcCENTRONICS  4    //-- 0x4000 ... 0x4FFF
#define gcIPX         5    //-- 0x5000 ... 0x5FFF
#define gcNETBIOS     6    //-- 0x6000 ... 0x6FFF

//-----------------------------------------------------------------------------
//-- Error Types (for logging)

#define ERR_FATAL  0 // Fatal Error, won't procede
#define ERR_WARN   1 // Warning Error, will procede with defaults (No Error Dialogue)
#define ERR_INFO   2 // Not an error, just a logging message (No Error Dialogue)
#define ERR_DEBUG  3 // Not an error, just debugging information (No Error Dialogue)

//-----------------------------------------------
//-- NetBios

#ifndef ADAPTER_STATUS_BLOCK

#include <nb30.h>

typedef struct tag_ADAPTER_STATUS_BLOCK {
	ADAPTER_STATUS	asb_header;
	NAME_BUFFER		asb_Names[32];
} ADAPTER_STATUS_BLOCK;

typedef struct tag_MAC_ADDRESS {
	unsigned char addr[6];
} MAC_ADDRESS;

#endif

//-----------------------------------------------------------------------------
//--  FileName Class Definition -----------------------------------------------
//-----------------------------------------------------------------------------
// #> CFileName
//
     
class CFileName {
		TCHAR	filename[MAX_PATH];
	public:
		GCOMMEXPORT 		CFileName	( ) { filename[0] = 0; }
		GCOMMEXPORT 		CFileName	( TCHAR *name ) { SetName(name); }
		GCOMMEXPORT void	SetName		( TCHAR *name ) { _tcscpy(filename,name); }
		GCOMMEXPORT void	SetExtension( TCHAR *ext );
		GCOMMEXPORT TCHAR*	Extension	( );
		GCOMMEXPORT TCHAR*	FileName	( );
		GCOMMEXPORT DWORD	FileSize	( );
		GCOMMEXPORT TCHAR*	FullName	( ) { return filename; }
};

//-----------------------------------------------------------------------------
//--  Timer Class Definition --------------------------------------------------
//-----------------------------------------------------------------------------
// #> Timer
//
     
class Timer {

        float timer,count;

     public:

        //-- Timer methods ----------------------------------------------------
        //
        //   Timers are kept in fractional units of seconds with a resolution 
        //   no less than 10ms.

        GCOMMEXPORT             Timer            ( float t = 2.0f ) { timer = t; Start(); }
        GCOMMEXPORT void        Set              ( float t = 2.0f ) { timer = t; }
        GCOMMEXPORT void        Start            ( );
        GCOMMEXPORT BOOL        IsTimeout        ( );
        GCOMMEXPORT float       Elapsed          ( );

};

//-----------------------------------------------------------------------------
//--  Base Class Definition ---------------------------------------------------
//-----------------------------------------------------------------------------
// #> tcCOMM
//
     
class tcCOMM {

     private:   
        
        //-- Windows Specific -------------------------------------------------
        
        HINSTANCE           tcphInst;
        HWND                hWnd;

        //-- System -----------------------------------------------------------
        
        BOOL                silentmode;
        PERROR_HANDLER      errorhandler;
        TCHAR               error_title[64];

     public:

        //-- Constructors/Destructors -----------------------------------------

        GCOMMEXPORT         tcCOMM           ( );
        GCOMMEXPORT        ~tcCOMM           ( );
     
        //-- Initialization process -------------------------------------------
        //
        
        virtual BOOL        Init             ( HWND hWnd         )=0;
        virtual BOOL        Setup            ( void *setupdata   )=0;
        virtual void        Close            ( )=0;

        //-- Application provided hooks for saving/restoring session ----------
        //
        //   If the application wants to save and restore session data, such
        //   as numbers, names, addresses, etc., it should use these methods.
        //   Each subclassed object should implement a method for saving and
        //   restoring its own session data.
        //
        //   The host will issue a LoadSession() with  previously saved data
        //   before issuing an Init() call. At the end of a session, it will
        //   issue the SaveSession() before calling Close();
        //
        //   The host will use EvaluateDataSize() to find out the size of the
        //   buffer needed (called prior to SaveSession()).
        //   

        virtual BOOL        SaveSession      ( void *ptr )=0;
        virtual BOOL        LoadSession      ( void *ptr )=0;
        virtual DWORD       EvaluateDataSize ( )=0;
        
        //-- Services ---------------------------------------------------------
        //
        //   If you want your own error handler, use this function to register
        //   one. Whenever an error occurs, it will be called with an error 
        //   code and with an optional error string message. The function
        //   prototype is:
        //
        //   void WINAPI ErrorHandler (int ErrorCode, TCHAR *ErrorMessage);
        //
        //   ErrorCode is one of the defined error codes above.
        //   ErrorMessage, if not NULL, contains a textual description of the
        //   error and can be used directly.
        //
        //   Note that only ERR_FATAL warrants a mandatory action. All other
        //   error types are handled as you please. The idea is to provide
        //   ongoing messages for logging purposes and debugging. Internally,
        //   only ERR_FATAL will generate a dialogue message (provided the
        //   silence flag is set to FALSE);
        //
        
        GCOMMEXPORT void    RegisterErrorHandler  (PERROR_HANDLER handler);
        
        //-- If you do not provide an error handler, the driver will produce
        //   its own error messages. If you don't pass a Window handler, the
        //   driver will use the system window (root) as the parent window.
        //   
        //   You can set the driver not to produce any error message at all 
        //   by setting the Silent flag to TRUE. Simply use SetSilentMode().


        HWND                GethWnd          ( )          { return  hWnd; }
        void                SethWnd          ( HWND hwnd ) { hWnd = hwnd; }
        BOOL                SilentMode       ( )     { return silentmode; }
        void                SetSilentMode    ( BOOL v ) { silentmode = v; }

        //-- Error Dialogue Title ---------------------------------------------
        //
        //   If you let the driver produce its own error dialogue boxes, you
        //   still can set the Window title if you want something other than
        //   the default "Transport Error".
        
        void                SetErrorTitle    ( TCHAR *t ) {_tcscpy(error_title,t);}

        //-- Internal Services ------------------------------------------------
        //
    
        void                SetInstance      ( HINSTANCE hi ) { tcphInst = hi; }
        HINSTANCE           GetInstance      (  )             { return (tcphInst); }
        TCHAR              *GetLastErrorText ( TCHAR *buf, DWORD size );
        void                Error            ( int type, const TCHAR *message );

};

//-----------------------------------------------------------------------------
//-- Interface

GCOMMEXPORT void*	gcommCreate		( int   type		);
GCOMMEXPORT void	gcommDestroy	( void* ptr			);
GCOMMEXPORT bool	gcInitNetRender	( char* target=NULL	);

//-----------------------------------------------------------------------------
//-- Utilities

GCOMMEXPORT	bool	gcGetMacAddress			(MAC_ADDRESS *addr);
GCOMMEXPORT	bool	gcMatchMacAddress		(MAC_ADDRESS *addr1, MAC_ADDRESS *addr2);
GCOMMEXPORT	void	gcMac2String			(MAC_ADDRESS *addr, TCHAR *string);
GCOMMEXPORT	void	gcMac2StringCondensed	(MAC_ADDRESS *addr, TCHAR *string);

#include "tcp.h"

#endif

//-- EOF: gcomm.h -------------------------------------------------------------
