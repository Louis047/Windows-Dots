reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Icons" /v 29 /f

taskkill /f /im explorer.exe
start explorer.exe