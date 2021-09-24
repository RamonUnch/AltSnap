@set WARNINGS=-Wall -Wformat-security -Wstrict-overflow -Wsign-compare -Wclobbered -Wempty-body -Wignored-qualifiers -Wuninitialized -Wtype-limits -Woverride-init  -Wlogical-op

@set CFLAGS=-Os -m64 -momit-leaf-frame-pointer -mno-stack-arg-probe -fno-ident -fomit-frame-pointer -fno-stack-check -fno-stack-protector -lkernel32 -lmsvcrt -luser32 -lgdi32 -flto -Wl,-dynamicbase,-nxcompat,--no-seh -s -Wp,-D_FORTIFY_SOURCE=2

:: -finline-functions-called-once 
:: -Wunused-parameter
:: -nostdlib 
:: -Wduplicated-cond  -Wduplicated-branches -Wnull-dereference

@taskkill /IM AltSnap.exe 2> nul

@ECHO WINDRES ALTDRAG.RC
@windres altsnap.rc altsnapr.o

@ECHO WINDRES HOOKS.RC
@windres hooks.rc hooksr.o

@ECHO CC ALTSNAP.EXE
@gcc -o AltSnap.exe altsnap.c altsnapr.o %CFLAGS% -lcomctl32 -mwindows -ladvapi32 -lshell32

@ECHO CC HOOKS.DLL
@gcc -o hooks.dll hooks.c hooksr.o -nostdlib %CFLAGS% -mdll -D__USE_MINGW_ANSI_STDIO=0 -eDllMain

@set CFLAGS=

@start AltSnap.exe
