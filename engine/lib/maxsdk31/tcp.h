//-----------------------------------------------------------------------------
// ----------------
// File ....: tcp.h
// ----------------
// Author...: Gus J Grubba
// Date ....: September 1995
// O.S. ....: Windows NT 3.51
//
// Note ....: Copyright 1991, 1995 Gus J Grubba
//
// History .: Sep, 03 1995 - Ported to C++ / WinNT
//
//-----------------------------------------------------------------------------

#ifndef _TCPINCLUDE_
#define _TCPINCLUDE_

#include <winsock.h>

//-- Constants ----------------------------------------------------------------

#define MAXUDPLEN           512
#define LOCALHOSTADDRESSH   0x0100007F  //-- (Host Order)
#define LOCALHOSTADDRESSN   0x7F000001  //-- (Network Order)

//-----------------------------------------------------------------------------
//-- Error Codes

#define GCRES_SERVICEERROR        0x1000
#define GCRES_GETHOSTERROR        0x1001
#define GCRES_CANNOTCREATESOCKET  0x1002
#define GCRES_CANNOTCONNECT       0x1003
#define GCRES_BINDERROR           0x1004
#define GCRES_CANNOTSERVE         0x1005
#define GCRES_DISCONNECTED        0x1006
#define GCRES_READERROR           0x1007
#define GCRES_INVALIDSERVERTHREAD 0x1008
#define GCRES_INVALIDPORT         0x1009
#define GCRES_NOTINITIALIZED      0x100A
#define GCRES_TOOBIG              0x100B
#define GCRES_WRITEERROR          0x100C
#define GCRES_TIMEOUT             0x100D

//-- Forward References ------------------------------------------------------

class ConnectionInfo;
class TCPcomm;

//-----------------------------------------------------------------------------
//-- Server Thread

typedef void (WINAPI *SERVER_THREAD)(
     ConnectionInfo *ci
);

typedef struct tag_tcpSRV {
     ConnectionInfo *ci;
     TCPcomm       *tcp;
} tcpSRV;

//-----------------------------------------------------------------------------
//-- Connection data

class ConnectionInfo {

        //-- All kept in host order

        void    *ptr, *param;        //-- Generic Pointers
        
        BOOL     blocking,dns;

        SOCKET   c_sock;             //-- The "client" socket
        DWORD    c_address;          //-- The "client" inet address
        char     c_name[MAX_PATH];   //-- The "client" hostname
        WORD     c_port;             //-- The "client" port

        SOCKET   s_sock;             //-- The "server" socket
        DWORD    s_address;          //-- The "server" inet address
        char     s_name[MAX_PATH];   //-- The "server" hostname
        WORD     s_port;             //-- The "server" port

        DWORD    bytes_s, bytes_r;
        
     public:
     
        GCOMMEXPORT          ConnectionInfo ( ) { Reset(); dns = FALSE;}

        GCOMMEXPORT void     Reset( ) {
                    ptr     = NULL;
                    param   = NULL;
                    bytes_s = 0;
                    bytes_r = 0;
                    ResetClient();
                    ResetServer();
                 }
        
        GCOMMEXPORT void     ResetClient ( ) {
                    c_sock    = INVALID_SOCKET;
                    c_address = 0;
                    c_name[0] = 0;
                 }
        
        GCOMMEXPORT void     ResetServer ( ) {
                    s_sock    = INVALID_SOCKET;
                    s_address = 0;
                    s_name[0] = 0;
                 }
        
        GCOMMEXPORT void     SetClientSocket    ( SOCKET s) { c_sock    = s;    }
        GCOMMEXPORT void     SetClientAddress   ( DWORD  a) { c_address = a;    }
        GCOMMEXPORT void     SetClientName      ( char  *n) { strcpy(c_name,n); }
        GCOMMEXPORT void     SetClientPort      ( WORD p)   { c_port    = p;    }
     
        GCOMMEXPORT void     SetServerSocket    ( SOCKET s) { s_sock    = s;    }
        GCOMMEXPORT void     SetServerAddress   ( DWORD  a) { s_address = a;    }
        GCOMMEXPORT void     SetServerName      ( char  *n) { strcpy(s_name,n); }
        GCOMMEXPORT void     SetServerPort      ( WORD p)   { s_port    = p;    }
     
        GCOMMEXPORT SOCKET   ClientSocket       ( ) { return c_sock;    }
        GCOMMEXPORT DWORD    ClientAddress      ( ) { return c_address; }
        GCOMMEXPORT WORD     ClientPort         ( ) { return c_port;    }
        GCOMMEXPORT char    *ClientName         ( ) { return c_name;    }
     
        GCOMMEXPORT SOCKET   ServerSocket       ( ) { return s_sock;    }
        GCOMMEXPORT WORD     ServerPort         ( ) { return s_port;    }
        GCOMMEXPORT DWORD    ServerAddress      ( ) { return s_address; }
        GCOMMEXPORT char    *ServerName         ( ) { return s_name;    }
     
        GCOMMEXPORT void     AddToBytesSent     ( DWORD b ) { bytes_s += b; }
        GCOMMEXPORT void     AddToBytesReceived ( DWORD b ) { bytes_r += b; }

        GCOMMEXPORT DWORD    BytesSent          ( ) { return bytes_s; }
        GCOMMEXPORT DWORD    BytesReceived      ( ) { return bytes_r; }

        GCOMMEXPORT void     ResetBytesSent     ( ) { bytes_s = 0; }
        GCOMMEXPORT void     ResetBytesReceived ( ) { bytes_r = 0; }
     
        GCOMMEXPORT void    *Ptr                ( ) { return ptr;         }
        GCOMMEXPORT void     SetPtr             ( void * p ) { ptr = p;   }
        GCOMMEXPORT void    *Param              ( ) { return param;       }
        GCOMMEXPORT void     SetParam           ( void * p ) { param = p; }

        GCOMMEXPORT void     SetBlocking        ( BOOL b ) { blocking = b; }
        GCOMMEXPORT BOOL     Blocking           ( ) { return blocking; }
        
        GCOMMEXPORT void     SetUseDns			( BOOL b ) { dns = b; }
        GCOMMEXPORT BOOL     UseDns				( ) { return dns; }
        
};

//-----------------------------------------------------------------------------
//--  BSD Socket Class Definition ---------------------------------------------
//-----------------------------------------------------------------------------
// #> TCPcomm
//
     
class TCPcomm : public tcCOMM {

     private:
     
        BOOL        initialized;
        WSADATA     WSAData;
        SOCKET      sSocket;
        
        BOOL        HandleStandardErrors    ( int err, TCHAR *msg );
        
     public:

        GCOMMEXPORT        TCPcomm          ( );
        GCOMMEXPORT       ~TCPcomm          ( );
     
        //-- House Keeping --------------------------------

        GCOMMEXPORT BOOL   Init             ( HWND hWnd );
        GCOMMEXPORT BOOL   Setup            ( void * ) { return TRUE; }
        GCOMMEXPORT void   Close            ( );
        GCOMMEXPORT BOOL   SaveSession      ( void *ptr ) { return TRUE; }
        GCOMMEXPORT BOOL   LoadSession      ( void *ptr ) { return TRUE; }
        GCOMMEXPORT DWORD  EvaluateDataSize ( )           { return 0;    }

        //-- Helpers --------------------------------------

        GCOMMEXPORT GCRES  GetHostAddress   ( DWORD *addr, char *name, char *fullname = NULL );
                    
        //-- TCP Transport --------------------------------

        GCOMMEXPORT GCRES  Serve            ( WORD port, SERVER_THREAD func, void *param );
        GCOMMEXPORT void   StopServer       ( );
        GCOMMEXPORT GCRES  Send             ( ConnectionInfo *ci, void *buf, int len, float timeout = 5.0f );
        GCOMMEXPORT GCRES  Receive          ( ConnectionInfo *ci, void *buf, int len, float timeout = 5.0f );
        GCOMMEXPORT GCRES  rlogin           ( ConnectionInfo *ci );
        GCOMMEXPORT GCRES  Connect          ( ConnectionInfo *ci );
        GCOMMEXPORT void   Disconnect       ( ConnectionInfo *ci );
        
        //-- UDP Transport --------------------------------
        
        #define UDP_BLOCK    1
        #define UDP_NONBLOCK 0

        GCOMMEXPORT GCRES  CreateUDPServer  ( ConnectionInfo *ci, BOOL block = UDP_NONBLOCK );
        GCOMMEXPORT GCRES  CloseUDPServer   ( ConnectionInfo *ci );
        GCOMMEXPORT GCRES  SendUDP          ( ConnectionInfo *ci, void *buf, int len, int *written, float timeout = 5.0f);
        GCOMMEXPORT GCRES  ReceiveUDP       ( ConnectionInfo *ci, void *buf, int len, int *read,    float timeout = 5.0f);

};

#endif

//-- EOF: tcp.h ---------------------------------------------------------------
