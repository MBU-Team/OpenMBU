/**********************************************************************
 *<
	FILE: impexp.h

	DESCRIPTION: Includes for importing and exporting geometry files

	CREATED BY:	Tom Hudson

	HISTORY: Created 26 December 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef _IMPEXP_H_
#define _IMPEXP_H_

// The following work on the specified file or bring up the browser if null
BOOL ImportFile(const TCHAR *buf = NULL, BOOL suppressPrompts=FALSE, Class_ID *importerID=NULL);
BOOL ExportFile(const TCHAR *buf = NULL, BOOL suppressPrompts=FALSE, DWORD options=0, Class_ID *exporterID=NULL);

class ImpInterface;
class ExpInterface;
class Interface;

// Returned by DoImport, DoExport
#define IMPEXP_FAIL 0
#define IMPEXP_SUCCESS 1
#define IMPEXP_CANCEL 2

// SceneImport::ZoomExtents return values
#define ZOOMEXT_NOT_IMPLEMENTED	-1		// The default (uses Preferences value)
#define ZOOMEXT_YES				TRUE	// Zoom extents after import
#define ZOOMEXT_NO				FALSE	// No zoom extents

// The scene import/export classes.  Right now, these are very similar, but this may change as things develop

class SceneImport {
public:
							SceneImport() {};
	virtual					~SceneImport() {};
	virtual int				ExtCount() = 0;					// Number of extemsions supported
	virtual const TCHAR *	Ext(int n) = 0;					// Extension #n (i.e. "3DS")
	virtual const TCHAR *	LongDesc() = 0;					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR *	ShortDesc() = 0;				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR *	AuthorName() = 0;				// ASCII Author name
	virtual const TCHAR *	CopyrightMessage() = 0;			// ASCII Copyright message
	virtual const TCHAR *	OtherMessage1() = 0;			// Other message #1
	virtual const TCHAR *	OtherMessage2() = 0;			// Other message #2
	virtual unsigned int	Version() = 0;					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd) = 0;		// Show DLL's "About..." box
	virtual int				DoImport(const TCHAR *name,ImpInterface *ii,Interface *i, BOOL suppressPrompts=FALSE) = 0;	// Import file
	virtual int				ZoomExtents() { return ZOOMEXT_NOT_IMPLEMENTED; }	// Override this for zoom extents control
	};

// SceneExport::DoExport options flags:
#define SCENE_EXPORT_SELECTED (1<<0)

class SceneExport {
public:
							SceneExport() {};
	virtual					~SceneExport() {};
	virtual int				ExtCount() = 0;					// Number of extemsions supported
	virtual const TCHAR *	Ext(int n) = 0;					// Extension #n (i.e. "3DS")
	virtual const TCHAR *	LongDesc() = 0;					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR *	ShortDesc() = 0;				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR *	AuthorName() = 0;				// ASCII Author name
	virtual const TCHAR *	CopyrightMessage() = 0;			// ASCII Copyright message
	virtual const TCHAR *	OtherMessage1() = 0;			// Other message #1
	virtual const TCHAR *	OtherMessage2() = 0;			// Other message #2
	virtual unsigned int	Version() = 0;					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd) = 0;		// Show DLL's "About..." box
	virtual int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0) = 0;	// Export file
	virtual BOOL			SupportsOptions(int ext, DWORD options) {return FALSE;} // Returns TRUE if all option bits set are supported for this extension
	};


#endif // _IMPEXP_H_
