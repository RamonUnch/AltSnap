@set WARNINGS=-Wall -Wformat-security -Wstrict-overflow -Wsign-compare -Wclobbered -Wempty-body -Wignored-qualifiers -Wuninitialized -Wtype-limits -Woverride-init  -Wlogical-op

@set CFLAGS=-Os -m64 -fno-stack-check -fno-stack-protector -mno-stack-arg-probe -lkernel32 -luser32 -lgdi32 -flto -Wl,-dynamicbase,-nxcompat,--no-seh -s

:: -finline-functions-called-once 
:: -Wunused-parameter
:: -nostdlib 
:: -Wduplicated-cond  -Wduplicated-branches -Wnull-dereference

@taskkill /IM AltDrag.exe 2> nul

@ECHO WINDRES ALTDRAG.RC
@windres altdrag.rc altdragr.o

@ECHO WINDRES HOOKS.RC
@windres hooks.rc hooksr.o

@ECHO CC ALTDRAG.EXE
@gcc -o AltDrag.exe altdrag.c altdragr.o %CFLAGS% -lcomctl32 -mwindows -ladvapi32 -lshell32

@ECHO CC HOOKS.DLL
@gcc -o hooks.dll hooks.c hooksr.o -nostdlib %CFLAGS% -mdll -D__USE_MINGW_ANSI_STDIO=0 -eDllMain

@set CFLAGS=

@start AltDrag.exe
