Set WshShell = CreateObject("WScript.Shell")
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

Set running = wmi.ExecQuery("SELECT * FROM Win32_Process WHERE Name='SmoothScroll.exe'")
If running.Count > 0 Then WScript.Quit

Dim lnk
lnk = WshShell.ExpandEnvironmentStrings("%APPDATA%") & "\Microsoft\Windows\Start Menu\Programs\SmoothScroll\SmoothScroll.lnk"

If CreateObject("Scripting.FileSystemObject").FileExists(lnk) Then
    WshShell.Run """" & lnk & """", 0, False
End If