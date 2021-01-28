//-----------------------------------------------------------------------------
// -------------------
// File ....: gupapi.h
// -------------------
// Author...: Gus Grubba
// Date ....: September 1998
//
// History .: Sep, 30 1998 - Started
//
//-----------------------------------------------------------------------------

//-- GUP Interface class

class GUPInterface : public InterfaceServer {

	public:

		virtual HINSTANCE	AppInst			() = 0;
		virtual HWND		AppWnd			() = 0;
		virtual DllDir*		AppDllDir		() = 0;
		virtual Interface*	Max				() = 0;
		virtual int			EnumTree		( ITreeEnumProc *proc ) = 0;

};

