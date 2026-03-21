Set WshShell = CreateObject("WScript.Shell")
WshShell.CurrentDirectory = CreateObject("WScript.Shell").ExpandEnvironmentStrings("%USERPROFILE%") & "\scoop\apps\glazewm\current"
WshShell.Run """" & WshShell.CurrentDirectory & "\glazewm.exe""", 0, False