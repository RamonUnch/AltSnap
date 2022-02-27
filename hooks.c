/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2021                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"
#define LONG_CLICK_MOVE
#define COBJMACROS
BOOL CALLBACK EnumMonitorsProc(HMONITOR, HDC, LPRECT , LPARAM );

// Timer messages
#define REHOOK_TIMER    WM_APP+1
#define SPEED_TIMER     WM_APP+2
#define GRAB_TIMER      WM_APP+3

#define CURSOR_ONLY 66
#define NOT_MOVED 33

#define STACK 0x1000

HWND g_transhwnd[4]; // 4 windows to make a hollow window
HWND g_timerhwnd;    // For various timers
HWND g_mchwnd;       // For the Action menu messages
static void UnhookMouse();
static void HookMouse();

// Enumerators
enum button { BT_NONE=0, BT_LMB=0x02, BT_RMB=0x03, BT_MMB=0x04, BT_MB4=0x05
            , BT_MB5=0x06, BT_WHEEL=0x07, BT_HWHEEL=0x08 };
enum resize { RZ_NONE=0, RZ_TOP, RZ_RIGHT, RZ_BOTTOM, RZ_LEFT, RZ_CENTER };
enum buttonstate {STATE_NONE, STATE_DOWN, STATE_UP};

static int init_movement_and_actions(POINT pt, enum action action, int button);
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

    HWND hwnd;
    HWND sclickhwnd;
    HWND mdiclient;
    DWORD clicktime;
    short hittest;
    short delta;
    unsigned Speed;

    UCHAR alt;
    UCHAR alt1;
    UCHAR blockaltup;
    UCHAR blockmouseup;

    UCHAR ignorekey;
    UCHAR ignoreclick;
    UCHAR ctrl;
    UCHAR shift;

    UCHAR snap;
    UCHAR moving;

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

    enum action action;
    struct {
        enum resize x, y;
    } resize;
} state;
// mdiclientpt is global!
// initialized by init_movement_and_actions
static POINT mdiclientpt;


// Snap
RECT *monitors = NULL;
unsigned nummonitors = 0;
RECT *wnds = NULL;
unsigned numwnds = 0;
HWND *hwnds = NULL;
unsigned numhwnds = 0;

// Settings
#define MAXKEYS 7
static struct {
    UCHAR AutoFocus;
    UCHAR AutoSnap;
    UCHAR AutoRemaximize;
    UCHAR Aero;

    UCHAR MDI;
    UCHAR InactiveScroll;
    UCHAR TTBActions;
    UCHAR ResizeCenter;

    UCHAR MoveRate;
    UCHAR ResizeRate;
    UCHAR SnapThreshold;
    UCHAR AeroThreshold;

    UCHAR AVoff;
    UCHAR AHoff;
    UCHAR FullWin;
    UCHAR ResizeAll;

    UCHAR AggressivePause;
    UCHAR AeroTopMaximizes;
    UCHAR UseCursor;
    UCHAR CenterFraction;

    UCHAR RefreshRate;
    UCHAR MMMaximize;
    UCHAR MinAlpha;
    UCHAR MoveTrans;

    char AlphaDelta;
    char AlphaDeltaShift;
    unsigned short AeroMaxSpeed;

    UCHAR keepMousehook;
    UCHAR KeyCombo;
    UCHAR FullScreen;
    UCHAR AggressiveKill;

    UCHAR SmartAero;
    UCHAR StickyResize;
    UCHAR HScrollKey;
    UCHAR ScrollLockState;

    UCHAR ShiftSnaps;
    UCHAR BLMaximized;
    UCHAR LongClickMove;

  # ifdef WIN64
    UCHAR FancyZone;
  #endif
    UCHAR UseZones;
    char InterZone;
    char SnapGap;

    UCHAR PiercingClick;
    UCHAR AeroSpeedTau;
    UCHAR ModKey;
    UCHAR RezTimer;

    UCHAR Hotkeys[MAXKEYS+1];
    UCHAR Shiftkeys[MAXKEYS+1];
    UCHAR Hotclick[MAXKEYS+1];
    UCHAR Killkey[MAXKEYS+1];

    struct {
        enum action LMB[4], RMB[4], MMB[4], MB4[4], MB5[4], Scroll[4], HScroll[4];
    } Mouse;
    enum action GrabWithAlt[2]; // Actions without click
    enum action MoveUp[2];      // Actions on (long) Move Up w/o drag
    enum action ResizeUp[2];    // Actions on (long) Resize Up w/o drag
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
static const struct {
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
    "NCHittest"
};
// Cursor data
HWND g_mainhwnd = NULL;

// Hook data
HINSTANCE hinstDLL = NULL;
HHOOK mousehook = NULL;

#define FixDWMRect(hwnd, rect) FixDWMRectLL(hwnd, rect, conf.SnapGap)
#undef GetWindowRectL
#define GetWindowRectL(hwnd, rect) GetWindowRectLL(hwnd, rect, conf.SnapGap)

// Specific includes
#include "snap.c"
#include "zones.c"

/////////////////////////////////////////////////////////////////////////////
// wether a window is present or not in a blacklist
static pure int blacklisted(HWND hwnd, const struct blacklist *list)
{
    wchar_t title[256]=L"", classname[256]=L"";
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
    wchar_t title[MAX_PATH]=L"";
    DorQWORD mode ;
    unsigned i ;

    // Null hwnd or empty list
    if (!hwnd || !list->length)
        return 0;
    // If the first element is *|* then we are in whitelist mode
    // mode = 1 => blacklist mode = 0 => whitelist;
    mode = (DorQWORD)list->items[0].title;
    i = !mode;

    GetWindowProgName(hwnd, title, ARR_SZ(title));

    // ProcessBlacklist is case-insensitive
    for ( ; i < list->length; i++) {
        if (list->items[i].title && !wcsicmp(title, list->items[i].title))
            return mode;
    }
    return !mode;
}

// To clamp width and height of windows
static pure int CLAMPW(int width)  { return CLAMP(state.mmi.Min.x, width,  state.mmi.Max.x); }
static pure int CLAMPH(int height) { return CLAMP(state.mmi.Min.y, height, state.mmi.Max.y); }

static int pure IsResizable(HWND hwnd)
{
    int ret =  conf.ResizeAll // bit two is the real thickframe state.
            | ((!!(GetWindowLongPtr(hwnd, GWL_STYLE)&WS_THICKFRAME)) << 1);

    if (!ret) ret = !!GetBorderlessFlag(hwnd);
    if (!ret) ret = !!blacklisted(hwnd, &BlkLst.AResize); // Always resize list

    return ret;
}
/////////////////////////////////////////////////////////////////////////////
// WM_ENTERSIZEMOVE or WM_EXITSIZEMOVE...
static void SendSizeMove(DWORD msg)
{
    // Don't send WM_ENTER/EXIT SIZEMOVE if the window is in SSizeMove BL
    if(!blacklisted(state.hwnd, &BlkLst.SSizeMove)) {
        PostMessage(state.hwnd, msg, 0, 0);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Overloading of the Hittest function to include a whitelist
static int HitTestTimeoutblL(HWND hwnd, LPARAM lParam)
{
    DorQWORD area=0;

    // Try first with the ancestor window for some buggy AppX?
    HWND ancestor = GetAncestor(hwnd, GA_ROOT);
    if (blacklisted(ancestor, &BlkLst.MMBLower)) return 0;
    if (hwnd != ancestor
    && blacklisted(ancestor, &BlkLst.NCHittest)) {
        SendMessageTimeout(ancestor, WM_NCHITTEST, 0, lParam, SMTO_NORMAL, 200, &area);
        if(area == HTCAPTION) return HTCAPTION;
    }
    return HitTestTimeoutL(hwnd, lParam);
}
#define HitTestTimeoutbl(hwnd, x, y) HitTestTimeoutblL(hwnd, MAKELPARAM(x, y))
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
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &mi);
    CopyRect(&monitors[nummonitors++], &mi.rcWork); //*lprcMonitor;

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
static void OffsetRectMDI(RECT *wnd)
{
    OffsetRect(wnd, -mdiclientpt.x, -mdiclientpt.y);
}
static int ShouldSnapTo(HWND window)
{
    LONG_PTR style;
    return window != state.hwnd
        && IsVisible(window)
        && !IsIconic(window)
        &&( ((style=GetWindowLongPtr(window, GWL_STYLE))&WS_CAPTION) == WS_CAPTION
           || (style&WS_THICKFRAME)
           || blacklisted(window,&BlkLst.Snaplist)
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
            HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(monitor, &mi);
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

        if ((restore = GetRestoreFlag(hwnd)) && restore&SNAPPED && restore&SNAPPEDSIDE) {
            snwnds[numsnwnds].flag = restore;
        } else if (conf.SmartAero&2 || IsWindowSnapped(hwnd)) {
            HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(monitor, &mi);
            snwnds[numsnwnds].flag = WhichSideRectInRect(&mi.rcWork, &wnd);
        } else {
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
        if(flag) {
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

        if(!PtInRect(&state.origin.mon, (POINT){nwnd->left+16, nwnd->top+16}))
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
// Pass NULL to reset Enum state and recalculate it
// at the next non null ptr.
static void EnumOnce(RECT **bd)
{
    static char enumed;
    static RECT borders;
    if (bd && !enumed) {
        Enum(); // Enumerate monitors and windows
        FixDWMRect(state.hwnd, &borders);
        enumed = 1;
        *bd = &borders;
    } else if (bd && enumed) {
        *bd = &borders;
    } else if (!bd) {
        enumed = 0;
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
static void ResizeSnap(int *posx, int *posy, int *wndwidth, int *wndheight)
{
    if(!state.snap || state.Speed > conf.AeroMaxSpeed) return;

    // thresholdx and thresholdy will shrink to make sure
    // the dragged window will snap to the closest windows
    short thresholdx, thresholdy;
    UCHAR stuckleft=0, stucktop=0, stuckright=0, stuckbottom=0;
    int stickleft=0, sticktop=0, stickright=0, stickbottom=0;
    thresholdx = thresholdy = conf.SnapThreshold;
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
    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd != SW_FULLSCREEN)
        wndpl.showCmd = sw_cmd;

    RECT fmon;
    if(sw_cmd == SW_MAXIMIZE || sw_cmd == SW_FULLSCREEN) {
        HMONITOR wndmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        HMONITOR monitor = MonitorFromPoint(state.prevpt, MONITOR_DEFAULTTONEAREST);

        MONITORINFO mi = { sizeof(MONITORINFO) };
        GetMonitorInfo(monitor, &mi);
        CopyRect(&fmon, &mi.rcMonitor);

        // Center window on monitor, if needed
        if (monitor != wndmonitor) {
            CenterRectInRect(&wndpl.rcNormalPosition, &mi.rcWork);
        }
    }

    SetWindowPlacement(hwnd, &wndpl);
    if (sw_cmd == SW_FULLSCREEN) {
        MoveWindowAsync(hwnd, fmon.left, fmon.top, fmon.right-fmon.left, fmon.bottom-fmon.top);
    }
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

#define RESIZEFLAG        SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE
#define MOVETHICKBORDERS  SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE
#define MOVEASYNC         SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS|SWP_DEFERERASE
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

    if (state.snap != conf.AutoSnap) return; // do not go further is snapping state is toggled.

    // Check on all the other snapped windows from the bottom most
    // To give precedence to the topmost windows
    unsigned i = numsnwnds;
    while (i--) {
        unsigned flag = snwnds[i].flag;
        RECT *wnd = &snwnds[i].wnd;
        // if the window is in current monitor
        if (PtInRect(mon, (POINT) { wnd->left+16, wnd->top+16 })) {
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

    MONITORINFO mi = { sizeof(MONITORINFO) };
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
    if(!conf.Aero ||!state.resizable) return 0;
    LastWin.maximize = 0;
    LastWin.snap = 0;

    if (!state.moving) {
        EnumSnapped();
    }

    // We HAVE to check the monitor for each pt...
    RECT mon;
    GetMonitorRect(&pt, 0, &mon);

    int pLeft  = mon.left   + AERO_TH ;
    int pRight = mon.right  - AERO_TH ;
    int pTop   = mon.top    + AERO_TH ;
    int pBottom= mon.bottom - AERO_TH ;
    int leftWidth, rightWidth, topHeight, bottomHeight;

    unsigned restore = GetRestoreFlag(state.hwnd);
    if(PtInRect(&(RECT){ pLeft, pTop, pRight, pBottom}, pt)) goto restore;

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
         &&(state.Speed < conf.AeroMaxSpeed)) {
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
/////////////////////////////////////////////////////////////////////////////
// Mod Key can return 0 or 1, maybe more in the future...
static pure int ModKey()
{
    return conf.ModKey && GetAsyncKeyState(conf.ModKey)&0x8000;
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

///////////////////////////////////////////////////////////////////////////
// Check if key is assigned in the HKlist
static int pure IsHotkeyy(unsigned char key, const UCHAR *HKlist)
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
#define IsHotkey(a)   IsHotkeyy(a, conf.Hotkeys)
#define IsHotclick(a) IsHotkeyy(a, conf.Hotclick)
static int pure IsKillkey(unsigned char a)
{
    return
        (0x41 <= a && a <= 0x5A) // A-Z vkeys
        || IsHotkeyy(a, conf.Killkey) ;
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
        restore = rdata_flag;
        state.origin.width = rwidth;
        state.origin.height = rheight;
        ClearRestoreData(state.hwnd);
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
                , pt.x - state.offset.x - mdiclientpt.x
                , pt.y - state.offset.y - mdiclientpt.y
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
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(state.origin.monitor, &mi);
            CopyRect(&fmon, &mi.rcMonitor);
            fmon.left++; fmon.top++;
            fmon.right--; fmon.bottom--;
        }
        RECT clip;
        if (state.mdiclient) {
            CopyRect(&clip, &state.origin.mon);
            OffsetRect(&clip, mdiclientpt.x, mdiclientpt.y);
        } else {
            CopyRect(&clip, &fmon);
        }
        ClipCursorOnce(&clip);
    }
}
///////////////////////////////////////////////////////////////////////////
// Get mdiclientpt and mdi monitor
static BOOL GetMDInfo(POINT *mdicpt, RECT *wnd)
{
    *mdicpt= (POINT) { 0, 0 };
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
    int i;
    for (i=0; i<4; i++ )if(g_transhwnd[i]) ShowWindow(g_transhwnd[i], nCmdShow);
}
#define HideTransWin() ShowTransWin(SW_HIDE)

static void MoveTransWin(int x, int y, int w, int h)
{
      #define f SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER
      HDWP hwndSS = BeginDeferWindowPos(4);
      if(hwndSS) hwndSS = DeferWindowPos(hwndSS,g_transhwnd[0],NULL,  x    , y    , w, 4, f);
      if(hwndSS) hwndSS = DeferWindowPos(hwndSS,g_transhwnd[1],NULL,  x    , y    , 4, h, f);
      if(hwndSS) hwndSS = DeferWindowPos(hwndSS,g_transhwnd[2],NULL,  x    , y+h-4, w, 4, f);
      if(hwndSS) hwndSS = DeferWindowPos(hwndSS,g_transhwnd[3],NULL,  x+w-4, y    , 4, h, f);
      #undef f
      if(hwndSS) EndDeferWindowPos(hwndSS);
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
    // mdiclientpt is global!
    pt.x -= mdiclientpt.x;
    pt.y -= mdiclientpt.y;

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
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
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
//            SetWindowPlacementThread(state.hwnd, &wndpl);
        }

    } else if (state.action == AC_RESIZE) {
        // Restore the window (to monitor size) if it's maximized
        if (!state.moving && IsZoomed(state.hwnd)) {
            ClearRestoreData(state.hwnd); //Clear restore flag and data
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
            GetWindowPlacement(state.hwnd, &wndpl);

            // Set size to origin monitor to prevent flickering
            CopyRect(&wnd, &state.origin.mon);
            CopyRect(&wndpl.rcNormalPosition, &wnd);

            if (state.mdiclient) {
                // Make it a little smaller since MDIClients by
                // default have scrollbars that would otherwise appear
                wndpl.rcNormalPosition.right -= 8;
                wndpl.rcNormalPosition.bottom -= 8;
            }
            wndpl.showCmd = SW_RESTORE;
            SetWindowPlacement(state.hwnd, &wndpl);
            if (state.mdiclient) {
                // Get new values from MDIClient,
                // since restoring the child have changed them,
                Sleep(1); // Sometimes needed
                GetMDInfo(&mdiclientpt, &wnd);
                CopyRect(&state.origin.mon, &wnd);

                state.origin.right = wnd.right;
                state.origin.bottom=wnd.bottom;
            }
            // Fix wnd for MDI offset and invisible borders
            RECT borders;
            FixDWMRect(state.hwnd, &borders);
            OffsetRect(&wnd, mdiclientpt.x, mdiclientpt.y);
            InflateRectBorder(&wnd, &borders);
        }
        // Clear restore flag
        if (!(conf.SmartAero&1)) {
            // Always clear when AeroSmart is disabled.
            ClearRestoreData(state.hwnd);
        } else {
            // Do not clear is the window was snapped to some side or rolled.
            unsigned smart_restore_flag=(SNZONE|SNAPPEDSIDE|ROLLED);
            if(!(GetRestoreFlag(state.hwnd) & smart_restore_flag))
                ClearRestoreData(state.hwnd);
        }

        // Figure out new placement
        if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
            if (state.shift) pt.x = state.shiftpt.x;
            else if (state.ctrl) pt.y = state.ctrlpt.y;
            wndwidth  = wnd.right-wnd.left + 2*(pt.x-state.offset.x);
            wndheight = wnd.bottom-wnd.top + 2*(pt.y-state.offset.y);
            posx = wnd.left - (pt.x - state.offset.x) - mdiclientpt.x;
            posy = wnd.top  - (pt.y - state.offset.y) - mdiclientpt.y;
            state.offset.x = pt.x;
            state.offset.y = pt.y;
        } else {
            if (state.resize.y == RZ_TOP) {
                wndheight = CLAMPH( (wnd.bottom-pt.y+state.offset.y) - mdiclientpt.y );
                posy = state.origin.bottom - wndheight;
            } else if (state.resize.y == RZ_CENTER) {
                posy = wnd.top - mdiclientpt.y;
                wndheight = wnd.bottom - wnd.top;
            } else if (state.resize.y == RZ_BOTTOM) {
                posy = wnd.top - mdiclientpt.y;
                wndheight = pt.y - posy + state.offset.y;
            }
            if (state.resize.x == RZ_LEFT) {
                wndwidth = CLAMPW( (wnd.right-pt.x+state.offset.x) - mdiclientpt.x );
                posx = state.origin.right - wndwidth;
            } else if (state.resize.x == RZ_CENTER) {
                posx = wnd.left - mdiclientpt.x;
                wndwidth = wnd.right - wnd.left;
            } else if (state.resize.x == RZ_RIGHT) {
                posx = wnd.left - mdiclientpt.x;
                wndwidth = pt.x - posx+state.offset.x;
            }
            wndwidth =CLAMPW(wndwidth);
            wndheight=CLAMPH(wndheight);

            // Check if the window will snap anywhere
            ResizeSnap(&posx, &posy, &wndwidth, &wndheight);
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
    wnd.left   = posx + mdiclientpt.x;
    wnd.top    = posy + mdiclientpt.y;
    wnd.right  = posx + mdiclientpt.x + wndwidth;
    wnd.bottom = posy + mdiclientpt.y + wndheight;

    if (!conf.FullWin) {
        static RECT bd;
        if(!state.moving) FixDWMRectLL(state.hwnd, &bd, 0);
        int tx      = posx + mdiclientpt.x + bd.left;
        int ty      = posy + mdiclientpt.y + bd.top;
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
    KEYBDINPUT ctrl[2] = { {vkey, 0, 0, 0, 0}, {vkey, 0 , KEYEVENTF_KEYUP, 0, 0} };
    ctrl[0].dwExtraInfo = ctrl[1].dwExtraInfo = GetMessageExtraInfo();
    INPUT input[2] = { {INPUT_KEYBOARD,{.ki = ctrl[0]}}, {INPUT_KEYBOARD,{.ki = ctrl[1]}} };
    state.ignorekey = 1;
    SendInput(2, input, sizeof(INPUT));
    state.ignorekey = 0;
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
    if (!button) return;

    DWORD MouseEvent = bmapping[button-2];
    DWORD mdata = 0;
    if (MouseEvent == MOUSEEVENTF_XDOWN) // XBUTTON
        mdata = button - 0x04; // mdata = 1 for X1 and 2 for X2
    // MouseEvent<<1 corresponds to MOUSEEVENTF_*UP
    MOUSEINPUT click[2] = { {0, 0, mdata, MouseEvent, 0, 0}
                          , {0, 0, mdata, MouseEvent<<1, 0, 0} };
    click[0].dwExtraInfo = click[1].dwExtraInfo = GetMessageExtraInfo();
    INPUT input[2] = { {INPUT_MOUSE, {.mi = click[0]}}, {INPUT_MOUSE, {.mi = click[1]}} };

    state.ignoreclick = 1;
    SendInput(2, input, sizeof(INPUT));
    state.ignoreclick = 0;
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
    // The way this works is that the alt key is "disguised" by sending ctrl keydown/keyup events
    if (state.blockaltup || state.action) {
        Send_CTRL();
    }

    // Hotkeys have been released
    state.alt = 0;
    state.alt1 = 0;
    state.blockaltup = 0;
    if (state.action && conf.GrabWithAlt[0]) {
        FinishMovement();
    }

    // Unhook mouse if no actions is ongoing.
    if (!state.action) {
        UnhookMouse();
    }
}

///////////////////////////////////////////////////////////////////////////
static int ActionPause(HWND hwnd, char pause)
{
    if (!blacklistedP(hwnd, &BlkLst.Pause)) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (ProcessHandle) {
            if (pause) NtSuspendProcess(ProcessHandle);
            else       NtResumeProcess(ProcessHandle);

            CloseHandle(ProcessHandle);
            return 1;
        }
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
// Kill the process from hwnd
DWORD WINAPI ActionKillThread(LPVOID hwnd)
{
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
static int ActionKill(HWND hwnd)
{
    //LOG("hwnd=%lx",(DWORD) hwnd);
    if (!hwnd) return 0;

    wchar_t classn[256];
    if(GetClassName(hwnd, classn, ARR_SZ(classn))
    && !wcscmp(classn, L"Ghost")) {
        PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        return 1;
    }

    if(blacklistedP(hwnd, &BlkLst.Pause))
       return 0;

    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, ActionKillThread, hwnd, 0, &lpThreadId));

    return 1;
}

static void SetForegroundWindowL(HWND hwnd)
{
    if (!state.mdiclient) {
        SetForegroundWindow(hwnd);
    } else {
        SetForegroundWindow(state.mdiclient);
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
    fprintf(f, "action=%d\nmoving=%d\nctrl=%d\nshift=%d\nalt=%d\nalt1=%d\n"
    , (int)state.action, (int)state.moving, (int)state.ctrl
    , (int)state.shift, (int)state.alt, (int)state.alt1);

    fprintf(f, "clickbutton=%d\nhwnd=%lx\nlwhwnd=%lx\nlwend=%d\n"
    "lwmaximize=%d\nlwmoveonly=%d\nlwsnap=%d\n"
    "blockaltup=%d\nblockmouseup=%d\nignorekey=%d\n\n\n"
    , (int)state.clickbutton, (DWORD)(DorQWORD)state.hwnd, (DWORD)(DorQWORD)LastWin.hwnd
    , (int)LastWin.end, (int)LastWin.maximize, (int)LastWin.moveonly, (int)LastWin.snap
    , (int)state.blockaltup, (int)state.blockmouseup, (int)state.ignorekey );

    fclose(f);
}
///////////////////////////////////////////////////////////////////////////
// Keep this one minimalist, it is always on.
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
//    if (state.ignorekey) LOGA("IgnoreKey")
    if (nCode != HC_ACTION || state.ignorekey) return CallNextHookEx(NULL, nCode, wParam, lParam);

    unsigned char vkey = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;
    if (vkey == VK_SCROLL) PostMessage(g_mainhwnd, WM_UPDATETRAY, 0, 0);
    if (ScrollLockState()) return CallNextHookEx(NULL, nCode, wParam, lParam);

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        if (!state.alt && !state.action
        && (!conf.KeyCombo || (state.alt1 && state.alt1 != vkey))
        && IsHotkey(vkey)) {
            //LOGA("\nALT DOWN");
            state.alt = vkey;
            state.blockaltup = 0;
            state.blockmouseup = 0;

            // Hook mouse
            HookMouse();
            if (conf.GrabWithAlt[0]) {
                POINT pt;
                enum action action = conf.GrabWithAlt[(vkey==conf.ModKey) || (!IsHotkey(conf.ModKey)&&ModKey())];
                if (action) {
                    GetCursorPos(&pt);
                    if (!init_movement_and_actions(pt, action, vkey)) {
                        UnhookMouse();
                    }
                }
            }
        } else if (conf.KeyCombo && !state.alt1 && IsHotkey(vkey)) {
            state.alt1 = vkey;

        } else if (IsHotkeyy(vkey, conf.Shiftkeys)) {
            if (!state.shift && vkey != conf.ModKey) {
                EnumOnce(NULL); // Reset enum state.
                if(conf.ShiftSnaps) state.snap = 3;
                state.shift = 1;
                state.shiftpt = state.prevpt; // Save point where shift was pressed.
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
            //LogState("ESCAPE KEY WAS PRESSED:\n"); // Debug stuff....
            state.ctrl = 0;
            state.shift = 0;
            LastWin.hwnd = NULL;
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
        } else if (conf.AggressivePause && state.alt && vkey == VK_PAUSE) {
            POINT pt;
            GetCursorPos(&pt);
            HWND hwnd = WindowFromPoint(pt);
            if (ActionPause(hwnd, state.shift)) return 1;
        } else if (conf.AggressiveKill && state.alt && state.ctrl && vkey == VK_F4) {
            // Kill on Ctrl+Alt+F4
            POINT pt; GetCursorPos(&pt);
            HWND hwnd = WindowFromPoint(pt);
            if(ActionKill(hwnd)) return 1;
        } else if (!state.ctrl && state.alt!=vkey && vkey != conf.ModKey
               && (vkey == VK_LCONTROL || vkey == VK_RCONTROL)) {
            RestrictToCurentMonitor();
            state.ctrl = 1;
            state.ctrlpt = state.prevpt; // Save point where ctrl was pressed.
            if (state.action) {
                SetForegroundWindowL(state.hwnd);
            }
        } else if (state.sclickhwnd && state.alt && (vkey == VK_LMENU || vkey == VK_RMENU)) {
            return 1;
        }

    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
        if (IsHotkey(vkey)) {
            //LOGA("ALT UP");
            HotkeyUp();
        } else if (IsHotkeyy(vkey, conf.Shiftkeys)) {
            state.shift = 0;
            state.snap = conf.AutoSnap;
            // if an action was performed and Alt is still down.
            if (state.alt && state.blockaltup && (vkey == VK_LSHIFT || vkey == VK_RSHIFT))
                Send_CTRL(); // send Ctrl to avoid Alt+Shift=>switch keymap
        } else if (vkey == VK_LCONTROL || vkey == VK_RCONTROL) {
            ClipCursorOnce(NULL); // Release cursor trapping
            state.ctrl = 0;
            // If there is no action then Control UP prevents AltDragging...
            if (!state.action) state.alt = 0;
        } else if (vkey == conf.ModKey) {
            ; // TODO, block modkey with Ctrl when needed...
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
static HWND GetClass_HideIfTooltip(POINT pt, HWND hwnd, wchar_t *classname, size_t classlen)
{
    GetClassName(hwnd, classname, classlen);

    if (!wcscmp(classname, TOOLTIPS_CLASS)) {
        ShowWindowAsync(hwnd, SW_HIDE);
        hwnd = WindowFromPoint(pt);
        if (!hwnd) return NULL;

        GetClassName(hwnd, classname, classlen);
    }
    return hwnd;
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

    // Get class behind eventual tooltip
    wchar_t classname[20] = L"";
    hwnd = GetClass_HideIfTooltip(pt, hwnd, classname, ARR_SZ(classname));

    // If it's a groupbox, grab the real window
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((style&BS_GROUPBOX) && !wcscmp(classname,L"Button")) {
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
    if ((wParam == WM_MOUSEWHEEL && (GetAsyncKeyState(conf.HScrollKey) &0x8000))) {
        wParam = WM_MOUSEHWHEEL;
        wp = -wp ; // Up is left, down is right
    }

    // Add button information since we don't get it with the hook
    if (GetAsyncKeyState(VK_LBUTTON) &0x8000) wp |= MK_LBUTTON;
    if (GetAsyncKeyState(VK_RBUTTON) &0x8000) wp |= MK_RBUTTON;
    if (GetAsyncKeyState(VK_CONTROL) &0x8000) wp |= MK_CONTROL;
    if (GetAsyncKeyState(VK_SHIFT)   &0x8000) wp |= MK_SHIFT;
    if (GetAsyncKeyState(VK_MBUTTON) &0x8000) wp |= MK_MBUTTON;
    if (GetAsyncKeyState(VK_XBUTTON1)&0x8000) wp |= MK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2)&0x8000) wp |= MK_XBUTTON2;

    // Forward scroll message
    SendMessage(hwnd, wParam, wp, lp);

    // Block original scroll event
    return 1;
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
    if (IsWindowVisible(window) && !IsIconic(window)
    && (GetWindowLongPtr(window, GWL_STYLE)&WS_CAPTION) == WS_CAPTION
    && state.origin.monitor == MonitorFromWindow(window, MONITOR_DEFAULTTONULL)) {
        hwnds[numhwnds++] = window;
    }
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// Returns the GA_ROOT window if not MDI or MDIblacklist
static HWND MDIorNOT(HWND hwnd, HWND *mdiclient_)
{
    HWND mdiclient = NULL;
    HWND root = GetAncestor(hwnd, GA_ROOT);

    if (conf.MDI && !blacklisted(root, &BlkLst.MDIs)) {
        while (hwnd != root) {
            HWND parent = GetParent(hwnd);
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
static int ActionAltTab(POINT pt, int delta)
{
    numhwnds = 0;

    if (conf.MDI) {
        // Get Class and Hide if tooltip
        wchar_t classname[32] = L"";
        HWND hwnd = WindowFromPoint(pt);
        hwnd = GetClass_HideIfTooltip(pt, hwnd, classname, ARR_SZ(classname));

        if (!hwnd) return 0;
        // Get MDIClient
        HWND mdiclient = NULL;
        if (!wcscmp(classname, L"MDIClient")) {
            mdiclient = hwnd; // we are pointing to the MDI client!
        } else {
            MDIorNOT(hwnd, &mdiclient); // Get mdiclient from hwnd
        }
        // Enumerate and then reorder MDI windows
        if (mdiclient) {
            EnumChildWindows(mdiclient, EnumAltTabWindows, 0);

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
        EnumWindows(EnumAltTabWindows, 0);
        if (numhwnds <= 1) {
            return 0;
        }

        // Reorder windows
        if (delta > 0) {
            SetForegroundWindow(hwnds[numhwnds-1]);
        } else {
            SetWindowLevel(hwnds[0], hwnds[numhwnds-1]);
            SetForegroundWindow(hwnds[1]);
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
/////////////////////////////////////////////////////////////////////////////
static void TogglesAlwaysOnTop(HWND hwnd);
static int xpure IsAeraCapbutton(int area);
static void ActionLower(HWND hwnd, int delta, UCHAR shift)
{
    // turn lower in Always on top if Ctrl or [_][O][X]
    if (delta > 0) {
        if (shift) {
            ToggleMaxRestore(hwnd);
        } else {
            if (conf.AutoFocus || state.ctrl) SetForegroundWindowL(hwnd);
            SetWindowLevel(hwnd, HWND_TOPMOST);
            SetWindowLevel(hwnd, HWND_NOTOPMOST);
        }
    } else if (delta == 0 && (state.ctrl || IsAeraCapbutton(state.hittest))) {
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
            SetWindowLevel(hwnd, HWND_BOTTOM);
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
        SetWindowPos(g_mainhwnd, NULL, pt.x-8, pt.y-8, 16, 16
                    , SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE);
        SetClassLongPtr(g_mainhwnd, GCLP_HCURSOR, (LONG_PTR)CursorToDraw());
        ShowWindow(g_mainhwnd, SW_SHOWNA);
    }
}

static int IsMXRolled(HWND hwnd, RECT *rc)
{
    MONITORINFO mi = { sizeof(MONITORINFO) };
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(monitor, &mi);
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
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
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
        state.offset.x = pt.x-mdiclientpt.x; // Used only if both x and y are CENTER
    } else {
        state.resize.x = RZ_RIGHT;
        state.offset.x = wnd->right-pt.x;
    }
    if (pt.y-wnd->top < (wndheight*SideS)/100) {
        state.resize.y = RZ_TOP;
        state.offset.y = pt.y-wnd->top;
    } else if (pt.y-wnd->top < (wndheight*CenteR)/100) {
        state.resize.y = RZ_CENTER;
        state.offset.y = pt.y-mdiclientpt.y;
    } else {
        state.resize.y = RZ_BOTTOM;
        state.offset.y = wnd->bottom-pt.y;
    }
    // Set window right/bottom origin
    state.origin.right = wnd->right-mdiclientpt.x;
    state.origin.bottom = wnd->bottom-mdiclientpt.y;
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
        posx = wnd.left - mdiclientpt.x;
        posy = wnd.top - mdiclientpt.y;

        if (state.resize.y == RZ_TOP) {
            posy = mon->top - bd.top;
            wndheight = CLAMPH(wnd.bottom-mdiclientpt.y - mon->top + bd.top);
        } else if (state.resize.y == RZ_BOTTOM) {
            wndheight = CLAMPH(mon->bottom - wnd.top+mdiclientpt.y + bd.bottom);
        }
        if (state.resize.x == RZ_RIGHT) {
            wndwidth =  CLAMPW(mon->right - wnd.left+mdiclientpt.x + bd.right);
        } else if (state.resize.x == RZ_LEFT) {
            posx = mon->left - bd.left;
            wndwidth =  CLAMPW(wnd.right-mdiclientpt.x - mon->left + bd.left);
        } else if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
            wndwidth = CLAMPW(mon->right - mon->left + bd.left + bd.right);
            posx = mon->left - bd.left;
            posy = wnd.top - mdiclientpt.y + bd.top ;
            restore |= SNMAXW;
        }
    } else { /* Aero Snap to corresponding side/corner */
        int leftWidth, rightWidth, topHeight, bottomHeight;
        EnumSnapped();
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
            posx = wnd.left - mdiclientpt.x + bd.left;
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
    if (IsZoomed(state.hwnd)) MaximizeRestore_atpt(state.hwnd, SW_RESTORE);
    MoveWindowAsync(hwnd, posx, posy, wndwidth, wndheight);
    // Save data to the window...
    SetRestoreData(hwnd, state.origin.width, state.origin.height, SNAPPED|restore);
}
/////////////////////////////////////////////////////////////////////////////
static int ActionResize(POINT pt, const RECT *wnd, int button)
{
    if(!state.resizable) {
        // state.blockmouseup = 1;
        state.action = AC_NONE;
        return 0; //1
    }
    // Aero-move this window if this is a double-click
    if (IsDoubleClick(button)) {
        SnapToCorner(state.hwnd);
        state.blockmouseup = 1; // Block mouse up (context menu would pops)
        state.clicktime = 0;    // Reset double-click time
        // Prevent mousedown from propagating
        return 1;
    }
    SetEdgeAndOffset(wnd, pt);
    if (state.resize.y == RZ_CENTER && state.resize.x == RZ_CENTER) {
        if (conf.ResizeCenter == 0) {
            state.resize.x = RZ_RIGHT;
            state.resize.y = RZ_BOTTOM;
            state.offset.y = wnd->bottom-pt.y;
            state.offset.x = wnd->right-pt.x;
        } else if (conf.ResizeCenter == 2) {
            state.action = AC_MOVE;
        }
    }
    return -1;
}
/////////////////////////////////////////////////////////////////////////////
static void ActionBorderless(HWND hwnd)
{
    LONG_PTR style;
    // Get anc clear eventual old style.
    LONG_PTR ostyle = ClearBorderlessFlag(hwnd);
    if (ostyle) {
        // Restore old style if found
        style = ostyle;
    } else if ((style=GetWindowLongPtr(hwnd, GWL_STYLE))) {
        SetBorderlessFlag(hwnd, style); // Save style
        if((style&WS_CAPTION) == WS_CAPTION || style&WS_THICKFRAME) {
            style &= state.shift? ~WS_CAPTION: ~(WS_CAPTION|WS_THICKFRAME);
        } else {
            style |= WS_OVERLAPPEDWINDOW;
        }
    }
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Under Windows 10, with DWM we HAVE to resize the windows twice
    // to have proper drawing. this is a bug...
    if (HaveDWM()) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top+1
                   , SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_FRAMECHANGED|SWP_NOZORDER);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top
                   , SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_NOZORDER);
    } else {
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_NOZORDER);
    }
}
/////////////////////////////////////////////////////////////////////////////
static int IsFullscreen(HWND hwnd, const RECT *wnd, const RECT *fmon)
{
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    // no caption and fullscreen window => LSB to 1
    return ((style&WS_CAPTION) != WS_CAPTION)
//        && ((style&WS_SYSMENU) != WS_SYSMENU)
        && EqualRect(wnd, fmon);
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
static void TogglesAlwaysOnTop(HWND hwnd)
{
    LONG_PTR topmost = GetWindowLongPtr(hwnd, GWL_EXSTYLE)&WS_EX_TOPMOST;
    SetWindowLevel(hwnd, topmost? HWND_NOTOPMOST: HWND_TOPMOST);
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
    if (CurrentMonOnly)  hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    if (restore == hwnd){
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
    else if (action==AC_ROLL)        RollWindow(hwnd, 0);
    else if (action==AC_MAXHV)       MaximizeHV(hwnd, state.shift);
    else if (action==AC_MINALL)      MinimizeAllOtherWindows(hwnd, state.shift);
    else if (action==AC_MUTE)        Send_KEY(VK_VOLUME_MUTE);
    else if (action==AC_SIDESNAP)    SnapToCorner(hwnd);
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

    if      (action == AC_ALTTAB)       ret = ActionAltTab(state.prevpt, state.delta);
    else if (action == AC_VOLUME)       ActionVolume(state.delta);
    else if (action == AC_TRANSPARENCY) ret = ActionTransparency(hwnd, state.delta);
    else if (action == AC_LOWER)        ActionLower(hwnd, state.delta, state.shift);
    else if (action == AC_MAXIMIZE)     ActionMaxRestMin(hwnd, state.delta);
    else if (action == AC_ROLL)         RollWindow(hwnd, state.delta);
    else if (action == AC_HSCROLL)      ret = ScrollPointedWindow(state.prevpt, -state.delta, WM_MOUSEHWHEEL);
    else                                ret = 0; // No action

    // ret is 0: next hook or 1: block whel and AltUp.
    state.blockaltup = ret && state.alt > 8; // block or not;
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

/////////////////////////////////////////////////////////////////////////////
// action cannot be AC_NONE here...
static int init_movement_and_actions(POINT pt, enum action action, int button)
{
    RECT wnd;

    // Make sure g_mainhwnd isn't in the way
    HideCursor();

    // Get window
    state.mdiclient = NULL;
    state.hwnd = WindowFromPoint(pt);
    DorQWORD lpdwResult;
    // Hide if tooltip
    wchar_t classname[20] = L"";
    state.hwnd = GetClass_HideIfTooltip(pt, state.hwnd, classname, ARR_SZ(classname));
    // Get MDI chlild hwnd or root hwnd if not MDI!
    state.hwnd = MDIorNOT(state.hwnd, &state.mdiclient);

    if (!state.hwnd || state.hwnd == LastWin.hwnd) {
        return 0;
    }

    // Get monitor info
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(monitor, &mi);
    CopyRect(&state.origin.mon, &mi.rcWork);
    RECT fmon;
    CopyRect(&fmon, &mi.rcMonitor);

    // mdiclientpt HAS to be set to Zero for ClientToScreen adds the offset
    mdiclientpt = (POINT) { 0, 0 };
    if (state.mdiclient) {
        GetMDInfo(&mdiclientpt, &fmon);
        CopyRect(&state.origin.mon, &fmon);
    }

    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    // Return if window is blacklisted,
    // if we can't get information about it,
    // or if the window is fullscreen.
    if (!state.hwnd
    || blacklistedP(state.hwnd, &BlkLst.Processes)
    ||(blacklisted(state.hwnd, &BlkLst.Windows)
       && !state.hittest && button != BT_WHEEL && button != BT_HWHEEL
      )// does not apply in titlebar, nor for the wheel action...
    || GetWindowPlacement(state.hwnd, &wndpl) == 0
    || GetWindowRect(state.hwnd, &wnd) == 0
    || ((state.origin.maximized = IsZoomed(state.hwnd)) && conf.BLMaximized)
    || ((state.origin.fullscreen = IsFullscreen(state.hwnd, &wnd, &fmon)) && !conf.FullScreen)
    ){
        return 0;
    }

    // An action will be performed...
    // Set state
    state.blockaltup = state.alt; // If alt is down...
    // return if window has to be moved/resized and does not respond in 1/4 s.
    if (MOUVEMENT(action)
    && !SendMessageTimeout(state.hwnd, 0, 0, 0, SMTO_NORMAL, 255, &lpdwResult)) {
        state.blockmouseup = 1;
        return 1; // Unresponsive window...
    }

    state.prevpt=pt;

    // Set origin width/height by default from current state/wndpl.
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);
    state.origin.width  = wndpl.rcNormalPosition.right-wndpl.rcNormalPosition.left;
    state.origin.height = wndpl.rcNormalPosition.bottom-wndpl.rcNormalPosition.top;

    // Set current snapping mode from the config.
    if (!state.snap) { state.snap = conf.AutoSnap; }

    // AutoFocus
    if (conf.AutoFocus || state.ctrl) { SetForegroundWindowL(state.hwnd); }

    // Do things depending on what button was pressed
    if (MOUVEMENT(action)) {
        // Set action state.
        state.action = action; // MOVE OR RESIZE
        state.resizable = IsResizable(state.hwnd);

        GetMinMaxInfo(state.hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions
        SetWindowTrans(state.hwnd);
        EnumOnce(NULL); // Reset enum stuff
        StartSpeedMes(); // Speed timer

        int ret;
        if (state.action == AC_MOVE) {
            ret = ActionMove(pt, button);
        } else {
            ret = ActionResize(pt, &wnd, button);
        }
        if      (ret == 1) return 1; // block mouse down!
        else if (ret == 0) return 0; // Next hook!
        else // ret == -1 ...
        UpdateCursor(pt);

        // Send WM_ENTERSIZEMOVE
        SendSizeMove(WM_ENTERSIZEMOVE);
    } else if (action == AC_MENU) {
        state.sclickhwnd = state.hwnd;
        PostMessage(g_mainhwnd, WM_SCLICK, (WPARAM)g_mchwnd, conf.AggressiveKill);
        return 1; // block mouse down
    } else if(button == BT_WHEEL || button == BT_HWHEEL) {
        // Wheel actions, directly return here
        // because maybe the action will not be done
        return DoWheelActions(state.hwnd, action);
    } else {
        SClickActions(state.hwnd, action);
        state.blockmouseup = 1; // because the is done
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
    return area == HTMINBUTTON || area == HTMAXBUTTON
        || area == HTCLOSE || area == HTHELP;
}
static int xpure IsAreaCaption(int area)
{
    return area == HTCAPTION
       || (area >= HTTOP && area <= HTTOPRIGHT)
       || area == HTSYSMENU;
}
static int IsAreaAnyCap(int area)
{
    return IsAreaCaption(area) || IsAeraCapbutton(area);
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
        int area = HitTestTimeoutbl(nhwnd, pt.x, pt.y);
        if (area == HTCAPTION || (button > BT_RMB && IsAreaAnyCap(area))) {
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
    if((state.hittest = InTitlebar(pt, action, button ))) {
        return init_movement_and_actions(pt, action, button);
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
static void ClickComboActions(enum action action)
{
    // Maximize/Restore the window if pressing Move, Resize mouse buttons.
    if(state.action == AC_MOVE && action == AC_RESIZE) {
        WaitMovementEnd();
        if (IsZoomed(state.hwnd)) {
            if (IsSamePTT(&state.clickpt, &state.prevpt)) {
                RestoreWindow(state.hwnd);
            } else {
                state.moving = 0;
                MouseMove(state.prevpt);
            }
        } else if (state.resizable) {
            state.moving = CURSOR_ONLY; // So that MouseMove will only move g_mainhwnd
            HideTransWin();
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
static xpure enum button GetButton(WPARAM wp, LPARAM lp)
{
    PMSLLHOOKSTRUCT msg = (PMSLLHOOKSTRUCT)lp;
    return
        (wp==WM_LBUTTONDOWN||wp==WM_LBUTTONUP)?BT_LMB:
        (wp==WM_MBUTTONDOWN||wp==WM_MBUTTONUP)?BT_MMB:
        (wp==WM_RBUTTONDOWN||wp==WM_RBUTTONUP)?BT_RMB:
        (wp==WM_MOUSEWHEEL)?BT_WHEEL:
        (wp==WM_MOUSEHWHEEL)?BT_HWHEEL:
        (HIWORD(msg->mouseData)==XBUTTON1)?BT_MB4:
        (HIWORD(msg->mouseData)==XBUTTON2)?BT_MB5:BT_NONE;
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

    // Get button/wheel info and buttonstate+wheel info.
    enum button button = GetButton(wParam, lParam);
    enum buttonstate buttonstate = GetButtonState(wParam);
    // Get wheel delta
    state.delta = GET_WHEEL_DELTA_WPARAM(msg->mouseData);

//    if (button<=BT_MB5)
//        LOGA("button=%d, %s", button, buttonstate==STATE_DOWN?"DOWN":buttonstate==STATE_UP?"UP":"NONE");

    // Get actions!
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
        if (ret == 0) return CallNextHookEx(NULL, nCode, wParam, lParam);
        else if (ret == 1) return 1;
    }

    // Long click grab timer
    if(conf.LongClickMove && !state.action && !state.alt) {
        if (wParam == WM_LBUTTONDOWN) {
            state.clickpt = pt;
            // Start Grab timer
            SetTimer(g_timerhwnd, GRAB_TIMER, GetDoubleClickTime(), NULL);
        } else if (wParam == WM_LBUTTONUP) {
            KillTimer(g_timerhwnd, GRAB_TIMER);
        }
    }

    // Nothing to do...
    if (!action)
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Handle another click if we are already busy with an action
    if (buttonstate == STATE_DOWN && state.action && state.action != conf.GrabWithAlt[ModKey()]) {
        if ((conf.MMMaximize&1))
            ClickComboActions(action); // Handle click combo!
        return 1; // Block mousedown so AltDrag.exe does not remove g_mainhwnd

    // INIT ACTIONS on mouse down if Alt is down...
    } else if (buttonstate == STATE_DOWN && state.alt) {
        //LogState("BUTTON DOWN:\n");
        // Double ckeck some hotkey is pressed.
        if (!state.action
        && !IsHotclick(state.alt)
        && !IsHotkeyDown()) {
            UnhookMouse();
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        // Start an action (alt is down)
        int ret = init_movement_and_actions(pt, action, button);
        if (!ret) return CallNextHookEx(NULL, nCode, wParam, lParam);
        else      return 1; // block mousedown

    // BUTTON UP
    } else if (buttonstate == STATE_UP) {
//        LogState("BUTTON UP:\n");
        SetWindowTrans(NULL); // Reset window transparency
        if (state.blockmouseup) {
            // block mouse up and decrement counter.
            state.blockmouseup--;
            return 1;
        }

        if (state.action == action
        && !state.moving // No drag occured
        && !state.ctrl // Ctrl is not down (because of focusing)
        && IsSamePTT(&pt, &state.clickpt) // same point
        && !IsDoubleClick(button)) { // Long click unless PiercingClick=1
            FinishMovement();
            // Mouse UP actions here only in case of MOVEMENT!:
            // Perform an action on mouse up without drag on move/resize
            if (action == AC_MOVE)   action = conf.MoveUp[ModKey()];
            if (action == AC_RESIZE) action = conf.ResizeUp[ModKey()];

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
    return CallNextHookEx(NULL, nCode, wParam, lParam);
} // END OF LL MOUSE PROCK

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

    // Do not unhook if not hooked or if the hook is still used for something
    if (!mousehook || conf.keepMousehook)
        return;

    // Remove mouse hook
    UnhookWindowsHookEx(mousehook);
    mousehook = NULL;
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
            if (state.moving) state.Speed=max(abs(oldpt.x-state.prevpt.x), abs(oldpt.y-state.prevpt.y));
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
            if (IsSamePTT(&state.prevpt, &state.clickpt)
            && (ptwnd = WindowFromPoint(state.prevpt))
            &&!IsAreaLongClikcable(HitTestTimeoutbl(ptwnd, state.prevpt.x, state.prevpt.y))) {
                state.ignoreclick=1;
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                state.ignoreclick=0;
                state.alt = 1;
                init_movement_and_actions(state.prevpt, AC_MOVE, 2);
                state.alt = 0;
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
// Window for single click commands
LRESULT CALLBACK SClickWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND && state.sclickhwnd) {
        enum action action = wParam;
        state.sclickhwnd = MDIorNOT(state.sclickhwnd, &state.mdiclient);

        SClickActions(state.sclickhwnd, action);
        state.sclickhwnd = NULL;

        return 0;
    } else if(msg == WM_KILLFOCUS) {
        // Menu gets hiden, be sure to zero-out the clickhwnd
        state.sclickhwnd = NULL;
    }
    // LOGA("msg=%X, wParam=%X, lParam=%lX", msg, wParam, lParam);
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
/////////////////////////////////////////////////////////////////////////////
// To be called before Free Library. Ideally it should free everything
__declspec(dllexport) void Unload()
{
    conf.keepMousehook = 0;
    if (mousehook) { UnhookWindowsHookEx(mousehook); mousehook = NULL; }
    DestroyWindow(g_timerhwnd);
    DestroyWindow(g_mchwnd);
    int i; for (i=0; i<4; i++) DestroyWindow(g_transhwnd[i]);
    UnregisterClass(APP_NAME"-Timers", hinstDLL);
    UnregisterClass(APP_NAME"-SClick", hinstDLL);
    UnregisterClass(APP_NAME"-Trans",  hinstDLL);

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
    struct blacklist *list = (void *)&BlkLst;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        readblacklist(inipath, list+i, BlackListStrings[i]);
    }
}
///////////////////////////////////////////////////////////////////////////
// Used to read Hotkeys and Hotclicks
static void readhotkeys(const wchar_t *inipath, const wchar_t *name, const wchar_t *def, UCHAR *keys)
{
    wchar_t txt[64];

    GetPrivateProfileString(L"Input", name, def, txt, ARR_SZ(txt), inipath);
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
static unsigned char readsinglekey(const wchar_t *inipath, const wchar_t *name,  const wchar_t *def)
{
    wchar_t txt[4];
    GetPrivateProfileString(L"Input", name, def, txt, ARR_SZ(txt), inipath);
    if (*txt) {
        return whex2u(txt);
    }
    return 0;
}
// Map action string to actual action enum
static enum action selectaction(const wchar_t *txt)
{
    static const char *action_map[] = ACTION_MAP;
    enum action ac;
    for (ac=0; ac < ARR_SZ(action_map); ac++) {
        if(!wscsicmp(txt, action_map[ac])) return ac;
    }
    return AC_NONE;
}
// Read all buttons actions from inipath
static UCHAR readbuttonactions(const wchar_t *inipath)
{
    wchar_t txt[32];
    static const struct {
        char *key;
        char *def;
        /* enum action *ptr; */
    } buttons[] = {
        // Primary Actions (performed with Alt+Click)
        // Must be in the same order than in the conf struct
        {"LMB",        "Move"     },
        {"LMBB",       "Resize"   },
        {"LMBT",       "Nothing"  },
        {"LMBTB",      "Nothing"  },

        {"RMB",        "Resize"   },
        {"RMBB",       "Move"     },
        {"RMBT",       "Nothing"  },
        {"RMBTB",      "Nothing"  },

        {"MMB",        "Maximize" },
        {"MMBB",       "Maximize" },
        {"MMBT",       "Lower"    },
        {"MMBTB",      "Nothing"  },

        {"MB4",        "Nothing"  },
        {"MB4B",       "Nothing"  },
        {"MB4T",       "Nothing"  },
        {"MB4TB",      "Nothing"  },

        {"MB5",        "Nothing"  },
        {"MB5B",       "Nothing"  },
        {"MB5T",       "Nothing"  },
        {"MB5TB",      "Nothing"  },

        {"Scroll",     "Nothing"  },
        {"ScrollB",    "Volume"   },
        {"ScrollT",    "Nothing"  },
        {"ScrollTB",   "Nothing"  },

        {"HScroll",    "Nothing"  },
        {"HScrollB",   "Nothing"  },
        {"HScrollT",   "Nothing"  },
        {"HScrollTB",  "Nothing"  },

        {"GrabWithAlt","Nothing"  },
        {"GrabWithAltB","Nothing" },

        {"MoveUp"     ,"Nothing"  },
        {"MoveUpB"    ,"Nothing"  },
        {"ResizeUp"   ,"Nothing"  },
        {"ResizeUpB"  ,"Nothing"  },
    };
    unsigned i;
    UCHAR action_menu_load = 0;
    for (i=0; i < ARR_SZ(buttons); i++) {
        wchar_t key[32];
        wchar_t def[32];
        str2wide(key, buttons[i].key);
        str2wide(def, buttons[i].def);
        GetPrivateProfileString(L"Input", key, def, txt, ARR_SZ(txt), inipath);
        enum action * const actionptr = &conf.Mouse.LMB[0]; // first action in list
        actionptr[i] = selectaction(txt);
        if (actionptr[i] == AC_MENU) action_menu_load = 1;
    }
    return action_menu_load;
}
///////////////////////////////////////////////////////////////////////////
// Create a window for msessages handeling.
static HWND KreateMsgWin(WNDPROC proc, wchar_t *name)
{
    WNDCLASSEX wnd = { sizeof(WNDCLASSEX), 0, proc, 0, 0, hinstDLL
                     , NULL, NULL, NULL, NULL, name, NULL };
    RegisterClassEx(&wnd);
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
        UCHAR *dest; wchar_t *section; char *name; int def;
    } optlist[] = {
        // [General]
        {&conf.AutoFocus,       L"General", "AutoFocus", 0 },
        {&conf.AutoSnap,        L"General", "AutoSnap", 0 },
        {&conf.Aero,            L"General", "Aero", 1 },
        {&conf.SmartAero,       L"General", "SmartAero", 1 },
        {&conf.StickyResize,    L"General", "StickyResize", 0 },
        {&conf.InactiveScroll,  L"General", "InactiveScroll", 0 },
        {&conf.MDI,             L"General", "MDI", 0 },
        {&conf.ResizeCenter,    L"General", "ResizeCenter", 1 },
        {&conf.CenterFraction,  L"General", "CenterFraction", 24 },
        {&conf.AHoff,           L"General", "AeroHoffset", 50 },
        {&conf.AVoff,           L"General", "AeroVoffset", 50 },
        {&conf.MoveTrans,       L"General", "MoveTrans", 255 },
        {&conf.MMMaximize,      L"General", "MMMaximize", 1 },

        // [Advanced]
        {&conf.ResizeAll,       L"Advanced", "ResizeAll", 1 },
        {&conf.FullScreen,      L"Advanced", "FullScreen", 1 },
        {&conf.BLMaximized,     L"Advanced", "BLMaximized", 0 },
        {&conf.AutoRemaximize,  L"Advanced", "AutoRemaximize", 0 },
        {&conf.SnapThreshold,   L"Advanced", "SnapThreshold", 20 },
        {&conf.AeroThreshold,   L"Advanced", "AeroThreshold", 5 },
        {&conf.AeroTopMaximizes,L"Advanced", "AeroTopMaximizes", 1 },
        {&conf.UseCursor,       L"Advanced", "UseCursor", 1 },
        {&conf.MinAlpha,        L"Advanced", "MinAlpha", 20 },
        {&conf.AlphaDeltaShift, L"Advanced", "AlphaDeltaShift", 8 },
        {&conf.AlphaDelta,      L"Advanced", "AlphaDelta", 64 },
        /* AeroMaxSpeed not here... */
        {&conf.AeroSpeedTau,    L"Advanced", "AeroSpeedTau", 64 },
        {&conf.SnapGap,         L"Advanced", "SnapGap", 0 },
        {&conf.ShiftSnaps,      L"Advanced", "ShiftSnaps", 1 },
        {&conf.PiercingClick,   L"Advanced", "PiercingClick", 0 },

        // [Performance]
        {&conf.FullWin,         L"Performance", "FullWin", 2 },
        {&conf.RefreshRate,     L"Performance", "RefreshRate", 0 },
        {&conf.RezTimer,        L"Performance", "RezTimer", 0 },
        {&conf.MoveRate,        L"Performance", "MoveRate", 2 },
        {&conf.ResizeRate,      L"Performance", "ResizeRate", 4 },

        // [Input]
        {&conf.TTBActions,      L"Input", "TTBActions", 0 },
        {&conf.AggressivePause, L"Input", "AggressivePause", 0 },
        {&conf.AggressiveKill,  L"Input", "AggressiveKill", 0 },
        {&conf.KeyCombo,        L"Input", "KeyCombo", 0 },
        {&conf.ScrollLockState, L"Input", "ScrollLockState", 0 },
        {&conf.LongClickMove,   L"Input", "LongClickMove", 0 },

        // [Zones]
        {&conf.UseZones,        L"Zones", "UseZones", 0 },
        {&conf.InterZone,       L"Zones", "InterZone", 0 },
      # ifdef WIN64
        {&conf.FancyZone,       L"Zones", "FancyZone", 0 },
      # endif
    };
    #pragma GCC diagnostic pop

    // Read all char options
    for (i=0; i < ARR_SZ(optlist); i++) {
        wchar_t name[128];
        str2wide(name, optlist[i].name);
        *optlist[i].dest = GetPrivateProfileInt(optlist[i].section, name, optlist[i].def, inipath);
    }

    // [General] consistency checks
    conf.CenterFraction=CLAMP(0, conf.CenterFraction, 100);
    conf.AHoff        = CLAMP(0, conf.AHoff,          100);
    conf.AVoff        = CLAMP(0, conf.AVoff,          100);
    conf.AeroSpeedTau = min(1, conf.AeroSpeedTau);
    conf.MinAlpha     = min(1, conf.MinAlpha);

    // [Advanced] Max Speed
    conf.AeroMaxSpeed  = GetPrivateProfileInt(L"Advanced", L"AeroMaxSpeed", 65535, inipath);

    // [Input]
    UCHAR action_menu_load = readbuttonactions(inipath);

    readhotkeys(inipath, L"Hotkeys",  L"A4 A5",   conf.Hotkeys);
    readhotkeys(inipath, L"Shiftkeys",L"A0 A1",   conf.Shiftkeys);
    readhotkeys(inipath, L"Hotclicks",L"",        conf.Hotclick);
    readhotkeys(inipath, L"Killkeys", L"09 2E",   conf.Killkey);

    conf.ModKey     = readsinglekey(inipath, L"ModKey", L"");
    conf.HScrollKey = readsinglekey(inipath, L"HScrollKey", L"10"); // VK_SHIFT

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
        int color[2];
        // Read the color for the TransWin from ini file
        readhotkeys(inipath, L"FrameColor",  L"80 00 80", (UCHAR *)&color[0]);
        WNDCLASSEX wnd = { sizeof(WNDCLASSEX), 0
                     , DefWindowProc, 0, 0, hinstDLL
                     , NULL, NULL
                     , CreateSolidBrush(color[0])
                     , NULL, APP_NAME"-Trans", NULL };
        ATOM transc = RegisterClassEx(&wnd);
        LOG("RegisterClassEx = %lX", (DWORD)transc);
        for (i=0; i<4; i++) { // the transparent window is made with 4 thin windows
            g_transhwnd[i] = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW  //|WS_EX_NOACTIVATE|
                             , APP_NAME"-Trans", NULL, WS_POPUP
                             , 0, 0, 0, 0, NULL, NULL, hinstDLL, NULL);
            LOG("CreateWindowEx[i] = %lX", (DWORD)(DorQWORD)g_transhwnd[i]);
        }
    }

    conf.keepMousehook = ((conf.TTBActions&1) || conf.InactiveScroll
                       || conf.Hotclick[0] || conf.LongClickMove);
    // Capture main hwnd from caller. This is also the cursor wnd
    g_mainhwnd = mainhwnd;

    if (conf.keepMousehook || conf.AeroMaxSpeed < 65535) {
        g_timerhwnd = KreateMsgWin(TimerWindowProc, APP_NAME"-Timers");
    }
    // Hook mouse if a permanent hook is needed
    if (conf.keepMousehook) {
        HookMouse();
        SetTimer(g_timerhwnd, REHOOK_TIMER, 5000, NULL); // Start rehook timer
    }
    // Window for Action Menu
    if (action_menu_load) {
        g_mchwnd = KreateMsgWin(SClickWindowProc, APP_NAME"-SClick");
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
