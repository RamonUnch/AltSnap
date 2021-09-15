@taskkill /IM AltSnap.exe 2> nul

make -fMakefileX64 %1

@if !%1 == !clean GOTO FINISH
@start AltSnap.exe

:FINISH
