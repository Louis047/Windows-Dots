@echo off

reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows\Personalization" /v NoLockScreen /t REG_DWORD /d 1 /f >nul

gpupdate /force >nul

taskkill /f /im LockApp.exe >nul 2>&1

echo Lock screen disabled and policy refreshed instantly.
pause