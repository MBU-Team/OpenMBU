exec("./guiProfiles.cs");
exec("./newChatHud.gui");
exec("./newChatHud.cs");
exec("./newMessageHud.gui");
exec("./newMessageHud.cs");
exec("./localMods.cs");

newChatHud_Init();

PlayGui.AddSubDialog(newChatHud);