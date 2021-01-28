
#include <windows.h>
#include <commctrl.h>

#include <fstream> 
#include <cstdio>
#include <cassert>

#include "resource.h"

#pragma warning ( disable: 4786 )
#include "DTSPlugin.h"
#include "DTSInterpolation.h"

using namespace DTS ;

HINSTANCE cDTSPlugin::hInstance ;

cDTSPlugin::cDTSPlugin()
{
   model = 0 ;
}

const char * cDTSPlugin::GetTitle()
{
#ifdef DEBUG
   return "Torque Shader Engine DTS (Debug)..." ;
#else
   return "Torque Shader Engine DTS..." ;
#endif
}

int cDTSPlugin::GetType()  
{
   return cMsPlugIn::eTypeExport ;
}

// -----------------------------------------------------------------------

bool extractSequence(char * name,MilkshapeShape::ImportConfig::Sequence* seq)
{
   // Seq: ambient=2-3, cyclic, fps=2
   if (strncmp(name,"seq:",4))
      return false;

   bool named = false;
   char *ptr = name+4;
   for (char *tok = strtok(ptr,","); tok != 0; tok = strtok(0,","))
   {
      // Strip lead/trailing white space
      for (; isspace(*tok); tok++);
      for (char* itr = tok + strlen(tok)-1; itr > tok && isspace(*itr); itr--)
         *itr = 0;

      //
      unsigned int fps;
      char buff[256],start[50],end[50];
      if (sscanf(tok,"%[a-zA-Z0-9]=%[0-9]-%[0-9]",buff,start,end) == 3)
      {
         seq->name = buff;
         // Let the user specify start and end frames "base" 1, the
         // structure requires "base" 0, and end to be one past the last
         // frame.
         seq->start = atoi(start) - 1;
         seq->end = atoi(end);
         named = true;
      }
      else
         if (sscanf(tok,"fps=%d",&fps))
            seq->fps = fps;
      else
         if (!strcmp(tok,"cyclic"))
            seq->cyclic = true;
   }

   if (!named)
   {
      char buff[512];
      sprintf(buff,"Malformed sequence material: %s",name);
      MessageBox (NULL,buff,"Torque Shader Engine (DTS) Exporter", MB_ICONSTOP | MB_OK) ;
   }
   return named;
}

bool extractOptions(char * name,MilkshapeShape::ImportConfig* config)
{
   // opt: size=%d, size=%d, scale=%f, cyclic, fps=%d
   if (strncmp(name,"opt:",4))
      return false;

   bool named = false;
   char *ptr = name+4;
   for (char *tok = strtok(ptr,","); tok != 0; tok = strtok(0,","))
   {
      // Strip lead/trailing white space
      for (; isspace(*tok); tok++);
      for (char* itr = tok + strlen(tok)-1; itr > tok && isspace(*itr); itr--)
         *itr = 0;

      //
      int iv;
      float fv;
      if (sscanf(tok,"scale=%f",&fv))
         config->scaleFactor = fv;
      else
         if (sscanf(tok,"size=%d",&iv))
            config->minimumSize = iv;
      else
         if (sscanf(tok,"fps=%d",&iv))
            config->animationFPS = iv;
      else
         if (!strcmp(tok,"cyclic"))
            config->animationCyclic = true;
   }
   return named;
}



// --------------------------------------------------------------------------
//  Settings dialog box
// --------------------------------------------------------------------------

static cDTSPlugin * currentPlugin = 0 ;

static BOOL CALLBACK dialogProc (HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
   assert (currentPlugin != 0) ;

   return currentPlugin->settingsDialog(wnd, msg, wparam, lparam) ;
}

BOOL cDTSPlugin::settingsDialog (HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
   char buffer[512] ;
   int  i ;

   switch (msg)
   {
      case WM_INITDIALOG:
      {
         savedConfig = config ;

         // Initialize edit controls

         sprintf (buffer, "%g", config.scaleFactor) ;
         SendDlgItemMessage (wnd, IDC_FACTOR, WM_SETTEXT, 0, (LPARAM)&buffer) ;
         EnableWindow (GetDlgItem(wnd, IDC_FACTOR), false) ;

         sprintf (buffer, "%d", config.minimumSize) ;
         SendDlgItemMessage (wnd, IDC_MINSIZE, WM_SETTEXT, 0, (LPARAM)&buffer) ;
         EnableWindow (GetDlgItem(wnd, IDC_MINSIZE), false) ;

         // Initialize check boxes

         SendDlgItemMessage (wnd, IDC_CHECK_MATERIALS, 
                             BM_SETCHECK, config.withMaterials, 0L) ;
         SendDlgItemMessage (wnd, IDC_CHECK_COLLVISIBLE, 
                             BM_SETCHECK, config.collisionVisible, 0L) ;
         SendDlgItemMessage (wnd, IDC_CHECK_ANIMATION, 
                             BM_SETCHECK, config.animation, 0L) ;

         // Only enable the mesh option if there are meshes called "Collision"
         bool collisionMesh = false;
         for (i = 0 ; i < msModel_GetMeshCount(model) ; i++)
         {
            msMesh * mesh = msModel_GetMeshAt (model, i) ;
            msMesh_GetName (mesh, buffer, 512) ;
            if (!_strnicmp (buffer, "Collision", 9))
               collisionMesh = true;
         }
         EnableWindow (GetDlgItem(wnd, IDC_COL_MESH),collisionMesh) ;
         if (!collisionMesh && config.collisionType == Shape::C_Mesh)
            config.collisionType = Shape::C_None;

         // Initialize radio buttons
 
         switch (config.collisionType)
         {
            case Shape::C_BBox:
               SendDlgItemMessage (wnd, IDC_COL_BBOX, BM_SETCHECK, 1, 0L) ;      break ;
            case Shape::C_Cylinder:
               SendDlgItemMessage (wnd, IDC_COL_CYLINDER, BM_SETCHECK, 1, 0L) ;  break ;
            case Shape::C_Mesh:
               SendDlgItemMessage (wnd, IDC_COL_MESH, BM_SETCHECK, 1, 0L) ;      break ;
            default:
               SendDlgItemMessage (wnd, IDC_COL_NONE, BM_SETCHECK, 1, 0L) ;      break ;
         }
         
         // Disable/enable controls 
         
         switch (config.collisionType)
         {
         case Shape::C_Mesh:
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_MESHES), TRUE) ;
            break ;
         case 0:
         case Shape::C_BBox:
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), FALSE) ;
            EnableWindow (GetDlgItem(wnd, IDC_MESHES), FALSE) ;
            break ;
         default:
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), TRUE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), TRUE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), TRUE) ;
            EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), TRUE) ;
            EnableWindow (GetDlgItem(wnd, IDC_MESHES), FALSE) ;
            break ;
         }

         // Animation Settings

         sprintf (buffer, "%d", config.animationFPS) ;
         SendDlgItemMessage (wnd, IDC_FPS, WM_SETTEXT, 0, (LPARAM)&buffer) ;
         SendDlgItemMessage (wnd, IDC_CHECK_CYCLIC, BM_SETCHECK, config.animationCyclic, 0L) ;
         EnableWindow (GetDlgItem(wnd, IDC_FPS), false) ;
         EnableWindow (GetDlgItem(wnd, IDC_ANIMATION), false) ;
         EnableWindow (GetDlgItem(wnd, IDC_CHECK_CYCLIC), false) ;

         // Initialize trackbar

         SendDlgItemMessage (wnd, IDC_COMPLEXITY, TBM_SETRANGE, FALSE, MAKELONG(0,10)) ;
         SendDlgItemMessage (wnd, IDC_COMPLEXITY, TBM_SETPOS, TRUE, (int)(10.0f * config.collisionComplexity)) ;

         // Initialize Animation Sequence list
         struct  {
            char* name;
            int width;
         } columns[5] =  {
             { "Name",  100 },
             { "Start", 40 },
             { "End",   40 },
             { "FPS",   40 },
             { "Cylic", 40 }
         };
         LVCOLUMN lvc;
         lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
         for (int ic = 0; ic < 5; ic++)
         {
            // Columns
            lvc.iSubItem = ic;
            lvc.pszText = columns[ic].name;
            lvc.cx = columns[ic].width;
            lvc.fmt = (!ic)? LVCFMT_LEFT: LVCFMT_CENTER;
            ListView_InsertColumn(GetDlgItem(wnd, IDC_SEQUENCE_LIST), ic, &lvc);
         }

         LVITEM lvi;
         lvi.mask = LVIF_TEXT;
         for (int sc = 0; sc < config.sequence.size(); sc++)
         {
            // List items
            MilkshapeShape::ImportConfig::Sequence& si = config.sequence[sc];

            lvi.iItem = sc;
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<char*>(si.name.data());
            ListView_InsertItem(GetDlgItem(wnd, IDC_SEQUENCE_LIST), &lvi);

            char buffer[256];
            lvi.pszText = buffer;
            lvi.iSubItem = 1;
            sprintf(buffer,"%d",si.start + 1);
            ListView_SetItem(GetDlgItem(wnd, IDC_SEQUENCE_LIST), &lvi);

            lvi.iSubItem = 2;
            sprintf(buffer,"%d",si.end);
            ListView_SetItem(GetDlgItem(wnd, IDC_SEQUENCE_LIST), &lvi);

            lvi.iSubItem = 3;
            sprintf(buffer,"%d",si.fps);
            ListView_SetItem(GetDlgItem(wnd, IDC_SEQUENCE_LIST), &lvi);

            lvi.iSubItem = 4;
            sprintf(buffer,"%s",si.cyclic? "true": "false");
            ListView_SetItem(GetDlgItem(wnd, IDC_SEQUENCE_LIST), &lvi);
         }

         break ;
      }

      case WM_COMMAND:

         // Handle check boxes 

         if (HIWORD(wparam) == BN_CLICKED)
         {
            if (LOWORD(wparam) == IDC_CHECK_MATERIALS)
            {
               config.withMaterials = 
                  (SendDlgItemMessage (wnd, IDC_CHECK_MATERIALS, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            }
            if (LOWORD(wparam) == IDC_CHECK_ANIMATION)
            {
               config.animation = 
                  (SendDlgItemMessage (wnd, IDC_CHECK_ANIMATION, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            }
            if (LOWORD(wparam) == IDC_CHECK_CYCLIC)
            {
               config.animationCyclic = 
                  (SendDlgItemMessage (wnd, IDC_CHECK_CYCLIC, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            }
            if (LOWORD(wparam) == IDC_CHECK_COLLVISIBLE)
            {
               config.collisionVisible = 
                  (SendDlgItemMessage (wnd, IDC_CHECK_COLLVISIBLE, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            }
         }

         // Handle radio buttons

         if (HIWORD(wparam) == BN_CLICKED)
         {
            if (LOWORD(wparam) == IDC_COL_BBOX)
               config.collisionType = Shape::C_BBox ;
            if (LOWORD(wparam) == IDC_COL_CYLINDER)
               config.collisionType = Shape::C_Cylinder ;
            if (LOWORD(wparam) == IDC_COL_MESH)
               config.collisionType = Shape::C_Mesh ;
            if (LOWORD(wparam) == IDC_COL_NONE)
               config.collisionType = Shape::C_None ;

            // Disable/enable controls 

            switch (config.collisionType)
            {
               case Shape::C_Mesh:
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_MESHES), TRUE) ;
                  break ;
               case 0:
               case Shape::C_BBox:
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), FALSE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_MESHES), FALSE) ;
                  break ;
               default:
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITY), TRUE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYLABEL), TRUE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMIN), TRUE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_COMPLEXITYMAX), TRUE) ;
                  EnableWindow (GetDlgItem(wnd, IDC_MESHES), FALSE) ;
                  break ;
            }
         }

         // Handle buttons at the bottom of the dialog box

         if (wparam == IDCANCEL)
         {
            config = savedConfig ;
            EndDialog (wnd, IDCANCEL) ;
            return TRUE ;
         }

         if (wparam == IDOK)
         {
            EndDialog (wnd, IDOK) ;
            return TRUE ;
         }

         if (wparam == IDABOUT)
         {
            MessageBox (wnd, 
               "Torque Shader Engine (DTS) Exporter\n\r"
               "Version 1.3 \n\r"
               "(C) GarageGames & José Luis Cebrián 2001\n\r"
               "____________________________________________________________\n\r\n\r"
               "This is a work in progress, many DTS features are currently not yet supported.\n\r"
               "Please check the www.garagegames.com web site for the latest information on this\n\r"
               "exporter or the Torque engine in general.\n\r"
               "\n\r"
               "If you make the collision meshes visible, they are assigned a texture called\n\r"
               "\"collision_mesh\".  If this JPG, or PNG texture does not exists in the .DTS\n\r"
               "directory, the collision mesh will appear grey in the engine.",
               "Torque Shader Engine (DTS) Exporter",
               MB_OK | MB_ICONINFORMATION) ;
            return TRUE ;
         }
         break ;

      case WM_NOTIFY:

         // Handle spin buttons
         
         if (lparam)
         {
            LPNMHDR hdr = (LPNMHDR)lparam ;
            if (hdr->code == UDN_DELTAPOS)
            {
               if (hdr->idFrom == IDC_FACTORSPIN)
               {
                  LPNMUPDOWN delta = (LPNMUPDOWN)lparam ;
                  if (delta->iDelta > 0)
                     config.scaleFactor *= 2.0f ;
                  if (delta->iDelta < 0)
                     config.scaleFactor *= 0.5f ;
                  sprintf (buffer, "%g", config.scaleFactor) ;
                  SendDlgItemMessage (wnd, IDC_FACTOR, WM_SETTEXT, 0, (LPARAM)&buffer) ;
               }
            }
         }
         break ;

      case WM_HSCROLL:

         // Handle trackbar

         if ((HWND)lparam == GetDlgItem(wnd, IDC_COMPLEXITY))
         {
            float pos = (float)SendDlgItemMessage(wnd, IDC_COMPLEXITY, TBM_GETPOS, 0, 0);
            config.collisionComplexity = pos / 10.0f ;
         }
         break ;

   }
   return FALSE ;
}



// --------------------------------------------------------------------------
//  Plugin entry point
// --------------------------------------------------------------------------

int cDTSPlugin::Execute (msModel * model)
{
   if (!model)
      return -1 ;

   this->model = model ;

   // Don't export an empty model

   if (msModel_GetMeshCount(model) == 0)
   {
      MessageBox (NULL, "The model is empty. Nothing to export.",
                  "Torque Shader Engine (DTS) Exporter", MB_ICONSTOP | MB_OK) ;
      return 0 ;
   }

   // If there are any meshes called "Collision", use those for collision

   for (int i = 0 ; i < msModel_GetMeshCount(model) ; i++)
   {
      char buffer[256] ;
      msMesh * mesh = msModel_GetMeshAt (model, i) ;
      msMesh_GetName (mesh, buffer, 256) ;

      if (_strnicmp (buffer, "Collision", 9) == 0)
      {
         config.collisionType = Shape::C_Mesh ;
         break;
      }
   }

   // Register the common controls before use
   // (it is safe to do this more than once)

   INITCOMMONCONTROLSEX ic ;
   ic.dwSize = sizeof(ic) ;
   ic.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES  ;
   InitCommonControlsEx (&ic) ;

   // Extract options and sequence information from the materials.
   // The options are done first since they can effect the animation
   // sequences and we don't want to be affected by material ordering.
   int numMaterials = msModel_GetMaterialCount(model) ;
   config.reset();

   for (i = 0 ; i < numMaterials ; i++)
   {
      char name[MS_MAX_PATH+1];
      msMaterial * msMat = msModel_GetMaterialAt(model, i);
      msMaterial_GetName (msMat, name, MS_MAX_PATH);
      extractOptions(name,&config);
   }
   for (i = 0 ; i < numMaterials ; i++)
   {
      char name[MS_MAX_PATH+1];
      msMaterial * msMat = msModel_GetMaterialAt(model, i);
      msMaterial_GetName (msMat, name, MS_MAX_PATH);
      MilkshapeShape::ImportConfig::Sequence si;
      si.fps = config.animationFPS;
      si.cyclic = config.animationCyclic;
      if (extractSequence(name,&si))
         config.sequence.push_back(si);
   }

   // If there were no sequences the default is a looping ambient
   // for the total timeline
   if (!config.sequence.size())
   {
      MilkshapeShape::ImportConfig::Sequence si;
      si.name = "Ambient";
      si.start = 0;
      si.end = msModel_GetTotalFrames (model);
      si.cyclic = true;
      si.fps = config.animationFPS;
      config.sequence.push_back(si);
   }

   // Show a preferences dialog

   currentPlugin = this ;
   if (DialogBox (hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, dialogProc) != IDOK)
   {
      currentPlugin = 0 ;
      return 0 ;
   }
   currentPlugin = 0 ;

   // Gets the file name

   OPENFILENAME ofn ;
   char         szFile[512] = "" ;

   ZeroMemory (&ofn, sizeof(ofn)) ;
   ofn.lStructSize = sizeof(ofn) ;
   ofn.hInstance   = GetModuleHandle(NULL) ;
   ofn.lpstrFilter = "DTS Files\0*.dts\0All\0*.*\0" ;
   ofn.lpstrFile   = szFile ;
   ofn.Flags       = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT ;
   ofn.nMaxFile    = 500 ;
   ofn.lpstrDefExt = "dts" ;

   if (GetSaveFileName (&ofn) != 0)
   {
      std::ofstream file (szFile, std::ios::binary | std::ios::trunc | std::ios::out) ;

      if (!file)
      {
         MessageBox (NULL, "Error creating the file.",
                     "Torque Shader Engine (DTS) Exporter", MB_ICONSTOP | MB_OK) ;
         return 0 ;
      }
      else
      {
         LoadCursor (hInstance, IDC_WAIT) ;
         MilkshapeShape shape(model, config) ;
         shape.save(file) ;
         LoadCursor (hInstance, IDC_ARROW) ;
         return 1 ;
      }
   }

   return 0 ;
}

// --------------------------------------------------------------------------
//  DLL entry point
// --------------------------------------------------------------------------

BOOL APIENTRY DllMain (HINSTANCE h, DWORD, LPVOID)
{
   cDTSPlugin::setInstance(h) ;
   return TRUE ;
}

cMsPlugIn * CreatePlugIn()
{
   return new cDTSPlugin() ;
}
