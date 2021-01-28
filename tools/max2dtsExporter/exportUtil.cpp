//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#pragma pack(push,8)
#include <MAX.H>
#include <istdplug.h>
#include <utilapi.h>
#pragma pack(pop)

#include "max2dtsExporter/SceneEnum.h"
#include "max2dtsExporter/maxUtil.h"

#include "Platform/platform.h"
#include "Platform/platformAssert.h"
#include "max2dtsExporter/exportUtil.h"
#include "max2dtsExporter/exporter.h"

extern HINSTANCE hInstance;
extern TCHAR *GetString(S32);
extern const char * GetVersionString();

#define EditBoxesToo 0x80000000

class ExportUtil : public UtilityObj
{
   public:

      IUtil *iu;
      Interface *ip;
      HWND hPanel1;
      HWND hPanel1b;
      HWND hPanel2;
      HWND hPanel3;
      HWND hPanel4;
      HWND hPanel5;

      //Constructor/Destructor
      ExportUtil();
      ~ExportUtil();

      void BeginEditParams(Interface *ip, IUtil *iu);
      void EndEditParams(Interface *ip, IUtil *iu);
      void DeleteThis() {}

      void Init(HWND hWnd);
      void Destroy(HWND hWnd);

      void initializePanels(S32 mask = -1);
};


static ExportUtil exportUtil;

// This is the Class Descriptor for the U plug-in
class ExportUtilClassDesc:public ClassDesc
{
   public:
   S32            IsPublic() {return 1;}
   void *         Create(BOOL loading = FALSE) { loading; return &exportUtil;}
   const TCHAR *  ClassName() {return "DTS Exporter Utility";}
   SClass_ID      SuperClassID() {return UTILITY_CLASS_ID;}
   Class_ID       ClassID() {return ExportUtil_CLASS_ID;}
   const TCHAR*   Category() {return "Exporter Utility";}
   void           ResetClassParams (BOOL fileReset);
};

static ExportUtilClassDesc exportUtilDesc;

ClassDesc * GetExportUtilDesc() {return &exportUtilDesc;}
Interface * GetMaxInterface() { return exportUtil.ip; }

//TODO: Should implement this method to reset the plugin params when Max is reset
void ExportUtilClassDesc::ResetClassParams (BOOL fileReset)
{

}

#define MAX_FILENAME_LEN 128
char filenameBuffer[MAX_FILENAME_LEN];

void findFiles(const char * filePattern, Vector<char *> & matches)
{
   WIN32_FIND_DATA findData;
   HANDLE searchHandle = FindFirstFile(filePattern,&findData);
   if (searchHandle != INVALID_HANDLE_VALUE)
   {
      matches.push_back(dStrdup(findData.cFileName));
      while (FindNextFile(searchHandle,&findData))
         matches.push_back(dStrdup(findData.cFileName));
   }
}

void clearFindFiles(Vector<char*> & matches)
{
   for (S32 i=0; i<matches.size(); i++)
      dFree(matches[i]);
   matches.clear();
}

void initOFN(OPENFILENAME & ofn, char * filter, char * ext, char * title, const char * dir)
{
    memset(filenameBuffer,0,MAX_FILENAME_LEN);
    memset(&ofn,0,sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = filenameBuffer;
    ofn.nMaxFile = MAX_FILENAME_LEN;
    filenameBuffer[0] = '\0';
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = dir;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrDefExt = ext;
}

static BOOL CALLBACK ExportUtilDlgProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
         break;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

static BOOL CALLBACK RenumberDlgFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         break;

      case WM_DESTROY:
         break;

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDCANCEL:
               EndDialog(hWnd,-99);
               break;
            case IDOK:
               char buffer[128];
               if (GetDlgItemText(hWnd,IDC_EDIT,buffer,128)>0)
               {
                  int newSize = dAtoi(buffer);
                  EndDialog(hWnd,newSize);
               }
               else
                  EndDialog(hWnd,-99);
               break;
         }
         break;
      }

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

static BOOL CALLBACK ExportUtilDlgProc1b(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
      {
         OPENFILENAME ofn;
         switch(LOWORD(wParam))
         {
            case IDC_EXPORT_WHOLE :
                initOFN(ofn,"Torque 3Space Shape File (*.dts)\0*.dts\0","dts","Export DTS",exportUtil.ip->GetDir(APP_EXPORT_DIR));
                if (GetSaveFileName(&ofn))
                    exportUtil.ip->ExportToFile(ofn.lpstrFile);
                break;
            case IDC_EXPORT_SEQUENCES :
                initOFN(ofn,"Torque 3Space Sequence File (*.dsq)\0*.dsq\0","dsq","Export DSQ",exportUtil.ip->GetDir(APP_EXPORT_DIR));
                if (GetSaveFileName(&ofn))
                    exportUtil.ip->ExportToFile(ofn.lpstrFile);
                break;
            case IDC_EXPORT_TEXT:
                initOFN(ofn,"Torque 3Space Scene Text Export (*.txt)\0*.txt\0","txt","Scene Text Export",exportUtil.ip->GetDir(APP_EXPORT_DIR));
                if (GetSaveFileName(&ofn))
                    exportUtil.ip->ExportToFile(ofn.lpstrFile);
                break;
            case IDC_RENUMBER:
            {
                S32 newNumber = DialogBox(hInstance,MAKEINTRESOURCE(IDD_RENUMBER),hWnd,RenumberDlgFunc);
                if (newNumber >=0 && exportUtil.ip)
                   renumberNodes(exportUtil.ip,newNumber);
                break;
            }
            case IDC_REGISTER_DETAILS:
                registerDetails(exportUtil.ip);
                break;
            case IDC_EMBED:
                embedSubtree(exportUtil.ip);
                break;
         }
         break;
      }


      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

static BOOL CALLBACK ExportUtilDlgProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_COLLAPSE : transformCollapse = !transformCollapse; break;
            case IDC_ENABLE_SEQUENCES : enableSequences = !enableSequences; break;
            case IDC_ENABLE_TWO_SIDED : enableTwoSidedMaterials = !enableTwoSidedMaterials; break;
            case IDC_ANIMATION_DELTA:
               if (HIWORD(wParam)==EN_CHANGE)
               {
                  ICustEdit * edit = GetICustEdit( GetDlgItem(exportUtil.hPanel2,IDC_ANIMATION_DELTA) );
                  if (!edit->GotReturn())
                     return TRUE;
                  F32 tmp = edit->GetFloat();
                  if (tmp > 10E-12f)
                     animationDelta = tmp;
                  ReleaseICustEdit(edit);
                  exportUtil.initializePanels(2 + EditBoxesToo);
               }
               break;
            case IDC_BASE_TEXTURE_PATH:
               if (HIWORD(wParam)==EN_CHANGE)
               {
                  ICustEdit * edit = GetICustEdit( GetDlgItem(exportUtil.hPanel2,IDC_BASE_TEXTURE_PATH) );
                  if (!edit->GotReturn())
                     return TRUE;
                  char buffer[256];
                  edit->GetText(buffer,sizeof(buffer));
                  setBaseTexturePath(buffer);
                  ReleaseICustEdit(edit);
                  exportUtil.initializePanels(2 + EditBoxesToo);
               }
               break;
            default : break;
         }
         exportUtil.initializePanels(2);
         break;


      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

static BOOL CALLBACK ExportUtilDlgProc3(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_ALLOW_EMPTY : allowEmptySubtrees = !allowEmptySubtrees; break;
            case IDC_ALLOW_CROSSED : allowCrossedDetails = !allowCrossedDetails; break;
            case IDC_ALLOW_UNUSED : allowUnusedMeshes = !allowUnusedMeshes; break;
            case IDC_ALLOW_OLD : allowOldSequences = !allowOldSequences; break;
            case IDC_REQUIRE_VICON : viconNeeded = !viconNeeded; break;
         }
         exportUtil.initializePanels(4);
         break;


      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

static BOOL CALLBACK ExportUtilDlgProc4(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
      {
         switch(LOWORD(wParam))
         {
            case IDC_PASS1 : dumpMask ^= PDPass1; break;
            case IDC_PASS2 : dumpMask ^= PDPass2; break;
            case IDC_PASS3 : dumpMask ^= PDPass3; break;
            case IDC_NODE_STATES : dumpMask ^= PDNodeStates; break;
            case IDC_OBJECT_STATES : dumpMask ^= PDObjectStates; break;
            case IDC_NODE_STATE_DETAILS : dumpMask ^= PDNodeStateDetails; break;
            case IDC_OBJECT_STATE_DETAILS : dumpMask ^= PDObjectStateDetails; break;
            case IDC_OBJECT_OFFSETS : dumpMask ^= PDObjectOffsets; break;
            case IDC_SEQUENCE_DETAILS : dumpMask ^= PDSequences; break;
            case IDC_SHAPE_HIERARCHY : dumpMask ^= PDShapeHierarchy; break;
         }
         exportUtil.initializePanels(8);
         break;
      }


      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

// ok,ok, so this should probably be in a header somewhere...so sue me
extern char globalConfig[270];

static BOOL CALLBACK ExportUtilDlgProc5(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) {
      case WM_INITDIALOG:
         exportUtil.Init(hWnd);
         break;

      case WM_DESTROY:
         exportUtil.Destroy(hWnd);
         break;

      case WM_COMMAND:
      {
         OPENFILENAME ofn;
         switch(LOWORD(wParam))
         {
            case IDC_CONFIG_LOAD_DEFAULT :
                SceneEnumProc::readConfigFile(globalConfig);
                break;
            case IDC_CONFIG_SAVE_DEFAULT :
                SceneEnumProc::writeConfigFile(globalConfig);
                break;
            case IDC_CONFIG_LOAD :
                initOFN(ofn,"Exporter Configuration File (*.cfg)\0*.cfg\0","cfg","Load Configuration File",NULL);
                if (GetOpenFileName(&ofn))
                    SceneEnumProc::readConfigFile(ofn.lpstrFile);
                break;
            case IDC_CONFIG_SAVE :
                initOFN(ofn,"Exporter Configuration File (*.cfg)\0*.cfg\0","dsq","Save Configuration File",NULL);
                if (GetSaveFileName(&ofn))
                    SceneEnumProc::writeConfigFile(ofn.lpstrFile);
                break;
         }
         exportUtil.initializePanels();
         break;
      }


      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_MOUSEMOVE:
         exportUtil.ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

ExportUtil::ExportUtil()
{
   iu = NULL;
   ip = NULL;
   hPanel1 = NULL;
   hPanel1b = NULL;
   hPanel2 = NULL;
   hPanel3 = NULL;
   hPanel4 = NULL;
   hPanel5 = NULL;
}

ExportUtil::~ExportUtil()
{

}

void ExportUtil::BeginEditParams(Interface *ip, IUtil *iu)
{
   this->iu = iu;
   this->ip = ip;
   hPanel1 = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL1),
      ExportUtilDlgProc1,
      "About",
      0,
      APPENDROLL_CLOSED);

   hPanel1b = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL1B),
      ExportUtilDlgProc1b,
      "Utilities",
      0);

   hPanel2 = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL2),
      ExportUtilDlgProc2,
      "Parameters",
      0,
      APPENDROLL_CLOSED);

   hPanel3 = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL3),
      ExportUtilDlgProc3,
      "Error Control",
      0,
      APPENDROLL_CLOSED);

   hPanel4 = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL4),
      ExportUtilDlgProc4,
      "Dump File Control",
      0,
      APPENDROLL_CLOSED);

   hPanel5 = ip->AddRollupPage(
      hInstance,
      MAKEINTRESOURCE(IDD_UTIL_PANEL5),
      ExportUtilDlgProc5,
      "Configuration Control",
      0,
      APPENDROLL_CLOSED);

   initializePanels();
}

void ExportUtil::EndEditParams(Interface *ip, IUtil *iu)
{
   this->iu = NULL;
   this->ip = NULL;

   ip->DeleteRollupPage(hPanel1);
   hPanel1 = NULL;

   ip->DeleteRollupPage(hPanel1b);
   hPanel1b = NULL;

   ip->DeleteRollupPage(hPanel2);
   hPanel2 = NULL;

   ip->DeleteRollupPage(hPanel3);
   hPanel3 = NULL;

   ip->DeleteRollupPage(hPanel4);
   hPanel4 = NULL;

   ip->DeleteRollupPage(hPanel5);
   hPanel5 = NULL;
}

void ExportUtil::Init(HWND hWnd)
{
}

void ExportUtil::Destroy(HWND hWnd)
{

}

void ExportUtil::initializePanels(S32 mask)
{

   ICustButton * button;
   ICustEdit   * edit;
   char buffer[255];

   // Panel 1 -- about panel
   if (mask & 1)
   {
      // add text to about panel
      dStrcpy(buffer,avar("Torque DTS Exporter\n%s\nCompiled %s\n%s",GetVersionString(),__DATE__,__TIME__));
      SetDlgItemText(hPanel1,IDC_STATIC_ABOUT,buffer);
   }

   // Panel 2 -- Parameters
   if (mask&2)
   {
      CheckDlgButton(hPanel2,IDC_COLLAPSE,transformCollapse);
      CheckDlgButton(hPanel2,IDC_ENABLE_SEQUENCES,enableSequences);
      CheckDlgButton(hPanel2,IDC_ENABLE_TWO_SIDED,enableTwoSidedMaterials);
      if (mask & EditBoxesToo)
      {
         edit = GetICustEdit(GetDlgItem(hPanel2,IDC_ANIMATION_DELTA));
         dStrcpy(buffer,avar("%f",animationDelta));
         if (edit) // may already be locked
         {
            edit->SetText( buffer );
            edit->WantReturn(true);
            ReleaseICustEdit(edit);
         }
         edit = GetICustEdit(GetDlgItem(hPanel2,IDC_BASE_TEXTURE_PATH));
         dStrcpy(buffer,baseTexturePath);
         if (edit)
         {
            edit->SetText( buffer );
            edit->WantReturn(true);
            ReleaseICustEdit(edit);
         }
      }
   }

   // Panel 3 -- Error Control
   if (mask & 4)
   {
      CheckDlgButton(hPanel3,IDC_ALLOW_EMPTY,allowEmptySubtrees);
      CheckDlgButton(hPanel3,IDC_ALLOW_CROSSED,allowCrossedDetails);
      CheckDlgButton(hPanel3,IDC_ALLOW_UNUSED,allowUnusedMeshes);
      CheckDlgButton(hPanel3,IDC_ALLOW_OLD,allowOldSequences);
      CheckDlgButton(hPanel3,IDC_REQUIRE_VICON,viconNeeded);
   }

   // Panel 4 -- Dump file control
   if (mask & 8)
   {
      CheckDlgButton(hPanel4,IDC_PASS1,dumpMask & PDPass1);
      CheckDlgButton(hPanel4,IDC_PASS2,dumpMask & PDPass2);
      CheckDlgButton(hPanel4,IDC_PASS3,dumpMask & PDPass3);
      CheckDlgButton(hPanel4,IDC_NODE_STATES,dumpMask & PDNodeStates);
      CheckDlgButton(hPanel4,IDC_OBJECT_STATES,dumpMask & PDObjectStates);
      CheckDlgButton(hPanel4,IDC_NODE_STATE_DETAILS,dumpMask & PDNodeStateDetails);
      CheckDlgButton(hPanel4,IDC_OBJECT_STATE_DETAILS,dumpMask & PDObjectStateDetails);
      CheckDlgButton(hPanel4,IDC_OBJECT_OFFSETS,dumpMask & PDObjectOffsets);
      CheckDlgButton(hPanel4,IDC_SEQUENCE_DETAILS,dumpMask & PDSequences);
      CheckDlgButton(hPanel4,IDC_SHAPE_HIERARCHY,dumpMask & PDShapeHierarchy);
   }

   // Panel 5 -- Configuration control
   if (mask & 16)
   {
      // make buttons push buttons rather than check buttons...
      button = GetICustButton(GetDlgItem(hPanel5,IDC_CONFIG_LOAD_DEFAULT));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel5,IDC_CONFIG_SAVE_DEFAULT));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel5,IDC_CONFIG_LOAD));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel5,IDC_CONFIG_SAVE));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
   }

   // Panel 1b -- Utility panel
   if (mask & 32)
   {
      // make buttons push buttons rather than check buttons...
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_EXPORT_WHOLE));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_EXPORT_SEQUENCES));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_EXPORT_TEXT));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_RENUMBER));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_EMBED));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
      button = GetICustButton(GetDlgItem(hPanel1b,IDC_REGISTER_DETAILS));
      button->SetType(CBT_PUSH);
      ReleaseICustButton(button);
   }
}
