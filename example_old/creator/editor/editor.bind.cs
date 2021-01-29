//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Mission Editor Manager
new ActionMap(EditorMap);

EditorMap.bindCmd(keyboard, "f2", "editor.setEditor(WorldEditor);", "");
EditorMap.bindCmd(keyboard, "f3", "editor.setEditor(TerrainEditor);", "");
EditorMap.bindCmd(keyboard, "f4", "editor.setEditor(Terraformer);", "");   
EditorMap.bindCmd(keyboard, "f5", "editor.setEditor(AIEditor);", "");   

EditorMap.bindCmd(keyboard, "alt s", "Canvas.pushDialog(EditorSaveMissionDlg);", "");
EditorMap.bindCmd(keyboard, "alt r", "lightScene(\"\", forceAlways);", "");
EditorMap.bindCmd(keyboard, "escape", "editor.close();", "");

// alt-#: set bookmark
for(%i = 0; %i < 9; %i++)
   EditorMap.bindCmd(keyboard, "alt " @ %i, "editor.setBookmark(" @ %i @ ");", "");

// ctrl-#: goto bookmark
for(%i = 0; %i < 9; %i++)
   EditorMap.bindCmd(keyboard, "ctrl " @ %i, "editor.gotoBookmark(" @ %i @ ");", "");


//------------------------------------------------------------------------------
// World Editor
new ActionMap(WorldEditorMap);
WorldEditorMap.bindCmd(keyboard, "space", "wEditor.nextMode();", "");

WorldEditorMap.bindCmd(keyboard, "delete", "wEditor.copySelection();wEditor.deleteSelection();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl c", "wEditor.copySelection();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl x", "wEditor.copySelection();wEditor.deleteSelection();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl v", "wEditor.pasteSelection();", "");

WorldEditorMap.bindCmd(keyboard, "ctrl z", "wEditor.undo();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl y", "wEditor.redo();", "");

WorldEditorMap.bindCmd(keyboard, "ctrl h", "wEditor.hideSelection(true);", "");
WorldEditorMap.bindCmd(keyboard, "alt h", "wEditor.hideSelection(false);", "");
WorldEditorMap.bindCmd(keyboard, "ctrl d", "wEditor.dropSelection();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl q", "wEditor.dropCameraToSelection();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl m", "wEditor.moveSelectionInPlace();", "");
WorldEditorMap.bindCmd(keyboard, "ctrl r", "wEditor.resetTransforms();", "");

WorldEditorMap.bindCmd(keyboard, "i", "Canvas.pushDialog(interiorDebugDialog);", "");
WorldEditorMap.bindCmd(keyboard, "o", "Canvas.pushDialog(WorldEditorSettingsDlg);", "");


//------------------------------------------------------------------------------
// Terrain Editor
new ActionMap(TerrainEditorMap);

TerrainEditorMap.bindCmd(keyboard, "ctrl z", "tEditor.undo();", "");
TerrainEditorMap.bindCmd(keyboard, "ctrl y", "tEditor.redo();", "");

TerrainEditorMap.bindCmd(keyboard, "left", "tEditor.offsetBrush(-1, 0);", "");
TerrainEditorMap.bindCmd(keyboard, "right", "tEditor.offsetBrush(1, 0);", "");
TerrainEditorMap.bindCmd(keyboard, "up", "tEditor.offsetBrush(0, 1);", "");
TerrainEditorMap.bindCmd(keyboard, "down", "tEditor.offsetBrush(0, -1);", "");

TerrainEditorMap.bindCmd(keyboard, "1", "TERaiseHeightActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "2", "TELowerHeightActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "3", "TESetHeightActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "4", "TESetEmptyActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "5", "TEClearEmptyActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "6", "TEFlattenHeightActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "7", "TESmoothHeightActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "8", "TESetMaterialActionRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "9", "TEAdjustHeightActionRadio.setValue(1);", "");

TerrainEditorMap.bindCmd(keyboard, "shift 1", "tEditor.processUsesBrush = true;TERaiseHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 2", "tEditor.processUsesBrush = true;TELowerHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 3", "tEditor.processUsesBrush = true;TESetHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 4", "tEditor.processUsesBrush = true;TESetEmptyActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 5", "tEditor.processUsesBrush = true;TEClearEmptyActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 6", "tEditor.processUsesBrush = true;TEFlattenHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 7", "tEditor.processUsesBrush = true;TESmoothHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 8", "tEditor.processUsesBrush = true;TESetMaterialActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");
TerrainEditorMap.bindCmd(keyboard, "shift 9", "tEditor.processUsesBrush = true;TEAdjustHeightActionRadio.setValue(1);tEditor.processUsesBrush = false;", "");

TerrainEditorMap.bindCmd(keyboard, "h", "TESelectModeRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "j", "TEPaintModeRadio.setValue(1);", "");
TerrainEditorMap.bindCmd(keyboard, "k", "TEAdjustModeRadio.setValue(1);", "");

TerrainEditorMap.bindCmd(keyboard, "i", "Canvas.pushDialog(interiorDebugDialog);", "");
TerrainEditorMap.bindCmd(keyboard, "o", "Canvas.pushDialog(TerrainEditorValuesSettingsGui, 99);", "");
TerrainEditorMap.bindCmd(keyboard, "m", "Canvas.pushDialog(TerrainEditorTextureSelectGui, 99);", "");

TerrainEditorMap.bindCmd(keyboard, "backspace", "tEditor.clearSelection();", "");


//------------------------------------------------------------------------------
// AI Editor
new ActionMap(AIEditorMap);

AIEditorMap.bindCmd(keyboard, "space", "aiEdit.nextMode();", "");

AIEditorMap.bindCmd(keyboard, "delete", "aiEdit.copySelection();aiEdit.deleteSelection();", "");
AIEditorMap.bindCmd(keyboard, "ctrl c", "aiEdit.copySelection();", "");
AIEditorMap.bindCmd(keyboard, "ctrl x", "aiEdit.copySelection();aiEdit.deleteSelection();", "");
AIEditorMap.bindCmd(keyboard, "ctrl v", "aiEdit.pasteSelection();", "");

AIEditorMap.bindCmd(keyboard, "ctrl h", "aiEdit.hideSelection(true);", "");
AIEditorMap.bindCmd(keyboard, "alt h", "aiEdit.hideSelection(false);", "");
AIEditorMap.bindCmd(keyboard, "ctrl d", "aiEdit.dropSelection();", "");
AIEditorMap.bindCmd(keyboard, "ctrl q", "aiEdit.dropCameraToSelection();", "");
AIEditorMap.bindCmd(keyboard, "ctrl m", "aiEdit.moveSelectionInPlace();", "");
AIEditorMap.bindCmd(keyboard, "ctrl r", "aiEdit.resetTransforms();", "");

AIEditorMap.bindCmd(keyboard, "i", "Canvas.pushDialog(interiorDebugDialog);", "");

