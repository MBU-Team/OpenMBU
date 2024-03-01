//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

$sgEditorItemNames::sgMenu = "Lighting Tools";
$sgEditorItemNames::sgMenuItem[0] = "Light Editor";
//$sgEditorItemNames::sgMenuItem[1] = "DRL Editor";
$sgEditorItemNames::sgMenuItem[2] = "Filtered Relight";
$sgEditorItemNames::sgMenuItem[3] = "Full Relight";

function EditorGui::getPrefs()
{
   // same defaults as WorldEditor ctor
   EWorldEditor.axisGizmoActive = getPrefSetting($pref::WorldEditor::axisGizmoActive, true);
   EWorldEditor.axisGizmoMaxScreenLen = getPrefSetting($pref::WorldEditor::axisGizmoMaxScreenLen, 200);
   EWorldEditor.boundingBoxCollision = getPrefSetting($pref::WorldEditor::boundingBoxCollision, true);
   EWorldEditor.defaultHandle = getPrefSetting($pref::WorldEditor::defaultHandle, "gui/Editor_DefaultHandle.png");
   EWorldEditor.dragRectColor = getPrefSetting($pref::WorldEditor::dragRectColor, "255 255 0");
   EWorldEditor.dropType = getPrefSetting($pref::WorldEditor::dropType, "screenCenter");
   EWorldEditor.faceSelectColor = getPrefSetting($pref::WorldEditor::faceSelectColor, "0 0 100 100");
   EWorldEditor.gridColor = getPrefSetting($pref::WorldEditor::gridColor, "255 255 255 20");
   EWorldEditor.gridSize = getPrefSetting($pref::WorldEditor::gridSize, "10 10 10");
   EWorldEditor.lockedHandle = getPrefSetting($pref::WorldEditor::lockedHandle, "gui/Editor_LockedHandle.png");
   EWorldEditor.maxScaleFactor = getPrefSetting($pref::WorldEditor::maxScaleFactor, 4000);
   EWorldEditor.minScaleFactor = getPrefSetting($pref::WorldEditor::minScaleFactor, 0.1);
   EWorldEditor.mouseMoveScale = getPrefSetting($pref::WorldEditor::mouseMoveScale, 0.2);
   EWorldEditor.mouseRotateScale = getPrefSetting($pref::WorldEditor::mouseRotateScale, 0.01);
   EWorldEditor.mouseScaleScale = getPrefSetting($pref::WorldEditor::mouseScaleScale, 0.01);
   EWorldEditor.objectsUseBoxCenter = getPrefSetting($pref::WorldEditor::objectsUseBoxCenter, true);
   EWorldEditor.objectTextColor = getPrefSetting($pref::WorldEditor::objectTextColor, "255 255 255");
   EWorldEditor.objMouseOverColor = getPrefSetting($pref::WorldEditor::objMouseOverColor, "0 255 0");
   EWorldEditor.objMouseOverSelectColor = getPrefSetting($pref::WorldEditor::objMouseOverSelectColor, "0 0 255");
   EWorldEditor.objSelectColor = getPrefSetting($pref::WorldEditor::objSelectColor, "255 0 0");
   EWorldEditor.planarMovement = getPrefSetting($pref::WorldEditor::planarMovement, true);
   EWorldEditor.planeDim = getPrefSetting($pref::WorldEditor::planeDim, 500);
   EWorldEditor.popupBackgroundColor = getPrefSetting($pref::WorldEditor::popupBackgroundColor, "100 100 100");
   EWorldEditor.popupTextColor = getPrefSetting($pref::WorldEditor::popupTextColor, "255 255 0");
   EWorldEditor.projectDistance = getPrefSetting($pref::WorldEditor::projectDistance, 2000);
   EWorldEditor.renderObjHandle = getPrefSetting($pref::WorldEditor::renderObjHandle, true);
   EWorldEditor.renderObjText = getPrefSetting($pref::WorldEditor::renderObjText, true);
   EWorldEditor.renderPlane = getPrefSetting($pref::WorldEditor::renderPlane, true);
   EWorldEditor.renderPlaneHashes = getPrefSetting($pref::WorldEditor::renderPlaneHashes, true);
   EWorldEditor.renderPopupBackground = getPrefSetting($pref::WorldEditor::renderPopupBackground, true);
   EWorldEditor.renderSelectionBox = getPrefSetting($pref::WorldEditor::renderSelectionBox, true);
   EWorldEditor.rotationSnap = getPrefSetting($pref::WorldEditor::rotationSnap, "15");
   EWorldEditor.selectHandle = getPrefSetting($pref::WorldEditor::selectHandle, "gui/Editor_SelectHandle.png");
   EWorldEditor.selectionBoxColor = getPrefSetting($pref::WorldEditor::selectionBoxColor, "255 255 0");
   EWorldEditor.showMousePopupInfo = getPrefSetting($pref::WorldEditor::showMousePopupInfo, true);
   EWorldEditor.snapRotations = getPrefSetting($pref::WorldEditor::snapRotations, false);
   EWorldEditor.snapToGrid = getPrefSetting($pref::WorldEditor::snapToGrid, false);
   EWorldEditor.undoLimit = getPrefSetting($pref::WorldEditor::undoLimit, 40);

   ETerrainEditor.softSelecting = 1;
   ETerrainEditor.currentAction = "raiseHeight";
   ETerrainEditor.currentMode = "select";
}

function EditorGui::setPrefs()
{
   $pref::WorldEditor::axisGizmoActive = EWorldEditor.axisGizmoActive;
   $pref::WorldEditor::axisGizmoMaxScreenLen = EWorldEditor.axisGizmoMaxScreenLen;
   $pref::WorldEditor::boundingBoxCollision = EWorldEditor.boundingBoxCollision;
   $pref::WorldEditor::defaultHandle = EWorldEditor.defaultHandle;
   $pref::WorldEditor::dragRectColor = EWorldEditor.dragRectColor;
   $Pref::WorldEditor::dropType = EWorldEditor.dropType;
   $pref::WorldEditor::gridColor = EWorldEditor.GridColor;
   $pref::WorldEditor::gridSize = EWorldEditor.GridSize;
   $pref::WorldEditor::lockedHandle = EWorldEditor.lockedHandle;
   $pref::WorldEditor::maxScaleFactor = EWorldEditor.maxScaleFactor;
   $pref::WorldEditor::minScaleFactor = EWorldEditor.minScaleFactor;
   $pref::WorldEditor::mouseMoveScale = EWorldEditor.mouseMoveScale;
   $pref::WorldEditor::mouseRotateScale = EWorldEditor.mouseRotateScale;
   $pref::WorldEditor::mouseScaleScale = EWorldEditor.mouseScaleScale;
   $pref::WorldEditor::objectsUseBoxCenter = EWorldEditor.objectsUseBoxCenter;
   $pref::WorldEditor::objectTextColor = EWorldEditor.ObjectTextColor;
   $pref::WorldEditor::objMouseOverColor = EWorldEditor.objMouseOverColor;
   $pref::WorldEditor::objMouseOverSelectColor = EWorldEditor.objMouseOverSelectColor;
   $pref::WorldEditor::objSelectColor = EWorldEditor.objSelectColor;
   $pref::WorldEditor::planarMovement = EWorldEditor.planarMovement;
   $pref::WorldEditor::planeDim = EWorldEditor.planeDim;
   $pref::WorldEditor::popupBackgroundColor = EWorldEditor.PopupBackgroundColor;
   $pref::WorldEditor::popupTextColor = EWorldEditor.PopupTextColor;
   $pref::WorldEditor::projectDistance = EWorldEditor.projectDistance;
   $pref::WorldEditor::raceSelectColor = EWorldEditor.faceSelectColor;
   $pref::WorldEditor::renderObjHandle = EWorldEditor.renderObjHandle;
   $pref::WorldEditor::renderObjText = EWorldEditor.renderObjText;
   $pref::WorldEditor::renderPlane = EWorldEditor.renderPlane;
   $pref::WorldEditor::renderPlaneHashes = EWorldEditor.renderPlaneHashes;
   $pref::WorldEditor::renderPopupBackground = EWorldEditor.renderPopupBackground;
   $pref::WorldEditor::renderSelectionBox = EWorldEditor.renderSelectionBox;
   $pref::WorldEditor::rotationSnap = EWorldEditor.rotationSnap;
   $pref::WorldEditor::selectHandle = EWorldEditor.selectHandle;
   $pref::WorldEditor::selectionBoxColor = EWorldEditor.selectionBoxColor;
   $pref::WorldEditor::showMousePopupInfo = EWorldEditor.showMousePopupInfo;
   $pref::WorldEditor::snapRotations = EWorldEditor.snapRotations;
   $pref::WorldEditor::snapToGrid = EWorldEditor.snapToGrid;
   $pref::WorldEditor::undoLimit = EWorldEditor.undoLimit;

}

function EditorGui::onSleep(%this)
{
   %this.setPrefs();
}

function EditorGui::init(%this)
{
   %this.getPrefs();

   if(!isObject("terraformer"))
      new Terraformer("terraformer");
      
   $SelectedOperation = -1;
   $NextOperationId   = 1;
   $HeightfieldDirtyRow = -1;

   EditorMenuBar.clearMenus();
   EditorMenuBar.addMenu("File", 0);
   EditorMenuBar.addMenuItem("File", "New Mission...", 1);
   EditorMenuBar.addMenuItem("File", "Open Mission...", 2, "Ctrl O");
   EditorMenuBar.addMenuItem("File", "Save Mission...", 3, "Ctrl S");
   EditorMenuBar.addMenuItem("File", "Save Mission As...", 4);
   EditorMenuBar.addMenuItem("File", "-", 0);
   EditorMenuBar.addMenuItem("File", "Import Terraform Data...", 6);
   EditorMenuBar.addMenuItem("File", "Import Texture Data...", 5);
   EditorMenuBar.addMenuItem("File", "-", 0);
   EditorMenuBar.addMenuItem("File", "Export Terraform Bitmap...", 5);
   EditorMenuBar.addMenuItem("File", "-", 0);
   EditorMenuBar.addMenuItem("File", "Toggle Map Editor...", 7, "F11");
   EditorMenuBar.addMenuItem("File", "Quit", 8);

   EditorMenuBar.addMenu("Edit", 1);
   EditorMenuBar.addMenuItem("Edit", "Undo", 1, "Ctrl Z");
   EditorMenuBar.setMenuItemBitmap("Edit", "Undo", 1);
   EditorMenuBar.addMenuItem("Edit", "Redo", 2, "Ctrl R");
   EditorMenuBar.setMenuItemBitmap("Edit", "Redo", 2);
   EditorMenuBar.addMenuItem("Edit", "-", 0);
   EditorMenuBar.addMenuItem("Edit", "Cut", 3, "Ctrl X");
   EditorMenuBar.setMenuItemBitmap("Edit", "Cut", 3);
   EditorMenuBar.addMenuItem("Edit", "Copy", 4, "Ctrl C");
   EditorMenuBar.setMenuItemBitmap("Edit", "Copy", 4);
   EditorMenuBar.addMenuItem("Edit", "Paste", 5, "Ctrl V");
   EditorMenuBar.setMenuItemBitmap("Edit", "Paste", 5);
   EditorMenuBar.addMenuItem("Edit", "-", 0);
   EditorMenuBar.addMenuItem("Edit", "Select All", 6, "Ctrl A");
   EditorMenuBar.addMenuItem("Edit", "Select None", 7, "Ctrl N");
   EditorMenuBar.addMenuItem("Edit", "-", 0);
   EditorMenuBar.addMenuItem("Edit", "World Editor Settings...", 12);
   EditorMenuBar.addMenuItem("Edit", "Terrain Editor Settings...", 13);
   EditorMenuBar.addMenuItem("Edit", "Model Importer Settings...", 14);

   EditorMenuBar.addMenu("Camera", 7);
   EditorMenuBar.addMenuItem("Camera", "Drop Camera at Player", 1, "Alt Q");
   EditorMenuBar.addMenuItem("Camera", "Drop Player at Camera", 2, "Alt W");
   EditorMenuBar.addMenuItem("Camera", "Toggle Camera", 10, "Alt C");
   EditorMenuBar.addMenuItem("Camera", "-", 0);
   EditorMenuBar.addMenuItem("Camera", "Slowest", 3, "Shift 1", 1);
   EditorMenuBar.addMenuItem("Camera", "Very Slow", 4, "Shift 2", 1);
   EditorMenuBar.addMenuItem("Camera", "Slow", 5, "Shift 3", 1);
   EditorMenuBar.addMenuItem("Camera", "Medium Pace", 6, "Shift 4", 1);
   EditorMenuBar.addMenuItem("Camera", "Fast", 7, "Shift 5", 1);
   EditorMenuBar.addMenuItem("Camera", "Very Fast", 8, "Shift 6", 1);
   EditorMenuBar.addMenuItem("Camera", "Fastest", 9, "Shift 7", 1);

   EditorMenuBar.addMenu("World", 6);
   EditorMenuBar.addMenuItem("World", "Lock Selection", 10, "Ctrl L");
   EditorMenuBar.addMenuItem("World", "Unlock Selection", 11, "Ctrl Shift L");
   EditorMenuBar.addMenuItem("World", "-", 0);
   EditorMenuBar.addMenuItem("World", "Hide Selection", 12, "Ctrl H");
   EditorMenuBar.addMenuItem("World", "Show Selection", 13, "Ctrl Shift H");
   EditorMenuBar.addMenuItem("World", "-", 0);
   EditorMenuBar.addMenuItem("World", "Delete Selection", 17, "Delete");
   EditorMenuBar.addMenuItem("World", "Camera To Selection", 14);
   EditorMenuBar.addMenuItem("World", "Reset Transforms", 15);
   EditorMenuBar.addMenuItem("World", "Drop Selection", 16, "Ctrl D");
   EditorMenuBar.addMenuItem("World", "Add Selection to Instant Group", 17);
   EditorMenuBar.addMenuItem("World", "-", 0);
   EditorMenuBar.addMenuItem("World", "Drop at Origin", 0, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop at Camera", 1, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop at Camera w/Rot", 2, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop below Camera", 3, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop at Screen Center", 4, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop at Centroid", 5, "", 1);
   EditorMenuBar.addMenuItem("World", "Drop to Ground", 6, "", 1);

   EditorMenuBar.addMenu("Action", 3);
   EditorMenuBar.addMenuItem("Action", "Select", 1, "", 1);
   EditorMenuBar.addMenuItem("Action", "Adjust Selection", 2, "", 1);
   EditorMenuBar.addMenuItem("Action", "-", 0);
   EditorMenuBar.addMenuItem("Action", "Add Dirt", 6, "", 1);
   EditorMenuBar.addMenuItem("Action", "Excavate", 6, "", 1);
   EditorMenuBar.addMenuItem("Action", "Adjust Height", 6, "", 1);
   EditorMenuBar.addMenuItem("Action", "Flatten", 4, "", 1);
   EditorMenuBar.addMenuItem("Action", "Smooth", 5, "", 1);
   EditorMenuBar.addMenuItem("Action", "Set Height", 7, "", 1);
   EditorMenuBar.addMenuItem("Action", "-", 0);
   EditorMenuBar.addMenuItem("Action", "Set Empty", 8, "", 1);
   EditorMenuBar.addMenuItem("Action", "Clear Empty", 8, "", 1);
   EditorMenuBar.addMenuItem("Action", "-", 0);
   EditorMenuBar.addMenuItem("Action", "Paint Material", 9, "", 1);

   EditorMenuBar.addMenu("Brush", 4);
   EditorMenuBar.addMenuItem("Brush", "Box Brush", 91, "", 1);
   EditorMenuBar.addMenuItem("Brush", "Circle Brush", 92, "", 1);
   EditorMenuBar.addMenuItem("Brush", "-", 0);
   EditorMenuBar.addMenuItem("Brush", "Soft Brush", 93, "", 2);
   EditorMenuBar.addMenuItem("Brush", "Hard Brush", 94, "", 2);
   EditorMenuBar.addMenuItem("Brush", "-", 0);
   EditorMenuBar.addMenuItem("Brush", "Size 1 x 1", 1, "Alt 1", 3);
   EditorMenuBar.addMenuItem("Brush", "Size 3 x 3", 3, "Alt 2", 3);
   EditorMenuBar.addMenuItem("Brush", "Size 5 x 5", 5, "Alt 3", 3);
   EditorMenuBar.addMenuItem("Brush", "Size 9 x 9", 9, "Alt 4", 3);
   EditorMenuBar.addMenuItem("Brush", "Size 15 x 15", 15, "Alt 5", 3);
   EditorMenuBar.addMenuItem("Brush", "Size 25 x 25", 25, "Alt 6", 3);

   EditorMenuBar.addMenu("Window", 2);
   EditorMenuBar.addMenuItem("Window", "World Editor", 2, "F2", 1);
   EditorMenuBar.addMenuItem("Window", "World Editor Inspector", 3, "F3", 1);
   EditorMenuBar.addMenuItem("Window", "World Editor Creator", 4, "F4", 1);
   EditorMenuBar.addMenuItem("Window", "Mission Area Editor", 5, "F5", 1);
   EditorMenuBar.addMenuItem("Window", "-", 0);
   EditorMenuBar.addMenuItem("Window", "Terrain Editor", 6, "F6", 1);
   EditorMenuBar.addMenuItem("Window", "Terrain Terraform Editor", 7, "F7", 1);
   EditorMenuBar.addMenuItem("Window", "Terrain Texture Editor", 8, "F8", 1);
   EditorMenuBar.addMenuItem("Window", "Terrain Texture Painter", 9, "", 1);

   EditorMenuBar.addMenu($sgEditorItemNames::sgMenu, 8);
   EditorMenuBar.addMenuItem($sgEditorItemNames::sgMenu, $sgEditorItemNames::sgMenuItem[0], 2, "F12");
   //EditorMenuBar.addMenuItem($sgEditorItemNames::sgMenu, $sgEditorItemNames::sgMenuItem[1], 3, "");
   EditorMenuBar.addMenuItem($sgEditorItemNames::sgMenu, "-", 0);
   EditorMenuBar.addMenuItem($sgEditorItemNames::sgMenu, $sgEditorItemNames::sgMenuItem[2], 4, "Alt F");
   EditorMenuBar.addMenuItem($sgEditorItemNames::sgMenu, $sgEditorItemNames::sgMenuItem[3], 5, "Alt L");
   
   EditorMenuBar.addMenu("MBU", 8);
   EditorMenuBar.addMenuItem("MBU", "Set Preview Camera", 2, "", 1);
   
   EditorMenuBar.onActionMenuItemSelect(0, "Adjust Height");
   EditorMenuBar.onBrushMenuItemSelect(0, "Circle Brush");
   EditorMenuBar.onBrushMenuItemSelect(0, "Soft Brush");
   EditorMenuBar.onBrushMenuItemSelect(9, "Size 9 x 9");
   EditorMenuBar.onCameraMenuItemSelect(6, "Medium Pace");
   EditorMenuBar.onWorldMenuItemSelect(0, "Drop at Screen Center");

   EWorldEditor.init();
   ETerrainEditor.attachTerrain();
   TerraformerInit();
   TextureInit();

   //
   //Creator.init();
   EditorTree.init();
   ObjectBuilderGui.init();

   EWorldEditor.isDirty = false;
   ETerrainEditor.isDirty = false;
   ETerrainEditor.isMissionDirty = false;
   EditorGui.saveAs = false;
}

function EditorQuitMission()
{  
   if(ETerrainEditor.isMissionDirty || ETerrainEditor.isDirty || EWorldEditor.isDirty)
   {
      MessageBoxYesNo("Mission Modified", "Would you like to save your changes before quiting?", "EditorDoQuitMission(true);", "EditorDoQuitMission(false);");
   }
   else
      EditorDoQuitMission(false);
}

function EditorDoQuitMission(%saveFirst)
{
   if(%saveFirst)
   {
      EditorSaveMissionMenu();
      Editor.close("MainMenuGui");
      disconnect();
   }
   else
   {
      Editor.close("MainMenuGui");
      disconnect();
   }
}

function EditorNewMission()
{
   if(ETerrainEditor.isMissionDirty || ETerrainEditor.isDirty || EWorldEditor.isDirty)
   {
      MessageBoxYesNo("Mission Modified", "Would you like to save changes to the current mission \"" @
         $Server::MissionFile @ "\" before creating a new mission?", "EditorDoNewMission(true);", "EditorDoNewMission(false);");
   }
   else
      EditorDoNewMission(false);
}

function EditorSaveMissionMenu()
{
   //if (MissionInfo.type !$= "DemoScene") {
   if ($BlockSave!=1) {
      if(EditorGui.saveAs)
         EditorSaveMissionAs();
      else
         EditorSaveMission();
   }  else {
      echo("To enable mission saving, make \$BlockSave=1.");
      MessageBoxOK("Save Function Disabled","The Save Mission function has been disabled for this demo walkthrough.\n\n  If you are sure you want to make changes to this mission and save them, go to the console (tilde \"~\" key) and type \"$BlockSave=0;\" first.");
   }
}

function EditorSaveMission()
{
   // just save the mission without renaming it

   // first check for dirty and read-only files:
   if((EWorldEditor.isDirty || ETerrainEditor.isMissionDirty) && !isWriteableFileName($Server::MissionFile))
   {
      MessageBoxOK("Error", "Mission file \""@ $Server::MissionFile @ "\" is read-only.");
      return false;
   }
   if(ETerrainEditor.isDirty && !isWriteableFileName(Terrain.terrainFile))
   {
      MessageBoxOK("Error", "Terrain file \""@ Terrain.terrainFile @ "\" is read-only.");
      return false;
   }
  
   // now write the terrain and mission files out:

   if(EWorldEditor.isDirty || ETerrainEditor.isMissionDirty)
      MissionGroup.save($Server::MissionFile);
   if(ETerrainEditor.isDirty)
      Terrain.save(Terrain.terrainFile);
   EWorldEditor.isDirty = false;
   ETerrainEditor.isDirty = false;
   ETerrainEditor.isMissionDirty = false;
   EditorGui.saveAs = false;

   return true;
}

function EditorDoSaveAs(%missionName)
{
   ETerrainEditor.isDirty = true;
   EWorldEditor.isDirty = true;
   %saveMissionFile = $Server::MissionFile;
   %saveTerrName = Terrain.terrainFile;

   $Server::MissionFile = %missionName;
   Terrain.terrainFile = filePath(%missionName) @ "/" @ fileBase(%missionName) @ ".ter";

   if(!EditorSaveMission())
   {
      $Server::MissionFile = %saveMissionFile;
      Terrain.terrainFile = %saveTerrName;
   }
}

function EditorSaveMissionAs()
{
   if ($BlockSave!=1) {
      getSaveFilename("*.mis", "EditorDoSaveAs", $Server::MissionFile);
   }  else {
      echo("To enable mission saving, make \$BlockSave=0.");
      MessageBoxOK("Save Function Disabled","The Save Mission function has been disabled for this demo walkthrough.\n\n  If you are sure you want to make changes to this mission and save them, go to the console (tilde \"~\" key) and type \"$BlockSave=0;\" first.");
   }
}

function EditorDoLoadMission(%file)
{
   // close the current editor, it will get cleaned up by MissionCleanup
   Editor.close();

   loadMission( %file, true ) ;
   
   if(MissionInfo.gameType $= "MultiPlayer")
      $Game::SPGemHunt = true;
   else
      $Game::SPGemHunt = false;
      
   $Server::ServerType = "SinglePlayer";

   // recreate and open the editor
   Editor::create();
   MissionCleanup.add(Editor);
   EditorGui.loadingMission = true;
   Editor.open();
}

function EditorSaveBeforeLoad()
{
   if(EditorSaveMission())
      getLoadFilename("*.mis", "EditorDoLoadMission");
}

function EditorDoNewMission(%saveFirst)
{
   if(%saveFirst)
      EditorSaveMission();

   EditorDoLoadMission(expandFilename("~/data/newMission.mis"));
   EditorGui.saveAs = true;
   EWorldEditor.isDirty = true;
   ETerrainEditor.isDirty = true;
}

function EditorOpenMission()
{
   if(ETerrainEditor.isMissionDirty || ETerrainEditor.isDirty || EWorldEditor.isDirty)
   {
      MessageBoxYesNo("Mission Modified", "Would you like to save changes to the current mission \"" @
         $Server::MissionFile @ "\" before opening a new mission?", "EditorSaveBeforeLoad();", "getLoadFilename(\"*.mis\", \"EditorDoLoadMission\");");
   }
   else
      getLoadFilename("*.mis", "EditorDoLoadMission");
}

function EditorMenuBar::onMenuSelect(%this, %menuId, %menu)
{
   if(%menu $= "File")
   {
      %editingHeightfield = ETerrainEditor.isVisible() && EHeightField.isVisible();
      EditorMenuBar.setMenuItemEnable("File", "Export Terraform Bitmap...", %editingHeightfield);
      EditorMenuBar.setMenuItemEnable("File", "Save Mission...", ETerrainEditor.isDirty || ETerrainEditor.isMissionDirty || EWorldEditor.isDirty);
   }
   else if(%menu $= "Edit")
   {
      // enable/disable undo, redo, cut, copy, paste depending on editor settings

      if(EWorldEditor.isVisible())
      {
         // do actions based on world editor...
         EditorMenuBar.setMenuItemEnable("Edit", "Select All", true);
         EditorMenuBar.setMenuItemEnable("Edit", "Paste", EWorldEditor.canPasteSelection());
         %canCutCopy = EWorldEditor.getSelectionSize() > 0;

         EditorMenuBar.setMenuItemEnable("Edit", "Cut", %canCutCopy);
         EditorMenuBar.setMenuItemEnable("Edit", "Copy", %canCutCopy);

      }
      else if(ETerrainEditor.isVisible())
      {
         EditorMenuBar.setMenuItemEnable("Edit", "Cut", false);
         EditorMenuBar.setMenuItemEnable("Edit", "Copy", false);
         EditorMenuBar.setMenuItemEnable("Edit", "Paste", false);
         EditorMenuBar.setMenuItemEnable("Edit", "Select All", false);
      }
   }
   else if(%menu $= "World")
   {
      %selSize = EWorldEditor.getSelectionSize();
      %lockCount = EWorldEditor.getSelectionLockCount();
      %hideCount = EWorldEditor.getSelectionHiddenCount();

      EditorMenuBar.setMenuItemEnable("World", "Lock Selection", %lockCount < %selSize);
      EditorMenuBar.setMenuItemEnable("World", "Unlock Selection", %lockCount > 0);
      EditorMenuBar.setMenuItemEnable("World", "Hide Selection", %hideCount < %selSize);
      EditorMenuBar.setMenuItemEnable("World", "Show Selection", %hideCount > 0);

      EditorMenuBar.setMenuItemEnable("World", "Add Selection to Instant Group", %selSize > 0);
      EditorMenuBar.setMenuItemEnable("World", "Camera To Selection", %selSize > 0);
      EditorMenuBar.setMenuItemEnable("World", "Reset Transforms", %selSize > 0 && %lockCount == 0);
      EditorMenuBar.setMenuItemEnable("World", "Drop Selection", %selSize > 0 && %lockCount == 0);
      EditorMenuBar.setMenuItemEnable("World", "Delete Selection", %selSize > 0 && %lockCount == 0);
   }
}

function EditorMenuBar::onMenuItemSelect(%this, %menuId, %menu, %itemId, %item)
{
   switch$(%menu)
   {
      case "File":
         %this.onFileMenuItemSelect(%itemId, %item);
      case "Edit":
         %this.onEditMenuItemSelect(%itemId, %item);
      case "World":
         %this.onWorldMenuItemSelect(%itemId, %item);
      case "Window":
         %this.onWindowMenuItemSelect(%itemId, %item);
      case "Action":
         %this.onActionMenuItemSelect(%itemId, %item);
      case "Brush":
         %this.onBrushMenuItemSelect(%itemId, %item);
      case "Camera":
         %this.onCameraMenuItemSelect(%itemId, %item);

      case $sgEditorItemNames::sgMenu:
         %this.onToggleSGTools(%itemId, %item);
         
      case "MBU":
         %this.onMBUMenuItemSelect(%itemId, %item);
   }
}

function EditorMenuBar::onFileMenuItemSelect(%this, %itemId, %item)
{
   switch$(%item)
   {
      case "New Mission...":
         EditorNewMission();
      case "Open Mission...":
         EditorOpenMission();
      case "Save Mission...":
         EditorSaveMissionMenu();
      case "Save Mission As...":
         EditorSaveMissionAs();
      case "Import Texture Data...":
         Texture::import();
      case "Import Terraform Data...":
         Heightfield::import();
      case "Export Terraform Bitmap...":
         Heightfield::saveBitmap("");
      case "Toggle Map Editor...":
         Editor.close("PlayGui");
      case "Quit":
         EditorQuitMission();
   }
}

function EditorMenuBar::onMBUMenuItemSelect(%this, %itemId, %item)
{
   switch$(%item)
   {
      case "Set Preview Camera":
         EditorSetPreviewCamera();
   }
}

function EditorSetPreviewCamera()
{
	CameraObj.setTransform(LocalClientConnection.camera.getTransform());
	EWorldEditor.isDirty = true;
	
	echo("Preview Camera has been moved!");
}

function EditorMenuBar::onCameraMenuItemSelect(%this, %itemId, %item)
{
   switch$(%item)
   {
      case "Drop Camera at Player":
         commandToServer('dropCameraAtPlayer');
      case "Drop Player at Camera":
         commandToServer('DropPlayerAtCamera');
      case "Toggle Camera":
         commandToServer('ToggleCamera');
      default:
         // all the rest are camera speeds:
         // item ids go from 3 (slowest) to 9 (fastest)
         %this.setMenuItemChecked("Camera", %itemId, true);
         // camera movement speed goes from 5 to 200:
         $Camera::movementSpeed = ((%itemId - 3) / 6.0) * 195 + 5;
   }
}

function EditorMenuBar::onActionMenuItemSelect(%this, %itemId, %item)
{
   EditorMenuBar.setMenuItemChecked("Action", %item, true);
   switch$(%item)
   {
      case "Select":
         ETerrainEditor.currentMode = "select";
         ETerrainEditor.selectionHidden = false;
         ETerrainEditor.renderVertexSelection = true;
         ETerrainEditor.setAction("select");
      case "Adjust Selection":
         ETerrainEditor.currentMode = "adjust";
         ETerrainEditor.selectionHidden = false;
         ETerrainEditor.setAction("adjustHeight");
         ETerrainEditor.currentAction = brushAdjustHeight;
         ETerrainEditor.renderVertexSelection = true;
      default:
         ETerrainEditor.currentMode = "paint";
         ETerrainEditor.selectionHidden = true;
         ETerrainEditor.setAction(ETerrainEditor.currentAction);
         switch$(%item)
         {
            case "Add Dirt":
               ETerrainEditor.currentAction = raiseHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Paint Material":
               ETerrainEditor.currentAction = paintMaterial;
               ETerrainEditor.renderVertexSelection = true;
            case "Excavate":
               ETerrainEditor.currentAction = lowerHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Set Height":
               ETerrainEditor.currentAction = setHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Adjust Height":
               ETerrainEditor.currentAction = brushAdjustHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Flatten":
               ETerrainEditor.currentAction = flattenHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Smooth":
               ETerrainEditor.currentAction = smoothHeight;
               ETerrainEditor.renderVertexSelection = true;
            case "Set Empty":
               ETerrainEditor.currentAction = setEmpty;
               ETerrainEditor.renderVertexSelection = false;
            case "Clear Empty":
               ETerrainEditor.currentAction = clearEmpty;
               ETerrainEditor.renderVertexSelection = false;
         }
         if(ETerrainEditor.currentMode $= "select")
            ETerrainEditor.processAction(ETerrainEditor.currentAction);
         else if(ETerrainEditor.currentMode $= "paint")
            ETerrainEditor.setAction(ETerrainEditor.currentAction);
   }
}

function EditorMenuBar::onBrushMenuItemSelect(%this, %itemId, %item)
{
   EditorMenuBar.setMenuItemChecked("Brush", %item, true);
   switch$(%item)
   {
      case "Box Brush":
         ETerrainEditor.setBrushType(box);
      case "Circle Brush":
         ETerrainEditor.setBrushType(ellipse);
      case "Soft Brush":
         ETerrainEditor.enableSoftBrushes = true;
      case "Hard Brush":
         ETerrainEditor.enableSoftBrushes = false;
      default:
         // the rest are brush sizes:
         ETerrainEditor.brushSize = %itemId;
         
         ETerrainEditor.setBrushSize(%itemId, %itemId);
   }
}

function EditorMenuBar::onWorldMenuItemSelect(%this, %itemId, %item)
{
   // edit commands for world editor...
   switch$(%item)
   {
      case "Lock Selection":
         EWorldEditor.lockSelection(true);
      case "Unlock Selection":
         EWorldEditor.lockSelection(false);
      case "Hide Selection":
         EWorldEditor.hideSelection(true);
      case "Show Selection":
         EWorldEditor.hideSelection(false);
      case "Camera To Selection":
         EWorldEditor.dropCameraToSelection();
      case "Reset Transforms":
         EWorldEditor.resetTransforms();
      case "Drop Selection":
         EWorldEditor.dropSelection();
      case "Delete Selection":
         EWorldEditor.deleteSelection();
      case "Add Selection to Instant Group":
         EWorldEditor.addSelectionToAddGroup();
      default:
         EditorMenuBar.setMenuItemChecked("World", %item, true);
         switch$(%item)
         {
            case "Drop at Origin":
               EWorldEditor.dropType = "atOrigin";
            case "Drop at Camera":
               EWorldEditor.dropType = "atCamera";
            case "Drop at Camera w/Rot":
               EWorldEditor.dropType = "atCameraRot";
            case "Drop below Camera":
               EWorldEditor.dropType = "belowCamera";
            case "Drop at Screen Center":
               EWorldEditor.dropType = "screenCenter";
            case "Drop to Ground":
               EWorldEditor.dropType = "toGround";
            case "Drop at Centroid":
               EWorldEditor.dropType = "atCentroid";
         }
   }
}

function EditorMenuBar::onEditMenuItemSelect(%this, %itemId, %item)
{
   if(%item $= "World Editor Settings...")
      Canvas.pushDialog(WorldEditorSettingsDlg);
   else if(%item $= "Terrain Editor Settings...")
      Canvas.pushDialog(TerrainEditorValuesSettingsGui, 99);
   else if (%item $= "Model Importer Settings...") {
      ModelImporterSettingsDlg.onWake();
      Canvas.pushDialog(ModelImporterSettingsDlg, 99);
   } else if(%item $= "Relight Scene")
      lightScene("", forceAlways);
   else if(EWorldEditor.isVisible())
   {
      // edit commands for world editor...
      switch$(%item)
      {
         case "Undo":
            EWorldEditor.undo();
         case "Redo":
            EWorldEditor.redo();
         case "Copy":
            EWorldEditor.copySelection();
         case "Cut":
            EWorldEditor.copySelection();
            EWorldEditor.deleteSelection();
         case "Paste":
            EWorldEditor.pasteSelection();
         case "Select All":
         case "Select None":
      }
   }
   else if(ETerrainEditor.isVisible())
   {
      // do some terrain stuffin'
      switch$(%item)
      {
         case "Undo":
            ETerrainEditor.undo();
         case "Redo":
            ETerrainEditor.redo();
         case "Select None":
            ETerrainEditor.clearSelection();
      }
   }
}

function EditorMenuBar::onToggleSGTools(%this, %itemId, %item)
{
   EditorGui.toggleSGTools(%item);
}

function EditorMenuBar::onWindowMenuItemSelect(%this, %itemId, %item)
{
   EditorGui.setEditor(%item);
}

function Creator::onWake(%this)
{
   Creator.init();
}

function EditorGui::setWorldEditorVisible(%this)
{
   EWorldEditor.setVisible(true);
   ETerrainEditor.setVisible(false);
   EditorMenuBar.setMenuVisible("World", true);
   EditorMenuBar.setMenuVisible("Action", false);
   EditorMenuBar.setMenuVisible("Brush", false);
   EWorldEditor.makeFirstResponder(true);
   EditorTree.open(MissionGroup,true);
}

function EditorGui::setTerrainEditorVisible(%this)
{
   EWorldEditor.setVisible(false);
   ETerrainEditor.setVisible(true);
   ETerrainEditor.attachTerrain();
   EHeightField.setVisible(false);
   ETexture.setVisible(false);
   EditorMenuBar.setMenuVisible("World", false);
   EditorMenuBar.setMenuVisible("Action", true);
   EditorMenuBar.setMenuVisible("Brush", true);
   ETerrainEditor.makeFirstResponder(true);
   EPainter.setVisible(false);
}

function EditorGui::toggleSGTools(%this, %item)
{
   switch$(%item)
   {
      case $sgEditorItemNames::sgMenuItem[0]:
         sgLightEditor.toggle();
      case $sgEditorItemNames::sgMenuItem[2]:
         sgLightEditor.filteredRelight();
      case $sgEditorItemNames::sgMenuItem[3]:
         lightScene("", forceAlways);
   }
}

function EditorGui::setEditor(%this, %editor)
{
   EditorMenuBar.setMenuItemBitmap("Window", %this.currentEditor, -1);
   EditorMenuBar.setMenuItemBitmap("Window", %editor, 0);
   %this.currentEditor = %editor;

   switch$(%editor)
   {
      case "World Editor":
         EWFrame.setVisible(false);
         EWMissionArea.setVisible(false);
         %this.setWorldEditorVisible();
      case "World Editor Inspector":
         EWFrame.setVisible(true);
         EWMissionArea.setVisible(false);
         EWCreatorPane.setVisible(false);
         EWInspectorPane.setVisible(true);
         %this.setWorldEditorVisible();
      case "World Editor Creator":
         EWFrame.setVisible(true);
         EWMissionArea.setVisible(false);
         EWCreatorPane.setVisible(true);
         EWInspectorPane.setVisible(false);
         %this.setWorldEditorVisible();
      case "Mission Area Editor":
         EWFrame.setVisible(false);
         EWMissionArea.setVisible(true);
         %this.setWorldEditorVisible();
      case "Terrain Editor":
         %this.setTerrainEditorVisible();
      case "Terrain Terraform Editor":
         %this.setTerrainEditorVisible();
         EHeightField.setVisible(true);
      case "Terrain Texture Editor":
         %this.setTerrainEditorVisible();
         ETexture.setVisible(true);
      case "Terrain Texture Painter":
         %this.setTerrainEditorVisible();
         EPainter.setVisible(true);
         EPainter.setup();

   }
}

function EditorGui::getHelpPage(%this)
{
   switch$(%this.currentEditor)
   {
      case "World Editor" or "World Editor Inspector" or "World Editor Creator":
         return "5. World Editor";
      case "Mission Area Editor":
         return "6. Mission Area Editor";
      case "Terrain Editor":
         return "7. Terrain Editor";
      case "Terrain Terraform Editor":
         return "8. Terrain Terraform Editor";
      case "Terrain Texture Editor":
         return "9. Terrain Texture Editor";
      case "Terrain Texture Painter":
         return "10. Terrain Texture Painter";
   }
}


function ETerrainEditor::setPaintMaterial(%this, %matIndex)
{
   ETerrainEditor.paintMaterial = EPainter.mat[%matIndex];
}

function ETerrainEditor::changeMaterial(%this, %matIndex)
{
   EPainter.matIndex = %matIndex;
   getLoadFilename("*.png\t*.jpg", EPainterChangeMat);
}

function EPainterChangeMat(%file)
{
   // make sure the material isn't already in the terrain.
   %file = filePath(%file) @ "/" @ fileBase(%file);
   for(%i = 0; %i < 6; %i++)
      if(EPainter.mat[%i] $= %file)
         return;

   EPainter.mat[EPainter.matIndex] = %file;
   %mats = "";
   for(%i = 0; %i < 6; %i++)
      %mats = %mats @ EPainter.mat[%i] @ "\n";
   ETerrainEditor.setTerrainMaterials(%mats);
   EPainter.setup();
   ("ETerrainMaterialPaint" @ EPainter.matIndex).performClick();
}

function EPainter::setup(%this)
{
   EditorMenuBar.onActionMenuItemSelect(0, "Paint Material");
   %mats = ETerrainEditor.getTerrainMaterials();
   %valid = true;
   for(%i = 0; %i < 6; %i++)
   {
      %mat = getRecord(%mats, %i);
      %this.mat[%i] = %mat;
      ("ETerrainMaterialText" @ %i).setText(fileBase(%mat));
      ("ETerrainMaterialBitmap" @ %i).setBitmap(%mat);
      ("ETerrainMaterialChange" @ %i).setActive(true);
      ("ETerrainMaterialPaint" @ %i).setActive(%mat !$= "");
      if(%mat $= "")
      {
         ("ETerrainMaterialChange" @ %i).setText("Add...");
         if(%valid)
            %valid = false;
         else
            ("ETerrainMaterialChange" @ %i).setActive(false);
      }
      else
         ("ETerrainMaterialChange" @ %i).setText("Change...");
   }
   ETerrainMaterialPaint0.performClick();
}

function EditorGui::onWake(%this)
{
   MoveMap.push();
   EditorMap.push();
   %this.setEditor(%this.currentEditor);
}

function EditorGui::onSleep(%this)
{
   EditorMap.pop();
   MoveMap.pop();
   $Server::CurrentScene.open();
}

function AreaEditor::onUpdate(%this, %area)
{
   AreaEditingText.setValue( "X: " @ getWord(%area,0) @ " Y: " @ getWord(%area,1) @ " W: " @ getWord(%area,2) @ " H: " @ getWord(%area,3));
}

function AreaEditor::onWorldOffset(%this, %offset)
{
}

function WorldEditor::onDelete(%this)
{
   EditorTree.deleteSelection();
}

function WorldEditor::onSelect(%this,%obj)
{
   EditorTree.addSelection(%obj); 
}

function WorldEditor::onUnSelect(%this,%obj)
{ 
   EditorTree.removeSelection(%obj);
}

function WorldEditor::onClearSelected(%this)
{
   EditorTree.clearSelection();
}

function WorldEditor::onClearSelection(%this)
{
   EditorTree.clearSelection();
}

function EditorTree::onClearSelected(%this)
{
   WorldEditor.clearSelection();
}

function EditorTree::init(%this)
{
   //%this.open(MissionGroup);

   // context menu
   new GuiControl(ETContextPopupDlg)
   {
    profile = "GuiModelessDialogProfile";
      horizSizing = "width";
      vertSizing = "height";
      position = "0 0";
      extent = "640 480";
      minExtent = "8 8";
      visible = "1";
      setFirstResponder = "0";
      modal = "1";
      
      new GuiPopUpMenuCtrl(ETContextPopup)
      {
         profile = "GuiScrollProfile";
         position = "0 0";
         extent = "0 0";
         minExtent = "0 0";
         maxPopupHeight = "200";
         command = "canvas.popDialog(ETContextPopupDlg);";
      };
   };
   ETContextPopup.setVisible(false);
}

function EditorTree::onInspect(%this, %obj)
{
   Inspector.inspect(%obj);
   InspectorNameEdit.setValue(%obj.getName());
}
function EditorTree::onAddSelection(%this, %obj)
{
   if($AIEdit)   
      aiEdit.selectObject(%obj);
   else
      EWorldEditor.selectObject(%obj);
}
function EditorTree::onRemoveSelection(%this, %obj)
{
   if($AIEdit)   
      aiEdit.selectObject(%obj);
   else
      EWorldEditor.unselectObject(%obj);
}
function EditorTree::onSelect(%this, %obj)
{
   if($AIEdit)   
      aiEdit.selectObject(%obj);
   else {
      EWorldEditor.selectObject(%obj);   
   }

}

function EditorTree::onUnselect(%this, %obj)
{
   if($AIEdit)
      aiEdit.unselectObject(%obj);
   else
      EWorldEditor.unselectObject(%obj);
}

function ETContextPopup::onSelect(%this, %index, %value)
{
   switch(%index)
   {
      case 0:
         EditorTree.contextObj.delete();
   }
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

function WorldEditor::init(%this)
{
   // add objclasses which we do not want to collide with
   %this.ignoreObjClass(TerrainBlock, Sky, AIObjective);

   // editing modes
   %this.numEditModes = 3;
   %this.editMode[0]    = "move";
   %this.editMode[1]    = "rotate";
   %this.editMode[2]    = "scale";

   // context menu
   new GuiControl(WEContextPopupDlg)
   {
      profile = "GuiModelessDialogProfile";
      horizSizing = "width";
      vertSizing = "height";
      position = "0 0";
      extent = "640 480";
      minExtent = "8 8";
      visible = "1";
      setFirstResponder = "0";
      modal = "1";
      
      new GuiPopUpMenuCtrl(WEContextPopup)
      {
         profile = "GuiScrollProfile";
         position = "0 0";
         extent = "0 0";
         minExtent = "0 0";
         maxPopupHeight = "200";
         command = "canvas.popDialog(WEContextPopupDlg);";
      };
   };
   WEContextPopup.setVisible(false);
}

//------------------------------------------------------------------------------

function WorldEditor::onDblClick(%this, %obj)
{
   // Commented out because making someone double click to do this is stupid
   // and has the possibility of moving hte object
   
   //Inspector.inspect(%obj);
   //InspectorNameEdit.setValue(%obj.getName());
}

function WorldEditor::onClick( %this, %obj )
{
   Inspector.inspect( %obj );
   InspectorNameEdit.setValue( %obj.getName() );
}

//------------------------------------------------------------------------------

function WorldEditor::export(%this)
{
   getSaveFilename("~/editor/*.mac", %this @ ".doExport", "selection.mac");
}

function WorldEditor::doExport(%this, %file)
{
   missionGroup.save("~/editor/" @ %file, true);
}

function WorldEditor::import(%this)
{
   getLoadFilename("~/editor/*.mac", %this @ ".doImport");
}

function WorldEditor::doImport(%this, %file)
{
   exec("~/editor/" @ %file);
}

function WorldEditor::onGuiUpdate(%this, %text)
{
}

function WorldEditor::getSelectionLockCount(%this)
{
   %ret = 0;
   for(%i = 0; %i < %this.getSelectionSize(); %i++)
   {
      %obj = %this.getSelectedObject(%i);
      if(%obj.locked $= "true")
         %ret++;
   }
   return %ret;
}

function WorldEditor::getSelectionHiddenCount(%this)
{
   %ret = 0;
   for(%i = 0; %i < %this.getSelectionSize(); %i++)
   {
      %obj = %this.getSelectedObject(%i);
      if(%obj.hidden $= "true")
         %ret++;
   }
   return %ret;
}

function WorldEditor::dropCameraToSelection(%this)
{
   if(%this.getSelectionSize() == 0)
      return;

   %pos = %this.getSelectionCentroid();
   %cam = LocalClientConnection.camera.getTransform();

   // set the pnt
   %cam = setWord(%cam, 0, getWord(%pos, 0));
   %cam = setWord(%cam, 1, getWord(%pos, 1));
   %cam = setWord(%cam, 2, getWord(%pos, 2));

   LocalClientConnection.camera.setTransform(%cam);
}

// * pastes the selection at the same place (used to move obj from a group to another)
function WorldEditor::moveSelectionInPlace(%this)
{
   %saveDropType = %this.dropType;
   %this.dropType = "atCentroid";
   %this.copySelection();
   %this.deleteSelection();
   %this.pasteSelection();
   %this.dropType = %saveDropType;
}

function WorldEditor::addSelectionToAddGroup(%this)
{
   for(%i = 0; %i < %this.getSelectionSize(); %i++) 
   {
      %obj = %this.getSelectedObject(%i);
      $InstantGroup.add(%obj);
   }
}   

// resets the scale and rotation on the selection set
function WorldEditor::resetTransforms(%this)
{
   %this.addUndoState();

   for(%i = 0; %i < %this.getSelectionSize(); %i++)
   {
      %obj = %this.getSelectedObject(%i);
      %transform = %obj.getTransform();

      %transform = setWord(%transform, 3, "0");
      %transform = setWord(%transform, 4, "0");
      %transform = setWord(%transform, 5, "1");
      %transform = setWord(%transform, 6, "0");
         
      //
      %obj.setTransform(%transform);
      %obj.setScale("1 1 1");
   }
}


function WorldEditorToolbarDlg::init(%this)
{
   WorldEditorInspectorCheckBox.setValue(WorldEditorToolFrameSet.isMember("EditorToolInspectorGui"));
   WorldEditorMissionAreaCheckBox.setValue(WorldEditorToolFrameSet.isMember("EditorToolMissionAreaGui"));
   WorldEditorTreeCheckBox.setValue(WorldEditorToolFrameSet.isMember("EditorToolTreeViewGui"));
   WorldEditorCreatorCheckBox.setValue(WorldEditorToolFrameSet.isMember("EditorToolCreatorGui"));
}

function Creator::addModelSearch(%this,  %extension, %base)
{
   %file = findFirstFile( %extension );
   %staticId = "";
   while( %file !$= "" ) 
   {
      // Determine which group to put the file in
      // and build the group heirarchy as we go
      %split    = strreplace(%file, "/", " ");
      %dirCount = getWordCount(%split)-1;
      %parentId = %base;
      
      for(%i=0; %i<%dirCount; %i++)
      {
         %parent = getWords(%split, 0, %i);
         // if the group doesn't exist create it
         if ( !%staticId[%parent] )
            %staticId[%parent] = %this.insertItem( %parentId, getWord(%split, %i));
         %parentId = %staticId[%parent];
      }
      // Add the file to the group
      %create = "TSStatic::create(\"" @ %file @ "\", \"" @ %extension @ "\");";
      %this.insertItem( %parentId, fileBase( %file ), %create,"TSStatic" );
   
      %file = findNextFile( %extension );
   }
}

function Creator::init( %this ) 
{
   //%this.clear();
   $InstantGroup = "MissionGroup";

   // ---------- INTERIORS    
   %base = %this.insertItem( 0, "Interiors" );

   // walk all the interiors and add them to the correct group
   %interiorId = "";
   %file = findFirstFile( "*.dif" );
   
   while( %file !$= "" ) 
   {
      // Determine which group to put the file in
      // and build the group hierarchy as we go
      %split    = strreplace(%file, "/", " ");
      %dirCount = getWordCount(%split)-1;
      %parentId = %base;
      
      for(%i=0; %i<%dirCount; %i++)
      {
         %parent = getWords(%split, 0, %i);
         
         // if the group doesn't exist create it
         if ( !%interiorId[%parent] )
            %interiorId[%parent] = %this.insertItem( %parentId, getWord(%split, %i));
         %parentId = %interiorId[%parent];
      }
      
      // Add the file to the group
      %create = "createInterior(" @ "\"" @ %file @ "\"" @ ");";
      %this.insertItem( %parentId, fileBase( %file ), %create,"Interior" );
   
      %file = findNextFile( "*.dif" );
   }


   // ---------- SHAPES - add in all the shapes now...
   %base = %this.insertItem(0, "Shapes");
   %dataGroup = "DataBlockGroup";
   
   for(%i = 0; %i < %dataGroup.getCount(); %i++)
   {
      %obj = %dataGroup.getObject(%i);
      echo ("Obj: " @ %obj.getName() @ " - " @ %obj.category );
      if(%obj.category !$= "" || %obj.category != 0)
      {
         %id = %this.findItemByName(%obj.category);
           if (%id  == 0) 
           { 
            %grp = %this.insertItem(%base,%obj.category);
            %this.insertItem(%grp, %obj.getName(), %obj.getClassName() @ "::create(" @ %obj.getName() @ ");","Item");
         }
         else
         {
            %this.insertItem(%id, %obj.getName(), %obj.getClassName() @ "::create(" @ %obj.getName() @ ");","Item");
         }
      }
   }


   // ---------- Static Shapes    
   %base = %this.insertItem( 0, "Static Shapes" );


   %extensions = "*.dae *.x *.x3d *.xgl *.fbx *.blend *.3ds *.3mf *.ase *.obj *.ply *.dxf *.lwo *.lws *.lxo *.stl *.ac *.amf *.ms3d *.cob *.scn *.xml *.irrmesh *.irr *.mdl";
   %extensions = %extensions SPC "*.md2 *.md3 *.pk3 *.mdc *.mmd *.smd *.vta *.3d *.b3d *.q3d *.q3s *.nff *.off *.hmp *.ndo *.gltf *.glb *.dae";
                        
   // walk all the statics and add them to the correct group
   %staticId = "";
   %file = findFirstFile( "*.dts" );
   while( %file !$= "" ) 
   {
      // Determine which group to put the file in
      // and build the group heirarchy as we go
      %split    = strreplace(%file, "/", " ");
      %dirCount = getWordCount(%split)-1;
      %parentId = %base;

      if (strStr(%file, ".cached.dts") != -1)
      {
          // check if dae exists by same name
          %doContinue = false;
          for (%k = 0; %k < getWordCount(%extensions); %k++)
          {
              %ext = strreplace(getWord(%extensions, %k), "*", "");
              %daeName = strreplace(%file, ".cached.dts", %ext);
              if (isFile(%daeName)) 
              {
                 %file = findNextFile( "*.dts" );
                 %doContinue = true;
                 break;
              }
          }
          if (%doContinue) continue;
                                }                      
      
      for(%i=0; %i<%dirCount; %i++)
      {
         %parent = strlwr(getWords(%split, 0, %i));
         // if the group doesn't exist create it
         if ( %staticId[%parent] $= "" )
            %staticId[%parent] = %this.insertItem( %parentId, getWord(%split, %i));
         %parentId = %staticId[%parent];
      }
      // Add the file to the group
      %create = "TSStatic::create(\"" @ %file @ "\");";
      %this.insertItem( %parentId, fileBase( %file ), %create,"TSStatic" );
   
      %file = findNextFile( "*.dts" );
   }
                        
   for (%k = 0; %k < getWordCount(%extensions); %k++)
   {
       %extension = getWord(%extensions, %k);
       %file = findFirstFile( %extension );
       while( %file !$= "" ) 
       {
          // Determine which group to put the file in
          // and build the group heirarchy as we go
          %split    = strreplace(%file, "/", " ");
          %dirCount = getWordCount(%split)-1;
          %parentId = %base;
      
          for(%i=0; %i<%dirCount; %i++)
          {
             %parent = getWords(%split, 0, %i);
             // if the group doesn't exist create it
             if ( !%staticId[%parent] )
                %staticId[%parent] = %this.insertItem( %parentId, getWord(%split, %i));
             %parentId = %staticId[%parent];
          }
          // Add the file to the group
          %create = "TSStatic::create(\"" @ %file @ "\", \"" @ %extension @ "\");";
          %this.insertItem( %parentId, fileBase( %file ), %create,"TSStatic" );
   
          %file = findNextFile( %extension );
       }
   }


   // *** OBJECTS - do the objects now...
   %objGroup[0] = "Environment";
   %objGroup[1] = "Mission";
   %objGroup[2] = "System";
   //%objGroup[3] = "AI";

   %Environment_Item[0]  = "Sky";
   %Environment_Item[1]  = "Sun";
   %Environment_Item[2]  = "Lightning";
   %Environment_Item[3]  = "Water";
   %Environment_Item[4]  = "Terrain";
   %Environment_Item[5]  = "AudioEmitter";
   %Environment_Item[6]  = "Precipitation";
   %Environment_Item[7]  = "ParticleEmitter";
   %Environment_Item[8]  = "fxSunLight";
   %Environment_Item[9]  = "fxShapeReplicator";
   %Environment_Item[10] = "fxFoliageReplicator";
   %Environment_Item[11] = "fxLight";
   
   %Environment_Item[12] = "sgLightObject";
   %Environment_Item[13] = "VolumeLight";
   %Environment_Item[14] = "sgMissionLightingFilter";
   %Environment_Item[15] = "sgDecalProjector";
   
   %Mission_Item[0] = "MissionArea";
   %Mission_Item[1] = "Path";
   %Mission_Item[2] = "PathMarker";
   %Mission_Item[3] = "Trigger";
   %Mission_Item[4] = "PhysicalZone";
   %Mission_Item[5] = "Camera";
   //%Mission_Item[5] = "GameType";
   //%Mission_Item[6] = "Forcefield";

   %System_Item[0] = "SimGroup";

   //%AI_Item[0] = "Objective";
   //%AI_Item[1] = "NavigationGraph";

   // objects group
   %base = %this.insertItem(0, "Mission Objects");

   // create 'em
   for(%i = 0; %objGroup[%i] !$= ""; %i++)
   {
      %grp = %this.insertItem(%base, %objGroup[%i]);

      %groupTag = "%" @ %objGroup[%i] @ "_Item";

      %done = false;
      for(%j = 0; !%done; %j++)
      {
         eval("%itemTag = " @ %groupTag @ %j @ ";");
         
         if(%itemTag $= "")
            %done = true;
         else 
         {
            //echo("itemTag:" @ %itemTag @ " j:" @ %j);
            %this.insertItem(%grp, %itemTag, "ObjectBuilderGui.build" @ %itemTag @ "();",%itemTag);
         }
      }
   }
}

function createInterior(%name)
{
   %obj = new InteriorInstance()
   {
      position = "0 0 0";
      rotation = "0 0 0";
      interiorFile = %name;
   };
   
   return(%obj);
}

//function Creator::onAction(%this)
//{
//   %this.currentSel = -1;
//   %this.currentRoot = -1;
//   %this.currentObj = -1;

  // %sel = %this.getSelected();
  // if(%sel == -1 || %this.isGroup(%sel) || !$missionRunning)
  //    return;
      
   // the value is the callback function..
  // if(%this.getValue(%sel) $= "")
  //    return;

//   %this.currentSel = %sel;
//   %this.currentRoot = %this.getRootGroup(%sel);

  // %this.create(%sel);
//}

function WorldEditor::onAddSelected(%this,%obj)
{
   EditorTree.addSelection(%obj);
}

function Creator::onSelect(%this)
{
   Creator.clearSelection();
}

function Creator::onInspect(%this,%obj)
{
   if(!$missionRunning)
      return;
      
   %objId = eval(%this.getItemValue(%obj));
      
   // drop it from the editor - only SceneObjects can be selected...   
   Creator.removeSelection(%obj);
   
   EditorTree.clearSelection();
   EWorldEditor.clearSelection();
   EWorldEditor.selectObject(%objId);
   EWorldEditor.dropSelection();
}

function Creator::create(%this, %sel)
{
   // create the obj and add to the instant group
   %obj = eval(%this.getItemValue(%sel));

   if(%obj == -1)
      return;

//   %this.currentObj = %obj;

   $InstantGroup.add(%obj);
      
   // drop it from the editor - only SceneObjects can be selected...
   EWorldEditor.clearSelection();
   EWorldEditor.selectObject(%obj);
   EWorldEditor.dropSelection();
}


function TSStatic::create(%shapeName, %extension)
{
   if ( %extension $= "*.dae" )
   {
      %shapeName = convertCollada(%shapeName);
      if (%shapeName $= "") return 0;
   } else if (%extension !$= "")
   {
      %shapeName = convertAssimp(%shapeName);
      if (%shapeName $= "") return 0;
   }
   %obj = new TSStatic() 
   {
      shapeName = %shapeName;
   };
   return(%obj);
}

function TSStatic::damage(%this)
{
   // prevent console error spam
}


//function Creator::getRootGroup(%sel)
//{
//   if(%sel == -1 || %sel == 0)
//      return(-1);
//
//   %parent = %this.getParent(%sel);
//   while(%parent != 0 || %parent != -1)
//   {
//      %sel = %parent;
//      %parent = %this.getParent(%sel);
//   }
//
//   return(%sel);
//}
//   
//function Creator::getLastItem(%rootGroup)
//{
//   %traverse = %rootGroup + 1;
//   while(%this.getRootGroup(%traverse) == %rootGroup)
//      %traverse++;
//   return(%traverse - 1);
//}
//
//function Creator::createNext(%this)
//{
//   if(%this.currentSel == -1 || %this.currentRoot == -1 || %this.currentObj == -1)
//      return;
//
//   %sel = %this.currentSel;
//   %this.currentSel++;
//
//   while(%this.currentSel != %sel)
//   {
//      if(%this.getRootGroup(%this.currentSel) != %this.currentRoot)
//         %this.currentSel = %this.currentRoot + 1;
//
//      if(%this.isGroup(%this.currentSel))
//         %this.currentSel++;
//      else
//         %sel = %this.currentSel;
//   }
//
//   //
//   %this.currentObj.delete();
//   %this.create(%sel);
//}
//
//function Creator::createPrevious(%this)
//{
//   if(%this.currentSel == -1 || %this.currentGroup == -1 || %this.currentObj == -1)
//      return;
//
//   %sel = %this.currentSel;
//   %this.currentSel--;
//
//   while(%this.currentSel != %sel)
//   {
//      if(%this.getRootGroup(%this.currentSel) != %this.currentRoot)
//         %this.currentSel = getLastItem(%this.currentRoot);
//
//      if(%this.isGroup(%this.currentSel))
//         %this.currentSel--;
//      else
//         %sel = %this.currentSel;
//   }
//
//   //
//   %this.currentObj.delete();
//   %this.create(%sel);
//}


function TerraformerGui::init(%this)
{
   TerraformerHeightfieldGui.init();
   TerraformerTextureGui.init();
}

function TerraformerGui::onWake(%this)
{
   // Only the canvas level gui's get wakes, so udpate manually.
   TerraformerTextureGui.update();
}

function TerraformerGui::onSleep(%this)
{
   %this.setPrefs();
}

$nextTextureId = 1;
$nextTextureRegister = 1000;
$selectedMaterial = -1;
$selectedTextureOperation = -1;
$TerraformerTextureDir = "common/editor/textureScripts";

//--------------------------------------

function TextureInit()
{
   // Assumes the terrain object is called terrain
   
   Texture_operation_menu.clear();
   Texture_operation_menu.setText("Placement Operations");
   Texture_operation_menu.add("Place by Fractal", 1);
   Texture_operation_menu.add("Place by Height", 2);
   Texture_operation_menu.add("Place by Slope", 3);
   Texture_operation_menu.add("Place by Water Level", 4);

   $HeightfieldSrcRegister = Heightfield_operation.rowCount()-1;

   // sync up the preview windows
   TexturePreview.setValue(HeightfieldPreview.getValue());
   %script = terrain.getTextureScript();
   if(%script !$= "")
      Texture::loadFromScript(%script);

   if (Texture_material.rowCount() == 0)
   {
      Texture_operation.clear();
      $nextTextureRegister = 1000;
   }
   else
   {
      // it's difficult to tell if the heightfield was modified so 
      // just in case flag all dependent operations as dirty.
      %rowCount = Texture_material.rowCount();
      for (%row = 0; %row < %rowCount; %row++)
      {
         %data = Texture_material.getRowText(%row);
         %entry= getRecord(%data,0);
         %reg  = getField(%entry,1);
         $dirtyTexture[ %reg ] = true;

         %opCount = getRecordCount(%data);
         for (%op = 2; %op < %opCount; %op++)
         {
            %entry= getRecord(%data,%op);
            %label= getField(%entry,0);
            if (%label !$= "Place by Fractal" && %label !$= "Fractal Distortion")
            {
               %reg  = getField(%entry,2);
               $dirtyTexture[ %reg ] = true;
            }
         }
      }
      Texture::previewMaterial();   
   }
}

function TerraformerTextureGui::refresh(%this)
{
}   


//--------------------------------------
function Texture_material_menu::onSelect(%this, %id, %text)
{
   %this.setText("Materials");

   // FORMAT
   //   material name
   //   register
   //     operation
   //       name
   //       tab name
   //       register
   //       distortion register
   //       {field,value}, ...
   //     operation
   //       ...
   Texture::saveMaterial();
   Texture::hideTab();   
   %id = Texture::addMaterial(%text @ "\t" @ $nextTextureRegister++);
   
   if (%id != -1)
   {
      Texture_material.setSelectedById(%id);
      Texture::addOperation("Fractal Distortion\ttab_DistortMask\t" @ $nextTextureRegister++ @ "\t0\tdmask_interval\t20\tdmask_rough\t0\tdmask_seed\t" @ terraFormer.generateSeed() @ "\tdmask_filter\t0.00000 0.00000 0.13750 0.487500 0.86250 1.00000 1.00000");
   }
}   


function Texture::addMaterialTexture()
{
   %root = filePath(terrain.terrainFile);
   getLoadFilename("*.png\t*.jpg", addLoadedMaterial);
}

function addLoadedMaterial(%file)
{
   Texture::saveMaterial();
   Texture::hideTab();   
   %text = filePath(%file) @ "/" @ fileBase(%file);
   %id = Texture::addMaterial(%text @ "\t" @ $nextTextureRegister++);
   if (%id != -1)
   {
      Texture_material.setSelectedById(%id);
      Texture::addOperation("Fractal Distortion\ttab_DistortMask\t" @ $nextTextureRegister++ @ "\t0\tdmask_interval\t20\tdmask_rough\t0\tdmask_seed\t" @ terraFormer.generateSeed() @ "\tdmask_filter\t0.00000 0.00000 0.13750 0.487500 0.86250 1.00000 1.00000");
   }
   Texture::save();
}

//--------------------------------------
function Texture_material::onSelect(%this, %id, %text) 
{
   Texture::saveMaterial();
   if (%id != $selectedMaterial)
   {
      $selectedTextureOperation = -1;
      Texture_operation.clear();
      
      Texture::hideTab();
      Texture::restoreMaterial(%id);
   }

   %matName = getField(%text, 0);
   ETerrainEditor.paintMaterial = %matName;

   Texture::previewMaterial(%id);
   $selectedMaterial = %id;
   $selectedTextureOperation = -1;
   Texture_operation.clearSelection();
}   


//--------------------------------------
function Texture_operation_menu::onSelect(%this, %id, %text)
{
   %this.setText("Placement Operations");
   %id = -1;

   if ($selectedMaterial == -1)
      return;

   %dreg = getField(Texture_operation.getRowText(0),2);
                     
   switch$ (%text)
   {
      case "Place by Fractal":
         %id = Texture::addOperation("Place by Fractal\ttab_FractalMask\t" @ $nextTextureRegister++ @ "\t" @ %dreg @ "\tfbmmask_interval\t16\tfbmmask_rough\t0.000\tfbmmask_seed\t" @ terraFormer.generateSeed() @ "\tfbmmask_filter\t0.000000 0.166667 0.333333 0.500000 0.666667 0.833333 1.000000\tfBmDistort\ttrue");

      case "Place by Height":
         %id = Texture::addOperation("Place by Height\ttab_HeightMask\t" @ $nextTextureRegister++ @ "\t" @ %dreg @ "\ttextureHeightFilter\t0 0.2 0.4 0.6 0.8 1.0\theightDistort\ttrue");

      case "Place by Slope":
         %id = Texture::addOperation("Place by Slope\ttab_SlopeMask\t" @ $nextTextureRegister++ @ "\t" @ %dreg @ "\ttextureSlopeFilter\t0 0.2 0.4 0.6 0.8 1.0\tslopeDistort\ttrue");

      case "Place by Water Level":
         %id = Texture::addOperation("Place by Water Level\ttab_WaterMask\t" @ $nextTextureRegister++ @ "\t" @ %dreg @ "\twaterDistort\ttrue");
   }

   // select it
   Texture::hideTab();
   if (%id != -1)
      Texture_operation.setSelectedById(%id);
}   


//--------------------------------------
function Texture_operation::onSelect(%this, %id, %text)
{
   Texture::saveOperation();   
   if (%id !$= $selectedTextureOperation)   
   {
      Texture::hideTab();   
      Texture::restoreOperation(%id);
      Texture::showTab(%id);   
   }

   Texture::previewOperation(%id);
   $selectedTextureOperation = %id;
}   


//--------------------------------------
function Texture::deleteMaterial(%id)
{
   if (%id $= "")
      %id = $selectedMaterial;
   if (%id == -1)   
      return;
   
   %row = Texture_material.getRowNumById(%id);
   
   Texture_material.removeRow(%row);

   // find the next row to select
   %rowCount = Texture_material.rowCount()-1;
   if (%row > %rowCount)
      %row = %rowCount;

   if (%id == $selectedMaterial)
      $selectedMaterial = -1;

   Texture_operation.clear();
   %id = Texture_material.getRowId(%row);
   Texture_material.setSelectedById(%id);
   Texture::save();
}   


//--------------------------------------
function Texture::deleteOperation(%id)
{
   if (%id $= "")
      %id = $selectedTextureOperation;
   if (%id == -1)   
      return;
   
   %row = Texture_operation.getRowNumById(%id);
   
   // don't delete the first entry
   if (%row == 0)
      return;
   
   Texture_operation.removeRow(%row);

   // find the next row to select
   %rowCount = Texture_operation.rowCount()-1;
   if (%row > %rowCount)
      %row = %rowCount;

   if (%id == $selectedTextureOperation)
      $selectedTextureOperation = -1;

   %id = Texture_operation.getRowId(%row);
   Texture_operation.setSelectedById(%id);
   Texture::save();
}   


//--------------------------------------
function Texture::applyMaterials()
{
   Texture::saveMaterial();
   %count = Texture_material.rowCount();
   if (%count > 0)   
   {
      %data = getRecord(Texture_material.getRowText(0),0);
      %mat_list = getField( %data, 0);
      %reg_list = getField( %data, 1);
      Texture::evalMaterial(Texture_material.getRowId(0));
      
      for (%i=1; %i<%count; %i++)
      {
         Texture::evalMaterial(Texture_material.getRowId(%i));
         %data = getRecord(Texture_material.getRowText(%i),0);
         %mat_list = %mat_list @ " " @ getField( %data, 0);
         %reg_list = %reg_list @ " " @ getField( %data, 1);
      }
      terraformer.setMaterials(%reg_list, %mat_list);
   }
}   


//--------------------------------------
function Texture::previewMaterial(%id)
{
   if (%id $= "")
      %id = $selectedMaterial;
   if (%id == -1)
      return;

   %data = Texture_material.getRowTextById(%id);
   %row  = Texture_material.getRowNumById(%id);
   %reg  = getField(getRecord(%data,0),1);

   Texture::evalMaterial(%id);

   terraformer.preview(TexturePreview, %reg);
}   


//--------------------------------------
function Texture::evalMaterial(%id)
{
   if (%id $= "")
      %id = $selectedMaterial;
   if (%id == -1)   
      return;

   %data = Texture_material.getRowTextbyId(%id);
   %reg  = getField(getRecord(%data,0), 1);

   // make sure all operation on this material are up to date
   // and accumulate register data for each
   %opCount = getRecordCount(%data);
   if (%opCount >= 2)    // record0=material record1=fractal
   {
      %entry = getRecord(%data, 1);
      Texture::evalOperationData(%entry, 1);
      for (%op=2; %op<%opCount; %op++)
      {
         %entry = getRecord(%data, %op);
         %reg_list = %reg_list @ getField(%entry, 2) @ " ";
         Texture::evalOperationData(%entry, %op);
      }
      // merge the masks in to the dst reg
      terraformer.mergeMasks(%reg_list, %reg);
   }
   Texture::save();
}   


//--------------------------------------
function Texture::evalOperation(%id)
{
   if (%id $= "")
      %id = $selectedTextureOperation;
   if (%id == -1)   
      return;
   
   %data   = Texture_operation.getRowTextById(%id);
   %row    = Texture_operation.getRowNumById(%id);
   
   if (%row != 0)    
      Texture::evalOperation( Texture_operation.getRowId(0) );

   Texture::evalOperationData(%data, %row);
   Texture::save();
}   


//--------------------------------------
function Texture::evalOperationData(%data, %row)
{
   %label  = getField(%data, 0);
   %reg    = getField(%data, 2);
   %dreg   = getField(%data, 3);
   %id     = Texture_material.getRowId(%row);

   if ( $dirtyTexture[%reg] == false )
   {
      return;
   }

   switch$ (%label)
   {
      case "Fractal Distortion":
         terraformer.maskFBm( %reg, getField(%data,5), getField(%data,7), getField(%data,9), getField(%data,11), false, 0 );

      case "Place by Fractal":
         terraformer.maskFBm( %reg, getField(%data,5), getField(%data,7), getField(%data,9), getField(%data,11), getField(%data,13), %dreg );

      case "Place by Height":
         terraformer.maskHeight( $HeightfieldSrcRegister, %reg, getField(%data,5), getField(%data,7), %dreg );

      case "Place by Slope":
         terraformer.maskSlope( $HeightfieldSrcRegister, %reg, getField(%data,5), getField(%data,7), %dreg );

      case "Place by Water Level":
         terraformer.maskWater( $HeightfieldSrcRegister, %reg, getField(%data,5), %dreg );
   }


   $dirtyTexture[%reg] = false;
}   



//--------------------------------------
function Texture::previewOperation(%id)
{
   if (%id $= "")
      %id = $selectedTextureOperation;
   if (%id == -1)
      return;

   %row  = Texture_operation.getRowNumById(%id);
   %data = Texture_operation.getRowText(%row);
   %reg  = getField(%data,2);

   Texture::evalOperation(%id);
   terraformer.preview(TexturePreview, %reg);
}   



//--------------------------------------
function Texture::restoreMaterial(%id)
{
   if (%id == -1)
      return;

   %data = Texture_material.getRowTextById(%id);
   
   Texture_operation.clear();
   %recordCount = getRecordCount(%data);
   for (%record=1; %record<%recordCount; %record++)
   {
      %entry = getRecord(%data, %record);
      Texture_operation.addRow($nextTextureId++, %entry);
   }
}   


//--------------------------------------
function Texture::saveMaterial()
{
   %id = $selectedMaterial;
   if (%id == -1)
      return;

   Texture::SaveOperation();
   %data = Texture_Material.getRowTextById(%id);
   %newData = getRecord(%data,0);

   %rowCount = Texture_Operation.rowCount();
   for (%row=0; %row<%rowCount; %row++)
      %newdata = %newdata @ "\n" @ Texture_Operation.getRowText(%row);

   Texture_Material.setRowById(%id, %newdata);
   Texture::save();
}   


//--------------------------------------
function Texture::restoreOperation(%id)
{
   if (%id == -1)
      return;

   %data = Texture_operation.getRowTextById(%id);

   %fieldCount = getFieldCount(%data);
   for (%field=4; %field<%fieldCount; %field += 2)
   {
      %obj = getField(%data, %field);
      %obj.setValue( getField(%data, %field+1) );
   }
   Texture::save();
}   


//--------------------------------------
function Texture::saveOperation()
{
   %id = $selectedTextureOperation;
   if (%id == -1)
      return;

   %data = Texture_operation.getRowTextById(%id);
   %newData = getField(%data,0) @ "\t" @ getField(%data,1) @ "\t" @ getField(%data,2) @ "\t" @ getField(%data,3); 

   // go through each object and update its value
   %fieldCount = getFieldCount(%data);
   for (%field=4; %field<%fieldCount; %field += 2)
   {
      %obj = getField(%data, %field);
      %newdata = %newdata @ "\t" @ %obj @ "\t" @ %obj.getValue();
   }

   %dirty = (%data !$= %newdata);
   %reg   = getField(%data, 2);
   $dirtyTexture[%reg] = %dirty;
      
   Texture_operation.setRowById(%id, %newdata);

   // mark the material register as dirty too
   if (%dirty == true)
   {
      %data = Texture_Material.getRowTextById($selectedMaterial);
      %reg  = getField(getRecord(%data,0), 1);
      $dirtyTexture[ %reg ] = true;
   }

   // if row is zero the fractal mask was modified
   // mark everything else in the list as dirty
    %row = Texture_material.getRowNumById(%id);
    if (%row == 0)
    {
       %rowCount = Texture_operation.rowCount();
       for (%r=1; %r<%rowCount; %r++)
       {
          %data = Texture_operation.getRowText(%r);
          $dirtyTexture[ getField(%data,2) ] = true;
       }
   }
   Texture::save();
}   


//--------------------------------------
function Texture::addMaterial(%entry)
{
   %id = $nextTextureId++;
   Texture_material.addRow(%id, %entry);
   
   %reg = getField(%entry, 1);
   $dirtyTexture[%reg] = true;

   Texture::save();
   return %id;
}   

//--------------------------------------
function Texture::addOperation(%entry)
{
   // Assumes: operation is being added to selected material

   %id = $nextTextureId++;
   Texture_operation.addRow(%id, %entry);

   %reg = getField(%entry, 2);
   $dirtyTexture[%reg] = true;

   Texture::save();
   return %id;
}   


//--------------------------------------
function Texture::save()
{
   %script = "";

   // loop through each operation and save it to disk
   %rowCount = Texture_material.rowCount();
   for(%row = 0; %row < %rowCount; %row++)
   {
      if(%row != 0)
         %script = %script @ "\n";
      %data = expandEscape(Texture_material.getRowText(%row));
      %script = %script @ %data;
   }
   terrain.setTextureScript(%script);
   ETerrainEditor.isDirty = true;
}

//--------------------------------------
function Texture::import()
{
   getLoadFilename("*.ter", "Texture::doLoadTexture");
}   

function Texture::loadFromScript(%script)
{
   Texture_material.clear();
   Texture_operation.clear();
   $selectedMaterial = -1;
   $selectedTextureOperation = -1;

   %i = 0;
   for(%rec = getRecord(%script, %i); %rec !$= ""; %rec = getRecord(%script, %i++))
      Texture::addMaterial(collapseEscape(%rec));
   // initialize dirty register array
   // patch up register usage
   // ...and deterime what the next register should be.
   $nextTextureRegister = 1000;
   %rowCount = Texture_material.rowCount();
   for (%row = 0; %row < %rowCount; %row++)
   {
      $dirtyTexture[ $nextTextureRegister ] = true;
      %data    = Texture_material.getRowText(%row);
      %rec     = getRecord(%data, 0);
      %rec     = setField(%rec, 1, $nextTextureRegister);
      %data    = setRecord(%data, 0, %rec);
      $nextTextureRegister++;
      
      %opCount = getRecordCount(%data);
      for (%op = 1; %op < %opCount; %op++)
      {
         if (%op == 1)
            %frac_reg = $nextTextureRegister;
         $dirtyTexture[ $nextTextureRegister ] = true;
         %rec  = getRecord(%data,%op);
         %rec  = setField(%rec, 2, $nextTextureRegister);
         %rec  = setField(%rec, 3, %frac_reg);
         %data = setRecord(%data, %op, %rec);
         $nextTextureRegister++;
      }
      %id = Texture_material.getRowId(%row);
      Texture_material.setRowById(%id, %data);
   }

   $selectedMaterial = -1;
   Texture_material.setSelectedById(Texture_material.getRowId(0));
}

//--------------------------------------
function Texture::doLoadTexture(%name)
{
   // ok, we're getting a terrain file...
   %newTerr = new TerrainBlock() // unnamed - since we'll be deleting it shortly:
   {
      position = "0 0 0";
      terrainFile = %name;
      squareSize = 8;
      visibleDistance = 100;
   };
   if(isObject(%newTerr))
   {
      %script = %newTerr.getTextureScript();
      if(%script !$= "")
         Texture::loadFromScript(%script);
      %newTerr.delete();
   }   
}   



//--------------------------------------
function Texture::hideTab()
{
   tab_DistortMask.setVisible(false);   
   tab_FractalMask.setVisible(false);   
   tab_HeightMask.setVisible(false);   
   tab_SlopeMask.setVisible(false);   
   tab_waterMask.setVisible(false);   
}   


//--------------------------------------
function Texture::showTab(%id)
{
   Texture::hideTab();
   %data = Texture_operation.getRowTextById(%id);
   %tab  = getField(%data,1);
   %tab.setVisible(true);
}   



$TerraformerHeightfieldDir = "common/editor/heightScripts";

function tab_Blend::reset(%this)
{
   blend_option.clear();
   blend_option.add("Add", 0);
   blend_option.add("Subtract", 1);
   blend_option.add("Max", 2);
   blend_option.add("Min", 3);
   blend_option.add("Multiply", 4);
}

function tab_fBm::reset(%this)
{
   fBm_detail.clear();
   fBm_detail.add("Very Low", 0);
   fBm_detail.add("Low", 1);
   fBm_detail.add("Normal", 2);
   fBm_detail.add("High", 3);
   fBm_detail.add("Very High", 4);
}

function tab_RMF::reset(%this)
{
   rmf_detail.clear();
   rmf_detail.add("Very Low", 0);
   rmf_detail.add("Low", 1);
   rmf_detail.add("Normal", 2);
   rmf_detail.add("High", 3);
   rmf_detail.add("Very High", 4);
}

function tab_terrainFile::reset(%this)
{
   // update tab controls..
   terrainFile_textList.clear();

   %filespec = $TerraformerHeightfieldDir @ "/*.ter";

   for(%file = findFirstFile(%filespec); %file !$= ""; %file = findNextFile(%filespec))
   {
      terrainFile_textList.addRow(%i++, fileBase(%file) @ fileExt(%file));
   }
}

function tab_canyon::reset()
{
}

function tab_smooth::reset()
{
}

function tab_smoothWater::reset()
{
}

function tab_smoothRidge::reset()
{
}

function tab_filter::reset()
{
}

function tab_turbulence::reset()
{
}

function tab_thermal::reset()
{
}

function tab_hydraulic::reset()
{
}

function tab_general::reset()
{
}

function tab_bitmap::reset()
{
}

function tab_sinus::reset()
{
}


//--------------------------------------

function Heightfield::resetTabs()
{
   tab_terrainFile.reset();
   tab_fbm.reset();
   tab_rmf.reset();
   tab_canyon.reset();
   tab_smooth.reset();
   tab_smoothWater.reset();
   tab_smoothRidge.reset();
   tab_filter.reset();
   tab_turbulence.reset();
   tab_thermal.reset();
   tab_hydraulic.reset();
   tab_general.reset();
   tab_bitmap.reset();
   tab_blend.reset();
   tab_sinus.reset();
}

//--------------------------------------
function TerraformerInit()
{
   Heightfield_options.clear();   
   Heightfield_options.setText("Operation");   
   Heightfield_options.add("fBm Fractal",0);
   Heightfield_options.add("Rigid MultiFractal",1);
   Heightfield_options.add("Canyon Fractal",2);
   Heightfield_options.add("Sinus",3);
   Heightfield_options.add("Bitmap",4);
   Heightfield_options.add("Turbulence",5);
   Heightfield_options.add("Smoothing",6);
   Heightfield_options.add("Smooth Water",7);
   Heightfield_options.add("Smooth Ridges/Valleys", 8);
   Heightfield_options.add("Filter",9);
   Heightfield_options.add("Thermal Erosion",10);
   Heightfield_options.add("Hydraulic Erosion",11);
   Heightfield_options.add("Blend",12);
   Heightfield_options.add("Terrain File",13);

   Heightfield::resetTabs();

   %script = Terrain.getHeightfieldScript();
   if(%script !$= "")
      Heightfield::loadFromScript(%script,true);

   if (Heightfield_operation.rowCount() == 0)
   {
      Heightfield_operation.clear();
      %id1 = Heightfield::add("General\tTab_general\tgeneral_min_height\t50\tgeneral_scale\t300\tgeneral_water\t0.000\tgeneral_centerx\t0\tgeneral_centery\t0");
      Heightfield_operation.setSelectedById(%id1);
   }

   Heightfield::resetTabs();
   Heightfield::preview();
}   

//--------------------------------------
function Heightfield_options::onSelect(%this, %_id, %text)
{
   Heightfield_options.setText("Operation");
   %id = -1;

   %rowCount = Heightfield_operation.rowCount();

   // FORMAT
   //  item name
   //  tab name
   //    control name 
   //    control value
   switch$(%text)
   {
      case "Terrain File":
         %id = HeightField::add("Terrain File\ttab_terrainFile\tterrainFile_terrFileText\tterrains/terr1.ter\tterrainFile_textList\tterr1.ter");

      case "fBm Fractal":
         %id = Heightfield::add("fBm Fractal\ttab_fBm\tfbm_interval\t9\tfbm_rough\t0.000\tfBm_detail\tNormal\tfBm_seed\t" @ terraformer.generateSeed());

      case "Rigid MultiFractal":
         %id = Heightfield::add("Rigid MultiFractal\ttab_RMF\trmf_interval\t4\trmf_rough\t0.000\trmf_detail\tNormal\trmf_seed\t" @ terraformer.generateSeed());

      case "Canyon Fractal":
         %id = Heightfield::add("Canyon Fractal\ttab_Canyon\tcanyon_freq\t5\tcanyon_factor\t0.500\tcanyon_seed\t" @ terraformer.generateSeed());
   
      case "Sinus":
         %id = Heightfield::add("Sinus\ttab_Sinus\tsinus_filter\t1 0.83333 0.6666 0.5 0.33333 0.16666 0\tsinus_seed\t" @ terraformer.generateSeed());
   
      case "Bitmap":
         %id = Heightfield::add("Bitmap\ttab_Bitmap\tbitmap_name\t");
         Heightfield::setBitmap();
   }


   if (Heightfield_operation.rowCount() >= 1)
   {
      switch$(%text)
      {
         case "Smoothing":
            %id = Heightfield::add("Smoothing\ttab_Smooth\tsmooth_factor\t0.500\tsmooth_iter\t0");
   
         case "Smooth Water":
            %id = Heightfield::add("Smooth Water\ttab_SmoothWater\twatersmooth_factor\t0.500\twatersmooth_iter\t0");

         case "Smooth Ridges/Valleys":
            %id = Heightfield::add("Smooth Ridges/Valleys\ttab_SmoothRidge\tridgesmooth_factor\t0.8500\tridgesmooth_iter\t1");
   
         case "Filter":
            %id = Heightfield::add("Filter\ttab_Filter\tfilter\t0 0.16666667 0.3333333 0.5 0.6666667 0.8333333 1");
   
         case "Turbulence":
            %id = Heightfield::add("Turbulence\ttab_Turbulence\tturbulence_factor\t0.250\tturbulence_radius\t10");
   
         case "Thermal Erosion":
            %id = Heightfield::add("Thermal Erosion\ttab_Thermal\tthermal_slope\t30\tthermal_cons\t80.0\tthermal_iter\t0");
   
         case "Hydraulic Erosion":
            %id = Heightfield::add("Hydraulic Erosion\ttab_Hydraulic\thydraulic_iter\t0\thydraulic_filter\t0 0.16666667 0.3333333 0.5 0.6666667 0.8333333 1");
      }
   }

   if (Heightfield_operation.rowCount() >= 2)
   {
      if("Blend" $= %text)
         %id = Heightfield::add("Blend\ttab_Blend\tblend_factor\t0.500\tblend_srcB\t" @ %rowCount-2 @"\tblend_option\tadd");
   }

   
   // select it
   if (%id != -1)
      Heightfield_operation.setSelectedById(%id);
}   


//--------------------------------------
function Heightfield::eval(%id)
{
   if (%id == -1)   
      return;
   
   %data  = restWords(Heightfield_operation.getRowTextById(%id));
   %label = getField(%data,0);
   %row   = Heightfield_operation.getRowNumById(%id);
    
   echo("Heightfield::eval:" @ %row @ "  " @ %label );

   switch$(%label)
   {
      case "General":
         if (Terrain.squareSize>0) %size = Terrain.squareSize;
         else %size = 8;
         terraformer.setTerrainInfo( 256, %size, getField(%data,3), getField(%data,5), getField(%data,7) );
         terraformer.setShift( getField(%data,9), getField(%data,11) );
         terraformer.terrainData(%row);

      case "Terrain File":
         terraformer.terrainFile(%row, getField(%data,3));

      case "fBm Fractal":
         terraformer.fBm( %row, getField(%data,3), getField(%data,5), getField(%data,7), getField(%data,9) );

      case "Sinus":
         terraformer.sinus( %row, getField(%data,3), getField(%data,5) );
    
      case "Rigid MultiFractal":
         terraformer.rigidMultiFractal( %row, getField(%data,3), getField(%data,5), getField(%data,7), getField(%data,9) );
    
      case "Canyon Fractal":
         terraformer.canyon( %row, getField(%data,3), getField(%data,5), getField(%data,7) );
   
      case "Smoothing":
         terraformer.smooth( %row-1, %row, getField(%data,3), getField(%data,5) );
       
      case "Smooth Water":
         terraformer.smoothWater( %row-1, %row, getField(%data,3), getField(%data,5) );
       
      case "Smooth Ridges/Valleys":
         terraformer.smoothRidges( %row-1, %row, getField(%data,3), getField(%data,5) );

      case "Filter":
         terraformer.filter( %row-1, %row, getField(%data,3) );
       
      case "Turbulence":
         terraformer.turbulence( %row-1, %row, getField(%data,3), getField(%data,5) );
       
      case "Thermal Erosion":
         terraformer.erodeThermal( %row-1, %row, getField(%data,3), getField(%data,5),getField(%data,7) );
       
      case "Hydraulic Erosion":
         terraformer.erodeHydraulic( %row-1, %row, getField(%data,3), getField(%data,5) );
       
      case "Bitmap":
         terraformer.loadGreyscale(%row, getField(%data,3));

      case "Blend":
         %rowCount = Heightfield_operation.rowCount();
         if(%rowCount > 2) 
         {
            %a = Heightfield_operation.getRowNumById(%id)-1;
            %b = getField(%data, 5);
            echo("Blend: " @ %data);
            echo("Blend: " @ getField(%data,3) @ "  " @ getField(%data,7));
            if(%a < %rowCount || %a > 0 || %b < %rowCount || %b > 0 ) 
               terraformer.blend(%a, %b, %row, getField(%data,3), getField(%data,7) );
            else
               echo("Heightfield Editor: Blend parameters out of range.");
         }
   }

}   

//--------------------------------------
function Heightfield::add(%entry)
{
   Heightfield::saveTab();
   Heightfield::hideTab();

   %id = $NextOperationId++;
   if ($selectedOperation != -1)
   {
      %row = Heightfield_operation.getRowNumById($selectedOperation) + 1;
      %entry = %row @ " " @ %entry;
      Heightfield_operation.addRow(%id, %entry, %row); // insert

      // adjust row numbers
      for(%i = %row+1; %i < Heightfield_operation.rowCount(); %i++)
      {
         %id = Heightfield_operation.getRowId(%i);
         %text = Heightfield_operation.getRowTextById(%id);
         %text = setWord(%text, 0, %i);
         Heightfield_operation.setRowById(%id, %text);
      }
   }
   else
   {
      %entry = Heightfield_operation.rowCount() @ " " @ %entry;
      Heightfield_operation.addRow(%id, %entry);   // add to end
   }

   %row = Heightfield_operation.getRowNumById(%id);
   if (%row <= $HeightfieldDirtyRow)
      $HeightfieldDirtyRow = %row;
   Heightfield::save();
   return %id;
}   


//--------------------------------------
function Heightfield::onDelete(%id)
{
   if (%id $= "")
      %id = $selectedOperation;

   %row = Heightfield_operation.getRowNumById(%id);
   
   // don't delete the first entry
   if (%row == 0)
      return;

   Heightfield_operation.removeRow(%row);
   
   // adjust row numbers
   for(%i = %row; %i < Heightfield_operation.rowCount(); %i++)
   {
      %id2 = Heightfield_operation.getRowId(%i);
      %text = Heightfield_operation.getRowTextById(%id2);
      %text = setWord(%text, 0, %i);
      Heightfield_operation.setRowById(%id2, %text);
   }

   // adjust the Dirty Row position
   if ($HeightfieldDirtyRow >= %row)
      $HeightfieldDirtyRow = %row;   
   
   // find the next row to select
   %rowCount = Heightfield_operation.rowCount()-1;
   if (%row > %rowCount)
      %row = %rowCount;

   if (%id == $selectedOperation)
      $selectedOperation = -1;

   %id = Heightfield_operation.getRowId(%row);
   Heightfield_operation.setSelectedById(%id);
   Heightfield::save();
}   


//--------------------------------------
function Heightfield_operation::onSelect(%this, %id, %text)
{
   Heightfield::saveTab();
   Heightfield::hideTab();

   $selectedOperation = %id; 
   Heightfield::restoreTab($selectedOperation);
   Heightfield::showTab($selectedOperation);
   Heightfield::preview($selectedOperation);
}   


//--------------------------------------
function Heightfield::restoreTab(%id)
{
   if (%id == -1)   
      return;

   Heightfield::hideTab();
   
   %data = restWords(Heightfield_operation.getRowTextById(%id));

   %fieldCount = getFieldCount(%data);
   for (%field=2; %field<%fieldCount; %field += 2)
   {
      %obj = getField(%data, %field);
      %obj.setValue( getField(%data, %field+1) );
   }
   Heightfield::save();
}  

 
//--------------------------------------
function Heightfield::saveTab()
{
   if ($selectedOperation == -1)   
      return;
   
   %data = Heightfield_operation.getRowTextById($selectedOperation);

   %rowNum = getWord(%data, 0);
   %data = restWords(%data);   
   %newdata = getField(%data,0) @ "\t" @ getField(%data,1);

   %fieldCount = getFieldCount(%data);
   for (%field=2; %field < %fieldCount; %field += 2)
   {
      %obj = getField(%data, %field);
      %newdata = %newdata @ "\t" @ %obj @ "\t" @ %obj.getValue();
   }
   // keep track of the top-most dirty operation
   // so we know who to evaluate later
   if (%data !$= %newdata)
   {
      %row = Heightfield_operation.getRowNumById($selectedOperation);
      if (%row <= $HeightfieldDirtyRow && %row > 0)
         $HeightfieldDirtyRow = %row;
   }

   Heightfield_operation.setRowById($selectedOperation, %rowNum @ " " @ %newdata);
   Heightfield::save();
}  


//--------------------------------------
function Heightfield::preview(%id)
{
   %rowCount = Heightfield_operation.rowCount();
   if (%id $= "")
      %id = Heightfield_operation.getRowId(%rowCount-1);

   %row = Heightfield_operation.getRowNumById(%id);

   Heightfield::refresh(%row);
   terraformer.previewScaled(HeightfieldPreview, %row);
}   


//--------------------------------------
function Heightfield::refresh(%last)
{
   if (%last $= "")   
      %last = Heightfield_operation.rowCount()-1;

   // always update the general info
   Heightfield::eval(Heightfield_operation.getRowId(0));

   for( 0; $HeightfieldDirtyRow<=%last; $HeightfieldDirtyRow++)
   {
      %id = Heightfield_operation.getRowId($HeightfieldDirtyRow);
      Heightfield::eval(%id);
   }
   Heightfield::save();
}   


//--------------------------------------
function Heightfield::apply(%id)
{
   %rowCount = Heightfield_operation.rowCount();
   if (%rowCount < 1)
      return;
   if (%id $= "")
      %id = Heightfield_operation.getRowId(%rowCount-1);

   %row = Heightfield_operation.getRowNumById(%id);

   HeightfieldPreview.setRoot();
   Heightfield::refresh(%row);
   terraformer.setTerrain(%row);

   terraformer.setCameraPosition(0,0,0); 
   ETerrainEditor.isDirty = true;
}   

//--------------------------------------
$TerraformerSaveRegister = 0;
function Heightfield::saveBitmap(%name)
{
   if(%name $= "")
      getSaveFilename("*.png", "Heightfield::doSaveBitmap",
         $TerraformerHeightfieldDir @ "/" @ fileBase($Client::MissionFile) @ ".png");
   else
      Heightfield::doSaveBitmap(%name);
}

function Heightfield::doSaveBitmap(%name)
{        
   terraformer.saveGreyscale($TerraformerSaveRegister, %name);
}

//--------------------------------------

function Heightfield::save()
{
   %script = "";
   %rowCount = Heightfield_operation.rowCount();
   for(%row = 0; %row < %rowCount; %row++)
   {
      if(%row != 0)
         %script = %script @ "\n";
      %data = restWords(Heightfield_operation.getRowText(%row));
      %script = %script @ expandEscape(%data);
   }
   terrain.setHeightfieldScript(%script);
   ETerrainEditor.isDirty = true;
}

//--------------------------------------
function Heightfield::import()
{
   getLoadFilename("*.ter", "Heightfield::doLoadHeightfield");
}   


//--------------------------------------
function Heightfield::loadFromScript(%script,%leaveCamera)
{
   echo(%script);

   Heightfield_operation.clear();
   $selectedOperation = -1;
   $HeightfieldDirtyRow = -1;

   // zero out all shifting
   HeightfieldPreview.reset();

   for(%rec = getRecord(%script, %i); %rec !$= ""; %rec = getRecord(%script, %i++))
      Heightfield::add(collapseEscape(%rec));

   if (Heightfield_operation.rowCount() == 0)
   {
      // if there was a problem executing the script restore
      // the operations list to a known state
      Heightfield_operation.clear();
      Heightfield::add("General\tTab_general\tgeneral_min_height\t50\tgeneral_scale\t300\tgeneral_water\t0.000\tgeneral_centerx\t0\tgeneral_centery\t0");
   }
   %data = restWords(Heightfield_operation.getRowText(0));
   %x = getField(%data,7); 
   %y = getField(%data,9); 
   HeightfieldPreview.setOrigin(%x, %y);
   Heightfield_operation.setSelectedById(Heightfield_operation.getRowId(0));

   // Move the control object to the specified position
   if (!%leaveCamera)
      terraformer.setCameraPosition(%x,%y); 
}   

//--------------------------------------
function strip(%stripStr, %strToStrip)
{
   %len = strlen(%stripStr);
   if(strcmp(getSubStr(%strToStrip, 0, %len), %stripStr) == 0)
      return getSubStr(%strToStrip, %len, 100000);
   return %strToStrip;
}

function Heightfield::doLoadHeightfield(%name)
{
   // ok, we're getting a terrain file...

   %newTerr = new TerrainBlock() // unnamed - since we'll be deleting it shortly:
   {
      position = "0 0 -1000";
      terrainFile = strip("terrains/", %name);
      squareSize = 8;
      visibleDistance = 100;
   };
   if(isObject(%newTerr))
   {
      %script = %newTerr.getHeightfieldScript();
      if(%script !$= "")
         Heightfield::loadFromScript(%script);
      %newTerr.delete();
   }   
}   


//--------------------------------------
function Heightfield::setBitmap()
{
   getLoadFilename("*.png", "Heightfield::doSetBitmap");
}   

//--------------------------------------
function Heightfield::doSetBitmap(%name)
{
   bitmap_name.setValue(%name);
   Heightfield::saveTab();
   Heightfield::preview($selectedOperation);
}   


//--------------------------------------
function Heightfield::hideTab()
{
   tab_terrainFile.setVisible(false);
   tab_fbm.setvisible(false);
   tab_rmf.setvisible(false);
   tab_canyon.setvisible(false);
   tab_smooth.setvisible(false);
   tab_smoothWater.setvisible(false);
   tab_smoothRidge.setvisible(false);
   tab_filter.setvisible(false);
   tab_turbulence.setvisible(false);
   tab_thermal.setvisible(false);
   tab_hydraulic.setvisible(false);
   tab_general.setvisible(false);
   tab_bitmap.setvisible(false);
   tab_blend.setvisible(false);
   tab_sinus.setvisible(false);
}


//--------------------------------------
function Heightfield::showTab(%id)
{
   Heightfield::hideTab();
   %data = restWords(Heightfield_operation.getRowTextById(%id));
   %tab  = getField(%data,1);
   echo("Tab data: " @ %data @ " tab: " @ %tab);
   %tab.setVisible(true);
}


//--------------------------------------
function Heightfield::center()
{
   %camera = terraformer.getCameraPosition(); 
   %x = getWord(%camera, 0);
   %y = getWord(%camera, 1);

   HeightfieldPreview.setOrigin(%x, %y);

   %origin = HeightfieldPreview.getOrigin();
   %x = getWord(%origin, 0);
   %y = getWord(%origin, 1);

   %root = HeightfieldPreview.getRoot();
   %x += getWord(%root, 0);
   %y += getWord(%root, 1);

   general_centerx.setValue(%x);
   general_centery.setValue(%y);
   Heightfield::saveTab();
}   

function ExportHeightfield::onAction()
{
   error("Time to export the heightfield...");
   if (Heightfield_operation.getSelectedId() != -1) {
      $TerraformerSaveRegister = getWord(Heightfield_operation.getValue(), 0);
      Heightfield::saveBitmap("");
   }
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
function onNeedRelight()
{
   if( RelightMessage.visible == false )
      RelightMessage.visible = true;
}

function TerrainEditor::onGuiUpdate(%this, %text)
{
   %mouseBrushInfo = " (Mouse Brush) #: " @ getWord(%text, 0) @ "  avg: " @ getWord(%text, 1);
   %selectionInfo = " (Selection) #: " @ getWord(%text, 2) @ "  avg: " @ getWord(%text, 3);
   
   TEMouseBrushInfo.setValue(%mouseBrushInfo);
   TEMouseBrushInfo1.setValue(%mouseBrushInfo);
   TESelectionInfo.setValue(%selectionInfo);
   TESelectionInfo1.setValue(%selectionInfo);
}

function TerrainEditor::offsetBrush(%this, %x, %y)
{
   %curPos = %this.getBrushPos();
   %this.setBrushPos(getWord(%curPos, 0) + %x, getWord(%curPos, 1) + %y);
}

function TerrainEditor::swapInLoneMaterial(%this, %name)
{
   // swapped?
   if(%this.baseMaterialsSwapped $= "true")
   {
      %this.baseMaterialsSwapped = "false";
      tEditor.popBaseMaterialInfo();
   }
   else
   {
      %this.baseMaterialsSwapped = "true";
      %this.pushBaseMaterialInfo();
      %this.setLoneBaseMaterial(%name);
   }
      
   //
   flushTextureCache();
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------


function TELoadTerrainButton::onAction(%this)
{
   getLoadFilename("terrains/*.ter", %this @ ".gotFileName");
}

function TELoadTerrainButton::gotFileName(%this, %name)
{
   //
   %pos = "0 0 0";
   %squareSize = "8";
   %visibleDistance = "1200";

   // delete current
   if(isObject(terrain))
   {
      %pos = terrain.position;
      %squareSize = terrain.squareSize;
      %visibleDistance = terrain.visibleDistance;

      terrain.delete();
   }

   // create new
   new TerrainBlock(terrain)
   {
      position = %pos;
      terrainFile = %name;
      squareSize = %squareSize;
      visibleDistance = %visibleDistance;
   };

   ETerrainEditor.attachTerrain();
}

function TerrainEditorSettingsGui::onWake(%this)
{
   TESoftSelectFilter.setValue(ETerrainEditor.softSelectFilter);
}

function TerrainEditorSettingsGui::onSleep(%this)
{
   ETerrainEditor.softSelectFilter = TESoftSelectFilter.getValue();
}

function TESettingsApplyButton::onAction(%this)
{
   ETerrainEditor.softSelectFilter = TESoftSelectFilter.getValue();
   ETerrainEditor.resetSelWeights(true);
   ETerrainEditor.processAction("softSelect");
}

function getPrefSetting(%pref, %default)
{
   // 
   if(%pref $= "")
      return(%default);
   else
      return(%pref);
}

//------------------------------------------------------------------------------

function Editor::open(%this)
{
   // prevent the mission editor from opening while the GuiEditor is open.
   if(Canvas.getContent() == GuiEditorGui.getId())
      return;
      
   $Canvas::forceMouse = true;

   Canvas.setContent(EditorGui);
}

function Editor::close(%this, %gui)
{
   if (%gui $= "")
      %gui = RootGui;
   Canvas.setContent(%gui);
   MessageHud.close();
   
   $Canvas::forceMouse = false;
}

//------------------------------------------------------------------------------
