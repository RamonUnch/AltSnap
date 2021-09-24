@taskkill /IM AltSnap.exe 2> nul
@taskkill /IM AltSnap.exe 2> nul

@if !%1 == !db GOTO DEBUG

make -fMakefileX64 %1

@if !%1 == !clean GOTO FINISH
@start AltSnap.exe
@GOTO FINISH

: DEBUG
@echo x86_64b Debug build
make -fMakefileX64db
gdb.exe AltSnap.exe

:FINISH
