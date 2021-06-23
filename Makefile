CC=gcc

INCLUDE=-IC:\msys64\mingw32\i686-w64-mingw32\include -ID:\Straw\M81\mingw32\i686-w64-mingw32\include

WARNINGS=-Wall -Wformat-security -Wstrict-overflow -Wsign-compare -Wclobbered \
    -Wempty-body -Wignored-qualifiers -Wuninitialized -Wtype-limits -Woverride-init \
    -Wlogical-op -Wno-multichar -Wno-attributes -Wduplicated-cond -Wduplicated-branches \
    -Wnull-dereference -Wno-unused-function -Wabsolute-value -Wshadow -Wstack-usage=4096

# -Wunused-parameter
# -Wtraditional-conversion
#-fira-region=one/mixed
# -Wstack-usage=2048

OPTI=-O2 -fira-region=mixed -fno-align-functions -fno-align-jumps -fno-align-labels -fno-align-loops -freorder-blocks-algorithm=simple -fno-tree-ch
#  -fshort-enums
CFLAGS=-Os -fno-stack-check -fno-stack-protector -fno-ident -fomit-frame-pointer \
    -mno-stack-arg-probe -march=i386 -mtune=i686 -mpreferred-stack-boundary=2 -momit-leaf-frame-pointer \
    -nostdlib -lmsvcrt -lkernel32 -luser32 -lgdi32 -s -fgcse-sm -fgcse-las -fno-plt \
    -Wl,-dynamicbase,-nxcompat,--no-seh,--relax -Wp,-D_FORTIFY_SOURCE=2 \
    $(INCLUDE) $(WARNINGS)


default: AltDrag.exe hooks.dll

hooks.dll : hooks.c hooks.h hooksr.o unfuck.h nanolibc.h
	$(CC) -o hooks.dll hooks.c hooksr.o $(CFLAGS) -mdll -e_DllMain@12

AltDrag.exe : altdragr.o altdrag.c hooks.h tray.c config.c languages.h languages.c unfuck.h nanolibc.h
	$(CC) -o AltDrag.exe altdrag.c altdragr.o $(CFLAGS) -Wl,--tsaware -lcomctl32 -mwindows -ladvapi32 -lshell32 -e_unfuckWinMain@0

altdragr.o : altdrag.rc window.rc resource.h AltDrag.exe.manifest media/find.cur media/find.ico media/icon.ico media/tray-disabled.ico media/tray-enabled.ico
	windres altdrag.rc altdragr.o

hooksr.o: hooks.rc
	windres hooks.rc hooksr.o

clean :
	rm altdragr.o AltDrag.exe hooksr.o hooks.dll
