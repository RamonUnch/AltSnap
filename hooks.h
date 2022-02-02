#ifndef ALTDRAG_RPC_H
#define ALTDRAG_RPC_H

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0400
#include <windows.h>

#include "unfuck.h"

// Undecorated entry name in x64 mode
#ifdef WIN64
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc"
#else
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc@12"
#endif

// App
#define APP_NAME       L"AltSnap"
#define APP_NAMEA      "AltSnap"
#define APP_VERSION    "1.50"
#define APP_PROPPT     APP_NAMEA"-RestoreDimentions"
#define APP_PROPFL     APP_NAMEA"-RestoreFlag"
#define APP_PROPOFFSET APP_NAMEA"-RestoreOffset"
#define FZ_PROPPT      "FancyZones_RestoreSize"
#define FZ_PROPZONES   "FancyZones_zones"

// User Messages
#define WM_TRAY           (WM_USER+2)
#define WM_SCLICK         (WM_USER+3)
#define WM_UPDCFRACTION   (WM_USER+4)
#define WM_UPDATETRAY     (WM_USER+5)
#define WM_OPENCONFIG     (WM_USER+6)
#define WM_CLOSECONFIG    (WM_USER+7)
#define WM_UPDATESETTINGS (WM_USER+8)
#define WM_ADDTRAY        (WM_USER+9)
#define WM_HIDETRAY       (WM_USER+10)
#define WM_REHOOKKEYBOARD (WM_USER+11)


// List of possible actions
enum action { AC_NONE=0, AC_MOVE, AC_RESIZE, AC_MENU, AC_MINIMIZE, AC_MAXIMIZE, AC_CENTER
            , AC_ALWAYSONTOP, AC_CLOSE, AC_LOWER, AC_BORDERLESS, AC_KILL
            , AC_MAXHV, AC_MINALL, AC_MUTE
            , AC_ROLL, AC_ALTTAB, AC_VOLUME, AC_TRANSPARENCY, AC_HSCROLL, AC_SIZEPS };

#define MOUVEMENT(action) (action <= AC_RESIZE)

// Convert zone number to ini name entry
static wchar_t *ZidxToZonestr(int idx, wchar_t *zname)
{
    wchar_t txt[8];
    zname[0] = '\0';
    wcscat(zname, L"Zone");
    wcscat(zname, itowL(idx, txt, 10)); // Zone Name from zone number

    return zname;
}

#endif /* ALTDRAG_RPC_H */
