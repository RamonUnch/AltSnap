:: simple build file for Visual Studio
:: 1st, set environement example if you want to target ARM64 from an AMD64.
:: call "%PROGRAMFILES%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=arm64 -host_arch=amd64

rc hooks.rc
rc altsnap.rc

:: you can disable some stuffs in AltSnap if needed...
::@set DEFINES= /D "NO_VISTA" /D "NO_OLEAPI"

cl /c altsnap.c /nologo /Ox /Oi /Os /Gy %DEFINES%

link altsnap.obj altsnap.res /subsystem:windows /entry:unfuckWinMain kernel32.lib user32.lib shell32.lib advapi32.lib gdi32.lib comctl32.lib vcruntime.lib

cl /c hooks.c /nologo /Ox /Oi /Os /Gy /GS- /LD %DEFINES%

link hooks.obj hooks.res /DLL /subsystem:windows /entry:DllMain kernel32.lib user32.lib gdi32.lib vcruntime.lib

::@set DEFINES=
