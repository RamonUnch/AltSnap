@taskkill /IM AltSnap.exe 2> nul
@taskkill /IM AltSnap.exe 2> nul

make %1

@if !%1 == !clean GOTO FINISH
@start AltSnap.exe

:FINISH
