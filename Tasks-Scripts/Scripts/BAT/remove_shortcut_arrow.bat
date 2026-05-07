@echo off

reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Icons" /v 29 /t REG_SZ /d "%windir%\System32\shell32.dll,-50" /f

taskkill /f /im explorer.exe
start explorer.exe