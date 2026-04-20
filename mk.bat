
:loop
@taskkill /IM AltSnap.exe 2> nul
@if %ERRORLEVEL% EQU 0 (
	goto :loop
)

make -j 2 %1

@if !%1 == !clean GOTO FINISH
@start AltSnap.exe

:FINISH
