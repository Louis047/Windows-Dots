Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")

' --- try PATH first (winget/custom installs) ---
paths = Split(WshShell.RegRead("HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path"), ";")

For Each p In paths
    If InStr(LCase(p), "\shims") = 0 Then
        If FSO.FileExists(p & "\glazewm.exe") Then
            WshShell.Run """" & p & "\glazewm.exe""", 0, False
            WScript.Quit
        End If
    End If
Next

' --- fallback to scoop real binary ---
exePath = WshShell.ExpandEnvironmentStrings("%USERPROFILE%") & "\scoop\apps\glazewm\current\glazewm.exe"

If FSO.FileExists(exePath) Then
    WshShell.CurrentDirectory = FSO.GetParentFolderName(exePath)
    WshShell.Run """" & exePath & """", 0, False
End If