Set WshShell = CreateObject("WScript.Shell")

lnk = WshShell.ExpandEnvironmentStrings("%APPDATA%") & "\Microsoft\Windows\Start Menu\Programs\SmoothScroll\SmoothScroll.lnk"

WshShell.Run """" & lnk & """", 0, False