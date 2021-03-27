CC=gcc

WARNINGS=-Wall -Wformat-security -Wstrict-overflow -Wsign-compare -Wclobbered \
    -Wempty-body -Wignored-qualifiers -Wuninitialized -Wtype-limits -Woverride-init \
    -Wlogical-op -Wno-multichar -Wno-attributes -Wduplicated-cond -Wduplicated-branches \
    -Wnull-dereference

CFLAGS=-Os -fno-stack-check -fno-stack-protector -foptimize-strlen -fno-ident \
    -mno-stack-arg-probe -march=i386 -mtune=i686 -mpreferred-stack-boundary=2 \
    -flto -nostdlib -lkernel32 -luser32 -lmsvcrt -lgdi32 -s \
    -Wl,-dynamicbase,-nxcompat,--no-seh,--relax -D__USE_MINGW_ANSI_STDIO=0 \
    $(WARNINGS)

default: AltDrag.exe hooks.dll

AltDrag.exe : altdragr.o altdrag.c hooks.h tray.c config.c languages.h languages.c unfuck.h
	$(CC) -o AltDrag.exe altdrag.c altdragr.o $(CFLAGS) -Wl,--build-id,--tsaware -lcomctl32 -mwindows -ladvapi32 -lshell32 -e_unfuckWinMain@0

altdragr.o : altdrag.rc window.rc resource.h x86.exe.manifest media/find.cur media/find.ico media/icon.ico media/tray-disabled.ico media/tray-enabled.ico
	windres altdrag.rc altdragr.o

hooks.dll : hooks.c hooks.h hooksr.o unfuck.h
	$(CC) -o hooks.dll hooks.c hooksr.o $(CFLAGS) -mdll -e_DllMain@12

hooksr.o: hooks.rc
	windres hooks.rc hooksr.o

clean :
	rm altdragr.o AltDrag.exe hooksr.o hooks.dll
