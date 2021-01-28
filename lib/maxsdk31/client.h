//-----------------------------------------------------------------------------
// -------------------
// File ....: Client.h
// -------------------
// Author...: Gus J Grubba
// Date ....: November 1995
// O.S. ....: Windows NT 3.51
//
// History .: Nov, 18 1995 - Created
//
// 3D Studio Max Network Rendering 
// 
// Client
//
//-----------------------------------------------------------------------------

#ifndef _CLIENTINCLUDE_
#define _CLIENTINCLUDE_

#ifndef  NETCEXPORT
#define  NETCEXPORT __declspec( dllimport )
#endif

//-----------------------------------------------------------------------------
//--  Interface Class ---------------------------------------------------------
//-----------------------------------------------------------------------------
     
class ClientInterface {

	public:

		virtual HINSTANCE	HostInst       		() = 0;
		virtual HWND		HostWnd          	() = 0;
		virtual TCHAR     	*GetDir           	(int i) = 0;
		virtual TCHAR     	*GetAppDir        	() = 0;
		virtual TSTR      	GetMaxFileName   	() = 0;
		virtual BOOL    	SaveMaxFile      	(TCHAR *name, TCHAR *archivename, bool archive = false) = 0;

};

//-----------------------------------------------------------------------------
//--  Base Class Definition ---------------------------------------------------
//-----------------------------------------------------------------------------
// #> Client
//
     
class Client {

	private:   
        
		TCPcomm          *tcp;
		ConnectionInfo   ci;

		#define          DEFNUMMANAGERS  4

		char             manager[DEFNUMMANAGERS][MAX_PATH];
		WORD             mgrport;

		Tab<ServerList>  Servers;
		ServerReg        sReg;
		ClientInterface  *iface;
		Interface		 *max;

        int                curServer;

        int                numSel;
        int               *selBuf;

        int                start,end,step;

		int				progressNFrames;
		BOOL			alertFailure,alertProgress,alertCompletion;
		BOOL			alertEnabled;
        
        //-- Flags defined in Common.h (NewJob flags)

        DWORD              flags;
        
        //-- Windows Specific -------------------------------------------------
        
        HWND               hWnd;
        HBITMAP            hBmpBulbOn,hBmpBulbBusy,hBmpBulbOff,hBmpBulbError;
        HBITMAP            hBmpBulbOnSel,hBmpBulbBusySel,hBmpBulbOffSel,hBmpBulbErrorSel;

        //-- Miscelaneous Functions -------------------------------------------

        void               UpdateManagerList         ( HWND hWnd );
        BOOL               GetMaxFile                ( HWND hWnd, TCHAR *name );

     public:

		//-- Job Info


		NewJob	*TheJob;


		//-- Constructors/Destructors -----------------------------------------

		NETCEXPORT         Client           ( ClientInterface *i, Interface *max );
		NETCEXPORT        ~Client           ( );
		void               Close            ( );
     
        //-- Public Interface -------------------------------------------------
        //

		NETCEXPORT BOOL	JobAssignment		( HWND hWnd, NewJob *job);
        NETCEXPORT BOOL	QueueManagement		( HWND hWnd );
        NETCEXPORT BOOL	JobMonitor			( HWND hWnd );

        //-- Dialog Functions -------------------------------------------------
        //

        BOOL			JobDlg           	( HWND,UINT,WPARAM,LPARAM );
        BOOL			PropDlg          	( HWND,UINT,WPARAM,LPARAM );
        BOOL			AlertDlg          	( HWND,UINT,WPARAM,LPARAM );
        BOOL			QueueDlg         	( HWND,UINT,WPARAM,LPARAM );
        BOOL			OutputExists		( HWND,UINT,WPARAM,LPARAM );
        
        BOOL			SubmitJob        	( HWND hWnd );
        void			LoadServerList   	( HWND hWnd );
        void			ResetServerList  	( HWND hWnd );
        void			HandleButtonState	( HWND hWnd );
        void			ShowServerProp   	( HWND hWnd );
        void			FixJobName       	( HWND hWnd );
        BOOL			CanConnect         	( HWND );
        void			ConnectManager     	( HWND );

        //-- Miscelaneous -----------------------------------------------------
        //

		Interface*		Max					( ) { return max; }
        void			ReadCfg				( );
        void			WriteCfg			( );
        
        BOOL			IsFile				( const TCHAR *filename );
        void			GetIniFile			( TCHAR *name );
        
};

//-----------------------------------------------------------------------------
//-- Interface

NETCEXPORT void *ClientCreate  ( ClientInterface *i, Interface *m );
NETCEXPORT void  ClientDestroy ( Client *v);

#endif

//-- EOF: Client.h ------------------------------------------------------------
