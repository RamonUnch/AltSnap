@set WARNINGS=-Wall -Wformat-security -Wstrict-overflow -Wsign-compare -Wclobbered -Wempty-body -Wignored-qualifiers -Wuninitialized -Wtype-limits -Woverride-init -Wduplicated-cond  -Wduplicated-branches -Wlogical-op  -Wnull-dereference
@set CFLAGS=-Os -fno-stack-check -fno-stack-protector -mno-stack-arg-probe -march=i386 -mtune=i686 -mpreferred-stack-boundary=2 -foptimize-strlen -nostdlib -lkernel32 -luser32 -lmsvcrt -flto -Wl,-dynamicbase,-nxcompat -Wp,-D_FORTIFY_SOURCE=2 -s %WARNINGS%

:: -finline-functions-called-once 
:: -Wunused-parameter

@taskkill /IM AltDrag.exe 2> nul

@windres altdrag.rc altdragr.o
@windres hooks.rc hooksr.o

gcc -o AltDrag.exe altdrag.c altdragr.o %CFLAGS% -lcomctl32 -mwindows -ladvapi32 -lshell32 -e_unfuckWinMain@0
gcc -o hooks.dll hooks.c hooksr.o %CFLAGS% -mdll -lgdi32 -e_DllMain@12

@set CFLAGS=

@start AltDrag.exe
