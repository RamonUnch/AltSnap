/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2022                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"
static void MoveWindowAsync(HWND hwnd, int x, int y, int w, int h);
static BOOL CALLBACK EnumMonitorsProc(HMONITOR, HDC, LPRECT , LPARAM );
static LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Timer messages
#define REHOOK_TIMER    WM_APP+1
#define SPEED_TIMER     WM_APP+2
#define GRAB_TIMER      WM_APP+3
//#define ALTUP_TIMER     WM_APP+4
#define POOL_TIMER      WM_APP+5

// #define NO_HOOK_LL

#define CURSOR_ONLY 66
#define NOT_MOVED 33
#define RESET_OFFSET 22
#define DRAG_WAIT 77
// Number of actions per button!
//  2 for Alt+Clikc
// +2 for Titlebar action
// +2 for Action while moving
// +2 for action while resizing
#define NACPB 8

#define STACK 0x1000

static HWND g_transhwnd[4]; // 4 windows to make a hollow window
static HWND g_timerhwnd;    // For various timers
static HWND g_mchwnd;       // For the Action menu messages
static HWND g_hkhwnd;       // For the hotkeys message window.

static void UnhookMouse();
static void HookMouse();
static void UnhookMouseOnly();
static HWND KreateMsgWin(WNDPROC proc, const TCHAR *name, LONG_PTR userdata);

// Enumerators
enum button { BT_NONE=0, BT_LMB=0x02, BT_RMB=0x03, BT_MMB=0x04, BT_MB4=0x05
            , BT_MB5=0x06,  BT_MB6=0x07,  BT_MB7=0x08,  BT_MB8=0x09
            , BT_MB9=0x0A,  BT_MB10=0x0B, BT_MB11=0x0C, BT_MB12=0x0D
            , BT_MB13=0x0E, BT_MB14=0x0F, BT_MB15=0x10, BT_MB16=0x11
            , BT_MB17=0x12, BT_MB18=0x13, BT_MB19=0x14, BT_MB20=0x15
            , BT_WHEEL=0x16, BT_HWHEEL=0x17 };
enum resizeX { RZ_XNONE=0, RZ_LEFT=1, RZ_RIGHT= 2, RZ_XCENTER=3 };
enum resizeY { RZ_YNONE=0, RZ_TOP= 1, RZ_BOTTOM=2, RZ_YCENTER=3 };
enum buttonstate {STATE_NONE, STATE_DOWN, STATE_UP};

#define BT_PROBE (1<<16)

static int init_movement_and_actions(POINT pt, HWND hwnd, enum action action, int button);
static void FinishMovement();
static void MoveTransWin(int x, int y, int w, int h);

static struct windowRR {
    HWND hwnd;
    int x;
    int y;
    int width;
    int height;
    UINT odpi;
    UCHAR end;
    UCHAR moveonly;
    UCHAR maximize;
    UCHAR snap;
} LastWin;

struct resizeXY {
    enum resizeX x;
    enum resizeY y;
};
static const struct resizeXY AUTORESIZE =   {RZ_XNONE, RZ_YNONE};
// State
static struct {
    struct {
        POINT Min;
        POINT Max;
    } mmi;
    POINT clickpt;
    POINT prevpt;
    POINT ctrlpt;
    POINT shiftpt;
    POINT offset;
    POINT mdipt;

    HWND hwnd;
    HWND sclickhwnd;
    HWND mdiclient;
    DWORD clicktime;
    short hittest;
    short delta;
    unsigned Speed;
    HMENU unikeymenu;
    volatile LONG ignorekey;
    volatile LONG ignoreclick;

    UCHAR alt;
    UCHAR alt1;
    UCHAR blockaltup;
    UCHAR ctrl;

    UCHAR shift;
    UCHAR snap;
    UCHAR altsnaponoff;
    UCHAR moving;

    UCHAR blockmouseup;
    UCHAR fwmouseup;
    UCHAR enumed;
    UCHAR usezones;

    UCHAR clickbutton;
    UCHAR resizable;
    struct {
        UCHAR maximized;
        UCHAR fullscreen;
        RECT mon;
        HMONITOR monitor;
        int width;
        int height;
        int right;
        int bottom;
        UINT dpi;
    } origin;

    UCHAR sactiondone;
    UCHAR xxbutton;
    UCHAR ignorept;
    enum action action;
    struct resizeXY resize;
} state;

// Snap
RECT *monitors = NULL;
unsigned nummonitors = 0;
RECT *wnds = NULL;
unsigned numwnds = 0;
HWND *hwnds = NULL;
unsigned numhwnds = 0;

// Settings
#define MAXKEYS 15
static struct config {
    // System settings
    short dragXth;
    short dragYth;
    short dbclickX;
    short dbclickY;
    // [General]
    UCHAR AutoFocus;
    UCHAR AutoSnap;
    UCHAR Aero;
    UCHAR SmartAero;
    UCHAR StickyResize;
    UCHAR InactiveScroll;
    UCHAR MDI;
    UCHAR ResizeCenter;
    UCHAR CenterFraction;
    UCHAR SidesFraction;
    UCHAR AVoff;
    UCHAR AHoff;
    UCHAR MoveTrans;
    UCHAR MMMaximize;
    // [Advanced]
    UCHAR ResizeAll;
    UCHAR FullScreen;
    UCHAR BLMaximized;
    UCHAR AutoRemaximize;
    UCHAR SnapThreshold;
    UCHAR AeroThreshold;
    UCHAR KBMoveStep;
    UCHAR KBMoveSStep;
    UCHAR AeroTopMaximizes;
    UCHAR UseCursor;
    UCHAR MinAlpha;
    char AlphaDelta;
    char AlphaDeltaShift;
    UCHAR ZoomFrac;
    UCHAR ZoomFracShift;
    UCHAR NumberMenuItems;
    UCHAR MaxMenuWidth;
    UCHAR AeroSpeedTau;
    char SnapGap;
    UCHAR ShiftSnaps;
    UCHAR PiercingClick;
    UCHAR DragSendsAltCtrl;
    UCHAR TopmostIndicator;
    UCHAR RCCloseMItem;
    UCHAR MaxKeysNum;
    UCHAR DragThreshold;
    UCHAR AblockHotclick;
    UCHAR MenuShowOffscreenWin;
    UCHAR MenuShowEmptyLabelWin;
    UCHAR IgnoreMinMaxInfo;
    // [Performance]
    UCHAR FullWin;
    UCHAR TransWinOpacity;
    UCHAR RefreshRate;
    UCHAR RezTimer;
    UCHAR PinRate;
    UCHAR MoveRate;
    UCHAR ResizeRate;
    // [Input]
    UCHAR TTBActions;
    UCHAR KeyCombo;
    UCHAR ScrollLockState;
    UCHAR LongClickMove;
    UCHAR UniKeyHoldMenu;
    // [Zones]
    UCHAR UseZones;
    UCHAR ShowZonesPrevw;
    UCHAR ZonesPrevwOpacity;
    UCHAR ZSnapMode;
    UCHAR LayoutNumber;
    char InterZone;
  # ifdef WIN64
    UCHAR FancyZone;
  #endif
    // [KBShortcuts]
    UCHAR UsePtWindow;
    // -- -- -- -- -- -- --
    UCHAR keepMousehook;
    UCHAR EndSendKey; // Used to be VK_CONTROL
    WORD AeroMaxSpeed;
    WORD LongClickMoveDelay;
    DWORD BLCapButtons;
    DWORD BLUpperBorder;
    int PinColor;

    UCHAR Hotkeys[MAXKEYS+1];
    UCHAR Shiftkeys[MAXKEYS+1];
    UCHAR Hotclick[MAXKEYS+1];
    UCHAR Killkey[MAXKEYS+1];
    UCHAR XXButtons[MAXKEYS+1];
    UCHAR ModKey[MAXKEYS+1];
    UCHAR HScrollKey[MAXKEYS+1];
    UCHAR ESCkeys[MAXKEYS+1];

    struct {
        enum action // Up to 20 BUTTONS!!!
          LMB[NACPB],  RMB[NACPB],  MMB[NACPB],  MB4[NACPB],  MB5[NACPB]
        , MB6[NACPB],  MB7[NACPB],  MB8[NACPB],  MB9[NACPB],  MB10[NACPB]
        , MB11[NACPB], MB12[NACPB], MB13[NACPB], MB14[NACPB], MB15[NACPB]
        , MB16[NACPB], MB17[NACPB], MB18[NACPB], MB19[NACPB], MB20[NACPB]
        , Scroll[NACPB], HScroll[NACPB]; // Plus vertical and horizontal wheels
    } Mouse;
    enum action GrabWithAlt[NACPB]; // Actions without click
    enum action MoveUp[NACPB];      // Actions on (long) Move Up w/o drag
    enum action ResizeUp[NACPB];    // Actions on (long) Resize Up w/o drag

    UCHAR *inputSequences[AC_SHRTF-AC_SHRT0]; // 36
} conf;

struct OptionListItem {
   const char *name; int def;
};
// [General]
static const struct OptionListItem General_uchars[] = {
    { "AutoFocus", 0 },
    { "AutoSnap", 0 },
    { "Aero", 1 },
    { "SmartAero", 1 },
    { "StickyResize", 0 },
    { "InactiveScroll", 0 },
    { "MDI", 0 },
    { "ResizeCenter", 1 },
    { "CenterFraction", 24 },
    { "SidesFraction", 255 },
    { "AeroHoffset", 50 },
    { "AeroVoffset", 50 },
    { "MoveTrans", 255 },
    { "MMMaximize", 1 },
};
// [Advanced]
static const struct OptionListItem Advanced_uchars[] = {
    { "ResizeAll", 1 },
    { "FullScreen", 1 },
    { "BLMaximized", 0 },
    { "AutoRemaximize", 0 },
    { "SnapThreshold", 20 },
    { "AeroThreshold", 5 },
    { "KBMoveStep", 100 },
    { "KBMoveSStep", 10 },
    { "AeroTopMaximizes", 1 },
    { "UseCursor", 1 },
    { "MinAlpha", 32 },
    { "AlphaDelta", 64 },
    { "AlphaDeltaShift", 8 },
    { "ZoomFrac", 16 },
    { "ZoomFracShift", 64 },
    { "NumberMenuItems", 0},
    { "MaxMenuWidth", 80},
    { "AeroSpeedTau", 64 },
    { "SnapGap", 0 },
    { "ShiftSnaps", 1 },
    { "PiercingClick", 0 },
    { "DragSendsAltCtrl", 0 },
    { "TopmostIndicator", 0 },
    { "RCCloseMItem", 1 },
    { "MaxKeysNum", 0 },
    { "DragThreshold", 1 },
    { "AblockHotclick", 0 },
    { "MenuShowOffscreenWin", 0 },
    { "MenuShowEmptyLabelWin", 0 },
    { "IgnoreMinMaxInfo", 0 },
};
// [Performance]
static const struct OptionListItem Performance_uchars[] = {
    { "FullWin", 2 },
    { "TransWinOpacity", 0 },
    { "RefreshRate", 0 },
    { "RezTimer", 0 },
    { "PinRate", 32 },
    { "MoveRate", 2 },
    { "ResizeRate", 4 },
};
// [Input]
static const struct OptionListItem Input_uchars[] = {
    { "TTBActions", 0 },
    { "KeyCombo", 0 },
    { "ScrollLockState", 0 },
    { "LongClickMove", 0 },
    { "UniKeyHoldMenu", 0 },
};
// [Zones]
static const struct OptionListItem Zones_uchars[] = {
    { "UseZones", 0 },
    { "ShowZonesPrevw", 1 },
    { "ZonesPrevwOpacity", 161 },
    { "ZSnapMode", 0 },
    { "LayoutNumber", 0 },
    { "InterZone", 32 },
  # ifdef WIN64
    { "FancyZone", 0 },
  # endif
};

// Blacklist (dynamically allocated)
struct blacklistitem {
    const TCHAR *title;
    const TCHAR *classname;
    const TCHAR *exename;
};
struct blacklist {
    struct blacklistitem *items;
    unsigned length;
    TCHAR *data;
};
static struct {
    struct blacklist Processes;
    struct blacklist Windows;
    struct blacklist Snaplist;
    struct blacklist MDIs;
    struct blacklist Pause;
    struct blacklist MMBLower;
    struct blacklist Scroll;
    struct blacklist IScroll;
    struct blacklist AResize;
    struct blacklist SSizeMove;
    struct blacklist NCHittest;
    struct blacklist Bottommost;
} BlkLst;
// MUST MATCH THE ABOVE!!!
static const char *BlackListStrings[] = {
    "Processes", // Max length is 15 char + NULL
    "Windows",
    "Snaplist",
    "MDIs",
    "Pause",
    "MMBLower",
    "Scroll",
    "IScroll",
    "AResize",
    "SSizeMove",
    "NCHittest",
    "Bottommost"
};
// Cursor data
HWND g_mainhwnd = NULL;

// Hook data
HINSTANCE hinstDLL = NULL;
HHOOK mousehook = NULL;

#define FixDWMRect(hwnd, rect) FixDWMRectLL(hwnd, rect, conf.SnapGap)
#undef GetWindowRectL
#define GetWindowRectL(hwnd, rect) GetWindowRectLL(hwnd, rect, conf.SnapGap)

// To clamp width and height of windows
static pure int CLAMPW(int width)  { return CLAMP(state.mmi.Min.x, width,  state.mmi.Max.x); }
static pure int CLAMPH(int height) { return CLAMP(state.mmi.Min.y, height, state.mmi.Max.y); }
static pure int ISCLAMPEDW(int x)  { return state.mmi.Min.x <= x && x <= state.mmi.Max.x; }
static pure int ISCLAMPEDH(int y)  { return state.mmi.Min.y <= y && y <= state.mmi.Max.y; }

/* If pt and ptt are it is the same points with 4px tolerence */
static xpure int IsSamePTT(const POINT *pt, const POINT *ptt)
{
    const short Tx = conf.dbclickX;
    const short Ty = conf.dbclickY;
    return !( pt->x > ptt->x+Tx || pt->y > ptt->y+Ty || pt->x < ptt->x-Tx || pt->y < ptt->y-Ty );
}

static xpure int IsPtDragOut(const POINT *pt, const POINT *ptt)
{
    const short Tx = conf.dragXth;
    const short Ty = conf.dragYth;
    return !( pt->x > ptt->x+Tx || pt->y > ptt->y+Ty || pt->x < ptt->x-Tx || pt->y < ptt->y-Ty );
}

// Specific includes
#include "snap.c"
#include "zones.c"

/////////////////////////////////////////////////////////////////////////////
// Wether a window is present or not in a blacklist
static pure int blacklisted(HWND hwnd, const struct blacklist *list)
{
    // Null hwnd or empty list
    if (!hwnd || !list->length || !list->items)
        return 0;
    // If the first element is *:*|* (NULL:NULL|NULL)then we are in whitelist mode
    // mode = 1 => blacklist, mode = 0 => whitelist;
    UCHAR mode = list->items[0].classname
              || list->items[0].title
              || list->items[0].exename;
    unsigned i = !mode; // Skip the first item...

    TCHAR title[256], classname[256];
    TCHAR exename[MAX_PATH];
    title[0] = classname[0] = exename[0] = '\0';
    GetWindowText(hwnd, title, ARR_SZ(title));
    GetClassName(hwnd, classname, ARR_SZ(classname));
    GetWindowProgName(hwnd, exename, ARR_SZ(exename));

    for ( ; i < list->length; i++) {
        if (!lstrcmp_star(classname, list->items[i].classname)
        &&  !lstrcmp_star(title, list->items[i].title)
        && (!list->items[i].exename || !lstrcmpi(exename, list->items[i].exename))
        ) {
            return mode;
        }
    }
    return !mode;
}

static int isClassName(HWND hwnd, const TCHAR *str)
{
    TCHAR classname[256];
    return GetClassName(hwnd, classname, ARR_SZ(classname))
        && !lstrcmp(classname, str);
}
/////////////////////////////////////////////////////////////////////////////
// The second bit (&2) will always correspond to the WS_THICKFRAME flag
static int pure IsResizable(HWND hwnd)
{
    int thickf = !!(GetWindowLongPtr(hwnd, GWL_STYLE)&WS_THICKFRAME);
    int ret =  conf.ResizeAll // bit two is the real thickframe state.
            | thickf | (thickf<<1);

    if (!ret) ret = !!GetBorderlessFlag(hwnd);
    if (!ret) ret = !!blacklisted(hwnd, &BlkLst.AResize); // Always resize list

    return ret;
}
static HMONITOR GetMonitorInfoFromWin(HWND hwnd, MONITORINFO *mi)
{
    mi->cbSize = sizeof(MONITORINFO);
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hmon, mi);
    return hmon;
}
/////////////////////////////////////////////////////////////////////////////
static BOOL IsFullscreen(HWND hwnd)
{
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    if ((style&WS_CAPTION) != WS_CAPTION) {
        RECT rc;
        if (GetWindowRect(hwnd, &rc)) {
            MONITORINFO mi;
            GetMonitorInfoFromWin(hwnd, &mi);
            return EqualRect(&rc, &mi.rcMonitor);
        }
    }
    return FALSE;
}
static int IsFullscreenF(HWND hwnd, const RECT *wnd, const RECT *fmon)
{
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    // No caption and fullscreen window
    return ((style&WS_CAPTION) != WS_CAPTION)
        && EqualRect(wnd, fmon);
}
static int IsFullScreenBL(HWND hwnd)
{
    return conf.FullScreen && IsFullscreen(hwnd);
}
/////////////////////////////////////////////////////////////////////////////
// WM_ENTERSIZEMOVE or WM_EXITSIZEMOVE...
static void SendSizeMove(DWORD msg)
{
//    LockWindowUpdate(WM_ENTERSIZEMOVE?state.hwnd:NULL);
    // Don't send WM_ENTER/EXIT SIZEMOVE if the window is in SSizeMove BL
    if(!blacklisted(state.hwnd, &BlkLst.SSizeMove)) {
        PostMessage(state.hwnd, msg, 0, 0);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Overloading of the Hittest function to include a whitelist
// x and y are in screen coordinate.
static int HitTestTimeoutbl(HWND hwnd, POINT pt)
{
    DorQWORD area=0;

    // Try first with the ancestor window for some buggy AppX?
    HWND ancestor = GetAncestor(hwnd, GA_ROOT);
    if (blacklisted(ancestor, &BlkLst.MMBLower)) return 0;
    if (hwnd != ancestor
    && blacklisted(ancestor, &BlkLst.NCHittest)) {
        SendMessageTimeout(ancestor, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y), SMTO_NORMAL, 200, &area);
        if(area == HTCAPTION) goto DOUBLECHECK_CAPTION;
    }
    area = HitTestTimeoutL(hwnd, MAKELPARAM(pt.x, pt.y));
    DOUBLECHECK_CAPTION:
    if (area == HTCAPTION) {
        // Double check that we are not inside one of the
        // caption buttons buttons because of buggy Win10..
        RECT buttonRc;
        if (GetCaptionButtonsRect(ancestor, &buttonRc) && PtInRect(&buttonRc, pt)) {
            // let us assume it is the minimize button, it makes no sence
            // But Windows is too buggy
            area = HTMINBUTTON;
        }
    }
    return area;
}
/////////////////////////////////////////////////////////////////////////////
//
static void GetMinMaxInfo(HWND hwnd, POINT *Min, POINT *Max)
{
    GetMinMaxInfoF(hwnd, Min, Max, conf.IgnoreMinMaxInfo);
}
/////////////////////////////////////////////////////////////////////////////
// Use NULL to restore old transparency.
// Set to -1 to clear old state
static void SetWindowTrans(HWND hwnd)
{
    static BYTE oldtrans;
    static HWND oldhwnd;
    if (conf.MoveTrans == 0 || conf.MoveTrans == 255) return;
    if (oldhwnd == hwnd) return; // Nothing to do
    if ((DorQWORD)hwnd == (DorQWORD)(-1)) {
        oldhwnd = NULL;
        oldtrans = 0;
        return;
    }

    if (hwnd && !oldtrans) {
        oldhwnd = hwnd;
        LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        if (exstyle&WS_EX_LAYERED) {
            BYTE ctrans=0;
            if(GetLayeredWindowAttributes(hwnd, NULL, &ctrans, NULL))
                if(ctrans) oldtrans = ctrans;
        } else {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle|WS_EX_LAYERED);
            oldtrans = 255;
        }
        SetLayeredWindowAttributes(hwnd, 0, conf.MoveTrans, LWA_ALPHA);
    } else if (!hwnd && oldhwnd) { // restore old trans;
        LONG_PTR exstyle = GetWindowLongPtr(oldhwnd, GWL_EXSTYLE);
        if (!oldtrans || oldtrans == 255) {
            SetWindowLongPtr(oldhwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
        } else {
            SetLayeredWindowAttributes(oldhwnd, 0, oldtrans, LWA_ALPHA);
        }
        oldhwnd = NULL;
        oldtrans = 0;
    }
}
static void *GetEnoughSpace(void *ptr, unsigned num, unsigned *alloc, size_t size)
{
    if (num >= *alloc) {
        void *nptr = realloc(ptr, (*alloc+4)*size);
        if (!nptr) { free(ptr);  *alloc=0; return NULL; }
        ptr = nptr;
        if(ptr) *alloc = (*alloc+4); // Realloc succeeded, increase count.
        else *alloc = 0;
    }
    return ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Enumerate callback proc
unsigned monitors_alloc = 0;
BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    // Make sure we have enough space allocated
    monitors = (RECT *)GetEnoughSpace(monitors, nummonitors, &monitors_alloc, sizeof(RECT));
    if (!monitors) return FALSE; // Stop enum, we failed
    // Add monitor
    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &mi);
    CopyRect(&monitors[nummonitors++], &mi.rcWork); //*lprcMonitor;

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
static void OffsetRectMDI(RECT *wnd)
{
    OffsetRect(wnd, -state.mdipt.x, -state.mdipt.y);
}
static int ShouldSnapTo(HWND hwnd)
{
    LONG_PTR style;
    return hwnd != state.hwnd
        && IsVisible(hwnd)
        && !IsIconic(hwnd)
        && !(GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_NOACTIVATE) // != WS_EX_NOACTIVATE
        &&( ((style=GetWindowLongPtr(hwnd, GWL_STYLE))&WS_CAPTION) == WS_CAPTION
           || (style&WS_THICKFRAME)
           || GetBorderlessFlag(hwnd)//&(WS_THICKFRAME|WS_CAPTION)
           || blacklisted(hwnd, &BlkLst.Snaplist)
        );
}
/////////////////////////////////////////////////////////////////////////////
unsigned wnds_alloc = 0;
BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    wnds = (RECT *)GetEnoughSpace(wnds, numwnds, &wnds_alloc, sizeof(RECT));
    if (!wnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized to taskbar,
    // not the window we are dragging and not blacklisted
    RECT wnd;
    if (ShouldSnapTo(window) && GetWindowRectL(window, &wnd)) {

        // Maximized?
        if (IsZoomed(window)) {
            // Skip maximized windows in MDI clients
            if (state.mdiclient) return TRUE;
            // Get monitor size
            MONITORINFO mi;
            GetMonitorInfoFromWin(window, &mi);
            // Crop this window so that it does not exceed the size of the monitor
            // This is done because when maximized, windows have an extra invisible
            // border (a border that stretches onto other monitors)
            CropRect(&wnd, &mi.rcWork);
        }
        OffsetRectMDI(&wnd);
        // Return if this window is overlapped by another window
        unsigned i;
        for (i=0; i < numwnds; i++) {
            if (RectInRect(&wnds[i], &wnd)) {
                return TRUE;
            }
        }
        // Add window to wnds db
        CopyRect(&wnds[numwnds++], &wnd);
    }
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// snapped windows database.
struct snwdata {
    RECT wnd;
    HWND hwnd;
    unsigned flag;
};
struct snwdata *snwnds;
unsigned numsnwnds = 0;
unsigned snwnds_alloc = 0;

BOOL CALLBACK EnumSnappedWindows(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    snwnds = (struct snwdata *)GetEnoughSpace(snwnds, numsnwnds, &snwnds_alloc, sizeof(struct snwdata));
    if (!snwnds) return FALSE; // Stop enum, we failed

    RECT wnd;
    if (ShouldSnapTo(hwnd)
    && !IsZoomed(hwnd)
    && GetWindowRectL(hwnd, &wnd)) {
        unsigned restore;

        if (conf.SmartAero&2 || IsWindowSnapped(hwnd)) {
            // In SMARTER snapping mode or if the WINDOW IS SNAPPED
            // We only consider the position of the window
            // to determine its snapping state
            MONITORINFO mi;
            GetMonitorInfoFromWin(hwnd, &mi);
            snwnds[numsnwnds].flag = WhichSideRectInRect(&mi.rcWork, &wnd);
        } else if ((restore = GetRestoreFlag(hwnd)) && restore&SNAPPED && restore&SNAPPEDSIDE) {
            // The window was AltSnapped...
            snwnds[numsnwnds].flag = restore;
        } else {
            // thiw window is not snapped.
            return TRUE; // next hwnd
        }
        // Add the window to the list
        OffsetRectMDI(&wnd);
        CopyRect(&snwnds[numsnwnds].wnd, &wnd);
        snwnds[numsnwnds].hwnd = hwnd;
        numsnwnds++;
    }
    return TRUE;
}
// If lParam is set to 1 then only windows that are
// touching the current window will be considered.
static void EnumSnapped()
{
    numsnwnds = 0;
    if (conf.SmartAero&1) {
        if(state.mdiclient)
            EnumChildWindows(state.mdiclient, EnumSnappedWindows, 0);
        else
            EnumDesktopWindows(NULL, EnumSnappedWindows, 0);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Uses the same DB than snapped windows db because they will never
// be used together Enum() vs EnumSnapped()
BOOL CALLBACK EnumTouchingWindows(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    snwnds = (struct snwdata *)GetEnoughSpace(snwnds, numsnwnds, &snwnds_alloc, sizeof(*snwnds));
    if (!snwnds) return FALSE; // Stop enum, we failed

    RECT wnd;
    if (ShouldSnapTo(hwnd)
    && !IsZoomed(hwnd)
    && IsResizable(hwnd)
    && !blacklisted(hwnd, &BlkLst.Windows)
    && GetWindowRectL(hwnd, &wnd)) {
        // Only considers windows that are
        // touching the currently resized window
        RECT statewnd;
        GetWindowRectL(state.hwnd, &statewnd);
        unsigned flag = AreRectsTouchingT(&statewnd, &wnd, conf.SnapThreshold/2);
        if (flag) {
            OffsetRectMDI(&wnd);

            // Return if this window is overlapped by another window
            unsigned i;
            for (i=0; i < numsnwnds; i++) {
                if (RectInRect(&snwnds[i].wnd, &wnd)) {
                    return TRUE;
                }
            }

            CopyRect(&snwnds[numsnwnds].wnd, &wnd);
            snwnds[numsnwnds].flag = flag;
            snwnds[numsnwnds].hwnd = hwnd;
            numsnwnds++;
        }
    }
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
//
static DWORD WINAPI EndDeferWindowPosThread(LPVOID hwndSS)
{
    EndDeferWindowPos(hwndSS);
    if (conf.RefreshRate) Sleep(conf.RefreshRate);
    LastWin.hwnd = NULL;
    return TRUE;
}
static void EndDeferWindowPosAsync(HDWP hwndSS)
{
    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, EndDeferWindowPosThread, hwndSS, STACK_SIZE_PARAM_IS_A_RESERVATION, &lpThreadId));
}
static int ShouldResizeTouching()
{
    return state.action == AC_RESIZE
        && ( (conf.StickyResize&1 && state.shift)
          || ((conf.StickyResize&3)==2 && !state.shift)
        );
}
static void EnumOnce(RECT **bd);
static int ResizeTouchingWindows(LPVOID lwptr)
{
    if (!ShouldResizeTouching()) return 0;
    RECT *bd;
    EnumOnce(&bd);
    if (!snwnds || !numsnwnds) return 0;
    struct windowRR *lw = (struct windowRR *)lwptr;
    // posx, posy,  correspond to the VISIBLE rect
    // of the current window...
    int posx = lw->x + bd->left;
    int posy = lw->y + bd->top;
    int width = lw->width - (bd->left+bd->right);
    int height = lw->height - (bd->top+bd->bottom);

    HDWP hwndSS = NULL; // For DeferwindowPos.
    if (conf.FullWin) {
        hwndSS = BeginDeferWindowPos(numsnwnds+1);
    }
    unsigned i;
    for (i=0; i < numsnwnds; i++) {
        RECT *nwnd = &snwnds[i].wnd;
        unsigned flag = snwnds[i].flag;
        HWND hwnd = snwnds[i].hwnd;

        POINT tpt;
        tpt.x = (nwnd->left+nwnd->right)/2;
        tpt.y = (nwnd->top+nwnd->bottom)/2 ;
        if(!PtInRect(&state.origin.mon, tpt))
            continue;

        if (PureLeft(flag)) {
            nwnd->right = posx;
        } else if (PureRight(flag)) {
            POINT Min, Max;
            GetMinMaxInfo(hwnd, &Min, &Max);
            nwnd->left = CLAMP(nwnd->right-Max.x, posx + width, nwnd->right-Min.x);
        } else if (PureTop(flag)) {
            nwnd->bottom = posy;
        } else if (PureBottom(flag)) {
            POINT Min, Max;
            GetMinMaxInfo(hwnd, &Min, &Max);
            nwnd->top = CLAMP(nwnd->bottom-Max.x, posy + height, nwnd->bottom-Min.x);
        } else {
            continue;
        }
        if (hwndSS) {
            RECT nbd;
            FixDWMRect(hwnd, &nbd);
            hwndSS = DeferWindowPos(hwndSS, hwnd, NULL
                    , nwnd->left - nbd.left
                    , nwnd->top - nbd.top
                    , nwnd->right - nwnd->left + nbd.left + nbd.right
                    , nwnd->bottom - nwnd->top + nbd.top + nbd.bottom
                    , SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
        }
        snwnds[i].flag = flag|TORESIZE;
    }

    if (hwndSS) {
        // Draw changes ONLY if full win is ON,
        hwndSS = DeferWindowPos(hwndSS, state.hwnd, NULL
                  , lw->x, lw->y, lw->width, lw->height
                  , SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
        if(hwndSS) EndDeferWindowPosAsync(hwndSS);
    }
    return 1;
}
/////////////////////////////////////////////////////////////////////////////
static void ResizeAllSnappedWindowsAsync()
{
    if (!conf.StickyResize || !numsnwnds) return;

    HDWP hwndSS = BeginDeferWindowPos(numsnwnds+1);
    unsigned i;
    for (i=0; i < numsnwnds; i++) {
        if(hwndSS && snwnds[i].flag&TORESIZE) {
            RECT bd;
            FixDWMRect(snwnds[i].hwnd, &bd);
            InflateRectBorder(&snwnds[i].wnd, &bd);
            hwndSS = DeferWindowPos(hwndSS, snwnds[i].hwnd, NULL
                    , snwnds[i].wnd.left
                    , snwnds[i].wnd.top
                    , snwnds[i].wnd.right - snwnds[i].wnd.left
                    , snwnds[i].wnd.bottom - snwnds[i].wnd.top
                    , SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
        }
    }
    if (hwndSS)
        hwndSS = DeferWindowPos(hwndSS, LastWin.hwnd, NULL
               , LastWin.x, LastWin.y, LastWin.width, LastWin.height
               , SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
    if(hwndSS) EndDeferWindowPosAsync(hwndSS);
    LastWin.hwnd = NULL;
}

///////////////////////////////////////////////////////////////////////////
// Just used in Enum
static void EnumMdi()
{
    // Make sure we have enough space allocated
    monitors = (RECT *)GetEnoughSpace(monitors, nummonitors, &monitors_alloc, sizeof(RECT));
    if (!monitors) return; // Fail

    // Add MDIClient as the monitor
    nummonitors = !!GetClientRect(state.mdiclient, &monitors[0]);

    if (state.snap > 1) {
        EnumChildWindows(state.mdiclient, EnumWindowsProc, 0);
    }
    if (conf.StickyResize) {
        EnumChildWindows(state.mdiclient, EnumTouchingWindows, 0);
    }
}
///////////////////////////////////////////////////////////////////////////
// Enumerate all monitors/windows/MDI depending on state.
static void Enum()
{
    nummonitors = 0;
    numwnds = 0;
    numsnwnds = 0;

    // MDI
    if (state.mdiclient && IsWindow(state.mdiclient)) {
        EnumMdi();
        return;
    }

    // Enumerate monitors
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, 0);

    // Enumerate windows
    if (state.snap >= 2) {
        EnumDesktopWindows(NULL, EnumWindowsProc, 0);
    }

    if (conf.StickyResize) {
        EnumDesktopWindows(NULL, EnumTouchingWindows, 0);
    }
}
///////////////////////////////////////////////////////////////////////////
//
#define RECALC_INVISIBLE_BORDERS ((RECT **)1)
static void EnumOnce(RECT **bd)
{
    static RECT borders;
    if (bd == RECALC_INVISIBLE_BORDERS) {
        FixDWMRect(state.hwnd, &borders);
        return;
    }
    if (bd && !(state.enumed&1)) {
        // LOGA("Enum");
        Enum(); // Enumerate monitors and windows
        FixDWMRect(state.hwnd, &borders);
        state.enumed |= 1;
        *bd = &borders;
    } else if (bd && state.enumed) {
        *bd = &borders;

    }
}
static void EnumSnappedOnce()
{
    if (!(state.enumed&2)) {
        // LOGA("EnumSnapped");
        EnumSnapped();
        state.enumed |= 2;
    }
}
///////////////////////////////////////////////////////////////////////////
void MoveSnap(int *_posx, int *_posy, int wndwidth, int wndheight, UCHAR pth)
{
    RECT *bd;
    if (!state.snap || state.Speed > conf.AeroMaxSpeed) return;
    EnumOnce(&bd);

    int posx = *_posx + bd->left;
    int posy = *_posy + bd->top;
    wndwidth  -= bd->left + bd->right;
    wndheight -= bd->top + bd->bottom;

    // thresholdx and thresholdy will shrink to make sure
    // the dragged window will snap to the closest windows
    int stickx=0, sticky=0;
    short thresholdx, thresholdy;
    UCHAR stuckx=0, stucky=0;
    thresholdx = thresholdy = pth; // conf.SnapThreshold;

    // Loop monitors and windows
    unsigned i, j;
    for (i=0, j=0; i < nummonitors || j < numwnds; ) {
        RECT snapwnd;
        UCHAR snapinside=0;

        // Get snapwnd
        if (monitors && i < nummonitors) {
            snapwnd = monitors[i];
            snapinside = 1;
            i++;
        } else if (wnds && j < numwnds) {
            snapwnd = wnds[j];
            snapinside = (state.snap != 2);
            j++;
        } else {
            // No monitors and no windows to snap to.
            return;
        }

        // Check if posx snaps
        if (IsInRangeT(posy, snapwnd.top, snapwnd.bottom, thresholdx)
        ||  IsInRangeT(snapwnd.top, posy, posy+wndheight, thresholdx)) {
            UCHAR snapinside_cond = (snapinside
                                  || posy + wndheight - thresholdx < snapwnd.top
                                  || snapwnd.bottom < posy + thresholdx);
            if (IsEqualT(snapwnd.right, posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's right edge
                stuckx = 1;
                stickx = snapwnd.right;
                thresholdx = snapwnd.right-posx;
            } else if (snapinside_cond && IsEqualT(snapwnd.right, posx+wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's right edge
                stuckx = 1;
                stickx = snapwnd.right - wndwidth;
                thresholdx = snapwnd.right-(posx+wndwidth);
            } else if (snapinside_cond && IsEqualT(snapwnd.left, posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's left edge
                stuckx = 1;
                stickx = snapwnd.left;
                thresholdx = snapwnd.left-posx;
            } else if (IsEqualT(snapwnd.left, posx+wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's left edge
                stuckx = 1;
                stickx = snapwnd.left - wndwidth;
                thresholdx = snapwnd.left-(posx+wndwidth);
            }
        }// end if posx snaps

        // Check if posy snaps
        if (IsInRangeT(posx, snapwnd.left, snapwnd.right, thresholdy)
        ||  IsInRangeT(snapwnd.left, posx, posx+wndwidth, thresholdy)) {
            UCHAR snapinside_cond = (snapinside || posx + wndwidth - thresholdy < snapwnd.left
                                  || snapwnd.right < posx+thresholdy);
            if (IsEqualT(snapwnd.bottom, posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's bottom edge
                stucky = 1;
                sticky = snapwnd.bottom;
                thresholdy = snapwnd.bottom-posy;
            } else if (snapinside_cond && IsEqualT(snapwnd.bottom, posy+wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's bottom edge
                stucky = 1;
                sticky = snapwnd.bottom - wndheight;
                thresholdy = snapwnd.bottom-(posy+wndheight);
            } else if (snapinside_cond && IsEqualT(snapwnd.top, posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's top edge
                stucky = 1;
                sticky = snapwnd.top;
                thresholdy = snapwnd.top-posy;
            } else if (IsEqualT(snapwnd.top, posy+wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's top edge
                stucky = 1;
                sticky = snapwnd.top-wndheight;
                thresholdy = snapwnd.top-(posy+wndheight);
            }
        } // end if posy snaps
    } // end for

    // Update posx and posy
    if (stuckx) {
        *_posx = stickx - bd->left;
    }
    if (stucky) {
        *_posy = sticky - bd->top;
    }
}

///////////////////////////////////////////////////////////////////////////
static void ResizeSnap(int *posx, int *posy, int *wndwidth, int *wndheight, UCHAR pthx, UCHAR pthy)
{
    if(!state.snap || state.Speed > conf.AeroMaxSpeed) return;

    // thresholdx and thresholdy will shrink to make sure
    // the dragged window will snap to the closest windows
    short thresholdx, thresholdy;
    UCHAR stuckleft=0, stucktop=0, stuckright=0, stuckbottom=0;
    int stickleft=0, sticktop=0, stickright=0, stickbottom=0;
    thresholdx = pthx;
    thresholdy = pthy;
    RECT *borders;
    EnumOnce(&borders);

    // Loop monitors and windows
    unsigned i, j;
    for (i=0, j=0; i < nummonitors || j < numwnds;) {
        RECT snapwnd;
        UCHAR snapinside;

        // Get snapwnd
        if (monitors && i < nummonitors) {
            CopyRect(&snapwnd, &monitors[i]);
            snapinside = 1;
            i++;
        } else if (wnds && j < numwnds) {
            CopyRect(&snapwnd, &wnds[j]);
            snapinside = (state.snap != 2);
            j++;
        } else {
            // nothing to snap to.
            return;
        }

        // Check if posx snaps
        if (IsInRangeT(*posy, snapwnd.top, snapwnd.bottom, thresholdx)
         || IsInRangeT(snapwnd.top, *posy, *posy + *wndheight, thresholdx)) {

            UCHAR snapinside_cond =  snapinside
                                 || (*posy+*wndheight-thresholdx < snapwnd.top)
                                 || (snapwnd.bottom < *posy+thresholdx) ;
            if (state.resize.x == RZ_LEFT
            && IsEqualT(snapwnd.right, *posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's right edge
                stuckleft = 1;
                stickleft = snapwnd.right;
                thresholdx = snapwnd.right - *posx;
            } else if (snapinside_cond && state.resize.x == RZ_RIGHT
            && IsEqualT(snapwnd.right, *posx+*wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's right edge
                stuckright = 1;
                stickright = snapwnd.right;
                thresholdx = snapwnd.right - (*posx + *wndwidth);
            } else if (snapinside_cond && state.resize.x == RZ_LEFT
            && IsEqualT(snapwnd.left, *posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's left edge
                stuckleft = 1;
                stickleft = snapwnd.left;
                thresholdx = snapwnd.left-*posx;
            } else if (state.resize.x == RZ_RIGHT
            && IsEqualT(snapwnd.left, *posx + *wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's left edge
                stuckright = 1;
                stickright = snapwnd.left;
                thresholdx = snapwnd.left - (*posx + *wndwidth);
            }
        }

        // Check if posy snaps
        if (IsInRangeT(*posx, snapwnd.left, snapwnd.right, thresholdy)
         || IsInRangeT(snapwnd.left, *posx, *posx+*wndwidth, thresholdy)) {

            UCHAR snapinside_cond = snapinside
                                 || (*posx+*wndwidth-thresholdy < snapwnd.left)
                                 || (snapwnd.right < *posx+thresholdy) ;
            if (state.resize.y == RZ_TOP
            && IsEqualT(snapwnd.bottom, *posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's bottom edge
                stucktop = 1;
                sticktop = snapwnd.bottom;
                thresholdy = snapwnd.bottom-*posy;
            } else if (snapinside_cond && state.resize.y == RZ_BOTTOM
            && IsEqualT(snapwnd.bottom, *posy + *wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's bottom edge
                stuckbottom = 1;
                stickbottom = snapwnd.bottom;
                thresholdy = snapwnd.bottom-(*posy+*wndheight);
            } else if (snapinside_cond && state.resize.y == RZ_TOP
            && IsEqualT(snapwnd.top, *posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's top edge
                stucktop = 1;
                sticktop = snapwnd.top;
                thresholdy = snapwnd.top-*posy;
            } else if (state.resize.y == RZ_BOTTOM
            && IsEqualT(snapwnd.top, *posy+*wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's top edge
                stuckbottom = 1;
                stickbottom = snapwnd.top;
                thresholdy = snapwnd.top-(*posy+*wndheight);
            }
        }
    } // end for

    // Update posx, posy, wndwidth and wndheight
    if (stuckleft) {
        *wndwidth = *wndwidth+*posx-stickleft + borders->left;
        *posx = stickleft - borders->left;
    }
    if (stucktop) {
        *wndheight = *wndheight+*posy-sticktop + borders->top;
        *posy = sticktop - borders->top;
    }
    if (stuckright) {
        *wndwidth = stickright-*posx + borders->right;
    }
    if (stuckbottom) {
        *wndheight = stickbottom-*posy + borders->bottom;
    }
}
/////////////////////////////////////////////////////////////////////////////
// Call with SW_MAXIMIZE or SW_RESTORE or below.
// Set origin&1 to set the restore position to the original dimentions
// set origin&2 for ASYNC window plamcemnt
#define SW_FULLSCREEN 28
static void MaximizeRestore_atpt(HWND hwnd, UINT sw_cmd, int origin)
{
    WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd != SW_FULLSCREEN)
        wndpl.showCmd = sw_cmd;

    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
    if(sw_cmd == SW_MAXIMIZE || sw_cmd == SW_FULLSCREEN) {
        HMONITOR wndmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        HMONITOR monitor = MonitorFromPoint(state.prevpt, MONITOR_DEFAULTTONEAREST);

        if (origin&1) {
            // set normal windpw plasement to origin.
            wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left + state.origin.width;
            wndpl.rcNormalPosition.bottom = wndpl.rcNormalPosition.top + state.origin.height;
        }

        GetMonitorInfo(monitor, &mi);

        // Center window on monitor, if needed
        if (monitor != wndmonitor) {
            CenterRectInRect(&wndpl.rcNormalPosition, &mi.rcWork);
        }
    }
    if (origin&2) wndpl.flags |= WPF_ASYNCWINDOWPLACEMENT;
    SetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd == SW_FULLSCREEN) {
        MoveWindowAsync(hwnd, mi.rcMonitor.left , mi.rcMonitor.top
                      , mi.rcMonitor.right-mi.rcMonitor.left
                      , mi.rcMonitor.bottom-mi.rcMonitor.top);
    }
}

static void MoveWindowAsync1(HWND hwnd, int x, int y, int w, int h)
{
    UINT flags = SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_ASYNCWINDOWPOS;
    if (conf.IgnoreMinMaxInfo) flags |= SWP_NOSENDCHANGING;
    SetWindowPos(hwnd, NULL, x, y, w, h, flags);
}
static void RestoreWindowToRect(HWND hwnd, const RECT *rc, UINT flags)
{
    RECT zbd, bd;
    FixDWMRect(hwnd, &zbd); // Zoomed invisible borders (that were applied)
    WINDOWPLACEMENT wndpl; wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);
    wndpl.showCmd = SW_RESTORE;
    wndpl.flags |= flags;
    CopyRect(&wndpl.rcNormalPosition, rc);
    if (LOBYTE(GetVersion()) >= 10) {
        // On Windows 10+ we got invisible borders...
        wndpl.flags &= ~WPF_ASYNCWINDOWPLACEMENT;
        // Synchronus restore because we have to check for Invisible
        // borders again that are different when Zoomed/restored.
        SetWindowPlacement(hwnd, &wndpl);
        FixDWMRect(hwnd, &bd); // Restored invisible borders
        if( !EqualRect(&zbd, &bd) ) {
            // Wrong invisible borders were applied,
            // correct it with an async move.
            #define r wndpl.rcNormalPosition
            DeflateRectBorder(&r, &zbd);
            InflateRectBorder(&r, &bd);
            MoveWindowAsync1(hwnd, r.left, r.top, r.right-r.left, r.bottom-r.top);
            #undef r
        }
    } else {
        SetWindowPlacement(hwnd, &wndpl);
    }
}
static void RestoreWindowTo(HWND hwnd, int x, int y, int w, int h)
{
    RECT rc = {x, y, x+w, y+h };
    RestoreWindowToRect(hwnd, &rc, 0);
}
/* Helper function to call SetWindowPos with the SWP_ASYNCWINDOWPOS flag
 * Also restores the window if needed.
 * Note that WPF_ASYNCWINDOWPLACEMENT was introduced with Windows 2000
 * but it seems not to be a problem for NT4, so it can be kept here. */
static void MoveWindowAsync(HWND hwnd, int x, int y, int w, int h)
{
    if (IsZoomed(hwnd) || IsWindowSnapped(hwnd)) {
        RECT rc = {x, y, x+w, y+h };
        RestoreWindowToRect(hwnd, &rc, WPF_ASYNCWINDOWPLACEMENT);
    } else {
        MoveWindowAsync1(hwnd, x, y, w, h);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Move the windows in a thread in case it is very slow to resize
static void MoveResizeWindowThread(struct windowRR *lw, UINT flag)
{
    HWND hwnd;
    hwnd = lw->hwnd;

    if (lw->end && conf.FullWin) Sleep(8); // At the End of movement...
    if (lw->end && !lw->maximize && (IsZoomed(hwnd) || IsWindowSnapped(hwnd))) {
        // Use Restore
        RestoreWindowTo(hwnd, lw->x, lw->y, lw->width, lw->height);
    } else {
//        PostMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(state.prevpt.x, state.prevpt.y));
//        if(!(flag&SWP_NOSIZE)) {
//            RECT rc = { lw->x, lw->y, lw->x + lw->width, lw->y + lw->height };
//            SendMessage(hwnd, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&rc);
//        }
        SetWindowPos(hwnd, NULL, lw->x, lw->y, lw->width, lw->height, flag);

        // Send WM_SYNCPAINT in case to wait for the end of movement
        // And to avoid windows to "slide through" the whole WM_MOVE queue
        if(flag&SWP_ASYNCWINDOWPOS) SendMessage(hwnd, WM_SYNCPAINT, 0, 0);
    if (conf.RefreshRate) ASleep(conf.RefreshRate); // Accurate!!!
    }
    lw->hwnd = NULL;
    lw->end = 0;
}

/* MOVEASYNC |SWP_DEFERERASE ??*/
#define RESIZEFLAG        SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE
#define MOVETHICKBORDERS  SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE
#define MOVEASYNC         SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS
static DWORD WINAPI MoveWindowThread(LPVOID LastWinV)
{
    struct windowRR *lw = (struct windowRR *)LastWinV;
//    RECT rc;
//    int notsamesize = 0;
//    if (GetWindowRect(lw->hwnd, &rc)) {
//        int cW = rc.right - rc.left;
//        int cH = rc.bottom - rc.top;
//        UINT cdpi = GetDpiForWindow(lw->hwnd);
//        if ( cdpi && cdpi != lw->odpi ) {
//            // If dpi is not the same we must check the *scaled* values.
//            if (cW == lw->width && cH == lw->height)
//                // Window had no time to resize between monitors?
//                notsamesize = 0;
//            else
//                notsamesize = (cW * lw->odpi)>>3 != (lw->width  * cdpi)>>3
//                       || (cH * lw->odpi)>>3 != (lw->height * cdpi)>>3;
//        } else {
//            notsamesize =  cW != lw->width ||  cH != lw->height;
//        }
//        //LOGA("MV: %d:%d/%d -> %d:%d/%d %s", cW, cH, cdpi , lw->width, lw->height, lw->odpi, notsamesize?"(dif)":"(eq)");
//    }
//    UINT flag = notsamesize? RESIZEFLAG: state.resizable&2 ? MOVETHICKBORDERS: MOVEASYNC;
    int notsamesize = 1;
    int nothingtodo = 0;
    RECT rc;
    if (conf.FullWin) {
        notsamesize = !lw->moveonly;
        if ( lw->odpi == GetDpiForWindow(lw->hwnd)
        && GetWindowRect(lw->hwnd, &rc) ) {
            int cW = rc.right - rc.left;
            int cH = rc.bottom - rc.top;
            nothingtodo =  rc.left == lw->x && rc.top == lw->y
                        && cW == lw->width &&  cH == lw->height;
        }

    } else {
        // Hollow rectangle mode.
        if( GetWindowRect(lw->hwnd, &rc) ) {
            int cW = rc.right - rc.left;
            int cH = rc.bottom - rc.top;
            notsamesize =  cW != lw->width ||  cH != lw->height;
        }
    }

    UINT flag = notsamesize? RESIZEFLAG: state.resizable&2 ? MOVETHICKBORDERS: MOVEASYNC;
    if (conf.IgnoreMinMaxInfo) flag |= SWP_NOSENDCHANGING;

    if (nothingtodo)
        lw->hwnd = NULL; // DONE!
    else
        MoveResizeWindowThread(lw, flag);

    return 0;
}
#undef RESIZEFLAG
#undef MOVETHICKBORDERS
#undef MOVEASYNC

static void MoveWindowInThread(struct windowRR *lw)
{
    DWORD lpThreadId;
    CloseHandle(
        CreateThread( NULL, STACK
            , MoveWindowThread
            , lw, STACK_SIZE_PARAM_IS_A_RESERVATION, &lpThreadId)
    );
}
///////////////////////////////////////////////////////////////////////////
// use snwnds[numsnwnds].wnd / .flag
static void GetAeroSnappingMetrics(int *leftWidth, int *rightWidth, int *topHeight, int *bottomHeight, const RECT *mon)
{
    *leftWidth    = CLAMPW((mon->right - mon->left)* conf.AHoff     /100);
    *rightWidth   = CLAMPW((mon->right - mon->left)*(100-conf.AHoff)/100);
    *topHeight    = CLAMPH((mon->bottom - mon->top)* conf.AVoff     /100);
    *bottomHeight = CLAMPH((mon->bottom - mon->top)*(100-conf.AVoff)/100);

    // do not go further is snapping state is toggled or shift is down.
    if (state.snap != conf.AutoSnap) return;
    EnumSnappedOnce();

    // Check on all the other snapped windows from the bottom most
    // To give precedence to the topmost windows
    unsigned i = numsnwnds;
    while (i--) {
        unsigned flag = snwnds[i].flag;
        const RECT *wnd = &snwnds[i].wnd;
        // if the window is in current monitor
        POINT tpt;
        tpt.x = (wnd->left+wnd->right)/2;
        tpt.y = (wnd->top+wnd->bottom)/2 ;
        if (PtInRect(mon, tpt)) {
            // We have a snapped window in the monitor
            if (flag & SNLEFT) {
                *leftWidth  = CLAMPW(wnd->right - wnd->left);
                *rightWidth = CLAMPW(mon->right - wnd->right);
            } else if (flag & SNRIGHT) {
                *rightWidth = CLAMPW(wnd->right - wnd->left);
                *leftWidth  = CLAMPW(wnd->left - mon->left);
            }
            if (flag & SNTOP) {
                *topHeight    = CLAMPH(wnd->bottom - wnd->top);
                *bottomHeight = CLAMPH(mon->bottom - wnd->bottom);
            } else if (flag & SNBOTTOM) {
                *bottomHeight = CLAMPH(wnd->bottom - wnd->top);
                *topHeight    = CLAMPH(wnd->top - mon->top);
            }
        }
    } // next i
}
///////////////////////////////////////////////////////////////////////////
static void GetMonitorRect(const POINT *pt, int full, RECT *_mon)
{
    if (state.mdiclient
    && GetClientRect(state.mdiclient, _mon)) {
        return; // MDI!
    }

    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(MonitorFromPoint(*pt, MONITOR_DEFAULTTONEAREST), &mi);

    CopyRect(_mon, full? &mi.rcMonitor : &mi.rcWork);
}
/////////////////////////////////////////////////////////////////////////
// also return monitor's DPI
//static UINT GetDPIMonitorRect(const POINT *pt, int full, RECT *_mon)
//{
//    UINT dpi = 0;
//    if (state.mdiclient
//    && GetClientRect(state.mdiclient, _mon)) {
//        return GetDpiForWindow(state.mdiclient); // MDI!
//    }
//
//    HMONITOR hmon = MonitorFromPoint(*pt, MONITOR_DEFAULTTONEAREST);
//    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
//    GetMonitorInfo(hmon, &mi);
//
//    UINT dpiX, dpiY;
//    if(S_OK == GetDpiForMonitorL(hmon, MDT_DEFAULT, &dpiX, &dpiY)) {
//        dpi = dpiX;
//    }
//
//    CopyRect(_mon, full? &mi.rcMonitor : &mi.rcWork);
//    return dpi;
//}

static void WaitMovementEnd()
{ // Only wait 64ms maximum
    if (conf.FullWin) {
        int i=0;
        while (LastWin.hwnd && i++ < 4) Sleep(16);
    }
    LastWin.hwnd = NULL; // Zero out in case.
}
///////////////////////////////////////////////////////////////////////////
#define AERO_TH conf.AeroThreshold
#define MM_THREAD_ON (LastWin.hwnd && conf.FullWin)
static int AeroMoveSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight)
{
    // return if last resizing is not finished or no Aero or not resizable.
    if((!conf.Aero && !(conf.UseZones&1)) || !state.resizable) return 0;

    LastWin.maximize = 0;
    LastWin.snap = 0;

    // We HAVE to check the monitor for each pt...
    RECT mon;
    GetMonitorRect(&pt, 0, &mon);

    int pLeft  = mon.left   + AERO_TH ;
    int pRight = mon.right  - AERO_TH ;
    int pTop   = mon.top    + AERO_TH ;
    int pBottom= mon.bottom - AERO_TH ;
    int leftWidth, rightWidth, topHeight, bottomHeight;

    int Left  = pLeft   + AERO_TH ;
    int Right = pRight  - AERO_TH ;
    int Top   = pTop    + AERO_TH ;
    int Bottom= pBottom - AERO_TH ;

    unsigned restore = GetRestoreFlag(state.hwnd);
    RECT trc;
    trc.left = pLeft; trc.top = pTop;
    trc.right = pRight; trc.bottom =pBottom;
    if (PtInRect(&trc, pt) || !conf.Aero) goto restore;

    GetAeroSnappingMetrics(&leftWidth, &rightWidth, &topHeight, &bottomHeight, &mon);
    LastWin.moveonly = 0; // We shall snap!
    // Move window
    if (pt.y < Top && pt.x < Left) {
        // Top left
        restore = SNAPPED|SNTOPLEFT;
        *wndwidth =  leftWidth;
        *wndheight = topHeight;
        *posx = mon.left;
        *posy = mon.top;
    } else if (pt.y < Top && Right < pt.x) {
        // Top right
        restore = SNAPPED|SNTOPRIGHT;
        *wndwidth = rightWidth;
        *wndheight = topHeight;
        *posx = mon.right-*wndwidth;
        *posy = mon.top;
    } else if (Bottom < pt.y && pt.x < Left) {
        // Bottom left
        restore = SNAPPED|SNBOTTOMLEFT;
        *wndwidth = leftWidth;
        *wndheight = bottomHeight;
        *posx = mon.left;
        *posy = mon.bottom - *wndheight;
    } else if (Bottom < pt.y && Right < pt.x) {
        // Bottom right
        restore = SNAPPED|SNBOTTOMRIGHT;
        *wndwidth = rightWidth;
        *wndheight= bottomHeight;
        *posx = mon.right - *wndwidth;
        *posy = mon.bottom - *wndheight;
    } else if (pt.y < pTop) {
        // Pure Top
        if (!state.shift ^ !(conf.AeroTopMaximizes&1)
        && (state.Speed < conf.AeroMaxSpeed)) {
             if (conf.FullWin) {
                MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE, 1);
                LastWin.hwnd = NULL;
                state.moving = 2;
                return 1;
            } else {
                *posx = mon.left;
                *posy = mon.top;
                *wndwidth = CLAMPW(mon.right - mon.left);
                *wndheight = CLAMPH( mon.bottom-mon.top );
                LastWin.maximize = 1;
                SetRestoreFlag(state.hwnd, SNAPPED|SNCLEAR); // To clear eventual snapping info
                return 0;
            }
        } else {
            restore = SNAPPED|SNTOP;
            *wndwidth = CLAMPW(mon.right - mon.left);
            *wndheight = topHeight;
            *posx = mon.left + (mon.right-mon.left)/2 - *wndwidth/2; // Center
            *posy = mon.top;
        }
    } else if (pt.y > pBottom) {
        // Pure Bottom
        restore = SNAPPED|SNBOTTOM;
        *wndwidth  = CLAMPW( mon.right-mon.left);
        *wndheight = bottomHeight;
        *posx = mon.left + (mon.right-mon.left)/2 - *wndwidth/2; // Center
        *posy = mon.bottom - *wndheight;
    } else if (pt.x < pLeft) {
        // Pure Left
        restore = SNAPPED|SNLEFT;
        *wndwidth = leftWidth;
        *wndheight = CLAMPH( mon.bottom-mon.top );
        *posx = mon.left;
        *posy = mon.top + (mon.bottom-mon.top)/2 - *wndheight/2; // Center
    } else if (pt.x > pRight) {
        // Pure Right
        restore = SNAPPED|SNRIGHT;
        *wndwidth =  rightWidth;
        *wndheight = CLAMPH( mon.bottom-mon.top );
        *posx = mon.right - *wndwidth;
        *posy = mon.top + (mon.bottom-mon.top)/2 - *wndheight/2; // Center
    } else {
        restore:
        if (restore&SNAPPED && !MM_THREAD_ON) {
            // Restore original window size
            // Clear restore data at the end of the movement
            LastWin.moveonly = 0;
            SetRestoreFlag(state.hwnd, restore|SNCLEAR);
            restore = 0;
            *wndwidth = state.origin.width;
            *wndheight = state.origin.height;
        }
    }

    // Aero-move the window?
    if (restore&SNAPPED) {
        *wndwidth  = CLAMPW(*wndwidth);
        *wndheight = CLAMPH(*wndheight);

        SetRestoreData(state.hwnd, state.origin.width, state.origin.height, restore);

        RECT borders;
        FixDWMRect(state.hwnd, &borders);
        *posx -= borders.left;
        *posy -= borders.top;
        *wndwidth += borders.left+borders.right;
        *wndheight+= borders.top+borders.bottom;

        // If we go too fast then do not move the window
        if (state.Speed > conf.AeroMaxSpeed) return 1;
        LastWin.moveonly = 0;
        if (conf.FullWin) {
            if (IsZoomed(state.hwnd)) {
                // Avoids flickering
                RestoreWindowTo(state.hwnd, *posx, *posy, *wndwidth, *wndheight);
                EnumOnce(RECALC_INVISIBLE_BORDERS);
            }
            int mmthreadend = !LastWin.hwnd;
            LastWin.hwnd = state.hwnd;
            LastWin.x = *posx;
            LastWin.y = *posy;
            LastWin.width = *wndwidth;
            LastWin.height = *wndheight;
            LastWin.snap = 1;
            if (mmthreadend) {
                MoveWindowInThread(&LastWin);
                return 1;
            }
        }
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
static void AeroResizeSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight)
{
    // return if last resizing is not finished
    if(!conf.Aero || MM_THREAD_ON || state.Speed > conf.AeroMaxSpeed)
        return;

    static RECT borders;
    if(!state.moving) {
        FixDWMRect(state.hwnd, &borders);
    }
    unsigned restore = GetRestoreFlag(state.hwnd);
    if (state.resize.x == RZ_XCENTER && state.resize.y == RZ_TOP && pt.y < state.origin.mon.top + AERO_TH) {
        restore = SNAPPED|SNMAXH;
        *wndheight = CLAMPH(state.origin.mon.bottom - state.origin.mon.top + borders.bottom + borders.top);
        *posy = state.origin.mon.top - borders.top;
    } else if (state.resize.x == RZ_LEFT && state.resize.y == RZ_YCENTER && pt.x < state.origin.mon.left + AERO_TH) {
        restore = SNAPPED|SNMAXW;
        *wndwidth = CLAMPW(state.origin.mon.right - state.origin.mon.left + borders.left + borders.right);
        *posx = state.origin.mon.left - borders.left;
    }
    // Aero-move the window?
    if (restore&SNAPPED && restore&(SNMAXH|SNMAXW)) {
        SetRestoreData(state.hwnd, state.origin.width, state.origin.height, restore);
    }
}
/////////////////////////////////////////////////////////////////////////////
static void HideCursor()
{
    // Reduce the size to 0 to avoid redrawing.
    SetWindowPos(g_mainhwnd, NULL, 0,0,0,0
        , SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE);
    ShowWindow(g_mainhwnd, SW_HIDE);
}
static pure int IsAKeyDown(const UCHAR *k)
{
    while (*k) {
        if(GetKeyState(*k++)&0x8000)
            return 1;
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Mod Key can return 0 or 1, maybe more in the future...
static pure int ModKey()
{
    return conf.ModKey[0]
        && IsAKeyDown(conf.ModKey);
}
///////////////////////////////////////////////////////////////////////////
// Get action of button
static enum action GetAction(const int button)
{
    if (button) { // Ugly pointer arithmetic (LMB <==> button == 2)
        return conf.Mouse.LMB[(button-2)*NACPB+ModKey()];
    } else {
        return AC_NONE;
    }
}
static enum action GetActionT(const int button)
{
    if (button) { // Ugly pointer arithmetic +2 compared to non titlebar
        return conf.Mouse.LMB[2+(button-2)*NACPB+ModKey()];
    } else {
        return AC_NONE;
    }
}
static enum action GetActionMR(const int button)
{
    if (button) {
        // Ugly pointer arithmetic
        // state.action == 1 or 2
        // MB[4/5] == Action/Alt while moving
        // MB[6/7] == Action/Alt while Resizing
        int offset = state.action<<1; // 2 or 4
        return conf.Mouse.LMB[2+offset+(button-2)*NACPB+ModKey()];
    } else {
        return AC_NONE;
    }
}
#define IsHotkey(a)   IsHotkeyy(a, conf.Hotkeys)
#define IsHotclick(a) IsHotkeyy(a, conf.Hotclick)
static int pure IsKillkey(unsigned char a)
{
    return
        (0x41 <= a && a <= 0x5A) // A-Z vkeys
        || IsHotkeyy(a, conf.Killkey) ;
}
static xpure int IsModKey(const UCHAR vkey)
{
    return IsHotkeyy(vkey, conf.ModKey);
}

static UCHAR TotNumberOfKeysDown()
{
    BYTE kb_state[256];
    GetKeyState(0); // You need that for GetKeyboardState()
    GetKeyboardState(kb_state);
    UCHAR numkeys=0;
    BYTE i;
    for (i=0x13; i < 0xFF; i++) {
        // vK codes go from 0 to 254 and we must skip a few
        if((0x3A <= i && i<=0x40) // Undefineds
        ||  i == 0x5E             // Reserved
        || (0x88 <= i && i<=0x8F) // Unassigned
        || (0x97 <= i && i<=0x9F) // Unassigned
        || (0xB8 <= i && i<=0xB9) // Reserved
        || (0xC1 <= i && i<=0xDA) // Reserved + Unassigned (D8-DA)
        || i == 0xE0 // Reserved
        || i == 0xE8 // Unassigned
        ) continue;

        numkeys += !!(kb_state[i]&0x80);
                 /*!!(GetKeyState(i)&0x8000)*/
    }
    return numkeys;
}
// Return true if required amount of hotkeys are holded.
// If KeyCombo is disabled, user needs to hold only one hotkey.
// Otherwise, user needs to hold at least two hotkeys.
static int IsHotkeyDown()
{
    // required keys 1 or 2
    UCHAR ckeys = 1 + conf.KeyCombo;

    // loop over all hotkeys
    const UCHAR *pos=&conf.Hotkeys[0];
    while (*pos && ckeys) {
        // check if key is held down. Use GetKeyState()?
        ckeys -= !!(GetAsyncKeyState(*pos++)&0x8000);
    }
    // return true if required amount of hotkeys are down
    return !ckeys && (!conf.MaxKeysNum || TotNumberOfKeysDown() <= conf.MaxKeysNum);
}

// returns the number of hotkeys/ModKeys that are pressed.
static int NumKeysDown()
{
    UCHAR keys = 0;
    // loop over all hotkeys
    const UCHAR *Hpos=&conf.Hotkeys[0];
    const UCHAR *Mpos=&conf.ModKey[0];
    while (*Hpos) {
        // check if key is held down
        keys += !!(GetKeyState(*Hpos++)&0x8000);
        keys += !!(GetKeyState(*Mpos++)&0x8000);
    }
    return keys;
}

/////////////////////////////////////////////////////////////////////////////
// index 1 => normal restore on any move restore & 1
// restore & 3 => Both 1 & 2 ie: Maximized then rolled.
static void RestoreOldWin(const POINT pt, unsigned was_snapped, RECT *ownd)
{
    // Restore old width/height?
    unsigned restore = 0;
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(state.hwnd, &rwidth, &rheight);

    if (((rdata_flag & SNAPPED) && !(state.origin.maximized&&rdata_flag&2))) {
        // Set origin width and height to the saved values
        if (!state.usezones) {
            restore = rdata_flag;
            state.origin.width = rwidth;
            state.origin.height = rheight;
            ClearRestoreData(state.hwnd);
        }
    }

    RECT wnd;
    GetWindowRect(state.hwnd, &wnd);
    // In case window is Maximized + Rolled get bottom where it needs to be
    // So that the window stays fully in the monitor
    // Note: a maximized then rolled window does not have the rolled flag.
    if (state.origin.maximized)
        wnd.bottom = state.origin.mon.bottom + state.mdipt.y;

    // Set offset
    state.offset.x = state.origin.width  * min(pt.x-wnd.left, wnd.right-wnd.left)
                   / max(wnd.right-wnd.left,1);
    state.offset.y = state.origin.height * min(pt.y-wnd.top, wnd.bottom-wnd.top)
                   / max(wnd.bottom-wnd.top,1);

    if (rdata_flag&ROLLED) {
        if (state.origin.maximized || was_snapped) {
            // if we restore a Rolled + Maximized/snapped window...
            state.offset.y = GetSystemMetricsForWin(SM_CYMIN, state.hwnd)/2;
        } else {
            state.offset.x = pt.x - wnd.left;
            state.offset.y = pt.y - wnd.top;
        }
    }

    if (restore) {
        LastWin.moveonly = 0;
//        SetWindowPos(state.hwnd, NULL
//                , pt.x - state.offset.x - state.mdipt.x
//                , pt.y - state.offset.y - state.mdipt.y
//                , state.origin.width, state.origin.height
//                , SWP_NOZORDER);
        ownd->left = pt.x - state.offset.x - state.mdipt.x;
        ownd->top =  pt.y - state.offset.y - state.mdipt.y;
        ownd->right = ownd->left + state.origin.width;
        ownd->bottom = ownd->top + state.origin.height;
        ClearRestoreData(state.hwnd);
    }
}
///////////////////////////////////////////////////////////////////////////
// Do not reclip the cursor if it is already clipped
// Do not unclip the cursor if it was not clipped by AltDrag.
static void ClipCursorOnce(const RECT *clip)
{
    static char trapped=0;
    if (trapped && !clip) {
        ClipCursor(NULL);
        trapped=0;
    } else if(!trapped && clip) {
        ClipCursor(clip);
        trapped = 1;
    }
}

static void RestrictCursorToMon()
{
    // Restrict pt within origin monitor if Ctrl is being pressed
    if (state.ctrl) {
        static HMONITOR origMonitor;
        static RECT fmon;
        if (origMonitor != state.origin.monitor || !state.origin.monitor) {
            origMonitor = state.origin.monitor;
            MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(state.origin.monitor, &mi);
            CopyRect(&fmon, &mi.rcMonitor);
            fmon.left++; fmon.top++;
            fmon.right--; fmon.bottom--;
        }
        RECT clip;
        if (state.mdiclient) {
            CopyRect(&clip, &state.origin.mon);
            OffsetRect(&clip, state.mdipt.x, state.mdipt.y);
        } else {
            CopyRect(&clip, &fmon);
        }
        ClipCursorOnce(&clip);
    }
}
///////////////////////////////////////////////////////////////////////////
// Get state.mdipt and mdi monitor
static BOOL GetMDInfo(POINT *mdicpt, RECT *wnd)
{
    mdicpt->x = mdicpt->y = 0;
    if (!GetClientRect(state.mdiclient, wnd)
    ||  !ClientToScreen(state.mdiclient, mdicpt) ) {
         return FALSE;
    }
    return TRUE;
}
///////////////////////////////////////////////////////////////////////////
//
static void SetOriginFromRestoreData(HWND hwnd, enum action action)
{
    // Set Origin width and height needed for AC_MOVE/RESIZE/CENTER/MAXHV
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(hwnd, &rwidth, &rheight);
    // Clear snapping info if asked.
    if (rdata_flag&SNCLEAR || (conf.SmartAero&4 && action == AC_MOVE)) {
        ClearRestoreData(hwnd);
        rdata_flag=0;
    }
    // Replace origin width/height if available in the restore Data.
    if (rdata_flag && !state.origin.maximized) {
        state.origin.width = rwidth;
        state.origin.height = rheight;
    }
}
/////////////////////////////////////////////////////////////////////////////
// Transparent window
// We use 4 thin windows to simulate a hollow window because the
// SetWindowRgn() function is very slow and would have to be called at every
// Mouse frame when resizing.
static void ShowTransWin(int nCmdShow)
{
    if(conf.TransWinOpacity) {
        if(g_transhwnd[0]) ShowWindow(g_transhwnd[0], nCmdShow);
    } else {
        int i;
        for (i=0; i<4; i++ )if(g_transhwnd[i]) ShowWindow(g_transhwnd[i], nCmdShow);
    }
}
#define HideTransWin() ShowTransWin(SW_HIDE)
static BOOL IsTransWinVisible() { return IsVisible(g_transhwnd[0]); }

static void MoveTransWinRaw(int x, int y, int w, int h)
{
    #define f SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING //|SWP_DEFERERASE
//      HDWP hwndSS = BeginDeferWindowPos(4);
    if(conf.TransWinOpacity) {
        SetWindowPos(g_transhwnd[0],NULL, x, y, w, h, f);
    } else {
        SetWindowPos(g_transhwnd[0],NULL,  x    , y    , w, 4, f);
        SetWindowPos(g_transhwnd[1],NULL,  x    , y    , 4, h, f);
        SetWindowPos(g_transhwnd[2],NULL,  x    , y+h-4, w, 4, f);
        SetWindowPos(g_transhwnd[3],NULL,  x+w-4, y    , 4, h, f);
    }
    #undef f
//      if(hwndSS) EndDeferWindowPos(hwndSS);
}
static void MoveTransWin(int x, int y, int w, int h)
{
    if (state.origin.dpi) {
        POINT pt = { x + w/2, y + h/2 };
        HMONITOR hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        if (hmon != state.origin.monitor && !GetRestoreFlag(state.hwnd)) {
            UINT ptdpi=0, dpiy_ignore=0;
            if ( S_OK == GetDpiForMonitorL(hmon, MDT_DEFAULT, &ptdpi, &dpiy_ignore) && ptdpi ) {
                w = MulDiv(w, ptdpi, state.origin.dpi);
                h = MulDiv(h, ptdpi, state.origin.dpi);
            }
        }
    }
    MoveTransWinRaw(x, y, w, h);
}
static DWORD CALLBACK WinPlacmntTrgead(LPVOID wndplptr)
{
    SetWindowPlacement(LastWin.hwnd, (WINDOWPLACEMENT *)wndplptr);
    LastWin.hwnd = NULL;
    return 0;
}
static void SetWindowPlacementThread(HWND hwnd, WINDOWPLACEMENT *wndplptr)
{
    LastWin.hwnd = hwnd;
    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, WinPlacmntTrgead, wndplptr, STACK_SIZE_PARAM_IS_A_RESERVATION, &lpThreadId));
}
// state.origin.mon initialized in init_mov..
// returns the final window rectangle.
static void RestoreToMonitorSize(HWND hwnd, RECT *wnd)
{
    ClearRestoreData(hwnd); //Clear restore flag and data
    WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);

    // Set size to origin monitor to prevent flickering
    // Get monitor info
    MONITORINFO mi;
    GetMonitorInfoFromWin(hwnd, &mi);

    CopyRect(wnd, &mi.rcWork);
    CopyRect(&wndpl.rcNormalPosition, wnd);

    if (state.mdiclient) {
        // Make it a little smaller since MDIClients by
        // default have scrollbars that would otherwise appear
        wndpl.rcNormalPosition.right -= 8;
        wndpl.rcNormalPosition.bottom -= 8;
    }
    wndpl.showCmd = SW_RESTORE;
    SetWindowPlacement(hwnd, &wndpl);
    if (state.mdiclient) {
        // Get new values from MDIClient,
        // since restoring the child have changed them,
        Sleep(1); // Sometimes needed
        GetMDInfo(&state.mdipt, wnd);
        CopyRect(&state.origin.mon, wnd);

        state.origin.right = wnd->right;
        state.origin.bottom=wnd->bottom;
    }
    // Fix wnd for MDI offset and invisible borders
    RECT borders;
    FixDWMRect(hwnd, &borders);
    OffsetRect(wnd, state.mdipt.x, state.mdipt.y);
    InflateRectBorder(wnd, &borders);
}
// Call if you need to resize the window.
// If smart areo is not enabled then, we need to clear the restore date
static void ClearRestoreFlagOnResizeIfNeeded(HWND hwnd)
{
    if (!(conf.SmartAero&1)) {
        // Always clear when AeroSmart is disabled.
        ClearRestoreData(hwnd);
    } else {
        // Do not clear is the window was snapped to some side or rolled.
        unsigned smart_restore_flag=(SNZONE|SNAPPEDSIDE|ROLLED);
        if(!(GetRestoreFlag(hwnd) & smart_restore_flag))
            ClearRestoreData(hwnd);
    }
}
///////////////////////////////////////////////////////////////////////////
static void UpdateCursor(POINT pt);
static void SetEdgeAndOffset(const RECT *wnd, POINT pt);
static void MouseMove(POINT pt)
{
    // Check if window still exists
    if (!IsWindow(state.hwnd))
        { LastWin.hwnd = NULL; UnhookMouse(); return; }

    if (conf.UseCursor) // Draw the invisible cursor window
        MoveWindow(g_mainhwnd, pt.x-128, pt.y-128, 256, 256, FALSE);

    if (state.moving == CURSOR_ONLY) {
        if (state.action == AC_RESIZE) {
            RECT rc;
            GetWindowRect(state.hwnd, &rc);
            SetEdgeAndOffset(&rc, pt);
            UpdateCursor(pt);
        }
        return; // Movement was blocked...
    }

    static RECT wnd; // wnd will be updated and is initialized once.
    if (!state.moving && !GetWindowRect(state.hwnd, &wnd)) return;
    int posx=0, posy=0, wndwidth=0, wndheight=0;

    // Restore Aero snapped window when movement starts
    UCHAR was_snapped = 0;
    if (!state.moving) {
        LastWin.odpi = state.origin.dpi;
        SetOriginFromRestoreData(state.hwnd, state.action);
        if (state.action == AC_MOVE) {
            was_snapped = IsWindowSnapped(state.hwnd);
            RestoreOldWin(pt, was_snapped, &wnd);
        }
    }


    // Convert pt in MDI coordinates.
    // state.mdipt is global!
    pt.x -= state.mdipt.x;
    pt.y -= state.mdipt.y;

    RestrictCursorToMon(); // When CTRL is pressed.

    // Get new position for window
    LastWin.end = 0;
    LastWin.moveonly = 0;
    if (state.action == AC_MOVE) {
        // SWP_NOSIZE to SetWindowPos
        LastWin.moveonly = state.moving;
        posx = pt.x-state.offset.x;
        posy = pt.y-state.offset.y;
        wndwidth = wnd.right-wnd.left;
        wndheight = wnd.bottom-wnd.top;

        // Check if the window will snap anywhere
        int ret = AeroMoveSnap(pt, &posx, &posy, &wndwidth, &wndheight);
        if (ret == 1) { state.moving = 1; return; }
        MoveSnap(&posx, &posy, wndwidth, wndheight, conf.SnapThreshold);
        MoveSnapToZone(pt, &posx, &posy, &wndwidth, &wndheight);

        // Restore window if maximized when starting
        if (!LastWin.snap && (was_snapped || IsZoomed(state.hwnd))) {
            LastWin.moveonly = 0;
            WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(state.hwnd, &wndpl);
            // Restore original width and height in case we are restoring
            // A Snapped + Maximized window.
            wndpl.showCmd = SW_RESTORE;
            unsigned restore = GetRestoreFlag(state.hwnd);
            if (!(restore&ROLLED)) { // Not if window is rolled!
                wndpl.rcNormalPosition.left = posx;
                wndpl.rcNormalPosition.top = posy;
                wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left + state.origin.width;
                wndpl.rcNormalPosition.bottom= wndpl.rcNormalPosition.top +  state.origin.height;
            }
            if (restore&SNTHENROLLED) ClearRestoreData(state.hwnd);// Only Flag?
            // Update wndwidth and wndheight
            wndwidth  = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
            wndheight = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
            if( !conf.FullWin && LOBYTE(GetVersion()) < 10 )
                wndpl.flags |= WPF_ASYNCWINDOWPLACEMENT;
            SetWindowPlacement(state.hwnd, &wndpl);
            EnumOnce(RECALC_INVISIBLE_BORDERS);
//            if (!conf.FullWin) {
//                wndpl.flags |= WPF_ASYNCWINDOWPLACEMENT;
//                SetWindowPlacement(state.hwnd, &wndpl);
//            } else {
//                LastWin.end=1;
//            }
        }

    } else if (state.action == AC_RESIZE) {
        // Restore the window (to monitor size) if it's maximized
        if (!state.moving && IsZoomed(state.hwnd)) {
              RestoreToMonitorSize(state.hwnd, &wnd);
        }
        // Clear restore flag if needed
        ClearRestoreFlagOnResizeIfNeeded(state.hwnd);
        // Figure out new placement
        if (state.resize.x == RZ_XCENTER && state.resize.y == RZ_YCENTER) {
            if (state.shift) pt.x = state.shiftpt.x;
            else if (state.ctrl) pt.y = state.ctrlpt.y;
            wndwidth  = wnd.right-wnd.left + 2*(pt.x-state.offset.x);
            wndheight = wnd.bottom-wnd.top + 2*(pt.y-state.offset.y);
            posx = wnd.left - (pt.x - state.offset.x) - state.mdipt.x;
            posy = wnd.top  - (pt.y - state.offset.y) - state.mdipt.y;

            // Keep the window perfectly centered.
            // even when going out of min or max sizes
            int W = CLAMPW(wndwidth);
            int dW = wndwidth - W;
            posx +=  dW/2;
            wndwidth = W;
            int H = CLAMPH(wndheight);
            int dH = wndheight - H;
            posy +=  dH/2;
            wndheight = H;

            state.offset.x = pt.x;
            state.offset.y = pt.y;

        } else {
            if (state.resize.y == RZ_TOP) {
                wndheight = CLAMPH( (wnd.bottom-pt.y+state.offset.y) - state.mdipt.y );
                posy = state.origin.bottom - wndheight;
            } else if (state.resize.y == RZ_YCENTER) {
                posy = wnd.top - state.mdipt.y;
                wndheight = wnd.bottom - wnd.top;
            } else if (state.resize.y == RZ_BOTTOM) {
                posy = wnd.top - state.mdipt.y;
                wndheight = pt.y - posy + state.offset.y;
            }
            if (state.resize.x == RZ_LEFT) {
                wndwidth = CLAMPW( (wnd.right-pt.x+state.offset.x) - state.mdipt.x );
                posx = state.origin.right - wndwidth;
            } else if (state.resize.x == RZ_XCENTER) {
                posx = wnd.left - state.mdipt.x;
                wndwidth = wnd.right - wnd.left;
            } else if (state.resize.x == RZ_RIGHT) {
                posx = wnd.left - state.mdipt.x;
                wndwidth = pt.x - posx+state.offset.x;
            }
            wndwidth =CLAMPW(wndwidth);
            wndheight=CLAMPH(wndheight);

            // Check if the window will snap anywhere
            ResizeSnap(&posx, &posy, &wndwidth, &wndheight, conf.SnapThreshold, conf.SnapThreshold);
            AeroResizeSnap(pt, &posx, &posy, &wndwidth, &wndheight);
        }
    }
    // LastWin is GLOBAL !
    UCHAR mouse_thread_finished = !LastWin.hwnd;
    LastWin.x      = posx;
    LastWin.y      = posy;
    LastWin.width  = wndwidth;
    LastWin.height = wndheight;

    // Update the static wnd with new dimentions.
    wnd.left   = posx + state.mdipt.x;
    wnd.top    = posy + state.mdipt.y;
    wnd.right  = posx + state.mdipt.x + wndwidth;
    wnd.bottom = posy + state.mdipt.y + wndheight;

    // Save hwnd After we are sure movement will occur.
    LastWin.hwnd   = state.hwnd;

    if (!conf.FullWin) {
        static RECT bd;
        if(!state.moving) FixDWMRectLL(state.hwnd, &bd, 0);
        int tx      = posx + state.mdipt.x + bd.left;
        int ty      = posy + state.mdipt.y + bd.top;
        int twidth  = wndwidth - bd.left - bd.right;
        int theight = wndheight - bd.top - bd.bottom;
        MoveTransWin(tx, ty, twidth, theight);
        if (!IsTransWinVisible())
            ShowTransWin(SW_SHOWNA);
        state.moving=1;
        ResizeTouchingWindows(&LastWin);

    } else if (mouse_thread_finished) {
        // Resize other windows
        if (!ResizeTouchingWindows(&LastWin)) {
            // The resize touching also resizes LastWin.
            MoveWindowInThread(&LastWin);
        }
        state.moving = 1;
    } else {
        Sleep(0);
        state.moving = NOT_MOVED; // Could not Move Window
    }
}
/////////////////////////////////////////////////////////////////////////////
static void Send_KEY(unsigned char vkey)
{
    KEYBDINPUT ctrl[2] = { {0, 0, 0, 0, 0}, {0, 0 , KEYEVENTF_KEYUP, 0, 0} };
    ctrl[0].wVk = ctrl[1].wVk = vkey;
    ctrl[0].dwExtraInfo = ctrl[1].dwExtraInfo = GetMessageExtraInfo();
//    ctrl[0].time = ctrl[1].time = GetTickCount();
    INPUT input[2]={{0},{0}};
    input[0].type = input[1].type = INPUT_KEYBOARD;
    input[0].ki = ctrl[0]; input[1].ki = ctrl[1];
    InterlockedIncrement(&state.ignorekey);
    SendInput(2, input, sizeof(INPUT));
    InterlockedDecrement(&state.ignorekey);
}
#define KEYEVENTF_KEYDOWN 0
// Call with or KEYEVENTF_KEYDOWN/KEYEVENTF_KEYUP
static void Send_KEY_UD(unsigned char vkey, WORD flags)
{
    KEYBDINPUT ctrl = {0, 0, 0, 0, 0};
    ctrl.wVk = vkey;
    ctrl.dwExtraInfo = GetMessageExtraInfo();
    ctrl.dwFlags = flags;
    ctrl.time = GetTickCount();
    INPUT input={0};
    input.type = INPUT_KEYBOARD;
    input.ki = ctrl;

    InterlockedIncrement(&state.ignorekey);
    SendInput(1, &input, sizeof(INPUT));
    InterlockedDecrement(&state.ignorekey);
}
#define Send_CTRL() if (conf.EndSendKey) Send_KEY(conf.EndSendKey)

// Send a sequence of Inputs.....
static void SendInputSequence(const UCHAR *seq)
{
    UCHAR len = *seq;
    while (len--) {
        UCHAR vKey = *++seq;
        UCHAR Down = *++seq;
        if(Down == 2) // Combined U, then D
            Send_KEY(vKey);
        else
            Send_KEY_UD(vKey, Down? KEYEVENTF_KEYDOWN: KEYEVENTF_KEYUP);
        //LOGA("Sending %x, %s", (UINT)vKey, Down? "Down": "Up");
    }
}

/////////////////////////////////////////////////////////////////////////////
// Sends the click down/click up sequence to the system
static DWORD WINAPI Send_ClickProc(LPVOID buttonD)
{
    enum button button = (enum button)(LONG_PTR)buttonD;
    static const WORD bmapping[] = {
          MOUSEEVENTF_LEFTDOWN
        , MOUSEEVENTF_RIGHTDOWN
        , MOUSEEVENTF_MIDDLEDOWN
        , MOUSEEVENTF_XDOWN, MOUSEEVENTF_XDOWN
    };
    if (!button || button > BT_MB5) return 0;

    DWORD MouseEvent = bmapping[button-2];
    DWORD mdata = 0;
    if (MouseEvent == MOUSEEVENTF_XDOWN) // XBUTTON
        mdata = button - 0x04; // mdata = 1 for X1 and 2 for X2
    // MouseEvent<<1 corresponds to MOUSEEVENTF_*UP
    MOUSEINPUT click[2];
    mem00(&click[0], sizeof(MOUSEINPUT)*2);
    click[0].mouseData = click[1].mouseData = mdata;
    click[0].dwFlags = MouseEvent;
    click[1].dwFlags = MouseEvent<<1;
    click[0].dwExtraInfo = click[1].dwExtraInfo = GetMessageExtraInfo();
    INPUT input[2];
    input[0].type = input[1].type = INPUT_MOUSE;
    input[0].mi = click[0]; input[1].mi = click[1];

    InterlockedIncrement(&state.ignoreclick);
    SendInput(2, input, sizeof(INPUT));
    InterlockedDecrement(&state.ignoreclick);
    return 0;
}
#define Send_Click(x) Send_ClickProc((LPVOID)(LONG_PTR)(x));
static void Send_Click_Thread(enum button button)
{
    DWORD id;
    CloseHandle(CreateThread(NULL, STACK, Send_ClickProc, (LPVOID)(LONG_PTR)button, STACK_SIZE_PARAM_IS_A_RESERVATION, &id));
}
/////////////////////////////////////////////////////////////////////////////
// Sends an unicode character to the system.
// KEYEVENTF_UNICODE requires at least Windows 2000
// Extended unicode page can be accessed by sending both lo&hi surrogates
static void SendUnicodeKey(WORD w)
{
    KEYBDINPUT ctrl[2] = {
        {0, 0, KEYEVENTF_UNICODE, 0, 0},
        {0, 0, KEYEVENTF_UNICODE|KEYEVENTF_KEYUP, 0, 0}
    };
    // mem00(&click[0], sizeof(KEYBDINPUT)*2);
    ctrl[0].wScan = ctrl[1].wScan = w;
//    ctrl[0].dwFlags = KEYEVENTF_UNICODE;
//    ctrl[1].dwFlags = KEYEVENTF_UNICODE|KEYEVENTF_KEYUP;
    ctrl[0].dwExtraInfo = ctrl[1].dwExtraInfo = GetMessageExtraInfo();
//    ctrl[0].time = ctrl[1].time = GetTickCount();
    INPUT input[2];
    input[0].type = input[1].type = INPUT_KEYBOARD;
    input[0].ki = ctrl[0]; input[1].ki = ctrl[1];

    InterlockedIncrement(&state.ignorekey);
    SendInput(2, input, sizeof(INPUT));
    InterlockedDecrement(&state.ignorekey);
}

///////////////////////////////////////////////////////////////////////////
static void RestrictToCurentMonitor()
{
    if (state.action || state.alt) {
        POINT pt;
        GetCursorPos(&pt);
        state.origin.maximized = 0; // To prevent auto-remax on Ctrl
        state.origin.monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    }
}
///////////////////////////////////////////////////////////////////////////
static void HotkeyUp()
{
    // Prevent the alt keyup from triggering the window menu to be selected
    // The way this works is that the alt key is "disguised" by sending
    // ctrl keydown/keyup events
    if (state.blockaltup || state.action) {
        //LOGA("SendCtrl");
        Send_CTRL();
        state.blockaltup = 0;
        // If there is more that one key down remaining
        // then we must block the next alt up.
        if (NumKeysDown() > 1) state.blockaltup = 1;
    }

    // Hotkeys have been released
    state.alt = 0;
    state.alt1 = 0;
    if (state.action
    && (conf.GrabWithAlt[0] || conf.GrabWithAlt[1])
    && (MOUVEMENT(conf.GrabWithAlt[0]) || MOUVEMENT(conf.GrabWithAlt[1]))) {
        FinishMovement();
    }

    // Unhook mouse if no actions is ongoing.
    if (!state.action) {
        UnhookMouse();
    }
}

///////////////////////////////////////////////////////////////////////////
static int ActionPause(HWND hwnd, UCHAR pause)
{
//    LOGA("Entering pause/resume %d", (int)pause);
    if (!blacklisted(hwnd, &BlkLst.Pause)) {
        DWORD pid=0;
        GetWindowThreadProcessId(hwnd, &pid);

        #define P1_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff)
        #define P2_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff)
        HANDLE ProcessHandle = OpenProcess(P2_ALL_ACCESS, FALSE, pid);
        if(!ProcessHandle)
            ProcessHandle = OpenProcess(P1_ALL_ACCESS, FALSE, pid);
        if(!ProcessHandle)
            ProcessHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);

        if (ProcessHandle) {
//            LOGA("Going to pause/resume %d", (int)pause);
            if (pause) NtSuspendProcess(ProcessHandle);
            else       NtResumeProcess(ProcessHandle);

            CloseHandle(ProcessHandle);
//            LOGA("Done");
            return 1;
        }
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
// Kill the process from hwnd
DWORD WINAPI ActionKillThread(LPVOID hwnd)
{
    if (state.shift) {
        // TODO: Aggressive command such as:
        // taskkill.exe /F /FI "status eq NOT RESPONDING" /FI "IMAGENAME ne AltSnap.exe" /FI "IMAGENAME ne dwm.exe"
    } else {
        DWORD pid;
        GetWindowThreadProcessId((HWND)hwnd, &pid);
        //LOG("pid=%lu", pid);

        // Open the process
        HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        //LOG("proc=%lx", (DWORD)proc);
        if (proc) {
            TerminateProcess(proc, 1);
            CloseHandle(proc);
        }
        return 1;
    }
    return 0;
}
static int ActionKill(HWND hwnd)
{
    //LOG("hwnd=%lx",(DWORD) hwnd);
    if (!hwnd) return 0;

    if(isClassName(hwnd, TEXT("Ghost"))) {
        PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        return 1;
    }

    if(blacklisted(hwnd, &BlkLst.Pause))
       return 0;

    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, ActionKillThread, hwnd, STACK_SIZE_PARAM_IS_A_RESERVATION, &lpThreadId));

    return 1;
}
// Try harder to actually set the window foreground.
static void ReallySetForegroundWindow(HWND hwnd)
{
    if (!hwnd) return;
    // Check existing foreground Window.
    HWND  fore = GetForegroundWindow();
    if (fore != hwnd) {
        if (state.alt != VK_MENU &&  state.alt != VK_CONTROL
        && !(GetKeyState(VK_CONTROL)&0x8000)
        && !(GetKeyState(VK_MENU)&0x8000)) {
            // If the physical Alt or Ctrl keys are not down
            // We need to activate the window with key input.
            // CTRL seems to work. Also Alt works but trigers the menu
            // So it is simpler to stick to CTRL.
            Send_KEY(VK_CONTROL);
        }
        BringWindowToTop(hwnd);
        SetForegroundWindow(hwnd);
    }
}
static void SetForegroundWindowL(HWND hwnd)
{
    if (!state.mdiclient) {
        ReallySetForegroundWindow(hwnd);
    } else {
        ReallySetForegroundWindow(state.mdiclient);
        PostMessage(state.mdiclient, WM_MDIACTIVATE, (WPARAM)hwnd, 0);
    }
}
// Returns true if AltDrag must be disabled based on scroll lock
// If conf.ScrollLockState&2 then Altdrag is disabled by Scroll Lock
// otherwise it is enabled by Scroll lock.
static int ScrollLockState()
{
    if (state.altsnaponoff)
        return 1; // AltSnap was disabled by AC_ASONOFF

    if( (conf.ScrollLockState&1)
    && !( !(GetKeyState(VK_SCROLL)&1) ^ !(conf.ScrollLockState&2) ) ) {
        if (state.action)
            FinishMovement();
        return 1;
    }
    return 0;
}
static void LogState(const char *Title)
{
    FILE *f=fopen("ad.log", "a"); // append data...
    fputs(Title, f);
    fprintf(f,
        "action=%d\n"
        "moving=%d\n"
        "ctrl=%d\n"
        "shift=%d\n"
        "alt=%d\n"
        "alt1=%d\n"
    , (int)state.action
    , (int)state.moving
    , (int)state.ctrl
    , (int)state.shift
    , (int)state.alt
       , (int)state.alt1);

    fprintf(f,
       "clickbutton=%d\n"
       "hwnd=%lx\n"
       "lwhwnd=%lx\n"
       "lwend=%d\n"
       "lwmoveonly=%d\n"
       "lwmaximize=%d\n"
       "lwsnap=%d\n"
       "blockaltup=%d\n"
       "blockmouseup=%d\n"
       "ignorekey=%d\n\n\n"
    , (int)state.clickbutton
    , (DWORD)(DorQWORD)state.hwnd
    , (DWORD)(DorQWORD)LastWin.hwnd
    , (int)LastWin.end
    , (int)LastWin.moveonly
    , (int)LastWin.maximize
    , (int)LastWin.snap
    , (int)state.blockaltup
    , (int)state.blockmouseup
    , (int)state.ignorekey );
    fclose(f);
}
static pure int XXButtonIndex(UCHAR vkey)
{
    WORD i;
    for (i=0; i < MAXKEYS && conf.XXButtons[i]; i++) {
        if(conf.XXButtons[i] == vkey)
            return i+3;
    }
    return -1;
}
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
static int SimulateXButton(WPARAM wp, WORD xbtidx)
{
//    WORD xbtidx = XXButtonIndex(vkey);
    MSLLHOOKSTRUCT msg;
    GetCursorPos(&msg.pt);
    // XButton number is in HIWORD(mouseData)
    msg.mouseData= xbtidx << 16;
    msg.flags=0;
    msg.time = GetTickCount();
    LowLevelMouseProc(HC_ACTION, wp, (LPARAM)&msg);
    return 1;
}
// Destroy AltSnap's menu
static void KillAltSnapMenu()
{
//    if (state.unikeymenu) {
//        EnableWindow(g_mchwnd, FALSE);
//        DestroyMenu(state.unikeymenu);
//        state.unikeymenu = NULL;
//    }
    if (g_mchwnd) {
        SendMessage(g_mchwnd, WM_CLOSE, 0, 0);
        g_mchwnd = NULL;
    }
    state.unikeymenu = NULL;
}
static void ToggleSnapState(void)
{
    state.snap = state.snap? 0: 3;
}
static void TogglesAlwaysOnTop(HWND hwnd);
static HWND MDIorNOT(HWND hwnd, HWND *mdiclient_);
///////////////////////////////////////////////////////////////////////////
// Keep this one minimalist, it is always on.
#ifdef __cplusplus
extern "C"
#endif
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
#if defined(_MSC_VER) && _MSC_VER > 1300
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
#endif

    if (nCode != HC_ACTION || state.ignorekey) return CallNextHookEx(NULL, nCode, wParam, lParam);

    PKBDLLHOOKSTRUCT kbh = ((PKBDLLHOOKSTRUCT)lParam);
    unsigned char vkey = kbh->vkCode;
//    DWORD scode = kbh->scanCode;
    int xxbtidx;
    HWND fhwnd = NULL;
//    if (vkey!=VK_F5) { // show key codes
//        LOGA("wp=%u, vKey=%lx, sCode=%lx, flgs=%lx, ex=%lx"
//        , wParam, kbh->vkCode, kbh->scanCode, kbh->flags, kbh->dwExtraInfo);
//    }
    if (vkey == VK_SCROLL) PostMessage(g_mainhwnd, WM_UPDATETRAY, 0, 0);
    if (ScrollLockState()) return CallNextHookEx(NULL, nCode, wParam, lParam);

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        if (!state.alt && !state.action
        && (!conf.KeyCombo || (state.alt1 && state.alt1 != vkey))
        && IsHotkey(vkey)
        && (!conf.MaxKeysNum || TotNumberOfKeysDown() < conf.MaxKeysNum)) {
            //LOGA("\nALT DOWN");
            state.alt = vkey;
            state.blockaltup = 0;
            state.sclickhwnd = NULL;
            KillAltSnapMenu(); // Hide unikey menu in case...

            // Release ALt even if we receave no AltUP message because of.
            // Stupid software that like to steal Alt.
            #ifdef ALTUP_TIMER
            DWORD kbspeed=2;
            if (GetKeyState(state.alt) & 0x8000) {
               // If the key is autorepeating
               SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &kbspeed, 0 );
               kbspeed = 33 + kbspeed * 12; // [0-31] -> 33 - 400 ms
               //LOGA("Set ALTUP_TIMER to %u (autorepeat)", kbspeed);
            } else {
               SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, &kbspeed, 0 );
               kbspeed = 250 + kbspeed * 250; // [0-3] -> 250 - 1000 ms
               //LOGA("Set ALTUP_TIMER to %u (First)", kbspeed);
            }
            KillTimer(g_timerhwnd, ALTUP_TIMER);
            SetTimer(g_timerhwnd, ALTUP_TIMER, kbspeed, NULL);
            #endif

            // Hook mouse
            HookMouse();
            if (conf.GrabWithAlt[0] || conf.GrabWithAlt[1]) {
                POINT pt;
                enum action action = conf.GrabWithAlt[IsModKey(vkey) || (!IsHotkey(conf.ModKey[0])&&ModKey())];
                if (action) {
                    state.blockmouseup = 0; // In case.
                    GetCursorPos(&pt);
                    if (!init_movement_and_actions(pt, NULL, action, vkey)) {
                        UnhookMouse();
                    }
                    state.blockmouseup = 0; // In case.
                }
            }
        } else if (conf.KeyCombo && !state.alt1 && IsHotkey(vkey)) {
            state.alt1 = vkey;

        } else if (IsHotkeyy(vkey, conf.Shiftkeys)) {
            if (!state.shift && !IsModKey(vkey)/* != conf.ModKey*/) {
                if (conf.ShiftSnaps) {
                    state.snap = conf.AutoSnap==3? 0: 3;
                }
                state.shift = 1;
                state.usezones = (conf.UseZones&9) != 9; // Zones and
                state.shiftpt = state.prevpt; // Save point where shift was pressed.
                state.enumed = 0; // Reset enum state.
            }

            // Block keydown to prevent Windows from changing keyboard layout
            if (state.alt && state.action) {
                return 1;
            }
        } else if (vkey == VK_SPACE && state.action && !IsSamePTT(&state.clickpt, &state.prevpt)) {
            ToggleSnapState();
            return 1; // Block to avoid sys menu.
        } else if (state.alt && state.action == conf.GrabWithAlt[ModKey()] && IsKillkey(vkey)) {
           // Release Hook on Alt+KillKey
           // eg: DisplayFusion Alt+Tab elevated windows captures AltUp
            HotkeyUp();
        } else if ( IsHotkeyy(vkey, conf.ESCkeys) ) { // USER PRESSED ESCAPE! (vkey == VK_ESCAPE)
            // Alsays disable shift and ctrl, in case of Ctrl+Shift+ESC.
            // LogState("ESCAPE KEY WAS PRESSED:\n"); // Debug stuff....
            state.ctrl = 0;
            state.shift = 0;
            LastWin.hwnd = NULL;
            state.ignorekey = 0; // In case ...
            state.ignoreclick = 0; // In case ...
            if (state.unikeymenu || g_mchwnd) {
                int ret = IsMenu(state.unikeymenu);
                KillAltSnapMenu();
                if (ret) return 1;
            }

            // Stop current action
            if (state.action || state.alt) {
                enum action action = state.action;
                HideTransWin();
                // Send WM_EXITSIZEMOVE
                SendSizeMove(WM_EXITSIZEMOVE);

                state.alt = 0;
                state.alt1 = 0;

                UnhookMouse();

                // Block ESC if an action was ongoing
                if (action) return 1;
            }
        } else if (!state.ctrl && state.alt!=vkey && !IsModKey(vkey)/*vkey != conf.ModKey*/
               && (vkey == VK_LCONTROL || vkey == VK_RCONTROL)) {
            RestrictToCurentMonitor();
            // If menu is present inform it that we pressed Ctrl.
            //if (state.unikeymenu) PostMessage(g_mchwnd, WM_CLOSEMODE, 1, 0);
            state.ctrl = 1;
            state.ctrlpt = state.prevpt; // Save point where ctrl was pressed.
            if (state.action) {
                SetForegroundWindowL(state.hwnd);
            }
        } else if (state.sclickhwnd && g_mchwnd && state.alt && (vkey == VK_LMENU || vkey == VK_RMENU)) {
            // Block Alt down when the altsnap's menu just opened
            if (state.unikeymenu==(HMENU)1
            || (IsWindow(state.sclickhwnd)  && IsWindow(g_mchwnd) && IsMenu(state.unikeymenu)))
                return 1;
        } else if ((xxbtidx = XXButtonIndex(vkey)) >=0
        && (GetAction(BT_MMB+xxbtidx) ||  GetActionT(BT_MMB+xxbtidx) || IsHotclick(BT_MMB+xxbtidx))) {
            if (!state.xxbutton) {
                state.xxbutton = 1; // To Ignore autorepeat...
                SimulateXButton(WM_XBUTTONDOWN, xxbtidx);
            }
            return 1;


        } else if (conf.UniKeyHoldMenu
        && (fhwnd=GetForegroundWindow())
        && !IsFullScreenBL(fhwnd)
        && !blacklisted(fhwnd, &BlkLst.Processes)
        && !blacklisted(fhwnd, &BlkLst.Windows)) {
            // Key lists used below...
            static const UCHAR ctrlaltwinkeys[] =
                {VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN, 0};
            static const UCHAR menupopdownkeys[] =
                { VK_BACK, VK_TAB, VK_APPS, VK_DELETE, VK_SPACE, VK_LEFT, VK_RIGHT
                , VK_PRIOR, VK_NEXT, VK_END, VK_HOME, 0};
            if (state.unikeymenu && IsMenu(state.unikeymenu) && !(GetKeyState(vkey)&0x8000)) {
                if (vkey == VK_SNAPSHOT) return CallNextHookEx(NULL, nCode, wParam, lParam);
                if (IsHotkeyy(vkey, menupopdownkeys)) {
                    KillAltSnapMenu();
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                }

                // Forward all keys to the menu...
                PostMessage(g_mchwnd, WM_KEYDOWN, vkey, 0); // all keys are "directed to the Menu"
                return 1; // block keydown

            } else if (!state.ctrl && !state.alt && (0x41 <= vkey && vkey <= 0x5A) && !IsAKeyDown(ctrlaltwinkeys) ) {
                // handle long A-Z keydown.
                if (GetKeyState(vkey)&0x8000) { // The key is autorepeating.
                    if(!IsMenu(state.unikeymenu)) {
                        KillAltSnapMenu();
                        state.unikeymenu = (HMENU)1;
                        g_mchwnd = KreateMsgWin(MenuWindowProc, TEXT(APP_NAMEA)TEXT("-SClick"), 2);
                        UCHAR shiftdown = GetKeyState(VK_SHIFT)&0x8000 || GetKeyState(VK_CAPITAL)&1;
                        PostMessage(g_mainhwnd, WM_UNIKEYMENU, (WPARAM)g_mchwnd, vkey|(shiftdown<<8) );
                    }
                    return 1; // block keydown
                }
            }
        }
    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
        if (IsHotkey(vkey)) {
            //LOGA("ALT UP");
            HotkeyUp();
        } else if (IsHotkeyy(vkey, conf.Shiftkeys)) {
            state.shift = 0;
            state.usezones = (conf.UseZones&9) == 9;
            state.snap = conf.AutoSnap;
            // if an action was performed and Alt is still down.
            if (state.alt && state.blockaltup && (vkey == VK_LSHIFT || vkey == VK_RSHIFT))
                Send_CTRL(); // send Ctrl to avoid Alt+Shift=>switch keymap
        } else if (state.blockaltup && IsModKey(vkey)) {
            // We release ModKey before Hotkey
            //LOGA("SendCtrlM");
            Send_CTRL();
            state.blockaltup = 0;
            // If there is more that one key down remaining
            // then we must block the next alt up.
            if (NumKeysDown() > 1) state.blockaltup = 1;
        } else if (vkey == VK_LCONTROL || vkey == VK_RCONTROL) {
            // If menu is present inform it that we released Ctrl.
            //if (state.unikeymenu) PostMessage(g_mchwnd, WM_CLOSEMODE, 0, 0);
            ClipCursorOnce(NULL); // Release cursor trapping
            state.ctrl = 0;
       // If there is no action then Control UP prevents AltDragging...
            if (!state.action) state.alt = 0;
        } else if ((xxbtidx = XXButtonIndex(vkey)) >=0 ) {
            state.xxbutton = 0;
            SimulateXButton(WM_XBUTTONUP, xxbtidx);
            return 1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
// 1.44
static int ScrollPointedWindow(POINT pt, int delta, WPARAM wParam)
{
    // Get window and foreground window
    HWND hwnd = WindowFromPoint(pt);
    if (!hwnd || blacklisted(hwnd, &BlkLst.IScroll))
        return 0;

    HWND foreground = GetForegroundWindow();

    // Return if foreground window is blacklisted
    if (foreground && blacklisted(foreground,&BlkLst.Windows))
        return 0;

    // If it's a groupbox, grab the real window
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((style&BS_GROUPBOX) && isClassName(hwnd, TEXT("Button"))) {
        HWND groupbox = hwnd;
        EnableWindow(groupbox, FALSE);
        hwnd = WindowFromPoint(pt);
        EnableWindow(groupbox, TRUE);
        if (!hwnd) return 0;
    }

    // Get wheel info
    WPARAM wp = delta << 16;
    LPARAM lp = MAKELPARAM(pt.x, pt.y);

    // Change WM_MOUSEWHEEL to WM_MOUSEHWHEEL if shift is being depressed
    // Introduced in Vista and far from all programs have implemented it.
    if (wParam == WM_MOUSEWHEEL && IsAKeyDown(conf.HScrollKey)) {
        wParam = WM_MOUSEHWHEEL;
        wp = -wp ; // Up is left, down is right
    }

    // Add button information since we don't get it with the hook
    static const UCHAR toOr[] = {
        VK_LBUTTON, // MK_LBUTTON  }, 1
        VK_RBUTTON, // MK_RBUTTON  }, 2
        VK_CONTROL, // MK_CONTROL  }, 4
        VK_SHIFT,   // MK_SHIFT    }, 8
        VK_MBUTTON, // MK_MBUTTON  }, 16
        VK_XBUTTON1,// MK_XBUTTON1 }, 32
        VK_XBUTTON2,// MK_XBUTTON2 }  64
    };
    unsigned i;
    for (i=0; i < ARR_SZ(toOr); i++) // Should we use GetKeyState?
        if (GetAsyncKeyState(toOr[i]) &0x8000) wp |= (1<<i);

    // Forward scroll message
    SendMessage(hwnd, wParam, wp, lp);
    // Simulate small steps wheel (can be used to debug other programs)
    //SendMessage(hwnd, WM_MOUSEWHEEL, (state.shift?(delta/7):delta)<<16, lp);

    // Block original scroll event
    return 1;
}
// Determine if we should select the window through AltTab equivalents
// We do not want the desktop window nor taskbar in this list
// We want usually a window with a taskbar or that has the WS_EX_APPWINDOW
// extended flag. Another case is the windows that were made borderless.
// We include all windows that do not have the WS_EX_TOOLWINDOW exstyle
// De also exclude all windows that are in the Bottommost list.
static int IsAltTabAble(HWND window)
{
    LONG_PTR xstyle;
    TCHAR txt[2];
    return IsVisible(window)
       && ((xstyle=GetWindowLongPtr(window, GWL_EXSTYLE))&WS_EX_NOACTIVATE) != WS_EX_NOACTIVATE
       && ( // Has a caption or borderless or present in taskbar.
            (xstyle&WS_EX_TOOLWINDOW) != WS_EX_TOOLWINDOW // Not a tool window
          ||(GetWindowLongPtr(window, GWL_STYLE)&WS_CAPTION) == WS_CAPTION // or has a caption
          ||(xstyle&WS_EX_APPWINDOW) == WS_EX_APPWINDOW // Or is forced in taskbar
          || GetBorderlessFlag(window) // Or we made it borderless
       )
       && (conf.MenuShowEmptyLabelWin || (GetWindowText(window, txt, ARR_SZ(txt)) && txt[0]))
       && !blacklisted(window, &BlkLst.Bottommost); // Exclude bottommost windows
}
static int IsToolWindow(HWND hwnd)
{
    return GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOOLWINDOW;
}
/////////////////////////////////////////////////////////////////////////////
unsigned hwnds_alloc = 0;

// lParam means to include minimized windows (pass TRK_LASERMODE to TrackMenuOfWindows)
BOOL CALLBACK EnumAltTabWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = (HWND *)GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized
    // to taskbar and on the same monitor as the cursor
    if (IsAltTabAble(window)
    && (!IsIconic(window) || (lParam && !IsToolWindow(window)))
    && state.origin.monitor == MonitorFromWindow(window,
            conf.MenuShowOffscreenWin ? MONITOR_DEFAULTTONEAREST : MONITOR_DEFAULTTONULL)) {
        hwnds[numhwnds++] = window;
    }
    return TRUE;
}
BOOL CALLBACK EnumAllAltTabWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = (HWND *)GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized
    // to taskbar, and either:
    //   offscreen windows are shown, or
    //   the window touches a monitor
    if (IsAltTabAble(window)
    && (!IsIconic(window) || (lParam && !IsToolWindow(window)))
    && (conf.MenuShowOffscreenWin || MonitorFromWindow(window, MONITOR_DEFAULTTONULL))) {
        hwnds[numhwnds++] = window;
    }
    return TRUE;
}
BOOL CALLBACK EnumTopMostWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = (HWND *)GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized
    // to taskbar and on the same monitor as the cursor
    if (IsAltTabAble(window)
    && GetWindowLongPtr(window, GWL_EXSTYLE)&WS_EX_TOPMOST) {
        hwnds[numhwnds++] = window;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
static pure BOOL StackedRectsT(const RECT *a, const RECT *b, const int T)
{ // Determine wether or not the windows are stacked...
    return RectInRectT(a, b, T) ||  RectInRectT(b, a, T);
}
// Similar to the EnumAltTabWindows, to be used in AltTab();
BOOL CALLBACK EnumStackedWindowsProc(HWND hwnd, LPARAM lasermode)
{
    // Make sure we have enough space allocated

    hwnds = (HWND *)GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed
    // Only store window if it's visible, not minimized to taskbar
    RECT wnd, refwnd;
    if (IsAltTabAble(hwnd)
    && !IsIconic(hwnd)
    && GetWindowRectL(state.hwnd, &refwnd)
    && GetWindowRectL(hwnd, &wnd)
    && (lasermode || StackedRectsT(&refwnd, &wnd, conf.SnapThreshold/2) )
    && InflateRect(&wnd, conf.SnapThreshold, conf.SnapThreshold)
    &&(state.ignorept || PtInRect(&wnd, state.prevpt))
    ){
        hwnds[numhwnds++] = hwnd;
        LOG("EnumStackedWindowsProc found");
    } else {
        LOG("EnumStackedWindowsProc skip");
    }
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////
// Returns the GA_ROOT window if not MDI or MDIblacklist
static HWND MDIorNOT(HWND hwnd, HWND *mdiclient_)
{
    HWND mdiclient = NULL;
    HWND root = GetAncestor(hwnd, GA_ROOT);

    if (conf.MDI && !blacklisted(root, &BlkLst.MDIs)) {
        while (hwnd != root) {
            HWND parent = GetParent(hwnd);
            if (!parent)             return root;
            else if (parent == hwnd) return hwnd;

            LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            if ((exstyle&WS_EX_MDICHILD)) {
                // Found MDI child, parent is now MDIClient window
                mdiclient = parent;
                break;
            }
            hwnd = parent;
        }
    } else {
        hwnd = root;
    }
    *mdiclient_ = mdiclient;
    return hwnd;
}
/////////////////////////////////////////////////////////////////////////////
static int ActionAltTab(POINT pt, short delta, short laser, WNDENUMPROC lpEnumFunc)
{
    numhwnds = 0;

    if (conf.MDI) {
        // Get Class name
        HWND hwnd = WindowFromPoint(pt);
        if (!hwnd) return 0;

        // Get MDIClient
        HWND mdiclient = NULL;
        if (isClassName(hwnd, TEXT("MDIClient"))) {
            mdiclient = hwnd; // we are pointing to the MDI client!
        } else {
            MDIorNOT(hwnd, &mdiclient); // Get mdiclient from hwnd
        }
        // Enumerate and then reorder MDI windows
        if (mdiclient) {
            EnumChildWindows(mdiclient, lpEnumFunc, laser);

            if (numhwnds > 1) {
                if (delta > 0) {
                    PostMessage(mdiclient, WM_MDIACTIVATE, (WPARAM) hwnds[numhwnds-1], 0);
                } else {
                    SetWindowLevel(hwnds[0], hwnds[numhwnds-1]);
                    PostMessage(mdiclient, WM_MDIACTIVATE, (WPARAM) hwnds[1], 0);
                }
            }
        }
    } // End if MDI

    // Enumerate windows
    if (numhwnds <= 1) {
        state.origin.monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        numhwnds = 0;
        EnumDesktopWindows(NULL, lpEnumFunc, laser);
        if (numhwnds <= 1) {
            return 0;
        }

        // Reorder windows
        if (delta > 0) {
            ReallySetForegroundWindow(hwnds[numhwnds-1]);
        } else {
            SetWindowLevel(hwnds[0], hwnds[numhwnds-1]);
            ReallySetForegroundWindow(hwnds[1]);
        }
    }
    return 1;
}
/////////////////////////////////////////////////////////////////////////////
// Helper function with a stupid static accululator for
// The wheel in case we have a fine wheel that is unable to
// trigger a single step.
static short ScaleDeltaAndAccum(short delta, short tar_delta)
{
    short step = ((int)tar_delta * (int)delta) / WHEEL_DELTA;
    // Only accumulate
    if( step == 0 )
    { // step is too small, we need to accumulate delta
        static short delta_acc = 0;
        delta_acc += delta;
        // Recalculate the step.
        step = (tar_delta * delta_acc) / WHEEL_DELTA;
        if( step == 0 )
            return 0;
        else // Reset the value (simpler)
            delta_acc = 0;
    }
    return step;
}

/////////////////////////////////////////////////////////////////////////////
// Changes the Volume on Windows 2000+ using VK_VOLUME_UP/VK_VOLUME_DOWN
// Also uses OLE API if available
#ifndef NO_OLEAPI
static const CLSID my_CLSID_MMDeviceEnumerator= {0xBCDE0395,0xE52F,0x467C,{0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E}};
static const GUID  my_IID_IMMDeviceEnumerator = {0xA95664D2,0x9614,0x4F35,{0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6}};
static const GUID  my_IID_IAudioEndpointVolume= {0x5CDF2C82,0x841E,0x4546,{0x97,0x22,0x0C,0xF7,0x40,0x78,0x22,0x9A}};
#define _WIN32_WINNT 0x0600
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#undef _WIN32_WINNT
/* OLE32.DLL */
static HRESULT (WINAPI *myCoInitialize)(LPVOID pvReserved);
static VOID (WINAPI *myCoUninitialize)( );
static HRESULT (WINAPI *myCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);

static BOOL LoadOleDll()
{

    HINSTANCE h = LoadLibraryA("OLE32.DLL");
    if (!h) return FALSE;

    myCoInitialize = (HRESULT (WINAPI *)(LPVOID))GetProcAddress(h, "CoInitialize");
    myCoUninitialize= (VOID (WINAPI *)( ))GetProcAddress(h, "CoUninitialize");
    myCoCreateInstance= (HRESULT (WINAPI *)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *))GetProcAddress(h, "CoCreateInstance");
    if (!myCoCreateInstance || !myCoUninitialize || !myCoInitialize) {
        FreeLibrary(h);
        return FALSE;
    }
    return TRUE;
}

static BOOL LoadOLEDLLOnce()
{
    static char HaveV=-1;
    if (HaveV == -1) {
        HaveV = LoadOleDll();
    }
    return HaveV;
}
// Use IAudioEndpointVolume COM interface to determine current Volume (Vista+)
static HRESULT GetCurrentVolumeMute(UINT *curentVol, UINT *maxVol, BOOL *muted)
{
    BYTE osver=LOBYTE(GetVersion());
    if (osver >= 6 && LoadOLEDLLOnce()) {
        HRESULT hr;
        IMMDeviceEnumerator *pDevEnum = NULL;
        IMMDevice *pDev = NULL;
        IAudioEndpointVolume *pAudioEndp = NULL;

        // Get audio endpoint
        myCoInitialize(NULL); // Needed for IAudioEndpointVolume
        hr = myCoCreateInstance(&my_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL
                              , &my_IID_IMMDeviceEnumerator, (void**)&pDevEnum);
        if (hr != S_OK) goto fail;

        hr = pDevEnum->lpVtbl->GetDefaultAudioEndpoint(pDevEnum, eRender, eMultimedia, &pDev);
        pDevEnum->lpVtbl->Release(pDevEnum);
        if (hr != S_OK) goto fail;

        hr = pDev->lpVtbl->Activate(pDev, &my_IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&pAudioEndp);
        pDev->lpVtbl->Release(pDev);
        if (hr != S_OK) goto fail;

        hr = pAudioEndp->lpVtbl->GetVolumeStepInfo(pAudioEndp, curentVol, maxVol)
           | pAudioEndp->lpVtbl->GetMute(pAudioEndp, muted);

        pAudioEndp->lpVtbl->Release(pAudioEndp);

        LOG("%d==GetVolumeStepInfo() -> %u (0 - %u) Muted=%d ", hr, curentVol, maxVol, muted);

        fail:
        myCoUninitialize();
        return hr;
    }
    return E_NOTIMPL;
}
#endif
// Under Vista this will change the main volume with ole interface,
static void ActionVolume(int delta)
{
    int num = (state.shift)?5:1;
    num = ScaleDeltaAndAccum(delta, num);
    num = abs(num);
    if(!num) return;

    #ifndef NO_OLEAPI
    UINT curentvol=0, maxvol=0;
    BOOL muted=FALSE;
    if (S_OK==GetCurrentVolumeMute(&curentvol, &maxvol, &muted) && curentvol) {
        if( delta < 0 && (UINT)num >= curentvol ) {
            // We would Set the volume to Zero
            // We prefer to mute instead,
            // because otherwise it resets volume balance.
            if( !muted)
                Send_KEY(VK_VOLUME_MUTE); // Mute
            return;
        } else if (delta > 0) {
            if( muted ) {
                Send_KEY(VK_VOLUME_MUTE); // UnMute
                return;
            }
        }
    }
    #endif // NO_OLEAPI
    while (num--)
        Send_KEY(delta>0? VK_VOLUME_UP: VK_VOLUME_DOWN);
}

/////////////////////////////////////////////////////////////////////////////
// Windows 2000+ Only
static int ActionTransparency(HWND hwnd, short delta)
{
    static int alpha=255;

    if (blacklisted(hwnd, &BlkLst.Windows)) return 0; // Spetial
    if (MOUVEMENT(state.action)) SetWindowTrans((HWND)-1);

    short pre_delta = (state.shift)? conf.AlphaDeltaShift: conf.AlphaDelta;
    int alpha_delta = ScaleDeltaAndAccum(delta, pre_delta);
    if (!alpha_delta) return 1;

    LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (alpha_delta < 0 && !(exstyle&WS_EX_LAYERED)) {
        // Add layered attribute to be able to change alpha
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle|WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    }

    BYTE old_alpha;
    if (GetLayeredWindowAttributes(hwnd, NULL, &old_alpha, NULL)) {
         alpha = old_alpha; // If possible start from the current aplha.
    }

    alpha = CLAMP(conf.MinAlpha, alpha+alpha_delta, 255); // Limit alpha

    if (alpha >= 255) // Remove layered attribute if opacity is 100%
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
    else
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

    return 1;
}
static void SetBottomMost(HWND hwnd)
{
    HWND lowhwnd = HWND_BOTTOM; // Lowest hwnd to consider.

    if(!blacklisted(hwnd, &BlkLst.Bottommost)) {
        // Ignore bottommost list if the lowered windows is also part of it.
        HWND tmph=hwnd;
        int i=0;
        while ((tmph = GetNextWindow(tmph, GW_HWNDNEXT)) && i++ < 1024) {
            if (IsVisible(tmph)
            && !IsIconic(tmph)
            && blacklisted(tmph, &BlkLst.Bottommost)) {
                lowhwnd = GetNextWindow(tmph, GW_HWNDPREV);
                break;
            }
        }
    }
    if (lowhwnd) SetWindowLevel(hwnd, lowhwnd);
}
/////////////////////////////////////////////////////////////////////////////
static int xpure IsAeraCapbutton(int area);
static void ActionLower(HWND hwnd, short delta, UCHAR shift, UCHAR fg)
{
    if (delta > 0) {
        if (shift) {
            ToggleMaxRestore(hwnd);
        } else {
            if (conf.AutoFocus || fg) SetForegroundWindowL(hwnd);
            SetWindowLevel(hwnd, HWND_TOPMOST);
            SetWindowLevel(hwnd, HWND_NOTOPMOST);
        }
    } else if (delta == 0 && (state.ctrl || IsAeraCapbutton(state.hittest))) {
        // turn lower in *Always on top* if Ctrl or [_][O][X]
        TogglesAlwaysOnTop(hwnd);
    } else {
        if (shift) {
            MinimizeWindow(hwnd);
        } else {
            if(hwnd == GetAncestor(GetForegroundWindow(), GA_ROOT)) {
                HWND tmp = GetWindow(hwnd, GW_HWNDNEXT);
                if(!tmp) tmp = GetWindow(hwnd, GW_HWNDPREV);
                if(tmp && hwnd != GetAncestor(tmp, GA_ROOT))
                    SetForegroundWindowL(tmp);
            }
            // Takes bottommost blacklist into account.
            SetBottomMost(hwnd);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
static void ActionMaxRestMin(HWND hwnd, int delta)
{
    int maximized = IsZoomed(hwnd);
    if (state.shift) {
        ActionLower(hwnd, delta, 0, state.ctrl);
        return;
    }

    if (delta > 0) {
        if (!maximized && IsResizable(hwnd))
            MaximizeWindow(hwnd);
    } else {
        if (maximized)
            RestoreWindow(hwnd);
        else
            MinimizeWindow(hwnd);
    }
    if (conf.AutoFocus) SetForegroundWindowL(hwnd);
}

/////////////////////////////////////////////////////////////////////////////
// Adjust brightness
static void ActionBrightness(const POINT pt, const short delta)
{
// Works oly for Desktop monitors.
#ifndef NO_BRIGHTNESS
    typedef struct _PHYSICAL_MONITOR {
        HANDLE hPhysicalMonitor;
        WCHAR  szPhysicalMonitorDescription[128];
    } PHYSICAL_MONITOR, *LPPHYSICAL_MONITOR;
    enum { MC_CAPS_BRIGHTNESS=2 };

    BOOL (WINAPI *myGetPhysMonitorsFromHM)(HMONITOR hMonitor, DWORD sz, LPPHYSICAL_MONITOR pmarr);
    BOOL (WINAPI *myGetMonitorBrightness)(HANDLE hMonitor, LPDWORD min, LPDWORD cur, LPDWORD max);
    BOOL (WINAPI *mySetMonitorBrightness)(HANDLE hMonitor, DWORD dwNewBrightness);
    BOOL (WINAPI *myDestroyPhysicalMonitors)(DWORD cnt, LPPHYSICAL_MONITOR pmarr);
    BOOL (WINAPI *myGetNumberOfPhysmons)(HMONITOR hMonitor, LPDWORD pdwNumberOfPhysicalMonitors);
    BOOL (WINAPI *myGetMonitorCapabilities)(HANDLE hMonitor, LPDWORD supcap, LPDWORD supcoltemp);
    HMODULE dll = LoadLibraryA("DXVA2.DLL");
    if (dll) {
        int ok;
        PHYSICAL_MONITOR *pm;
        DWORD dwMCap=0, wdColtemp=0;
        DWORD numpm=0;
        HMONITOR hmon;

        LOG("DXVA2.DLL Loaded");
        myGetPhysMonitorsFromHM =(BOOL (WINAPI *)(HMONITOR , DWORD , LPPHYSICAL_MONITOR))
            GetProcAddress(dll, "GetPhysicalMonitorsFromHMONITOR");
        myGetMonitorBrightness = (BOOL (WINAPI *)(HANDLE, LPDWORD, LPDWORD, LPDWORD))
            GetProcAddress(dll, "GetMonitorBrightness");
        mySetMonitorBrightness = (BOOL (WINAPI *)(HANDLE , DWORD ))
            GetProcAddress(dll, "SetMonitorBrightness");
        myDestroyPhysicalMonitors=(BOOL (WINAPI *)(DWORD, LPPHYSICAL_MONITOR))
            GetProcAddress(dll, "DestroyPhysicalMonitors");
        myGetNumberOfPhysmons   =(BOOL (WINAPI *)(HMONITOR, LPDWORD))
            GetProcAddress(dll, "GetNumberOfPhysicalMonitorsFromHMONITOR");
        myGetMonitorCapabilities=(BOOL (WINAPI *)(HANDLE, LPDWORD, LPDWORD))
            GetProcAddress(dll, "GetMonitorCapabilities");

        if (!(myGetPhysMonitorsFromHM && myGetMonitorBrightness
        && mySetMonitorBrightness && myDestroyPhysicalMonitors
        && myGetNumberOfPhysmons && myGetMonitorCapabilities))
            goto fail;

        LOG("We got Monitor Brightness functions!");
        hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

        if (!hmon || !myGetNumberOfPhysmons(hmon, &numpm) || !numpm) {
            LOG("GetNumberOfPhysmons(%x) failed", (UINT)(UINT_PTR)hmon);
            goto fail;
        }

        LOG("NumberOfPhysmons=%lu", numpm);
        pm = (PHYSICAL_MONITOR *)calloc(numpm, sizeof(PHYSICAL_MONITOR));
        if( !pm ) goto fail;
        pm->szPhysicalMonitorDescription[0] = '\0';
        if (!myGetPhysMonitorsFromHM(hmon, numpm, pm)) {
            LOG("Unable to get PhysicalMonitor");
            goto fail;
        }
        LOG( "Physical Monitor=%x, %ls", pm->hPhysicalMonitor, pm->szPhysicalMonitorDescription);

        ok = myGetMonitorCapabilities(pm->hPhysicalMonitor, &dwMCap, &wdColtemp);
        LOG("GetMonitorCapabilities()=%d => CAP=%lx", ok, dwMCap);
        if (ok && MC_CAPS_BRIGHTNESS & dwMCap) {
            DWORD min=0, cur=0, max=0;
            ok = myGetMonitorBrightness(pm->hPhysicalMonitor, &min, &cur, &max);
            LOG("GetMonitorBrightness()=%d", ok);
            LOG("Brightness of %ls: min=%lu, cur=%lu, max=%lu" , pm->szPhysicalMonitorDescription, min, cur, max);
            if (ok) {
                int step = max(1, (max-min)/20);
                int newbr = cur + (delta>0? step: -step);
                newbr = CLAMP(min, newbr, max);
                ok = mySetMonitorBrightness(pm->hPhysicalMonitor, newbr);
                LOG( "SetMonitorBrightness(%x, %d)=%d", (UINT)(UINT_PTR)pm->hPhysicalMonitor, newbr, ok);
            }
        }

        ok = myDestroyPhysicalMonitors(numpm, pm);
        LOG( "DestroyPhysicalMonitors(%lu, %x)=%d", numpm, (UINT)(UINT_PTR)pm, ok);
        free(pm);

        fail:
        LOG("Free DXVA2.DLL");
        FreeLibrary(dll);
    }
#endif //NO_BRIGHTNESS
}

/////////////////////////////////////////////////////////////////////////////
static HCURSOR CursorToDraw()
{
    HCURSOR cursor;

    if (conf.UseCursor == 3) {
        return LoadCursor(NULL, IDC_ARROW);
    }
    if (state.action == AC_MOVE) {
        if (conf.UseCursor == 4)
            return LoadCursor(NULL, IDC_SIZEALL);
        cursor = LoadCursor(NULL, conf.UseCursor>1? IDC_ARROW: IDC_HAND);
        if (!cursor) cursor = LoadCursor(NULL, IDC_ARROW); // Fallback;
        return cursor;
    }

    if ((state.resize.y == RZ_TOP && state.resize.x == RZ_LEFT)
     || (state.resize.y == RZ_BOTTOM && state.resize.x == RZ_RIGHT)) {
        return LoadCursor(NULL, IDC_SIZENWSE);
    } else if ((state.resize.y == RZ_TOP && state.resize.x == RZ_RIGHT)
     || (state.resize.y == RZ_BOTTOM && state.resize.x == RZ_LEFT)) {
        return LoadCursor(NULL, IDC_SIZENESW);
    } else if ((state.resize.y == RZ_TOP && state.resize.x == RZ_XCENTER)
     || (state.resize.y == RZ_BOTTOM && state.resize.x == RZ_XCENTER)) {
        return LoadCursor(NULL, IDC_SIZENS);
    } else if ((state.resize.y == RZ_YCENTER && state.resize.x == RZ_LEFT)
     || (state.resize.y == RZ_YCENTER && state.resize.x == RZ_RIGHT)) {
        return LoadCursor(NULL, IDC_SIZEWE);
    } else {
        return LoadCursor(NULL, IDC_SIZEALL);
    }
}
static void UpdateCursor(POINT pt)
{
    // Update cursor
    if (conf.UseCursor && g_mainhwnd) {
        SetClassLongPtr(g_mainhwnd, GCLP_HCURSOR, (LONG_PTR)CursorToDraw());
        ShowWindow(g_mainhwnd, SW_SHOWNA);
        SetWindowPos(g_mainhwnd, HWND_TOPMOST, pt.x-8, pt.y-8, 16, 16
            , SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE|SWP_NOSENDCHANGING);

    }
}

static int IsMXRolled(HWND hwnd, const RECT *rc)
{
    MONITORINFO mi;
    GetMonitorInfoFromWin(hwnd, &mi);
    // Consider the window rolled if its height less than a quarter of monitors
    return (rc->bottom - rc->top) < (mi.rcWork.bottom - mi.rcWork.top) / 4;
}
/////////////////////////////////////////////////////////////////////////////
// Roll/Unroll Window. If delta > 0: Roll if < 0: Unroll if == 0: Toggle.
static void RollWindow(HWND hwnd, int delta)
{
    // if the window is maximized do a spetial
    // treatement with no restore flags
    RECT rc;
    GetWindowRect(hwnd, &rc);

    if (IsZoomed(hwnd)) {
        int ismxrolled = IsMXRolled(hwnd, &rc);
        if (delta <= 0 && ismxrolled) {
            // Unroll Maximized window
            WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwnd, &wndpl);
            wndpl.showCmd = SW_SHOWMINIMIZED;
            SetWindowPlacement(hwnd, &wndpl);
            wndpl.showCmd = SW_SHOWMAXIMIZED;
            SetWindowPlacement(hwnd, &wndpl);
        } else if(delta >= 0 && !ismxrolled) {
            // Roll maximized window
            SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left
                  , GetSystemMetrics(SM_CYMIN)
                  , SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_ASYNCWINDOWPOS);

        }
        return;
    }
    // Handle non maximized windows
    unsigned restore = GetRestoreFlag(hwnd);

    if (restore&ROLLED && delta <= 0) { // UNROLL
        // Restore the Old height
        // Set origin width and height to the saved values
        int width, height;
        GetRestoreData(hwnd, &width, &height);
        width = rc.right - rc.left; // keep current width
        ClearRestoreData(hwnd);
         SetWindowPos(hwnd, NULL, 0, 0, width, height
                , SWP_NOSENDCHANGING|SWP_NOZORDER|SWP_NOMOVE|SWP_ASYNCWINDOWPOS);

    } else if (((!(restore&ROLLED) && delta == 0)) || delta > 0 ) { // ROLL
        if (!(restore & ROLLED)) { // Save window size if not saved already.
            SetRestoreData(hwnd, rc.right - rc.left, rc.bottom - rc.top, 0);
            // Add the SNAPPED falg is maximized and and add the SNTHENROLLED flag is snapped
            SetRestoreFlag(hwnd, ROLLED|IsWindowSnapped(hwnd)<<10);
        }
        SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left
              , GetSystemMetrics(SM_CYMIN)
              , SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_ASYNCWINDOWPOS);
    }
}

static int ActionZoom(HWND hwnd, short delta, short center)
{
    if(!IsResizable(hwnd)) return 0;

    RECT rc, orc;
    GetWindowRect(hwnd, &rc);
    if (IsZoomed(hwnd)) RestoreToMonitorSize(hwnd, &rc);
    ClearRestoreFlagOnResizeIfNeeded(hwnd);
    SetEdgeAndOffset(&rc, state.prevpt);
    OffsetRectMDI(&rc);
    CopyRect(&orc, &rc);

    int left=0, right=0, top=0, bottom=0;
    int div = state.shift ? conf.ZoomFracShift: conf.ZoomFrac;
    // We increase/decrease at least 1 pixel or SnapThreshold/2.
    UCHAR T = state.shift? 1: conf.SnapThreshold/2+1;
    UCHAR CT = !center ^ !state.ctrl
             || (state.resize.x == RZ_XCENTER && state.resize.y == RZ_YCENTER);

    if (!conf.AutoSnap
    || (CT && (state.resize.x == RZ_XCENTER || state.resize.y == RZ_YCENTER)) )
    {   // Try to conserve a bit better the aspect ratio when in center mode.
        T = 1; // Or when no snapping has to occur.
    }

    GetMinMaxInfo(hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions

    if (state.resize.x == RZ_LEFT) {
        right = max(T, (rc.right-rc.left)/div);
        state.resize.x = RZ_RIGHT;
    } else if (state.resize.x == RZ_RIGHT) {
        left  = max(T, (rc.right-rc.left)/div);
        state.resize.x = RZ_LEFT;
    } else if (state.resize.x == RZ_XCENTER && CT) {
        left  = max(T, (state.prevpt.x-rc.left)  /div);
        right = max(T, (rc.right-state.prevpt.x) /div);
    }

    if (state.resize.y == RZ_TOP) {
        bottom = max(T, (rc.bottom-rc.top)/div);
        state.resize.y = RZ_BOTTOM;
    } else if (state.resize.y == RZ_BOTTOM) {
        top    = max(T, (rc.bottom-rc.top)/div);
        state.resize.y = RZ_TOP;
    } else if (state.resize.y == RZ_YCENTER && CT) {
        top   = max(T, (state.prevpt.y-rc.top)   /div);
        bottom= max(T, (rc.bottom-state.prevpt.y)/div);
    }
    if (delta < 0) {
        // Zoom out
        rc.left += left;
        rc.right -= right;
        rc.top += top;
        rc.bottom -= bottom;
    } else {
        // Zoom in
        rc.left -= left;
        rc.right +=right;
        rc.top -= top;
        rc.bottom += bottom;
    }

    int x = rc.left, y = rc.top, width = rc.right-rc.left, height= rc.bottom-rc.top;

    state.hwnd = hwnd;
    state.snap = conf.AutoSnap; // Only use autosnap setting.

    ResizeSnap(&x, &y, &width, &height
              , min( max(left, right)-1, conf.SnapThreshold )  // initial x threshold
              , min( max(top, bottom)-1, conf.SnapThreshold ) );// initial y threshold
    // Make sure that the windows does not move
    // in case it is resized from bottom/right
    if (state.resize.x == RZ_LEFT) x = x+width - CLAMPW(width);
    if (state.resize.y == RZ_TOP) y = y+height - CLAMPH(height);
    // Avoid runaway effect when zooming in/out too much.
    if (state.resize.x == RZ_XCENTER && !ISCLAMPEDW(width)) x = orc.left;
    if (state.resize.y == RZ_YCENTER && !ISCLAMPEDH(height)) y = orc.top;
    width = CLAMPW(width); // Double check
    height = CLAMPH(height);

    MoveWindowAsync(hwnd, x, y, width, height);
    return 1;
}
static int IsDoubleClick(int button)
{ // Never validate a double-click if the click has to pierce
    return !conf.PiercingClick && state.clickbutton == button
        && GetTickCount()-state.clicktime <= GetDoubleClickTime();
}
/////////////////////////////////////////////////////////////////////////////
static int ActionMove(POINT pt, int button)
{
    // If this is a double-click
    if (IsDoubleClick(button)) {
        SetOriginFromRestoreData(state.hwnd, AC_MOVE);
        LastWin.hwnd = NULL;
        if (state.shift) {
            RollWindow(state.hwnd, 0); // Roll/Unroll Window...
        } else if (state.ctrl) {
            MinimizeWindow(state.hwnd);
        } else if (state.resizable) {
            // Toggle Maximize window
            state.action = AC_NONE; // Stop move action
            state.clicktime = 0; // Reset double-click time
            state.blockmouseup = 1; // Block the mouseup, otherwise it can trigger a context menu
            ToggleMaxRestore(state.hwnd);
        }
        // Prevent mousedown from propagating
        return 1;
    } else if (conf.MMMaximize&2) {
        MouseMove(pt); // Restore with simple Click
    } else if (conf.DragThreshold >= 2 || (conf.DragThreshold == 1 && IsAnySnapped(state.hwnd))) {
        // Wait some move threshold before drag...
        // If the Window was maximized or if the user enabled the option.
        state.moving = DRAG_WAIT;
    }

    return -1;
}
static void SetEdgeAndOffsetCF(const RECT *wnd, const UCHAR centerfrac, POINT pt)
{
    // Set edge and offset
    // Think of the window as nine boxes (corner regions get 38%, middle only 24%)
    // Does not use state.origin.width/height since that is based on wndpl.rcNormalPosition
    // which is not what you see when resizing a window that Windows Aero resized
    int wndwidth  = wnd->right  - wnd->left;
    int wndheight = wnd->bottom - wnd->top;
    int SideS = (100-centerfrac)/2;
    int CenteR = 100-SideS;

    if (pt.x-wnd->left < (wndwidth*SideS)/100) {
        state.resize.x = RZ_LEFT;
        state.offset.x = pt.x-wnd->left;
    } else if (pt.x-wnd->left < (wndwidth*CenteR)/100) {
        state.resize.x = RZ_XCENTER;
        state.offset.x = pt.x-state.mdipt.x; // Used only if both x and y are CENTER
    } else {
        state.resize.x = RZ_RIGHT;
        state.offset.x = wnd->right-pt.x;
    }
    if (pt.y-wnd->top < (wndheight*SideS)/100) {
        state.resize.y = RZ_TOP;
        state.offset.y = pt.y-wnd->top;
    } else if (pt.y-wnd->top < (wndheight*CenteR)/100) {
        state.resize.y = RZ_YCENTER;
        state.offset.y = pt.y-state.mdipt.y;
    } else {
        state.resize.y = RZ_BOTTOM;
        state.offset.y = wnd->bottom-pt.y;
    }
    // Set window right/bottom origin
    state.origin.right = wnd->right-state.mdipt.x;
    state.origin.bottom = wnd->bottom-state.mdipt.y;
}

// Use disgonals to determine the closest side.
static void SetEdgeToClosestSide(const RECT *wnd, const POINT pt)
{
    const int W = wnd->right - wnd->left;
    const int H = wnd->bottom - wnd->top;
    const int x = pt.x - wnd->left;
    const int y = pt.y - wnd->top;
    char TR = y * W     <= H * x; // T/C or R/C mode
    char TL = (H-y) * W >= H * x; // B/C or C/C mode
    if (TR) { // Top or right
        if (TL) {
            state.resize.y = RZ_TOP;
            state.offset.y = pt.y-wnd->top;
        } else {
            state.resize.x = RZ_RIGHT;
            state.offset.x = wnd->right-pt.x;
        }
    } else { // Bottom or Left
        if (TL) { // Bottom right
            state.resize.x = RZ_LEFT;
            state.offset.x = pt.x-wnd->left;
        } else {
            state.resize.y = RZ_BOTTOM;
            state.offset.y = wnd->bottom-pt.y;
        }
    }
}
static void SetEdgeAndOffset(const RECT *wnd, const POINT pt)
{
    SetEdgeAndOffsetCF(wnd, conf.CenterFraction, pt);
    if (conf.SidesFraction == conf.CenterFraction) {
        // Classic 9 quadrent mode
        return;
    }
    // Special mode.
    if (state.resize.x != RZ_XCENTER || state.resize.y != RZ_YCENTER) {
        SetEdgeAndOffsetCF(wnd, conf.SidesFraction, pt);
        if (conf.SidesFraction < conf.CenterFraction) {
        } else { // (SidesFraction > CenterFraction)
            SetEdgeToClosestSide(wnd, pt);
        }
    }


}

static void NextBorders(RECT *pos, const RECT *cur, const RECT *def)
{
    unsigned i;
    CopyRect(pos, def);
    for (i=0; i<numsnwnds; i++) {
        const RECT *rc = &snwnds[i].wnd;
        const unsigned flg = snwnds[i].flag;
        POINT tpt;
        tpt.x =  (rc->left+rc->right)/2; tpt.y = (rc->top+rc->bottom)/2;
        if (!PtInRect(def, tpt) )
            continue;

        if (flg&(SNZONE|SNLEFT)   && rc->right < cur->left) pos->left   = max(pos->left, rc->right);
        if (flg&(SNZONE|SNTOP)    && rc->bottom < cur->top) pos->top    = max(pos->top, rc->bottom);
        if (flg&(SNZONE|SNRIGHT)  && rc->left > cur->right) pos->right  = min(pos->right, rc->left);
        if (flg&(SNZONE|SNBOTTOM) && rc->top > cur->bottom) pos->bottom = min(pos->bottom, rc->top);
    }
}
#define SNTO_CNSNAP 0
#define SNTO_EXTEND 1
#define SNTO_NEXTBD 2
#define SNTO_MOVETO 4
static void SnapToCorner(HWND hwnd, struct resizeXY resize, UCHAR flags)
{
    // When trying to Snap or extend a non-resizeable window
    if (!(flags&SNTO_MOVETO) && !IsResizable(hwnd))
        flags = SNTO_MOVETO | SNTO_NEXTBD; // Move to next bd instead

    SetOriginFromRestoreData(hwnd, AC_MOVE);
    GetMinMaxInfo(hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions

    // Get and set new position
    int posx, posy; // wndwidth and wndheight are defined above
    unsigned restore = 1;
    RECT *mon = &state.origin.mon;
    RECT bd, wnd;
    GetWindowRect(hwnd, &wnd);
    SetEdgeAndOffset(&wnd, state.prevpt); // state.resize.x/y & state.offset.x/y
    if (!resize.x && !resize.y)
        resize = state.resize;
    FixDWMRect(hwnd, &bd);
    int wndwidth  = wnd.right  - wnd.left;
    int wndheight = wnd.bottom - wnd.top;

    RECT tmp;

    if (flags & SNTO_NEXTBD) {
        // Find next borders in each direction and set it up as monitor.
        EnumSnapped();
        RECT vwnd;
        GetWindowRectL(hwnd, &vwnd);
        NextBorders(&tmp, &vwnd, mon);
        mon = &tmp;
    }
    if (flags & SNTO_EXTEND) {
    /* Extend window's borders to monitor/Next border */
        posx = wnd.left - state.mdipt.x;
        posy = wnd.top - state.mdipt.y;

        if (resize.y == RZ_TOP) {
            posy = mon->top - bd.top;
            wndheight = CLAMPH(wnd.bottom-state.mdipt.y - mon->top + bd.top);
        } else if (resize.y == RZ_BOTTOM) {
            wndheight = CLAMPH(mon->bottom - wnd.top+state.mdipt.y + bd.bottom);
        }
        if (resize.x == RZ_RIGHT) {
            wndwidth =  CLAMPW(mon->right - wnd.left+state.mdipt.x + bd.right);
        } else if (resize.x == RZ_LEFT) {
            posx = mon->left - bd.left;
            wndwidth =  CLAMPW(wnd.right-state.mdipt.x - mon->left + bd.left);
        } else if (resize.x == RZ_XCENTER && resize.y == RZ_YCENTER) {
            wndwidth = CLAMPW(mon->right - mon->left + bd.left + bd.right);
            posx = mon->left - bd.left;
            posy = wnd.top - state.mdipt.y + bd.top ;
            restore |= SNMAXW;
        }
    } else if (flags & SNTO_MOVETO) {
    /* Move the windows to a border (monitor or next border) */
        posx = wnd.left - state.mdipt.x;
        posy = wnd.top - state.mdipt.y;
        if (resize.x == RZ_LEFT) {
            posx =  mon->left - bd.left;
        } else if (resize.x == RZ_RIGHT) {
            posx = mon->right + bd.right - wndwidth;
        }
        if (resize.y == RZ_TOP) {
            posy =  mon->top - bd.top;
        } else if (resize.y == RZ_BOTTOM) {
            posy = mon->bottom + bd.bottom - wndheight;
        }
    } else { /* Aero Snap to corresponding side/corner */
        int leftWidth, rightWidth, topHeight, bottomHeight;
        // EnumSnapped();
        GetAeroSnappingMetrics(&leftWidth, &rightWidth, &topHeight, &bottomHeight, mon);
        wndwidth =  leftWidth;
        wndheight = topHeight;
        posx = mon->left;
        posy = mon->top;
        restore = SNTOPLEFT;

        if (resize.y == RZ_YCENTER) {
            wndheight = CLAMPH(mon->bottom - mon->top); // Max Height
            posy += (mon->bottom - mon->top)/2 - wndheight/2;
            restore &= ~SNTOP;
        } else if (resize.y == RZ_BOTTOM) {
            wndheight = bottomHeight;
            posy = mon->bottom - wndheight;
            restore &= ~SNTOP;
            restore |= SNBOTTOM;
        }

        if (resize.x == RZ_XCENTER && resize.y != RZ_YCENTER) {
            wndwidth = CLAMPW( (mon->right-mon->left) ); // Max width
            posx += (mon->right - mon->left)/2 - wndwidth/2;
            restore &= ~SNLEFT;
        } else if (resize.x == RZ_XCENTER) {
            restore &= ~SNLEFT;
            if(resize.y == RZ_YCENTER) {
                restore |= SNMAXH;
                if(state.ctrl) {
                    LastWin.hwnd = NULL;
                    ToggleMaxRestore(hwnd);
                    return;
                }
            }
            wndwidth = wnd.right - wnd.left - bd.left - bd.right;
            posx = wnd.left - state.mdipt.x + bd.left;
        } else if (resize.x == RZ_RIGHT) {
            wndwidth = rightWidth;
            posx = mon->right - wndwidth;
            restore |= SNRIGHT;
            restore &= ~SNLEFT;
        }
        // FixDWMRect
        posx -= bd.left; posy -= bd.top;
        wndwidth += bd.left+bd.right; wndheight += bd.top+bd.bottom;
    }
    MoveWindowAsync(hwnd, posx, posy, wndwidth, wndheight);
    // Save data to the window...
    if ( !(GetRestoreFlag(hwnd)&SNAPPED) )
        SetRestoreData(hwnd, state.origin.width, state.origin.height, SNAPPED|restore);
}

/////////////////////////////////////////////////////////////////////////////
static int ActionResize(POINT pt, const RECT *wnd, int button)
{
    if(!state.resizable) {
        state.action = AC_NONE;
        return 0;// Next Hook
    }
    // Aero-move this window if this is a double-click
    if (IsDoubleClick(button)) {
        state.action = AC_NONE; // Stop resize action
        SnapToCorner(state.hwnd, AUTORESIZE, !state.shift ^ !(conf.AeroTopMaximizes&2));
        state.blockmouseup = 1; // Block mouse up (context menu would pop)
        state.clicktime = 0;    // Reset double-click time
        // Prevent mousedown from propagating
        return 1;
    }
    SetEdgeAndOffset(wnd, pt);
    if (state.resize.y == RZ_YCENTER && state.resize.x == RZ_XCENTER) {
        if (conf.ResizeCenter == 0) {
            // Use Bottom Right Mode
            state.resize.x = RZ_RIGHT;
            state.resize.y = RZ_BOTTOM;
            state.offset.y = wnd->bottom-pt.y;
            state.offset.x = wnd->right-pt.x;
        } else if (conf.ResizeCenter == 2) {
            // Switch to Move action
            state.action = AC_MOVE;
        } else if (conf.ResizeCenter == 3) {
            // Use diagonals to select pure L/C R/C T/C B/C
            SetEdgeToClosestSide(wnd, pt);
        }
    }
    if (conf.DragThreshold >= 3) {
        state.moving = DRAG_WAIT;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////////////////
static void ActionBorderless(HWND hwnd)
{
    LONG_PTR style=0;
    // Get anc clear eventual old style.
    LONG_PTR ostyle = ClearBorderlessFlag(hwnd);
    if (ostyle) {
        // Restore old style if found
        style = ostyle;
    } else {
        style=GetWindowLongPtr(hwnd, GWL_STYLE);
        if (!style) return;
        SetBorderlessFlag(hwnd, style); // Save style
        if((style&WS_CAPTION) == WS_CAPTION || style&WS_THICKFRAME) {
            style &= state.shift? ~WS_CAPTION: ~(WS_CAPTION|WS_THICKFRAME);
        } else {
            style |= WS_OVERLAPPEDWINDOW;
        }
    }
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Under Windows 10, with DWM we HAVE to resize the windows twice
    // to have proper drawing. this is a bug.
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top+1
               , SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_FRAMECHANGED|SWP_NOZORDER);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top
               , SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_NOZORDER);
}
/////////////////////////////////////////////////////////////////////////////
#define CW_RESTORE (1<<0)
#define CW_TRIM    (1<<1)
static void CenterWindow(HWND hwnd, unsigned flags)
{
    RECT mon;// = state.origin.mon;
    POINT pt;
    int width, height;
    if (flags & CW_RESTORE) {
        SetOriginFromRestoreData(hwnd, AC_MOVE);
        width = state.origin.width;
        height = state.origin.height;
    } else {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        width = rc.right - rc.left;
        height = rc.bottom - rc.top;
    }
    GetCursorPos(&pt);
    GetMonitorRect(&pt, 0, &mon);


    int x = mon.left+ ((mon.right-mon.left)-width)/2;
    int y = mon.top + ((mon.bottom-mon.top)-height)/2;

    if (flags & CW_TRIM) {
        // Trim the window to the current monitor
        if (x < mon.left) {
            x = mon.left;
            width = mon.right - mon.left;
        }

        if (y < mon.top) {
            y = mon.top;
            height = mon.bottom - mon.top;
        }
    }
    MoveWindowAsync(hwnd, x, y, width, height);
}

//#define EVENT_HOOK
// TODO: Event Hook
#ifdef EVENT_HOOK
static HWND GetPinWindow(HWND owner);
static BOOL CALLBACK PostPinWindowsProcMessage(HWND hwnd, LPARAM lp);
static void CALLBACK HandleWinEvent(
    HWINEVENTHOOK hook, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild,
    DWORD dwEventThread, DWORD dwmsEventTime)
{
    HWND pinhwnd = (HWND) hook; // 1st param...
    if (hwnd
    && (event == EVENT_OBJECT_DESTROY || event == EVENT_OBJECT_LOCATIONCHANGE)
    && idChild == INDEXID_CONTAINER
    && idObject == OBJID_WINDOW) {
        if (pinhwnd && GetParent(pinhwnd) == hwnd) {
            //TCHAR txt[32];
            //MessageBox(NULL, itostr(event, txt, 16), NULL, 0);
            PostMessage(pinhwnd, WM_TIMER, 0, 0);
        }
    }
}
#endif // EVENT_HOOK

// Used with TrackMenuOfWindows
#define TRK_LASERMODE (1<<0)
#define TRK_MOVETOMONITOR (1<<1)

static void TrackMenuOfWindows(WNDENUMPROC EnumProc, LPARAM flags);
/////////////////////////////////////////////////////////////////////////////
// Pin window callback function
// We store the old owner window style in GWLP_USERDATA
// and the rightoffset/topoffset in the first LONG_PTR of cbWndExtra (+0)
static LRESULT CALLBACK PinWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
    case WM_CREATE: {
#ifdef EVENT_HOOK
// TODO: Event Hook
        HWND owner = GetWindow(hwnd, GW_OWNER);
        DWORD lpdwProcessId;
        DWORD threadid = GetWindowThreadProcessId(owner, &lpdwProcessId);
        int prop = SetProp(owner, TEXT(APP_NAMEA)TEXT("-Pin"), NULL);
        RemoveProp(owner,  TEXT(APP_NAMEA)TEXT("-Pin"));

        if (prop && threadid && lpdwProcessId) {
            #if defined(_M_AMD64) || defined(WIN64)
                #define THUNK_SIZE 22
            #else
                #define THUNK_SIZE 13
            #endif
            BYTE *thunk = (BYTE*)VirtualAlloc( NULL, THUNK_SIZE, MEM_COMMIT, PAGE_READWRITE );
            if (!thunk) break;
            // Replace the first parameter with the handle of the PinWindow.
            // FIXME: Handle MIPS/Alpha/PowerPC/ia-64/ARM/ARM64...
            #if defined(_M_AMD64) || defined(WIN64)
                // AMD x86_64 windows : rcx, rdx, r8, r9 (ints).
                // xmm0, xmm1, xmm2, xmm3, floating points
                // Then push right to left.
                // ----------------------------------
                // ; mov rcx hwnd
                // ; mov rax Procedure
                // ; jmp rax
                *(WORD  *)(thunk+ 0) = 0xB948;
                *(HWND  *)(thunk+ 2) = hwnd; // <- Replace 1st param
                *(WORD  *)(thunk+10) = 0xB848;
                *(void **)(thunk+12) = (LONG_PTR*)HandleWinEvent;
                *(WORD  *)(thunk+20) = 0xE0FF;
            #elif defined(_M_IX86)
                // i386 __stdcall: push right to left
                // ----------------------------------
                // ; mov dword ptr [esp+4] hwnd
                // ; jmp Procedure
                *(DWORD *)(thunk+0) = 0x042444C7;
                *(HWND  *)(thunk+4) = hwnd; // <- Replace 1st param
                *(BYTE  *)(thunk+8) = 0xE9;
                *(DWORD *)(thunk+9) = (BYTE*)((void*)HandleWinEvent)-(thunk+13);
            #else
                #error Unsupported CPU type, implement thunking or undefine EVENT_HOOK.
            #endif
            DWORD oldprotect;
            // Restrict thunk to execute only.
            BOOL ret = VirtualProtect(thunk, THUNK_SIZE, PAGE_EXECUTE, &oldprotect);
            if (!ret) break;
            FlushInstructionCache(GetCurrentProcess(), thunk, THUNK_SIZE);

            HWINEVENTHOOK hook =
            SetWinEventHook(
                EVENT_OBJECT_DESTROY, EVENT_OBJECT_LOCATIONCHANGE, // Range of events=8001-800Bh
                NULL, // Handle to DLL.
                (WINEVENTPROC)thunk, // The callback function (thunked)
                lpdwProcessId, threadid, // Process and thread IDs of interest (0 = all)
                WINEVENT_OUTOFCONTEXT // Flags.
                );
            SetWindowLongPtr(hwnd, 0+sizeof(LONG_PTR), (LONG_PTR)hook); // save hook
            SetWindowLongPtr(hwnd, 0+2*sizeof(LONG_PTR), (LONG_PTR)thunk); // save thunk
            if (hook) {
                PostMessage(hwnd, WM_TIMER, 0, 0);
                break;
            }
        }
#endif // EVENT_HOOK
        SetTimer(hwnd, 1, conf.PinRate, NULL);

    } break;
    case WM_DPICHANGED: {
        // Reset the oldstyle, so we have to recalculate size/offset
        // of the pin window on DPI change.
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
    } break;
    case WM_SETTINGCHANGE: {
        if (wp == SPI_SETNONCLIENTMETRICS) {
            // We have to recalculate size/offset on global
            // non client metrics change.
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        }
    } break;
    case WM_TIMER: {
        HWND ow;
        if (!(ow=GetWindow(hwnd, GW_OWNER))) {
            // Owner no longer exists!
            // We have no reasons to be anymore.
            DestroyWindow(hwnd);
            return 0;
        }
        // Destroy the pin if the owner is no longer topmost
        // or no longer exists
        LONG_PTR xstyle;
        if(!IsWindow(ow)
        || !IsWindowVisible(ow)
        || !((xstyle = GetWindowLongPtr(ow, GWL_EXSTYLE))&WS_EX_TOPMOST)) {
            DestroyWindow(hwnd);
            return 0;
        }

        RECT rc; // Owner rect.
        GetWindowRect(ow, &rc);
        LONG_PTR style  = GetWindowLongPtr(ow, GWL_STYLE);
        LONG_PTR OldOwStyle = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (OldOwStyle == style) {
            // the data were saved for the correct style!!!
            LONG_PTR RTOffset = GetWindowLongPtr(hwnd, 0); // get Right and Top offset
            SetWindowPos(hwnd, NULL
                , rc.right-LOWORDPTR(RTOffset), rc.top+HIWORDPTR(RTOffset), 0, 0
                , SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOSENDCHANGING);
            return 0; // DONE!
        }

        // Calculate offsets, sets window position and save data
        // to the GWLP_USERDATA stuff!
        UINT dpi = GetDpiForWindow(hwnd); // Use pin's hwnd
        int CapButtonWidth, PinW, PinH;
        PinW = GetSystemMetricsForDpi(SM_CXSIZE, dpi); // Caption button width
        PinH = GetSystemMetricsForDpi(SM_CYSIZE, dpi); // Caption button height

        int bdx=0, bdy=0;
        if (style&WS_THICKFRAME) {
            bdx = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
            bdy = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);
        } else if (style&WS_CAPTION) { // Caption or border
            bdx = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
            bdy = GetSystemMetricsForDpi(SM_CYFIXEDFRAME, dpi);
        }
        // Vista/7/8.x/10 extra padding Not working???
        // bdy += GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

        RECT btrc;
        if (GetCaptionButtonsRect(ow, &btrc)) {
            CapButtonWidth = btrc.right - btrc.left;
            // bdx = rc.right - btrc.right;
        } else {
            UCHAR btnum=0; // Number of caption buttons.
            if ((style&(WS_SYSMENU|WS_CAPTION)) == (WS_SYSMENU|WS_CAPTION)) {
                btnum++;                         // WS_SYSMENU => Close button [X]
                btnum += 2 * !!(style&(WS_MINIMIZEBOX|WS_MAXIMIZEBOX)); // [O] [_]
                btnum += ( (xstyle&WS_EX_CONTEXTHELP)                       // [?]
                          && (style&(WS_MINIMIZEBOX|WS_MAXIMIZEBOX)) != (WS_MINIMIZEBOX|WS_MAXIMIZEBOX) );
            }
            CapButtonWidth = btnum * PinW;
        }
        // Adjust PinW and PinH to have nice stuff.
        PinW -= 2;
        PinH -= 2;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, style); // Save old window style
        int rightoffset = CapButtonWidth+PinW+bdx+GetSystemMetricsForDpi(SM_CXBORDER, dpi)*2;
        int topoffset = bdy+1;
        // Cache local hwnd storage for pin win offsets in the first LONG_PTR of cbWndExtra
        SetWindowLongPtr(hwnd, 0, MAKELONGPTR(rightoffset,topoffset));
        // Move and size the window...
        SetWindowPos(hwnd, NULL
            , rc.right-rightoffset, rc.top+topoffset, PinW, PinH
            , SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING);
        return 0;
    } break;
    case WM_PAINT: {
        TCHAR Topchar = HIBYTE(HIWORD(conf.PinColor&0xFF000000));
        if (Topchar) {
            if(!GetUpdateRect(hwnd, NULL, FALSE))
                return 0;
            PAINTSTRUCT ps;
            RECT cr;
            GetClientRect(hwnd, &cr);
            BeginPaint(hwnd, &ps);
            SetBkMode(ps.hdc, TRANSPARENT);
            DrawText(ps.hdc, &Topchar, 1, &cr, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
            EndPaint(hwnd, &ps);
            return 0;
      }
    } break;
    case WM_LBUTTONUP: {
        DestroyWindow(hwnd);
        return 0;
    } break;
    case WM_RBUTTONUP: {
        state.mdiclient = NULL; // In case...
        TrackMenuOfWindows(EnumTopMostWindows, 0);
        return 0;
    } break;
//    case WM_GETPINNEDHWND: {
//        // Returns the handle to the topmost window
//        HWND ow = GetWindow(hwnd, GW_OWNER);
//        if(ow && IsWindow(ow)) return (LRESULT)ow;
//    } break;
    case WM_DESTROY: {
        KillTimer(hwnd, 1);

// TODO: Event Hook
#ifdef EVENT_HOOK
        HWINEVENTHOOK hook = (HWINEVENTHOOK)GetWindowLongPtr(hwnd, 0+sizeof(LONG_PTR));
        if (hook) UnhookWinEvent(hook);
        // Free the thunk.
        BYTE *thunk = (BYTE *)GetWindowLongPtr(hwnd, 0+2*sizeof(LONG_PTR));
        if (thunk) VirtualFree(thunk, 0, MEM_RELEASE);
#endif // EVENT_HOOK
        // Remove topmost flag if the pin gets destroyed.
        HWND ow;
        if((ow=GetWindow(hwnd, GW_OWNER))
        && IsWindow(ow)
        && (GetWindowLongPtr(ow, GWL_EXSTYLE)&WS_EX_TOPMOST) )
            SetWindowLevel(ow, HWND_NOTOPMOST);
    } break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
static HWND CreatePinWindow(const HWND owner)
{
    WNDCLASSEX wnd;
    if(!GetClassInfoEx(hinstDLL, TEXT(APP_NAMEA)TEXT("-Pin"), &wnd)) {
        // Register the class if no already created.
        mem00(&wnd, sizeof(wnd));
        wnd.cbSize = sizeof(WNDCLASSEX);
        wnd.style = CS_NOCLOSE|CS_HREDRAW|CS_VREDRAW;
        wnd.lpfnWndProc = PinWindowProc;
#ifdef EVENT_HOOK
        // We need some data to save the thunk and event hook.
        wnd.cbWndExtra = sizeof(LONG_PTR) * 3;
#else
        wnd.cbWndExtra = sizeof(LONG_PTR);
#endif
        wnd.hInstance = hinstDLL;
        wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
        wnd.hbrBackground = CreateSolidBrush(conf.PinColor&0x00FFFFFF);
        wnd.lpszClassName =  TEXT(APP_NAMEA)TEXT("-Pin");
        RegisterClassEx(&wnd);
    }
    HWND ret = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST
                   , TEXT(APP_NAMEA)TEXT("-Pin"), NULL
                   , WS_POPUP|WS_BORDER /* Start invisible */
                   , 0, 0, 0, 0
                   , owner, NULL, hinstDLL, NULL);
    // Show pin window without activating it to avoid focus loss.
    ShowWindow(ret, SW_SHOWNA);
    return ret;
}
static BOOL CALLBACK PostPinWindowsProcMessage(HWND hwnd, LPARAM lp)
{
    if(isClassName(hwnd, TEXT(APP_NAMEA)TEXT("-Pin"))) {
        if(lp == WM_CLOSE)
            DestroyWindow(hwnd);
        else
            PostMessage(hwnd, lp, 0, 0);
    }

    return TRUE; // Next hwnd
}
static BOOL CALLBACK FindPinWinProc(HWND hwnd, LPARAM lp)
{
    HWND *inouth = (HWND *)lp;
    if(GetWindow(hwnd, GW_OWNER) == inouth[1]) {
        inouth[0] = hwnd;
        return FALSE;
    }
    return TRUE;
}
static HWND GetPinWindow(HWND owner)
{
    HWND inouth[2] = { NULL, owner };
    EnumThreadWindows(GetCurrentThreadId(), FindPinWinProc, (LPARAM)inouth);
    return inouth[0];
}

/////////////////////////////////////////////////////////////////////////////
static void TogglesAlwaysOnTop(HWND hwnd)
{
    // Use the Root owner
    hwnd = GetRootOwner(hwnd);
    LONG_PTR topmost = GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOPMOST;
    SetWindowLevel(hwnd, topmost? HWND_NOTOPMOST: HWND_TOPMOST);

    if (!topmost) {
        if (conf.TopmostIndicator) CreatePinWindow(hwnd);
    } else {
        // Destroy Pin Window?
        if (conf.TopmostIndicator) DestroyWindow(GetPinWindow(hwnd));
    }
    // Always set foreground #442
    ReallySetForegroundWindow(hwnd);
}
/////////////////////////////////////////////////////////////////////////////
static void ActionMaximize(HWND hwnd)
{
    LastWin.hwnd = NULL;
    if (state.shift) {
        MinimizeWindow(hwnd);
    } else if (IsResizable(hwnd)) {
        ToggleMaxRestore(hwnd);
    }
}
/////////////////////////////////////////////////////////////////////////////
static void MaximizeHV(HWND hwnd, int horizontal)
{
    RECT rc, bd, mon;
    if (!IsResizable(hwnd) || !GetWindowRect(hwnd, &rc)) return;
    OffsetRectMDI(&rc);

    POINT pt;
    GetCursorPos(&pt);
    GetMonitorRect(&pt, 0, &mon);
    SetOriginFromRestoreData(hwnd, AC_MOVE);

    SetRestoreData(hwnd, state.origin.width, state.origin.height, SNAPPED);
    FixDWMRect(hwnd, &bd);
    if (horizontal) {
        SetRestoreFlag(hwnd, SNAPPED|SNMAXW);
        MoveWindowAsync(hwnd
            , mon.left-bd.left
            , rc.top
            , mon.right-mon.left + bd.left+bd.right
            , rc.bottom-rc.top);
    } else { // vertical
        SetRestoreFlag(hwnd, SNAPPED|SNMAXH);
        MoveWindowAsync(hwnd
            , rc.left
            , mon.top - bd.top
            , rc.right - rc.left
            , mon.bottom - mon.top + bd.top+bd.bottom);
    }
}

/////////////////////////////////////////////////////////////////////////////
HWND *minhwnds=NULL;
unsigned minhwnds_alloc=0;
unsigned numminhwnds=0;
struct MinimizeWindowProcParams {
    HMONITOR hMon;
    HWND clickedhwnd;
};
BOOL CALLBACK MinimizeWindowProc(HWND hwnd, LPARAM lParam)
{
    minhwnds = (HWND *)GetEnoughSpace(minhwnds, numminhwnds, &minhwnds_alloc, sizeof(HWND));
    if (!minhwnds) return FALSE; // Stop enum, we failed
    const struct MinimizeWindowProcParams *p = (const struct MinimizeWindowProcParams *) lParam;
    hwnd = GetRootOwner(hwnd);

    if (hwnd != p->clickedhwnd
    // && IsAltTabAble(hwnd) ????
    && IsVisible(hwnd)
    && !IsIconic(hwnd)
    && (WS_MINIMIZEBOX&GetWindowLongPtr(hwnd, GWL_STYLE))){
        if (!p->hMon || p->hMon == MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)) {
            MinimizeWindow(hwnd);
            minhwnds[numminhwnds++] = hwnd;
        }
    }
    return TRUE;
}
static void MinimizeAllOtherWindows(HWND hwnd, int CurrentMonOnly)
{
    static HWND restore = NULL;
    HMONITOR hMon = NULL;
    hwnd = GetRootOwner(hwnd);
//    if (!hwnd) return;

    if (CurrentMonOnly) hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    if (restore == hwnd) {
        // We have to restore all saved windows (minhwnds) when
        // we click again on the same hwnd and have everything saved...
        unsigned i;
        for (i=0; i < numminhwnds; i++) {
            HWND hrest = minhwnds[i];
            if (IsWindow(hrest)
            && IsIconic(hrest)
            && (!hMon || hMon == MonitorFromWindow(hrest, MONITOR_DEFAULTTONEAREST))){
                // Synchronus restoration to keep the order of windows...
                ShowWindow(hrest, SW_RESTORE);
                SetWindowLevel(hrest, HWND_BOTTOM);
            }
        }
        SetForegroundWindowL(hwnd);
        numminhwnds = 0;
        restore = NULL;
    } else {
        struct MinimizeWindowProcParams p = { NULL, hwnd };
        restore = hwnd;
        numminhwnds = 0;
        if (state.mdiclient) {
            EnumChildWindows(state.mdiclient, MinimizeWindowProc, (LPARAM)&p);
        } else {
            p.hMon = hMon;
            EnumDesktopWindows(NULL, MinimizeWindowProc, (LPARAM)&p);
        }
    }
}

static pure BOOL IsRectInMonitors(const RECT *rc)
{
    unsigned i;
    for(i=0; i < nummonitors; i++) {
        int inx = monitors[i].left < rc->right-8 && rc->left+8 < monitors[i].right;
        int iny = monitors[i].top < rc->bottom-8 && rc->top+8 < monitors[i].bottom;
        if (inx && iny) // Windows is inside one of the monitors.
            return TRUE;
    }
    return FALSE;
}
// Move the window step by step to a direction.
// signed step size and direction = 0 for X 1 for Y
static void StepWindow(HWND hwnd, short step, UCHAR direction)
{
    RECT rc;
    if (IsZoomed(hwnd)) {
        //return;
        RestoreToMonitorSize(hwnd, &rc);
    }
    if(!GetWindowRect(hwnd, &rc)) return;
    int x=rc.left, y=rc.top;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    UCHAR threshold = min(abs(step)-1, conf.SnapThreshold);

    if (direction==0) {
        x += step; // x-axis
        if (threshold) MoveSnap(&x, &y, width, height, threshold);
        y = rc.top; // y does not change.
        rc.left = x;
        rc.right = x + width;
    } else {
        y += step; // y-axis;
        if (threshold) MoveSnap(&x, &y, width, height, threshold);
        x = rc.left; // x does not change.
        rc.top = y;
        rc.bottom = y + height;
    }

    if(IsRectInMonitors(&rc)) {
        // Do not move if target rect is not in a monitor.
        UINT flags = SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS;
        if (conf.IgnoreMinMaxInfo) flags |= SWP_NOSENDCHANGING;
        SetWindowPos(hwnd, NULL, x, y, 0, 0, flags);
    }
}
// Make a menu filled with the windows that are enumed through EnumProc
// And Track it!!!!
static TCHAR Int2Accel(int i)
{
    if (conf.NumberMenuItems)
        return i<10? TEXT('0')+i: TEXT('A')+i-10;
    else
        return i<26? TEXT('A')+i: TEXT('0')+i-26;
}
#include <oleacc.h>
struct menuitemdata {
    #ifdef _UNICODE
    MSAAMENUINFO msaa;
    #endif
    TCHAR *txtptr;
    HICON icon;
};
static void MoveToCurrentMonitorIfNeeded(HWND hwnd)
{
    if (state.mdiclient) return;
    RECT rc;
    GetWindowRect(hwnd, &rc);

    POINT pt;
    pt.x = rc.left + (rc.right - rc.left) / 2;
    pt.y = rc.top + (rc.bottom - rc.top) / 2;

    assert(state.origin.monitor != 0);

    // If the window centre is outside the current monitor
    // (use the point instead of window, since MonitorFromWindow
    // returns ones that are only touching, which are still not
    // accessible for the user)
    if (MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) != state.origin.monitor) {
        // Put the window on-screen
        CenterWindow(hwnd, CW_TRIM);
    }
}
static void TrackMenuOfWindows(WNDENUMPROC EnumProc, LPARAM flags)
{
    state.sclickhwnd = NULL;
    KillAltSnapMenu();
    g_mchwnd = KreateMsgWin(MenuWindowProc, TEXT(APP_NAMEA)TEXT("-SClick"), 3);
    if (!g_mchwnd) return; // Failed to create g_mchwnd...
    // Fill up hwnds[] with the stacked windows.
    numhwnds = 0;
    HWND mdiclient = state.mdiclient;
    if (mdiclient) {
        EnumChildWindows(mdiclient, EnumProc, flags & TRK_LASERMODE);
    } else {
        EnumDesktopWindows(NULL, EnumProc, flags & TRK_LASERMODE);
    }
    if (!hwnds) return; // Enum failed

    LOG("Number of stacked windows = %u", numhwnds);
    if(numhwnds == 0) return;

    state.sclickhwnd = state.hwnd;
    numhwnds = min(numhwnds, 36); // Max 36 stacked windows

    HMENU menu = CreatePopupMenu();
    state.unikeymenu = menu;
    unsigned i;
    TCHAR * const failed_string = TEXT("---");

    struct menuitemdata data[36]; // Always fits into the stack
    for (i=0; i < numhwnds; i++) {
        int textlen = GetWindowTextLength(hwnds[i]);
        // Limit the size to the asked width
        if (conf.MaxMenuWidth)
            textlen = min(textlen, conf.MaxMenuWidth);

        // 5 + textlen + 1 * null
        // Allocate some memory
        data[i].txtptr = (TCHAR *)malloc((textlen + 6) * sizeof(TCHAR));
        if (data[i].txtptr) {
            // Allocation succeeded
            TCHAR *txt = data[i].txtptr;
            GetWindowText(hwnds[i], txt+5, textlen + 1);
            txt[0] = TEXT('&');
            txt[1] = Int2Accel(i);
            txt[2] = TEXT(' '); txt[3] = TEXT('-'); txt[4] = TEXT(' ');
        } else {
           // Display static text.
           data[i].txtptr = failed_string;
        }
        // Fill up MSAA structure for screen readers.
        #ifdef _UNICODE // Only available in unicode mode
        data[i].msaa.dwMSAASignature = MSAA_MENU_SIG;
        data[i].msaa.cchWText = lstrlen(data[i].txtptr);
        data[i].msaa.pszWText = data[i].txtptr;
        #endif

        // GetWindowIcon return a shared resource not to be freed
        data[i].icon = GetWindowIcon(hwnds[i]);
        MENUITEMINFO lpmi= { sizeof(MENUITEMINFO) };
        lpmi.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
        lpmi.fType = MFT_OWNERDRAW; /*MFT_STRING*/
        lpmi.wID = i+1; // Id starts at 1 because 0 is for ESCAPE.
        lpmi.dwItemData = (ULONG_PTR)&data[i];
        #ifdef _UNICODE
        lpmi.dwTypeData = (LPWSTR)&data[i].msaa;
        lpmi.cch = sizeof(MSAAMENUINFO);
        #else
        lpmi.dwTypeData=NULL;
        lpmi.cch=0;
        #endif
        InsertMenuItem(menu, i+1, FALSE, &lpmi);
    }
    POINT pt;
    GetCursorPos(&pt);
    ReallySetForegroundWindow(g_mchwnd);
    i = (unsigned)TrackPopupMenu(menu,
        TPM_RETURNCMD/*|TPM_NONOTIFY*/|GetSystemMetrics(SM_MENUDROPALIGNMENT)
        , pt.x, pt.y, 0, g_mchwnd, NULL);
    state.mdiclient = mdiclient;
    LOG("menu=%u", i);
    // if the return value is in the range..
    if (0 < i && i <= numhwnds) {
        HWND hwnd = hwnds[i-1];
//        TCHAR buf[128];
//        PrintHwndDetails(hwnd, buf);
//        MessageBox(NULL, buf, TEXT("Info"), 0);
        if(IsIconic(hwnd))
            RestoreWindow(hwnd);
        SetForegroundWindowL(hwnd);

        if (flags & TRK_MOVETOMONITOR)
            MoveToCurrentMonitorIfNeeded(hwnd);
    }

    DestroyMenu(menu);
    DestroyWindow(g_mchwnd);
    g_mchwnd = NULL;

    // Free strings
    for (i=0; i < numhwnds; i++) {
        if (data[i].txtptr != failed_string)
            free(data[i].txtptr);
    }

    return;
}
static void ActionStackList(int lasermode)
{
    // Just post the message to the Hotkeys message only window
    // So that we do not get stuck in the hook chain.
    PostMessage(g_hkhwnd, WM_STACKLIST, lasermode, (LPARAM)EnumStackedWindowsProc);
}
static void ActionASOnOff()
{
    if (state.action) FinishMovement();
    state.altsnaponoff = !GetProp(g_mainhwnd, APP_ASONOFF);
    SetProp(g_mainhwnd, APP_ASONOFF, (HANDLE)(DorQWORD)state.altsnaponoff);
    PostMessage(g_mainhwnd, WM_UPDATETRAY, 0, 0);
}
static void ActionMoveOnOff(HWND hwnd)
{
    if (GetProp(hwnd, APP_MOVEONOFF))
        RemoveProp(hwnd, APP_MOVEONOFF);
    else
        SetProp(hwnd, APP_MOVEONOFF, (HANDLE)1);
}
static void ActionMenu(HWND hwnd)
{
    state.sclickhwnd = NULL;
    KillAltSnapMenu();
    g_mchwnd = KreateMsgWin(MenuWindowProc, TEXT(APP_NAMEA)TEXT("-SClick"), 1);
    state.sclickhwnd = hwnd;
    // Send message to Open Action Menu
    ReallySetForegroundWindow(g_mainhwnd);
    PostMessage(
        g_mainhwnd, WM_SCLICK, (WPARAM)g_mchwnd,
       ( !state.ignorept )                                    // LP_CURSORPOS
       | !!(GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOPMOST)<<1//LP_TOPMOST
       | !!GetBorderlessFlag(hwnd) << 2                      // LP_BORDERLESS
       | IsZoomed(hwnd) << 3                                  // LP_MAXIMIZED
       | !!(GetRestoreFlag(hwnd)&2) << 4                         // LP_ROLLED
       | !!GetProp(state.hwnd, APP_MOVEONOFF) << 5            // LP_MOVEONOFF
       | (state.alt <= BT_MB5) << 6                         // LP_NOALTACTION
    );
}
// Finds the window that is just next the specified one.
// Works better if they are arranged like in snap layouts.
struct FindTiledWindow_struct {
    POINT opt; // in
    long owidth;
    long oheight;
    HWND ihwnd; // in
    HWND ohwnd; // out
    POINT distance; // internal
    unsigned char direction; // in
    unsigned char diagonal; // in
};
static BOOL CALLBACK FindTiledWindowEnumProc(HWND hwnd, LPARAM lp)
{
    struct FindTiledWindow_struct *tw = (struct FindTiledWindow_struct *)lp;
    RECT rc;
    if (tw->ihwnd == hwnd || !IsAltTabAble(hwnd) || !GetWindowRectL(hwnd, &rc))
        return TRUE; // Next hwnd
    POINT pt;
    pt.x = (rc.left + rc.right) / 2;
    pt.y = (rc.top + rc.bottom) / 2;
    long dx = pt.x - tw->opt.x;
    long dy = pt.y - tw->opt.y;
    long adx = abs(dx);
    long ady = abs(dy);
    //LOGA("adx = %d, ady = %d, tw->opt=%d,%d tw->oheight = %d, tw->owidth = %d"
    //    , adx,      ady,   tw->opt.x,tw->opt.y,  tw->oheight, tw->owidth);

    if(tw->diagonal) {
        // We only use the position of the center of each window.
        // We check windows within a 45deg cone around the direction of choice.
        switch (tw->direction) {
        case 0: // LEFT
            if (dx < 0 && ady <= adx
            && (adx < tw->distance.x || (adx == tw->distance.x && ady < tw->distance.y)) )
                break; // Window is closer...
            return TRUE; // skip
        case 1: // UP
            if (dy < 0 && adx <= ady
            && (ady < tw->distance.y || (ady == tw->distance.y && adx < tw->distance.x)) )
                break;
            return TRUE;
        case 2: // RIGHT
            if (dx > 0 && ady <= adx
            && (adx < tw->distance.x || (adx == tw->distance.x && ady < tw->distance.y)) )
                break;
            return TRUE;
        case 3: // DOWN
            if (dy > 0 && adx <= ady
            && (ady < tw->distance.y || (ady == tw->distance.y && adx < tw->distance.x)) )
                break;
            return TRUE;
        default: // WTF?
            UNREACHABLE();
        }
    } else {
        // Square mode (not all space is covered)
        long beamW = min(tw->oheight, tw->owidth);
        switch (tw->direction) {
        case 0: // LEFT
            if (dx < 0 && ady < beamW
            && (adx < tw->distance.x || (adx == tw->distance.x && ady < tw->distance.y)) )
                break; // Window is closer...
            return TRUE; // skip
        case 1: // UP
            if (dy < 0 && adx < beamW
            && (ady < tw->distance.y || (ady == tw->distance.y && adx < tw->distance.x)) )
                break;
            return TRUE;
        case 2: // RIGHT
            if (dx > 0 && ady < beamW
            && (adx < tw->distance.x || (adx == tw->distance.x && ady < tw->distance.y)) )
                break;
            return TRUE;
        case 3: // DOWN
            if (dy > 0 && adx < beamW
            && (ady < tw->distance.y || (ady == tw->distance.y && adx < tw->distance.x)) )
                break;
            return TRUE;
        default: // WTF?
            UNREACHABLE();
        }
    }
    // Update tw struct if we find a closer window.
    tw->distance.x = adx;
    tw->distance.y = ady;
    tw->ohwnd = hwnd;

    return TRUE;
}
static HWND FindTiledWindow(HWND hwnd, unsigned char direction)
{
    assert(direction < 4 );

    RECT rc;
    if (GetWindowRectL(hwnd, &rc)) {
        struct FindTiledWindow_struct tw;
        long w = (rc.right - rc.left) / 2;
        long h = (rc.bottom - rc.top )/ 2;
        tw.opt.x = (rc.left + rc.right) / 2;
        tw.opt.y = (rc.top + rc.bottom) / 2;
        tw.owidth  = w;
        tw.oheight = h;
        tw.ihwnd = hwnd;
        tw.ohwnd = NULL;
        tw.distance.x = 0x7ffffff0;
        tw.distance.y = 0x7ffffff0;
        tw.direction = direction;
        tw.diagonal = 0;

        EnumDesktopWindows(NULL, FindTiledWindowEnumProc, (LPARAM)&tw);
        if (!tw.ohwnd) { // Try again with diagonals
            tw.diagonal = 1;
            EnumDesktopWindows(NULL, FindTiledWindowEnumProc, (LPARAM)&tw);
        }
//        // Last resort, try with larger and larger beam...
//        tw.diagonal = 0;
//        int i;
//        for (i=2; !tw.ohwnd && i<5; i++) {
//            tw.oheight = i*h;
//            tw.owidth  = i*h;
//            EnumDesktopWindows(NULL, FindTiledWindowEnumProc, (LPARAM)&tw);
//        }
//        // TODO: Handle MDI clients.
//        if (state.mdiclient) {
//            EnumChildWindows(state.mdiclient, FindTiledWindowEnumProc, (LPARAM)&tw);
//        } else {
//            EnumDesktopWindows(NULL, FindTiledWindowEnumProc, (LPARAM)&tw);
//        }
        // Log hwnd details (usefull for debugging)
//        TCHAR buf[1024]=TEXT("");
//        PrintHwndDetails(tw.ohwnd, buf);
//        static const char *directionStr="LTRB";
//        LOGA("FOCUSING_%c %S", directionStr[direction], buf);

        return tw.ohwnd;
    }
    return NULL;
}
/////////////////////////////////////////////////////////////////////////////
// Single click commands
static void SClickActions(HWND hwnd, enum action action)
{
    const struct resizeXY RXY_LEFT_CENTER =   {RZ_LEFT, RZ_YCENTER};
    const struct resizeXY RXY_RIGHT_CENTER =  {RZ_RIGHT, RZ_YCENTER};
    const struct resizeXY RXY_CENTER_TOP =    {RZ_XCENTER, RZ_TOP};
    const struct resizeXY RXY_CENTER_BOTTOM = {RZ_XCENTER, RZ_BOTTOM};

    state.sactiondone = action;
    LOG("Going to perform action %d", (int)action);
    switch (action) {
    case AC_MINIMIZE:    MinimizeWindow(hwnd); break;
    case AC_MAXIMIZE:    ActionMaximize(hwnd); break;
    case AC_CENTER:      CenterWindow(hwnd, !state.shift /*state.shift? 0: CW_RESTORE*/); break;
    case AC_ALWAYSONTOP: TogglesAlwaysOnTop(hwnd); break;
    case AC_CLOSE:       PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0); break;
    case AC_LOWER:       ActionLower(hwnd, 0, state.shift, state.ctrl); break;
    case AC_FOCUS:       ActionLower(hwnd, +120, state.shift, 1); break;
    case AC_BORDERLESS:  ActionBorderless(hwnd); break;
    case AC_KILL:        ActionKill(hwnd); break;
    case AC_PAUSE:       ActionPause(hwnd, 1); break;
    case AC_RESUME:      ActionPause(hwnd, 0); break;
    case AC_ROLL:        RollWindow(hwnd, 0); break;
    case AC_MAXHV:       MaximizeHV(hwnd, state.shift); break;
    case AC_MINALL:      MinimizeAllOtherWindows(hwnd, state.shift); break;
    case AC_MUTE:        Send_KEY(VK_VOLUME_MUTE); break;
    case AC_SIDESNAP:    SnapToCorner(hwnd, AUTORESIZE, !!state.shift); break;
    case AC_EXTENDSNAP:  SnapToCorner(hwnd, AUTORESIZE, !state.shift); break;
    case AC_EXTENDTNEDGE:SnapToCorner(hwnd, AUTORESIZE, state.shift?SNTO_MOVETO|SNTO_NEXTBD:SNTO_EXTEND|SNTO_NEXTBD); break;
    case AC_MOVETNEDGE:  SnapToCorner(hwnd, AUTORESIZE, state.shift?SNTO_EXTEND|SNTO_NEXTBD:SNTO_MOVETO|SNTO_NEXTBD); break;
    case AC_MENU:        ActionMenu(hwnd); break;
    case AC_NSTACKED:    ActionAltTab(state.prevpt, +120,  state.shift, EnumStackedWindowsProc); break;
    case AC_NSTACKED2:   ActionAltTab(state.prevpt, +120, !state.shift, EnumStackedWindowsProc); break;
    case AC_PSTACKED:    ActionAltTab(state.prevpt, -120,  state.shift, EnumStackedWindowsProc); break;
    case AC_PSTACKED2:   ActionAltTab(state.prevpt, -120, !state.shift, EnumStackedWindowsProc); break;
    case AC_STACKLIST:   ActionStackList(state.shift ? TRK_LASERMODE : 0); break;
    case AC_STACKLIST2:  ActionStackList(state.shift ? 0 : TRK_LASERMODE); break;
    case AC_ALTTABLIST:
        PostMessage(g_hkhwnd, WM_STACKLIST, TRK_MOVETOMONITOR | TRK_LASERMODE,
            state.shift?(LPARAM)EnumAllAltTabWindows:(LPARAM)EnumAltTabWindows); break;
    case AC_ALTTABFULLLIST:
        PostMessage(g_hkhwnd, WM_STACKLIST, TRK_MOVETOMONITOR | TRK_LASERMODE,
            state.shift?(LPARAM)EnumAltTabWindows:(LPARAM)EnumAllAltTabWindows); break;
    case AC_MLZONE:      MoveWindowToTouchingZone(hwnd, 0, 0); break; // mLeft
    case AC_MTZONE:      MoveWindowToTouchingZone(hwnd, 1, 0); break; // mTop
    case AC_MRZONE:      MoveWindowToTouchingZone(hwnd, 2, 0); break; // mRight
    case AC_MBZONE:      MoveWindowToTouchingZone(hwnd, 3, 0); break; // mBottom
    case AC_XLZONE:      MoveWindowToTouchingZone(hwnd, 0, 1); break; // xLeft
    case AC_XTZONE:      MoveWindowToTouchingZone(hwnd, 1, 1); break; // xTop
    case AC_XRZONE:      MoveWindowToTouchingZone(hwnd, 2, 1); break; // xRight
    case AC_XBZONE:      MoveWindowToTouchingZone(hwnd, 3, 1); break; // xBottom
    case AC_XTNLEDGE:    SnapToCorner(hwnd, RXY_LEFT_CENTER,   SNTO_EXTEND|SNTO_NEXTBD); break;
    case AC_XTNTEDGE:    SnapToCorner(hwnd, RXY_CENTER_TOP,    SNTO_EXTEND|SNTO_NEXTBD); break;
    case AC_XTNREDGE:    SnapToCorner(hwnd, RXY_RIGHT_CENTER,  SNTO_EXTEND|SNTO_NEXTBD); break;
    case AC_XTNBEDGE:    SnapToCorner(hwnd, RXY_CENTER_BOTTOM, SNTO_EXTEND|SNTO_NEXTBD); break;
    case AC_MTNLEDGE:    SnapToCorner(hwnd, RXY_LEFT_CENTER,   SNTO_MOVETO|SNTO_NEXTBD); break;
    case AC_MTNTEDGE:    SnapToCorner(hwnd, RXY_CENTER_TOP,    SNTO_MOVETO|SNTO_NEXTBD); break;
    case AC_MTNREDGE:    SnapToCorner(hwnd, RXY_RIGHT_CENTER,  SNTO_MOVETO|SNTO_NEXTBD); break;
    case AC_MTNBEDGE:    SnapToCorner(hwnd, RXY_CENTER_BOTTOM, SNTO_MOVETO|SNTO_NEXTBD); break;
    case AC_STEPL:       StepWindow(hwnd, -conf.KBMoveStep, 0); break;
    case AC_STEPT:       StepWindow(hwnd, -conf.KBMoveStep, 1); break;
    case AC_STEPR:       StepWindow(hwnd, +conf.KBMoveStep, 0); break;
    case AC_STEPB:       StepWindow(hwnd, +conf.KBMoveStep, 1); break;
    case AC_SSTEPL:      StepWindow(hwnd, -conf.KBMoveSStep, 0); break;
    case AC_SSTEPT:      StepWindow(hwnd, -conf.KBMoveSStep, 1); break;
    case AC_SSTEPR:      StepWindow(hwnd, +conf.KBMoveSStep, 0); break;
    case AC_SSTEPB:      StepWindow(hwnd, +conf.KBMoveSStep, 1); break;
    case AC_FOCUSL:      ReallySetForegroundWindow(FindTiledWindow(hwnd, 0)); break;
    case AC_FOCUST:      ReallySetForegroundWindow(FindTiledWindow(hwnd, 1)); break;
    case AC_FOCUSR:      ReallySetForegroundWindow(FindTiledWindow(hwnd, 2)); break;
    case AC_FOCUSB:      ReallySetForegroundWindow(FindTiledWindow(hwnd, 3)); break;

    case AC_ASONOFF:     ActionASOnOff(); break;
    case AC_MOVEONOFF:   ActionMoveOnOff(hwnd); break;
    default:
        // Shortcuts 0 - 35
        if (AC_SHRT0 <=action && action < AC_SHRT0+ARR_SZ(conf.inputSequences)
        &&  conf.inputSequences[action-AC_SHRT0] ) {
            SendInputSequence(conf.inputSequences[action-AC_SHRT0]); break;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
//
static int DoWheelActions(HWND hwnd, enum action action)
{
    state.sactiondone = action;
    // Return if in the scroll blacklist.
    if (blacklisted(hwnd, &BlkLst.Scroll)) {
        return 0; // Next hook!
    }
    int ret=1;
    switch (action) {
    case AC_ALTTAB:       ActionAltTab(state.prevpt, state.delta, /*laser=0*/0
                             , state.shift?EnumStackedWindowsProc:EnumAltTabWindows); break;
    case AC_VOLUME:       ActionVolume(state.delta); break;
    case AC_TRANSPARENCY: ret = ActionTransparency(hwnd, state.delta); break;
    case AC_LOWER:        ActionLower(hwnd, state.delta, state.shift, state.ctrl); break;
    case AC_MAXIMIZE:     ActionMaxRestMin(hwnd, state.delta); break;
    case AC_ROLL:         RollWindow(hwnd, state.delta); break;
    case AC_HSCROLL:      ret = ScrollPointedWindow(state.prevpt, -state.delta, WM_MOUSEHWHEEL); break;
    case AC_ZOOM:         ret = ActionZoom(hwnd, state.delta, 0); break;
    case AC_ZOOM2:        ret = ActionZoom(hwnd, state.delta, 1); break;
    case AC_NPSTACKED:    ActionAltTab(state.prevpt, state.delta,  state.shift, EnumStackedWindowsProc); break;
    case AC_NPSTACKED2:   ActionAltTab(state.prevpt, state.delta, !state.shift, EnumStackedWindowsProc); break;
//    case AC_BRIGHTNESS:   ActionBrightness(state.prevpt, state.delta); break;
    default: {
        ret = 0; // No action
        // Use Shrt(X) on WheelUp and Shrt(X+1) on Wheel Down.
        UCHAR rac = action + (state.delta<0);
        if (AC_SHRT0 <=rac && rac < AC_SHRT0+ARR_SZ(conf.inputSequences)
        &&  conf.inputSequences[rac-AC_SHRT0] ) {
            ret = 1;
            SendInputSequence(conf.inputSequences[rac-AC_SHRT0]); break;
        }
    }break;
    }
    // ret is 0: next hook or 1: block whel and AltUp.
    state.blockaltup = ret && state.alt > BT_HWHEEL; // block or not;
    return ret; // block or next hook
}

/////////////////////////////////////////////////////////////////////////////
static void StartSpeedMes()
{
    if (conf.AeroMaxSpeed < 65535)
        SetTimer(g_timerhwnd, SPEED_TIMER, conf.AeroSpeedTau, NULL);
}
static void StopSpeedMes()
{
    if (conf.AeroMaxSpeed < 65535)
        KillTimer(g_timerhwnd, SPEED_TIMER); // Stop speed measurement
}
static DWORD WINAPI SendAltCtrlAlt(LPVOID p)
{
    Send_KEY_UD(VK_MENU, KEYEVENTF_KEYDOWN);
    Send_KEY(VK_CONTROL);
    Send_KEY_UD(VK_MENU, KEYEVENTF_KEYUP);

    return 1;
}
static int xpure DoubleClamp(int ptx, int left, int right, int rwidth)
{ // Same logic applies to y axis
    int posx;
    int cwidth = min(rwidth, right-left);
    if (right-ptx < cwidth/2)
        posx = right - rwidth;
    else if (ptx-left < cwidth/2)
        posx = left;
    else
        posx = ptx - 2*rwidth/5;

    return posx;
}
/////////////////////////////////////////////////////////////////////////////
// If we pass buttonX BT_PROBE it will tell us if we pass the blacklist.
static int init_movement_and_actions(POINT pt, HWND hwnd, enum action action, int buttonX)
{
    RECT wnd;
    state.prevpt = pt; // in case
    int button = LOWORD(buttonX);
    UCHAR probemode = HIWORD(buttonX);

    // Make sure nothing is in the way
    HideCursor();
    state.sclickhwnd = NULL;
    KillAltSnapMenu();

    // Get window from point or use the given one.
    // Get MDI chlild hwnd or root hwnd if not MDI!
    state.mdiclient = NULL;
    state.hwnd = hwnd? hwnd: MDIorNOT(WindowFromPoint(pt), &state.mdiclient);
    if (!state.hwnd || state.hwnd == LastWin.hwnd || !IsWindow(state.hwnd)) {
        return 0;
    }

    // Get monitor info
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &mi);
    CopyRect(&state.origin.mon, &mi.rcWork);
//    LOGA("MonitorInfo got!");

    // state.mdipt HAS to be set to Zero for ClientToScreen adds the offset
    state.mdipt.x = state.mdipt.y = 0;
    if (state.mdiclient) {
        GetMDInfo(&state.mdipt, &mi.rcMonitor);
        CopyRect(&state.origin.mon, &mi.rcMonitor);
    }
//    LOGA("MDI info got!");

    WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
    // Return if window is blacklisted,
    // if we can't get information about it,
    // or if the window is fullscreen.
    // state.hwnd == GetDesktopWindow()
    // state.hwnd == GetShellWindow()
    if (blacklisted(state.hwnd, &BlkLst.Processes)
    || isClassName(state.hwnd, TEXT(APP_NAMEA)TEXT("-Pin"))
    ||(blacklisted(state.hwnd, &BlkLst.Windows)
       && !state.hittest && button != BT_WHEEL && button != BT_HWHEEL
      )// does not apply in titlebar, nor for the wheel action...
    || GetWindowPlacement(state.hwnd, &wndpl) == 0
    || GetWindowRect(state.hwnd, &wnd) == 0
    || ((state.origin.maximized = IsZoomed(state.hwnd)) && conf.BLMaximized)
    || ((state.origin.fullscreen = IsFullscreenF(state.hwnd, &wnd, &mi.rcMonitor)) && !conf.FullScreen)
    ){
        return 0;
    }
//    LOGA("Blacklists passed!");

    // If no action is to be done then we passed all balcklists
    if (probemode || action == AC_NONE) return 1;

    // Set state
    state.blockaltup = state.alt; // If alt is down...
    // return if window has to be moved/resized and does not respond in 1/4 s.
    state.prevpt=pt;

    // Set origin width/height by default from current state/wndpl.
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);
    state.origin.dpi     = GetDpiForWindow(state.hwnd);
    state.origin.width  = wndpl.rcNormalPosition.right-wndpl.rcNormalPosition.left;
    state.origin.height = wndpl.rcNormalPosition.bottom-wndpl.rcNormalPosition.top;
    state.resizable = IsResizable(state.hwnd);

    GetMinMaxInfo(state.hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions

    // Do things depending on what button was pressed
    if (MOUVEMENT(action)) {
        state.sactiondone = action;
        if (GetProp(state.hwnd, APP_MOVEONOFF)) {
            state.action = AC_NONE;
            return 0; // Movement was disabled for this window.
        }
        // AutoFocus on movement/resize.
        if (conf.AutoFocus || state.ctrl) {
            SetForegroundWindowL(state.hwnd);
        }
        if (conf.DragSendsAltCtrl
        && !(GetKeyState(VK_MENU)&0x8000)
        && !(GetKeyState(VK_SHIFT)&0x8000)) {
            // This will pop down menu and stuff
            // In case autofocus did not do it.
            // LOGA("SendAltCtrlAlt");
            DWORD lpThreadId; // In new thread because of lag under Win10
            CloseHandle(CreateThread(NULL, STACK, SendAltCtrlAlt, 0, STACK_SIZE_PARAM_IS_A_RESERVATION, &lpThreadId));
        }

        // Set action statte.
        state.action = action; // MOVE OR RESIZE
        // Wether or not we will use the zones
        state.usezones = ((conf.UseZones&9) == 9)^state.shift;

        state.enumed = 0; // Reset enum stuff
        StartSpeedMes(); // Speed timer

        int ret;
        if (state.action == AC_MOVE) {
            ret = ActionMove(pt, button);
        } else {
            ret = ActionResize(pt, &wnd, button);
        }
        if      (ret == 1) return 1; // block mouse down!
        else if (ret == 0) return 0; // Next hook!
        // else ret == -1 ...
        UpdateCursor(pt);
        SetWindowTrans(state.hwnd);

        // Send WM_ENTERSIZEMOVE
        SendSizeMove(WM_ENTERSIZEMOVE);
    } else if(button == BT_WHEEL || button == BT_HWHEEL) {
        // Wheel actions, directly return here
        // because maybe the action will not be done
        if (GetProp(state.hwnd, APP_MOVEONOFF)) {
            state.action = AC_NONE;
            return 0; // Wheel was disabled for this window.
        }
        return DoWheelActions(state.hwnd, action);
    } else if (action==AC_RESTORE) {
        int rwidth, rheight;
        if (GetRestoreData(state.hwnd, &rwidth, &rheight)&SNAPPED) {
            ClearRestoreData(state.hwnd);
            RECT rc;
            GetWindowRect(state.hwnd, &rc);
            int posx = DoubleClamp(pt.x, rc.left, rc.right, rwidth) - state.mdipt.x;
            int posy = DoubleClamp(pt.y, rc.top, rc.bottom, rheight) - state.mdipt.y;

            MoveWindowAsync(state.hwnd, posx, posy, rwidth, rheight);
        }
        if (state.hittest==2 && button == BT_LMB) return 0;
        state.blockmouseup = 1;
    } else {
        SClickActions(state.hwnd, action);
        state.blockmouseup = 1; // because the action is done
    }
    // AN ACTION HAS BEEN DONE!!!

    // Remember time, position and button of this click
    // so we can check for double-click
    state.clicktime = GetTickCount();
    state.clickpt = pt;
    state.clickbutton = button;

    // Prevent mousedown from propagating
    return 1;
}
static int xpure IsAeraCapbutton(int area)
{
    return area == HTMINBUTTON
        || area == HTMAXBUTTON
        || area == HTCLOSE
        || area == HTHELP;
}
static int xpure IsAreaAnyCap(int area)
{
    return area == HTCAPTION // Caption
       || area == HTSYSMENU  // System menu
       || IsAeraCapbutton(area); // Caption buttons
}
static int xpure IsAreaTopRZ(int area)
{
    return (area >= HTTOP && area <= HTTOPRIGHT); // Top resizing border
}
static int InTitlebar(POINT pt, enum action action,  enum button button)
{
    int willtest = ((conf.TTBActions&1) && !state.alt)
                || ((conf.TTBActions&2) &&  state.alt);
    if (willtest && action) {
        HWND nhwnd = WindowFromPoint(pt);
        if (!nhwnd) return 0; // Next hook!
        // HWND hwnd = MDIorNOT(nhwnd, &state.mdiclient);
        // if (blacklisted(hwnd, &BlkLst.Windows)) return 0; // Next hook

        // Hittest to see if we are in a caption!
        // Only accept caption buttons as a titlebar for the buttons that are
        // Not in the blacklist (default LMB and RMB ttb actions only apply
        // to the real titlebar (hittest=2).
        assert(button-2 < 32 && button-2 >= 0); // Max is 32 buttons.
        int area = HitTestTimeoutbl(nhwnd, pt);
        if ( area == HTCAPTION // Real caption
        || ( !(conf.BLCapButtons&(1<<(button-2))) && IsAreaAnyCap(area))
        || ( !(conf.BLUpperBorder&(1<<(button-2))) && IsAreaTopRZ(area)) )
        {
            return area;
        }
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Actions to be performed in the Titlebar...
static int TitleBarActions(POINT pt, enum action action, enum button button)
{
    state.hittest = 0; // Cursor in titlebar?
    if (!conf.TTBActions) return -1; // fall through
    if ((state.hittest = InTitlebar(pt, action, button))) {
        return init_movement_and_actions(pt, NULL, action, button);
    }
    return -1; // Fall through
}

/////////////////////////////////////////////////////////////////////////////
// Called on MouseUp and on AltUp when using GrabWithAlt
//static void FinishMovement() { PostMessage(g_hkhwnd, WM_FINISHMOVEMENT, 0, 0); }
static void FinishMovement()
{
    LOG("FinishMovement");
    StopSpeedMes();
    ShowSnapLayoutPreview(ZONES_PREV_HIDE);
//    Sleep(5000);
    if (LastWin.hwnd
    && (state.moving == NOT_MOVED || (!conf.FullWin && state.moving == 1))) {
        if (!conf.FullWin && state.action == AC_RESIZE) {
            ResizeAllSnappedWindowsAsync();
        }
        if (IsWindow(LastWin.hwnd) && !LastWin.snap){
            if (LastWin.maximize) {
                MaximizeRestore_atpt(LastWin.hwnd, SW_MAXIMIZE, 3);
                LastWin.hwnd = NULL;
            } else {
                LastWin.end = 1;
                MoveWindowInThread(&LastWin);
            }
        }
    }
    // Clear restore data if needed
    unsigned rdata_flag = GetRestoreFlag(state.hwnd);
    if (rdata_flag&SNCLEAR) {
        ClearRestoreData(state.hwnd);
    }

    // Auto Remaximize if option enabled and conditions are met.
    if (conf.AutoRemaximize && state.moving
    && (state.origin.maximized || state.origin.fullscreen)
    && !state.shift && !state.mdiclient && state.action == AC_MOVE) {
        state.action = AC_NONE;
        HMONITOR monitor = MonitorFromPoint(state.prevpt, MONITOR_DEFAULTTONEAREST);
        if (monitor != state.origin.monitor) {
            Sleep(8);  // Wait a little for moveThread.
            WaitMovementEnd(); // extra waiting in case...

            if (state.origin.maximized) {
                MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE, 2);
            }
            if (state.origin.fullscreen) {
                MaximizeRestore_atpt(state.hwnd, SW_FULLSCREEN, 2);
            }
        }
    }

    HideTransWin();
    // Send WM_EXITSIZEMOVE
    SendSizeMove(WM_EXITSIZEMOVE);

    state.action = AC_NONE;
    state.moving = 0;
    state.snap = conf.AutoSnap;

    // Unhook mouse if Alt is released
    if (!state.alt) {
        UnhookMouse();
    } else {
        // Just hide g_mainhwnd
        HideCursor();
    }
}

/////////////////////////////////////////////////////////////////////////////
// state.action is the current action
// TODO: Generalize click combo...
static void LockMovement()
{
    state.moving = CURSOR_ONLY;
    LastWin.hwnd = NULL;
    if(!conf.FullWin) HideTransWin();
}
static int ClickComboActions(enum action action)
{
    if (!(conf.MMMaximize&1)) return 0;
    // Maximize/Restore the window if pressing Move, Resize mouse buttons.
    if (state.action == AC_MOVE && action == AC_RESIZE) {
        WaitMovementEnd();
        if (IsZoomed(state.hwnd)) {
            if (IsSamePTT(&state.clickpt, &state.prevpt)) {
                state.moving = CURSOR_ONLY;
                RestoreWindow(state.hwnd);
            } else {
                state.moving = 0;
                MouseMove(state.prevpt);
            }
        } else if (state.resizable) {
            LockMovement();
            if (IsHotclick(state.alt)) {
                state.action = AC_NONE;
                state.moving = 0;
            }
            MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE, 2);
        }
        state.blockmouseup = 1;
        return 1;
    }
    return 0;
}

// Generalization of Click combo.
static void DoComboActions(enum action action, enum button button)
{
    // For safety
    if( !MOUVEMENT(state.action) )
        return;

    enum action accombo = GetActionMR(button);
    if (ActionInfo(accombo) & (ACINFO_MOVE|ACINFO_RESIZE|ACINFO_CLOSE)) {
        LockMovement();
    }
    if (button == BT_WHEEL || button == BT_HWHEEL) {
        // Handle wheel combo.
        if (accombo) {
            DoWheelActions(state.hwnd, accombo);
            // No mouseup to block for wheel actions...
        }
    } else {
        // Other buttons.
        if (accombo) {
            SClickActions(state.hwnd, accombo);
            state.blockmouseup = 1;
        } else {
            // Try to do Move/Resize combo..
            if( ClickComboActions(action) )
                return;

            // Make default actions if nothing was overwritten.
            switch(button) {
            //case BT_MB4: break;
            // MB5 toggles the snapping mode
            case BT_MB5: ToggleSnapState(); break;
            // Toggle SnapToZones with RMB/MMB by default.
            default: ActionToggleSnapToZoneMode(); break;
            }
            state.blockmouseup = 1; // Block anyway
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
//
static xpure int GetButton(WPARAM wp, LPARAM lp)
{
    PMSLLHOOKSTRUCT msg = (PMSLLHOOKSTRUCT)lp;
    return
        (wp==WM_LBUTTONDOWN||wp==WM_LBUTTONUP)?BT_LMB:
        (wp==WM_MBUTTONDOWN||wp==WM_MBUTTONUP)?BT_MMB:
        (wp==WM_RBUTTONDOWN||wp==WM_RBUTTONUP)?BT_RMB:
        (wp==WM_MOUSEWHEEL)?BT_WHEEL:
        (wp==WM_MOUSEHWHEEL)?BT_HWHEEL:
        (wp==WM_XBUTTONDOWN||wp==WM_XBUTTONUP)? BT_MB4-1+HIWORD(msg->mouseData):
        BT_NONE;
}
static xpure enum buttonstate GetButtonState(WPARAM wp)
{
    return
        (  wp==WM_LBUTTONDOWN||wp==WM_MBUTTONDOWN
        || wp==WM_RBUTTONDOWN||wp==WM_XBUTTONDOWN)? STATE_DOWN
        : (wp==WM_LBUTTONUP  ||wp==WM_MBUTTONUP
        || wp==WM_RBUTTONUP  ||wp==WM_XBUTTONUP)? STATE_UP
        : (wp==WM_MOUSEWHEEL ||wp==WM_MOUSEHWHEEL)? STATE_DOWN
        : STATE_NONE;
}
/////////////////////////////////////////////////////////////////////////////
// This is somewhat the main function, it is active only when the ALT key is
// pressed, or is always on when conf.keepMousehook is enabled.
//
// We should not call the next Hook for button 6-20 (manual call only).
#define CALLNEXTHOOK (button>BT_MB5 && button <= BT_MB20? 1: CallNextHookEx(NULL, nCode, wParam, lParam))

#ifdef NO_HOOK_LL
#define CallNextHookEx(NULL, nCode, wParam, lParam) 0
#endif // NO_HOOK_LL
//
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
//    if (state.ignoreclick) LOGA("IgnoreClick")
    // Set up some variables
    PMSLLHOOKSTRUCT msg = (PMSLLHOOKSTRUCT)lParam;
    POINT pt = msg->pt;
//    if ((0x201 > wParam || wParam > 0x205) && wParam != 0x20a && wParam != WM_MOUSEMOVE)
//        LOGA("wParam=%lx, data=%lx, time=%lu, extra=%lx, block?=%d, ignored?=%d", (DWORD)wParam
//            , (DWORD)msg->mouseData, (DWORD)msg->time, (DWORD)msg->dwExtraInfo
//            , (int)state.blockmouseup, (int)state.ignoreclick);
    if (nCode != HC_ACTION || state.ignoreclick || ScrollLockState())
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Mouse move, only if it is not exactly the same point than before
    if (wParam == WM_MOUSEMOVE) {
        if (SamePt(pt, state.prevpt)) return CallNextHookEx(NULL, nCode, wParam, lParam);
        // Store prevpt so we can check if the hook goes stale
        if( state.moving == DRAG_WAIT ) {
            if (IsPtDragOut(&state.prevpt, &pt))
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            state.moving = 0;
            MouseMove(state.prevpt);
        }
        state.prevpt = pt;

        // Reset double-click time
        if (!IsSamePTT(&pt, &state.clickpt)) {
            state.clicktime = 0;
        }
        // Move the window  && (state.moving || !IsSamePTT(&pt, &state.clickpt))
        if (state.action && !state.blockmouseup) { // resize or move...
            // Move the window every few frames.
            static DWORD oldtime;
            if (conf.RezTimer==1) {
                // Only move window if the EVENT TIME is different.
                //LOGA("msg->time=%lu", msg->time);
                if (msg->time != oldtime) {
                    MouseMove(pt);
                    oldtime = msg->time;
                }
            } else {
                static UCHAR updaterate;
                updaterate = (updaterate+1)%(state.action==AC_MOVE? conf.MoveRate: conf.ResizeRate);
                if (!updaterate) {
                    MouseMove(pt);
                } else if (conf.RezTimer == 3 && msg->time != oldtime) {
                    MouseMove(pt);
                    oldtime = msg->time;
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    //Get Button state and data.
    enum buttonstate buttonstate = GetButtonState(wParam);
    enum button button = (enum button)GetButton(wParam, lParam);
    // Get wheel delta
    state.delta = GET_WHEEL_DELTA_WPARAM(msg->mouseData);

//    if (button<=BT_MB5)
//        LOGA("button=%d, %s", button, buttonstate==STATE_DOWN?"DOWN":buttonstate==STATE_UP?"UP":"NONE");

    // Get actions or alternate (depends on ModKey())!
    enum action action = GetAction(button); // Normal action
    enum action ttbact = GetActionT(button);// Titlebar action

    // Handle another click if we are already busy with an action
    if (buttonstate == STATE_DOWN && state.action && state.action != conf.GrabWithAlt[ModKey()]) {
        // Handle click combo action!
        DoComboActions(action, button);
        return 1; // Block mousedown so altsnap does not remove g_mainhwnd
    }

    // Handle Titlebars actions if any
    if (ttbact && buttonstate == STATE_DOWN) {
        int ret = TitleBarActions(pt, ttbact, button);
        // If we have nothing to do in the titlebar
        if (ret < 0 && conf.InactiveScroll && !state.alt && !state.action
        && (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL)) {
            // Scroll inactive window with wheel action...
            ret = ScrollPointedWindow(pt, state.delta, wParam);
        }
        if (ret == 0) return CALLNEXTHOOK;
        else if (ret == 1) return 1;
        ttbact = AC_NONE; // No titlebar action to be done.
    }

    // Check if the click is is a Hotclick and should enable ALT.
    // If the hotclick is also mapped to an action, then we execute it.
    int is_hotclick = IsHotclick(button);
    if (!state.alt && is_hotclick && buttonstate == STATE_DOWN) {
        state.alt = button;
        // Start an action now if hotclick is also an action.
        // If action == AC_NONE, we are checking for blacklists...
        if (!action) {
            // If no action is to be made,we must reset
            // clickpt and actiondone for the Click UP
            // to be forwarded.
            state.sactiondone = AC_NONE;
            state.clickpt = pt;
        }
        int ret = init_movement_and_actions(pt, NULL, action, button);
        if (ret) {
            // Not balcklisted, action may have been performed!
            if (action) state.alt = 0; // Done!
            return 1;
        }

        // Window is blacklisted.
        // So me must forward the click...
        state.alt = 0; // release alt!
        state.fwmouseup = 1; // Forward up click...
        return CALLNEXTHOOK; // forward down click
    } else if (state.alt == button && is_hotclick && buttonstate == STATE_UP) {
        state.alt = 0;
        // Block hotclick up if not an action
        // Because it will not be done by state.blockmouseup
        // if (!action) return 1;
        if (!action && (conf.AblockHotclick || state.sactiondone))
            return 1;
        // If no action is to be done, we forward the click
        Send_Click_Thread(button);
        return 1;
    }

    // Check if we must BLOCK MOUSE UP... (after releasing hotclicks)
    if (buttonstate == STATE_UP) {
        // fw/block mouse up and decrement counter.
        if (state.fwmouseup) {
            state.fwmouseup = 0;
            //LOGA("forwarded BT%d mouse up", button);
            return CALLNEXTHOOK;
        } else if (state.blockmouseup) {
            state.blockmouseup--;
            if(!state.blockmouseup && !state.action && !state.alt)
                UnhookMouseOnly(); // We no longer need the hook.
            //LOGA("blocked BT%d mouse up", button);
            return 1;
        }
    }

    // Long click grab timer
    if (conf.LongClickMove && !state.action && !state.alt) {
        if (wParam == WM_LBUTTONDOWN) {
            state.clickpt = pt;
            // Start Grab timer
            SetTimer(g_timerhwnd, GRAB_TIMER, conf.LongClickMoveDelay, NULL);
        } else {
            // Cancel Grab timer.
            KillTimer(g_timerhwnd, GRAB_TIMER);
            return CALLNEXTHOOK;
        }
    }

    // Nothing to do...
    if (!action && !ttbact && buttonstate == STATE_DOWN)
        return CALLNEXTHOOK;//CallNextHookEx(NULL, nCode, wParam, lParam);

    // INIT ACTIONS on mouse down if Alt is down...
    if (buttonstate == STATE_DOWN && state.alt) {
        //LogState("BUTTON DOWN:\n");
        // Double ckeck some hotkey is pressed.
        if (!state.action
        && !IsHotclick(state.alt)
        && !IsHotkeyDown()) {
            UnhookMouse();
            return CALLNEXTHOOK; //CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        // Start an action (alt is down)
        int ret = init_movement_and_actions(pt, NULL, action, button);
        if (!ret) return CALLNEXTHOOK;//CallNextHookEx(NULL, nCode, wParam, lParam);
        else      return 1; // block mousedown

    // BUTTON UP
    } else if (buttonstate == STATE_UP) {
        //LogState("BUTTON UP:\n");
        SetWindowTrans(NULL); // Reset window transparency

        if( action
        &&( state.action == action || (state.action == AC_MOVE && action == AC_RESIZE))
        &&(!state.moving || state.moving == DRAG_WAIT)// No drag occured
        &&  state.sactiondone <= AC_RESIZE // Only move/resize may have happened in the meantime
        && !state.ctrl // Ctrl is not down (because of focusing)
        && IsSamePTT(&pt, &state.clickpt) // same point (within drag)
        && !IsDoubleClick(button)) { // Long click unless PiercingClick=1
            FinishMovement();
            // Mouse UP actions here only in case of MOVEMENT!:
            // Perform an action on mouse up without drag on move/resize
            int inTTB = 2*(!!state.hittest); //If we are in the titlebar add two
            if (action == AC_MOVE)   action = conf.MoveUp[ModKey()+inTTB];
            if (action == AC_RESIZE) action = conf.ResizeUp[ModKey()+inTTB];

            if (action > AC_RESIZE) {
                SClickActions(state.hwnd, action);
            } else {
                LOG("Forwarding the %d the click!", button);
                // Forward the click if no action was Mapped!
                // Win 10+ does not like receaving button down
                // when the button is already down, so we create a thread.
                Send_Click_Thread(button);
            }
            return 1; // block mouseup
        }
        // If a button performing an action is released,
        // we finish all moveent and proceed.
        if (action && state.action) {
            FinishMovement();
            return 1;
        }
    }
    return CALLNEXTHOOK; //CallNextHookEx(NULL, nCode, wParam, lParam);
} // END OF LL MOUSE PROCK
#undef CALLNEXTHOOK

/////////////////////////////////////////////////////////////////////////////
static void HookMouse()
{
    state.moving = 0; // Used to know the first time we call MouseMove.
    #ifndef NO_HOOK_LL
    if (conf.keepMousehook) {
        PostMessage(g_timerhwnd, WM_TIMER, REHOOK_TIMER, 0);
    }
    #endif

    // Check if mouse is already hooked
    if (mousehook)
        return ;

    // Set up the mouse hook
    #ifdef NO_HOOK_LL
    mousehook = (void*)SetTimer(g_timerhwnd, POOL_TIMER, 32, NULL);
    #else
    mousehook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hinstDLL, 0);
    #endif
}
/////////////////////////////////////////////////////////////////////////////
static void UnhookMouseOnly()
{
    // Do not unhook if not hooked or if the hook is still used for something
    if (!mousehook || conf.keepMousehook || state.blockmouseup)
        return;

    // Remove mouse hook
    #ifdef NO_HOOK_LL
    KillTimer(g_timerhwnd, POOL_TIMER);
    #else
    UnhookWindowsHookEx(mousehook);
    #endif
    mousehook = NULL;
}
static void UnhookMouse()
{
    // Stop action
    state.action = AC_NONE;
    state.ctrl = 0;
    state.shift = 0;
    state.moving = 0;

    SetWindowTrans(NULL);
    StopSpeedMes();

    HideCursor();

    // Release cursor trapping in case...
    ClipCursorOnce(NULL);

    UnhookMouseOnly();
}
static xpure int IsAreaLongClikcable(int area)
{
    return IsAeraCapbutton(area)
        || area == HTHSCROLL
        || area == HTVSCROLL
        || area == HTSYSMENU;
}
/////////////////////////////////////////////////////////////////////////////
// Window for timers only...
LRESULT CALLBACK TimerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TIMER) {
        switch (wParam) {
        #ifndef NO_HOOK_LL
        case REHOOK_TIMER: {
            // Silently rehook hooks if they have been stopped (>= Win7 and LowLevelHooksTimeout)
            // This can often happen if locking or sleeping the computer a lot
            POINT pt;
            GetCursorPos(&pt); // I donot know if we should really use the ASYN version.
            if (mousehook && !SamePt(state.prevpt, pt)) {
                UnhookWindowsHookEx(mousehook);
                mousehook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hinstDLL, 0);
            }
            } break;
        #endif
        case SPEED_TIMER: {
            static POINT oldpt;
            static int has_moved_to_fixed_pt;
            if (state.moving)
                state.Speed=max(abs(oldpt.x-state.prevpt.x), abs(oldpt.y-state.prevpt.y));
            else state.Speed=0;
            oldpt = state.prevpt;
            if (state.moving && state.Speed == 0 && !has_moved_to_fixed_pt && !MM_THREAD_ON) {
                has_moved_to_fixed_pt = 1;
                MouseMove(state.prevpt);
            }
            if (state.Speed) has_moved_to_fixed_pt = 0;
            } break;
        case GRAB_TIMER: {
            // Grab the action after a certain amount of time of click down
            HWND ptwnd;
            UCHAR buttonswaped;
            POINT pt;
            GetMsgPT(&pt); // Hopefully the real current cursor position
            if (IsSamePTT(&pt, &state.clickpt)
            &&  GetAsyncKeyState(1 + (buttonswaped = !!GetSystemMetrics(SM_SWAPBUTTON)))
            && (ptwnd = WindowFromPoint(pt))
            &&!IsAreaLongClikcable(HitTestTimeoutbl(ptwnd, pt))) {
                // Determine if we should actually move the Window by probing with AC_NONE
                state.hittest = 0; // No specific hittest here.
                int ret = init_movement_and_actions(pt, NULL, AC_MOVE, BT_PROBE);
                if (ret) { // Release mouse click if we have to move.
                    InterlockedIncrement(&state.ignoreclick);
                    mouse_event(buttonswaped?MOUSEEVENTF_RIGHTUP:MOUSEEVENTF_LEFTUP
                               , 0, 0, 0, GetMessageExtraInfo());
                    InterlockedDecrement(&state.ignoreclick);
                    init_movement_and_actions(pt, NULL, AC_MOVE, 0);
                }
            }
            KillTimer(g_timerhwnd, GRAB_TIMER);
            return 0;
            } break;
        #ifdef ALTUP_TIMER
        case ALTUP_TIMER : {
            // Simulate AltUp (dumb)
            HotkeyUp();
            KillTimer(g_timerhwnd, ALTUP_TIMER);
            } break;
        #endif
        #ifdef NO_HOOK_LL
        case POOL_TIMER: {
            static MSLLHOOKSTRUCT omsll; // old state
            static BYTE obts[4];
            BYTE nbts[4] = {
                !!(GetAsyncKeyState(VK_LBUTTON)&0x8000),
                !!(GetAsyncKeyState(VK_RBUTTON)&0x8000),
                !!(GetAsyncKeyState(VK_MBUTTON)&0x8000),
                0,
            };
            MSLLHOOKSTRUCT nmsll = {0};
            GetCursorPos(&nmsll.pt);
            nmsll.time = GetTickCount();
            if (nmsll.time == omsll.time)
                return 0;

            if (!SamePt(nmsll.pt, omsll.pt))
                LowLevelMouseProc(HC_ACTION, WM_MOUSEMOVE, (LPARAM)&nmsll);
            if (nbts[0] != obts[0])
                LowLevelMouseProc(HC_ACTION, nbts[0]?WM_LBUTTONDOWN:WM_LBUTTONUP, (LPARAM)&nmsll);
            if (nbts[1] != obts[1])
                LowLevelMouseProc(HC_ACTION, nbts[1]?WM_RBUTTONDOWN:WM_RBUTTONUP, (LPARAM)&nmsll);
            if (nbts[2] != obts[2])
                LowLevelMouseProc(HC_ACTION, nbts[2]?WM_MBUTTONDOWN:WM_MBUTTONUP, (LPARAM)&nmsll);

            memcpy(&omsll, &nmsll, sizeof(omsll));
            memcpy(obts, nbts, sizeof(obts));
            return 0;
            } break;
        #endif // NO_HOOK_LL
        default:;
        }
    } else if (msg == WM_DESTROY) {
        KillTimer(g_timerhwnd, REHOOK_TIMER);
        KillTimer(g_timerhwnd, SPEED_TIMER);
        KillTimer(g_timerhwnd, GRAB_TIMER);
        #ifdef NO_HOOK_LL
        KillTimer(g_timerhwnd, POOL_TIMER);
        #endif // NO_HOOK_LL

    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
///////////////////////////////////////////////////////////////////////////
// Function to calculate the necessary dimentions for the menuitem.
// In response to the WM_MEASUREITEM message
static LPARAM MeasureMenuItem(HWND hwnd, WPARAM wParam, LPARAM lParam, UINT dpi, HFONT mfont)
{
    LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT)lParam;
    if(!lpmi) return FALSE;
    struct menuitemdata *data = (struct menuitemdata *)lpmi->itemData;
    if(!data) return FALSE;
    TCHAR *text = data->txtptr;
    //LOGA("WM_MEASUREITEM: id=%u, txt=%S", lpmi->itemID, data->txtptr);

    HDC dc = GetDC(hwnd);

    // Select proper font.
//    HFONT mfont = CreateNCMenuFont(dpi);
    HFONT oldfont=(HFONT)SelectObject(dc, mfont);

    int xmargin = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
    int ymargin = GetSystemMetricsForDpi(SM_CYFIXEDFRAME, dpi);
    int xicosz =  GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int yicosz =  GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    SIZE sz; // Get text size in both dimentions
    GetTextExtentPoint32(dc, text, lstrlen(text), &sz);

    // Text width + icon width + 4 margins
    lpmi->itemWidth = sz.cx + xicosz + 4*xmargin;

    // Text height/Icon height + margin
    lpmi->itemHeight = max(sz.cy, yicosz) + ymargin;

    SelectObject(dc, oldfont); // restore old font
//    DeleteObject(mfont); // Delete menufont.
    ReleaseDC(hwnd, dc);
    return TRUE;
}
///////////////////////////////////////////////////////////////////////////
// Function to custom draw the menu item, in response to WM_DRAWITEM.
// We must both draw he small icon and the menu text.
// We must also draw the selected menu with the highligh color.
static LRESULT DrawMenuItem(HWND hwnd, WPARAM wParam, LPARAM lParam, UINT dpi, HFONT mfont)
{
    LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
    if (!di) return FALSE;
    struct menuitemdata *data = (struct menuitemdata *)di->itemData;
    if (!data) return FALSE;

    // Try to be dpi-aware as good as we can...
    int xmargin = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
    int xicosz =  GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int yicosz =  GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    //LOGA("WM_DRAWITEM: id=%u, txt=%S", di->itemID, data->txtptr);

    int bgcol, txcol;
    if(di->itemState & ODS_SELECTED) {
        // Item is highlited
        bgcol = COLOR_HIGHLIGHT ;
        txcol = COLOR_HIGHLIGHTTEXT ;
    } else {
        // normal
        bgcol = COLOR_MENU ;
        txcol = COLOR_MENUTEXT ;
    }
    if(di->itemState & ODS_GRAYED) {
        txcol = COLOR_GRAYTEXT;
    }

    HBRUSH bgbrush = GetSysColorBrush(bgcol);
    // Set
    SetBkColor(di->hDC, GetSysColor(bgcol));
    SetTextColor(di->hDC, GetSysColor(txcol));

    // Highlight menu entry
    HPEN oldpen=(HPEN)SelectObject(di->hDC, GetStockObject(NULL_PEN));
    HBRUSH oldbrush=(HBRUSH)SelectObject(di->hDC, bgbrush);
    Rectangle(di->hDC, di->rcItem.left, di->rcItem.top, di->rcItem.right+1, di->rcItem.bottom+1);

//    HFONT mfont = CreateNCMenuFont(dpi);
    HFONT oldfont=(HFONT)SelectObject(di->hDC, mfont);

    SIZE sz;
    GetTextExtentPoint32(di->hDC, data->txtptr, lstrlen(data->txtptr), &sz);
    //LOGA("WM_DRAWITEM: txtXY=%u, %u, txt=%S", (UINT)sz.cx, (UINT)sz.cy, data->txtptr);

    int totheight = di->rcItem.bottom - di->rcItem.top; // total menuitem height
    int yicooffset = (totheight - yicosz)/2; // Center icon vertically
    int ytxtoffset = (totheight - sz.cy)/2;   // Center text vertically

    DrawIconEx(di->hDC
        , di->rcItem.left+xmargin
        , di->rcItem.top + yicooffset
        , (di->itemState & ODS_GRAYED)
          ? LoadIcon(NULL, IDI_HAND)
          : data->icon
        , xicosz, yicosz
        , 0, NULL, DI_NORMAL);
    if (IsIconic(hwnds[di->itemID-1])) {
        HPEN npen = CreatePen(PS_SOLID, yicosz/5, GetSysColor(txcol));
        HPEN prevpen = (HPEN)SelectObject(di->hDC, npen);
        MoveToEx(di->hDC, di->rcItem.left+xmargin, di->rcItem.top + yicooffset+yicosz-1, NULL);
        LineTo(di->hDC, di->rcItem.left+xmargin+xicosz,di->rcItem.top + yicooffset+yicosz-1);
        DeleteObject(SelectObject(di->hDC, prevpen));
    }
// TODO: Draw a cross...
//    if (state.ctrl) {
//        HPEN npen = CreatePen(PS_SOLID, yicosz/5, GetSysColor(txcol));
//        HPEN prevpen = SelectObject(di->hDC, npen);
//        MoveToEx(di->hDC, di->rcItem.left+xmargin,        di->rcItem.top + yicooffset, NULL);
//        LineTo(di->hDC,   di->rcItem.left+xmargin+xicosz, di->rcItem.top + yicooffset+yicosz);
//        MoveToEx(di->hDC, di->rcItem.left+xmargin, di->rcItem.top + yicooffset+yicosz, NULL);
//        LineTo(di->hDC,   di->rcItem.left+xmargin+xicosz, di->rcItem.top + yicooffset);
//
//        DeleteObject(SelectObject(di->hDC, prevpen));
//    }

    // Adjust x offset for Text drawing...
    di->rcItem.left += xicosz + xmargin*3;
    di->rcItem.top += ytxtoffset;
    //LOGA("menuitemheight = %ld", di->rcItem.bottom-di->rcItem.top);
    DrawText(di->hDC, data->txtptr, -1, &di->rcItem, 0); // Menuitem Text

    // Restore dc context
    SelectObject(di->hDC, oldfont); // restore old font
//    DeleteObject(mfont); // Delete menufont.
    SelectObject(di->hDC, oldpen);
    SelectObject(di->hDC, oldbrush);

    return TRUE;
}
static void SendSYSCOMMANDToMenuItem(HWND hwnd, int id, HMENU hmenu, WPARAM sc_command )
{
    if (conf.RCCloseMItem
    && 0 <= id && (UINT)id < numhwnds
    && GetWindowLongPtr(hwnd, GWLP_USERDATA) == 3
    && IsWindow(hwnds[id]) ) {
        if (sc_command == SC_CLOSE // remove topmost flag for close command
        &&  GetWindowLongPtr(hwnds[id], GWL_EXSTYLE)&WS_EX_TOPMOST) {
            HWND pinhwnd = GetPinWindow(hwnds[id]);
            if (pinhwnd) DestroyWindow(pinhwnd);
        }
        PostMessage(hwnds[id], WM_SYSCOMMAND, sc_command, 0);

        if (sc_command == SC_CLOSE)
            EnableMenuItem(hmenu, id, MF_BYPOSITION|MF_GRAYED);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Window for single click commands for menu
LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static UINT dpi;
    static HFONT mfont= NULL;
    static HWND fhwndori = NULL;
    switch (msg) {
    case WM_CREATE: {
        // Save the original foreground window.
        dpi = GetDpiForWindow(hwnd);
        fhwndori = GetForegroundWindow();
        mfont = NULL;
        } break;
    case WM_DPICHANGED: {
        dpi = LOWORD(wParam); // Update dpi value if changed...
        if (mfont) {
            DeleteObject(mfont); // Delete menufont if needed.
            mfont = NULL;
        }
        } break;
    case WM_INITMENU:
        state.unikeymenu = (HMENU)wParam;
        break;
    case WM_GETCLICKHWND:
        return (LRESULT)state.sclickhwnd;
    case WM_COMMAND: {
        if (LOWORD(wParam)) {
            // UNIKEY MENU (LOWORD of wParam munst be non NULL.
            // LOG("Unikey menu WM_COMMAND, wp=%X, lp=%X", (UINT)wParam, (UINT)lParam);
            Send_KEY(VK_BACK); // Errase old char...

            // Send UCS-2 or Lower+Upper UTF-16 surrogates of the UNICODE char.
            SendUnicodeKey(LOWORD(wParam)); // USC-2 or Lower surrogate
            if(HIWORD(wParam)) SendUnicodeKey(HIWORD(wParam)); // Upper surrogate

            state.sclickhwnd = NULL;
        } else if (HIWORD(wParam) && IsWindow(state.sclickhwnd) ) {
            // ACTION MENU LOWORD(wParam) has to be zero to differenctiae with unikey menu
            LOG("Action Menu WM_COMMAND, wp=%X, lp=%X", (UINT)wParam, (UINT)lParam);
            enum action action = (enum action)HIWORD(wParam);
            if (action) {
                state.prevpt = state.clickpt;
                if(action == AC_ORICLICK) {
                    ShowWindow(hwnd, SW_HIDE);
                    SetCursorPos(state.clickpt.x, state.clickpt.y);
                    Send_Click(state.clickbutton);
                } else {
                    SClickActions(state.sclickhwnd, action);
                }

                // We should not refocus windows if those
                // actions were performed...
                if (action == AC_LOWER || action == AC_MINIMIZE
                ||  action == AC_KILL || action == AC_CLOSE)
                    fhwndori = NULL;
            }
            // Menu closes now.
            state.unikeymenu = NULL;
            PostMessage(hwnd, WM_CLOSE, 0, 0); // Done!
            return 0;
        }
        } break;
    case WM_MENURBUTTONUP:
    case WM_MBUTTONUP: {
        // The user released the right button.
        // We must close the corresponding Window in the windows list
        int id = wParam; // Zero-based menu id.
        WPARAM sc_command = SC_MINIMIZE;
        HMENU hmenu = (HMENU)lParam;
        if (msg == WM_MBUTTONUP) {
            // Simulate teh same thing with Middle mouse UP.
            POINT pt;
            DWORD msgpos = GetMessagePos();
            pt.x = GET_X_LPARAM(msgpos);
            pt.y = GET_Y_LPARAM(msgpos);
            hmenu = state.unikeymenu;
            id = MenuItemFromPoint(hwnd, hmenu, pt);
            sc_command = SC_CLOSE;
        }
        SendSYSCOMMANDToMenuItem(hwnd, id, hmenu, sc_command);
        } break;
    // OWNER DRAWN MENU !!!!!
    case WM_MEASUREITEM:
        // Create Menu font if not already created.
        if(!mfont) mfont = CreateNCMenuFont(dpi);
        return MeasureMenuItem(hwnd, wParam, lParam, dpi, mfont);

//    case msg == WM_MENUSELECT; {
//        USHORT id = (UINT) LOWORD(wParam); // identifier of the menu item
//        USHORT fuFlags = (UINT) HIWORD(wParam);
//        if (fuFlags == 0xFFFF && !lParam) {
//            ShowTransWin(SW_HIDE);
//        } else if (fuFlags&MF_HILITE && fuFlags&MF_OWNERDRAW
//        && 0 < id && id <= numhwnds) {
//            HWND selhwnd = hwnds[id-1]; // currently selected hwnd.
//            RECT rc;
//            GetWindowRectL(selhwnd, &rc);
//            ShowTransWin(SW_SHOWNA);
//            MoveTransWin(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
//        }
//        } break;

    case WM_DRAWITEM:
        return DrawMenuItem(hwnd, wParam, lParam, dpi, mfont);

    case WM_MENUCHAR: {
//        LOGA("WM_MENUCHAR: %X", wParam);
        // Turn the input character into a menu identifier.
        WORD cc = LOWORD(wParam);
        TCHAR c = (TCHAR)( cc  |  (('A' <= cc && cc <= 'Z')<<5) );
        if (cc==VK_ESCAPE) return MNC_CLOSE<<16;
        if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 3) {
            int closewindow=0;
            WORD item;
            if (conf.NumberMenuItems) {
                // Lower case the input character.
                // O-9 then A-Z
                item = ('0' <= c && c <= '9')? c-'0'
                     : ('a' <= c && c <= 'z')? c-'a'+10
                     : 0xFFFF;
            } else {
                // A-Z then 0-9
                // If UPPERCASE
                closewindow = conf.RCCloseMItem && 'A' <= cc && cc <= 'Z' && GetKeyState(VK_SHIFT)&0x8000;
                item = ('a' <= c && c <= 'z')? c-'a'
                     : ('0' <= c && c <= '9')? c-'0'+26
                     : 0xFFFF;
            }
            // Execute item if the key is valid.
            if (item != 0xFFFF && item <=  numhwnds) {
                if (closewindow)
                    //PostMessage(hwnd, WM_MENURBUTTONUP, item, lParam);
                    SendSYSCOMMANDToMenuItem(hwnd, item, (HMENU)lParam, SC_CLOSE);
                else
                    return item|MNC_EXECUTE<<16;
            }
        }
        } break;
    case WM_SYSCHAR:
        MessageBox(NULL, NULL, NULL, 0);
        break;
    case WM_KILLFOCUS:
        // Menu gets hiden, be sure to zero-out the clickhwnd
        state.sclickhwnd = NULL;
        break;
    case WM_DESTROY: {
        LOG("Destroying Menu window!");
        if (mfont) DeleteObject(mfont); // Delete menufont if needed.
        if (fhwndori
        && state.sclickhwnd == fhwndori
        && GetWindowLongPtr(hwnd, GWLP_USERDATA) == 1
        && IsWindow(fhwndori)) {
            // Restore the old foreground window
            SetForegroundWindow(fhwndori);
        }
        fhwndori = NULL;
        state.sclickhwnd = NULL;
        state.unikeymenu = NULL;
        } break;
    }
    // LOGA("msg=%X, wParam=%X, lParam=%lX", msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK HotKeysWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_HOTKEY) {
        int actionint = 0;
        int ptwindow = 0;
        if (wParam > 0xC000) { // HOTKEY
            // The user Pressed a hotkey.
            actionint = wParam - 0xC000; // Remove the Offset
            ptwindow = conf.UsePtWindow;
            LOG("Hotkey Pressed, action = %d", actionint);
        } else if (0x0000 < wParam && wParam < 0x1000) {
            // The user called AltSnap.exe -afACTION
            actionint = wParam - 0x0000; // Remove the Offset
            ptwindow = 0;
        } else if (0x1000 < wParam && wParam < 0x2000) {
            // The user called AltSnap.exe -apACTION
            actionint = wParam - 0x1000; // Remove the Offset
            ptwindow = 1;
        }
        enum action action =  (enum action)actionint;

        if (action > AC_RESIZE) { // Exclude resize action in case...
            POINT pt;
            GetMsgPT(&pt);
            static const enum action noinitactions[] = { AC_KILL, AC_PAUSE, AC_RESUME, AC_ASONOFF, AC_NONE };
            if (IsActionInList(action, noinitactions)) {
                // Some actions pass directly through the default blacklists...
                HWND targethwnd = ptwindow? WindowFromPoint(pt): GetForegroundWindow();
                if (IsWindow(targethwnd)) {
                    SClickActions(targethwnd, action);
                }
            } else {
                // For all other actions.
                HWND target_hwnd = NULL;
                if (!ptwindow) {
                    target_hwnd = GetForegroundWindow();
                    // List of actions for which point should default to center.
                    static const enum action resetPTaclist[] = {AC_MENU, AC_NSTACKED, AC_PSTACKED, AC_NONE };
                    if (IsActionInList(action, resetPTaclist)) {
                        state.ignorept = 1;
                    }
                    // Might be of use for something?
                    //} else if (IsActionInList(action, ptCenterAcList)) {
                    //    // We must select the center of the current window as the point
                    //    // pt to which the action will be done.
                    //    RECT rc;
                    //    GetWindowRect(target_hwnd, &rc);
                    //    pt.x = (rc.left+rc.right)/2;
                    //    pt.y = (rc.top+rc.bottom)/2;
                    //}
                }
                state.shift = state.ctrl = 0; // In case...
                init_movement_and_actions(pt, target_hwnd, action, 0);
                state.blockmouseup = 0; // We must not block mouseup in this case...
                state.ignorept = 0; // Reset...
            }
            return 0;
        }
    } else if (msg == WM_STACKLIST) {
        TrackMenuOfWindows((WNDENUMPROC)lParam, wParam);
        return 0;
//    } else if (msg == WM_FINISHMOVEMENT) {
//        FinishMovementWM();
    } else if (msg == WM_SETLAYOUTNUM) {
        SetLayoutNumber(wParam);
    } else if (msg == WM_GETLAYOUTREZ) {
        return GetLayoutRez(wParam);
    } else if (msg == WM_GETBESTLAYOUT) {
        return GetBestLayoutFromMonitors();
    } else if (msg == WM_GETZONESLEN) {
        unsigned idx = (unsigned)wParam;
        return nzones[idx];
    } else if (msg == WM_GETZONES) {
        unsigned idx = (unsigned)wParam;
        RECT *dZones = (RECT*)lParam;
        CopyZones(dZones, idx);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void freeblacklists()
{
    struct blacklist *list = (struct blacklist *)&BlkLst;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        free(list->data);
        free(list->items);
        list++;
    }
}
/////////////////////////////////////////////////////////////////////////////
// To be called before Free Library. Ideally it should free everything
static void freeallinputSequences(void);
#ifdef __cplusplus
extern "C"
#endif
__declspec(dllexport) void WINAPI Unload()
{
#if defined(_MSC_VER) && _MSC_VER > 1300
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
#endif
    conf.keepMousehook = 0;
    if (mousehook) { UnhookWindowsHookEx(mousehook); mousehook = NULL; }
    DestroyWindow(g_timerhwnd);
    KillAltSnapMenu();
    if (conf.TransWinOpacity) {
        DestroyWindow(g_transhwnd[0]);
        g_transhwnd[0] = NULL;
    } else {
        int i;
        for (i=0; i<4; i++) {
            DestroyWindow(g_transhwnd[i]);
            g_transhwnd[i] = NULL;
        }
    }

    unsigned ac;
    for(ac=AC_MENU; ac<AC_MAXVALUE; ac++)
        UnregisterHotKey(g_hkhwnd, 0xC000+ac);
    DestroyWindow(g_hkhwnd);

    EnumThreadWindows(GetCurrentThreadId(), PostPinWindowsProcMessage, WM_CLOSE);
    UnregisterClass(TEXT(APP_NAMEA)TEXT("-Timers"), hinstDLL);
    UnregisterClass(TEXT(APP_NAMEA)TEXT("-SClick"), hinstDLL);
    UnregisterClass(TEXT(APP_NAMEA)TEXT("-Trans"),  hinstDLL);
    UnregisterClass(TEXT(APP_NAMEA)TEXT("-Pin"),    hinstDLL);
    UnregisterClass(TEXT(APP_NAMEA)TEXT("-HotKeys"),hinstDLL);

    SnapLayoutPreviewCreateDestroy(NULL /*DESTROY*/);

    freeblacklists();

    freeallinputSequences();

    free(monitors);
    free(hwnds);
    free(wnds);
    free(snwnds);
    free(minhwnds);
    freezones();
}
/////////////////////////////////////////////////////////////////////////////
// blacklist is coma separated and title and class are | separated.
// valid items are: exename.exe:title|class, title|class, exename:title, title
static void readblacklist(const TCHAR *section, struct blacklist *blacklist, const char *bl_str)
{
    LPCTSTR txt = GetSectionOptionCStr(section, bl_str, NULL);
    if (!txt || !*txt) {
        return;
    }
    blacklist->data = (TCHAR *)malloc((lstrlen(txt)+1)*sizeof(TCHAR));
    if (!blacklist->data) return;
    lstrcpy(blacklist->data, txt);
    TCHAR *pos = blacklist->data;

    while (pos) {
        TCHAR *exenm = pos;

        // Move pos to next item (if any)
        pos = lstrchr(pos, TEXT(','));
        // Zero out the coma and eventual spaces
        if (pos) {
            do {
                *pos++ = '\0';
            } while(*pos == ' ');
        } // No more changes to pos.

        TCHAR *title = lstrchr(exenm, TEXT(':')); // go to the :
        // Look for the klass
        TCHAR *klass = lstrchr(exenm, TEXT('|')); // go to the next |

        // Split the item with NULL
        if (title) {
            // if klass we are in the exename:title|class, format?
            if (klass) {
                if (klass < title) {
                    // if a ':' comes after a '|' the there is a ':' in the class
                    // and exename is not specified ie: title|class format.
                    // We do this because there can be no '|' in a filename
                    title = exenm;
                    exenm = NULL;
                } else {
                    // we are in the exename:title(|klass), format
                    *title = '\0'; // zero out the ':'
                    title++;
                }
                *klass++ = '\0'; // zero out the '|'
            }
        } else if (klass) {
            // We did not find the ':' but we found a '|'
            // => we are in the title|class format !
            // => no exe name specified.
            *klass = '\0'; // Split the item with NULL
            klass++;
            title = exenm;
            exenm = NULL;
        } else {
            // We found no ':' nor '|'
            // We are in the exename only format.
        }

        // Add blacklist item
        if (title && title[0] == '*' && title[1] == '\0') {
            title = NULL; // Title is a single *
        }
        if (klass && klass[0] == '*' && klass[1] == '\0') {
            klass = NULL; // class is a single *
        }
        if (exenm && exenm[0] == '*' && exenm[1] == '\0') {
            exenm = NULL; // exename is a single *
        }
        // Allocate space
        struct blacklistitem *olditem = blacklist->items;
        blacklist->items = (struct blacklistitem *)realloc(blacklist->items, (blacklist->length+1)*sizeof(struct blacklistitem));
        if (!blacklist->items) {
            // restore old item if realloc failed
            // It will jst be a shorter blacklist
            // May be NULL as well...
            blacklist->items=olditem;
            break; // Stop the loop
        }

        // Store item
        LOG( "%ls:%ls|%ls", exenm, title, klass);
        blacklist->items[blacklist->length].exename = exenm;
        blacklist->items[blacklist->length].title = title;
        blacklist->items[blacklist->length].classname = klass;
        blacklist->length++;
    } // end while
}
// Read all the blacklitsts
#define blacklist_section_length 32767
void readallblacklists(TCHAR *inipath)
{
    mem00(&BlkLst, sizeof(BlkLst));

    TCHAR *section = (TCHAR *)malloc(blacklist_section_length*sizeof(TCHAR));
    if (!section) return;
    GetPrivateProfileSection(TEXT("Blacklist"), section, blacklist_section_length, inipath);

    struct blacklist *list = &BlkLst.Processes;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        readblacklist(section, list+i, BlackListStrings[i]);
    }
    free(section);
}
#undef blacklist_section_length

///////////////////////////////////////////////////////////////////////////
// Used to read Hotkeys and Hotclicks
static unsigned readhotkeys(const TCHAR *inisection, const char *name, const TCHAR *def, UCHAR *keys, unsigned MaxKeys)
{
    LPCTSTR txt = GetSectionOptionCStr(inisection, name, def);
    unsigned i=0;
    if(!txt || !*txt) return i;
    const TCHAR *pos = txt;
    while (*pos) {
        // Store key
        if (i == MaxKeys) break;
        keys[i++] = lstrhex2u(pos);

        while (*pos && *pos >= '0') pos++; // go to the end of the word
        while (*pos && *pos < '0') pos++; // go to next char after spaces.
    }
    keys[i] = 0;
    return i;
}
static enum action readaction(const TCHAR *section, const char *key)
{
    LPCTSTR txt = GetSectionOptionCStr(section, key, TEXT("Nothing"));
    if(!txt || !*txt) return AC_NONE;

    return MapActionW(txt);
}
// Read all buttons actions from inipath
void readbuttonactions(const TCHAR *inputsection)
{
    static const char* buttons[] = {
        "LMB", "RMB", "MMB", "MB4", "MB5",
        "MB6",  "MB7",  "MB8",
        "MB9",  "MB10", "MB11", "MB12",
        "MB13", "MB14", "MB15", "MB16",
        "MB17", "MB18", "MB19", "MB20",
        "Scroll", "HScroll",

        "GrabWithAlt",
        "MoveUp", "ResizeUp",
    };

    unsigned i;
    for (i=0; i < ARR_SZ(buttons); i++) {
        enum action * const actionptr = &conf.Mouse.LMB[0]; // first action in list

        char key[32];
        strcpy(key, buttons[i]);
        int len = lstrlenA(key);
        // Read primary action (no sufix)
        actionptr[NACPB*i+0] = readaction(inputsection, key);
        key[len] = 'B'; key[len+1] = '\0'; // Secondary B sufixe
        actionptr[NACPB*i+1] = readaction(inputsection, key);

        // Titlbar actions
        key[len] = 'T'; key[len+1] = '\0'; // Titlebar T sufixes
        actionptr[NACPB*i+2] = readaction(inputsection, key);
        key[len+1] = 'B'; key[len+2] = '\0'; // TB
        actionptr[NACPB*i+3] = readaction(inputsection, key);

        // Action while moving
        key[len] = 'M'; key[len+1] = '\0'; // Action while Moving M sufixe
        actionptr[NACPB*i+4] = readaction(inputsection, key);
        key[len+1] = 'B'; key[len+2] = '\0'; // MB
        actionptr[NACPB*i+5] = readaction(inputsection, key);

        // Action while resizing
        key[len] = 'R'; key[len+1] = '\0'; // Action while Moving M sufixe
        actionptr[NACPB*i+6] = readaction(inputsection, key);
        key[len+1] = 'B'; key[len+2] = '\0'; // MB
        actionptr[NACPB*i+7] = readaction(inputsection, key);
    }
}
///////////////////////////////////////////////////////////////////////////
// Create a window for msessages handeling timers, menu etc.
static HWND KreateMsgWin(WNDPROC proc, const TCHAR *name, LONG_PTR userdata)
{
    WNDCLASSEX wnd;
    if(!GetClassInfoEx(hinstDLL, name, &wnd)) {
        // Register the class if no already created.
        mem00(&wnd, sizeof(wnd));
        wnd.cbSize = sizeof(WNDCLASSEX);
        wnd.lpfnWndProc = proc;
        wnd.hInstance = hinstDLL;
        wnd.lpszClassName = name;
        RegisterClassEx(&wnd);
    }
    HWND parent =  (LOBYTE(LOWORD(GetVersion())) >= 5)?HWND_MESSAGE:g_mainhwnd;
    HWND hwnd = CreateWindowEx(0, wnd.lpszClassName, NULL, 0
                     , 0, 0, 0, 0, parent, NULL, hinstDLL, NULL);
    if (hwnd && userdata)
        SetWindowLongPtr(hwnd, GWLP_USERDATA, userdata);
    return hwnd;
}

///////////////////////////////////////////////////////////////////////////
static void CreateTransWin(const TCHAR *inisection)
{
    int color[2];
    // Read the color for the TransWin from ini file
    readhotkeys(inisection, "FrameColor",  TEXT("80 00 80"), (UCHAR *)&color[0], 3);
    WNDCLASSEX wnd;
    mem00(&wnd, sizeof(wnd));
    wnd.cbSize = sizeof(WNDCLASSEX);
//    wnd.style = CS_SAVEBITS;
    wnd.lpfnWndProc = DefWindowProc;
    wnd.hInstance = hinstDLL;
    wnd.hbrBackground = CreateSolidBrush(color[0]);
    wnd.lpszClassName = TEXT(APP_NAMEA)TEXT("-Trans");
    RegisterClassEx(&wnd);
    g_transhwnd[0] = NULL;
    if (conf.TransWinOpacity) {
        int xflags = conf.TransWinOpacity==255
                   ? WS_EX_TOPMOST|WS_EX_TOOLWINDOW
                   : WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED;
        g_transhwnd[0] = CreateWindowEx(xflags
                             , wnd.lpszClassName, NULL, WS_POPUP
                             , 0, 0, 0, 0, NULL, NULL, hinstDLL, NULL);
        if(conf.TransWinOpacity != 255)
            SetLayeredWindowAttributes(g_transhwnd[0], 0, conf.TransWinOpacity, LWA_ALPHA);
    }
    if (!g_transhwnd[0]) {
        int i;
        for (i=0; i<4; i++) { // the transparent window is made with 4 thin windows
            g_transhwnd[i] = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW
                             , wnd.lpszClassName, NULL, WS_POPUP
                             , 0, 0, 0, 0, g_mainhwnd, NULL, hinstDLL, NULL);
            LOG("CreateWindowEx[i] = %lX", (DWORD)(DorQWORD)g_transhwnd[i]);
        }
    }
}

void registerAllHotkeys(const TCHAR* inipath)
{
    g_hkhwnd = KreateMsgWin(HotKeysWinProc, TEXT(APP_NAMEA)TEXT("-HotKeys"), 0);
    ChangeWindowMessageFilterExL(g_hkhwnd, WM_HOTKEY, /*MSGFLT_ALLOW*/1, NULL);
    // MOD_ALT=1, MOD_CONTROL=2, MOD_SHIFT=4, MOD_WIN=8
    // RegisterHotKey(g_hkhwnd, 0xC000 + AC_KILL,   MOD_ALT|MOD_CONTROL, VK_F4); // F4=73h
    // Read All shortcuts in the [KBShortcuts] section.
    TCHAR inisection[1800];
    GetPrivateProfileSection(TEXT("KBShortcuts"), inisection, ARR_SZ(inisection), inipath);

    conf.UsePtWindow = GetSectionOptionInt(inisection, "UsePtWindow", 0);

    #define ACVALUE(a, b, c) (b),
    static const char *action_names[] = { ACTION_MAP };
    #undef ACVALUE
    unsigned ac;
    for (ac=AC_MENU; ac < ARR_SZ(action_names); ac++) {
        WORD HK = GetSectionOptionInt(inisection, action_names[ac], 0);
        if(LOBYTE(HK) && HIBYTE(HK)) {
            // Lobyte is the virtual key code and hibyte is the mod_key
            if(!RegisterHotKey(g_hkhwnd, 0xC000 + ac, HIBYTE(HK), LOBYTE(HK))) {
                LOG("Error registering hotkey %s=%x", action_names[ac], (unsigned)HK);
                #ifdef LOG_STUFF
                TCHAR title[76], acN[32];
                lstrcpy_s(title, ARR_SZ(title), TEXT(APP_NAMEA)TEXT(": unable to register hotkey for action "));
                str2tchar_s(acN, ARR_SZ(acN)-1, action_names[ac]);
                lstrcat_s(title, ARR_SZ(title), acN);
                ErrorBox(title);
                #endif // LOG_STUFF
            }
            LOG("OK registering hotkey %s=%x", action_names[ac], (unsigned)HK);
        }
    }
}
static void readalluchars(UCHAR *dest, const TCHAR * const inisection, const struct OptionListItem *optlist, size_t listlen)
{
    // Read all char options
    unsigned i;
    for (i=0; i < listlen; i++) {
        *dest++ = GetSectionOptionInt(inisection, optlist[i].name, optlist[i].def);
    }
}
void readallinputSequences(const TCHAR *inisection)
{
    UCHAR buf[512];
    char shrtN[6] = "Shrt0";
    size_t i;

    mem00(conf.inputSequences, sizeof(conf.inputSequences));

    for (i=0; i< ARR_SZ(conf.inputSequences); i++) {
        shrtN[4] = i<10? '0' + i: 'A'-10 + i;
        unsigned len = readhotkeys(inisection, shrtN, TEXT(""), buf+1, 508) / 2;
        buf[0] = len;
        if (len) {
            UCHAR *seq = (UCHAR *)malloc(len*2+1*sizeof(UCHAR));
            if (seq) {
                memcpy(seq, buf, len*2+1*sizeof(UCHAR));
                conf.inputSequences[i] = seq;
            }
        }
    }
}
static void freeallinputSequences(void)
{
    size_t i;
    for (i=0; i< ARR_SZ(conf.inputSequences); i++)
        free(conf.inputSequences[i]);
}

///////////////////////////////////////////////////////////////////////////
// Has to be called at startup, it mainly reads the config.
#ifdef __cplusplus
extern "C"
#endif
__declspec(dllexport) HWND WINAPI Load(HWND mainhwnd)
{
#if defined(_MSC_VER) && _MSC_VER > 1300
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
#endif
    // Load settings
    TCHAR inipath[MAX_PATH];
    unsigned i;
    state.action = AC_NONE;
    state.shift = 0;
    state.moving = 0;
    LastWin.hwnd = NULL;

    // GET SYSTEM SETTINGS
    DWORD dragthreshold=0;
    if (SystemParametersInfo(/*SPI_GETMOUSEDRAGOUTTHRESHOLD*/0x0084, 0, &dragthreshold, 0)) {
        conf.dragXth = conf.dragYth = dragthreshold;
    } else {
        // Unable to retreave the new drag-out Threshold
        // Default to twice the usual drag threshold.
        conf.dragXth  = GetSystemMetrics(SM_CXDRAG)<<1;
        conf.dragYth  = GetSystemMetrics(SM_CYDRAG)<<1;
    }

    conf.dbclickX = GetSystemMetrics(SM_CXDOUBLECLK);
    conf.dbclickY = GetSystemMetrics(SM_CYDOUBLECLK);


    // Get ini path
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    lstrcpy(&inipath[lstrlen(inipath)-3], TEXT("ini"));

    TCHAR stk_inisection[1420], *inisection; // Stack buffer.
    size_t inisectionlen = 8192;
    inisection = (TCHAR *)malloc(inisectionlen*sizeof(TCHAR));
    if(!inisection) {
        inisection = stk_inisection;
        inisectionlen = ARR_SZ(stk_inisection);
    }

    // [General]
    GetPrivateProfileSection(TEXT("General"), inisection, inisectionlen, inipath);
    readalluchars(&conf.AutoFocus, inisection, General_uchars, ARR_SZ(General_uchars));

    // [General] consistency checks
    conf.CenterFraction=min(conf.CenterFraction, 100);
    if(conf.SidesFraction == 255) conf.SidesFraction = conf.CenterFraction;
    conf.AHoff        = min(conf.AHoff,          100);
    conf.AVoff        = min(conf.AVoff,          100);
    conf.AeroSpeedTau = max(1, conf.AeroSpeedTau);
    conf.MinAlpha     = max(1, conf.MinAlpha);
    state.snap = conf.AutoSnap;

    // [Advanced]
    GetPrivateProfileSection(TEXT("Advanced"), inisection, inisectionlen, inipath);
    readalluchars(&conf.ResizeAll, inisection, Advanced_uchars, ARR_SZ(Advanced_uchars));

    conf.ZoomFrac      = max(2, conf.ZoomFrac);
    conf.ZoomFracShift = max(2, conf.ZoomFracShift);
    conf.BLCapButtons  = GetSectionOptionInt(inisection, "BLCapButtons", 3);
    conf.BLUpperBorder = GetSectionOptionInt(inisection, "BLUpperBorder", 3);
    conf.AeroMaxSpeed  = GetSectionOptionInt(inisection, "AeroMaxSpeed", 65535);
    conf.LongClickMoveDelay = GetSectionOptionInt(inisection, "LongClickMoveDelay", 0);
    if (conf.LongClickMoveDelay == 0)
        conf.LongClickMoveDelay = GetDoubleClickTime();

    GetPrivateProfileSection(TEXT("Performance"), inisection, inisectionlen, inipath);
    readalluchars(&conf.FullWin, inisection, Performance_uchars, ARR_SZ(Performance_uchars));
    conf.MoveRate     = max(1, conf.MoveRate);
    conf.ResizeRate   = max(1, conf.ResizeRate);

    // [Performance]
    if (conf.RezTimer == 2 || conf.RezTimer == 4) {
        // 2 => Auto 1 (if 60Hz monitor) or 0.
        // 4 => Auto 1 (if 60Hz monitor) or 3.
        conf.RezTimer = conf.RezTimer == 2? 0: 3;
        DEVMODE dvm;
        mem00(&dvm, sizeof(dvm));
        dvm.dmSize = sizeof(DEVMODE);
        if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dvm)) {
            LOG("Display Frequency = %dHz", dvm.dmDisplayFrequency);
            if (dvm.dmDisplayFrequency == 60)
                conf.RezTimer = 1;
        }
    }
    if (conf.RezTimer) conf.RefreshRate=0; // Ignore the refresh rate in RezTimer mode.
    if (conf.FullWin == 2) { // Use current config to determine if we use FullWin.
        BOOL drag_full_win=1;  // Default to ON if unable to detect
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &drag_full_win, 0);
        conf.FullWin = drag_full_win;
    }

    GetPrivateProfileSection(TEXT("Input"), inisection, inisectionlen, inipath);
    readalluchars(&conf.TTBActions, inisection, Input_uchars, ARR_SZ(Input_uchars));
    readbuttonactions(inisection);

    if (conf.TopmostIndicator) {
        int color[2];
        readhotkeys(inisection, "PinColor",  TEXT("FF FF 00 54"), (UCHAR *)&color[0], 4);
        conf.PinColor = color[0];
    }
    // Prepare the transparent window
    if (!conf.FullWin) {
        CreateTransWin(inisection);
    }

    // Same order than in the conf struct
    static const struct hklst {
        char *name; TCHAR *def;
    } hklst[] = {
        { "Hotkeys",   TEXT("A4 A5") }, // VK_LMENU VK_RMENU
        { "Shiftkeys", TEXT("A0 A1") }, // VK_LSHIFT VK_RSHIFT
        { "Hotclicks", NULL },
        { "Killkeys",  TEXT("09 2E") }, // VK_TAB VK_DELETE
        { "XXButtons", NULL },
        { "ModKey",    NULL },
        { "HScrollKey", TEXT("10") }, // VK_SHIFT
        { "ESCKeys",   TEXT("1B") }, // VK_ESCAPE = 1B
    };
    for (i=0; i < ARR_SZ(hklst); i++) {
        readhotkeys(inisection, hklst[i].name, hklst[i].def, &conf.Hotkeys[i*(MAXKEYS+1)], MAXKEYS);
    }
    UCHAR eHKs[MAXKEYS+1]; // Key to be sent at the end of a movment.
    readhotkeys(inisection, "EndSendKey", TEXT("11"), eHKs, MAXKEYS);
    conf.EndSendKey = eHKs[0];

    // Read User Shortcuts/InputSequences
    readallinputSequences(inisection);

    // Read all the BLACKLITSTS
    readallblacklists(inipath);

    ResetDB(); // Zero database of restore info (snap.c)

    GetPrivateProfileSection(TEXT("Zones"), inisection, inisectionlen, inipath);
    readalluchars(&conf.UseZones, inisection, Zones_uchars, ARR_SZ(Zones_uchars));

    if (conf.UseZones&1) { // We are using Zones
        if(conf.UseZones&2) { // Grid Mode
            ReadGrids(inisection);
        } else {
            ReadZones(inisection);
        }
        SnapLayoutPreviewCreateDestroy(inisection /*CREATE*/);
    }

    if (inisection != stk_inisection)
        free(inisection);

    conf.keepMousehook = ((conf.TTBActions&1) // titlebar action w/o Alt
                       || conf.InactiveScroll // Inactive scrolling
                       || conf.Hotclick[0] // Hotclick
                       || conf.LongClickMove); // Move with long click
    // Capture main hwnd from caller. This is also the cursor wnd
    g_mainhwnd = mainhwnd;

    if (conf.keepMousehook || conf.AeroMaxSpeed < 65535) {
        g_timerhwnd = KreateMsgWin(TimerWindowProc, TEXT(APP_NAMEA)TEXT("-Timers"), 0);
    }

    // read and register all shortcuts related options.
    registerAllHotkeys(inipath);

    // Hook mouse if a permanent hook is needed
    if (conf.keepMousehook) {
        HookMouse();
        SetTimer(g_timerhwnd, REHOOK_TIMER, 5000, NULL); // Start rehook timer
    }
    return g_hkhwnd;
}
/////////////////////////////////////////////////////////////////////////////
// Do not forget the -e_DllMain@12 for gcc... -eDllMain for x86_64
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        hinstDLL = hInst;
    }
    return TRUE;
}
