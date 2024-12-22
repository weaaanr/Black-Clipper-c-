@echo off
title BlackClipper Uninstaller
color 0c

:: Запрос прав администратора
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo [!] Requesting administrative privileges...
    powershell -Command "Start-Process '%~s0' -Verb RunAs"
    exit /b
)

echo [*] BlackClipper Uninstaller started...
echo.

:: Остановка процессов
echo [*] Stopping processes...
taskkill /F /IM "WinSystemHelper*.exe" /T 2>nul
taskkill /F /IM "WindowsSecurityService*.exe" /T 2>nul
taskkill /F /IM "Microsoft*Helper.exe" /T 2>nul
echo.

:: Удаление задач планировщика
echo [*] Removing scheduled tasks...
for /f "tokens=*" %%i in ('schtasks /query /fo list ^| findstr /i "Windows.*Update"') do (
    schtasks /delete /tn "%%i" /f 2>nul
)
schtasks /delete /tn "Windows Security Service" /f 2>nul
echo.

:: Удаление файлов
echo [*] Removing files...
:: Удаление из AppData
for /d %%i in ("%LOCALAPPDATA%\Microsoft*Helper") do (
    rd /s /q "%%i" 2>nul
)
rd /s /q "%LOCALAPPDATA%\WindowsSecurityService" 2>nul

:: Удаление из System32
del /f /q "C:\Windows\System32\Tasks\install.exe" 2>nul
echo.

:: Очистка реестра
echo [*] Cleaning registry...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "WinSystemHelper" /f 2>nul
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "WindowsSecurityService" /f 2>nul
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "$77FirstRun" /f 2>nul
echo.

:: Очистка буфера обмена
echo [*] Clearing clipboard...
echo off | clip
echo.

echo [+] Cleanup completed successfully!
echo [*] System will be clean after restart
timeout /t 5
exit 