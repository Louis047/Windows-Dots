Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")
Set wmi = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\cimv2")

Do
    Set p = wmi.ExecQuery("SELECT * FROM Win32_Process WHERE Name='explorer.exe'")
    If p.Count > 0 Then Exit Do
    WScript.Sleep 1000
Loop

Dim ready : ready = False
Dim tries : tries = 0
Do While Not ready And tries < 30
    ready = WshShell.AppActivate("Program Manager")
    If Not ready Then WScript.Sleep 1000
    tries = tries + 1
Loop

WScript.Sleep 2000

Set running = wmi.ExecQuery("SELECT * FROM Win32_Process WHERE Name='glazewm.exe'")
If running.Count > 0 Then WScript.Quit

Dim exePath : exePath = ""

Dim scoopBin : scoopBin = WshShell.ExpandEnvironmentStrings("%USERPROFILE%") & "\scoop\apps\glazewm\current\glazewm.exe"
If FSO.FileExists(scoopBin) Then
    exePath = scoopBin
Else
    On Error Resume Next
    Dim hklm : hklm = WshShell.RegRead("HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path")
    Dim hkcu : hkcu = WshShell.RegRead("HKCU\Environment\Path")
    On Error GoTo 0

    Dim pth
    For Each pth In Split(hklm & ";" & hkcu, ";")
        pth = Trim(pth)
        If pth <> "" Then
            Dim candidate : candidate = pth & "\glazewm.exe"
            If FSO.FileExists(candidate) Then
                exePath = candidate
                Exit For
            End If
        End If
    Next
End If

If exePath <> "" Then
    WshShell.CurrentDirectory = FSO.GetParentFolderName(exePath)
    WshShell.Run """" & exePath & """", 0, False
End If