@echo off
:: Check for elevation
fsutil dirty query %systemdrive% >nul 2>&1
if '%errorlevel%' NEQ '0' (
    :: Not elevated, so relaunch using vbscript
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "cmd.exe", "/c ""%~f0"" %*", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /b
)

setlocal

:: =============================================================================
:: = This batch will remove the AltSnap's Scheduled task!                      =
:: = If you do not want to continue, close the window or hit Ctrl+C            =
:: =============================================================================

echo =============================================================================
echo = This batch will remove the AltSnap's Scheduled task!                      =
echo = If you do not want to continue, close the window or hit Ctrl+C            =
echo =============================================================================

echo.
echo Going to run command:
echo schtasks.exe /DELETE /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9
echo.

set /p "choice=Are you sure you want to delete the AltSnap scheduled task? [Y/n]: "

:: Handle empty input (user just pressed Enter) as "Y"
if "%choice%"=="" (
    set "choice=y"
)

if /i "%choice%" equ "y" (
    echo Deleting the AltSnap scheduled task...
    schtasks.exe /DELETE /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9 /F
) else (
    echo Deletion cancelled.
)

pause
exit /b 0
