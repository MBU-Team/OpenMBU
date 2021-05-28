//-----------------------------------------------------------------------------
// ---------------------
// File ....: ViewFile.h
// ---------------------
// Author...: Gus J Grubba
// Date ....: September 1995
// O.S. ....: Windows NT 3.51
//
// History .: Nov, 02 1995 - Created
//
// This is the "View File" option in MAX's File menu.
//
//-----------------------------------------------------------------------------

#ifndef _VIEWFINCLUDE_
#define _VIEWFINCLUDE_

#ifndef  VWFEXPORT
#define  VWFEXPORT __declspec( dllimport )
#endif

//-----------------------------------------------------------------------------
//--  Base Class Definition ---------------------------------------------------
//-----------------------------------------------------------------------------
// #> ViewFile
//
     
class ViewFile {

     private:   
        
        //-- Windows Specific -------------------------------------------------
        
        HWND              hWnd;

     public:

        //-- Constructors/Destructors -----------------------------------------

        VWFEXPORT         ViewFile           ( );
        VWFEXPORT        ~ViewFile           ( );
     
        //-- The Method -------------------------------------------------------
        //

        VWFEXPORT void    View              ( HWND hWnd );

};

//-----------------------------------------------------------------------------
//-- Interface

VWFEXPORT void *ViewFileCreate  ( );
VWFEXPORT void  ViewFileDestroy ( ViewFile *v);

#endif

//-- EOF: ViewFile.h ----------------------------------------------------------
