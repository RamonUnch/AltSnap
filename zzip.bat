::rename Altdrag.ini Altdrag_ramon.ini
::rename AltDrag_default.ini AltDrag.ini

advzipNT -a4 -i 256 AltDrag_bin.zip AltDrag.txt AltDrag.exe Hooks.dll AltDrag.ini AltDrag.xml License.txt Lang\*
Deflopt AltDrag_bin.zip

advzipNT -a4 -i 32 AltDrag_src.zip AltDrag.txt AltDrag.ini AltDrag.xml altdrag.nsi License.txt altdrag.c altdrag.rc config.c hooks.c hooks.rc languages.c languages.h resource.h tray.c hooks.h unfuck.h nanolibc.h window.rc AltDrag.exe.manifest mk.bat media\* Lang\*

Deflopt AltDrag_src.zip

::rename Altdrag.ini Altdrag_default.ini
::rename AltDrag_ramon.ini AltDrag.ini
