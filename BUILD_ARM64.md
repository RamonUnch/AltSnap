# Building AltSnap for Windows ARM64 (WOA)

## Docker Cross-Compilation (Recommended)

This uses the `dockcross/windows-arm64` Docker image with LLVM 14 and MinGW-w64 toolchain.

### Prerequisites

Docker must be installed and running:
```bash
docker --version
```

### Building

Build for ARM64 using Docker:
```bash
# Build everything
make -f MakefileARM64Docker

# Build just the executable
make -f MakefileARM64Docker AltSnap.exe

# Build just the DLL
make -f MakefileARM64Docker hooks.dll

# Clean build artifacts
make -f MakefileARM64Docker clean
```


## Output

The build produces:
- `AltSnap.exe` - Main GUI executable for Windows ARM64
- `hooks.dll` - Support library for Windows ARM64

Both files are PE32+ executables targeting Aarch64 (ARM64) for MS Windows.

## Compatibility

- Windows 10 on ARM (version 1709+)
- Windows 11 on ARM
- ARM64 devices like Surface Pro X, ARM-based laptops, etc.

## Build Details

- **Toolchain:** LLVM 14/Clang with MinGW-w64 headers
- **Architecture:** ARMv8-A (ARM64)
- **Build time:** ~60 seconds (after initial image download)
- **Docker image size:** ~600MB (downloaded once)

## Troubleshooting

If you get permission errors on the built files, they may be owned by root (from Docker). Fix with:
```bash
sudo chown $USER:$USER AltSnap.exe hooks.dll
```