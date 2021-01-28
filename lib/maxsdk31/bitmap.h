//-----------------------------------------------------------------------------
// -------------------
// File ....: bitmap.h
// -------------------
// Author...: Tom Hudson
// Date ....: Sptember 1994
//
// History .: Sep, 01 1994 - Started serious coding
//            Oct, 18 1994 - First major revision for DLLs, restructuring
//            Jul, 10 1995 - Began working with it (Gus J Grubba)
//
//-----------------------------------------------------------------------------
      
#ifndef BITMAP_H_DEFINED
#define BITMAP_H_DEFINED

#define NOAVIFILE
#include <vfw.h>
#undef  NOAVIFILE

#include "palutil.h"
#include "linklist.h"
#include "gbuf.h"

//-- Defines that may change with compiler

#define INTBITS (sizeof(int) * 8)

//-- Class ID's for various DLL's

#define IMGCLASSID      1
#define CYCLECLASSID    2
#define FLICCLASSID     5
#define TARGACLASSID    6
#define YUVCLASSID      7
#define FBCLASSID       8
#define WSDCLASSID      9
#define IFLCLASSID      10
#define BMPCLASSID      11
#define JPEGCLASSID     12
#define TARGAPLSCLASSID 13
#define AVICLASSID      14
#define RLACLASSID      15

//-- local definitions

class BitmapManager;
class BitmapManagerImp;
class BitmapStorage;
class BitmapFilter;
class BitmapDither;
class BitmapInfo;
class BitmapPicker;
class BitmapIO;
class Bitmap;

//-- Temporary definitions to make the compiler happy

class GraphicsWindow;
class BMMInterface;
class DllDir;

//-- External data 

extern int  TheSystemFrame;         // TO DO: Move to App data structure?

//-----------------------------------------------------------------------------
//-- The Primary Bitmap Manager Object
//

extern BMMExport BitmapManager *TheManager; // TO DO: Move to App data structure?

//-- Common Macros ------------------------------------------------------------

//-- Just to make it shorter

#define bVirtual BMMExport virtual

//-- Set up a NULL macro

#ifndef NULL
#define NULL (0)
#endif

//-- Pixel storage classes ----------------------------------------------------
// These are in maxtypes.h now.
//typedef struct {
//   BYTE r,g,b;
//} BMM_Color_24;
//
//typedef struct {
//   BYTE r,g,b,a;
//} BMM_Color_32;
//
//typedef struct {
//   WORD r,g,b;
//} BMM_Color_48;
//
//typedef struct {
//   WORD r,g,b,a;
//} BMM_Color_64;

//-- Generic bitmap information structure -------------------------------------

typedef struct {
   int   width,height;
   float aspect,gamma;
   DWORD flags;
} BMMImageInfo;

//-- Basic bitmap types supported by Bitmap Manager

#define BMM_NO_TYPE              0       // Not allocated yet
#define BMM_LINE_ART             1
#define BMM_PALETTED             2
#define BMM_GRAY_8               3
#define BMM_GRAY_16              4       
#define BMM_TRUE_16              5
#define BMM_TRUE_32              6
#define BMM_TRUE_64              7

//-- Information Only

#define BMM_TRUE_24              8       
#define BMM_TRUE_48              9       
#define BMM_YUV_422              10
#define BMM_BMP_4                11      //-- Windows BMP 16 color bitmap
#define BMM_PAD_24               12      //-- Padded 24 bit (in a 32 bit register)

//-- Textual Limits

#define MAX_DESCRIPTION          256

//-- The number of bitmap formats supported internally

//#define BMM_FORMATS            6

//-- File types

//#define BMM_NOTYPE             0
//#define BMM_TARGA              1       // System Targa I/O driver
//#define BMM_GIF                2       // System GIF I.O driver

//-- Gamma limits

#define MINGAMMA                 0.2f
#define MAXGAMMA                 5.0f

//-- openMode values

#define BMM_NOT_OPEN             0       // Not opened yet
#define BMM_OPEN_R               1       // Read-only
#define BMM_OPEN_W               2       // Write

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Error Codes for BMMRES type functions

typedef unsigned short BMMRES;              // Traps the use of int or BOOL

#define BMMRES_SUCCESS                0
#define BMMRES_ERRORTAKENCARE         1     // Error - Function has already taken action
#define BMMRES_FILENOTFOUND           2
#define BMMRES_MEMORYERROR            3
#define BMMRES_NODRIVER               4     // Device driver responsible for image not present
#define BMMRES_IOERROR                5
#define BMMRES_INVALIDFORMAT          6
#define BMMRES_CORRUPTFILE            7
#define BMMRES_SINGLEFRAME            8     // Goto request on a single frame image
#define BMMRES_INVALIDUSAGE           9     // Bad argument passed to function (Developer Mistake)
#define BMMRES_RETRY                  10    // User selected "Retry" from error dialogue
#define BMMRES_NUMBEREDFILENAMEERROR  11
#define BMMRES_INTERNALERROR          12
#define BMMRES_BADFILEHEADER          13
#define BMMRES_CANTSTORAGE            14
#define BMMRES_BADFRAME               15    // Invalid Frame Number Requested

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- BitmapIO usage types ( returned by BitmapIO::Capability() )

#define BMMIO_NONE                         0   // Not defined yet

#define BMMIO_READER                   (1<<0)  // Reads images
#define BMMIO_WRITER                   (1<<1)  // Writes images
#define BMMIO_EXTENSION                (1<<2)  // Uses file extension (File Filter Type)
#define BMMIO_MULTIFRAME               (1<<3)  // "File" contains multiple frames (i.e. FLC, AVI)

#define BMMIO_THREADED                 (1<<4)  // Thread aware plug-in
#define BMMIO_RANDOM_ACCESS            (1<<5)  // Can read and/or write frames in any order
#define BMMIO_NON_CONCURRENT_ACCESS    (1<<6)  // Cannot handle multiple, concurrent requests (FLC, AVI, VTR's, etc)

#define BMMIO_OWN_VIEWER               (1<<7)  // Driver has its own image viewer for its image type.

//-- Frame Buffers

#define BMMIO_FRAMEBUFFER              (1<<7)  // Frame Buffer Driver
#define BMMIO_GRABBER                  (1<<8)  // Device Grabs Video

//-- If the device is able to show its own image info dialogue this flag will be 
//   set. Otherwise, the host must use BitmapManager::GetImageInfo() and display
//   a generic info dialogue.

#define BMMIO_INFODLG            		(1<<9) // Has Info Dialog

//-- Uninterruptible Driver (AVI, FLIC, etc. cannot stop and go) (GG 02/26/97)

#define BMMIO_UNINTERRUPTIBLE			(1<<10)

//-- Drivers that may have a different image for a same given Max frame and same file/device
//   name should define this and implement the EvalMatch() method.

#define BMMIO_EVALMATCH				(1<<11)

//-- Special IFL device

#define BMMIO_IFL                		(1<<28)

//-- There is only a single call to the plug-in's control panel but the call specifies
//   the nature of the operation going on. It's up to the plug-in to provide different
//   interfaces if needed. If one control serves two or more services, all the pertinent
//   flags should be set.

#define BMMIO_CONTROLREAD        (1<<29) // Device Driver has Control Panel for Read Operations
#define BMMIO_CONTROLWRITE       (1<<30) // Device Driver has Control Panel for Write Operations
#define BMMIO_CONTROLGENERIC     (1<<31) // Device Driver has a generic Control Panel

//-----------------------------------------------------------------------------
//-- Bitmap close options

#define BMM_CLOSE_COMPLETE       0
#define BMM_CLOSE_ABANDON        1

//-- Filter Types

#define BMM_FILTER_NONE          0
#define BMM_FILTER_DUMMY         1
#define BMM_FILTER_SUM           2
#define BMM_FILTER_PYRAMID       3

//-- Filter Flag values

#define BMM_FILTER_NOFLAGS       ((DWORD)0)
#define BMM_FILTER_LERP_WRAP     ((DWORD)(1<<0))

//-- Dither Types

#define BMM_DITHER_NONE          0
#define BMM_DITHER_FLOYD         1

//-- Pyramidal filter information

#define MAX_PYRAMID_DEPTH        12
#define LAYER_DIM                MAX_PYRAMID_DEPTH+1

typedef struct {
   WORD  dmax;
   void  *map[LAYER_DIM];
   void  *alpha[LAYER_DIM]; 
} BMM_Pyramid;

//-- Summed-area table information

typedef struct {
   DWORD *sat_r,*sat_g,*sat_b,*sat_a;
} BMM_SAT;

//-- Bitmap::CopyImage options

#define COPY_IMAGE_CROP                 0                   // Copy image to current map size w/cropping if necessary
#define COPY_IMAGE_RESIZE_LO_QUALITY    1                   // Resize source image to destination map size (draft)
#define COPY_IMAGE_RESIZE_HI_QUALITY    2                   // Resize source image to destination map size (final)
#define COPY_IMAGE_USE_CUSTOM           3                   // Based on Image Input Options (BitmapInfo *)

// Class for storing a linked list of file extension strings
MakeLinkedList(TSTR);

//-- GRAINSTART
//-- Callback for Bitmap Effects
typedef BOOL (WINAPI *PBITMAP_FX_CALLBACK)(
    LPVOID lpparam, int done, int total, TCHAR *msg
    );
//-- GRAINEND

//-----------------------------------------------------------------------------
//-- I/O Handler
//
   
class BMM_IOHandler {

      //-- Name and Capabilities ------------------------
      
      TCHAR      ioShortDesc[MAX_DESCRIPTION]; 
      TCHAR      ioLongDesc[MAX_DESCRIPTION];
      DWORD      ioCapabilities;

      //-- Extension for file types
      
      TSTRList   ioExtension;
      
      //-- DLL Handler ----------------------------------
      
      ClassDesc *cd;
	  int		dllNumber;
      
   public:

      BMMExport BMM_IOHandler();
      
      BMMExport TCHAR           *ShortDescription ( const TCHAR  *d = NULL );
      BMMExport TCHAR           *LongDescription  ( const TCHAR  *d = NULL );
      BMMExport TCHAR           *Extension        ( int index, const TCHAR  *e = NULL );
      BMMExport int              NumExtensions    ( )                { return ioExtension.Count(); }

      BMMExport void             SetCD            ( ClassDesc *dll ) { cd = dll;};
      BMMExport ClassDesc       *GetCD            ( )                { return cd;};
      BMMExport void             SetDllNumber     ( int num )        { dllNumber = num; }
      BMMExport int              GetDllNum        ( )                { return dllNumber; }

      BMMExport void             SetCapabilities  ( DWORD cap )      { ioCapabilities |= cap;};
      BMMExport DWORD            GetCapabilities  ( )                { return (ioCapabilities);};
      BMMExport BOOL             TestCapabilities ( DWORD cap )      { return ((ioCapabilities & cap) != 0);};
      BMMExport BMM_IOHandler&   operator=(BMM_IOHandler &from);
};

//-----------------------------------------------------------------------------
//-- List of I/O Handlers
//

// Class for storing a linked list of Bitmap Manager BMM_IOHandler objects
MakeLinkedList(BMM_IOHandler);
   
class BMM_IOList: public BMM_IOHandlerList {

      BOOL     listed;
       
   public:

      BMM_IOList          ( )                { listed = FALSE; }

      BOOL     Listed     ( BOOL f)          { listed = f; return (listed);};
      BOOL     Listed     ( )                { return (listed);};

      BMMExport int   FindDevice             ( const TCHAR *name);
      BMMExport int   FindDeviceFromFilename ( const TCHAR *name);
      BMMExport int   ResolveDevice          ( BitmapInfo *bi   );
      BMMExport DWORD GetDeviceCapabilities  ( const TCHAR *name);

      //-- This Creates an Instance - Make sure to "delete" it after use.

      BMMExport BitmapIO *CreateDevInstance( const TCHAR *d );
      BMMExport BitmapIO *CreateDevInstance( int idx );

};



//-----------------------------------------------------------------------------
//-- RenderInfo Class
//

enum ProjectionType { ProjPerspective=0, ProjParallel=1 };

class RenderInfo {
   public:
   RenderInfo();
   ProjectionType projType;  
   float kx,ky;            // 3D to 2D projection scale factor 
   float xc,yc;           // screen origin
   BOOL fieldRender;    // field rendered?
   BOOL fieldOdd;         // if true, the first field is Odd lines
   // Render time and tranformations for the 2 fields, if field rendering. 
   // If not, use renderTime[0], etc.
   TimeValue renderTime[2]; 
   Matrix3 worldToCam[2];     
   Matrix3 camToWorld[2];
   BMMExport Point2 MapWorldToScreen(Point3 p, int field=0) { return MapCamToScreen(worldToCam[field]*p);}
   BMMExport Point2 MapCamToScreen(Point3 p);  // map point in camera space to screen
   BMMExport Ray MapScreenToCamRay(Point2 p);  // get viewing ray through screen point, in camera space
   BMMExport Ray MapScreenToWorldRay(Point2 p, int field=0); // get viewing ray through screen point, in world space
   };

//-----------------------------------------------------------------------------
//-- Bitmap Info Class
//

//-- Bitmap flags

#define MAP_NOFLAGS              ((DWORD)0)
#define MAP_READY                ((DWORD)(1<<0))
#define MAP_HAS_ALPHA            ((DWORD)(1<<1))
#define MAP_ALPHA_PREMULTIPLIED  ((DWORD)(1<<2))
#define MAP_PALETTED             ((DWORD)(1<<3))
#define MAP_FRAME_SYSTEM_LOCKED  ((DWORD)(1<<4))
#define MAP_DITHERED             ((DWORD)(1<<5))
#define MAP_FLIPPED              ((DWORD)(1<<6))     // Flipped horizontally
#define MAP_INVERTED             ((DWORD)(1<<7))     // Flipped vertically

//#define MAP_CUSTOMSIZE           ((DWORD)(1<<8))     // Custom size for input
//#define MAP_RESIZE               ((DWORD)(1<<9))     // Resize when input

#ifdef DESIGN_VER
#define MAP_HAS_BGIMAGE          ((DWORD)(1<<29))    // internal use only
#endif
#define MAP_LEGAL_DELETE	     ((DWORD)(1<<30))    // internal use only
#define MAP_VIEW_FILTERED        ((DWORD)(1<<31))    // Test stuff

#define MAP_ALL_FLAGS            0xFFFFFFFF

//-----------------------------------------------------------------------------
//-- Messages sent back by various (client) methods

//-- Sent by the plug-in to notify host of current progress. The host should
//   return TRUE if it's ok to continue or FALSE to abort process.

#define BMM_PROGRESS   WM_USER + 0x120   //-- wParam: Current lParam: Total

//-- Sent by the plug-in to check for process interruption. The host should
//   return FALSE (by setting lParam) if it's ok to continue or TRUE to abort 
//   process.

#define BMM_CHECKABORT WM_USER + 0x121   //-- wParam: 0       lParam: *BOOL

//-- Sent by the plug-in to display an optional textual message (for progress
//   report).

#define BMM_TEXTMSG    WM_USER + 0x122   //-- wParam: 0       lParam: LPCTSTR

class BitmapInfo {
   
		//-- Image name in case of named images such as files -------

		TCHAR                    name[MAX_PATH];

		//-- Device name gotten from BMM_IOHandler::LongDescription() 

		TCHAR                    device[MAX_DESCRIPTION];

		//-- Window Handle to send BMM_UPDATE messages --------------

		HWND                     hWnd;

		//-- Plug-In Parameter Block --------------------------------

		void                    *pidata;
		DWORD                    pisize;

		//-- Basic Image Data ---------------------------------------
		//
		//   When reading an image, or asking for image info, these
		//   fields will tell the user what the image is like.
		//
		//   When creating an image the user will set these fields to
		//   the desired parameters.
		//

		WORD                     width,height;
		float                    aspect,gamma;
		int                      fstart,fend;
		DWORD                    flags,type;

		//-- User data (what user wants) ----------------------------
		//
		//   Custom dimmensions, custom start and end point when
		//   reading sequence of imges, frame to fetch/save, etc.
		//

		WORD                     cwidth,cheight;   
		int                      custxpos,custypos;
		int                      start,end;        
		int                      step,preset_al;
		float                    custgamma;        

		#define BMM_UNDEF_FRAME   0x7FFF0000


		#define BMM_CUSTOM_GAMMA        ((DWORD)(1 << 0))
		#define BMM_CUSTOM_SIZE         ((DWORD)(1 << 1))
		#define BMM_CUSTOM_RESFIT       ((DWORD)(1 << 2))
		#define BMM_CUSTOM_POS          ((DWORD)(1 << 3))
		#define BMM_CUSTOM_FILEGAMMA    ((DWORD)(1 << 4))
		#define BMM_CUSTOM_IFLENUMFILES ((DWORD)(1 << 5))

		#define BMM_CUSTOM_POSNW  0
		#define BMM_CUSTOM_POSN   1
		#define BMM_CUSTOM_POSNE  2
		#define BMM_CUSTOM_POSW   3
		#define BMM_CUSTOM_POSCN  4
		#define BMM_CUSTOM_POSE   5
		#define BMM_CUSTOM_POSSW  6
		#define BMM_CUSTOM_POSS   7
		#define BMM_CUSTOM_POSSE  8

		DWORD                    customflags;

		int                      fnumber;           //-- Defines frame to
                                                  //   read or write.

		//-- When reading a sequence of frames, loopflag indicates what to
		//   do when reading beyond the end of available frames. It 
		//   defaults to BMM_SEQ_WRAP.

		#define  BMM_SEQ_WRAP     0                  //-- Wraps around back to start point
		#define  BMM_SEQ_ERROR    1                  //-- Generates an error
		#define  BMM_SEQ_HOLD     2                  //-- Holds last frame when done
		        
		WORD                     loopflag;         

		void                     doConstruct                 ( );
      
   public:

		BMMExport                BitmapInfo                  ( );
		BMMExport                BitmapInfo                  ( TCHAR *n );
		BMMExport                BitmapInfo                  ( BitmapInfo &bi );
		BMMExport               ~BitmapInfo                  ( );

		//-- Bitmap Flags

		BMMExport DWORD          Flags                       ( ) { return (flags); }
		BMMExport DWORD          SetFlags                    ( DWORD f ) { flags |=  f; return (flags); }
		BMMExport DWORD          ResetFlags                  ( DWORD f ) { flags &= ~f; return (flags); }
		BMMExport BOOL           TestFlags                   ( DWORD f ) { return (flags & f); }

		//-- Generic Read

		BMMExport WORD           Width                       ( ) { return (width); }
		BMMExport WORD           Height                      ( ) { return (height); }
		BMMExport float          Gamma                       ( ) { return (gamma); }
		BMMExport float          Aspect                      ( ) { return (aspect);}
		BMMExport int            Type                        ( ) { return (type); }
		BMMExport int            FirstFrame                  ( ) { return (fstart); }
		BMMExport int            LastFrame                   ( ) { return (fend); }
		BMMExport int            NumberFrames                ( ) { return (fend - fstart + 1); }
		BMMExport int            CurrentFrame                ( ) { return (fnumber); }
		BMMExport WORD           SequenceOutBound            ( ) { return (loopflag); }

		//-- "Name" returns full path of image file

		BMMExport const TCHAR   *Name                        ( ) { return (const TCHAR *)name;   }

		//-- "Filename" returns just the name of image file

		BMMExport const TCHAR   *Filename                    ( );

		//-- "Device" is the device reponsible for producing this image.
		//   For file types, this is just informative. For non file types
		//   this is the way this image is identified. Therefore, it is
		//   important to save both name and device in order to properly
		//   identify an image.

		BMMExport const TCHAR   *Device                      ( ) { return (const TCHAR *)device; }

		//-- Compare Two Bitmaps

		BMMExport BOOL           CompareName                 ( BitmapInfo *bi );

		//-- Copy Image info. Only name, device and image characteristics are
		//   copied. User info, such as Custom Width, etc. is not copied. 

		BMMExport void           CopyImageInfo               ( BitmapInfo *from );

		//-- Generic Write

		BMMExport int            SetFirstFrame               ( int m ) { int o = fstart;   fstart   = m; return (o);}
		BMMExport int            SetLastFrame                ( int s ) { int o = fend;     fend     = s; return (o);}
		BMMExport int            SetCurrentFrame             ( int v ) { int o = fnumber;  fnumber  = v; return (o);}
		BMMExport WORD           SetSequenceOutBound         ( WORD  c ) { WORD  o = loopflag; loopflag = c; return (o);}

		BMMExport WORD           SetWidth                    ( WORD  s ) { WORD  o = width;    width    = s; return (o);}
		BMMExport WORD           SetHeight                   ( WORD  u ) { WORD  o = height;   height   = u; return (o);}
		BMMExport float          SetGamma                    ( float c ) { float o = gamma;    gamma    = c; return (o);}
		BMMExport float          SetAspect                   ( float k ) { float o = aspect;   aspect   = k; return (o);}
		BMMExport int            SetType                     ( int   s ) { int   o = type;     type     = s; return (o);}

		BMMExport const TCHAR   *SetName                     ( const TCHAR *n );
		BMMExport const TCHAR   *SetDevice                   ( const TCHAR *d );

		//-- Custom Input Processing

		BMMExport WORD           CustWidth                   ( ) { return (cwidth);        }
		BMMExport WORD           CustHeight                  ( ) { return (cheight);       }
		BMMExport void           SetCustWidth                ( WORD w ) { cwidth  = w;     }
		BMMExport void           SetCustHeight               ( WORD h ) { cheight = h;     }
		BMMExport int            StartFrame                  ( ) { return (start);         }
		BMMExport int            EndFrame                    ( ) { return (end);           }
		BMMExport void           SetStartFrame               ( int s )  { start = s;      }
		BMMExport void           SetEndFrame                 ( int e )  { end   = e;      }
		BMMExport void           SetCustomX                  ( int x ) { custxpos = x;    }
		BMMExport void           SetCustomY                  ( int y ) { custypos = y;    }
		BMMExport int            GetCustomX                  ( ) { return custxpos;        }
		BMMExport int            GetCustomY                  ( ) { return custypos;        }
		BMMExport void           SetCustomGamma              ( float g ) { custgamma = g;  }
		BMMExport float          GetCustomGamma              ( ) { return custgamma;       }
		BMMExport void           SetCustomStep               ( int s ) { step = s;         }
		BMMExport int            GetCustomStep               ( ) { return step;            }
		BMMExport void           SetPresetAlignment          ( int p ) { preset_al = p;    }
		BMMExport int            GetPresetAlignment          ( ) { return preset_al;       }

		//-- Custom Input Flags

		BMMExport DWORD          GetCustomFlags              ( ) { return (customflags);             }
		BMMExport void           SetCustomFlag               ( DWORD f ) { customflags |=  f;        }
		BMMExport void           ResetCustomFlag             ( DWORD f ) { customflags &= ~f;        }
		BMMExport BOOL           TestCustomFlags             ( DWORD f ) { return (customflags & f); }

		//-- Plug-In Parameter Block

		BMMExport void*			GetPiData                   ( ) { return pidata;          }
		BMMExport void          SetPiData                   ( void *ptr ) { pidata = ptr; }
		BMMExport DWORD         GetPiDataSize               ( ) { return pisize;          }
		BMMExport void          SetPiDataSize               ( DWORD s ) { pisize = s;     }
		BMMExport void          ResetPiData                 ( );
		BMMExport BOOL          AllocPiData                 ( DWORD size );

		//-- Used to create Format Specific Parameter Block. Name and/or Device must be defined before using it.

		BMMExport void*			CreateFmtSpecBlock			( void );

		BMMExport void          Copy                        ( BitmapInfo *from ); //\\-- OBSOLETE --\\//
		BMMExport BitmapInfo    &operator=                  ( BitmapInfo &from );

		//-- Load/Save

		BMMExport IOResult       Save                        ( ISave *isave );
		BMMExport IOResult       Load                        ( ILoad *iload );
		BMMExport void           EnumAuxFiles                ( NameEnumCallback& nameEnum, DWORD flags);
		  
		//-- Miscelaneous

		BMMExport BOOL           Validate                    ( );
		BMMExport HWND           GetUpdateWindow             ( )           { return hWnd; }
		BMMExport void           SetUpdateWindow             ( HWND hwnd ) { hWnd = hwnd; }
		BMMExport DWORD          GetGChannels                ( );
		BMMExport DWORD          GetDeviceFlags              ( );

};

//-----------------------------------------------------------------------------
//-- Bitmap I/O Class
//
//   None of these methods are to be used directly. Use the BitmapManager for
//   any image I/O.
//


class BitmapIO {
    private:
      UWORD* outputGammaTab;   // this may be owned by gammaMgr
      UWORD* privGammaTab;     // private gamma table owned by the BitmapIO.
           
   protected:
      float                   gamma;     
      Bitmap                  *map;                        // The bitmap using this OUTPUT handler
      BitmapStorage           *storage;                    // The storage used by this INPUT handler

      int                      openMode;                   // See above
      
      //-- Linked list pointers for multiple output of a single bitmap

      BitmapIO                *prevIO;                     
      BitmapIO                *nextIO;

   public:

      // Used by the subclassed BitmapIO's to get pixels for output with
      // the appropriate output gamma correction.
      BMMExport  int           GetOutputPixels          ( int x,int y,int pixels,BMM_Color_64  *ptr, BOOL preMultAlpha=TRUE);
      
      // Used by the subclassed BitmapIO's to get 32 bit pixels for output with
      // the appropriate output gamma correction and dither. 
      BMMExport  int           GetDitheredOutputPixels  ( int x,int y,int pixels,BMM_Color_32  *ptr, BOOL preMultAlpha=TRUE);

      // Used by the subclassed BitmapIO's to get a DIB  for output with
      // the appropriate output gamma correction. 
      BMMExport  PBITMAPINFO   GetOutputDib             ( int depth = 24   );

      // Used by the subclassed BitmapIO's to get a DIB  for output with
      // the appropriate output gamma correction and dither
      BMMExport  PBITMAPINFO   GetDitheredOutputDib      ( int depth = 24   );

      BMMExport  float         OutputGamma();

      // If a BitmapIO wants to do its own dithering, it should call
      // these to find out if dithering is wanted.  If it is a 24 bit or
      // 32 bit format, it would usually just call GetDitheredOutputPixels instead.
      BMMExport  BOOL          DitherTrueColor();
      BMMExport  BOOL          DitherPaletted();

      // Calculate a color palette for output color packing: gamma corrects
      BMMExport  int           CalcOutputPalette(int palsize, BMM_Color_48 *pal);
   
      BMMExport                BitmapIO                    ( );
      bVirtual                ~BitmapIO                    ( );
      
      BitmapInfo               bi;
   
      inline    int            OpenMode                    ( ) { return (openMode); }
      inline    void           SetPrev                     ( BitmapIO *prev) { prevIO = prev; };
      inline    void           SetNext                     ( BitmapIO *next) { nextIO = next; };
      inline    BitmapIO      *Prev                        ( ) { return prevIO; };
      inline    BitmapIO      *Next                        ( ) { return nextIO; };
      
      BMMExport BitmapStorage *Storage                     ( );
      inline    Bitmap        *Map                         ( ) { return map; };

      bVirtual  int            ExtCount                    ( ) = 0;                                // Number of extemsions supported
      bVirtual  const TCHAR   *Ext                         ( int n ) = 0;                          // Extension #n (i.e. "3DS")
      bVirtual  const TCHAR   *LongDesc                    ( ) = 0;                                // Long ASCII description (i.e. "Targa 2.0 Image File")
      bVirtual  const TCHAR   *ShortDesc                   ( ) = 0;                                // Short ASCII description (i.e. "Targa")
      bVirtual  const TCHAR   *AuthorName                  ( ) = 0;                                // ASCII Author name
      bVirtual  const TCHAR   *CopyrightMessage            ( ) = 0;                                // ASCII Copyright message
      bVirtual  UINT           Version                     ( ) = 0;                                // Version number * 100 (i.e. v3.01 = 301)
      
      bVirtual  int            Capability                  ( ) = 0;                                // Returns IO module ability flags (see above)
      bVirtual  void           ShowAbout                   ( HWND hWnd ) = 0;                      // Show DLL's "About..." box

      //-- If the BMMIO_OWN_VIEWER flag is set, this method will be called
      //   whenever the user wants to view an image for this device. This
      //   is for devices which can "play" image sequences such as AVI's, FLIC's, etc.
      //-- TH 2/26/96 -- Added BOOL return to indicate if view worked.  If it didn't,
      //   it returns FALSE and the caller can view by the normal mechanism.

      bVirtual  BOOL           ShowImage                   ( HWND hWnd, BitmapInfo *bi ) { return FALSE; }

      //-- Show DLL's Control Panel
      //
      //   If the user exists through an Ok, this function will return TRUE.
      //   If the user cancels out, it will return FALSE. False indicates
      //   nothing has changed so the system won't bother asking the plug-in
      //   if it wants to save data.
      //
      //   This function is only called if the plug-in has defined it supports
      //   it (through the Capability  flag above).  The flag will indicate to
      //   the plug-in what operation is this control for (read, write, or
      //   generic).
      //
      
      bVirtual  BOOL           ShowControl                 ( HWND hWnd, DWORD flag ) { return FALSE; }
      
      //-- Parameter Block Load and Save ------------------------------------
      //
      //  The host will call EvaluateConfigure() to determine the buffer size
      //  required by the plug-in.
      //
      //  SaveConfigure() will be called so the plug-in can transfer its
      //  parameter block to the host ( ptr is a pre-allocated buffer).
      //
      //  LoadConfigure() will be called so the plug-in can load its 
      //  parameter block back.
      //  
      //  Memory management is performed by the host using standard
      //  LocalAlloc() and LocalFree().
      //  
      
      bVirtual  DWORD          EvaluateConfigure           ( ) = 0;
      bVirtual  BOOL           LoadConfigure               ( void *ptr ) = 0;
      bVirtual  BOOL           SaveConfigure               ( void *ptr ) = 0;

      //-- Used internaly to make sure current block belongs to Plug-In
      
      bVirtual  BOOL           ValidatePiData              ( BitmapInfo *bi );

      //-- System Interface
      
      BMMExport BOOL           SilentMode                  ( );


      //-- Calculate Desired Frame
      //
      //   This is for multiframe sequences. It processes the desired frame
      //   based on user options. It is used at the Load() function to find
      //   out which frame to load.
      //
      //   "fbi"    is the one passed to Load()
      //   "frame"  is a pointer to an integer to receive the frame number
      
      BMMExport BMMRES         GetFrame                    ( BitmapInfo *fbi, int *frame);
      
      //-- Critical Error Handling
      
      BMMExport BMMRES         ProcessImageIOError         ( BitmapInfo *bi, TCHAR *string = NULL);
      BMMExport BMMRES         ProcessImageIOError         ( BitmapInfo *bi, int errorcode);
      
      //---------------------------------------------------------------------
      //-- Channels Required (for Output)
      //
      //   By setting this flag, the plug-in can request the host to generate
      //   the given channels. Prior to Rendering, the host will scan the
      //   plug-ins in the chain of events and list all types of channels
      //   being requested. The plug-in, at the time of the Write() call, will
      //   have access to these channels through the channel interface
      //   described below in BitmapStorage().
      //
      //   The generation of these channels should not, normally, be a 
      //   default setting for a plug-in. These channels are memory hungry and
      //   if the plug-in won't use it, it should not ask for it. Normally
      //   the plug-in would ask the user which channels to save and set only
      //   the proper flags.
      //
      
      bVirtual  DWORD          ChannelsRequired            ( ) { return BMM_CHAN_NONE; }
      
      //-- Image Info
      
      bVirtual  BMMRES         GetImageInfoDlg             ( HWND hWnd, BitmapInfo *bi, const TCHAR *filename = NULL ) {return BMMRES_NODRIVER;}
      bVirtual  BMMRES         GetImageInfo                (            BitmapInfo *bi ) = 0;
      
      //-- Image File Loaders (IFL handlers)
      
      bVirtual  BMMRES         GetImageName                ( BitmapInfo *bi, TCHAR *filename) {filename[0]=0; return (BMMRES_SUCCESS);}

      //-- Image I/O (Not to use directly)
      
      bVirtual  BitmapStorage *Load                        ( BitmapInfo *bi, Bitmap *map, BMMRES *status ) = 0;      

      bVirtual  BMMRES         OpenOutput                  ( BitmapInfo *bi, Bitmap *map );
      bVirtual  BMMRES         Write                       ( int frame );
      bVirtual  int            Close                       ( int flag );
      bVirtual  PAVIFILE       GetPaviFile                 ( ) { return NULL; }

      // used internally to build output gamma table

      BMMExport void			InitOutputGammaTable(BitmapInfo*bi);

      //-- Evaluate Matching Frame (R2)

      bVirtual  void			EvalMatch		            ( TCHAR *matchString ) { matchString[0] = 0; }




};

//-----------------------------------------------------------------------------
//-- Bitmap Storage Class
//
//   None of these methods are to be used directly. Use the Bitmap class for
//   any image read/write.
//

//-- Channel Operations (for Get/Put16Channel)

#define BMM_CHANNEL_RED          0    //-- Get/Put only Red
#define BMM_CHANNEL_GREEN        1    //-- Get/Put only Green
#define BMM_CHANNEL_BLUE         3    //-- Get/Put only Blue
#define BMM_CHANNEL_ALPHA        4    //-- Get/Put only Alpha
#define BMM_CHANNEL_Z            5    //-- Get/Put only Z
#define BMM_CHANNEL_LUMINANCE    6    //-- Get (R+G+B)/3

class BitmapStorage {
#ifdef DESIGN_VER
   friend class GcsBitmap;
#endif
   protected:
   
      int                      openMode;                   // See above
      UINT                     usageCount;                 // Number of Bitmaps using this storage
      BitmapManager           *manager;

      int                      flags;
      int                      type;                       // See "Basic bitmap types", below

      BMM_Color_48             palette[256];               // 256 palette entries max
      int                      paletteSlots;
      UWORD                    *gammaTable;               // Gamma correction table
      
      RenderInfo               *rendInfo;
	  GBuffer                  *gbuffer;
   public:

      BMMExport                BitmapStorage               ( );
      bVirtual                ~BitmapStorage               ( );

      BitmapInfo               bi;
	   TCHAR*					evalString;

      // gamma 
      BMMExport   float        SetGamma(float gam);         
      inline      int          HasGamma                    ( ) { return (gammaTable!=NULL)          ? 1:0; };
      BMMExport   void         SetHasGamma(BOOL onOff);   
      void               UpdateGammaTable(); 
      BMMExport   UWORD       *GetInputGammaTable();

      inline  BitmapManager   *Manager                     ( ) { return manager;     }
      inline  int              OpenMode                    ( ) { return openMode;    }
      inline  int              Width                       ( ) { return bi.Width();  }
      inline  int              Height                      ( ) { return bi.Height(); }
      inline  float            Aspect                      ( ) { return bi.Aspect(); }
      inline  float            Gamma                       ( ) { return bi.Gamma();  }
      
      inline  int              Paletted                    ( ) { return (flags & MAP_PALETTED)            ? paletteSlots:0; }
      inline  int              IsDithered                  ( ) { return (flags & MAP_DITHERED)            ? 1:0; };
      inline  int              PreMultipliedAlpha          ( ) { return (flags & MAP_ALPHA_PREMULTIPLIED) ? 1:0; };
      inline  int              HasAlpha                    ( ) { return (flags & MAP_HAS_ALPHA)           ? 1:0; };
      
      inline  int              UsageCount                  ( ) { return usageCount; };
      inline  int              Type                        ( ) { return type; };
      inline  int              Flags                       ( ) { return flags; };
      inline  void             SetFlags                    ( DWORD f ) { flags |=  f; }

      bVirtual int             MaxRGBLevel                 ( ) = 0;
      bVirtual int             MaxAlphaLevel               ( ) = 0;
      
      //-- Scaling Tools

      bVirtual void           Scale       	               ( WORD *, int, WORD *, int );
      bVirtual BOOL           GetSRow     	               ( WORD *, int, WORD *, int );
      bVirtual BOOL           PutSRow     	               ( WORD *, int, WORD *, int );
      bVirtual BOOL           GetSCol     	               ( WORD *, WORD *, int, int );
      bVirtual BOOL           PutSCol     	               ( WORD *, WORD *, int, int );
      bVirtual BOOL           ScaleY      	               ( Bitmap *, BMM_Color_64 *, WORD *, WORD *, HWND, int cw = 0, int ch = 0 );
      bVirtual BOOL           ScaleX      	               ( Bitmap *, BMM_Color_64 *, WORD *, WORD *, HWND, int cw = 0, int ch = 0 );
      bVirtual int            StraightCopy                 ( Bitmap *from );

      bVirtual void           *GetStoragePtr               ( int *type ) { *type = BMM_NO_TYPE; return (NULL); }
      bVirtual void           *GetAlphaPtr                 ( int *type ) { *type = BMM_NO_TYPE; return (NULL); }

      //-- These are the standard methods for accessing image pixels
      
      bVirtual int             Get16Gray                   ( int x,int y,int pixels,WORD  *ptr) = 0;
      bVirtual int             Put16Gray                   ( int x,int y,int pixels,WORD  *ptr) = 0;
      bVirtual int             GetLinearPixels             ( int x,int y,int pixels,BMM_Color_64  *ptr) = 0;
      bVirtual int             GetPixels                   ( int x,int y,int pixels,BMM_Color_64  *ptr) = 0;
      bVirtual int             PutPixels                   ( int x,int y,int pixels,BMM_Color_64  *ptr) = 0;
      bVirtual int             GetIndexPixels              ( int x,int y,int pixels,unsigned char *ptr) = 0;
      bVirtual int             PutIndexPixels              ( int x,int y,int pixels,unsigned char *ptr) = 0;
      
      bVirtual int             CropImage                   ( int width,int height,BMM_Color_64 fillcolor) = 0;
      bVirtual int             CropImage                   ( int width,int height,int fillindex)  = 0;
      bVirtual int             ResizeImage                 ( int width,int height,int newpalette) = 0;

      bVirtual int             CopyCrop                    ( Bitmap *from, BMM_Color_64 fillcolor );
      bVirtual int             CopyScaleLow                ( Bitmap *from );
      bVirtual int             CopyScaleHigh               ( Bitmap *from, HWND hWnd, BMM_Color_64 **buf = NULL, int w=0, int h=0 );

      bVirtual int             CopyImage                   ( Bitmap *from,int operation,BMM_Color_64 fillcolor, BitmapInfo *bi = NULL);
      bVirtual int             CopyImage                   ( Bitmap *from,int operation,int fillindex);
      bVirtual int             GetPalette                  ( int start,int count,BMM_Color_48 *ptr) = 0;
      bVirtual int             SetPalette                  ( int start,int count,BMM_Color_48 *ptr) = 0;
      bVirtual int             GetFiltered                 ( float u,float v,float du,float dv,BMM_Color_64 *ptr) = 0;
      
      //-- User Interface
      
      bVirtual int             Allocate                    ( BitmapInfo *bi,BitmapManager *manager,int openMode)  = 0;
      bVirtual int             Connect                     ( ) = 0;
      bVirtual int             Disconnect                  ( ) = 0;
      bVirtual int             MapReady                    ( ) = 0;
      bVirtual int             ClosestColor                ( BMM_Color_48 color);
      bVirtual int             ClosestColor                ( int r,int g,int b);

      // GBuffer methods ----------------------
      // get a pointer to specified channel: also determine its type for check
      bVirtual void*    GetChannel(ULONG channelID, ULONG& chanType) { return gbuffer?gbuffer->GetChannel(channelID, chanType):NULL;}

      GBuffer *GetGBuffer()  { return gbuffer; } 

      // create the specified channels -- return channels present: (creates GBuffer if non-existent); 
      bVirtual ULONG    CreateChannels(ULONG channelIDs);
      
      // delete all the channels in channelIDs
      bVirtual void     DeleteChannels(ULONG channelIDs) { if (gbuffer) gbuffer->DeleteChannels(channelIDs); }  

      // query which channels are present
      bVirtual ULONG     ChannelsPresent() { return gbuffer?gbuffer->ChannelsPresent():0;  }


      // For output bitmaps, can get RenderInfo, which is written by the
      // renderer
      // AllocRenderInfo will alloc only if RenderInfo doesn't yet exist.
      BMMExport RenderInfo* AllocRenderInfo(); 
      // GetRenderInfo just hands back RenderInfo pointer
      BMMExport RenderInfo* GetRenderInfo();

};

//-----------------------------------------------------------------------------
//-- Bitmap Filter Class
//
//   Private class not to be documented
//

class BitmapFilter {
   
   protected:
   
      UINT                     usageCount;                 // Number of Bitmaps using this storage
      BitmapManager           *manager;                    // Pointer to bitmap manager
      BitmapStorage           *storage;                    // Pointer to storage itself
      DWORD                    flags;                      // Filter flags
      int                      dirty;                      // Needs updating flag
      UINT                     type;                       // Type index of filter
      
   public:
   
      BMMExport                BitmapFilter();
      bVirtual                ~BitmapFilter();
      
      inline    DWORD          Flags                       ( ) { return flags; };
      inline    void           SetFlag                     ( DWORD flag) { flags |= flag; dirty = 1; };
      inline    void           ToggleFlag                  ( DWORD flag) { flags ^= flag; dirty = 1; };
      inline    void           ClearFlag                   ( DWORD flag) { flags &= (~flag); dirty = 1; };
      inline    UINT           Type                        ( ) { return type; };
      inline    void           SetType                     ( UINT t) { type = t; };
      BMMExport int            Initialize                  ( BitmapManager *m,BitmapStorage *s);

      virtual   int            GetFiltered                 ( float u,float v,float du,float dv,BMM_Color_64 *ptr) = 0;
      virtual   void           Free                        ( ) {};

      BMMExport int            Connect                     ( );
      BMMExport int            Disconnect                  ( );
      BMMExport int            SetStorage                  ( BitmapStorage *storage);
      inline  BitmapStorage   *GetStorage                  ( ) { return storage; };
      inline    void           MakeDirty                   ( ) { dirty = 1; };

};

//-----------------------------------------------------------------------------
//-- Bitmap Dither Class
//
//   Private class not to be documented

class BitmapDither {
   
   protected:
   
      BitmapStorage           *storage;                    // Pointer to storage itself
      int                      type;                       // Type index of filter
      
   public:
    
      BMMExport                BitmapDither                ( );
      bVirtual                ~BitmapDither                ( );
      inline    UINT           Type                        ( ) { return type; };
      inline    void           SetType                     ( UINT t) { type = t; };
      BMMExport int            Initialize                  ( BitmapStorage *s);
      virtual   int            PutPixels                 ( int x,int y,int pixels,BMM_Color_64 *ptr) = 0;
      virtual   void           Free                        ( ) {};
      BMMExport int            SetStorage                  ( BitmapStorage *storage);
    
};


// Callback for notifying bitmaps that their Storage has changed, and 
// any on screen displays need to be refreshed.  Installed via
// Bitmap::SetNotify();
// VFBClosed is 
class BitmapNotify{
public:
	virtual int Changed(ULONG flags)=0;
	virtual void VFBClosed() {}  // called when VFB is closed
	};

// Callback for interactive adjustment of bitmap "Cropping rectangle", passed
// in as an argument to Bitmap::Display.
class CropCallback {
	public:
	virtual float GetInitU()=0;
	virtual float GetInitV()=0;
	virtual float GetInitW()=0;
	virtual float GetInitH()=0;
	virtual BOOL GetInitMode()=0;
	virtual void SetValues(float u, float v, float w, float h, BOOL md)=0;
	virtual void OnClose()=0;
	};
	
//-----------------------------------------------------------------------------
//-- Basic Bitmap Class
//
//
   
#define BMM_SINGLEFRAME -2000000L

class Bitmap {
   friend class BitmapManagerImp;
      
   private:
   
      DWORD                    flags;                      // See above

      BitmapManager            *manager;                   // Manager of this bitmap
      BitmapIO                 *output;                    // Head of output handler list
      BitmapFilter             *filter;                    // Filtered access methods
      BitmapDither             *dither;                    // Dither methods
      BitmapStorage            *storage;                   // Actual storage
      UINT                     filterType;                 // Filtered access type
      UINT                     ditherType;                 // Dither type
	  DWORD					   modifyID;                   // changes when bitmap changes: used in render effects
      int                      Free();

	  void 					   *vfbData;	

	  BitmapNotify *bmNotify;		// Called when storage is change so display can update

      friend LRESULT CALLBACK  InputWndProc                ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	  // To delete a Bitmap, call Bitmap::DeleteThis()
      BMMExport               ~Bitmap                      ( );

	  // To get a new Bitmap, call BitmapManager::NewBitmap()
      BMMExport                Bitmap                      ( BitmapManager *manager = TheManager );
     
   public:

      inline  BitmapManager   *Manager                     ( ) { return manager; };

      //-- Don't use these unless you know what you're doing ----------------
      
      BMMExport int            Create                      ( BitmapInfo *bi );
      BMMExport BOOL           FixDeviceName               ( BitmapInfo *bi );
      inline    int            MapReady                    ( ) { if (storage) return storage->MapReady(); return 0; };
      BMMExport void           AddOutput                   ( BitmapIO *out  );
      BMMExport void           RemoveOutput                ( BitmapIO *out  );
      BMMExport BitmapIO *     FindOutput                  ( BitmapInfo *bi );
      BMMExport PAVIFILE       GetPaviFile                 ( BitmapInfo *bi );
	  inline  void 			   *GetVFBData				   ( )	{ return vfbData; }
	  inline  void 			    SetVFBData				   (void *vfb )	{ vfbData = vfb; }

      
      //-- Public Interface -------------------------------------------------
      
      BMMExport void           DeleteThis                  ( ); // Call this , NOT delete, to free a Bitmap.		      
      inline  DWORD            Flags                       ( ) { return flags; };
      inline  void             SetFlag                     ( DWORD flag ) { flags |= flag; };
      inline  void             ToggleFlag                  ( DWORD flag ) { flags ^= flag; };
      inline  void             ClearFlag                   ( DWORD flag ) { flags &= (~flag); };
      
      inline  int              Width                       ( ) { if (storage) return storage->Width();              return 0; };
      inline  int              Height                      ( ) { if (storage) return storage->Height();             return 0; };
      inline  float            Aspect                      ( ) { if (storage) return storage->Aspect();             return (float)0.0; };
      inline  float            Gamma                       ( ) { if (storage) return storage->Gamma();              return (float)0.0; };
      inline  int              Paletted                    ( ) { if (storage) return storage->Paletted();           return 0; };
      inline  int              IsDithered                  ( ) { if (storage) return storage->IsDithered();         return 0; };
      inline  int              PreMultipliedAlpha          ( ) { if (storage) return storage->PreMultipliedAlpha(); return 0; };
      inline  int              HasAlpha                    ( ) { if (storage) return storage->HasAlpha();           return 0; };
      inline  int              MaxRGBLevel                 ( ) { if (storage) return storage->MaxRGBLevel();        return 0; };
      inline  int              MaxAlphaLevel               ( ) { if (storage) return storage->MaxAlphaLevel();      return 0; };
      
              int              Put16Gray                   ( int x,int y,int pixels,WORD *ptr )
                                                           { if (storage) return storage->Put16Gray(x,y,pixels,ptr); return 0; };
      inline  void            *GetStoragePtr               ( int *type ) 
                                                           { if (storage) return storage->GetStoragePtr(type);       return NULL; };
      inline  void            *GetAlphaPtr                 ( int *type ) 
                                                           { if (storage) return storage->GetAlphaPtr(type);         return NULL; };
      inline  int              Get16Gray                   ( int x,int y,int pixels,WORD *ptr )
                                                           { if (storage) return storage->Get16Gray(x,y,pixels,ptr);         return 0; };
      inline  int              GetPixels                   ( int x,int y,int pixels,BMM_Color_64 *ptr )
                                                           { if (storage) return storage->GetPixels(x,y,pixels,ptr);         return 0; };
      BMMExport int            PutPixels                   ( int x,int y,int pixels,BMM_Color_64 *ptr );
      inline  int              GetLinearPixels             ( int x,int y,int pixels,BMM_Color_64 *ptr )
                                                           { if (storage) return storage->GetLinearPixels(x,y,pixels,ptr);     return 0; };
      inline  int              GetIndexPixels              ( int x,int y,int pixels,BYTE *ptr )
                                                           { if (storage) return storage->GetIndexPixels(x,y,pixels,ptr);      return 0; };
      inline  int              PutIndexPixels              ( int x,int y,int pixels,BYTE *ptr )
                                                           { if (storage) return storage->PutIndexPixels(x,y,pixels,ptr);      return 0; };
      inline  int              CropImage                   ( int width,int height,BMM_Color_64 fillcolor)
                                                           { if (storage) return storage->CropImage(width,height,fillcolor);   return 0; };
      inline  int              CropImage                   ( int width,int height,int fillindex)
                                                           { if (storage) return storage->CropImage(width,height,fillindex);   return 0; };
      inline  int              ResizeImage                 ( int width,int height,int newpalette)
                                                           { if (storage) return storage->ResizeImage(width,height,newpalette);return 0; };
      inline  int              CopyImage                   ( Bitmap *from,int operation,BMM_Color_64 fillcolor, BitmapInfo *bi = NULL)
                                                           { if (storage) return storage->CopyImage(from,operation,fillcolor,bi); return 0; };
      inline  int              CopyImage                   ( Bitmap *from,int operation,int fillindex)
                                                           { if (storage) return storage->CopyImage(from,operation,fillindex); return 0; };
      inline  int              GetPalette                  ( int start,int count,BMM_Color_48 *ptr)
                                                           { if (storage) return storage->GetPalette(start,count,ptr);         return 0; };
      inline  int              SetPalette                  ( int start,int count,BMM_Color_48 *ptr)
                                                           { if (storage) return storage->SetPalette(start,count,ptr);         return 0; };

	  //-- GRAINSTART

      // Effects methods (GG 11/03/98) ----------

      BMMExport void			FilmGrain				( float grain, BOOL mask, PBITMAP_FX_CALLBACK callback = NULL, void *param = NULL );

	  //-- GRAINEND

      // GBuffer methods ---------------------

	  inline void             *GetChannel                ( ULONG channelID, ULONG& chanType ) 
                                       { if (storage) return storage->GetChannel(channelID, chanType); return NULL; }   
      
      inline GBuffer *GetGBuffer()  { return storage? storage->GetGBuffer(): NULL; } 

      inline ULONG            CreateChannels             ( ULONG channelIDs ) 
                                             { if (storage) return storage->CreateChannels(channelIDs); return 0; }   
      inline void             DeleteChannels             ( ULONG channelIDs ) 
                                             { if (storage) storage->DeleteChannels(channelIDs); }     
      inline ULONG            ChannelsPresent            ( )   
                                             { if (storage) return storage->ChannelsPresent();  return 0; }   
      inline RenderInfo*           GetRenderInfo()        { if (storage) return storage->GetRenderInfo(); return NULL; }

      inline RenderInfo*           AllocRenderInfo()     { if (storage) return storage->AllocRenderInfo(); return NULL; }


      //---------------------------------------------------------------------
      //
      //   This call will check with the plug-in (file or device) defined in 
      //   the given BitmapInfo and prepare (create) the proper channels. If 
      //   a given channel already exists, no new channel will be created. 
      //
      //   After creating a bitmap, use this function to define the optional 
      //   channels that may be required by the given handler. 
      //

      BMMExport BOOL          PrepareGChannels           ( BitmapInfo *bi ); 
      BMMExport BOOL          PrepareGChannels           ( DWORD channels ); 


      BMMExport int           GetFiltered                ( float u,float v,float du,float dv,BMM_Color_64 *ptr );
      BMMExport int           SetDither                  ( UINT ditherType );
      BMMExport int           SetFilter                  ( UINT filterType );
      inline    int           HasFilter                  ( ) { return (filter) ? 1:0; };
      inline    BitmapFilter  *Filter                    ( ) { return filter; }; 
      BMMExport int           SetStorage                 ( BitmapStorage *storage);
      inline    BitmapStorage *Storage                   ( ) { return storage; };
      inline    void          NullStorage                ( ) { storage = NULL; };
      
      //-- Windows DIB Conversion -------------------------------------------
      //
      //   Where depth is either 24 (BGR) or 32 (BGR0)
      //

      BMMExport PBITMAPINFO    ToDib                       ( int depth = 24, UWORD *gam=NULL, BOOL dither=FALSE);

      //-- Do not use this directly. Instead, use BitmapManager::Create(PBITMAPINFO)

      BMMExport BOOL           FromDib                     ( PBITMAPINFO pbmi );
      
      //-- Image output operations ------------------------------------------
      //
      //  To write a single image to a file/device:
      //
      //  *> Create BitmapInfo class: BitmapInfo bi;
      //
      //  *> Define output file/device:
      //
      //     Directly:       bi.SetName("file.tga");
      //   or
      //     User Interface: BitmapManager::SelectFileOutput( ... &bi ...)
      //
      //  *> Define bitmap: 
      //
      //                     bi.SetWidth(x)
      //                     bi.SetHeight(y)
      //                     etc...
      //
      //  *> Create bitmap:  Bitmap *map = BitmapManager::Create(&bi);
      //                     
      //
      //  *> Do something:   map->Fill({0,0,0});
      //
      //  *> OpenOutput:     map->OpenOutput(&bi);
      //
      //  *> Write:          map->Write(&bi)
      //
      //  *> Close:          map->Close(&bi)
      //
      //  To write a multiframe file, just keep doing something different to
      //  the bimap and keep writting. 
      //
      //  To write a sequence of images to a file/device:
      //
      //  *> Create BitmapInfo class: BitmapInfo bi;
      //
      //  *> Define output file/device:
      //
      //     Directly:       bi.SetName("file.tga");
      //   or
      //     User Interface: BitmapManager::SelectFileOutput( ... &bi ...)
      //
      //  *> Define bitmap: 
      //
      //                     bi.SetWidth(x)
      //                     bi.SetHeight(y)
      //
      //                     bi.SetFirstFrame(0)
      //                     bi.SetLastFrame(29)
      //
      //                     etc...
      //
      //  *> Create bitmap:  Bitmap *map = BitmapManager::Create(&bi);
      //                     
      //
      //  *> OpenOutput:     map->OpenOutput(&bi);
      //
      //     for (x = 0 to 29) {
      //        *> Do something to image...
      //        *> Write:    map->Write(&bi,x);
      //     }
      //
      //  *> Close:          map->Close(&bi)
      //
      //
      //  Note: You can add any number of  outputs to a bitmap. Just keep
      //  calling map->OpenInput() with different outputs (Targa file AND
      //  Frame Buffer for instance). To write or close a specific output,
      //  use Write()  and Close().  To write  and close them all at once,
      //  use WriteAll() and CloseAll().
      //
      //  It is ok to use WriteAll() and CloseAll() if you have just one
      //  output defined.
      //
      
      BMMExport BMMRES         OpenOutput                  ( BitmapInfo *bi );                                        // Open output
      BMMExport BMMRES         Write                       ( BitmapInfo *bi, int frame = BMM_SINGLEFRAME );         // Write frame to file
      BMMExport BMMRES         WriteAll                    ( int frame = BMM_SINGLEFRAME );                         // Write all open outputs
      BMMExport int            Close                       ( BitmapInfo *bi, int flag = BMM_CLOSE_COMPLETE );         // Close an open output
      BMMExport int            CloseAll                    ( int flag = BMM_CLOSE_COMPLETE);                          // Close all open outputs

      //-- Window gravity

      #define   BMM_UL  1      //-- Upper Left
      #define   BMM_LL  2      //-- Lower Left
      #define   BMM_UR  3      //-- Upper Right
      #define   BMM_LR  4      //-- Upper Left
      #define   BMM_CN  5      //-- Center

      #define   BMM_RND 10     //-- Renderer (Save/Restore)
      #define   BMM_VPP 11     //-- Video Post Primary (Save/Restore)
      #define   BMM_VPS 12     //-- Video Post Secondary (Save/Restore)

      BMMExport int            Display                     ( TCHAR *title = NULL, 	int position = BMM_CN, 
      		BOOL autonomous = FALSE, BOOL savebutton = TRUE, CropCallback *crop=NULL );
      BMMExport int            UnDisplay                   ( );
      BMMExport HWND           GetWindow                   ( );
      BMMExport void           RefreshWindow               ( RECT *rect = NULL );
      BMMExport void           SetWindowTitle              ( TCHAR *title );
	  BMMExport void           SetCroppingValues		   ( float u, float v, float w, float h, BOOL placeImage);

      //-- Get a Different Frame  -------------------------------------------
      //
      //   For  multifrane bitmaps (FLI's, AVI's, DDR devices, etc.),  if you
      //   simply want to load  another frame replacing a previously "Load"ed
      //   image.
      //
      //   If used with single frame drivers or if the driver doesn't support
      //   this function,  it returns BMMRES_SINGLEFRAME. If the return value
      //   is BMMRES_SUCCESS,  a new frame  has  been  loaded  into the given 
      //   bitmap.
      //
      //   To define desired frame, use bi->SetCurrentFrame( frame );
      //
      
      BMMExport BMMRES         GoTo                        ( BitmapInfo *bi );

      //-- Handy built-in functions

      BMMExport int            Fill                        ( int r,int g,int b,int alpha);

	  // Set a callback so can get notified if storage changed
      BMMExport void 		  SetNotify( BitmapNotify *bmnot=NULL);
      BitmapNotify *		  GetNotify() { return bmNotify; }
		
	  BMMExport BOOL IsAutonomousVFB();

 	  // Generic expansion function
	  BMMExport int Execute(int cmd, ULONG arg1=0, ULONG arg2=0, ULONG arg3=0); 

	  DWORD GetModifyID() { return modifyID; }
	  void SetModifyID(DWORD m) { modifyID = m; }
	  BMMExport void IncrModifyID();

      // Print the bitmap (if supported by the host app)
      BMMExport void Print(bool silent = false);

	  BMMExport void ShowProgressLine(int y); // y<0 to hide
};

//-- Various Bitmap In-Memory Lists -------------------------------------------

struct BMMStorageList {
	BitmapStorage *ptr;
	BMMStorageList *next;
	};

struct BMMFilterList {
	BitmapFilter *ptr;
	BMMFilterList *next;
	} ;

struct BMMBitmapList {
	Bitmap *ptr;
	BMMBitmapList *next;
	};

typedef struct tag_BMMGammaSettings {
	BitmapManager *mgr;
	BitmapInfo    *bi;
	BOOL           out;
	} BMMGammaSettings;     

typedef struct tag_BMMVfbPalette {
	BYTE  r,g,b;
	} BMMVfbPalette;     

class BitmapFileInputDialog {
	public:
	virtual BOOL BrowseBitmapFilesInput(BitmapInfo* info, HWND hWnd, TCHAR* title, BOOL view) = 0;
	};

class BitmapFileOutputDialog {
	public:
	virtual BOOL BrowseBitmapFilesOutput(BitmapInfo* info, HWND hWnd, TCHAR* title) = 0;
	};

//-----------------------------------------------------------------------------
//-- Main Bitmap Manager Class
//

class BitmapManager {
   
   public:
   
      BMMVfbPalette            *pal;

      //-- Construction/Destruction
      
      BitmapManager               ( BMMInterface *i) { pal = NULL; }
      BitmapManager               ( BMMInterface *i,const TCHAR *name) { pal = NULL; }
      virtual  BMMExport    ~BitmapManager               ( );
      friend void       DoConstruct      ( BitmapManager *m, BMMInterface *i, const TCHAR *name);

      //-- These are for internal use only
      
      virtual int            DeleteAllMaps               ( )=0;
      virtual int            AddStorage                  ( BitmapStorage *s)=0;
      virtual int            AddFilter                   ( BitmapFilter *a)=0;
      virtual int            AddBitmap                   ( Bitmap *b)=0;
      virtual int            DelStorage                  ( BitmapStorage *s)=0;
      virtual int            DelFilter                   ( BitmapFilter *a)=0;
      virtual int            DelBitmap                   ( Bitmap *b)=0;
      virtual BitmapFilter  *FindFilter                  ( BitmapStorage *s,UINT type)=0;
      virtual BitmapStorage *FindStorage                 ( BitmapInfo *bi, int openMode)=0;
      virtual int            FnametoBitMapInfo           ( BitmapInfo *bi )=0;       
      virtual void           FixFileExt                  ( OPENFILENAME &ofn, const TCHAR *extension)=0;
      virtual void           MakeVfbPalette              ( )=0;

      BMM_IOList               ioList;
      virtual void           ListIO                      ( )=0;
      
      //---------------------------------------------------------------------
      //-- Public Interface -------------------------------------------------
      
      //-- Host Interface

      virtual HINSTANCE      AppInst                     ( )=0;
      virtual HWND           AppWnd                      ( )=0;
      virtual DllDir        *AppDllDir                   ( )=0;
      virtual TCHAR         *GetDir                      (int i)=0;
      virtual BOOL           AddMapDir                   (TCHAR *dir,int update)=0;
      virtual int            GetMapDirCount              ( )=0;
      virtual TCHAR         *GetMapDir                   (int i)=0;
      virtual Interface     *Max                         ()=0;
	  virtual Bitmap        *NewBitmap                   ()=0; // returns a new Bitmap 

	  // Set a replacement for the bitmap file dialog. NULL will set the default.
	  virtual void			SetFileInputDialog(BitmapFileInputDialog* dlg = NULL)=0;
	  virtual void			SetFileOutputDialog(BitmapFileOutputDialog* dlg = NULL)=0;

      //-- These won't stay here. Error handling will be dealt in a couple of 
      //   different ways. There will be a "Silent" flag that will be set by
      //   the client and tested here in order to know if an error dialog should
      //   go up. Normally, if the user is sitting in front of the computer
      //   this flag will be FALSE. When rendering in the background, or network
      //   rendering, etc., this flag will be TRUE. There should be some kind of
      //   "preferences" configuration for this behavior.
      //
      //   There also will be a method for logging errors. This method will check
      //   for a "loggin on/off" flag and either add the entry or ignore it. The
      //   bitmap manager and its devices will log everything that goes wrong.
      //   When silent mode is on and logging is also on, this is the method to
      //   check what went bad. Having each device logging its own error will
      //   enable a more accurate description of the problem (as opposed to "I/O
      //   error").
      //
      //   Gus
      //
      
      virtual int            Status                      ( int *sCount, int *aCount, int *bCount)=0;
      virtual int            Error                       ( const TCHAR *string)=0;

      //-- Error handling ---------------------------------------------------
      //
      //

      //-- Max Interface (means no logging)
      
      #define LG_NOLOG         0
      
      //-- User Interface
      
      #define LG_FATAL         ((DWORD)(1 << 0))
      #define LG_INFO          ((DWORD)(1 << 1))
      #define LG_DEBUG         ((DWORD)(1 << 2))
      #define LG_WARN          ((DWORD)(1 << 3))

      //-- User Interface 
      
      virtual BOOL           SilentMode                  ( )=0;
      virtual void           SysLog                      ( int type, char *format, ... )=0;

      //-- Max Interface (used internally)
      
      virtual BOOL           SetSilentMode               ( BOOL s )=0;
      virtual void           SetLogLevel                 ( DWORD level )=0;
      virtual DWORD          GetLogLevel                 ( )=0;

      //-- Creating a new bitmap from scracth -------------------------------
      //
      //   Make  sure the given  BitmapInfo class has the proper data for the
      //   creation of the bitmap.  If you used the BitmapManager function to
      //   define the bitmap (SelectBitmapOutput()), both filename and device
      //   driver have been defined for you. 
      //
      //   Make sure to set the type of bimap using bi.SetType(). This will 
      //   define the storage type as in (so far):
      //
      //   BMM_LINE_ART 
      //   BMM_PALETTED 
      //   BMM_GRAY_8
      //   BMM_GRAY_16
      //   BMM_TRUE_16  
      //   BMM_TRUE_32  
      //   BMM_TRUE_64  
      //
      //   Do NOT use BMM_TRUE_24 nor BMM_TRUE_48. These are read only types.
      //
      //   Example code is in src/app/vpexecut.cpp
      //
      //
      //   Once a bitmap has been created, use its own methods for adding
      //   outputs and writing it (i.e. map->OpenOutput(), map->Write() and
      //   map->Close()).
      //
      //   Gus
      //
      
      virtual BMMExport Bitmap        *Create                      ( BitmapInfo *bi   )=0;

      //-- Creating a new bitmap from an existing Windows DIB ---------------
      //
      //   To Convert a Bitmap to a Windows DIB check Bitmap::ToDib()
      //

      virtual BMMExport Bitmap        *Create                      ( PBITMAPINFO pbmi )=0;

      //-- Loads Bitmap -----------------------------------------------------
      //
      //   Loads a bitmap.
      //
      //   Like most other  bitmap  functions, you should define the image to
      //   to load (either setting the name/device directly in BitmapInfo bi,
      //   or having  SelectFileInput() do it for you).  Once bi has the name
      //   of the image you want to  load, call Load() to create a new Bitmap
      //   which contains the image. Additional options may be set by calling
      //   ImageInputOptions()  before calling Load().  That will as the user 
      //   for special details such as  positioning of smaller/larger images,
      //   etc. All this does is to set the proper fields in BitmapInfo. It's
      //   up to you to use those.
      //
      //   BitmapInfo defaults to frame "zero". For multifrane files, such as
      //   *.avi, *.ifl, *.flc, etc. you should  specify the frame number you
      //   want. Do it by using bi.SetCurrentFrame(f)  before calling Load().
      //   
      //   
      //   Note: If loading images from a device, make sure bi.Name() is
      //         empty (bi.SetName(_T(""));). This is automatic if you use
      //         SelectDeviceInput(). If you just create a BitmapInfo
      //         instance and set the device name by hand (bi.SetDevice()),
      //         this is also automatic as both name and device names are
      //         by default empty. This should only be a concern if you
      //         reuse a BitmapInfo class previously used for image files.
      //   
      //   
      //   One of the methods in BitmapInfo returns a  window handle to  send
      //   progress report messages. If you  want to  receive these messages,
      //   set the window handle  (  bi->SetUpdateWindow(hWnd)  ) and process
      //   BMM_PROGRESS messages (see above).
      //   
      //   Gus
      //
      
      virtual BMMExport Bitmap        *Load                       ( BitmapInfo *bi, BMMRES *status = NULL)=0;

      //-- Load Image into an existing Bitmap  ----------------

      virtual BMMRES         LoadInto                    ( BitmapInfo *bi, Bitmap **map, BOOL forceReload=FALSE )=0;
      
      //-- General User Interface -------------------------------------------
      
      virtual BMMRES         GetImageInfoDlg             ( HWND hWnd, BitmapInfo *bi, const TCHAR *filename = NULL )=0;
      virtual BMMRES         GetImageInfo                (            BitmapInfo *bi, const TCHAR *filename = NULL )=0;
      virtual BOOL           ImageInputOptions           ( BitmapInfo *bi, HWND hWnd )=0;
      virtual BOOL           SelectDeviceInput           ( BitmapInfo *bi, HWND hWnd )=0;
      virtual BOOL           SelectDeviceOutput          ( BitmapInfo *bi, HWND hWnd )=0;

      virtual BOOL           SelectFileOutput            ( BitmapInfo *bi, 
                                                             HWND hWnd, 
                                                             TCHAR *title = NULL)=0;

      virtual BOOL           SelectFileInput             ( BitmapInfo *bi, 
                                                             HWND hWnd, 
                                                             TCHAR *title = NULL)=0;
      
      virtual BOOL           SelectFileInputEx           ( BitmapInfo *bi, 
                                                             HWND hWnd, 
                                                             TCHAR *title  = NULL,
                                                             BOOL viewonly = FALSE)=0;
      
	  virtual void RefreshAllVFBs()=0;

	  virtual void DeleteAllAutonomousVFBMaps()=0;

	  // Generic expansion function
	  virtual int Execute(int cmd, ULONG arg1=0, ULONG arg2=0, ULONG arg3=0)=0; 

	  virtual void BeginSavingLoadErrorFiles()=0;  // --Begin accumulating a list of files that didn't load,
	  											   // and don't put up load errors in the meantime.
	  virtual NameTab &GetLoadErrorFileList()=0;  // -- List of names of files not found
	  virtual void EndSavingLoadErrorFiles()=0;  // --End accumulating a list of files that didn't load, free the list.
      virtual BMMExport bool CanImport(const TCHAR* filename)=0;

};

//-----------------------------------------------------------------------------
//-- Forward References for Bitmap Functions
//
//   Internal Use
//

extern int                       ValidBitmapType             ( int type );
extern BMMExport BitmapStorage  *BMMCreateStorage            ( BitmapManager *manager,UINT type );
extern BMMExport BitmapFilter   *BMMCreateFilter             ( BitmapManager *manager,UINT type );
extern BMMExport BitmapDither   *BMMCreateDither             ( BitmapManager *manager,UINT type );
extern BMMExport int             BMMCalcPalette              ( Bitmap *map,int colors,BMM_Color_48 *palette );
extern BMMExport BYTE            BMMClosestColor             ( BMM_Color_64 *color,BMM_Color_48 *palette,int colors );

extern BMMExport void            OpenBMM                     ( BMMInterface *i );
extern BMMExport void            CloseBMM                    ( );
extern           BOOL CALLBACK   BMMGammaDlgProc             (HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
extern           void            ExtractExtension            ( TCHAR *string, TCHAR *ext );

//-----------------------------------------------------------------------------
//-- Handy-dandy pixel buffer class
//

class PixelBuf {

   private:

      BMM_Color_64 *buf;
      int           width;

   public:

      inline                   PixelBuf ( int width) { buf = (BMM_Color_64 *)calloc(width,sizeof(BMM_Color_64)); this->width=width; };
      inline                  ~PixelBuf ( ) { if(buf) free(buf); };
      inline BMM_Color_64     *Ptr      ( ) { return buf; };

      int                      Fill     ( int start, int count, BMM_Color_64 color ) {
                                  int ix,jx=start+count;
                                  if (jx > width)
                                     return 0;
                                  for(ix=start; ix<jx; buf[ix++]=color);
                                  return 1;
                               };
};

//-- Public Utilities ---------------------------------------------------------

BMMExport int    BMMCreateNumberedFilename  ( const TCHAR *namein,int frame,TCHAR *nameout );
BMMExport int    BMMGetFullFilename         ( BitmapInfo *bi );
BMMExport BOOL      BMMIsFile                  ( const TCHAR *filename );
BMMExport void      BMMSplitFilename           ( const TCHAR *name,TCHAR *p,TCHAR *f,TCHAR *e );
BMMExport void      BMMAppendSlash             ( TCHAR *path );
BMMExport BOOL      BMMGetUniversalName        ( TCHAR *szUniv, const TCHAR *szDrive );
BMMExport LPTSTR BMMGetLastErrorText        ( LPTSTR lpszBuf, DWORD dwSize );

// Get a color packer.  When done, be sure to call its DeleteThis();
BMMExport ColorPacker *BMMNewColorPacker(
   int w,            // width of bitmap to be packed
   BMM_Color_48 *pal,   // palette to use
   int npal,         // number of entries in the palette
   BYTE* remap=NULL  // optional remap done at last stage.
   );


BMMExport Quantizer *BMMNewQuantizer();

//  Rearrange palette "pal" ( which has colors 0..ncols-1 occupied, 
//  in descending order of frequency),  into "newpal" so that the colors 10-245 are 
//  populated first, then 0-9, then 246-255.  Sets optional array "remap" to map the
//  old palette index values to the new ones    
BMMExport void FixPaletteForWindows(BMM_Color_48 *pal, BMM_Color_48 *newpal,int ncols, BYTE *remap=NULL);

//-- Cleanup ------------------------------------------------------------------

#undef bVirtual


#endif BITMAP_H_DEFINED

//-- EOF: bitmap.h ------------------------------------------------------------
