CC=gcc
WR=windres

ifdef ComSpec
	RM = cmd.exe /C del /Q
else
	RM = rm -f
endif

# Common Warnings
WARNINGS=-Wall -pedantic -Wno-unused-function

WARNINGS+= \
	-Wformat-security \
	-Wformat=2 \
	-Wsign-compare \
	-Wuninitialized \
	-Wno-multichar \
	-Wno-unused-function \
	-Wshadow \
	-Wcast-align \
	-Wredundant-decls $(EXTRA_WARNINGS)

# GCC8+
#	-Wimplicit-fallthrough \
#	-Wc++-compat \
#
#	-Wformat-signedness \
#	-Wstrict-overflow \
#	-Warray-bounds=2 \
#	-Woverflow \
#	-Wclobbered \
#	-Wempty-body \
#	-Wduplicated-cond \
#	-Wduplicated-branches \
#	-Wnull-dereference \
#	-Wwrite-strings
#
#	-Wno-attributes \
#	-Wignored-qualifiers \
#	-Woverride-init \
#	-Wlogical-op \
#	-Wtype-limits \
#
#	-Wsuggest-attribute=pure \
#	-Wsuggest-attribute=const \
#	-Wsuggest-attribute=noreturn \
#
#	-Wstack-usage=4096 \
#	-Wframe-larger-than=4096 \
#	-Werror=vla \
#	-Walloca \
#
#	-Wstringop-overflow=4 \
#	-Wold-style-declaration \
#	-Wjump-misses-init \
#

# GCC 10+
#WARNINGS += -Warith-conversion

# GCC 13+
#WARNINGS += -Wuse-after-free=3

# -Wcast-qual
# -Wsign-conversion
# -Wconversion
# -Wcast-qual

# -Wunsafe-loop-optimizations
# -Wpadded
# -Wstrict-overflow=5
# -Wtrivial-auto-var-init -ftrivial-auto-var-init=pattern \
# -Wunused-parameter
# -Wtraditional-conversion
# -fira-region=one/mixed
# -Wstack-usage=2048
# -finput-charset=UTF-8
# -Wc++-compat
#	-D__USE_MINGW_ANSI_STDIO=0 ## useless now
# -fanalyzer
# -mshstk

######
# Determines the platform
EXTRA_TARGET_CFLAGS=-m32 -march=i386 -mtune=i686 \
	-mpreferred-stack-boundary=2 \
	-momit-leaf-frame-pointer \
	-mno-stack-arg-probe

WR_FLAGS=-Fpe-i386
# end
#####

CFLAGS=-Os -std=c99 \
	$(EXTRA_TARGET_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-finput-charset=UTF-8 \
	-DUNICODE -D_UNICODE \
	-fshort-wchar \
	-fno-stack-check \
	-fno-ident \
	-fomit-frame-pointer \
	-fshort-enums \
	-fno-exceptions \
	-fno-asynchronous-unwind-tables \
	-fmerge-all-constants \
	-fgcse-sm \
	-DSTRICT \
	-Wp,-D_FORTIFY_SOURCE=2 \
	$(WARNINGS) \

LDFLAGS=-nostdlib \
	-lmsvcrt \
	-lkernel32 \
	-luser32 \
	-lgdi32 \
	-ladvapi32 \
	-s \
	-Wl,-s,-dynamicbase \
	-Wl,-nxcompat \
	-Wl,--no-seh \
	-Wl,--relax \
	-Wl,--disable-runtime-pseudo-reloc \
	-Wl,--enable-auto-import \
	-Wl,--disable-stdcall-fixup \

#	-Wl,--stack=1048576

EXELD = $(LDFLAGS) \
	-Wl,--tsaware \
	-lcomctl32 \
	-ladvapi32 \
	-lshell32 \
	$(EXTRA_EXE_LDFLAGS)

#	-Wl,--disable-reloc-section

default: AltSnap.exe hooks.dll

hooks.dll : hooks.c hooks.h hooksr.o unfuck.h nanolibc.h zones.c snap.c stealth.c
	$(CC) -o hooks.dll hooks.c hooksr.o $(CFLAGS) $(LDFLAGS) -mdll -fpic -e_DllMain@12 -Wl,--kill-at

AltSnap.exe : altsnapr.o altsnap.c hooks.h tray.c config.c languages.h languages.c unfuck.h nanolibc.h
	$(CC) -o AltSnap.exe altsnap.c altsnapr.o $(CFLAGS) $(EXELD) -mwindows -e_unfuckWinMain@0

altsnapr.o : altsnap.rc window.rc resource.h AltSnap.exe.manifest media/find.cur media/find.ico media/icon.ico media/tray-disabled.ico media/tray-enabled.ico
	$(WR) altsnap.rc altsnapr.o $(WR_FLAGS)

hooksr.o: hooks.rc resource.h
	$(WR) hooks.rc hooksr.o $(WR_FLAGS)

clean :
	$(RM) altsnapr.o AltSnap.exe hooksr.o hooks.dll
