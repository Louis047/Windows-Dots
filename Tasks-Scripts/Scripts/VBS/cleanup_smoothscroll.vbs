On Error Resume Next

Set WshShell = CreateObject("WScript.Shell")

If Err.Number = 0 Then
    WshShell.RegDelete "HKCU\Software\SmoothScroll\kssInstallDate"
    Set WshShell = Nothing
End If

On Error GoTo 0