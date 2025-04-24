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
:: = Setup Scheduled task for elevated AltSnap auto-start without UAC prompt   =
:: = Make sure you are running this from an elevated shell                     =
:: = Default AltSnap executable location is %%APPDATA%%\AltSnap\AltSnap.exe     =
:: = Adjust the executable location in the AltSnap.xml file if AltSnap is      =
:: = installed in a different location                                         =
:: = If you do not want to continue, close the window or hit Ctrl+C            =
:: =============================================================================

echo =============================================================================
echo = Setup Scheduled task for elevated AltSnap auto-start without UAC prompt   =
echo = Make sure you are running this from an elevated shell                     =
echo = Default AltSnap executable location is %%APPDATA%%\AltSnap\AltSnap.exe     =
echo = Adjust the executable location in the AltSnap.xml file if AltSnap is      =
echo = installed in a different location                                         =
echo = If you do not want to continue, close the window or hit Ctrl+C            =
echo =============================================================================

set "AltSnapXML=%APPDATA%\AltSnap\AltSnap.xml"
set "AltSnapExe=%APPDATA%\AltSnap\AltSnap.exe"

:: Check if AltSnap.exe exists
if not exist "%AltSnapExe%" (
    echo.
    echo AltSnap executable not found in the default location: %AltSnapExe%
    echo Please ensure AltSnap is installed or specify the correct path in the AltSnap.xml file.
    echo.
    pause
    exit /b 1
)

:: Check if AltSnap.xml exists
if not exist "%AltSnapXML%" (
    echo.
    echo AltSnap.xml file not found: %AltSnapXML%
    echo Please ensure the AltSnap.xml file exists in the specified location.
    echo.
    pause
    exit /b 1
)

echo.
echo Going to run command:
echo schtasks.exe /CREATE /XML "%AltSnapXML%" /TN "AltSnap" %*
echo.
pause

schtasks.exe /CREATE /XML "%AltSnapXML%" /TN "AltSnap" %*

echo.
pause

endlocal
exit /b 0
