//***************************************************************************
// SystemUtilities.h
// A collection of system utilities
// Christer Janson
// Discreet, A division of Autodesk, Inc.
// San Francisco, CA - March 27, 2000

UtilExport bool	IsDebugging();			// Are we running under a debugger?
UtilExport int	NumberOfProcessors();	// Number of processors in the system.
UtilExport bool	IsWindows9x();			// Are we running on Windows 9x?
UtilExport bool	IsWindows98or2000();	// Are we running on Windows 98 or 2000?
UtilExport int	GetScreenWidth();		// The width of the screen (including multiple monitors)
UtilExport int	GetScreenHeight();		// The height of the screen (including multiple monitors)
