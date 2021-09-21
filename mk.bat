@taskkill /IM AltSnap.exe 2> nul

::@if !%TMDGCC9% == ! (GOTO TDM) else (goto END)

:::TDM
::set TMDGCC9=1
::call tdmpath.bat

:::END

make %1

@if !%1 == !clean GOTO FINISH
@start AltSnap.exe

:FINISH
