::schtasks /CREATE /XML .\AltSnap.xml /TR "%~dp0AltSnap.exe"
:: If you do not want to use the xml file you can use:
:: schtasks.exe /CREATE /TN "AltSnap" /TR "%~dp0AltSnap.exe" /SC onlogon /RL highest /DELAY 0:10
:: However AltSnap will be killed after 3 days by default.
:: The only way not to have this autokill is to use an xml file.
:: Make sure to adjust the path to the AltSnap executable in the AltSnap.xml file.
@echo =============================================================================
@echo = Setup Scheduled task for elevated AltSnap auto-start without UAC prompt   =
@echo = Make sure you are running this from an elevated shell                     =
@echo = If you do not want to continue, close the window or hit Ctrl+C            =
@echo =============================================================================
@echo Going to run command:
@echo schtasks.exe /CREATE /XML .\AltSnap.xml /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9
@pause
schtasks.exe /CREATE /XML .\AltSnap.xml /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9
@pause
