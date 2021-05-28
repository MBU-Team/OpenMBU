/*	
 *		iDragAndDrop.h - SDK API to desktop drag-and-drop services in MAX
 *
 *			Copyright © Autodesk, Inc, 2000.  John Wainwright.
 *
 */

#ifndef _H_IDRAGNDROP
#define _H_IDRAGNDROP

#include "iFnPub.h"
#include "iMacroScript.h"

class IDragAndDropMgr; 
class DragAndDropHandler;
class DropType;

// Core interface to the OLE drag-and-drop manager

// utility Tab<> specialization to hold URL strings
class URLTab : public Tab<TCHAR*>
{
public:
	BOOL downloaded;  // set to indicate URL package has been downloaded & names now reflect local copies

	URLTab() { downloaded = FALSE; }
	~URLTab() { Clear(); }
	URLTab& operator=(const Tab<TCHAR*>& tb) {
		for (int i = 0; i < tb.Count(); i++)
		{
			TCHAR* u = new TCHAR [strlen(tb[i]) + 1];
			_tcscpy(u, tb[i]);
			Append(1, &u);
		}
		downloaded = FALSE;
		return *this;
	}	
	URLTab& operator=(const URLTab& tb) {
		for (int i = 0; i < tb.Count(); i++)
		{
			TCHAR* u = new TCHAR [strlen(tb[i]) + 1];
			_tcscpy(u, tb[i]);
			Append(1, &u);
		}
		downloaded = tb.downloaded;
		return *this;
	}	
	void Add(TCHAR* url) { 
		TCHAR* u = new TCHAR [strlen(url) + 1];
		_tcscpy(u, url);
		Append(1, &u);
	}
	void Change(int i, TCHAR* url) {
		if (i < Count() && (*this)[i] != NULL)
			delete (*this)[i];
		TCHAR* u = new TCHAR [strlen(url) + 1];
		_tcscpy(u, url);
		(*this)[i] = u;		
	}
	void Clear() {
		for (int i = 0; i < Count(); i++)
			delete (*this)[i];
		SetCount(0);
		downloaded = FALSE;
	}
};

// DnD Manager interface ID
#define DND_MGR_INTERFACE   Interface_ID(0x51163ddb, 0x2a4f1619)
inline IDragAndDropMgr* GetDragAndDropMgr() { return (IDragAndDropMgr*)GetCOREInterface(DND_MGR_INTERFACE); }

// function IDs 
enum { dndmgr_enableDandD, 
	   dndmgr_globalEnableDnD, 
	   dndmgr_isEnabled, 
	   dndmgr_dropPackage, 
	   dndmgr_downloadDirectory, 
	   dndmgr_downloadPackage,
	   dndmgr_downloadUrlToDisk,
	   dndmgr_importContextNode,
	}; 

// class IDragAndDropMgr
// manager interface 
class IDragAndDropMgr : public FPStaticInterface 
{
public:
	virtual void    EnableDandD(BOOL flag)=0;   // global enabled/disable
	virtual BOOL	IsEnabled()=0;
	virtual BOOL	EnableDandD(HWND hwnd, BOOL flag, DragAndDropHandler* handler = NULL)=0; // per window
	virtual BOOL	DropPackage(HWND hwnd, POINT& point, URLTab& package)=0; 
	virtual BOOL	DownloadPackage(URLTab& package, TCHAR* directory, HWND hwnd = NULL, bool showProgress = false)=0; 
	virtual TCHAR*	GetDownloadDirectory()=0;
	virtual int		NumHandlers(HWND hwnd)=0;			// iterate handlers for given window
	virtual DragAndDropHandler* GetHandler(HWND hwnd, int i)=0;
	virtual bool	DownloadUrlToDisk(HWND hwnd, TCHAR* url, TCHAR* fileName, DWORD dlgflags=0)=0;
	virtual INode*	ImportContextNode()=0;
}; 

// class DragAndDropHandler
// virtual base class for DnD handlers 
// subclass instances can be registered with DnD Mgr to specialize 
// DnD handling for individual windows
class DragAndDropHandler : public InterfaceServer
{
public:
	DragAndDropHandler() { if (dndMgr == NULL) dndMgr = GetDragAndDropMgr(); }

	// low-level IDropTarget methods, override these to get low-level control
	//  the default implementations parse the dropping dataObject and call the MAX-specific DnD methods below
	CoreExport virtual HRESULT DragEnter(HWND window, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	CoreExport virtual HRESULT Drop(HWND window, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	CoreExport virtual HRESULT DragOver(HWND window, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
	CoreExport virtual HRESULT DragLeave(HWND window) { return E_FAIL; }

	// high-level drag-and-drop operations, the dataObject has been decoded and the drop-type parsed
	virtual HRESULT DragEnter(HWND window, DropType* type, DWORD grfKeyState, POINT& pt, DWORD* pdwEffect) { return E_FAIL; }
	virtual HRESULT Drop(HWND window, DropType* type, DWORD grfKeyState, POINT& pt, DWORD* pdwEffect) { return E_FAIL; }
	virtual HRESULT DragOver(HWND window, DWORD grfKeyState, POINT& pt, DWORD * pdwEffect) { return E_FAIL; }
	// DragLeave() high- & low-level are the same:  virtual HRESULT DragLeave(HWND window) { return E_FAIL; }

	// called when d&d manager starts & stops managing a window for this handler
	virtual void Acquire() { }; 
	virtual void Release() { }; 

protected:
	DropType*  current_droptype;  // parsed drop type for current DnD operation
	CoreExport static IDragAndDropMgr* dndMgr; // cached pointer to DnD manager
};

// class DropClipFormat
// base class for the various supported clipboard formats.
// Distinguished instance of each subclass represents a
// particular IDataObject clip format or package of related formats
// that can be accepted by MAX's various windows.  Prime responsibility
// of each is to recognize its presence in a dropped (pasted?) IDataObject
// and to parse the data object into one of the acceptable DropTypes.
// 
class DropClipFormat : public InterfaceServer
{
protected:
	CoreExport static Tab<DropClipFormat*> clipFmts;  // table of supported clip formats
public:
	// each DropClimFormat instance created is kept in the clipFmts table
	DropClipFormat() { DropClipFormat* cf = this; clipFmts.Append(1, &cf); }

	// used primarily by the DragEnter() handler to find the DropClipFormat
	// corresponding to the currently dropping IDataObject
	CoreExport static DropClipFormat* FindClipFormat(IDataObject* pDataObject);

	// specialized by individual clipformats to detect its format(s) present in 
	// the given IDataObject
	virtual bool CheckClipFormat(IDataObject* pDataObject) { return false; }
	
	// specialized by individual clip format types to parse IDataObject
	// into appropriate DropType
	virtual DropType* ParseDataObject(IDataObject* pDataObject) { return NULL; }
};

// built-in clip formats

// iDrop package URL
class IDropPackageClipFmt : public DropClipFormat
{
public:
	bool CheckClipFormat(IDataObject* pDataObject);
	DropType* ParseDataObject(IDataObject* pDataObject);
};

// VIZable file URL
class VIZableClipFmt : public DropClipFormat
{
public:
	bool CheckClipFormat(IDataObject* pDataObject);
	DropType* ParseDataObject(IDataObject* pDataObject);
};

// internal dropScript
class DropScriptClipFmt : public DropClipFormat
{
public:
	bool CheckClipFormat(IDataObject* pDataObject);
	DropType* ParseDataObject(IDataObject* pDataObject);
};


// class DropType
// base class for dropable content types
// Distinguished instances of subclasses represent different types of drop content, 
// such as a file distinguished by file suffix or a scene object
// The active DropClipFormat parses dropped IDataObject into one
// of these dropped types, filling its data members with appropriate
// guff from the data object.
class DropType : public IDropSource, public IDataObject
{
protected:
	CoreExport static IDragAndDropMgr* dndMgr;	// cached pointer to DnD manager
	CoreExport static bool dragging;			// drop source state...
	CoreExport static POINT startPt;
	CoreExport static WPARAM startKeyState;
	CoreExport static HWND startWnd;
	CoreExport static bool loaded;				// flags if curent package already downloaded

public:
	// currently dropping data object
	CoreExport static IDataObject* current_dataobject;

	DropType() { if (dndMgr == NULL) dndMgr = GetDragAndDropMgr(); }
	
	// clears current parsed drop data
	static void Init() { current_dataobject = NULL; loaded = false; }

	// DropType code access, provides an integer code specific
	// to the droptype
	virtual int TypeCode()=0;
	virtual bool IsDropType(int code) { return code == TypeCode(); }
	
	// ------- drop target methods & members --------------

	// perform any droptype-specific load prior to using the data, eg 
	// downloading URLs to local machine
	virtual bool Load(bool showProgress = true) { return true; }

	// dropeffect currently supported by accepted dropping type
	virtual DWORD DropEffect() { return DROPEFFECT_MOVE; }

	// -------- drop source methods and members -----------------

	// from IUnknown
	CoreExport STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
	CoreExport STDMETHODIMP_(ULONG) AddRef(void)  { return 1; }
	CoreExport STDMETHODIMP_(ULONG) Release(void) { return 1; }
	
	// from IDataObject
	CoreExport STDMETHODIMP GetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium) { return E_UNEXPECTED; }
	CoreExport STDMETHODIMP GetDataHere(FORMATETC* pFormatetc, STGMEDIUM* pmedium) { return E_UNEXPECTED; }
	CoreExport STDMETHODIMP QueryGetData(FORMATETC* pFormatetc) { return E_UNEXPECTED; }
	CoreExport STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pFormatetcIn, FORMATETC* pFormatetcOut);
	CoreExport STDMETHODIMP SetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium, BOOL fRelease);
	CoreExport STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatetc);
	CoreExport STDMETHODIMP DAdvise(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
	CoreExport STDMETHODIMP DUnadvise(DWORD dwConnection);
	CoreExport STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise);

	// from IDropSource
	CoreExport STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	CoreExport STDMETHODIMP GiveFeedback(DWORD dwEffect) { return DRAGDROP_S_USEDEFAULTCURSORS; }

	// drop start checking methods 
	CoreExport virtual void InitDragDropCheck(LPARAM mousePt, WPARAM keyState, HWND hwnd);
	CoreExport virtual void CancelDragDropCheck();
	CoreExport virtual bool DragDropCheck(LPARAM mousePt, WPARAM keyState, DWORD allowedEffects);
	CoreExport virtual bool ReadyToDrag(){ return false; }
};

// built-in DropType codes
//   3rd-party defined types should use random 32 bit codes above 0x10000000

#define FILE_DROPTYPE				0x00000001
#define DROPSCRIPT_DROPTYPE			0x00000002
#define SCENEFILE_DROPTYPE			0x00000003
#define IMAGEFILE_DROPTYPE			0x00000004
#define IMPORTFILE_DROPTYPE			0x00000005
#define SCRIPTFILE_DROPTYPE			0x00000006
#define DROPSCRIPTFILE_DROPTYPE		0x00000007
#define BITMAP_DROPTYPE				0x00000008
#define MSZIPFILE_DROPTYPE			0x00000009

// class FileDropType
//   intermediate base class for drop content in the form of a 
//   package of file names or URLS 
class FileDropType : public DropType
{
protected:
	CoreExport static Tab<FileDropType*> fileDropTypes;  // table of supported file drop types
	CoreExport static TSTR download_directory;			  // cache for default download directory

	// URL download utilities
	CoreExport static bool CheckForCachedFile(TCHAR* filename);
	CoreExport static bool IsInternetCachedFile(const TCHAR* filename);
	CoreExport static bool	AppendUrlFilename(const TCHAR* szUrl, TCHAR* szPathname, bool& is_URL);

public:
	CoreExport static URLTab current_package;

	FileDropType() { FileDropType* dt = this; fileDropTypes.Append(1, &dt); }
	static void Init() { current_package.Clear(); DropType::Init(); }

	// From DropType
	int TypeCode() { return FILE_DROPTYPE; }
	bool IsDropType(int code) { return code == TypeCode() || code == FILE_DROPTYPE; }

	// ------- drop target methods & members --------------

	CoreExport bool Load(bool showProgress = true);
	
	// global finder of assoicated droptype given filename (or pDataObject)
	CoreExport static FileDropType* FindDropType(TCHAR* filename, IDataObject* pDataObject = NULL);

	// specialize this for each droppable file type to provide type detector
	virtual bool	  CheckDropType(TCHAR* filename) { return false; } 

	// package download utilities
	CoreExport static bool   DownloadPackage(URLTab& package, TCHAR* szDirectory, HWND hwnd = NULL, bool showProgress = true);
	CoreExport static TCHAR* GetDownloadDirectory();
	CoreExport static bool DownloadUrlToDisk(HWND hwnd, TCHAR* szUrl, TCHAR* szPathname, DWORD flags=0);

	// -------- drop source methods and members -----------------

	// from IDataObject
	STDMETHODIMP GetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium) { return E_UNEXPECTED; }
	STDMETHODIMP GetDataHere(FORMATETC* pFormatetc, STGMEDIUM* pmedium) { return E_UNEXPECTED; }
	STDMETHODIMP QueryGetData(FORMATETC* pFormatetc) { return E_UNEXPECTED; }

};

// class DropScriptDropType
//   intermediate base class for drop content in the form of a 
//   dropScript 
class DropScriptDropType : public DropType
{
public:
	CoreExport static MacroEntry* current_dropscript;

	// From DropType
	int TypeCode() { return DROPSCRIPT_DROPTYPE; }
	bool IsDropType(int code) { return code == TypeCode() || code == DROPSCRIPT_DROPTYPE; }
	DWORD DropEffect() { return DROPEFFECT_MOVE; }
	
	// ------- drop target methods & members --------------

	// compile & run support methods
	CoreExport BOOL CompileDropScript(TCHAR* filename);
	CoreExport BOOL RunDropScriptDragEnter(FPParams* params);
	CoreExport BOOL RunDropScriptDragOver(FPParams* params);
	CoreExport BOOL RunDropScriptDrop(FPParams* params);

	// -------- drop source methods and members -----------------

	// from IDataObject
	CoreExport STDMETHODIMP GetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium);
	CoreExport STDMETHODIMP GetDataHere(FORMATETC* pFormatetc, STGMEDIUM* pmedium);
	CoreExport STDMETHODIMP QueryGetData(FORMATETC* pFormatetc);

	// drop start checking methods 
	void InitDragDropCheck(MacroEntry* dropscript, LPARAM mousePt, WPARAM keyState, HWND hwnd)
	{
		DropType::InitDragDropCheck(mousePt, keyState, hwnd);
		current_dropscript = dropscript;
	}
	bool ReadyToDrag() { return current_dropscript != NULL; }
};

// the built-in type classes

// first the file types, usually sourced by the iDrop active-X control on a 
// web page or by draggin files from the Windows desktop/explorer...

// .max scene file
class SceneFileDropType : public FileDropType
{
public:
	// From DropType
	int TypeCode() { return SCENEFILE_DROPTYPE; }
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// image files (.bmp, .tga, etc.)
class ImageFileDropType : public FileDropType
{
public:
	// From DropType
	int TypeCode() { return IMAGEFILE_DROPTYPE; }
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// importable files (.3ds, .dxf, etc.)
class ImportFileDropType : public FileDropType
{
public:
	// From DropType
	int TypeCode() { return IMPORTFILE_DROPTYPE; }
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// script files (.ms, .mse, .mcr)
class ScriptFileDropType : public FileDropType
{
public:
	// From DropType
	int TypeCode() { return SCRIPTFILE_DROPTYPE; }
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// drop script files (.ds, .dse)
class DropScriptFileDropType : public FileDropType, public DropScriptDropType
{
public:
	// From DropType
	int TypeCode() { return DROPSCRIPTFILE_DROPTYPE; }
	bool Load(bool showProgress = true);
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// script zip package files (.mzp)
class MSZipPackageFileDropType : public FileDropType, public DropScriptDropType
{
public:
	TSTR extract_dir;
	TSTR drop_file; 
	DropType* drop_file_type;   // if drop_file is not a dropScript

	// From DropType
	int TypeCode() { return MSZIPFILE_DROPTYPE; }
	bool Load(bool showProgress = true);
	
	// From FileDropType
	CoreExport bool CheckDropType(TCHAR* filename);
};

// MAX-internal types

// bitmap
class BitmapDropType : public DropType
{
public:
	// From DropType
	int TypeCode() { return BITMAP_DROPTYPE; }
	
};

// built-in type instances
extern CoreExport SceneFileDropType sceneFileDropType; 
extern CoreExport ImageFileDropType imageFileDropType; 
extern CoreExport ScriptFileDropType scriptFileDropType; 
extern CoreExport DropScriptFileDropType dropScriptFileDropType; 
extern CoreExport DropScriptDropType dropScriptDropType; 
extern CoreExport BitmapDropType bitmapDropType; 
extern CoreExport MSZipPackageFileDropType msZipPackageFileDropType; 


#endif

