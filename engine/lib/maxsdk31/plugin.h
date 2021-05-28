
#ifndef PLUGIN_H_DEFINED
#define PLUGIN_H_DEFINED

/*******************************************************************
 *
 *    DESCRIPTION: DLL Plugin classes
 *
 *    AUTHOR: Dan Silva
 *
 *    HISTORY: 11/30/94 Started coding
 *
 *******************************************************************/

typedef unsigned long ulong;

#include <plugapi.h>
//---------------------------------------------------------
// This is the interface to a DLL
// Each DLL may implement any number of classes. 
//
class DllDesc {
public:
	HINSTANCE		handle;
	TSTR fname;
    bool loaded;                         // if false, just basic info gleaned from Registry
	const TCHAR*	(*vDescription)();   // copy of this string can be stored in the DLL dir
	int				(*vNumClasses)();    // How many classes does this DLL implement?
    ClassDesc*		(*vClassDesc)(int i); // Hands back a ClassDesc for the ith class
	CoreExport	 	DllDesc(); 
	CoreExport void		 	Free() { FreeLibrary(handle); }
	CoreExport const TCHAR*	Description() { return (*vDescription)(); };
	CoreExport int 			NumberOfClasses() { return (*vNumClasses)(); };
	ClassDesc*		operator[](int i) { return (*vClassDesc)(i); };
	int operator==( const DllDesc& dd ) const { return 0; }

	TSTR tDescription;
	int tNumClasses;
	TSTR directory;
	};


/*-----------------------------------------------------------------
 We will build one big DllDir on startup.
 As we do this, we will build a set of lists, one for each SuperClass.
 For a given super class, we want to 
	(a) Enumerate all public subclasses 
	(b) Enumerate all subclasses
	(c) Find the subClass for a given subClass ID.

--------------------------------------------------------------*/

class ClassEntry {
	int dllNumber;  // index into the master Dll list
	int classNum;  	// index of the class within the DLL
	int isPublic;
	int useCount;  // used for enumerating classes in summary info
	Class_ID classID;
	TSTR name;
	TSTR category;
	ClassDesc *cd;
	bool loaded;
	
	// The following are use to store the last rollup state for a
	// given class. 'scroll' is the scroll amount of the command panel
	// and pageState is the open close state for up to 32 rollup pages. (when the bit is set that means the rollup page is closed)
	int scroll;
	DWORD pageState;
	
	public:
		CoreExport ClassEntry();
		CoreExport ClassEntry(const ClassEntry &ce);
		CoreExport ClassEntry(ClassDesc *cld, int dllN, int index, bool load);
		CoreExport ~ClassEntry();
		CoreExport void Set(ClassDesc *cld, int dllN, int index, bool load);
		int DllNumber() { return dllNumber; }
		int IsPublic() { return isPublic; }
		Class_ID ClassID() { return classID; }
		TSTR &ClassName() { return name; }
		TSTR &Category() { return category; }
		int GetScroll() {return scroll;}
		void SetScroll(int s) {scroll = s;}
		BOOL PageStateSet() {return pageState != 0x7fffffff;}  // test if pageState has been actually set, default initializes to 0x7fffffff
		BOOL GetPageState(int i) {return (pageState&(1<<i))?TRUE:FALSE;}
		void SetPageState(int i,BOOL state) {if (state) pageState |= (1<<i); else pageState &= ~(1<<i);}
		int UseCount() { return useCount; }
		void IncUseCount () { useCount++; }
	    void SetUseCount(int i) { useCount = i; }
		ClassDesc *CD() { return cd; }
		CoreExport int IsAccType(int accType);
		CoreExport ClassEntry& operator=( const ClassEntry &ce ); 
		int operator==( const ClassEntry &ce ) const { return 0; }
		bool IsLoaded() { return loaded; }
		int ClassNumber() { return classNum; }
		CoreExport ClassDesc *FullCD();
	};

// access type.
#define ACC_PUBLIC 1
#define ACC_PRIVATE 2
#define ACC_ALL (ACC_PUBLIC|ACC_PRIVATE)

typedef ClassEntry* PClassEntry;
typedef Tab<PClassEntry> SubClassTab;


//
// Class SClassUIInfo...
// Allows developer to provide some additional information on a superclass.
// Currently this includes a color, and a method which
// draws a representative image in a Windows DC.
// DrawRepresentation(...) can return false to indicate that no image was drawn.
// DrawRepresentation(...) should cache its image (if applicable) as the method
// is called repeatedly while drawing certain UI components (like the schematic
// view).
//

class SClassUIInfo
	{
	public:

	// A color associated with the superclass.  This is currently used to draw nodes
	// in the schematic view at extreme zoom-outs where it is impossible to draw
	// legible node names.
	CoreExport virtual COLORREF Color(SClass_ID superClassID) { return RGB(128, 128, 128); };

	// Draws an image which represents the superclass (usually an icon) in a rectangle
	// in a given Windows DC.  The implementation should attempt to draw the image as fast
	// as possible as this method is called repeatedly while drawing certain UI
	// components.  Return false if no image was drawn and a generic stand-in image will
	// be used.  The provided "bkColor" is the average background color of the surface on
	// which the image is being drawn.  It can be used, if desired, to antialias the image.
	CoreExport virtual bool DrawRepresentation(SClass_ID superClassID, COLORREF bkColor, HDC hDC, Rect &rect) { return false; }
	};

class SubClassList {
		int iEnum;
		ulong superID;
		SubClassTab subClList;
		SClassUIInfo *uiInfo;
    public:
		CoreExport ~SubClassList();
		SubClassList(ulong sid=0) { superID = sid; uiInfo = NULL; }
		ClassEntry& operator[](int i){ return(*subClList[i]);}
		CoreExport int FindClass(Class_ID subClassID);	  // Get a class by its class ID
		CoreExport int FindClass(const TCHAR *name);    // Get a class by its class name
		CoreExport void AddClass(ClassDesc *cld, int dllNum, int index, bool load); 
		CoreExport int  Count(int accType);
		ulong SuperID() { return superID; }
		int operator==(const SubClassList &sl) {return(0);}

		// Enumerate.
		CoreExport int GetFirst(int accType);	 // get first ClassDesc of accType
		CoreExport int GetNext(int accType);	 // get next ClassDesc of accType (NULL for End)

		int operator==( const SubClassList& lst ) const { return 0; }
		CoreExport int DeleteClass(ClassDesc *cld); 

		// Allows developer to provide some additional information on a superclass.
		// Currently this includes a descriptive string, a color, and a method which
		// draws a representative image in a Windows DC.
		CoreExport void SetUIInfo(SClassUIInfo *uiInfo);

		// Retrieves additional UI related information on a given superclass.  Returns
		// NULL if no superclass information was assigned.
		CoreExport SClassUIInfo *GetUIInfo();
		CoreExport void ReplaceClass(int idx, ClassDesc *cld, int dllNum, int index, bool load); 
	};


typedef SubClassList* PSubClassList;
typedef Tab<PSubClassList> ListOfClassLists;

/* ClassDirectory: A list of SubClassLists, one for each pluggable
   super class */
class ClassDirectory {
	ListOfClassLists cl;
	public:				  
		CoreExport ~ClassDirectory();
		CoreExport SubClassList* GetClassList(SClass_ID superClassID);	
		CoreExport ClassDesc* FindClass(SClass_ID superClassID, Class_ID subClassID);
		CoreExport ClassEntry *FindClassEntry(SClass_ID superClassID, Class_ID subClassID);
		SubClassList& operator[](int i){ return(*cl[i]);}
		CoreExport void AddSuperClass(SClass_ID superClassID);
		// returns 0 if class already exists
		// returns -1 if superclass was unknown
		// returns 1 if class added successfully
		CoreExport int  AddClass(ClassDesc *cdesc, int dllNum, int index, bool load);
		int Count() { return cl.Count(); }	
		CoreExport int DeleteClass(ClassDesc *cdesc);

		// Allows developer to provide some additional information on a superclass.
		// Currently this includes a descriptive string, a color, and a method which
		// draws a representative image in a Windows DC.
		// Function returns true if successful or false if the superclass was not found.
		CoreExport bool SetUIInfoForSClass(SClass_ID superClassID, SClassUIInfo *uiInfo);

		// Retrieves additional UI related information on a given superclass.  Returns
		// NULL if the superclass was not found or if no superclass information was
		// assigned.
		CoreExport SClassUIInfo *GetUIInfoForSClass(SClass_ID superClassID);
	};

/* DllDirectory: A list DllDescriptors, one for every DLL loaded 
   Also contains the ClassDirectory of all classes implemented in these
   DLL's */

typedef DllDesc *PDllDesc;

class DllDir {
	Tab<PDllDesc> dll;    // list of Descriptors for all the loaded DLL's
	ClassDirectory classDir; // All the classes implemented in these DLL's 
	int  AddDll(const DllDesc *dllDesc, bool late);
	public:
		CoreExport ~DllDir();
		CoreExport void UnloadAllDlls();
		int Count() { return dll.Count(); }
		DllDesc& operator[](int i) { return(*dll[i]); }
		CoreExport int LoadDllsFromDir(TCHAR *directory, TCHAR *  wildcard, HWND hwnd=NULL);
		ClassDirectory& ClassDir() { return classDir; }
		CoreExport bool LoadADll(TCHAR *, bool late);
	};


/* DataClassDesc: A ClassDesc for classes in plug-ins but described by
   Registry entries */

class DataClassDesc : public ClassDesc {
public:
	TSTR category;
	DWORD classIDA;
	DWORD classIDB;
	DWORD superClassID;
	TSTR className;
	DWORD isPublic;
	DWORD okToCreate;
	DWORD extCount;
	TSTR ext;
	TSTR shortDesc;
	TSTR longDesc;
	DWORD supportsOptions;
	DWORD capability;
	DWORD inputTypeA;	// For Modifiers
	DWORD inputTypeB;	// For Modifiers
	TSTR internalName;
	TCHAR* internalNamePtr;

	DataClassDesc() : internalNamePtr(NULL) {}
	~DataClassDesc();
	int IsPublic() { return isPublic; }
	void *Create(BOOL loading=FALSE) { return NULL; }
	int BeginCreate(Interface *i) { return 0; }
	int EndCreate(Interface *i) { return 0; }
	const TCHAR *ClassName() { return className; }
	SClass_ID SuperClassID() { return superClassID; }
	Class_ID ClassID() { return Class_ID(classIDA, classIDB); }
	const TCHAR *Category() { return category; }
	BOOL OkToCreate(Interface *i) { return okToCreate; }	
	BOOL HasClassParams() { return FALSE; }
	void EditClassParams(HWND hParent) {}
	void ResetClassParams(BOOL fileReset=FALSE) {}

    // These functions return keyboard shortcut tables that plug-ins can use
    int NumShortcutTables() { return 0; }
    ShortcutTable *GetShortcutTable(int i) { return NULL; }

	// Class IO
	BOOL NeedsToSave() { return FALSE; }
	IOResult Save(ISave *isave) { return IO_OK; }
	IOResult Load(ILoad *iload) { return IO_OK; }

	// bits of dword set indicate corrresponding rollup page is closed.
	// the value 0x7fffffff is returned by the default implementation so the
	// command panel can detect this method is not being overridden, and just leave 
	// the rollups as is.
	DWORD InitialRollupPageState() { return 0x7fffffff; }

	// ParamBlock2-related metadata interface, supplied & implemented in ClassDesc2 (see maxsdk\include\iparamb2.h)
	
	const TCHAR *InternalName() { return internalNamePtr; }
	HINSTANCE HInstance() { return NULL; }
	// access parameter block descriptors for this class
	int NumParamBlockDescs() { return 0; }
	ParamBlockDesc2 *GetParamBlockDesc(int i) { return NULL; }
	ParamBlockDesc2 *GetParamBlockDescByID(BlockID id) { return NULL; }
	void AddParamBlockDesc(ParamBlockDesc2* pbd) {}
	// automatic UI management
	void BeginEditParams(IObjParam *ip, ReferenceMaker* obj, ULONG flags, Animatable *prev) {}
	void EndEditParams(IObjParam *ip, ReferenceMaker* obj, ULONG flags, Animatable *prev) {}
	void InvalidateUI(ParamBlockDesc2* pbd) {}
	// automatic ParamBlock construction
	void MakeAutoParamBlocks(ReferenceMaker* owner) {}
	// access automatically-maintained ParamMaps, by simple index or by associated ParamBlockDesc
	int NumParamMaps() { return 0; }
	IParamMap2 *GetParamMap(int i) { return NULL; }
	IParamMap2 *GetParamMap(ParamBlockDesc2* pbd) { return NULL; }
	// maintain user dialog procs on automatically-maintained ParamMaps
	void SetUserDlgProc(ParamBlockDesc2* pbd, ParamMap2UserDlgProc* proc=NULL) {}
	ParamMap2UserDlgProc *GetUserDlgProc(ParamBlockDesc2* pbd) { return NULL; }

	// Class can draw an image to represent itself graphically...
	bool DrawRepresentation(COLORREF bkColor, HDC hDC, Rect &rect) { return FALSE; }
};


#endif // PLUGIN_H_DEFINED
