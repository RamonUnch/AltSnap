# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AltSnap is a Windows utility that enables Alt+Click window moving/resizing, inspired by Linux window managers. It is a C/Win32 fork of Stefan Sundin's AltDrag, rewritten by Raymond Gillibert. Targets Windows NT 4 through Windows 11. GPLv3+ licensed.

## Build Commands

All builds require MinGW GCC (or Clang/TCC for alternative Makefiles).

```bash
make                          # 32-bit release (default)
make -f MakefileX64           # 64-bit release
make -f Makefiledb            # 32-bit debug
make -f MakefileX64db         # 64-bit debug
make clean                    # Remove build artifacts (deletes AltSnap.exe + hooks.dll)
```

Quick rebuild scripts (kill running instance, build, restart):
- `mk.bat` — 32-bit
- `mk64.bat` — 64-bit

MSVC builds: `mkmsvc.bat` (Visual Studio) or `mkvcc6.bat` (VC++ 6.0)

Release packaging: `makerelease.bat` builds both architectures and creates NSIS installers + zip archives.

## Architecture

**Two-component design — EXE + DLL:**

### AltSnap.exe (main executable)
- **Entry point**: `unfuckWinMain()` in `altsnap.c` — bypasses CRT, calls `tWinMain()`
- Single compilation unit: `altsnap.c` #includes `tray.c`, `config.c`, `languages.c` directly (not separate .o files)
- Responsibilities: system tray icon, configuration dialog (property sheets), loading hooks.dll, message loop, elevation, command-line parsing

### hooks.dll (hook library — bulk of program logic)
- **Entry point**: `DllMain()` in `hooks.c` (~6,750 lines)
- Exports: `Load()` (returns WNDPROC), `Unload()`, `LowLevelKeyboardProc()`
- Also a single compilation unit: `hooks.c` #includes `zones.c` and `snap.c`
- Responsibilities: low-level keyboard/mouse hooks, window move/resize logic, snap zones, Aero snap emulation, action menu, scroll handling, all ~50 actions

### Key headers
- `hooks.h` — shared header with action enums (X-macro `ACTION_MAP`), structs, IPC message definitions, window property names
- `unfuck.h` — Win32 compatibility layer; dynamically loads newer APIs (DWM, DPI) at runtime so the binary runs on older Windows
- `nanolibc.h` — minimal C library (the project links with `-nostdlib`)
- `languages.h` — L10N string definitions

### Data flow
1. AltSnap.exe starts, loads hooks.dll, calls `Load()` to get a WNDPROC
2. DLL installs `WH_KEYBOARD_LL` hook
3. On Alt+Click detection, DLL hooks mouse and handles movement/resize
4. Config read from `AltSnap.ini` (portable) or `%APPDATA%\AltSnap\AltSnap.ini`
5. EXE handles config UI/tray/IPC; DLL handles all window manipulation

## Coding Conventions

- Pure C99, Win32 API only — no frameworks, no C runtime (`-nostdlib`)
- Unicode build (`-DUNICODE -D_UNICODE`, `-municode`, `-fshort-wchar`)
- `-fshort-enums` — enums may be smaller than int
- Heavy size optimization (`-Os`, `-fmerge-all-constants`, `-fno-exceptions`, etc.)
- Warnings treated as errors for VLAs (`-Werror=vla`)
- Stack usage limit: 4096 bytes per function (`-Wstack-usage=4096`)
- Actions are defined via X-macro `ACTION_MAP` in `hooks.h` — to add a new action, add an `ACVALUE()` entry and implement the handler in `hooks.c`

## Configuration Files

- `AltSnap.dni` — default config template (UTF-16LE INI, ~1,400 lines, heavily commented). If no `AltSnap.ini` exists, this is copied to create one
- `Lang/*.ini` — 22 translation files (UTF-16LE)
- `.gitattributes` — enforces UTF-16LE-BOM encoding for `.dni` and `Lang/*` files
- `altsnap.nsi` — NSIS installer script

## Testing

No automated tests or CI. Manual testing only — build and run the exe.
