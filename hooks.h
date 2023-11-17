#ifndef ALTDRAG_RPC_H
#define ALTDRAG_RPC_H

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <windows.h>
#include "unfuck.h"

#ifndef LOW_LEVEL_KB_PROC
#define LOW_LEVEL_KB_PROC "LowLevelKeyboardProc"
#endif
#ifndef LOAD_PROC
#define LOAD_PROC "Load"
#endif
#ifndef UNLOAD_PROC
#define UNLOAD_PROC "Unload"
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
#define APP_PROPPT     APP_NAME TEXT("-RDim")
#define APP_PROPFL     APP_NAME TEXT("-RFlag")
#define APP_PROPOFFSET APP_NAME TEXT("-ROffset")
#define APP_PRBDLESS   APP_NAME TEXT("-RStyle")
#define APP_ROLLED     APP_NAME TEXT("-Rolled")
#define APP_ASONOFF    APP_NAME TEXT("-ASOnOff")
#define APP_MOVEONOFF  APP_NAME TEXT("-MoveOnOff")
#define APP_PRODPI     APP_NAME TEXT("-ODPI")
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
#define WM_GETZONESLEN    (WM_USER+19)
#define WM_GETZONES       (WM_USER+20)

// List of possible actions
// ACVALUE(AC_ENUM, "IniString", Info)
#define ACTION_MAP \
    ACVALUE(AC_NONE=0,       "Nothing",     00) \
    ACVALUE(AC_MOVE,         "Move",        MR) \
    ACVALUE(AC_RESIZE,       "Resize",      MR) \
    ACVALUE(AC_RESTORE,      "Restore",     MR) \
    ACVALUE(AC_MENU,         "Menu",        CL) \
    ACVALUE(AC_MINIMIZE,     "Minimize",    MR) \
    ACVALUE(AC_MAXIMIZE,     "Maximize",    MR) \
    ACVALUE(AC_CENTER,       "Center",      MR) \
    ACVALUE(AC_ALWAYSONTOP,  "AlwaysOnTop", ZO) \
    ACVALUE(AC_CLOSE,        "Close",       CL) \
    ACVALUE(AC_LOWER,        "Lower",       ZO) \
    ACVALUE(AC_FOCUS,        "Focus",       ZO) \
    ACVALUE(AC_BORDERLESS,   "Borderless",  00) \
    ACVALUE(AC_KILL,         "Kill",        CL) \
    ACVALUE(AC_PAUSE,        "Pause",       CL) \
    ACVALUE(AC_RESUME,       "Resume",      CL) \
    ACVALUE(AC_MAXHV,        "MaximizeHV",  MR) \
    ACVALUE(AC_MINALL,       "MinAllOther", 00) \
    ACVALUE(AC_MUTE,         "Mute",        00) \
    ACVALUE(AC_SIDESNAP,     "SideSnap",    MR) \
    ACVALUE(AC_EXTENDSNAP,   "ExtendSnap",  MR) \
    ACVALUE(AC_EXTENDTNEDGE, "ExtendTNEdge",MR) \
    ACVALUE(AC_MOVETNEDGE,   "MoveTNEdge",  MV) \
    ACVALUE(AC_NSTACKED,     "NStacked",    ZO) \
    ACVALUE(AC_NSTACKED2,    "NStacked2",   ZO) \
    ACVALUE(AC_PSTACKED,     "PStacked",    ZO) \
    ACVALUE(AC_PSTACKED2,    "PStacked2",   ZO) \
    ACVALUE(AC_STACKLIST,    "StackList",   CL) \
    ACVALUE(AC_STACKLIST2,   "StackList2",  CL) \
    ACVALUE(AC_ALTTABLIST,   "AltTabList",  CL) \
    ACVALUE(AC_ALTTABFULLLIST, "AltTabFullList",  CL) \
    ACVALUE(AC_ASONOFF,      "ASOnOff",     CL) \
    ACVALUE(AC_MOVEONOFF,    "MoveOnOff",   CL) \
    \
    ACVALUE(AC_MLZONE, "MLZone", MR) \
    ACVALUE(AC_MTZONE, "MTZone", MR) \
    ACVALUE(AC_MRZONE, "MRZone", MR) \
    ACVALUE(AC_MBZONE, "MBZone", MR) \
    ACVALUE(AC_XLZONE, "XLZone", MR) \
    ACVALUE(AC_XTZONE, "XTZone", MR) \
    ACVALUE(AC_XRZONE, "XRZone", MR) \
    ACVALUE(AC_XBZONE, "XBZone", MR) \
    \
    ACVALUE(AC_XTNLEDGE, "XTNLEdge", MR) \
    ACVALUE(AC_XTNTEDGE, "XTNTEdge", MR) \
    ACVALUE(AC_XTNREDGE, "XTNREdge", MR) \
    ACVALUE(AC_XTNBEDGE, "XTNBEdge", MR) \
    ACVALUE(AC_MTNLEDGE, "MTNLEdge", MV) \
    ACVALUE(AC_MTNTEDGE, "MTNTEdge", MV) \
    ACVALUE(AC_MTNREDGE, "MTNREdge", MV) \
    ACVALUE(AC_MTNBEDGE, "MTNBEdge", MV) \
    \
    ACVALUE(AC_STEPL,  "StepL",  MR) \
    ACVALUE(AC_STEPT,  "StepT",  MR) \
    ACVALUE(AC_STEPR,  "StepR",  MR) \
    ACVALUE(AC_STEPB,  "StepB",  MR) \
    ACVALUE(AC_SSTEPL, "SStepL", MR) \
    ACVALUE(AC_SSTEPT, "SStepT", MR) \
    ACVALUE(AC_SSTEPR, "SStepR", MR) \
    ACVALUE(AC_SSTEPB, "SStepB", MR) \
    \
    ACVALUE(AC_FOCUSL,  "FocusL",  ZO) \
    ACVALUE(AC_FOCUST,  "FocusT",  ZO) \
    ACVALUE(AC_FOCUSR,  "FocusR",  ZO) \
    ACVALUE(AC_FOCUSB,  "FocusB",  ZO) \
    \
    ACVALUE(AC_ROLL,         "Roll",         MR) \
    ACVALUE(AC_ALTTAB,       "AltTab",       ZO) \
    ACVALUE(AC_VOLUME,       "Volume",       00) \
    ACVALUE(AC_TRANSPARENCY, "Transparency", 00) \
    ACVALUE(AC_HSCROLL,      "HScroll",      00) \
    ACVALUE(AC_ZOOM,         "Zoom",         MR) \
    ACVALUE(AC_ZOOM2,        "Zoom2",        MR) \
    ACVALUE(AC_NPSTACKED,    "NPStacked",    ZO) \
    ACVALUE(AC_NPSTACKED2,   "NPStacked2",   ZO)

#define ACVALUE(a, b, c) a,
enum action { ACTION_MAP AC_SHRT0, AC_SHRT9=AC_SHRT0+10, AC_MAXVALUE, AC_ORICLICK };
#undef ACVALUE

// List of extra info options
#define ACINFO_MOVE     (1)
#define ACINFO_RESIZE   (2)
#define ACINFO_ZORDER   (4)
#define ACINFO_CLOSE    (8)
#define MV ACINFO_MOVE
#define RZ ACINFO_RESIZE
#define ZO ACINFO_ZORDER
#define CL ACINFO_CLOSE

#define MR (ACINFO_MOVE|ACINFO_RESIZE)

// Helper function to get extra action info
static xpure UCHAR ActionInfo(enum action action)
{
    #define ACVALUE(a, b, c) (c),
    static const UCHAR action_info[] = { ACTION_MAP };

    #undef ACVALUE
    return action_info[action];
}
#undef MV
#undef RZ
#undef ZO
#undef CL
#undef MR

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
    do {
        if(ac == *aclst)
            return 1;
    } while(*aclst++ != AC_NONE);

    return 0;
}
// Convert zone number to ini name entry
static TCHAR *ZidxToZonestr(int laynum, int idx, TCHAR zname[AT_LEAST 32])
{
    if (laynum > 9 ) return NULL;
    TCHAR txt[UINT_DIGITS+1];
    zname[0] = !laynum?TEXT('\0'): TEXT('A')+laynum-1 ;
    zname[1] = '\0';
    lstrcat_s(zname, 32, TEXT("Zone"));
    lstrcat_s(zname, 32, Uint2lStr(txt, idx)); // Zone Name from zone number

    return zname;
}
static char *ZidxToZonestrA(int laynum, int idx, char zname[AT_LEAST 32])
{
    if (laynum > 9 ) return NULL;
    char txt[16];
    zname[0] = !laynum?'\0': 'A'+laynum-1 ;
    zname[1] = '\0';
    lstrcat_sA(zname, 32, "Zone");
    lstrcat_sA(zname, 32, Uint2lStrA(txt, idx)); // Zone Name from zone number

    return zname;
}

// Map action string to actual action enum
static enum action MapActionW(const TCHAR *txt)
{
    #define ACVALUE(a, b, c) (b),
    static const char *action_map[] = { ACTION_MAP };
    #undef ACVALUE
    UCHAR ac;
    for (ac=0; ac < ARR_SZ(action_map); ac++) {
        if(!strtotcharicmp(txt, action_map[ac]))
            return (enum action)ac;
    }
    if (txt[0] == 'S' && txt[1] == 'h' && txt[2] == 'r' && txt[3] == 't'
    &&  txt[4] <= '9' && txt[4] >= '0') {
        return (enum action)(AC_SHRT0 + txt[4] - '0');
    }
    return AC_NONE;
}

#endif /* ALTDRAG_RPC_H */
