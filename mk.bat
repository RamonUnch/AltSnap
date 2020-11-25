@set CFLAGS= -Os -fno-stack-check -fno-stack-protector -mno-stack-arg-probe -march=i386 -mtune=i686 -mpreferred-stack-boundary=2 -foptimize-strlen -nostdlib -lkernel32 -luser32 -lmsvcrt -flto -s -Wall -Wl,-dynamicbase,-nxcompat -Wformat-security -Wstrict-overflow -Wp,-D_FORTIFY_SOURCE=2

:: -Wextra -Wno-sign-compare -Wno-implicit-fallthrough -Wno-missing-field-initializers

@taskkill /IM AltDrag.exe 2> nul

@windres altdrag.rc altdragr.o
@windres hooks.rc hooksr.o

gcc -o AltDrag.exe altdrag.c altdragr.o %CFLAGS% -lcomctl32 -mwindows -ladvapi32 -lshell32 -e_unfuckWinMain@0
gcc -o hooks.dll hooks.c hooksr.o %CFLAGS% -mdll -lgdi32 -e_DllMain@12

@set CFLAGS=

@start AltDrag.exe
