
#include "msLib.h"
#include "msPlugIn.h"

#include "DTSMilkshapeShape.h"

//! Implements the Milkshape exporter plugin

using namespace DTS ;

class cDTSPlugin : public cMsPlugIn
{
   public:

      cDTSPlugin() ;

      //! Gets the plugin type (exporter)
      virtual int          GetType() ;

      //! Gets the plugin title ("V12 Exporter")
      virtual const char * GetTitle() ;

      //! Does all the work (displays the settings dialog box and does the export)
      virtual int          Execute (msModel * pModel) ;

      //! We need the hInstance of the DLL. The WinMain function call this.
      static void setInstance(HINSTANCE h) { hInstance = h ; }

      //! The settings dialog procedure
      BOOL settingsDialog (HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) ;

   private:

      static HINSTANCE hInstance ;

      MilkshapeShape::ImportConfig config ;
      MilkshapeShape::ImportConfig savedConfig ;

      msModel * model ;
} ;