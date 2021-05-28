//-----------------------------------------------------------------------------
// --------------------
// File ....: Filters.h
// --------------------
// Author...: Gus Grubba
// Date ....: September 1995
//
// History .: Sep, 07 1995 - Started
//
//-----------------------------------------------------------------------------
		
#ifndef    FILTERS_H_DEFINED
#define    FILTERS_H_DEFINED

#include <fltapi.h>
#include <tvnode.h>

//-- Just to make it shorter

#define dVirtual FLTExport virtual

//-- How long can a filter name be

#define MAXFILTERNAME  MAX_PATH
#define MAXRESOURCE    MAX_PATH

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Frame Range
//
	
class FrameRange {

		int start;
		int end;
		int current;
		
	 public:
	 
		FLTExport       FrameRange  ( ) {start = end = current  = 0;}
		FLTExport       ~FrameRange ( ) {};

		FLTExport int   First       ( ) {   return (start); }
		FLTExport int   Last        ( ) {   return (end); }
		FLTExport int   Count       ( ) {   return (end - start + 1); }
		FLTExport int   Current     ( ) {   return (current);   }
		FLTExport int   Elapsed     ( ) {   return (current -   start); }

		FLTExport void  SetFirst    ( int u ) { start = u; }
		FLTExport void  SetLast     ( int u ) { end = u; }
		FLTExport void  SetCurrent  ( int u ) { current = u; }


};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Forward Reference

class ImageFilter;
class FilterManager;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Time Change Notification (R2)

class TimeChange : public TimeChangeCallback {
	public:
		BOOL set;
		TimeChange () { set = FALSE; }
		ImageFilter *filter;
		void TimeChanged(TimeValue t);
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Filter Info
//
	
enum MaskType {
	MASK_R = 0,  
	MASK_G, 
	MASK_B, 
	MASK_A, 
	MASK_L, 
	MASK_Z, 
	MASK_MTL_ID,
	MASK_NODE_ID

};

#define NUMMASKFLAGS (MASK_NODE_ID - MASK_R) + 1

class ImageFilterInfo {

		//-- Name of the filter used internally for identitification.

		TCHAR            name[MAXFILTERNAME];

		//-- Filters may want to identify themselves by something more  
		//   specific than their names. Specially filters that give names
		//   to parameter sets. If "resource" below is not empty, it
		//   will be used to identify the filter in the Video Post Queue.
		//   This is saved along with everything else by the host (Max).
		//   If all the filter needs is a resource to identify a set of
		//   parameters, this will sufice.

		TCHAR            resource[MAXRESOURCE];

		//-- Plug-In Parameter Block ------------------------------------------
		//
		//    No direct access to clients. Use the  methods in the  filter class.
		//

		void             *pidata;
		DWORD            pisize;

		//-- New R2 Stuff

		TCHAR			*userlabel;		//-- Optional label given by user
		ITrackViewNode	*node;			//-- TV Node (if any)
		Class_ID		nodeid;			//-- TV Node ID (if any);

		int				flttype;

	public:

		FLTExport        ImageFilterInfo                  ( );
		FLTExport       ~ImageFilterInfo                  ( );

		//-- Mask Information -------------------------------------------------

		BOOL                         maskenabled,evCopy;
		BOOL                         invertedmask;
		BitmapInfo                   mask;
		WORD                         maskflag;
		
		//-- This is a BitmapInfo that holds information about the current 
		//   Video Post main queue Image buffer. This can be used to get
		//   VP's (or target image) resolution, etc. To make an analogy, if
		//   this was a BitmapIO plug-in, this is the BitmapInfo given as
		//   the argument. This used primarilly at the time the filter
		//   receives the "Setup()" call as at render time, all this can be
		//   found in srcmap.

		BitmapInfo                   imgQueue;

		//-- Internal Helpers -------------------------------------------------

		FLTExport void              SetName         ( const TCHAR *n )    { _tcscpy(name,n);}
		FLTExport void              SetResource     ( const TCHAR *n )    { _tcscpy(resource,n);}
		FLTExport const TCHAR		*Name           ( )   { return    (const TCHAR *)name;}
		FLTExport const TCHAR		*Resource       ( )   { return    (const TCHAR *)resource;}
		
		//-- Plug-In Parameter Block ------------------------------------------
		
		FLTExport void              *GetPiData      ( ) { return pidata; }
		FLTExport void              SetPiData       ( void    *ptr ) { pidata = ptr; }
		FLTExport DWORD          	GetPiDataSize   ( )   { return    pisize; }
		FLTExport void              SetPiDataSize   ( DWORD s ) { pisize = s; }
		FLTExport void              ResetPiData     ( );
		FLTExport BOOL              AllocPiData     ( DWORD size  );

		FLTExport ImageFilterInfo &operator= (  ImageFilterInfo &from );
		
		//-- Load/Save
		
		FLTExport IOResult       	Save            ( ISave *isave );
		FLTExport IOResult       	Load            ( ILoad *iload, Interface *max );
		
		//-- Execution  Info ---------------------------------------------------
		//
		//    12/06/95 - GG
		//
		//    QueueRange defines    the entire Video Post Queue range. Execution
		//    is only the portion being rendered. This is, unless   the user    selects
		//    a "range", the same as QueueRange. FilterRange is where this  filter
		//    starts    and ends.
		//
		//    Video Post Queue
		//
		//              1         2         3         4         5
		//    0----|----|----|----|----|----|----|----|----|----|----|----|----|---- ...
		//
		//    Video Post spans from 0 to 49 (QueueRange) Start: 0  End: 49
		//
		//    qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
		//
		//    User executes a "range" from 10 to 30 (Execution Range) Start: 10 End: 30
		//
		//                  uuuuuuuuuuuuuuuuuuuuu
		//
		//    This filter appears in the queue from 5 to 35 (Filter Range) Start: 5 End: 35
		//
		//           fffffffffffffffffffffffffffffff        

		FrameRange                   QueueRange;              //-- Entire Video Post Queue
		FrameRange                   ExecutionRange;          //-- Segement being rendered
		FrameRange                   FilterRange;             //-- Filter Segment
		
		//----------------------------------------------------------------------
		//-- R2 Stuff Below ----------------------------------------------------
		//----------------------------------------------------------------------
	
		//-- Trackview Node Functions ------------------------------------------

		FLTExport ITrackViewNode	*Node	( ) 				{ return node;	}
		FLTExport void				SetNode (ITrackViewNode	*n) { node = n;		}

		FLTExport Class_ID			NodeID		( )		 		{ return nodeid;}
		FLTExport void				SetNodeID	( Class_ID id )	{ nodeid = id;  }

		//-- Optional Label given by user while adding or editing a filter. This label
		//   replaces the filter's name in Video Post's tracks for easier identification.

		FLTExport TCHAR				*UserLabel		( )			{ return userlabel; }

		//-- Used by VP to update the label. Not to be used by filters.

		FLTExport void				SetUserLabel	( TCHAR *l)	{ userlabel = l; }

		//-- Used to determine what type of filter this is at "Setup" time.

		#define	FLT_FILTER	0
		#define	FLT_LAYER	1

		FLTExport int				FilterType		( )	{ return flttype; }
		FLTExport void				SetFilterType	( int type ) { flttype = type; }

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Filter Plug-Ins Handler
//
	
class FLT_FilterHandler {

		//-- Name and Capabilities  ------------------------
		
		TCHAR         fltDescription[MAXFILTERNAME];    
		DWORD         fltCapability;

		//-- DLL    Handler ----------------------------------
		
		ClassDesc *cd;
		
	public:

		FLT_FilterHandler();
		
		FLTExport TCHAR           *Description        ( const TCHAR  *d = NULL  );

		FLTExport void             SetCD              ( ClassDesc *dll )    { cd = dll;}
		FLTExport ClassDesc       *GetCD              ( )                   { return    cd;}

		FLTExport void             SetCapabilities  ( DWORD cap )      { fltCapability |= cap;}
		FLTExport DWORD            GetCapabilities  ( )                { return    (fltCapability);}
		FLTExport BOOL             TestCapabilities ( DWORD cap )      { return    (fltCapability  & cap);}

};

//-----------------------------------------------------------------------------
//--   Messages    sent back by various    (client)    methods

//--   Sent by the plug-in to notify   host of current progress. The   host should
// return TRUE if  it's ok to continue or FALSE to abort process.

#define    FLT_PROGRESS    WM_USER + 0x20    //-- wParam:  Current lParam: Total

//--   Sent by the plug-in to check for    process interruption. The host should
// return FALSE (by setting *lParam) if it's   ok  to  continue    or  TRUE to abort 
// process.

#define	FLT_CHECKABORT	WM_USER + 0x21    //-- wParam:  0         lParam: BOOL*

//--   Sent by the plug-in to display an optional textual  message (for progress
// report).

#define	FLT_TEXTMSG		WM_USER + 0x22    //-- wParam:  0         lParam: LPCTSTR

//--   Sent by the host TO the plug-in to notify the time has changed (the user
// moved the time slider in Max).

#define    FLT_TIMECHANGED	WM_USER + 0x23    //-- wParam:  0         lParam: TimeValue t

//--   Sent by the host TO the plug-in to notify that an Undo operation has been done.
// The plugin will set some boolean internally and wait for the next WM_PAINT message
// in order to update any spinners or other values that may have been undone.

#define    FLT_UNDO		WM_USER + 0x24    //-- wParam:  0         lParam: 0

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   List of Filter  Plug-Ins
//
	
class FLT_FilterList: public   Tab<FLT_FilterHandler> {

		BOOL        listed;
		 
	public:

		FLT_FilterList        ( )           { listed    = FALSE;    }

		BOOL        Listed    ( BOOL    f)  { listed    = f; return (listed);};
		BOOL        Listed    ( )           { return    (listed);};

		FLTExport int    FindFilter              ( const TCHAR *name );
		FLTExport DWORD  GetFilterCapabilities   ( const TCHAR *name );

		//-- This Creates   an  Instance    - Make sure to  "delete"    it  after   use.

		FLTExport ImageFilter *CreateFilterInstance(const TCHAR *d);

};

//-----------------------------------------------------------------------------
//-- Undo Notification

class UndoNotify : public TVNodeNotify {
	HWND hWnd;
public:
	UndoNotify (HWND hwnd) {hWnd = hwnd;}
	RefResult NotifyRefChanged	(Interval changeInt, RefTargetHandle hTarget, 
			PartID& partID,  RefMessage message) {
			SendMessage(hWnd,FLT_UNDO,0,0);
			InvalidateRect(hWnd,NULL,FALSE);
			return(REF_SUCCEED);
	}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   ImageFilter Capability Flags
//
// It  is  valid   for a   plug-in to both Filter and  Compositor. If  both flags are
// set, the    user will be able   to  select it from  both the    Filter list and from
// the Compositor  list.   The plug-in will know it is running as  a filter    when
// the foreground  map pointer is  NULL.
//

#define IMGFLT_NONE              0      //  None

#define IMGFLT_MASK              (1<<0) //  Supports Masking
#define IMGFLT_CONTROL           (1<<1) //  Plug-In has a Control Panel
#define IMGFLT_FILTER            (1<<2) //  Plug-In is a Filter
#define IMGFLT_COMPOSITOR        (1<<3) //  Plug-In is a Compositor
#define IMGFLT_THREADED          (1<<4) //  Thread aware plug-in

//-- Class ID's for various DLL's

#define NEGATIVECLASSID 0x4655434A
#define ALPHACLASSID    0x655434A4
#define ADDCLASSID      0x55434A46
#define BLURCLASSID     0x5434A465
#define CROSFADECLASSID 0x434A4655
#define GLOWCLASSID     0x35A46554
#define COOKIECLASSID   0x4A465543
#define WIPECLASSID     0xA4655434
#define FADECLASSID     0x4655434B
#define PDALPHACLASSID  0x655434B4

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Image   Filter Class
//

class ImageFilter {
	
	protected:

		BOOL	interactive;
		HWND	vpSetuphWnd,vphWnd,dlghWnd;

		//-- Bitmap Pointers --------------------------------------------------
		//
		// All filters will have at least a pointer to "srcmap". This is VP's
		// (or any other process') main image pipeline.
		//
		// Composition filters will also receive a second [frgmap] bitmap
		// which should be composited above the main [srcmap] bitmap.
		//
		// If "mskmap" is not NULL, it will contain a pointer to a grayscale
		// image to be used as a mask for the process.
		//
		// 12/06/95 - GG
		//
		// The srcmap (Background) is the Video Post queue bitmap. Use its
		// methods to find out dimmensions (width, height, aspect ratio, etc.)
		// If the queue is using Alpha channel, it will be noted in the bitmap
		// flags (srcmap). The same is true for Z and G buffers. Again, simply
		// use the bitmap methods to access these.
		//
		
		Bitmap                      *srcmap;         //--   Source (Background)
		Bitmap                      *mskmap;         //--   Mask (Grayscale Masking)
		Bitmap                      *frgmap;         //--   Foreground (for layering/transitions)
		
		//-- Set    by  Host ----------------------------------
		
		ImageFilterInfo         *ifi;
		
	public:
	
		FLTExport	ImageFilter     ( );
		dVirtual	~ImageFilter	( );
		
		//-- Filter Info    ---------------------------------
		
		dVirtual     const TCHAR    *Description      ( ) = 0; // ASCII description (i.e. "Convolution Filter")
		dVirtual     const TCHAR    *AuthorName       ( ) = 0; // ASCII Author name
		dVirtual     const TCHAR    *CopyrightMessage ( ) = 0; // ASCII Copyright message
		dVirtual     UINT            Version          ( ) = 0; // Version number * 100 (i.e. v3.01 = 301)
		dVirtual     DWORD           Capability       ( ) = 0; // Returns capability flags (see above)

		//-- Dialogs ----------------------------------------------------------
		//
		//    An About Box  is  mandatory. The  Control panel is optional and   its 
		//    existence should be flagged   by  the Capability  flag above.
		//

		dVirtual     void            ShowAbout         ( HWND    hWnd ) =    0;
		dVirtual     BOOL            ShowControl       ( HWND    hWnd ) {    return FALSE; }

		//-- Parameter  Setting (Host's Responsability) ----

		dVirtual     void            SetSource         ( Bitmap *map )     {srcmap = map;}
		dVirtual     void            SetForeground     ( Bitmap *map )     {frgmap = map;}
		dVirtual     void            SetMask           ( Bitmap *map )     {mskmap = map;}
		dVirtual     void            SetFilterInfo     ( ImageFilterInfo *i ) {ifi   = i;}

		//-- Execution  ------------------------------------
		//
		//    The   "hWnd" argument is a    window handler  to  which
		//    the   plug-in will be sending messages.

		dVirtual     BOOL            Render            ( HWND    hWnd ) =    0;

		//-- Max    Interface ----------------------------------------------------
		//
		//    Some of Max's core functions exported through the Interface class.
		//

		dVirtual  Interface *Max  ( );

		//-- Helpers --------------------------------------

		dVirtual  int     Lerp    (int a, int b, int l);
		dVirtual  int     Lerp    (int a, int b, float f);
		
		//-- Parameter  Block   Load and    Save ------------------------------------
		//
		//   The host will  call EvaluateConfigure() to determine the   buffer size
		//   required by the plug-in.
		//
		//   SaveConfigure() will be called so the  plug-in can transfer    its
		//   parameter block to the host ( ptr is a pre-allocated   buffer).
		//
		//   LoadConfigure() will be called so the  plug-in can load its    
		//   parameter block back.
		//   
		//   Memory management is performed by the  host using standard
		//   LocalAlloc() and   LocalFree().
		//   
		
		dVirtual  DWORD   EvaluateConfigure  ( )           { return 0; }
		dVirtual  BOOL    LoadConfigure      ( void *ptr ) { return (FALSE); }
		dVirtual  BOOL    SaveConfigure      ( void *ptr ) { return (FALSE); }

		//-- Preview Facility -------------------------------------------------
		//
		//    This is used  by  plug-ins    that want to have   a preview bitmap while
		//    displaying its control dialogue.
		//
		//    The   flag controls how   much of the queue   to  run:
		//
		//    PREVIEW_BEFORE - The queue is run up  to  the event before the    filter
		//    calling it.
		//
		//    PREVIEW_UP ----- The queue is run up  to  the event (filter) calling
		//    this function.
		//
		//    PREVIEW_WHOLE -- The whole queue is run   including events after
		//    this filter.
		//
		//    The   given   frame   is  the Video Post  Queue   frame   number and not  Max's
		//    frame number.
		//
		//
		//    Parameters:
		//
		//    hWnd -    WIndow handle to send messages to. These are    the progress,
		//    check for abort, text messages    etc. If the plug in wants to support
		//    a cancel button   and progress bars   etc, it must handle these messages.
		//    It is Ok to send a    NULL window handle in which case    nothing is checked.
		//
		//    back -    Pointer to a Bitmap pointer. If the Bitmap pointer  is  NULL,   a
		//    new   bitmap is created   using   the given dimmensions. This pointer must be
		//    NULL the first time this  function    is  called as the bitmap    must be
		//    created by Video Post. Once   this function is called and a   bitmap is
		//    returned, it  is  ok  to  call it again using this map.   In  this case, Video
		//    Post will simply use it instead of creating a new one.    You must    delete
		//    the   bitmap when done.
		//
		//    fore -    For layer plug-ins, this points to the  foreground image.   This is
		//    only valid if flag    is  set to PREVIEW_BEFORE. In this case back will hold  
		//    Video Post main   queue   and fore    will have the foreground image to be 
		//    composited. This is usefull   if  you, a layer plug-in, want  to  collect the 
		//    images    and run a real  time preview. If flag is not PREVIEW_BEFORE,    fore
		//    will be a NULL pointer indicating there   is  no  bitmap.
		//
		//    frame - The desired frame. Make sure  you request a frame within  the
		//    range your plug-in    is  active.
		//
		//    width & height - Self explanatory.
		//
		//    flag -    Explained above.
		//

		#ifndef PREVIEW_BEFORE
		#define PREVIEW_BEFORE  1
		#define PREVIEW_UP      2
		#define PREVIEW_WHOLE   3
		#endif

		dVirtual    BOOL CreatePreview  ( 
				HWND hWnd,                      //-- Window handle to send  messages    to
				Bitmap **back,                  //-- Pointer to Bitmap Pointer (Background)
				int frame,                      //-- Desired Frame
				int width,                      //-- Desired Width
				int height,                     //-- Desired Height
				float   aspect,                 //-- Desired Aspect Ratio
				Bitmap **fore   = NULL,         //-- Pointer to Bitmap Pointer (Foreground)
				DWORD   flag    = PREVIEW_UP );

		//----------------------------------------------------------------------
		//-- Channels Required
		//
		//    By setting this   flag,   the plug-in can request the host    to  generate
		//    the   given   channels. Prior to Rendering,   the host    will scan the
		//    plug-ins in the   chain   of  events and list all types of channels
		//    being requested. The plug-in, at the  time of the Render()    call,   
		//    will have access to these channels through    the channel interface
		//    described in  Bitmap.h    - BitmapStorage.
		//
		//    The   generation of these channels should not, normally,  be  a 
		//    default setting   for a   plug-in.    These   channels    are memory hungry   and
		//    if the    plug-in won't use   it, it should not   ask for it. Normally
		//    the   plug-in would ask   the user    which   channels    to  use and set only
		//    the   proper flags.
		//
		
		dVirtual     DWORD   ChannelsRequired       ( ) {   return BMM_CHAN_NONE; }
		

		//----------------------------------------------------------------------
		//-- R2 Stuff Below ----------------------------------------------------
		//----------------------------------------------------------------------
	
		TimeChange	timeChange;
		UndoNotify*	undonotify;

		dVirtual HWND	DlgHandle			( void ) { return dlghWnd; }

		//-- Filter Control Dialogue Interactivity -----------------------------

		dVirtual void	MakeDlgInteractive	( HWND hWnd );
		dVirtual BOOL	IsInteractive		( void ) { return interactive; }

		//-- Trackview Node Functions ------------------------------------------

		dVirtual ITrackViewNode *CreateNode ( );
		dVirtual ITrackViewNode *Node ( ) { return ifi->Node(); }

		//-- FilterUpdate() ----------------------------------------------------
		//
		// Whenever a filter instance is created or updated (i.e. the user went,
		// through the Filter Edit Control dialogue) this is call is issued to 
		// the filter. The filter may use it to create/update its node controls.
		//
		// See example in negative.cpp.

		dVirtual void	FilterUpdate	( ) { }

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//--   Main Filter Manager Class
//
//

class FilterManager    {
	
		TCHAR                   name[MAXFILTERNAME];
		FLTInterface            *iface;
		ImageFilterInfo         *ifi;
		Interface               *max;

		//-- General Private    Methods
		
		BOOL                    SetupPlugIn                 ( HWND hWnd, WORD   item );
		void                    HandleMaskFile              ( HWND hWnd, WORD   item );
		
		//-- Image Filter   Private Methods
		
		int                     GetCurrentFilter            ( HWND hWnd, TCHAR *plugin  );
		void                    HandleFilterDialogState     ( HWND hWnd );

	public:
	
		FLTExport                   FilterManager           ( FLTInterface  *i);
		FLTExport                   FilterManager           ( FLTInterface  *i,const    TCHAR   *name);
		FLTExport                   ~FilterManager          ( );
		
		FLTExport FLTInterface  *iFace                      ( ) {   return iface;}
		
		void                        DoConstruct             ( FLTInterface  *i,const    TCHAR   *name);
		
		FLT_FilterList              fltList;
		FLTExport void              ListFilters             ( );
		
		FLTExport HINSTANCE         AppInst                 ( );
		FLTExport HWND              AppWnd                  ( );
		FLTExport DllDir            *AppDllDir              ( );
		FLTExport Interface         *Max                    ( ) {   return max; }
		
		//-- User Interface -------------------------------

		BOOL						ImageFilterControl		( HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam );
		
		//-- This function will create a    mask bitmap based
		//    on the    given   ImageFilterInfo class.
		
		Bitmap                      *ProcessMask                    ( HWND hWnd, ImageFilterInfo    *ii );
		
		//-- This function will list all    available filter 
		//    plug-ins. The "item" argument defines an id 
		//    for   a combo box to  receive the list whithin 
		//    the   hWnd context. It returns the number of  
		//    filters found.
		
		FLTExport int               GetFilterList                   ( HWND hWnd, int item );
		FLTExport int               GetLayerList                    ( HWND hWnd, int item );
		
		//-- This runs  the show. Thew  window handle is used
		//    to send progress messages back. See above the
		//    discussion about messages. The    host should
		//    check keyboard and    cancel buttons  and return
		//    FALSE to a FLT_PROGRESS or FLT_CHECKABORT
		//    message telling   the Plug-In to  cancel.
		
		FLTExport BOOL              RenderFilter     ( HWND hWnd, 
													   ImageFilterInfo *ii, 
													   Bitmap *map,
													   Bitmap *foreMap = NULL);
		
		//-- This will  bring   a full blown dialog giving  the
		//    user an interface to select   and define a plug-
		//    in filter. Returns    FALSE   if  the user    cancels.
		
		FLTExport BOOL              SelectImageFilter( HWND hWnd, ImageFilterInfo *ii    );
		
		//-- This will  fill out    the given combo box with a
		//    list of available mask options
		
		FLTExport void              ListMaskOptions  ( HWND hWnd, int item);

		//----------------------------------------------------------------------
		//-- R2 Stuff Below ----------------------------------------------------
		//----------------------------------------------------------------------
	
		//-- Internal Use

		FLTExport void              UpdateFilter		( ImageFilterInfo *ii );

		
};

//-----------------------------------------------------------------------------
//--   Forward References
//

extern FLTExport   void             OpenFLT         (  FLTInterface *i );
extern FLTExport   void             CloseFLT        (  );

//-----------------------------------------------------------------------------
//--   The Primary Filter Manager  Object
//
// TO  DO: Move    to  App data    structure?

extern FLTExport FilterManager *TheFilterManager; 

#endif

//-- EOF: filters.h -----------------------------------------------------------
