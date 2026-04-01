@echo off
net session >nul 2>&1 || (echo Run as admin & pause & exit)

reg delete "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts" /v "Segoe UI Emoji (TrueType)" /f
copy /Y "%~dp0seguiemj.ttf" "%WINDIR%\Fonts\seguiemj.ttf"
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts" /v "Segoe UI Emoji (TrueType)" /t REG_SZ /d "seguiemj.ttf" /f

taskkill /f /im explorer.exe
start explorer.exe
pause