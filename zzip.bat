::rename Altdrag.ini Altdrag_ramon.ini
::rename AltDrag_default.ini AltDrag.ini

advzipNT -a4 -i 256 AltDrag_bin.zip AltDrag.txt AltDrag.exe Hooks.dll AltDrag.ini License.txt Lang\*
Deflopt AltDrag_bin.zip

advzipNT -a4 -i 32 AltDrag_src.zip AltDrag.txt AltDrag.ini License.txt altdrag.c altdrag.rc config.c hooks.c hooks.rc languages.c languages.h resource.h tray.c rpc.h unfuck.h window.rc x86.exe.manifest mk.bat media\* Lang\*

Deflopt AltDrag_src.zip

::rename Altdrag.ini Altdrag_default.ini
::rename AltDrag_ramon.ini AltDrag.ini

