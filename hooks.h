#ifndef ALTDRAG_RPC_H
#define ALTDRAG_RPC_H

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <windows.h>
#include "unfuck.h"

// Undecorated entry name in x64 mode
#ifdef WIN64
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc"
#else
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc@12"
#endif

// Extra messages for Action Menu
#define LP_CURSORPOS   (1<<0)
#define LP_TOPMOST     (1<<1)
#define LP_BORDERLESS  (1<<2)
#define LP_MAXIMIZED   (1<<3)
#define LP_ROLLED      (1<<4)
#define LP_MOVEONOFF   (1<<5)
#define LP_NOALTACTION (1<<6)

// App
#define APP_NAME       TEXT("AltSnap")
#define APP_NAMEA      "AltSnap"
#define APP_VERSION    "1.59"
#define APP_PROPPT     TEXT(APP_NAMEA"-RDim")
#define APP_PROPFL     TEXT(APP_NAMEA"-RFlag")
#define APP_PROPOFFSET TEXT(APP_NAMEA"-ROffset")
#define APP_PRBDLESS   TEXT(APP_NAMEA"-RStyle")
#define APP_ROLLED     TEXT(APP_NAMEA"-Rolled")
#define APP_ASONOFF    TEXT(APP_NAMEA"-ASOnOff")
#define APP_MOVEONOFF  TEXT(APP_NAMEA"-MoveOnOff")
#define FZ_PROPPT      TEXT("FancyZones_RestoreSize")
#define FZ_PROPZONES   TEXT("FancyZones_zones")

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
#define WM_UNIKEYMENU     (WM_USER+11)
#define WM_GETCLICKHWND   (WM_USER+12)
#define WM_STACKLIST      (WM_USER+13)
#define WM_FINISHMOVEMENT (WM_USER+14)
#define WM_CLOSEMODE      (WM_USER+15)
#define WM_SETLAYOUTNUM   (WM_USER+16)
#define WM_GETLAYOUTREZ   (WM_USER+17)
#define WM_GETBESTLAYOUT  (WM_USER+18)
// List of possible actions
enum action {
    AC_NONE=0, AC_MOVE, AC_RESIZE, AC_RESTORE
  , AC_MENU, AC_MINIMIZE, AC_MAXIMIZE
  , AC_CENTER , AC_ALWAYSONTOP, AC_CLOSE, AC_LOWER, AC_BORDERLESS
  , AC_KILL, AC_PAUSE, AC_RESUME, AC_MAXHV, AC_MINALL, AC_MUTE
  , AC_SIDESNAP, AC_EXTENDSNAP
  , AC_NSTACKED, AC_NSTACKED2, AC_PSTACKED, AC_PSTACKED2
  , AC_STACKLIST, AC_STACKLIST2, AC_ALTTABLIST
  , AC_ASONOFF, AC_MOVEONOFF
  , AC_MLZONE, AC_MTZONE, AC_MRZONE, AC_MBZONE
  , AC_XLZONE, AC_XTZONE, AC_XRZONE, AC_XBZONE
  , AC_STEPL, AC_STEPT, AC_STEPR, AC_STEPB
  , AC_SSTEPL, AC_SSTEPT, AC_SSTEPR, AC_SSTEPB
  , AC_ROLL, AC_ALTTAB, AC_VOLUME, AC_TRANSPARENCY, AC_HSCROLL
  , AC_ZOOM, AC_ZOOM2, AC_NPSTACKED, AC_NPSTACKED2
  , AC_MAXVALUE
  , AC_ORICLICK
};
// List of actions strings, keep the SAME ORDER than above
#define ACTION_MAP { \
    "Nothing", "Move", "Resize", "Restore"                         \
  , "Menu", "Minimize", "Maximize"                                 \
  , "Center", "AlwaysOnTop", "Close", "Lower", "Borderless"        \
  , "Kill", "Pause", "Resume", "MaximizeHV", "MinAllOther", "Mute" \
  , "SideSnap", "ExtendSnap"                                       \
  , "NStacked", "NStacked2", "PStacked", "PStacked2"               \
  , "StackList", "StackList2", "AltTabList"                        \
  , "ASOnOff", "MoveOnOff"                                         \
  , "MLZone", "MTZone", "MRZone", "MBZone"                         \
  , "XLZone", "XTZone", "XRZone", "XBZone"                         \
  , "StepL", "StepT", "StepR", "StepB"                             \
  , "SStepL", "SStepT", "SStepR", "SStepB"                         \
  , "Roll", "AltTab", "Volume", "Transparency", "HScroll"          \
  , "Zoom", "Zoom2", "NPStacked", "NPStacked2"                     \
}

#define MOUVEMENT(action) (action <= AC_RESIZE)

///////////////////////////////////////////////////////////////////////////
// Check if key is assigned in the HKlist
static int pure IsHotkeyy(unsigned char key, const unsigned char *HKlist)
{
    const UCHAR *pos=&HKlist[0];
    while (*pos) {
        if (key == *pos) {
            return 1;
        }
        pos++;
    }
    return 0;
}
static int pure IsActionInList(const enum action ac, const enum action *aclst)
{
    return IsHotkeyy(ac, aclst);
}
// Convert zone number to ini name entry
static TCHAR *ZidxToZonestr(int laynum, int idx, TCHAR *zname)
{
    if (laynum > 9 ) return NULL;
    TCHAR txt[16];
    zname[0] = !laynum?TEXT('\0'): TEXT('A')+laynum-1 ;
    zname[1] = '\0';
    lstrcat(zname, TEXT("Zone"));
    lstrcat(zname, itostr(idx, txt, 10)); // Zone Name from zone number

    return zname;
}
static char *ZidxToZonestrA(int laynum, int idx, char *zname)
{
    if (laynum > 9 ) return NULL;
    char txt[16];
    zname[0] = !laynum?'\0': 'A'+laynum-1 ;
    zname[1] = '\0';
    lstrcatA(zname, "Zone");
    lstrcatA(zname, itostrA(idx, txt, 10)); // Zone Name from zone number

    return zname;
}

// Map action string to actual action enum
static enum action MapActionW(const TCHAR *txt)
{
    static const char *action_map[] = ACTION_MAP;
    enum action ac;
    for (ac=0; ac < ARR_SZ(action_map); ac++) {
        if(!strtotcharicmp(txt, action_map[ac])) return ac;
    }
    return AC_NONE;
}

#endif /* ALTDRAG_RPC_H */

