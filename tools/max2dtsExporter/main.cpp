//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#pragma pack(push,8)
#include <MAX.H>
#pragma pack(pop)

// NOTE:
// if you have problems linking this exporter it is likely you are encountering a
// bug in Microsofts include\basetsd.h header.
//
// The problem is that Visual C++ defines INT_PTR to 'long' when it is
// supposed to be defined as an 'int' (on ia32 platforms). You can either
// use a supported build environment by updating to the platform SDK, or
// you can use the unsupported environment by manually fixing the problem
// in the header "On or around line 123 of include\basetsd.h change:
//
//    typedef long INT_PTR, *PINT_PTR;
//    typedef unsigned long UINT_PTR, *PUINT_PTR;
// TO
//    typedef int INT_PTR, *PINT_PTR;
//    typedef unsigned int UINT_PTR, *PUINT_PTR;

#include "max2dtsExporter/exporter.h"
#include "max2dtsExporter/sceneEnum.h"
#include "core/filestream.h"
#include "ts/tsShapeInstance.h"
#include "max2dtsExporter/sequence.h"
#include "max2dtsExporter/exportUtil.h"
#include "max2dtsExporter/skinHelper.h"


#define DLLEXPORT __declspec(dllexport)


#if defined(TORQUE_DEBUG)
   const char * const gProgramVersion = "0.900d-beta";
#else
   const char * const gProgramVersion = "0.900r-beta";
#endif

HINSTANCE hInstance;

S32 gSupressUI = 0;
TCHAR maxFile[1024];
S32 controlsInit = FALSE;
char dllPath[256];
char globalConfig[270];

TCHAR *GetString(S32 id)
{
    static TCHAR buf[256];

    if (hInstance)
    {
        return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
    }

    return NULL;
}
const char * days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char * months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

static S32 Alert(const char * s1, const char * s2, S32 option = MB_OK)
{
   if (gSupressUI)
   {
      FileStream fs;
      fs.open(avar("%smax2difExport.error.log",dllPath),FileStream::ReadWrite);
      if (fs.getStatus() != Stream::Ok)
      {
         fs.close();
         return -1;
      }
      fs.setPosition(fs.getStreamSize());

      // what's the time and date?
      Platform::LocalTime localTime;
      Platform::getLocalTime(localTime);
      char timeBuffer[256];
      dStrcpy(timeBuffer,avar("%i:%i on %s %s %i",localTime.hour,localTime.min,days[localTime.weekday],months[localTime.month],localTime.monthday));

      // write the error to the log file
      char buffer[1024];
      dStrcpy(buffer,avar("Error exporting file \"%s\" at %s:\r\n  --> %s\r\n\r\n",maxFile,timeBuffer,s1));
      fs.writeLine((unsigned char*) buffer);
      fs.close();
      return -1;
   }
   else
   {
      TSTR str1(s1);
      TSTR str2(s2);
      return MessageBox(GetActiveWindow(), str1, str2, option);
   }
}

static S32 Alert(S32 s1, S32 s2 = IDS_TH_DTSEXP, S32 option = MB_OK)
{
   return Alert(GetString(s1),GetString(s2),option);
}

static S32 Alert(const char * s1, S32 s2 = IDS_TH_DTSEXP, S32 option = MB_OK)
{
   return Alert(s1,GetString(s2),option);
}

const char * GetVersionString()
{
   static char versionString[128];
   dStrcpy(versionString,avar("Version %s (dts v%i.%.2i)",gProgramVersion, DTS_EXPORTER_CURRENT_VERSION/100, DTS_EXPORTER_CURRENT_VERSION%100));
   return versionString;
}

//------------------------------------------------------
// Our main plugin class:

class _Exporter : public SceneExport
{
    char extension[10]; // 'DTS' or 'DSQ'

    public:
         _Exporter(const char *);
        ~_Exporter();

        S32           ExtCount();         // Number of extensions supported
        const TCHAR  *Ext(S32);           // Extension #n
        const TCHAR  *LongDesc();         // Long ASCII description
        const TCHAR  *ShortDesc();        // Short ASCII description
        const TCHAR  *AuthorName();       // ASCII Author name
        const TCHAR  *CopyrightMessage(); // ASCII Copyright message
        const TCHAR  *OtherMessage1();    // Other message #1
        const TCHAR  *OtherMessage2();    // Other message #2
        U32           Version();          // Version number * 100
        void          ShowAbout(HWND);    // Show DLL's "About..." box

        S32 DoExport(const TCHAR *, ExpInterface *, Interface *, int, DWORD);
};

//------------------------------------------------------
// Jaguar interface code

extern void mInstallLibrary_C();
namespace Memory
{
extern bool gAlwaysLogLeaks;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/)
{
    hInstance = hinstDLL;

    if (!controlsInit)
    {
        controlsInit = TRUE;

        // 3DS custom controls
        InitCustomControls(hInstance);

        // Initialize common Windows controls
        InitCommonControls();

        // install math library (set up function pointers)
        mInstallLibrary_C();

        // set "factory" default parameter values
        SceneEnumProc::setInitialDefaults();

        // always log memory leaks -- some of them aren't really leaks,
        // but we want to see them and we don't want the annoying box asking us
        // about it...
        Memory::gAlwaysLogLeaks = true;

        // get DLL path
        GetModuleFileName(hInstance,dllPath,sizeof(dllPath));
        char * p  = dStrrchr(dllPath,'\\');
        char * p2 = dStrrchr(dllPath,':');
        if (p && *p=='\\')
            *(p+1) = '\0';
        else if(p2 && *p2==':')
            *(p2+1) = '\0';
        else
            dllPath[0] = '\0';

        // load the global config file if we can find it
        dStrcpy(globalConfig,dllPath);
        dStrcpy(globalConfig+dStrlen(globalConfig),"dtsGlobal.cfg");
        SceneEnumProc::readConfigFile(globalConfig);
    }

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return(TRUE);
}


// defines for setting exporter type and description
#define  _EXPORTER_CLASS_NAME    "Torque Shape Exporter"
#define  _DTS_EXPORTER_CLASS_ID  Class_ID(0x296a4787, 0x2ec557fc)
#define  _DSQ_EXPORTER_CLASS_ID  Class_ID(0x65d74e76, 0x10da4a97)
#define  _TXT_EXPORTER_CLASS_ID  Class_ID(0x6bfb02d7, 0x13860666)
#define  _EXPORTER_CLASS_SDESC   "Torque Shape Exporter"

// more code for interfacing with max
class _DTSClassDesc : public ClassDesc
{
    public:
        S32         IsPublic() { return 1; }
        void        *Create(BOOL loading = FALSE) { return new _Exporter("DTS"); }
        const TCHAR *ClassName() { return _EXPORTER_CLASS_NAME; }
        SClass_ID   SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
        Class_ID    ClassID() { return _DTS_EXPORTER_CLASS_ID; }
        const TCHAR *Category() { return GetString(IDS_TH_SHAPEEXPORT);  }
};

// more code for interfacing with max
class _DSQClassDesc : public ClassDesc
{
    public:
        S32         IsPublic() { return 1; }
        void        *Create(BOOL loading = FALSE) { return new _Exporter("DSQ"); }
        const TCHAR *ClassName() { return _EXPORTER_CLASS_NAME; }
        SClass_ID   SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
        Class_ID    ClassID() { return _DSQ_EXPORTER_CLASS_ID; }
        const TCHAR *Category() { return GetString(IDS_TH_SHAPEEXPORT);  }
};

// more code for interfacing with max
class _TXTClassDesc : public ClassDesc
{
    public:
        S32         IsPublic() { return 1; }
        void        *Create(BOOL loading = FALSE) { return new _Exporter("TXT"); }
        const TCHAR *ClassName() { return _EXPORTER_CLASS_NAME; }
        SClass_ID   SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
        Class_ID    ClassID() { return _TXT_EXPORTER_CLASS_ID; }
        const TCHAR *Category() { return GetString(IDS_TH_SHAPEEXPORT);  }
};

static _DTSClassDesc _DTSDesc;
static _DSQClassDesc _DSQDesc;
static _TXTClassDesc _TXTDesc;

DLLEXPORT const TCHAR *LibDescription()
{
    return GetString(IDS_TH_DTSEXPORTDLL);
}

DLLEXPORT S32 LibNumberClasses()
{
    return 6;
}

DLLEXPORT ClassDesc *LibClassDesc(S32 i)
{
    switch(i)
    {
        case 0:
            return &_DTSDesc;

        case 1:
            return &_DSQDesc;

        case 2:
            return &_TXTDesc;

        case 3:
            return GetSequenceDesc();

        case 4:
            return GetExportUtilDesc();

        case 5:
            return GetSkinHelperDesc();

        default:
            return NULL;
    }
}

// Return version so can detect obsolete DLLs
DLLEXPORT ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

//
// ._DTS export module functions follow:
//

_Exporter::_Exporter(const char * _extension)
{
    dStrcpy(extension,_extension);
}

_Exporter::~_Exporter()
{
}

S32 _Exporter::ExtCount()
{
    return 1;
}

// Extensions supported for import/export modules
const TCHAR *_Exporter::Ext(S32 n)
{
    switch(n)
    {
        case 0:
            return _T(extension);

        default:
            return _T("");
    }
}

// Long ASCII description (e.g., "Targa 2.0 Image File")
const TCHAR *_Exporter::LongDesc()
{
    return GetString(IDS_TH_DTSFILE_LONG);
}

// Short ASCII description (e.g., "Targa")
const TCHAR *_Exporter::ShortDesc()
{
    return _EXPORTER_CLASS_SDESC;
}

// ASCII Author name
const TCHAR *_Exporter::AuthorName()
{
    return GetString(IDS_TH_AUTHOR);
}

// ASCII Copyright message
const TCHAR *_Exporter::CopyrightMessage()
{
    return GetString(IDS_TH_COPYRIGHT_COMPANY);
}

// Other message #1
const TCHAR *_Exporter::OtherMessage1()
{
    return _T("");
}

// Other message #2
const TCHAR *_Exporter::OtherMessage2()
{
    return _T("");
}

// Version number * 100 (i.e. v3.01 = 301)
U32 _Exporter::Version()
{
    return DTS_EXPORTER_CURRENT_VERSION;
}

// Optional
void _Exporter::ShowAbout(HWND hWnd)
{
}

//------------------------------------------------------

// be more thourough about this later...
U32  saveDumpMask;
bool saveEnableSequences;
bool saveCollapse;
bool saveAllowEmptySubtrees;
bool saveAllowCrossedDetails;
bool saveAllowUnusedMeshes;
bool saveViconNeeded;
bool saveAllowOldSequences;
F32  saveAnimationDelta;
F32  saveMaxFrameRate;
S32  saveT2AutoDetail;

void saveConfig()
{
    saveDumpMask = dumpMask;
    saveEnableSequences = enableSequences;
    saveCollapse = transformCollapse;
    saveAllowEmptySubtrees = allowEmptySubtrees;
    saveAllowCrossedDetails = allowCrossedDetails;
    saveAllowUnusedMeshes = allowUnusedMeshes;
    saveViconNeeded = viconNeeded;
    saveAllowOldSequences = allowOldSequences;
    saveAnimationDelta = animationDelta;
    saveMaxFrameRate = maxFrameRate;
    saveT2AutoDetail = t2AutoDetail;
}

void restoreConfig()
{
    dumpMask = saveDumpMask;
    enableSequences = saveEnableSequences;
    transformCollapse = saveCollapse;
    allowEmptySubtrees = saveAllowEmptySubtrees;
    allowCrossedDetails = saveAllowCrossedDetails;
    allowUnusedMeshes = saveAllowUnusedMeshes;
    viconNeeded = saveViconNeeded;
    allowOldSequences = saveAllowOldSequences;
    animationDelta = saveAnimationDelta;
    maxFrameRate = saveMaxFrameRate;
    t2AutoDetail = saveT2AutoDetail;
}

S32 cheapSemaphore = 0;

S32 _dts_save(const TCHAR *fname, ExpInterface *ei, Interface *gi)
{
   if (cheapSemaphore)
   {
       Alert("One export at a time:  Export code not re-entrant.");
       return 1;
   }
   cheapSemaphore = 1;

   char filename[1024];
   dStrcpy(filename,fname);
   char * ch = filename;
   while (*ch!='\0')
   {
      *ch = tolower(*ch);
      ch++;
   }

   SceneEnumProc myScene;
   dStrcpy(maxFile,gi->GetCurFilePath());

   //-----------------------------------------------
   // read in the config file...
   char configFilename[256];
   dStrcpy(configFilename,maxFile);
   char * p  = dStrrchr(configFilename,'\\');
   char * p2 = dStrrchr(configFilename,':');
   if (p && *p=='\\')
      dStrcpy(p+1,"*.cfg");
   else if (p2 && *p2==':')
      dStrcpy(p2+1,"*.cfg");
   else
      dStrcpy(configFilename,"*.cfg");
   const char * error = myScene.readConfigFile(configFilename);
   if (error)
      myScene.setExportError(error);

   //-----------------------------------------------
   // Error?
   if (myScene.isExportError())
   {
       Alert(myScene.getExportError());
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   // create dump file...
   myScene.startDump(filename,maxFile);

   //-----------------------------------------------
   // read config file again so we have a record of it...
   myScene.readConfigFile(configFilename);

   //-----------------------------------------------
   // Error?
   if (myScene.isExportError())
   {
       Alert(myScene.getExportError());
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   // tweak some parameters depending on export type...
   if (dStrstr(filename,(char*)".dsq"))
   {
       // we're exporting sequences, so turn this on
       if (!enableSequences)
       {
           enableSequences = true;
           SceneEnumProc::printDump(PDAlways,"\r\nEnabling \"Param::SequenceExport\" (doing *.dsq export).\r\n");
       }
       SceneEnumProc::exportType = 's'; // sequence
   }
   if (dStrstr(filename,(char*)".txt"))
   {
       // doing a text file dump -- export everything
       if (!enableSequences)
       {
           enableSequences = true;
           SceneEnumProc::printDump(PDAlways,"\r\nEnabling \"Param::SequenceExport\" (doing *.txt export).\r\n");

       }
       if (transformCollapse)
       {
           transformCollapse = false;
           SceneEnumProc::printDump(PDAlways,"\r\nDisabling \"Param::CollapseTransforms\" (doing *.txt export).\r\n");
       }
       SceneEnumProc::exportType = 't'; // text
   }
   if (dStrstr(filename,(char*)".dts"))
      SceneEnumProc::exportType = 'w'; // whole shape

   //-----------------------------------------------
   // Error?
   if (myScene.isExportError())
   {
       Alert(myScene.getExportError());
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   // Get the nodes we're interested in!
   // We also do some checking to make sure everything
   // we need is present ... if something is missing,
   // an error will be returned.
   myScene.enumScene(ei->theScene);

   //-----------------------------------------------
   // Error?
   if (myScene.isExportError())
   {
       Alert(myScene.getExportError());
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   // Any useful nodes?
   if (myScene.isEmpty())
   {
       Alert("No data to export");
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   // make sure we get rid of target file before opening it
   // otherwise, it'll never shrink
   File zap;
   zap.open(filename,File::Write);
   zap.close();

   //-----------------------------------------------
   // open a file to save the exported shape to:
   FileStream file;
   file.open(filename,FileStream::Write);
   if( file.getStatus() != Stream::Ok )
   {
      Alert(IDS_TH_CANTCREATE);
      cheapSemaphore = 0;
      return(0);
   }

   //-----------------------------------------------
   // actually do the export:
   myScene.processScene();

   //-----------------------------------------------
   // Error?
   if (myScene.isExportError())
   {
       Alert(myScene.getExportError());
       file.close();
       cheapSemaphore = 0;
       return 1;
   }

   //-----------------------------------------------
   //  now save the shape
   TSShape * pShape = myScene.getShape();

   if (dStrstr(filename,(char*)".dts"))
       pShape->write(&file);
   else if (dStrstr(filename,(char*)".dsq"))
       pShape->exportSequences(&file);
   else if (dStrstr(filename,(char*)".txt"))
       myScene.exportTextFile(&file);

   //-----------------------------------------------
   // close the file and report any problems:

   if( file.getStatus() != Stream::Ok )
   {
      Alert(IDS_TH_WRITEERROR);
      file.close();
      remove(filename);
      cheapSemaphore = 0;
      return(0);
   }
   else
      file.close();

   //-----------------------------------------------
   // write the shape out to dump file
   if (PDShapeHierarchy & dumpMask && dStrstr(filename,(char*)".dts"))
   {
      // write out the structure of the newly created shape
      // but read it from the file first...
      if (!SceneEnumProc::dumpShape(filename))
      {
          Alert(avar("Error opening created file \"%s\".",filename));
          file.close();
          cheapSemaphore = 0;
          return 1;
      }
   }

   cheapSemaphore = 0;
   return 1;
}

S32 _Exporter::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, S32 supressUI, DWORD)
{
   gSupressUI = supressUI;

   // so we can go back to defaults when we're done
   saveConfig();

   S32 status;

   status = _dts_save(filename, ei, gi);

   restoreConfig();

   if(status == 0)
      return 1;        // Dialog cancelled

   if(status < 0)
      return 0;        // Real, honest-to-goodness error

   return status;
}
