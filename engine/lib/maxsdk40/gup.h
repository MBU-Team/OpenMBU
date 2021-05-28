//-----------------------------------------------------------------------------
// ----------------
// File ....: gup.h
// ----------------
// Author...: Gus Grubba
// Date ....: September 1998
//
// History .: Sep, 30 1998 - Started
//
//-----------------------------------------------------------------------------
		
#ifndef	GUP_H_DEFINED
#define	GUP_H_DEFINED

#include <gupapi.h>

//-- Just to make it shorter

#define	dVirtual GUPExport virtual

//-- Forward References

class GUP;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Plug-Ins Handler
//
	
class GUPHandler : public InterfaceServer {

		//-- DLL Handler ----------------------------------
		
		ClassDesc*	cd;
		Class_ID	id;
		GUP*		instance;
		
	public:

								GUPHandler		( );

		GUPExport	void		SetCD			( ClassDesc *dll )	{ cd = dll;	}
		GUPExport	ClassDesc*	GetCD			( )					{ return cd;}
		GUPExport	void		SetID			( Class_ID cid )	{ id = cid;	}
		GUPExport	Class_ID	GetID			( )					{ return id;}
		GUPExport	void		SetInstance		( GUP *ins )		{ instance = ins;  }
		GUPExport	GUP*		GetInstance		( )					{ return instance; }

		GUPExport	GUP*		Gup				( )					{ return instance; }

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- List of Loaded Plug-Ins
//
	
class GUPList: public Tab<GUPHandler> {

	public:

		GUPExport			GUPList				( )	{ ; }
		GUPExport	int		FindGup				( const Class_ID id );
		GUPExport	GUP*	CreateInstance		( const Class_ID id );

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Global Utility Plug-In Class
//

class GUP : public InterfaceServer {
	
	public:
	
		GUPExport	GUP								( );
		dVirtual	~GUP							( );

		//-----------------------------------------------------------
		//-- Plug-In Service Methods
		
		dVirtual	HINSTANCE		MaxInst			( );
		dVirtual	HWND			MaxWnd			( );
		dVirtual	DllDir*			MaxDllDir		( );
		dVirtual	Interface*		Max				( );
		dVirtual	BitmapManager*	Bmi				( );
		dVirtual	int				EnumTree		( ITreeEnumProc *proc );

		//-----------------------------------------------------------
		//-- Plug-In Service Methods (Scripting)
		
		dVirtual	bool			ExecuteStringScript	( TCHAR *string );
		dVirtual	bool			ExecuteFileScript	( TCHAR *file );

		//-----------------------------------------------------------
		//-----------------------------------------------------------
		//-- Plug-In Implementation Methods
		
		//-----------------------------------------------------------
		// Start() is called at boot time. If the plug-in
		// desires to remain loaded, it returns GUPRESULT_KEEP. If
		// it returns GUPRESULT_NOKEEP, it will be discarded. If it
		// returns GUPRESULT_ABORT MAX will be shut down.

		#define GUPRESULT_KEEP		0x00
		#define GUPRESULT_NOKEEP	0x01
		#define GUPRESULT_ABORT		0x03

		dVirtual	DWORD			Start		( ) = 0;
		
		//-------------------------------------------------
		// Stop is called prior to discarding the plug-in
		// for persistent plug-ins (i.e. only those that 
		// returned GUPRESULT_KEEP for Start() above).
		
		dVirtual	void			Stop		( ) = 0;

		//-------------------------------------------------
		// Control is an entry point for external access to
		// GUP plug-ins. For instance, Utility plugins can
		// invoke their GUP plugin counterpart and have 
		// direct access to them.
		//
		
		dVirtual	DWORD			Control		( DWORD parameter ) { return 0;}

		//-------------------------------------------------
		// Optional Methods for saving the plug-in state
		// within a scene file.

		dVirtual	IOResult		Save		( ISave *isave );
		dVirtual	IOResult		Load		( ILoad *iload );
		
		// RK: 06/28/00, added it to support static instances
		dVirtual	void			DeleteThis	( ) { }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Main GUP Manager Class
//
//

class GUPManager    {
	
		GUPInterface*	iface;

		int				InitList		( );
		bool			listed;
		
	public:
	
		GUPExport					GUPManager		( GUPInterface *i );
		GUPExport					~GUPManager		( );
		
		GUPList		gupList;
		
		GUPExport	bool			Ready			( );
		GUPExport	bool			Init			( );
		GUPExport	GUPInterface*	Interface		( ) { return iface; }
		GUPExport	IOResult		Save			( ISave *isave );
		GUPExport	IOResult		Load			( ILoad *iload );

};

//-----------------------------------------------------------------------------
//-- Exported
//

extern GUPExport	void			OpenGUP			(  GUPInterface *i );
extern GUPExport	void			CloseGUP		(  );
extern GUPExport	GUPManager*		gupManager; 
extern GUPExport	GUP*			OpenGupPlugIn	( const Class_ID id); 

//-----------------------------------------------------------------------------
//-- Cleanup

#undef	dVirtual
#endif	GUP_H_DEFINED

//-- EOF: gup.h ---------------------------------------------------------------
