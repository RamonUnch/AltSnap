@echo =============================================================================
@echo = This batch will remove the AltSnap's Scheduled task!                      =
@echo = If you do not want to continue, close the window or hit Ctrl+C            =
@echo =============================================================================
@echo Going to run command:
@echo schtasks.exe /DELETE /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9
@pause
schtasks.exe /DELETE /TN "AltSnap" %1 %2 %3 %4 %5 %6 %7 %8 %9
@pause
