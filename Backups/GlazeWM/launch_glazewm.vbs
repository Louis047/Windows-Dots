Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")

tmp = WshShell.ExpandEnvironmentStrings("%TEMP%") & "\glazewm_path.txt"

' run silently
WshShell.Run "cmd /c where glazewm > """ & tmp & """", 0, True

If FSO.FileExists(tmp) Then
    Set file = FSO.OpenTextFile(tmp, 1)
    If Not file.AtEndOfStream Then
        exePath = Trim(file.ReadLine)
        WshShell.CurrentDirectory = FSO.GetParentFolderName(exePath)
        WshShell.Run """" & exePath & """", 0, False
    End If
    file.Close
    FSO.DeleteFile tmp
End If