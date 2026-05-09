Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")

' --- wait for explorer ---
Do
    Set p = GetObject("winmgmts:").ExecQuery("select * from Win32_Process where Name='explorer.exe'")
    If p.Count > 0 Then Exit Do
    WScript.Sleep 1000
Loop

' extra buffer
WScript.Sleep 3000

' --- launch YASB via CLI ---
WshShell.Run "yasbc start --silent", 0, False