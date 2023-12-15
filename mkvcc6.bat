rc hooks.rc
rc altsnap.rc

@set DEFINES=/D "DWORD_PTR=DWORD" /D "LONG_PTR=INT_PTR" /D "ULONG_PTR=DWORD"
@set DEFINES= %DEFINES% /D "NO_VISTA" /D "NO_OLEAPI" /D "DECORATED_HOOKS_DLL_PROCS"

cl /c /Tp altsnap.c /nologo /Ox /Oi /Os /Gy  %DEFINES%

link altsnap.obj altsnap.res /nodefaultlib /filealign:512 /subsystem:windows /OPT:REF /OPT:ICF,7 /LARGEADDRESSAWARE  /machine:I386  /entry:unfuckWinMain kernel32.lib user32.lib shell32.lib advapi32.lib gdi32.lib comctl32.lib

cl /c /Tp hooks.c /nologo /Ox /Oi /Os /Gy /GS- /LD %DEFINES%

link hooks.obj hooks.res /nodefaultlib /DLL /filealign:512 /subsystem:windows /OPT:REF /OPT:ICF,7 /LARGEADDRESSAWARE  /machine:I386  /entry:DllMain kernel32.lib user32.lib gdi32.lib

@set DEFINES=
