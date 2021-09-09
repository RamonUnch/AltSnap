advzipNT -a4 -i 64 AltSnap_bin.zip AltSnap.txt AltSnap.exe Hooks.dll AltSnap.ini AltSnap.xml License.txt Lang\*
Deflopt AltSnap_bin.zip

advzipNT -a4 -i 16 AltSnap_src.zip AltSnap.txt AltSnap.ini AltSnap.xml altsnap.nsi License.txt altsnap.c altsnap.rc config.c hooks.c hooks.rc languages.c languages.h resource.h tray.c hooks.h unfuck.h nanolibc.h window.rc AltSnap.exe.manifest makefile media\* Lang\*

Deflopt AltSnap_src.zip
