/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2022                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"
#define LONG_CLICK_MOVE
#define COBJMACROS
static BOOL CALLBACK EnumMonitorsProc(HMONITOR, HDC, LPRECT , LPARAM );
static LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Timer messages
#define REHOOK_TIMER    WM_APP+1
#define SPEED_TIMER     WM_APP+2
#define GRAB_TIMER      WM_APP+3

#define CURSOR_ONLY 66
#define NOT_MOVED 33

#define STACK 0x1000

static HWND g_transhwnd[4]; // 4 windows to make a hollow window
static HWND g_timerhwnd;    // For various timers
static HWND g_mchwnd;       // For the Action menu messages
static HWND g_hkhwnd;       // For the hotkeys message window.

static void UnhookMouse();
static void HookMouse();
static void UnhookMouseOnly();
static HWND KreateMsgWin(WNDPROC proc, wchar_t *name);

// Enumerators
enum button { BT_NONE=0, BT_LMB=0x02, BT_RMB=0x03, BT_MMB=0x04, BT_MB4=0x05
            , BT_MB5=0x06,  BT_MB6=0x07,  BT_MB7=0x08,  BT_MB8=0x09
            , BT_MB9=0x0A,  BT_MB10=0x0B, BT_MB11=0x0C, BT_MB12=0x0D
            , BT_MB13=0x0E, BT_MB14=0x0F, BT_MB15=0x10, BT_MB16=0x11
            , BT_MB17=0x12, BT_MB18=0x13, BT_MB19=0x14, BT_MB20=0x15
            , BT_WHEEL=0x16, BT_HWHEEL=0x17 };
enum resize { RZ_NONE=0, RZ_TOP, RZ_RIGHT, RZ_BOTTOM, RZ_LEFT, RZ_CENTER };
enum buttonstate {STATE_NONE, STATE_DOWN, STATE_UP};

static int init_movement_and_actions(POINT pt, HWND hwnd, enum action action, int button);
static void FinishMovement();
static void MoveTransWin(int x, int y, int w, int h);

static struct windowRR {
    HWND hwnd;
    int x;
    int y;
    int width;
    int height;
    UCHAR end;
    UCHAR maximize;
    UCHAR moveonly;
    UCHAR snap;
} LastWin;

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

    UCHAR moving;
    UCHAR blockmouseup;
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
    } origin;

    UCHAR xxbutton;
    enum action action;
    struct {
        enum resize x, y;
    } resize;
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
    UCHAR AeroTopMaximizes;
    UCHAR UseCursor;
    UCHAR MinAlpha;
    char AlphaDelta;
    char AlphaDeltaShift;
    UCHAR ZoomFrac;
    UCHAR ZoomFracShift;
    UCHAR NumberMenuItems;
    UCHAR AeroSpeedTau;
    char SnapGap;
    UCHAR ShiftSnaps;
    UCHAR PiercingClick;
    UCHAR DragSendsAltCtrl;
    UCHAR TopmostIndicator;
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
    char InterZone;
  # ifdef WIN64
    UCHAR FancyZone;
  #endif
    // [KBShortcuts]
    UCHAR UsePtWindow;
    // -- -- -- -- -- -- --
    UCHAR keepMousehook;
    WORD AeroMaxSpeed;
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

    struct {
        enum action // Up to 20 BUTTONS!!!
          LMB[4],  RMB[4],  MMB[4],  MB4[4],  MB5[4]
        , MB6[4],  MB7[4],  MB8[4],  MB9[4],  MB10[4]
        , MB11[4], MB12[4], MB13[4], MB14[4], MB15[4]
        , MB16[4], MB17[4], MB18[4], MB19[4], MB20[4]
        , Scroll[4], HScroll[4]; // Plus vertical and horizontal wheels
    } Mouse;
    enum action GrabWithAlt[4]; // Actions without click
    enum action MoveUp[4];      // Actions on (long) Move Up w/o drag
    enum action ResizeUp[4];    // Actions on (long) Resize Up w/o drag
} conf;


// Blacklist (dynamically allocated)
struct blacklistitem {
    wchar_t *title;
    wchar_t *classname;
};
struct blacklist {
    struct blacklistitem *items;
    unsigned length;
    wchar_t *data;
};
static struct {
    struct blacklist Processes;
    struct blacklist Windows;
    struct blacklist Snaplist;
    struct blacklist MDIs;
    struct blacklist Pause;
    struct blacklist MMBLower;
    struct blacklist Scroll;
    struct blacklist AResize;
    struct blacklist SSizeMove;
    struct blacklist NCHittest;
    struct blacklist Bottommost;
} BlkLst;
// MUST MATCH THE ABOVE!!!
static const char *BlackListStrings[] = {
    "Processes",
    "Windows",
    "Snaplist",
    "MDIs",
    "Pause",
    "MMBLower",
    "Scroll",
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

// Specific includes
#include "snap.c"
#include "zones.c"

/////////////////////////////////////////////////////////////////////////////
// Wether a window is present or not in a blacklist
static pure int blacklisted(HWND hwnd, const struct blacklist *list)
{
    wchar_t title[256], classname[256];
    DorQWORD mode ;
    unsigned i;

    // Null hwnd or empty list
    if (!hwnd || !list->length)
        return 0;
    // If the first element is *|* (NULL|NULL)then we are in whitelist mode
    // mode = 1 => blacklist, mode = 0 => whitelist;
    mode = (DorQWORD)list->items[0].classname|(DorQWORD)list->items[0].title;
    i = !mode;

    GetWindowText(hwnd, title, ARR_SZ(title));
    GetClassName(hwnd, classname, ARR_SZ(classname));

    for ( ; i < list->length; i++) {
        if (!wcscmp_star(classname, list->items[i].classname)
        &&  !wcscmp_rstar(title, list->items[i].title)) {
              return mode;
        }
    }
    return !mode;
}
static pure int blacklistedP(HWND hwnd, const struct blacklist *list)
{
    wchar_t title[MAX_PATH];
    DorQWORD mode ;
    unsigned i ;

    // Null hwnd or empty list
    if (!hwnd || !list->length)
        return 0;
    // If the first element is *|* then we are in whitelist mode
    // mode = 1 => blacklist mode = 0 => whitelist;
    mode = (DorQWORD)list->items[0].title;
    i = !mode;

    if (!GetWindowProgName(hwnd, title, ARR_SZ(title)))
        return 0;

    // ProcessBlacklist is case-insensitive
    for ( ; i < list->length; i++) {
        if (list->items[i].title && !wcsicmp(title, list->items[i].title))
            return mode;
    }
    return !mode;
}
static int isClassName(HWND hwnd, const wchar_t *str)
{
    wchar_t classname[256];
    return GetClassName(hwnd, classname, ARR_SZ(classname))
        && !wcscmp(classname, str);
}
/////////////////////////////////////////////////////////////////////////////
// The second bit (&2) will always correspond to the WS_THICKFRAME flag
static int pure IsResizable(HWND hwnd)
{
    int ret =  conf.ResizeAll // bit two is the real thickframe state.
            | ((!!(GetWindowLongPtr(hwnd, GWL_STYLE)&WS_THICKFRAME)) << 1);

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
        ptr = realloc(ptr, (*alloc+4)*size);
        if(ptr) *alloc = (*alloc+4); // Realloc succeeded, increase count.
    }
    return ptr;
}

/////////////////////////////////////////////////////////////////////////////
// Enumerate callback proc
unsigned monitors_alloc = 0;
BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    // Make sure we have enough space allocated
    monitors = GetEnoughSpace(monitors, nummonitors, &monitors_alloc, sizeof(RECT));
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
    wnds = GetEnoughSpace(wnds, numwnds, &wnds_alloc, sizeof(RECT));
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
    snwnds = GetEnoughSpace(snwnds, numsnwnds, &snwnds_alloc, sizeof(struct snwdata));
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
    snwnds = GetEnoughSpace(snwnds, numsnwnds, &snwnds_alloc, sizeof(struct snwdata));
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
    CloseHandle(CreateThread(NULL, STACK, EndDeferWindowPosThread, hwndSS, 0, &lpThreadId));
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
    if (!numsnwnds) return 0;
    struct windowRR *lw = lwptr;
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
        tpt.x = nwnd->left+16;
        tpt.y = nwnd->top+16 ;
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
    monitors = GetEnoughSpace(monitors, nummonitors, &monitors_alloc, sizeof(RECT));
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
static void EnumOnce(RECT **bd)
{
    static RECT borders;
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
void MoveSnap(int *_posx, int *_posy, int wndwidth, int wndheight)
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
    thresholdx = thresholdy = conf.SnapThreshold;

    // Loop monitors and windows
    unsigned i, j;
    for (i=0, j=0; i < nummonitors || j < numwnds; ) {
        RECT snapwnd;
        UCHAR snapinside;

        // Get snapwnd
        if (i < nummonitors) {
            snapwnd = monitors[i];
            snapinside = 1;
            i++;
        } else if (j < numwnds) {
            snapwnd = wnds[j];
            snapinside = (state.snap != 2);
            j++;
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
        if (i < nummonitors) {
            CopyRect(&snapwnd, &monitors[i]);
            snapinside = 1;
            i++;
        } else if (j < numwnds) {
            CopyRect(&snapwnd, &wnds[j]);
            snapinside = (state.snap != 2);
            j++;
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
#define SW_FULLSCREEN 28
static void MaximizeRestore_atpt(HWND hwnd, UINT sw_cmd)
{
    WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd != SW_FULLSCREEN)
        wndpl.showCmd = sw_cmd;

    MONITORINFO mi; mi.cbSize = sizeof(MONITORINFO);
    if(sw_cmd == SW_MAXIMIZE || sw_cmd == SW_FULLSCREEN) {
        HMONITOR wndmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        HMONITOR monitor = MonitorFromPoint(state.prevpt, MONITOR_DEFAULTTONEAREST);

        GetMonitorInfo(monitor, &mi);

        // Center window on monitor, if needed
        if (monitor != wndmonitor) {
            CenterRectInRect(&wndpl.rcNormalPosition, &mi.rcWork);
        }
    }

    SetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd == SW_FULLSCREEN) {
        MoveWindowAsync(hwnd, mi.rcMonitor.left , mi.rcMonitor.top
                      , mi.rcMonitor.right-mi.rcMonitor.left
                      , mi.rcMonitor.bottom-mi.rcMonitor.top);
    }
}
static void RestoreWindowToRect(HWND hwnd, const RECT *rc)
{
    WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wndpl);
    wndpl.showCmd = SW_RESTORE;
    CopyRect(&wndpl.rcNormalPosition, rc);
    SetWindowPlacement(hwnd, &wndpl);
}
static void RestoreWindowTo(HWND hwnd, int x, int y, int w, int h)
{
    RECT rc = {x, y, x+w, y+h };
    RestoreWindowToRect(hwnd, &rc);
}
/////////////////////////////////////////////////////////////////////////////
// Move the windows in a thread in case it is very slow to resize
static void MoveResizeWindowThread(struct windowRR *lw, UINT flag)
{
    HWND hwnd;
    hwnd = lw->hwnd;

    if (lw->end && conf.FullWin) Sleep(8); // At the End of movement...

    SetWindowPos(hwnd, NULL, lw->x, lw->y, lw->width, lw->height, flag);
    // Send WM_SYNCPAINT in case to wait for the end of movement
    // And to avoid windows to "slide through" the whole WM_MOVE queue
    if(flag&SWP_ASYNCWINDOWPOS) SendMessage(hwnd, WM_SYNCPAINT, 0, 0);
    if (conf.RefreshRate) ASleep(conf.RefreshRate); // Accurate!!!

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
    UINT flag = !lw->moveonly? RESIZEFLAG: state.resizable&2 ? MOVETHICKBORDERS: MOVEASYNC;

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
            , lw, 0, &lpThreadId)
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
        RECT *wnd = &snwnds[i].wnd;
        // if the window is in current monitor
        POINT tpt;
        tpt.x = wnd->left+16;
        tpt.y = wnd->top+16 ;
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
static void WaitMovementEnd()
{ // Only wait 192ms maximum
    int i=0;
    while (LastWin.hwnd && i++ < 12) Sleep(16);
    LastWin.hwnd = NULL; // Zero out in case.
}
///////////////////////////////////////////////////////////////////////////
#define AERO_TH conf.AeroThreshold
#define MM_THREAD_ON (LastWin.hwnd && conf.FullWin)
static int AeroMoveSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight)
{
    // return if last resizing is not finished or no Aero or not resizable.
    if((!conf.Aero && !conf.UseZones&1) || !state.resizable) return 0;

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

    unsigned restore = GetRestoreFlag(state.hwnd);
    RECT trc;
    trc.left = pLeft; trc.top = pTop;
    trc.right = pRight; trc.bottom =pBottom;
    if (PtInRect(&trc, pt) || !conf.Aero) goto restore;

    GetAeroSnappingMetrics(&leftWidth, &rightWidth, &topHeight, &bottomHeight, &mon);
    int Left  = pLeft   + AERO_TH ;
    int Right = pRight  - AERO_TH ;
    int Top   = pTop    + AERO_TH ;
    int Bottom= pBottom - AERO_TH ;
    LastWin.moveonly = 0; // We are resizing the window.

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
                MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE);
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
            SetRestoreFlag(state.hwnd, restore|SNCLEAR);
            restore = 0;
            *wndwidth = state.origin.width;
            *wndheight = state.origin.height;
            LastWin.moveonly = 0; // Restored => resize
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
        if (conf.FullWin) {
            if (IsZoomed(state.hwnd)) MaximizeRestore_atpt(state.hwnd, SW_RESTORE);
            int mmthreadend = !LastWin.hwnd;
            LastWin.hwnd = state.hwnd;
            LastWin.x = *posx;
            LastWin.y = *posy;
            LastWin.width = *wndwidth;
            LastWin.height = *wndheight;
            LastWin.moveonly = 0; // Snap => resize
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
    if (state.resize.x == RZ_CENTER && state.resize.y == RZ_TOP && pt.y < state.origin.mon.top + AERO_TH) {
        restore = SNAPPED|SNMAXH;
        *wndheight = CLAMPH(state.origin.mon.bottom - state.origin.mon.top + borders.bottom + borders.top);
        *posy = state.origin.mon.top - borders.top;
    } else if (state.resize.x == RZ_LEFT && state.resize.y == RZ_CENTER && pt.x < state.origin.mon.left + AERO_TH) {
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
        if(GetAsyncKeyState(*k++)&0x8000)
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
static enum action GetAction(const enum button button)
{
    if (button) { // Ugly pointer arithmetic (LMB <==> button == 2)
        return conf.Mouse.LMB[(button-2)*4+ModKey()];
    } else {
        return AC_NONE;
    }
}
static enum action GetActionT(const enum button button)
{
    if (button) { // Ugly pointer arithmetic +2 compared to non titlebar
        return conf.Mouse.LMB[2+(button-2)*4+ModKey()];
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
        // check if key is held down
        ckeys -= !!(GetAsyncKeyState(*pos++)&0x8000);
    }
    // return true if required amount of hotkeys are down
    return !ckeys;
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
        keys += !!(GetAsyncKeyState(*Hpos++)&0x8000);
        keys += !!(GetAsyncKeyState(*Mpos++)&0x8000);
    }
    return keys;
}

/////////////////////////////////////////////////////////////////////////////
// index 1 => normal restore on any move restore & 1
// restore & 3 => Both 1 & 2 ie: Maximized then rolled.
static void RestoreOldWin(const POINT pt, unsigned was_snapped)
{
    // Restore old width/height?
    unsigned restore = 0;
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(state.hwnd, &rwidth, &rheight);

    if (((rdata_flag & SNAPPED) && !(state.origin.maximized&&rdata_flag&2))) {
        // Set origin width and height to the saved values
        if(!state.usezones){
            restore = rdata_flag;
            state.origin.width = rwidth;
            state.origin.height = rheight;
            ClearRestoreData(state.hwnd);
        }
    }

    RECT wnd;
    GetWindowRect(state.hwnd, &wnd);
    if (state.origin.maximized)
        wnd.bottom = state.origin.mon.bottom;

    // Set offset
    state.offset.x = state.origin.width  * min(pt.x-wnd.left, wnd.right-wnd.left)
                   / max(wnd.right-wnd.left,1);
    state.offset.y = state.origin.height * min(pt.y-wnd.top, wnd.bottom-wnd.top)
                   / max(wnd.bottom-wnd.top,1);

    if (rdata_flag&ROLLED) {
        if (state.origin.maximized || was_snapped){
            // if we restore a  Rolled Maximized/snapped window...
            state.offset.y = GetSystemMetrics(SM_CYMIN)/2;
        } else {
            state.offset.x = pt.x - wnd.left;
            state.offset.y = pt.y - wnd.top;
        }
    }

    if (restore) {
        SetWindowPos(state.hwnd, NULL
                , pt.x - state.offset.x - state.mdipt.x
                , pt.y - state.offset.y - state.mdipt.y
                , state.origin.width, state.origin.height
                , SWP_NOZORDER);
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
static void SetOriginFromRestoreData(HWND hnwd, enum action action)
{
    // Set Origin width and height needed for AC_MOVE/RESIZE/CENTER/MAXHV
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(state.hwnd, &rwidth, &rheight);
    // Clear snapping info if asked.
    if (rdata_flag&SNCLEAR || (conf.SmartAero&4 && action == AC_MOVE)) {
        ClearRestoreData(state.hwnd);
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

static void MoveTransWin(int x, int y, int w, int h)
{
    #define f SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER //|SWP_DEFERERASE
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
    CloseHandle(CreateThread(NULL, STACK, WinPlacmntTrgead, wndplptr, 0, &lpThreadId));
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
static void MouseMove(POINT pt)
{
    // Check if window still exists
    if (!IsWindow(state.hwnd))
        { LastWin.hwnd = NULL; UnhookMouse(); return; }

    if (conf.UseCursor) // Draw the invisible cursor window
        MoveWindow(g_mainhwnd, pt.x-128, pt.y-128, 256, 256, FALSE);

    if (state.moving == CURSOR_ONLY) return; // Movement was blocked...

    // Restore Aero snapped window when movement starts
    UCHAR was_snapped = 0;
    if (!state.moving) {
        SetOriginFromRestoreData(state.hwnd, state.action);
        if (state.action == AC_MOVE) {
            was_snapped = IsWindowSnapped(state.hwnd);
            RestoreOldWin(pt, was_snapped);
        }
    }

    static RECT wnd; // wnd will be updated and is initialized once.
    if (!state.moving && !GetWindowRect(state.hwnd, &wnd)) return;
    int posx=0, posy=0, wndwidth=0, wndheight=0;

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
        LastWin.moveonly = 1;

        posx = pt.x-state.offset.x;
        posy = pt.y-state.offset.y;
        wndwidth = wnd.right-wnd.left;
        wndheight = wnd.bottom-wnd.top;

        // Check if the window will snap anywhere
        MoveSnap(&posx, &posy, wndwidth, wndheight);
        int ret = AeroMoveSnap(pt, &posx, &posy, &wndwidth, &wndheight);
        if (ret == 1) { state.moving = 1; return; }
        MoveSnapToZone(pt, &posx, &posy, &wndwidth, &wndheight);

        // Restore window if maximized when starting
        if (was_snapped || IsZoomed(state.hwnd)) {
            LastWin.moveonly = 0;
            WINDOWPLACEMENT wndpl; wndpl.length =sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(state.hwnd, &wndpl);
            // Restore original width and height in case we are restoring
            // A Snapped + Maximized window.
            wndpl.showCmd = SW_RESTORE;
            unsigned restore = GetRestoreFlag(state.hwnd);
            if (!(restore&ROLLED)) { // Not if window is rolled!
                wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left + state.origin.width;
                wndpl.rcNormalPosition.bottom= wndpl.rcNormalPosition.top +  state.origin.height;
            }
            if (restore&SNTHENROLLED) ClearRestoreData(state.hwnd);// Only Flag?
            // Update wndwidth and wndheight
            wndwidth  = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
            wndheight = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
            SetWindowPlacement(state.hwnd, &wndpl);
            // TODO: use a thread for sloooow windows.
//            SetWindowPlacementThread(state.hwnd, &wndpl);
        }

    } else if (state.action == AC_RESIZE) {
        // Restore the window (to monitor size) if it's maximized
        if (!state.moving && IsZoomed(state.hwnd)) {
              RestoreToMonitorSize(state.hwnd, &wnd);
        }
        // Clear restore flag if needed
        ClearRestoreFlagOnResizeIfNeeded(state.hwnd);
        // Figure out new placement
        if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
            if (state.shift) pt.x = state.shiftpt.x;
            else if (state.ctrl) pt.y = state.ctrlpt.y;
            wndwidth  = wnd.right-wnd.left + 2*(pt.x-state.offset.x);
            wndheight = wnd.bottom-wnd.top + 2*(pt.y-state.offset.y);
            posx = wnd.left - (pt.x - state.offset.x) - state.mdipt.x;
            posy = wnd.top  - (pt.y - state.offset.y) - state.mdipt.y;

            // Keep the window it perfectly centered.
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
            } else if (state.resize.y == RZ_CENTER) {
                posy = wnd.top - state.mdipt.y;
                wndheight = wnd.bottom - wnd.top;
            } else if (state.resize.y == RZ_BOTTOM) {
                posy = wnd.top - state.mdipt.y;
                wndheight = pt.y - posy + state.offset.y;
            }
            if (state.resize.x == RZ_LEFT) {
                wndwidth = CLAMPW( (wnd.right-pt.x+state.offset.x) - state.mdipt.x );
                posx = state.origin.right - wndwidth;
            } else if (state.resize.x == RZ_CENTER) {
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
    LastWin.hwnd   = state.hwnd;
    LastWin.x      = posx;
    LastWin.y      = posy;
    LastWin.width  = wndwidth;
    LastWin.height = wndheight;

    // Update the static wnd with new dimentions.
    wnd.left   = posx + state.mdipt.x;
    wnd.top    = posy + state.mdipt.y;
    wnd.right  = posx + state.mdipt.x + wndwidth;
    wnd.bottom = posy + state.mdipt.y + wndheight;

    if (!conf.FullWin) {
        static RECT bd;
        if(!state.moving) FixDWMRectLL(state.hwnd, &bd, 0);
        int tx      = posx + state.mdipt.x + bd.left;
        int ty      = posy + state.mdipt.y + bd.top;
        int twidth  = wndwidth - bd.left - bd.right;
        int theight = wndheight - bd.top - bd.bottom;
        MoveTransWin(tx, ty, twidth, theight);
        if(!state.moving)
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
    INPUT input[2];
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
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki = ctrl;
    InterlockedIncrement(&state.ignorekey);
    SendInput(1, &input, sizeof(INPUT));
    InterlockedDecrement(&state.ignorekey);
}
#define Send_CTRL() Send_KEY(VK_CONTROL)

/////////////////////////////////////////////////////////////////////////////
// Sends the click down/click up sequence to the system
static void Send_Click(enum button button)
{
    static const WORD bmapping[] = {
          MOUSEEVENTF_LEFTDOWN
        , MOUSEEVENTF_RIGHTDOWN
        , MOUSEEVENTF_MIDDLEDOWN
        , MOUSEEVENTF_XDOWN, MOUSEEVENTF_XDOWN
    };
    if (!button || button > BT_MB5) return;

    DWORD MouseEvent = bmapping[button-2];
    DWORD mdata = 0;
    if (MouseEvent == MOUSEEVENTF_XDOWN) // XBUTTON
        mdata = button - 0x04; // mdata = 1 for X1 and 2 for X2
    // MouseEvent<<1 corresponds to MOUSEEVENTF_*UP
    MOUSEINPUT click[2];
    memset(&click[0], 0, sizeof(MOUSEINPUT)*2);
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
    // memset(&click[0], 0, sizeof(KEYBDINPUT)*2);
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
    if (!blacklistedP(hwnd, &BlkLst.Pause)) {
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
        GetWindowThreadProcessId(hwnd, &pid);
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

    if(isClassName(hwnd, L"Ghost")) {
        PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        return 1;
    }

    if(blacklistedP(hwnd, &BlkLst.Pause))
       return 0;

    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, ActionKillThread, hwnd, 0, &lpThreadId));

    return 1;
}
// Try harder to actually set the window foreground.
static void ReallySetForegroundWindow(HWND hwnd)
{
    // Check existing foreground Window.
    HWND  fore = GetForegroundWindow();
    if (fore != hwnd) {
        if (state.alt != VK_MENU &&  state.alt != VK_CONTROL) {
            // If the physical Alt or Ctrl keys are not down
            // We need to activate the window with key input.
            // CTRL seems to work. Also Alt works but trigers the menu
            // So it is simpler to stick to CTRL.
            Send_CTRL();
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
    return (conf.ScrollLockState&1) &&
        !( !(GetKeyState(VK_SCROLL)&1) ^ !(conf.ScrollLockState&2) );
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
       "lwmaximize=%d\n"
       "lwmoveonly=%d\n"
       "lwsnap=%d\n"
       "blockaltup=%d\n"
       "blockmouseup=%d\n"
       "ignorekey=%d\n\n\n"
    , (int)state.clickbutton
    , (DWORD)(DorQWORD)state.hwnd
    , (DWORD)(DorQWORD)LastWin.hwnd
    , (int)LastWin.end
    , (int)LastWin.maximize
    , (int)LastWin.moveonly
    , (int)LastWin.snap
    , (int)state.blockaltup
    , (int)state.blockmouseup
    , (int)state.ignorekey );
    fclose(f);
}
static pure int XXButtonIndex(UCHAR vkey)
{
    WORD i;
    for (i=0; i < 15 && conf.XXButtons[i]; i++) {
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
static void TogglesAlwaysOnTop(HWND hwnd);
static HWND MDIorNOT(HWND hwnd, HWND *mdiclient_);
///////////////////////////////////////////////////////////////////////////
// Keep this one minimalist, it is always on.
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
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
        && IsHotkey(vkey)) {
            //LOGA("\nALT DOWN");
            state.alt = vkey;
            state.blockaltup = 0;
            state.sclickhwnd = NULL;
            KillAltSnapMenu(); // Hide unikey menu in case...

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
            state.snap = state.snap? 0: 3;
            return 1; // Block to avoid sys menu.
        } else if (state.alt && state.action == conf.GrabWithAlt[ModKey()] && IsKillkey(vkey)) {
           // Release Hook on Alt+KillKey
           // eg: DisplayFusion Alt+Tab elevated windows captures AltUp
            HotkeyUp();
        } else if (vkey == VK_ESCAPE) { // USER PRESSED ESCAPE!
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
            state.ctrl = 1;
            state.ctrlpt = state.prevpt; // Save point where ctrl was pressed.
            if (state.action) {
                SetForegroundWindowL(state.hwnd);
            }
        } else if (state.sclickhwnd && g_mchwnd && state.alt && (vkey == VK_LMENU || vkey == VK_RMENU)) {
            // Block Alt down when the altsnap's menu just opened
            if (IsWindow(state.sclickhwnd)  && IsWindow(g_mchwnd) && IsMenu(state.unikeymenu))
                return 1;
        } else if ((xxbtidx = XXButtonIndex(vkey)) >=0
        && (GetAction(BT_MMB+xxbtidx) ||  GetActionT(BT_MMB+xxbtidx))) {
            if (!state.xxbutton) {
                state.xxbutton = 1; // To Ignore autorepeat...
                SimulateXButton(WM_XBUTTONDOWN, xxbtidx);
            }
            return 1;


        } else if (conf.UniKeyHoldMenu
        && (fhwnd=GetForegroundWindow())
        && !IsFullScreenBL(fhwnd)
        && !blacklistedP(fhwnd, &BlkLst.Processes)
        && !blacklisted(fhwnd, &BlkLst.Windows)) {
            // Key lists used below...
            static const UCHAR ctrlaltwinkeys[] =
                {VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN, 0};
            static const UCHAR menupopdownkeys[] =
                { VK_BACK, VK_TAB, VK_APPS, VK_DELETE, VK_SPACE, VK_LEFT, VK_RIGHT
                , VK_PRIOR, VK_NEXT, VK_END, VK_HOME, 0};
            if (state.unikeymenu && IsMenu(state.unikeymenu) && !(GetAsyncKeyState(vkey)&0x8000)) {
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
                if (GetAsyncKeyState(vkey)&0x8000) { // The key is autorepeating.
                    if(!IsMenu(state.unikeymenu)) {
                        KillAltSnapMenu();
                        g_mchwnd = KreateMsgWin(MenuWindowProc, APP_NAME"-SClick");
                        UCHAR shiftdown = GetAsyncKeyState(VK_SHIFT)&0x8000 || GetKeyState(VK_CAPITAL)&1;
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
    HWND foreground = GetForegroundWindow();

    // Return if no window or if foreground window is blacklisted
    if (!hwnd || (foreground && blacklisted(foreground,&BlkLst.Windows)))
        return 0;

    // If it's a groupbox, grab the real window
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((style&BS_GROUPBOX) && isClassName(hwnd, L"Button")) {
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
    for (i=0; i < ARR_SZ(toOr); i++)
        if (GetAsyncKeyState(toOr[i]) &0x8000) wp |= (1<<i);

    // Forward scroll message
    SendMessage(hwnd, wParam, wp, lp);

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
    wchar_t txt[2];
    return IsVisible(window)
       && !IsIconic(window)
       && ((xstyle=GetWindowLongPtr(window, GWL_EXSTYLE))&WS_EX_NOACTIVATE) != WS_EX_NOACTIVATE
       && ( // Has a caption or borderless or present in taskbar.
            (xstyle&WS_EX_TOOLWINDOW) != WS_EX_TOOLWINDOW // Not a tool window
          ||(GetWindowLongPtr(window, GWL_STYLE)&WS_CAPTION) == WS_CAPTION // or has a caption
          ||(xstyle&WS_EX_APPWINDOW) == WS_EX_APPWINDOW // Or is forced in taskbar
          || GetBorderlessFlag(window) // Or we made it borderless
       )
       && GetWindowText(window, txt, ARR_SZ(txt)) && txt[0]
       && !blacklisted(window, &BlkLst.Bottommost); // Exclude bottommost windows
}
/////////////////////////////////////////////////////////////////////////////
unsigned hwnds_alloc = 0;
BOOL CALLBACK EnumAltTabWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized
    // to taskbar and on the same monitor as the cursor
    if (IsAltTabAble(window)
    && state.origin.monitor == MonitorFromWindow(window, MONITOR_DEFAULTTONULL)) {
        hwnds[numhwnds++] = window;
    }
    return TRUE;
}
BOOL CALLBACK EnumTopMostWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
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
BOOL CALLBACK EnumStackedWindowsProc(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    hwnds = GetEnoughSpace(hwnds, numhwnds, &hwnds_alloc, sizeof(HWND));
    if (!hwnds) return FALSE; // Stop enum, we failed

    // Only store window if it's visible, not minimized to taskbar
    RECT wnd, refwnd;
    if (IsAltTabAble(hwnd)
    && GetWindowRectL(state.hwnd, &refwnd)
    && GetWindowRectL(hwnd, &wnd)
    && (state.shift || StackedRectsT(&refwnd, &wnd, conf.SnapThreshold/2) )
    && InflateRect(&wnd, conf.SnapThreshold, conf.SnapThreshold)
    && PtInRect(&wnd, state.prevpt)
    ){
        hwnds[numhwnds++] = hwnd;
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
static int ActionAltTab(POINT pt, int delta, WNDENUMPROC lpEnumFunc)
{
    numhwnds = 0;

    if (conf.MDI) {
        // Get Class name
        HWND hwnd = WindowFromPoint(pt);
        if (!hwnd) return 0;

        // Get MDIClient
        HWND mdiclient = NULL;
        if (isClassName(hwnd, L"MDIClient")) {
            mdiclient = hwnd; // we are pointing to the MDI client!
        } else {
            MDIorNOT(hwnd, &mdiclient); // Get mdiclient from hwnd
        }
        // Enumerate and then reorder MDI windows
        if (mdiclient) {
            EnumChildWindows(mdiclient, lpEnumFunc, 0);

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
        EnumWindows(lpEnumFunc, 0);
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
// Changes the Volume on Windows 2000+ using VK_VOLUME_UP/VK_VOLUME_DOWN
static void ActionVolume(int delta)
{
    UCHAR num = (state.shift)?5:1;
    while (num--)
        Send_KEY(delta>0? VK_VOLUME_UP: VK_VOLUME_DOWN);

    return;
}
/////////////////////////////////////////////////////////////////////////////
// Windows 2000+ Only
static int ActionTransparency(HWND hwnd, int delta)
{
    static int alpha=255;

    if (blacklisted(hwnd, &BlkLst.Windows)) return 0; // Spetial
    if (MOUVEMENT(state.action)) SetWindowTrans((HWND)-1);

    int alpha_delta = (state.shift)? conf.AlphaDeltaShift: conf.AlphaDelta;
    alpha_delta *= sign(delta);

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
static void ActionLower(HWND hwnd, int delta, UCHAR shift)
{
    if (delta > 0) {
        if (shift) {
            ToggleMaxRestore(hwnd);
        } else {
            if (conf.AutoFocus || state.ctrl) SetForegroundWindowL(hwnd);
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
        ActionLower(hwnd, delta, 0);
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
    } else if ((state.resize.y == RZ_TOP && state.resize.x == RZ_CENTER)
     || (state.resize.y == RZ_BOTTOM && state.resize.x == RZ_CENTER)) {
        return LoadCursor(NULL, IDC_SIZENS);
    } else if ((state.resize.y == RZ_CENTER && state.resize.x == RZ_LEFT)
     || (state.resize.y == RZ_CENTER && state.resize.x == RZ_RIGHT)) {
        return LoadCursor(NULL, IDC_SIZEWE);
    } else {
        return LoadCursor(NULL, IDC_SIZEALL);
    }
}
static void UpdateCursor(POINT pt)
{
    // Update cursor
    if (conf.UseCursor && g_mainhwnd) {
        SetWindowPos(g_mainhwnd, HWND_TOPMOST, pt.x-8, pt.y-8, 16, 16
                    , SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE);
        SetClassLongPtr(g_mainhwnd, GCLP_HCURSOR, (LONG_PTR)CursorToDraw());
        ShowWindow(g_mainhwnd, SW_SHOWNA);
    }
}

static int IsMXRolled(HWND hwnd, RECT *rc)
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

static void SetEdgeAndOffset(const RECT *wnd, POINT pt);
static int ActionZoom(HWND hwnd, int delta, int center)
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
             || (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER);

    if (!conf.AutoSnap
    || (CT && (state.resize.x == RZ_CENTER || state.resize.y == RZ_CENTER)) )
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
    } else if (state.resize.x == RZ_CENTER && CT) {
        left  = max(T, (state.prevpt.x-rc.left)  /div);
        right = max(T, (rc.right-state.prevpt.x) /div);
    }

    if (state.resize.y == RZ_TOP) {
        bottom = max(T, (rc.bottom-rc.top)/div);
        state.resize.y = RZ_BOTTOM;
    } else if (state.resize.y == RZ_BOTTOM) {
        top    = max(T, (rc.bottom-rc.top)/div);
        state.resize.y = RZ_TOP;
    } else if (state.resize.y == RZ_CENTER && CT) {
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
              , min(max(left, right)-1, conf.SnapThreshold)  // initial x threshold
              , min(max(top, bottom)-1, conf.SnapThreshold));// initial y threshold
    // Make sure that the windows does not move
    // in case it is resized from bottom/right
    if (state.resize.x == RZ_LEFT) x = x+width - CLAMPW(width);
    if (state.resize.y == RZ_TOP) y = y+height - CLAMPH(height);
    // Avoid runaway effect when zooming in/out too much.
    if (state.resize.x == RZ_CENTER && !ISCLAMPEDW(width)) x = orc.left;
    if (state.resize.y == RZ_CENTER && !ISCLAMPEDH(height)) y = orc.top;
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
    }
    return -1;
}
static void SetEdgeAndOffset(const RECT *wnd, POINT pt)
{
    // Set edge and offset
    // Think of the window as nine boxes (corner regions get 38%, middle only 24%)
    // Does not use state.origin.width/height since that is based on wndpl.rcNormalPosition
    // which is not what you see when resizing a window that Windows Aero resized
    int wndwidth  = wnd->right  - wnd->left;
    int wndheight = wnd->bottom - wnd->top;
    int SideS = (100-conf.CenterFraction)/2;
    int CenteR = 100-SideS;

    if (pt.x-wnd->left < (wndwidth*SideS)/100) {
        state.resize.x = RZ_LEFT;
        state.offset.x = pt.x-wnd->left;
    } else if (pt.x-wnd->left < (wndwidth*CenteR)/100) {
        state.resize.x = RZ_CENTER;
        state.offset.x = pt.x-state.mdipt.x; // Used only if both x and y are CENTER
    } else {
        state.resize.x = RZ_RIGHT;
        state.offset.x = wnd->right-pt.x;
    }
    if (pt.y-wnd->top < (wndheight*SideS)/100) {
        state.resize.y = RZ_TOP;
        state.offset.y = pt.y-wnd->top;
    } else if (pt.y-wnd->top < (wndheight*CenteR)/100) {
        state.resize.y = RZ_CENTER;
        state.offset.y = pt.y-state.mdipt.y;
    } else {
        state.resize.y = RZ_BOTTOM;
        state.offset.y = wnd->bottom-pt.y;
    }
    // Set window right/bottom origin
    state.origin.right = wnd->right-state.mdipt.x;
    state.origin.bottom = wnd->bottom-state.mdipt.y;
}
static void SnapToCorner(HWND hwnd)
{
    SetOriginFromRestoreData(hwnd, AC_MOVE);
    GetMinMaxInfo(hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions
    state.action = AC_NONE; // Stop resize action

    // Get and set new position
    int posx, posy; // wndwidth and wndheight are defined above
    int restore = 1;
    RECT *mon = &state.origin.mon;
    RECT bd, wnd;
    GetWindowRect(hwnd, &wnd);
    SetEdgeAndOffset(&wnd, state.prevpt); // state.resize.x/y & state.offset.x/y
    FixDWMRect(hwnd, &bd);
    int wndwidth  = wnd.right  - wnd.left;
    int wndheight = wnd.bottom - wnd.top;

    if (!state.shift ^ !(conf.AeroTopMaximizes&2)) {
    /* Extend window's borders to monitor */
        posx = wnd.left - state.mdipt.x;
        posy = wnd.top - state.mdipt.y;

        if (state.resize.y == RZ_TOP) {
            posy = mon->top - bd.top;
            wndheight = CLAMPH(wnd.bottom-state.mdipt.y - mon->top + bd.top);
        } else if (state.resize.y == RZ_BOTTOM) {
            wndheight = CLAMPH(mon->bottom - wnd.top+state.mdipt.y + bd.bottom);
        }
        if (state.resize.x == RZ_RIGHT) {
            wndwidth =  CLAMPW(mon->right - wnd.left+state.mdipt.x + bd.right);
        } else if (state.resize.x == RZ_LEFT) {
            posx = mon->left - bd.left;
            wndwidth =  CLAMPW(wnd.right-state.mdipt.x - mon->left + bd.left);
        } else if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
            wndwidth = CLAMPW(mon->right - mon->left + bd.left + bd.right);
            posx = mon->left - bd.left;
            posy = wnd.top - state.mdipt.y + bd.top ;
            restore |= SNMAXW;
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

        if (state.resize.y == RZ_CENTER) {
            wndheight = CLAMPH(mon->bottom - mon->top); // Max Height
            posy += (mon->bottom - mon->top)/2 - wndheight/2;
            restore &= ~SNTOP;
        } else if (state.resize.y == RZ_BOTTOM) {
            wndheight = bottomHeight;
            posy = mon->bottom - wndheight;
            restore &= ~SNTOP;
            restore |= SNBOTTOM;
        }

        if (state.resize.x == RZ_CENTER && state.resize.y != RZ_CENTER) {
            wndwidth = CLAMPW( (mon->right-mon->left) ); // Max width
            posx += (mon->right - mon->left)/2 - wndwidth/2;
            restore &= ~SNLEFT;
        } else if (state.resize.x == RZ_CENTER) {
            restore &= ~SNLEFT;
            if(state.resize.y == RZ_CENTER) {
                restore |= SNMAXH;
                if(state.ctrl) {
                    LastWin.hwnd = NULL;
                    ToggleMaxRestore(hwnd);
                    return;
                }
            }
            wndwidth = wnd.right - wnd.left - bd.left - bd.right;
            posx = wnd.left - state.mdipt.x + bd.left;
        } else if (state.resize.x == RZ_RIGHT) {
            wndwidth = rightWidth;
            posx = mon->right - wndwidth;
            restore |= SNRIGHT;
            restore &= ~SNLEFT;
        }
        // FixDWMRect
        posx -= bd.left; posy -= bd.top;
        wndwidth += bd.left+bd.right; wndheight += bd.top+bd.bottom;
    }
    if (IsZoomed(state.hwnd))
        RestoreWindowTo(state.hwnd, posx, posy, wndwidth, wndheight);
    else
        MoveWindowAsync(hwnd, posx, posy, wndwidth, wndheight);
    // Save data to the window...
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
        SnapToCorner(state.hwnd);
        state.blockmouseup = 1; // Block mouse up (context menu would pop)
        state.clicktime = 0;    // Reset double-click time
        // Prevent mousedown from propagating
        return 1;
    }
    SetEdgeAndOffset(wnd, pt);
    if (state.resize.y == RZ_CENTER && state.resize.x == RZ_CENTER) {
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
            int W = wnd->right - wnd->left;
            int H = wnd->bottom - wnd->top;
            int x = pt.x - wnd->left;
            int y = pt.y - wnd->top;
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
static void CenterWindow(HWND hwnd)
{
    RECT mon;
    POINT pt;
    if (IsZoomed(hwnd)) return;
    SetOriginFromRestoreData(hwnd, AC_MOVE);
    GetCursorPos(&pt);
    GetMonitorRect(&pt, 0, &mon);
    MoveWindowAsync(hwnd
        , mon.left+ ((mon.right-mon.left)-state.origin.width)/2
        , mon.top + ((mon.bottom-mon.top)-state.origin.height)/2
        , state.origin.width
        , state.origin.height);
}

/////////////////////////////////////////////////////////////////////////////
// Pin window callback function
struct pinwindata {
    LONG_PTR OldOwStyle;
    short rightoffset;
    short topoffset;
};

//static void TrackMenuOfWindows(HWND menuhwnd, WNDENUMPROC EnumProc);
static DWORD WINAPI TrackMenuOfWindows(LPVOID ppEnumProc);
static LRESULT CALLBACK PinWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
    case WM_CREATE: {
        SetTimer(hwnd, 1, conf.PinRate, NULL);
    } break;
    case WM_SETTINGCHANGE: {
        // Free and resets the windows data.
        if (wp == SPI_SETNONCLIENTMETRICS) {
            free((void *)GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
        struct pinwindata *data;
        if ((data = (struct pinwindata *)GetWindowLongPtr(hwnd, GWLP_USERDATA)) && data->OldOwStyle == style) {
            // the data were saved for the correct style!!!
            SetWindowPos(hwnd, NULL
                , rc.right-data->rightoffset, rc.top+data->topoffset, 0, 0
                , SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE);
            return 0;
        }
        // Calculate offsets, sets window position and save data
        // to the GWLP_USERDATA stuff!
        int CapButtonWidth, PinW, PinH;
        UINT dpi = GetDpiForWindow(ow); // Use parent window...
        PinW = GetSystemMetricsForDpi(SM_CXSIZE, dpi);
        PinH = GetSystemMetricsForDpi(SM_CYSIZE, dpi);

        RECT btrc;
        if (GetCaptionButtonsRect(ow, &btrc)) {
            CapButtonWidth = btrc.right - btrc.left;
        } else {
            UCHAR btnum=0; // Number of caption buttons.
            if ((style&(WS_SYSMENU|WS_CAPTION)) == (WS_SYSMENU|WS_CAPTION)) {
                btnum++;    // WS_SYSMENU => Close button [X]
                btnum += !!(style&WS_MINIMIZEBOX);     // [_]
                btnum += !!(style&WS_MAXIMIZEBOX);     // [O]
                btnum += !!(xstyle&WS_EX_CONTEXTHELP); // [?]
            }
            btnum =  min(btnum, 3); // Maximum 3 button.
            CapButtonWidth = btnum * PinW;
        }
        // Adjust PinW and PinH to have nice stuff.
        int bdx=0, bdy=0;
        if (style&WS_THICKFRAME) {
            bdx = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
            bdy = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);
        } else if (style&WS_CAPTION) { // Caption or border
            bdx = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
            bdy = GetSystemMetricsForDpi(SM_CYFIXEDFRAME, dpi);
        }
        PinW -= 2;
        PinH -= 2;
        data = calloc(1, sizeof(*data));
        data->OldOwStyle = style;
        data->rightoffset = CapButtonWidth+PinW+bdx+4;
        data->topoffset = bdy+1;
        // Cache local hwnd storage...
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        // Move and size the window...
        SetWindowPos(hwnd, NULL
            , rc.right-data->rightoffset, rc.top+data->topoffset, PinW, PinH
            , SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
        return 0;
    } break;
    case WM_PAINT : {
        wchar_t Topchar = HIBYTE(HIWORD(conf.PinColor&0xFF000000));
        if (Topchar) {
            PAINTSTRUCT ps;
            RECT cr;
            GetClientRect(hwnd, &cr);
            BeginPaint(hwnd, &ps);
            SetBkMode(ps.hdc, TRANSPARENT);
            DrawTextW(ps.hdc, &Topchar, 1, &cr, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
            EndPaint(hwnd, &ps);
      }
    } break;
    case WM_LBUTTONDOWN: {
        DestroyWindow(hwnd);
        return 0;
    } break;
    case WM_RBUTTONDOWN: {
        state.mdiclient = NULL; // In case...
        TrackMenuOfWindows((LPVOID)EnumTopMostWindows);
        return 0;
    } break;
//    case WM_GETPINNEDHWND: {
//        // Returns the handle to the topmost window
//        HWND ow = GetWindow(hwnd, GW_OWNER);
//        if(IsWindow(ow) && IsVisible(ow)) return (LRESULT)ow;
//    } break;
    case WM_DESTROY: {
        // Free PinWin local data...
        free((void *)GetWindowLongPtr(hwnd, GWLP_USERDATA));
        KillTimer(hwnd, 1);
        // Remove topmost flag if the pin gets destroyed.
        HWND ow;
        if((ow=GetWindow(hwnd, GW_OWNER))
        && (GetWindowLongPtr(ow, GWL_EXSTYLE)&WS_EX_TOPMOST) )
            SetWindowLevel(ow, HWND_NOTOPMOST);
    } break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
static HWND CreatePinWindow()
{
    WNDCLASSEX wnd;
    if(!GetClassInfoEx(hinstDLL, APP_NAME"-Pin", &wnd)) {
        // Register the class if no already created.
        memset(&wnd, 0, sizeof(wnd));
        wnd.cbSize = sizeof(WNDCLASSEX);
        wnd.style = CS_NOCLOSE;
        wnd.lpfnWndProc = PinWindowProc;
        wnd.cbWndExtra = sizeof(void*);
        wnd.hInstance = hinstDLL;
        wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
        wnd.hbrBackground = CreateSolidBrush(conf.PinColor&0x00FFFFFF);
        wnd.lpszClassName =  APP_NAME"-Pin";
        RegisterClassEx(&wnd);
    }
    return CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST
                      , wnd.lpszClassName, NULL
                      , WS_POPUP /* Start invisible */
                      , 0, 0, 0, 0, NULL, NULL, hinstDLL, NULL);
}
/////////////////////////////////////////////////////////////////////////////
static void TogglesAlwaysOnTop(HWND hwnd)
{
    // Use the Root owner
    hwnd = GetRootOwner(hwnd);
    LONG_PTR topmost = GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOPMOST;
    if(!topmost) SetForegroundWindow(hwnd);
    SetWindowLevel(hwnd, topmost? HWND_NOTOPMOST: HWND_TOPMOST);
    if(conf.TopmostIndicator) {
        HWND pw = CreatePinWindow();
        SetWindowLongPtr(pw, GWLP_HWNDPARENT, (LONG_PTR)hwnd);
        ShowWindowAsync(pw, SW_SHOWNA);
    }
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
    SetOriginFromRestoreData(state.hwnd, AC_MOVE);

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
BOOL CALLBACK MinimizeWindowProc(HWND hwnd, LPARAM hMon)
{
    minhwnds = GetEnoughSpace(minhwnds, numminhwnds, &minhwnds_alloc, sizeof(HWND));
    if (!minhwnds) return FALSE; // Stop enum, we failed

    if (hwnd != state.sclickhwnd
    && IsVisible(hwnd)
    && !IsIconic(hwnd)
    && (WS_MINIMIZEBOX&GetWindowLongPtr(hwnd, GWL_STYLE))){
        if (!hMon || (HMONITOR)hMon == MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)) {
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
        state.sclickhwnd = hwnd;
        restore = hwnd;
        numminhwnds = 0;
        if (state.mdiclient) {
            EnumChildWindows(state.mdiclient, MinimizeWindowProc, 0);
        } else {
            EnumDesktopWindows(NULL, MinimizeWindowProc, (LPARAM)hMon);
        }
        state.sclickhwnd = NULL;
    }
}

// Make a menu filled with the windows that are enumed through EnumProc
// And Track it!!!!
static wchar_t Int2Accel(int i)
{
    if (conf.NumberMenuItems)
        return i<9? L'0'+i: L'A'+i-9;
    else
        return i<26? L'A'+i: L'0'+i-26;
}
#include <oleacc.h>
struct menuitemdata {
    MSAAMENUINFO msaa;
    wchar_t txt[80];
    HICON icon;
};
static DWORD WINAPI TrackMenuOfWindows(LPVOID ppEnumProc)
{
    WNDENUMPROC EnumProc = (WNDENUMPROC)ppEnumProc;
    state.sclickhwnd = NULL;
    KillAltSnapMenu();
    g_mchwnd = KreateMsgWin(MenuWindowProc, APP_NAME"-SClick");

    // Fill up hwnds[] with the stacked windows.
    numhwnds = 0;
    HWND mdiclient = state.mdiclient;
    if (mdiclient) {
        EnumChildWindows(mdiclient, EnumProc, 0);
    } else {
        EnumDesktopWindows(NULL, EnumProc, 0);
    }

    state.sclickhwnd = state.hwnd;
    numhwnds = min(numhwnds, 36); // Max 36 stacked windows

    HMENU menu = CreatePopupMenu();
    state.unikeymenu = menu;
    unsigned i;

    struct menuitemdata *data = calloc(numhwnds, sizeof(struct menuitemdata));
    for (i=0; i<numhwnds; i++) {
        wchar_t *txt = data[i].txt;
        GetWindowText(hwnds[i], txt+5, 80-6);
        txt[0] = L'&';
        txt[1] = Int2Accel(i);
        txt[2] = L' '; txt[3] = L'-'; txt[4] = L' ';

        // Fill up MSAA structure for screen readers.
        data[i].msaa.dwMSAASignature = MSAA_MENU_SIG;
        data[i].msaa.cchWText = wcslen(txt);
        data[i].msaa.pszWText = txt;

        data[i].icon = GetWindowIcon(hwnds[i]);
        MENUITEMINFO lpmi= { sizeof(MENUITEMINFO) };
        lpmi.fMask = MIIM_DATA|MIIM_TYPE|MIIM_ID;
        lpmi.fType = MFT_OWNERDRAW; /*MFT_STRING*/
        lpmi.wID = i;
        lpmi.dwItemData = (ULONG_PTR)&data[i];
        lpmi.dwTypeData = (LPWSTR)&data[i].msaa;
        lpmi.cch = sizeof(MSAAMENUINFO);
        InsertMenuItem(menu, i, FALSE, &lpmi);
    }
    POINT pt;
    GetCursorPos(&pt);
    ReallySetForegroundWindow(g_mchwnd);
    i = (unsigned)TrackPopupMenu(menu,
        TPM_RETURNCMD|TPM_NONOTIFY|GetSystemMetrics(SM_MENUDROPALIGNMENT)
        , pt.x, pt.y, 0, g_mchwnd, NULL);
    state.mdiclient = mdiclient;
    SetForegroundWindowL(hwnds[i]);

    DestroyMenu(menu);
    state.unikeymenu = NULL;
    DestroyWindow(g_mchwnd);
    g_mchwnd = NULL;
    state.sclickhwnd = NULL;
    free(data);

    return 0;
}
static void ActionStackList()
{
    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, TrackMenuOfWindows, (LPVOID)EnumStackedWindowsProc, 0, &lpThreadId));
}
static void ActionMenu(HWND hwnd)
{
    state.sclickhwnd = NULL;
    KillAltSnapMenu();
    g_mchwnd = KreateMsgWin(MenuWindowProc, APP_NAME"-SClick");
    state.sclickhwnd = hwnd;
    // Send message to Open Action Menu
    ReallySetForegroundWindow(g_mainhwnd);
    PostMessage(
        g_mainhwnd, WM_SCLICK, (WPARAM)g_mchwnd,
       ( state.prevpt.x != MAXLONG )                            // LP_CURSORPOS
       | !!(GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOPMOST)<<1 // LP_TOPMOST
       | !!GetBorderlessFlag(hwnd) << 2                        // LP_BORDERLESS
       | IsZoomed(hwnd) << 3                                    // LP_MAXIMIZED
       | !!(GetRestoreFlag(hwnd)&2) << 4                           // LP_ROLLED
    );
}
/////////////////////////////////////////////////////////////////////////////
// Single click commands
static void SClickActions(HWND hwnd, enum action action)
{
    if      (action==AC_MINIMIZE)    MinimizeWindow(hwnd);
    else if (action==AC_MAXIMIZE)    ActionMaximize(hwnd);
    else if (action==AC_CENTER)      CenterWindow(hwnd);
    else if (action==AC_ALWAYSONTOP) TogglesAlwaysOnTop(hwnd);
    else if (action==AC_CLOSE)       PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    else if (action==AC_LOWER)       ActionLower(hwnd, 0, state.shift);
    else if (action==AC_BORDERLESS)  ActionBorderless(hwnd);
    else if (action==AC_KILL)        ActionKill(hwnd);
    else if (action==AC_PAUSE)       ActionPause(hwnd, 1);
    else if (action==AC_RESUME)      ActionPause(hwnd, 0);
    else if (action==AC_ROLL)        RollWindow(hwnd, 0);
    else if (action==AC_MAXHV)       MaximizeHV(hwnd, state.shift);
    else if (action==AC_MINALL)      MinimizeAllOtherWindows(hwnd, state.shift);
    else if (action==AC_MUTE)        Send_KEY(VK_VOLUME_MUTE);
    else if (action==AC_SIDESNAP)    SnapToCorner(hwnd);
    else if (action==AC_MENU)        ActionMenu(hwnd);
    else if (action==AC_NSTACKED)    ActionAltTab(state.prevpt, +1, EnumStackedWindowsProc);
    else if (action==AC_NSTACKED2)   {state.shift = 1; ActionAltTab(state.prevpt, +1, EnumStackedWindowsProc); state.shift = 0;}
    else if (action==AC_PSTACKED)    ActionAltTab(state.prevpt, -1, EnumStackedWindowsProc);
    else if (action==AC_PSTACKED2)   { state.shift = 1; ActionAltTab(state.prevpt, -1, EnumStackedWindowsProc); state.shift = 0;}
    else if (action==AC_STACKLIST)   ActionStackList();
    else if (action==AC_MLZONE)      MoveWindowToTouchingZone(hwnd, 0, 0); // mLeft
    else if (action==AC_MTZONE)      MoveWindowToTouchingZone(hwnd, 1, 0); // mTop
    else if (action==AC_MRZONE)      MoveWindowToTouchingZone(hwnd, 2, 0); // mBottom
    else if (action==AC_MBZONE)      MoveWindowToTouchingZone(hwnd, 3, 0); // mBight
    else if (action==AC_XLZONE)      MoveWindowToTouchingZone(hwnd, 0, 1); // xLeft
    else if (action==AC_XTZONE)      MoveWindowToTouchingZone(hwnd, 1, 1); // xTop
    else if (action==AC_XRZONE)      MoveWindowToTouchingZone(hwnd, 2, 1); // xBottom
    else if (action==AC_XBZONE)      MoveWindowToTouchingZone(hwnd, 3, 1); // xBight
}
/////////////////////////////////////////////////////////////////////////////
//
static int DoWheelActions(HWND hwnd, enum action action)
{
    // Return if in the scroll blacklist.
    if (blacklisted(hwnd, &BlkLst.Scroll)) {
        return 0; // Next hook!
    }
    int ret=1;

    if      (action == AC_ALTTAB)       ActionAltTab(state.prevpt, state.delta, state.shift?EnumStackedWindowsProc:EnumAltTabWindows);
    else if (action == AC_VOLUME)       ActionVolume(state.delta);
    else if (action == AC_TRANSPARENCY) ret = ActionTransparency(hwnd, state.delta);
    else if (action == AC_LOWER)        ActionLower(hwnd, state.delta, state.shift);
    else if (action == AC_MAXIMIZE)     ActionMaxRestMin(hwnd, state.delta);
    else if (action == AC_ROLL)         RollWindow(hwnd, state.delta);
    else if (action == AC_HSCROLL)      ret = ScrollPointedWindow(state.prevpt, -state.delta, WM_MOUSEHWHEEL);
    else if (action == AC_ZOOM)         ret = ActionZoom(hwnd, state.delta, 0);
    else if (action == AC_ZOOM2)        ret = ActionZoom(hwnd, state.delta, 1);
    else if (action == AC_NPSTACKED)    ActionAltTab(state.prevpt, state.delta, EnumStackedWindowsProc);
//    else if (action == AC_BRIGHTNESS)   ActionBrightness(state.prevpt, state.delta);
    else                                ret = 0; // No action

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
/////////////////////////////////////////////////////////////////////////////
// If the action is AC_NONE it will tell us if we pass the blacklist.
static int init_movement_and_actions(POINT pt, HWND hwnd, enum action action, int button)
{
    RECT wnd;
    state.prevpt = pt; // in case

    // Make sure nothing is in the way
    HideCursor();
    state.sclickhwnd = NULL;
    KillAltSnapMenu();

    // Get window from point or use the given one.
    // Get MDI chlild hwnd or root hwnd if not MDI!
    state.mdiclient = NULL;
    state.hwnd = hwnd? hwnd: MDIorNOT(WindowFromPoint(pt), &state.mdiclient);

    if (!state.hwnd || state.hwnd == LastWin.hwnd) {
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
    if (!state.hwnd
    || blacklistedP(state.hwnd, &BlkLst.Processes)
    || isClassName(state.hwnd, APP_NAME"-Pin")
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
    if (action == AC_NONE) return 1;

    // Set state
    state.blockaltup = state.alt; // If alt is down...
    // return if window has to be moved/resized and does not respond in 1/4 s.
    state.prevpt=pt;

    // Set origin width/height by default from current state/wndpl.
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);
    state.origin.width  = wndpl.rcNormalPosition.right-wndpl.rcNormalPosition.left;
    state.origin.height = wndpl.rcNormalPosition.bottom-wndpl.rcNormalPosition.top;

    GetMinMaxInfo(state.hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions

    // Do things depending on what button was pressed
    if (MOUVEMENT(action)) {
        DorQWORD lpdwResult;
        if(!SendMessageTimeout(state.hwnd, 0, 0, 0, SMTO_NORMAL, 128, &lpdwResult)) {
            state.blockmouseup = 1;
            return 1; // Unresponsive window...
        }
        // AutoFocus on movement/resize.
        if (conf.AutoFocus || state.ctrl) {
            SetForegroundWindowL(state.hwnd);
        }
        if (conf.DragSendsAltCtrl
        && !(GetAsyncKeyState(VK_MENU)&0x8000)
        && !(GetAsyncKeyState(VK_SHIFT)&0x8000)) {
            // This will pop down menu and stuff
            // In case autofocus did not do it.
            // LOGA("SendAltCtrlAlt");
            DWORD lpThreadId; // In new thread because of lag under Win10
            CloseHandle(CreateThread(NULL, STACK, SendAltCtrlAlt, 0, 0, &lpThreadId));
        }

        // Set action statte.
        state.action = action; // MOVE OR RESIZE
        state.resizable = IsResizable(state.hwnd);
        // Wether or not we will use the zones
        state.usezones = ((conf.UseZones&9) == 9)^state.shift;

        SetWindowTrans(state.hwnd);
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

        // Send WM_ENTERSIZEMOVE
        SendSizeMove(WM_ENTERSIZEMOVE);
    } else if(button == BT_WHEEL || button == BT_HWHEEL) {
        // Wheel actions, directly return here
        // because maybe the action will not be done
        return DoWheelActions(state.hwnd, action);
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
        // Not in the blacklist (default LMB and RMB ttb actions only applies
        // to the real titlebar (hittest=2).
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
static void FinishMovement()
{
    StopSpeedMes();
    if (LastWin.hwnd
    && (state.moving == NOT_MOVED || (!conf.FullWin && state.moving == 1))) {
        if (!conf.FullWin && state.action == AC_RESIZE) {
            ResizeAllSnappedWindowsAsync();
        }
        if (IsWindow(LastWin.hwnd) && !LastWin.snap){
            if (LastWin.maximize) {
                MaximizeRestore_atpt(LastWin.hwnd, SW_MAXIMIZE);
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
                MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE);
            }
            if (state.origin.fullscreen) {
                MaximizeRestore_atpt(state.hwnd, SW_FULLSCREEN);
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
static void ClickComboActions(enum action action)
{
    // Maximize/Restore the window if pressing Move, Resize mouse buttons.
    if(state.action == AC_MOVE && action == AC_RESIZE) {
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
            state.moving = CURSOR_ONLY; // So that MouseMove will only move g_mainhwnd
            HideTransWin();
            if (IsHotclick(state.alt)) {
                state.action = AC_NONE;
                state.moving = 0;
            }
            MaximizeRestore_atpt(state.hwnd, SW_MAXIMIZE);
        }
        state.blockmouseup = 1;
    } else if (state.action == AC_RESIZE && action == AC_MOVE && !state.moving) {
        HideTransWin();
        SnapToCorner(state.hwnd);
        HideCursor();
        state.blockmouseup = 2; // block two mouse up events!
    }
}
/////////////////////////////////////////////////////////////////////////////
//
static xpure enum button GetButton(WPARAM wp, LPARAM lp)
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
//
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
//    if (state.ignoreclick) LOGA("IgnoreClick")
    if (nCode != HC_ACTION || state.ignoreclick || ScrollLockState())
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Set up some variables
    PMSLLHOOKSTRUCT msg = (PMSLLHOOKSTRUCT)lParam;
    POINT pt = msg->pt;
    // Mouse move, only if it is not exactly the same point than before
    if (wParam == WM_MOUSEMOVE) {
        if (SamePt(pt, state.prevpt)) return CallNextHookEx(NULL, nCode, wParam, lParam);
        // Store prevpt so we can check if the hook goes stale
        state.prevpt = pt;

        // Reset double-click time
        if (!IsSamePTT(&pt, &state.clickpt)) {
            state.clicktime = 0;
        }
        // Move the window  && (state.moving || !IsSamePTT(&pt, &state.clickpt))
        if (state.action && !state.blockmouseup) { // resize or move...
            // Move the window every few frames.
            if (conf.RezTimer) {
                // Only move window if the EVENT TIME is different.
                static DWORD oldtime;
                if (msg->time != oldtime) {
                    MouseMove(pt);
                    oldtime = msg->time;
                }
            } else {
                static char updaterate;
                updaterate = (updaterate+1)%(state.action==AC_MOVE? conf.MoveRate: conf.ResizeRate);
                if(!updaterate) MouseMove(pt);
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

//    if ((0x201 > wParam || wParam > 0x205) && wParam != 0x20a)
//        LOGA("wParam=%lx, data=%lx, time=%lu, extra=%lx", (DWORD)wParam
//            , (DWORD)msg->mouseData, (DWORD)msg->time, (DWORD)msg->dwExtraInfo);

    //Get Button state and data.
    enum buttonstate buttonstate = GetButtonState(wParam);
    enum button button = GetButton(wParam, lParam);
    // Get wheel delta
    state.delta = GET_WHEEL_DELTA_WPARAM(msg->mouseData);

    // Check if we must block mouse up...
    if (buttonstate == STATE_UP && state.blockmouseup) {
        // block mouse up and decrement counter.
        state.blockmouseup--;
        if(!state.blockmouseup && !state.action && !state.alt)
            UnhookMouseOnly(); // We no longer need the hook.
        return 1;
    }

//    if (button<=BT_MB5)
//        LOGA("button=%d, %s", button, buttonstate==STATE_DOWN?"DOWN":buttonstate==STATE_UP?"UP":"NONE");

    // Get actions or alternate (depends on ModKey())!
    enum action action = GetAction(button); // Normal action
    enum action ttbact = GetActionT(button);// Titlebar action

    // Check if the click is is a Hotclick and should enable ALT.
    // If the hotclick is also mapped to an action, then we fall through
    int is_hotclick = IsHotclick(button);
    if (is_hotclick && buttonstate == STATE_DOWN) {
        state.alt = button;
        if (!action) return 1; // Block mouse up if it is not also an action...
    } else if (is_hotclick && buttonstate == STATE_UP) {
        state.alt = 0;
        if (!action) return 1; // Block hotclick up
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

    // Long click grab timer
    if (conf.LongClickMove && !state.action && !state.alt) {
        if (wParam == WM_LBUTTONDOWN) {
            state.clickpt = pt;
            // Start Grab timer
            SetTimer(g_timerhwnd, GRAB_TIMER, GetDoubleClickTime(), NULL);
        } else {
            // Cancel Grab timer.
            KillTimer(g_timerhwnd, GRAB_TIMER);
            return CALLNEXTHOOK;
        }
    }

    // Nothing to do...
    if (!action && !ttbact && buttonstate == STATE_DOWN)
        return CALLNEXTHOOK;//CallNextHookEx(NULL, nCode, wParam, lParam);

    // Handle another click if we are already busy with an action
    if (buttonstate == STATE_DOWN && state.action && state.action != conf.GrabWithAlt[ModKey()]) {
        if ((conf.MMMaximize&1))
            ClickComboActions(action); // Handle click combo!
        else if (conf.UseZones&1 && state.action == AC_MOVE) {
            state.usezones = !state.usezones;
            state.blockmouseup = 1;
            MouseMove(state.prevpt);
        } else {
            state.blockmouseup = 1;
        }
        return 1; // Block mousedown so altsnap does not remove g_mainhwnd

    // INIT ACTIONS on mouse down if Alt is down...
    } else if (buttonstate == STATE_DOWN && state.alt) {
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
        // LogState("BUTTON UP:\n");
        SetWindowTrans(NULL); // Reset window transparency

        if (state.action == action
        && !state.moving // No drag occured
        && !state.ctrl // Ctrl is not down (because of focusing)
        && IsSamePTT(&pt, &state.clickpt) // same point
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
                // Forward the click if no action was Mapped
                Send_Click(button);
            }
            return 1; // block mousedown

        } else if (state.action || is_hotclick) {
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
    if (conf.keepMousehook) {
        PostMessage(g_timerhwnd, WM_TIMER, REHOOK_TIMER, 0);
    }

    // Check if mouse is already hooked
    if (mousehook)
        return ;

    // Set up the mouse hook
    mousehook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hinstDLL, 0);
    if (!mousehook)
        return ;
}
/////////////////////////////////////////////////////////////////////////////
static void UnhookMouseOnly()
{
    // Do not unhook if not hooked or if the hook is still used for something
    if (!mousehook || conf.keepMousehook || state.blockmouseup)
        return;

    // Remove mouse hook
    UnhookWindowsHookEx(mousehook);
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
        if (wParam == REHOOK_TIMER) {
            // Silently rehook hooks if they have been stopped (>= Win7 and LowLevelHooksTimeout)
            // This can often happen if locking or sleeping the computer a lot
            POINT pt;
            GetCursorPos(&pt);
            if (mousehook && !SamePt(state.prevpt, pt)) {
                UnhookWindowsHookEx(mousehook);
                mousehook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hinstDLL, 0);
            }
        } else if (wParam == SPEED_TIMER) {
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
        } else if (wParam == GRAB_TIMER) {
            // Grab the action after a certain amount of time of click down
            HWND ptwnd;
            UCHAR buttonswaped;
            POINT pt;
            GetCursorPos(&pt); // Hopefully the real current cursor position
            if (IsSamePTT(&pt, &state.clickpt)
            &&  GetAsyncKeyState(1 + (buttonswaped = !!GetSystemMetrics(SM_SWAPBUTTON)))
            && (ptwnd = WindowFromPoint(pt))
            &&!IsAreaLongClikcable(HitTestTimeoutbl(ptwnd, pt))) {
                // Determine if we should actually move the Window by probing with AC_NONE
                state.hittest = 0; // No specific hittest here.
                int ret = init_movement_and_actions(pt, NULL, AC_NONE, 0);
                if (ret) { // Release mouse click if we have to move.
                    InterlockedIncrement(&state.ignoreclick);
                    mouse_event(buttonswaped?MOUSEEVENTF_RIGHTUP:MOUSEEVENTF_LEFTUP
                               , 0, 0, 0, GetMessageExtraInfo());
                    InterlockedIncrement(&state.ignoreclick);
                    init_movement_and_actions(pt, NULL, AC_MOVE, 0);
                }
            }
            KillTimer(g_timerhwnd, GRAB_TIMER);
        }
        return 0;
    } else if (msg == WM_DESTROY) {
        KillTimer(g_timerhwnd, REHOOK_TIMER);
        KillTimer(g_timerhwnd, SPEED_TIMER);
        KillTimer(g_timerhwnd, GRAB_TIMER);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// Window for single click commands for menu
LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND fhwndori = NULL;
    if (msg == WM_CREATE) {
        // Save the original foreground window.
        fhwndori = GetForegroundWindow();

    } else if (msg == WM_INITMENU) {
        state.unikeymenu = (HMENU)wParam;
        // state.sclickhwnd = (HWND)lParam; // Child hwnd that was clicked.
    } else if (msg == WM_GETCLICKHWND) {
        return (LRESULT)state.sclickhwnd;
    } else if (msg == WM_COMMAND && wParam > 32) {
        Send_KEY(VK_BACK); // Errase old char...

        // Send UCS-2 or Lower+Upper UTF-16 surrogates of the UNICODE char.
        SendUnicodeKey(LOWORD(wParam)); // USC-2 or Lower surrogate
        if(HIWORD(wParam)) SendUnicodeKey(HIWORD(wParam)); // Upper surrogate

        state.sclickhwnd = NULL;
    } else if (msg == WM_COMMAND && IsWindow(state.sclickhwnd)) {
        // ACTION MENU
        enum action action = wParam;
        if (action) {
            state.prevpt = state.clickpt;
            SClickActions(state.sclickhwnd, action);

            // We should not refocus windows if those
            // actions were performed...
            if (action == AC_LOWER || action == AC_MINIMIZE
            ||  action == AC_KILL || action == AC_CLOSE)
                state.sclickhwnd = NULL;
        }
        // Menu closes now.
        state.unikeymenu = NULL;
        DestroyWindow(hwnd); // Done!
        return 0;
    // OWNER DRAWN MENU !!!!!
    } else if (msg == WM_MEASUREITEM) {
        LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT)lParam;
        if(!lpmi) return FALSE;
        struct menuitemdata *data = (struct menuitemdata *)lpmi->itemData;
        if(!data) return FALSE;
        wchar_t *text = data->txt;
        //LOGA("WM_MEASUREITEM: id=%u, txt=%S", lpmi->itemID, data->txt);

        HDC dc = GetDC(hwnd);
        UINT dpi = GetDpiForWindow(hwnd);

        // Select proper font.
        HFONT mfont = GetNCMenuFont(dpi);
        HFONT oldfont=SelectObject(dc, mfont);

        int xmargin = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
        int ymargin = GetSystemMetricsForDpi(SM_CYFIXEDFRAME, dpi);
        int xicosz =  GetSystemMetricsForDpi(SM_CXSMICON, dpi);
        int yicosz =  GetSystemMetricsForDpi(SM_CYSMICON, dpi);

        SIZE sz; // Get text size in both dimentions
        GetTextExtentPoint32(dc, text, wcslen(text), &sz);

        // Text width + icon width + 4 margins
        lpmi->itemWidth = sz.cx + xicosz + 4*xmargin;

        // Text height/Icon height + margin
        lpmi->itemHeight = max(sz.cy, yicosz) + ymargin;

        SelectObject(dc, oldfont); // restore old font
        DeleteObject(mfont); // Delete menufont.
        ReleaseDC(hwnd, dc);
        return TRUE;

    } else if (msg == WM_DRAWITEM) {
        // WE MUST DRAW THE MENU ITEM HERE
        LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
        if (!di) return FALSE;
        struct menuitemdata *data = (struct menuitemdata *)di->itemData;
        if (!data) return FALSE;

        // Try to be dpi-aware as good as we can...
        UINT dpi = GetDpiForWindow(hwnd);
        int xmargin = GetSystemMetricsForDpi(SM_CXFIXEDFRAME, dpi);
        int xicosz =  GetSystemMetricsForDpi(SM_CXSMICON, dpi);
        int yicosz =  GetSystemMetricsForDpi(SM_CYSMICON, dpi);

//        LOGA("WM_DRAWITEM: id=%u, txt=%S", di->itemID, data->txt);

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
        HBRUSH bgbrush = GetSysColorBrush(bgcol);
        // Set
        SetBkColor(di->hDC, GetSysColor(bgcol));
        SetTextColor(di->hDC, GetSysColor(txcol));

        // Highlight menu entry
        HPEN oldpen=SelectObject(di->hDC, GetStockObject(NULL_PEN));
        HBRUSH oldbrush=SelectObject(di->hDC, bgbrush);
        Rectangle(di->hDC, di->rcItem.left, di->rcItem.top, di->rcItem.right+1, di->rcItem.bottom+1);
        int totheight = di->rcItem.bottom - di->rcItem.top; // total menuitem height

        HFONT mfont = GetNCMenuFont(dpi);
        HFONT oldfont=SelectObject(di->hDC, mfont);

        SIZE sz;
        GetTextExtentPoint32(di->hDC, data->txt, wcslen(data->txt), &sz);
//        LOGA("WM_DRAWITEM: txtXY=%u, %u, txt=%S", (UINT)sz.cx, (UINT)sz.cy, data->txt);

        int yicooffset = (totheight - yicosz)/2; // Center icon vertically
        int ytxtoffset = (totheight - sz.cy)/2;   // Center text vertically

        DrawIconEx(di->hDC
            , di->rcItem.left+xmargin
            , di->rcItem.top + yicooffset
            , data->icon, xicosz, yicosz
            , 0, 0, DI_NORMAL);
        // Adjust x offset for Text drawing...
        di->rcItem.left += xicosz + xmargin*3;
        di->rcItem.top += ytxtoffset;
//        LOGA("menuitemheight = %ld", di->rcItem.bottom-di->rcItem.top);
        DrawText(di->hDC, data->txt, -1, &di->rcItem, 0); // Menuitem Text

        // restore dc context
        SelectObject(di->hDC, oldfont); // restore old font
        DeleteObject(mfont); // Delete menufont.
        SelectObject(di->hDC, oldpen);
        SelectObject(di->hDC, oldbrush);
        return TRUE;

    } else if (msg == WM_MENUCHAR) {
//        LOGA("WM_MENUCHAR: %X", wParam);
        // Turn the input character into a menu identifier.
        WORD cc = LOWORD(wParam);
        // Lower case the input character.
        TCHAR c = (TCHAR)( cc  |  ((cc - 'A' < 26)<<5) );
        WORD item;
        if (conf.NumberMenuItems) {
            item = ('0' <= c && c <= '9')? c-'0'
                 : ('a' <= c && c <= 'z')? c-'a'+10
                 : 0xFFFF;
        } else {
            item = ('a' <= c && c <= 'z')? c-'a'
                 : ('0' <= c && c <= '9')? c-'0'+26
                 : 0xFFFF;
        }
       // Execute item if the key is valid.
       if (item != 0xFFFF)
           return item|MNC_EXECUTE<<16;
    } else if (msg == WM_KILLFOCUS) {
        // Menu gets hiden, be sure to zero-out the clickhwnd
        state.sclickhwnd = NULL;
    } else if (msg == WM_DESTROY) {
        if (state.sclickhwnd == fhwndori && IsWindow(fhwndori)) {
            // Restore the foreground window
            SetForegroundWindow(fhwndori);
        }
        fhwndori = NULL;
        state.sclickhwnd = NULL;
        state.unikeymenu = NULL;
    }
    // LOGA("msg=%X, wParam=%X, lParam=%lX", msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK HotKeysWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_HOTKEY) {
        int action = wParam - 0xC000; // Remove the Offset
        if (action > AC_RESIZE) { // Exclude resize action in case...
            // Some actions pass directly through the default blacklists...
            POINT pt;
            GetCursorPos(&pt);
            if (!conf.UsePtWindow
            && (action == AC_MENU)) {
                pt.x = MAXLONG;
            }
            if (action == AC_KILL || action == AC_PAUSE || action == AC_RESUME) {
                SClickActions(conf.UsePtWindow? WindowFromPoint(pt): GetForegroundWindow(), action);
            } else {
                state.shift = state.ctrl = 0; // In case...
                init_movement_and_actions(pt, conf.UsePtWindow? NULL: GetForegroundWindow(), action, 0);
                state.blockmouseup = 0;
                state.hwnd=NULL;
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void freeblacklists()
{
    struct blacklist *list = (void *)&BlkLst;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        free(list->data);
        free(list->items);
        list++;
    }
}
static BOOL WINAPI DestroyPinWindowsProc(HWND hwnd, LPARAM lp)
{
    if(isClassName(hwnd, APP_NAME"-Pin"))
        DestroyWindow(hwnd);

    return TRUE; // Next hwnd
}
/////////////////////////////////////////////////////////////////////////////
// To be called before Free Library. Ideally it should free everything
__declspec(dllexport) void Unload()
{
    conf.keepMousehook = 0;
    if (mousehook) { UnhookWindowsHookEx(mousehook); mousehook = NULL; }
    DestroyWindow(g_timerhwnd);
    KillAltSnapMenu();
    if (conf.TransWinOpacity) {
        DestroyWindow(g_transhwnd[0]);
    } else {
        int i;
        for (i=0; i<4; i++) DestroyWindow(g_transhwnd[i]);
    }

    unsigned ac;
    for(ac=AC_MENU; ac<AC_MAXVALUE; ac++)
        UnregisterHotKey(g_hkhwnd, 0xC000+ac);
    DestroyWindow(g_hkhwnd);

    EnumWindows(DestroyPinWindowsProc, 0);
    UnregisterClass(APP_NAME"-Timers", hinstDLL);
    UnregisterClass(APP_NAME"-SClick", hinstDLL);
    UnregisterClass(APP_NAME"-Trans",  hinstDLL);
    UnregisterClass(APP_NAME"-Pin",    hinstDLL);
    UnregisterClass(APP_NAME"-HotKeys",    hinstDLL);

    freeblacklists();

    free(monitors);
    free(hwnds);
    free(wnds);
    free(snwnds);
    free(minhwnds);
}
/////////////////////////////////////////////////////////////////////////////
// blacklist is coma separated and title and class are | separated.
static void readblacklist(const wchar_t *inipath, struct blacklist *blacklist, const char *bl_str)
{
    wchar_t txt[1968];
    wchar_t bl_W[32];
    str2wide(bl_W, bl_str);

    blacklist->data = NULL;
    blacklist->length = 0;
    blacklist->items = NULL;

    DWORD ret = GetPrivateProfileString(L"Blacklist", bl_W, L"", txt, ARR_SZ(txt), inipath);
    if (!ret || txt[0] == '\0') {
        return;
    }
    blacklist->data = malloc((wcslen(txt)+1)*sizeof(wchar_t));
    wcscpy(blacklist->data, txt);
    wchar_t *pos = blacklist->data;

    while (pos) {
        wchar_t *title = pos;
        wchar_t *class = wcschr(pos, L'|'); // go to the next |

        // Move pos to next item (if any)
        pos = wcschr(pos, L',');
        // Zero out the coma and eventual spaces
        if (pos) {
            do {
                *pos++ = '\0';
            } while(*pos == ' ');
        }

        // Split the item with NULL
        if (class) {
           *class = '\0';
           class++;
        }
        // Add blacklist item
        if (title) {
            if (title[0] == '\0') {
                title = L"";
            } else if (title[0] == '*' && title[1] == '\0') {
                title = NULL;
            }
            if (class && class[0] == '*' && class[1] == '\0') {
                class = NULL;
            }
            // Allocate space
            blacklist->items = realloc(blacklist->items, (blacklist->length+1)*sizeof(struct blacklistitem));

            // Store item
            blacklist->items[blacklist->length].title = title;
            blacklist->items[blacklist->length].classname = class;
            blacklist->length++;
        }
    } // end while
}
// Read all the blacklitsts
void readallblacklists(wchar_t *inipath)
{
    struct blacklist *list = &BlkLst.Processes;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        readblacklist(inipath, list+i, BlackListStrings[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
// Used to read Hotkeys and Hotclicks
static void readhotkeys(const wchar_t *inipath, const char *name, const wchar_t *def, UCHAR *keys)
{
    wchar_t txt[64];
    wchar_t nameW[64];
    str2wide(nameW, name);

    GetPrivateProfileString(L"Input", nameW, def, txt, ARR_SZ(txt), inipath);
    UCHAR i=0;
    wchar_t *pos = txt;
    while (*pos) {
        // Store key
        if (i == MAXKEYS) break;
        keys[i++] = whex2u(pos);

        while (*pos && *pos != ' ') pos++; // go to next space
        while (*pos == ' ') pos++; // go to next char after spaces.
    }
    keys[i] = 0;
}
// Map action string to actual action enum
static enum action readaction(const wchar_t *inipath, const wchar_t *key)
{
    wchar_t txt[32];
    GetPrivateProfileString(L"Input", key, L"Nothing", txt, ARR_SZ(txt), inipath);

    static const char *action_map[] = ACTION_MAP;
    enum action ac;
    for (ac=0; ac < ARR_SZ(action_map); ac++) {
        if(!wscsicmp(txt, action_map[ac])) return ac;
    }
    return AC_NONE;
}
// Read all buttons actions from inipath
static void readbuttonactions(const wchar_t *inipath)
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

        wchar_t key[32];
        str2wide(key, buttons[i]);
        int len = wcslen(key);
        // Read primary action (no sufix)
        actionptr[4*i+0] = readaction(inipath, key);

        key[len] = 'B'; key[len+1] = '\0'; // Secondary B sufixe
        actionptr[4*i+1] = readaction(inipath, key);

        key[len] = 'T'; key[len+1] = '\0'; // Titlebar T sufixes
        actionptr[4*i+2] = readaction(inipath, key);

        key[len] = 'T'; key[len+1] = 'B'; key[len+2] = '\0'; // TB
        actionptr[4*i+3] = readaction(inipath, key);
    }
}
///////////////////////////////////////////////////////////////////////////
// Create a window for msessages handeling timers, menu etc.
static HWND KreateMsgWin(WNDPROC proc, wchar_t *name)
{
    WNDCLASSEX wnd;
    if(!GetClassInfoEx(hinstDLL, name, &wnd)) {
        // Register the class if no already created.
        memset(&wnd, 0, sizeof(wnd));
        wnd.cbSize = sizeof(WNDCLASSEX);
        wnd.lpfnWndProc = proc;
        wnd.hInstance = hinstDLL;
        wnd.lpszClassName = name;
        RegisterClassEx(&wnd);
    }
    return CreateWindowEx(0, wnd.lpszClassName, NULL, 0
                     , 0, 0, 0, 0, g_mainhwnd, NULL, hinstDLL, NULL);
}
///////////////////////////////////////////////////////////////////////////
// Has to be called at startup, it mainly reads the config.
__declspec(dllexport) void Load(HWND mainhwnd)
{
    // Load settings
    wchar_t inipath[MAX_PATH];
    unsigned i;
    state.action = AC_NONE;
    state.shift = 0;
    state.moving = 0;
    LastWin.hwnd = NULL;

    // Get ini path
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    wcscpy(&inipath[wcslen(inipath)-3], L"ini");

    #pragma GCC diagnostic ignored "-Wpointer-sign"
    static const struct OptionListItem {
        const wchar_t *section; const char *name; const int def;
    } optlist[] = {
        // [General]
        {L"General", "AutoFocus", 0 },
        {L"General", "AutoSnap", 0 },
        {L"General", "Aero", 1 },
        {L"General", "SmartAero", 1 },
        {L"General", "StickyResize", 0 },
        {L"General", "InactiveScroll", 0 },
        {L"General", "MDI", 0 },
        {L"General", "ResizeCenter", 1 },
        {L"General", "CenterFraction", 24 },
        {L"General", "AeroHoffset", 50 },
        {L"General", "AeroVoffset", 50 },
        {L"General", "MoveTrans", 255 },
        {L"General", "MMMaximize", 1 },

        // [Advanced]
        {L"Advanced", "ResizeAll", 1 },
        {L"Advanced", "FullScreen", 1 },
        {L"Advanced", "BLMaximized", 0 },
        {L"Advanced", "AutoRemaximize", 0 },
        {L"Advanced", "SnapThreshold", 20 },
        {L"Advanced", "AeroThreshold", 5 },
        {L"Advanced", "AeroTopMaximizes", 1 },
        {L"Advanced", "UseCursor", 1 },
        {L"Advanced", "MinAlpha", 32 },
        {L"Advanced", "AlphaDeltaShift", 8 },
        {L"Advanced", "AlphaDelta", 64 },
        {L"Advanced", "ZoomFrac", 16 },
        {L"Advanced", "ZoomFracShift", 64 },
        {L"Advanced", "NumberMenuItems", 0},
        /* AeroMaxSpeed not here... */
        {L"Advanced", "AeroSpeedTau", 64 },
        {L"Advanced", "SnapGap", 0 },
        {L"Advanced", "ShiftSnaps", 1 },
        {L"Advanced", "PiercingClick", 0 },
        {L"Advanced", "DragSendsAltCtrl", 0 },
        {L"Advanced", "TopmostIndicator", 0 },

        // [Performance]
        {L"Performance", "FullWin", 2 },
        {L"Performance", "TransWinOpacity", 0 },
        {L"Performance", "RefreshRate", 0 },
        {L"Performance", "RezTimer", 0 },
        {L"Performance", "PinRate", 32 },
        {L"Performance", "MoveRate", 2 },
        {L"Performance", "ResizeRate", 4 },

        // [Input]
        {L"Input", "TTBActions", 0 },
        {L"Input", "KeyCombo", 0 },
        {L"Input", "ScrollLockState", 0 },
        {L"Input", "LongClickMove", 0 },
        {L"Input", "UniKeyHoldMenu", 0 },

        // [Zones]
        {L"Zones", "UseZones", 0 },
        {L"Zones", "InterZone", 0 },
      # ifdef WIN64
        {L"Zones", "FancyZone", 0 },
      # endif
        // [KBShortcuts]
        {L"KBShortcuts", "UsePtWindow", 0 },
    };
    #pragma GCC diagnostic pop

    // Read all char options
    UCHAR *dest = &conf.AutoFocus; // 1st element.
    for (i=0; i < ARR_SZ(optlist); i++) {
        wchar_t name[128];
        str2wide(name, optlist[i].name);
        *dest++ = GetPrivateProfileInt(optlist[i].section, name, optlist[i].def, inipath);
    }

    // [General] consistency checks
    conf.CenterFraction=CLAMP(0, conf.CenterFraction, 100);
    conf.AHoff        = CLAMP(0, conf.AHoff,          100);
    conf.AVoff        = CLAMP(0, conf.AVoff,          100);
    conf.AeroSpeedTau = max(1, conf.AeroSpeedTau);
    conf.MinAlpha     = max(1, conf.MinAlpha);
    state.snap = conf.AutoSnap;

    // [Advanced]
    conf.ZoomFrac      = max(2, conf.ZoomFrac);
    conf.ZoomFracShift = max(2, conf.ZoomFracShift);
    conf.BLCapButtons  = GetPrivateProfileInt(L"Advanced", L"BLCapButtons", 3, inipath);
    conf.BLUpperBorder = GetPrivateProfileInt(L"Advanced", L"BLUpperBorder", 3, inipath);
    conf.AeroMaxSpeed  = GetPrivateProfileInt(L"Advanced", L"AeroMaxSpeed", 65535, inipath);
    if (conf.TopmostIndicator) {
        int color[4];
        readhotkeys(inipath, "PinColor",  L"FF FF 00 54", (UCHAR *)&color[0]);
        conf.PinColor = color[0];
    }

    // [Input]
    readbuttonactions(inipath);

    // Same order than in the conf struct
    static const struct hklst {
        char *name; wchar_t *def;
    } hklst[] = {
        { "Hotkeys",   L"A4 A5" },
        { "Shiftkeys", L"A0 A1" },
        { "Hotclicks", NULL    },
        { "Killkeys",  L"09 2E" },
        { "XXButtons", NULL },
        { "ModKey",    NULL },
        { "HScrollKey", L"10" },
    };
    for (i=0; i < ARR_SZ(hklst); i++) {
        readhotkeys(inipath, hklst[i].name, hklst[i].def, &conf.Hotkeys[i*(MAXKEYS+1)]);
    }

    // Read all the BLACKLITSTS
    readallblacklists(inipath);

    ResetDB(); // Zero database of restore info (snap.c)

    // [Zones] Configuration
    if (conf.UseZones&1) { // We are using Zones
        if(conf.UseZones&2) { // Grid Mode
            unsigned GridNx = GetPrivateProfileInt(L"Zones", L"GridNx", 0, inipath);
            unsigned GridNy = GetPrivateProfileInt(L"Zones", L"GridNy", 0, inipath);
            if (GridNx && GridNy)
                GenerateGridZones(GridNx, GridNy);
        } else {
            ReadZones(inipath);
        }
    }

    // [Performance]
    if (conf.RezTimer) conf.RefreshRate=0; // Ignore the refresh rate in RezTimer mode.
    if (conf.FullWin == 2) { // Use current config to determine if we use FullWin.
        BOOL drag_full_win=1;  // Default to ON if unable to detect
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &drag_full_win, 0);
        conf.FullWin = drag_full_win;
    }

    // Prepare the transparent window
    if (!conf.FullWin) {
        int color[4]; // 16 bytes to be sure no overfows
        // Read the color for the TransWin from ini file
        readhotkeys(inipath, "FrameColor",  L"80 00 80", (UCHAR *)&color[0]);
        WNDCLASSEX wnd;
        memset(&wnd, 0, sizeof(wnd));
        wnd.cbSize = sizeof(WNDCLASSEX);
        wnd.lpfnWndProc = DefWindowProc;
        wnd.hInstance = hinstDLL;
        wnd.hbrBackground = CreateSolidBrush(color[0]);
        wnd.lpszClassName = APP_NAME"-Trans";
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
            for (i=0; i<4; i++) { // the transparent window is made with 4 thin windows
                g_transhwnd[i] = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW  //|WS_EX_NOACTIVATE|
                                 , wnd.lpszClassName, NULL, WS_POPUP
                                 , 0, 0, 0, 0, NULL, NULL, hinstDLL, NULL);
                LOG("CreateWindowEx[i] = %lX", (DWORD)(DorQWORD)g_transhwnd[i]);
            }
        }
    }

    conf.keepMousehook = ((conf.TTBActions&1) // titlebar action w/o Alt
                       || conf.InactiveScroll // Inactive scrolling
                       || conf.Hotclick[0] // Hotclick
                       || conf.LongClickMove); // Move with long click
    // Capture main hwnd from caller. This is also the cursor wnd
    g_mainhwnd = mainhwnd;

    if (conf.keepMousehook || conf.AeroMaxSpeed < 65535) {
        g_timerhwnd = KreateMsgWin(TimerWindowProc, APP_NAME"-Timers");
    }

    g_hkhwnd = KreateMsgWin(HotKeysWinProc, APP_NAME"-HotKeys");
    // MOD_ALT=1, MOD_CONTROL=2, MOD_SHIFT=4, MOD_WIN=8
    // RegisterHotKey(g_hkhwnd, 0xC000 + AC_KILL,   MOD_ALT|MOD_CONTROL, VK_F4); // F4=73h
    // Read All shortcuts in the [KBShortcuts] section.
    static const char *action_names[] = ACTION_MAP;
    unsigned ac;
    for (ac=AC_MENU; ac < ARR_SZ(action_names); ac++) {
        wchar_t txt[32];
        str2wide(txt, action_names[ac]);
        WORD HK = GetPrivateProfileInt(L"KBShortcuts", txt, 0, inipath);
        if(LOBYTE(HK) && HIBYTE(HK)) {
            // Lobyte is the virtual key code and hibyte is the mod_key
            if(!RegisterHotKey(g_hkhwnd, 0xC000 + ac, HIBYTE(HK), LOBYTE(HK))) {
                LOG("Error registering hotkey %s=%x", action_names[ac], (unsigned)HK);
            }
        }
    }

    // Hook mouse if a permanent hook is needed
    if (conf.keepMousehook) {
        HookMouse();
        SetTimer(g_timerhwnd, REHOOK_TIMER, 5000, NULL); // Start rehook timer
    }
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
