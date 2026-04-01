Set WshShell = CreateObject("WScript.Shell")

On Error Resume Next
WshShell.RegDelete "HKCU\Software\SmoothScroll\kssInstallDate"
On Error GoTo 0