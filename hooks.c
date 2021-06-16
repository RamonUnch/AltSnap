/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2021                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0400
#include <windows.h>
#define COBJMACROS
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "unfuck.h"
#include "hooks.h"
// App
#define APP_NAME L"AltDrag"

// Boring stuff
#define INIT_TIMER      WM_APP+1
#define REHOOK_TIMER    WM_APP+2
#define SPEED_TIMER     WM_APP+3

#define CURSOR_ONLY 66
#define NOT_MOVED 33
#define STACK 0x1000

HWND g_timerhwnd;
HWND g_mchwnd;
static void UnhookMouse();
static void HookMouse();

// Enumerators
enum button { BT_NONE=0, BT_LMB=0x02, BT_RMB=0x03, BT_MMB=0x04, BT_MB4=0x05, BT_MB5=0x06 };
enum resize { RZ_NONE=0, RZ_TOP, RZ_RIGHT, RZ_BOTTOM, RZ_LEFT, RZ_CENTER };
enum cursor { HAND, SIZENWSE, SIZENESW, SIZENS, SIZEWE, SIZEALL };

static int init_movement_and_actions(POINT pt, enum action action, int button);
static void FinishMovement();

// Window database
#define NUMWNDDB 64
#define SNAPPED  1
#define ROLLED   2
#define SNLEFT    (1<<2)
#define SNRIGHT   (1<<3)
#define SNTOP     (1<<4)
#define SNBOTTOM  (1<<5)
#define SNMAXH    (1<<6)
#define SNMAXW    (1<<7)
#define SNCLEAR   (1<<8) // to clear the flag at init movement.
#define TORESIZE  (1<<9)
#define SNTHENROLLED (1<<10)
#define SNTOPLEFT     (SNTOP|SNLEFT)
#define SNTOPRIGHT    (SNTOP|SNRIGHT)
#define SNBOTTOMLEFT  (SNBOTTOM|SNLEFT)
#define SNBOTTOMRIGHT (SNBOTTOM|SNRIGHT)
#define SNAPPEDSIDE   (SNTOPLEFT|SNBOTTOMRIGHT)

#define PureLeft(flag)   ( (flag&SNLEFT) &&  !(flag&(SNTOP|SNBOTTOM))  )
#define PureRight(flag)  ( (flag&SNRIGHT) && !(flag&(SNTOP|SNBOTTOM))  )
#define PureTop(flag)    ( (flag&SNTOP) && !(flag&(SNLEFT|SNRIGHT))    )
#define PureBottom(flag) ( (flag&SNBOTTOM) && !(flag&(SNLEFT|SNRIGHT)) )

struct wnddata {
    unsigned restore;
    HWND hwnd;
    int width;
    int height;
};
struct {
    struct wnddata items[NUMWNDDB];
    struct wnddata *pos;
} wnddb;

RECT oldRect;
HDC hdcc;
HPEN hpenDot_Global=NULL;
HDWP hWinPosInfo;

struct windowRR {
    HWND hwnd;
    int end;
    int x;
    int y;
    int width;
    int height;
} LastWin;

// State
struct {
    POINT clickpt;
    POINT prevpt;
    POINT offset;
    HWND hwnd;
    HWND sclickhwnd;
    HWND mdiclient;
    struct wnddata *wndentry;
    DWORD clicktime;
    unsigned Speed;

    UCHAR alt;
    UCHAR alt1;
    UCHAR blockaltup;
    UCHAR blockmouseup;

    UCHAR ignorectrl;
    UCHAR ctrl;
    UCHAR shift;
    UCHAR snap;

    UCHAR moving;
    UCHAR clickbutton;
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

    struct {
        POINT Min;
        POINT Max;
    } mmi;
} state;

// Snap
RECT *monitors = NULL;
int nummonitors = 0;
RECT *wnds = NULL;
int numwnds = 0;
HWND *hwnds = NULL;
int numhwnds = 0;
HWND progman = NULL;

// Settings
#define MAXKEYS 7
struct hotkeys_s {
    unsigned char length;
    unsigned char keys[MAXKEYS];
};

struct {
    enum action GrabWithAlt;

    UCHAR AutoFocus;
    UCHAR AutoSnap;
    UCHAR AutoRemaximize;
    UCHAR Aero;

    UCHAR MDI;
    UCHAR InactiveScroll;
    UCHAR LowerWithMMB;
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
    UCHAR RollWithTBScroll;
    UCHAR MMMaximize;
    UCHAR MinAlpha;

    char AlphaDelta;
    char AlphaDeltaShift;
    unsigned short AeroMaxSpeed;

    UCHAR MoveTrans;
    UCHAR NormRestore;
    UCHAR AeroSpeedTau;
    UCHAR ToggleRzMvKey;

    UCHAR keepMousehook;
    UCHAR KeyCombo;
    UCHAR FullScreen;
    UCHAR AggressiveKill;

    UCHAR SmartAero;
    UCHAR StickyResize;
    UCHAR HScrollKey;
    UCHAR ScrollLockState;

    struct hotkeys_s Hotkeys;
    struct hotkeys_s Hotclick;
    struct hotkeys_s Killkey;

    struct {
        enum action LMB, RMB, MMB, MB4, MB5, Scroll, HScroll;
    } Mouse;
} conf;

// Blacklist (dynamically allocated)
struct blacklistitem {
    wchar_t *title;
    wchar_t *classname;
};
struct blacklist {
    struct blacklistitem *items;
    int length;
    wchar_t *data;
};
struct {
    struct blacklist Processes;
    struct blacklist Windows;
    struct blacklist Snaplist;
    struct blacklist MDIs;
    struct blacklist Pause;
    struct blacklist MMBLower;
    struct blacklist Scroll;
    struct blacklist SSizeMove;
} BlkLst = { {NULL, 0, NULL}, {NULL, 0, NULL}, {NULL, 0, NULL}
           , {NULL, 0, NULL}, {NULL, 0, NULL}, {NULL, 0, NULL}
           , {NULL, 0, NULL}, {NULL, 0, NULL} };

// Cursor data
HWND g_mainhwnd = NULL;

// Hook data
HINSTANCE hinstDLL = NULL;
HHOOK mousehook = NULL;

/////////////////////////////////////////////////////////////////////////////
// wether a window is present or not in a blacklist
static pure int blacklisted(HWND hwnd, struct blacklist *list)
{
    wchar_t title[256]=L"", classname[256]=L"";
    DorQWORD mode ;
    int i;

    // Null hwnd or empty list
    if (!hwnd || !list->length)
        return 0;
    // If the first element is *|* then we are in whitelist mode
    // mode = 1 => blacklist mode = 0 => whitelist;
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
static pure int blacklistedP(HWND hwnd, struct blacklist *list)
{
    wchar_t title[MAX_PATH]=L"";
    DorQWORD mode ;
    int i ;

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

static int IsResizable(HWND hwnd)
{
    return (conf.ResizeAll || GetWindowLongPtr(hwnd, GWL_STYLE)&WS_THICKFRAME);
}
/////////////////////////////////////////////////////////////////////////////
static void SendSizeMove_on(int on)
{
    // Don't send WM_ENTER/EXIT SIZEMOVE if the window is in SSizeMove BL
    if(!blacklisted(state.hwnd, &BlkLst.SSizeMove)) {
        PostMessage(state.hwnd, on? WM_ENTERSIZEMOVE: WM_EXITSIZEMOVE, 0, 0);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Use NULL to restore old transparency.
static void SetWindowTrans(HWND hwnd)
{
    static BYTE oldtrans;
    static HWND oldhwnd;
    if (conf.MoveTrans == 0 || conf.MoveTrans == 255 || !conf.FullWin) return;

    if(hwnd) {
        oldhwnd = hwnd;
        LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        if (exstyle&WS_EX_LAYERED) {
            GetLayeredWindowAttributes(hwnd, NULL, &oldtrans, NULL);
        } else {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle|WS_EX_LAYERED);
            oldtrans = 255;
        }
        SetLayeredWindowAttributes(hwnd, 0, conf.MoveTrans, LWA_ALPHA);
    } else if (oldhwnd) { // restore old trans;
        LONG_PTR exstyle = GetWindowLongPtr(oldhwnd, GWL_EXSTYLE);
        if (oldtrans == 255) {
            SetWindowLongPtr(oldhwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
        } else {
            SetLayeredWindowAttributes(oldhwnd, 0, oldtrans, LWA_ALPHA);
        }
        oldhwnd = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Enumerate callback proc
int monitors_alloc = 0;
static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    // Make sure we have enough space allocated
    if (nummonitors >= monitors_alloc) {
        monitors_alloc++;
        monitors = realloc(monitors, monitors_alloc*sizeof(RECT));
    }
    // Add monitor
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &mi);
    CopyRect(&monitors[nummonitors++], &mi.rcWork); //*lprcMonitor;

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
static void OffsetRectMDI(RECT *wnd, HWND mdihwnd)
{
    if (mdihwnd) {
        POINT mdiclientpt = { 0, 0 };
        if(ClientToScreen(mdihwnd, &mdiclientpt))
            OffsetRect(wnd, -mdiclientpt.x, -mdiclientpt.y);
    }
}
static int ShouldSnapTo(HWND window)
{
    LONG_PTR style;
    return window != state.hwnd && window != progman
        && IsWindowVisible(window) && !IsIconic(window)
         &&( ((style=GetWindowLongPtr(window,GWL_STYLE))&WS_CAPTION) == WS_CAPTION
           || (style&WS_THICKFRAME) == WS_THICKFRAME
           || blacklisted(window,&BlkLst.Snaplist));
}
/////////////////////////////////////////////////////////////////////////////
int wnds_alloc = 0;
static BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    if (numwnds >= wnds_alloc) {
        wnds_alloc += 8;
        wnds = realloc(wnds, wnds_alloc*sizeof(RECT));
    }

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
        // Return if this window is overlapped by another window
        int i;
        for (i=0; i < numwnds; i++) {
            if (RectInRect(&wnds[i], &wnd)) {
                return TRUE;
            }
        }
        // Add window to wnds db
        OffsetRectMDI(&wnd, state.mdiclient);
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
int numsnwnds = 0;
int snwnds_alloc = 0;
static pure struct wnddata *GetWindowInDB(HWND hwndd);
static BOOL CALLBACK EnumSnappedWindows(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    if (numsnwnds >= snwnds_alloc) {
        snwnds_alloc += 4;
        snwnds = realloc(snwnds, snwnds_alloc*sizeof(struct snwdata));
    }

    RECT wnd;
    if (ShouldSnapTo(hwnd) && !IsZoomed(hwnd) && GetWindowRectL(hwnd, &wnd)) {
        struct wnddata *entry;

        if (lParam) { // in case of Resize...
            // Only considers windows that are
            // touching the currently resized window
            RECT statewnd;
            GetWindowRectL(state.hwnd, &statewnd);
            if(!AreRectsTouchingT(&statewnd, &wnd, conf.SnapThreshold/2)) {
                return TRUE;
            }
        }

        if ((entry = GetWindowInDB(hwnd)) && entry->restore&SNAPPED && entry->restore&SNAPPEDSIDE) {
            snwnds[numsnwnds].flag = entry->restore;
        } else if (IsWindowSnapped(hwnd)) {
            HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(monitor, &mi);
            snwnds[numsnwnds].flag = WhichSideRectInRect(&mi.rcWork, &wnd);
        } else {
            return TRUE; // next hwnd
        }
        // Add the window to the list
        OffsetRectMDI(&wnd, state.mdiclient);
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
    if (conf.SmartAero) {
        if(state.mdiclient)
            EnumChildWindows(state.mdiclient, EnumSnappedWindows, 0);
        else
            EnumWindows(EnumSnappedWindows, 0);
    }
}
/////////////////////////////////////////////////////////////////////////////
static BOOL CALLBACK EnumTouchingWindows(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    if (numsnwnds >= snwnds_alloc) {
        snwnds_alloc += 4;
        snwnds = realloc(snwnds, snwnds_alloc*sizeof(struct snwdata));
    }

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
        unsigned flag;
        if((flag = AreRectsTouchingT(&statewnd, &wnd, conf.SnapThreshold/2))) {
            OffsetRectMDI(&wnd, state.mdiclient);

            // Return if this window is overlapped by another window
            int i;
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
static void EnumOnce(RECT **bd);
static void ResizeTouchingWindows(int posx, int posy, int width, int height)
{
    RECT *bd;
    EnumOnce(&bd);
    posx += bd->left; posy += bd->top;
    width -= bd->left+bd->right;
    height -= bd->top+bd->bottom;
    int i;
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
        RECT nbd;
        FixDWMRect(hwnd, &nbd);

        if (conf.FullWin) {
            MoveWindowAsync(hwnd, nwnd->left-nbd.left, nwnd->top-nbd.top
                      , nwnd->right - nwnd->left + nbd.left + nbd.right
                      , nwnd->bottom - nwnd->top + nbd.top + nbd.bottom, TRUE);
        }
        snwnds[i].flag = flag|TORESIZE;
    }
}
/////////////////////////////////////////////////////////////////////////////
static void ResizeAllSnappedWindowsAsync()
{
    int i;
    for (i=0; i < numsnwnds; i++) {
        if(snwnds[i].flag&TORESIZE) {
            RECT bd;
            FixDWMRect(snwnds[i].hwnd, &bd);
            InflateRectBorder(&snwnds[i].wnd, &bd);
            MoveWindowAsync(snwnds[i].hwnd
                , snwnds[i].wnd.left, snwnds[i].wnd.top
                , snwnds[i].wnd.right-snwnds[i].wnd.left
                , snwnds[i].wnd.bottom-snwnds[i].wnd.top, TRUE);
        }
    }
}
///////////////////////////////////////////////////////////////////////////
// Just used in Enum
static void EnumMdi()
{
    // Make sure we have enough space allocated
    if (nummonitors >= monitors_alloc) {
        monitors_alloc++;
        monitors = realloc(monitors, monitors_alloc*sizeof(RECT));
    }
    // Add MDIClient as the monitor
    RECT wnd;
    if (GetClientRect(state.mdiclient, &wnd) != 0) {
        CopyRect(&monitors[nummonitors++], &wnd);
    }
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
    if (state.mdiclient) {
        EnumMdi();
        return;
    }

    // Update handle to progman
    if (!IsWindow(progman)) {
        progman = FindWindow(L"Progman", L"Program Manager");
    }

    // Enumerate monitors
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, 0);

    // Enumerate windows
    if (state.snap >= 2) {
        EnumWindows(EnumWindowsProc, 0);
    }

    if (conf.StickyResize) {
        EnumWindows(EnumTouchingWindows, 0);
    }
}
///////////////////////////////////////////////////////////////////////////
// Pass NULL to reset Enum state and recalculate it
// at the next non null ptr.
static void EnumOnce(RECT **bd)
{
    static int enumed;
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
static void MoveSnap(int *posx, int *posy, int wndwidth, int wndheight)
{
    RECT *borders;
    EnumOnce(&borders);
    if (!state.snap || state.Speed > (int)conf.AeroMaxSpeed) return;

    // thresholdx and thresholdy will shrink to make sure
    // the dragged window will snap to the closest windows
    int i, j, thresholdx, thresholdy, stuckx=0, stucky=0, stickx=0, sticky=0;
    thresholdx = thresholdy = conf.SnapThreshold;

    // Loop monitors and windows
    for (i=0, j=0; i < nummonitors || j < numwnds; ) {
        RECT snapwnd;
        int snapinside;

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
        if (IsInRangeT(*posy, snapwnd.top, snapwnd.bottom, thresholdx)
        ||  IsInRangeT(snapwnd.top, *posy, *posy+wndheight, thresholdx)) {
            int snapinside_cond = (snapinside || *posy + wndheight - thresholdx < snapwnd.top
                                  || snapwnd.bottom < *posy + thresholdx);
            if (IsEqualT(snapwnd.right, *posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's right edge
                stuckx = 1;
                stickx = snapwnd.right - borders->left;
                thresholdx = snapwnd.right-*posx;
            } else if (snapinside_cond && IsEqualT(snapwnd.right, *posx+wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's right edge
                stuckx = 1;
                stickx = snapwnd.right + borders->right - wndwidth;
                thresholdx = snapwnd.right-(*posx+wndwidth);
            } else if (snapinside_cond && IsEqualT(snapwnd.left, *posx, thresholdx)) {
                // The left edge of the dragged window will snap to this window's left edge
                stuckx = 1;
                stickx = snapwnd.left - borders->left;
                thresholdx = snapwnd.left-*posx;
            } else if (IsEqualT(snapwnd.left, *posx+wndwidth, thresholdx)) {
                // The right edge of the dragged window will snap to this window's left edge
                stuckx = 1;
                stickx = snapwnd.left + borders->right -wndwidth;
                thresholdx = snapwnd.left-(*posx+wndwidth);
            }
        }// end if posx snaps

        // Check if posy snaps
        if (IsInRangeT(*posx, snapwnd.left, snapwnd.right, thresholdy)
         || IsInRangeT(snapwnd.left, *posx, *posx+wndwidth, thresholdy)) {
            int snapinside_cond = (snapinside || *posx + wndwidth - thresholdy < snapwnd.left
                                  || snapwnd.right < *posx+thresholdy);
            if (IsEqualT(snapwnd.bottom, *posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's bottom edge
                stucky = 1;
                sticky = snapwnd.bottom - borders->top;
                thresholdy = snapwnd.bottom-*posy;
            } else if (snapinside_cond && IsEqualT(snapwnd.bottom, *posy+wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's bottom edge
                stucky = 1;
                sticky = snapwnd.bottom + borders->bottom - wndheight;
                thresholdy = snapwnd.bottom-(*posy+wndheight);
            } else if (snapinside_cond && IsEqualT(snapwnd.top, *posy, thresholdy)) {
                // The top edge of the dragged window will snap to this window's top edge
                stucky = 1;
                sticky = snapwnd.top - borders->top;
                thresholdy = snapwnd.top-*posy;
            } else if (IsEqualT(snapwnd.top, *posy+wndheight, thresholdy)) {
                // The bottom edge of the dragged window will snap to this window's top edge
                stucky = 1;
                sticky = snapwnd.top-wndheight + borders->bottom;
                thresholdy = snapwnd.top-(*posy+wndheight);
            }
        } // end if posy snaps
    } // end for

    // Update posx and posy
    if (stuckx) {
        *posx = stickx;
    }
    if (stucky) {
        *posy = sticky;
    }
}

///////////////////////////////////////////////////////////////////////////
static void ResizeSnap(int *posx, int *posy, int *wndwidth, int *wndheight)
{
    RECT *borders;
    EnumOnce(&borders);
    if(!state.snap || state.Speed > (int)conf.AeroMaxSpeed) return;

    // thresholdx and thresholdy will shrink to make sure
    // the dragged window will snap to the closest windows
    int i, j, thresholdx, thresholdy;
    int stuckleft=0, stucktop=0, stuckright=0, stuckbottom=0
      , stickleft=0, sticktop=0, stickright=0, stickbottom=0;
    thresholdx = thresholdy = conf.SnapThreshold;
    // Loop monitors and windows
    for (i=0, j=0; i < nummonitors || j < numwnds;) {
        RECT snapwnd;
        int snapinside;

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

            int snapinside_cond = (snapinside || *posy+*wndheight-thresholdx < snapwnd.top
                                  || snapwnd.bottom < *posy+thresholdx);
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

            int snapinside_cond = (snapinside || *posx+*wndwidth-thresholdy < snapwnd.left
                                || snapwnd.right < *posx+thresholdy);
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
#define SW_TOGGLE_MAX_RESTORE 27
#define SW_FULLSCREEN 28
static void Maximize_Restore_atpt(HWND hwnd, const POINT *pt, UINT sw_cmd, HMONITOR monitor)
{
    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(hwnd, &wndpl);
    RECT fmon;
    if(sw_cmd == SW_TOGGLE_MAX_RESTORE)
        wndpl.showCmd = (wndpl.showCmd==SW_MAXIMIZE)? SW_RESTORE: SW_MAXIMIZE;
    else if (sw_cmd == SW_FULLSCREEN)
        ;// nothing;
    else
        wndpl.showCmd = sw_cmd;

    if(wndpl.showCmd == SW_MAXIMIZE || sw_cmd == SW_FULLSCREEN) {
        HMONITOR wndmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (!monitor) {
            POINT ptt;
            if (pt) ptt = *pt;
            else GetCursorPos(&ptt);
            monitor = MonitorFromPoint(ptt, MONITOR_DEFAULTTONEAREST);
        }
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
        MoveWindowAsync(hwnd, fmon.left, fmon.top, fmon.right-fmon.left, fmon.bottom-fmon.top, TRUE);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Move the windows in a thread in case it is very slow to resize
static DWORD WINAPI MoveWindowThread(LPVOID LastWinV)
{
    int ret;
    HWND hwnd;
    struct windowRR *lw = LastWinV;
    hwnd = lw->hwnd;

    if (lw->end && conf.FullWin) Sleep(conf.RefreshRate+5);

    ret = SetWindowPos(hwnd, NULL, lw->x, lw->y, lw->width, lw->height
                     , SWP_NOACTIVATE|SWP_NOREPOSITION); // |WP_NOSENDCHANGING|SWP_DEFERERASE
    if (lw->end) {
        RedrawWindow(hwnd, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN);
        lw->hwnd = NULL;
        return !ret;
    }

    if (conf.RefreshRate) Sleep(conf.RefreshRate);

    lw->hwnd = NULL;

    return !ret;
}
static void MoveWindowInThread(struct windowRR *lw)
{
    DWORD lpThreadId;
    CloseHandle(CreateThread(NULL, STACK, MoveWindowThread, lw, 0, &lpThreadId));
}
///////////////////////////////////////////////////////////////////////////
// use snwnds[numsnwnds].wnd / .flag
static void GetAeroSnappingMetrics(int *leftWidth, int *rightWidth, int *topHeight, int *bottomHeight, const RECT *mon)
{
    *leftWidth    = CLAMPW((mon->right - mon->left)* conf.AHoff     /100);
    *rightWidth   = CLAMPW((mon->right - mon->left)*(100-conf.AHoff)/100);
    *topHeight    = CLAMPH((mon->bottom - mon->top)* conf.AVoff     /100);
    *bottomHeight = CLAMPH((mon->bottom - mon->top)*(100-conf.AVoff)/100);

    // Check on all the other snapped windows from the bottom most
    // To give precedence to the topmost windows
    int i;
    for (i=numsnwnds-1; i >= 0; i--) {
        int flag = snwnds[i].flag;
        RECT *wnd = &snwnds[i].wnd;
        // if the window is in current monitor
        if (PtInRect(mon, (POINT) { wnd->left+16, wnd->top+16 })) {
            // We have a snapped window in the monitor
            if (flag & SNLEFT) {
                *rightWidth = CLAMPW(mon->right - wnd->right);
            } else if (flag & SNRIGHT) {
                *leftWidth = CLAMPW(wnd->left - mon->left);
            }
            if (flag & SNTOP) {
                *bottomHeight = CLAMPH(mon->bottom - wnd->bottom);
            } else if (flag & SNBOTTOM) {
                *topHeight = CLAMPH(wnd->top - mon->top);
            }
        }
    } // next i
}
///////////////////////////////////////////////////////////////////////////
static void GetMonitorRect(const POINT *pt, int full, RECT *_mon)
{
    if (state.mdiclient) {
        RECT mdirect;
        if (GetClientRect(state.mdiclient, &mdirect)) {
            CopyRect(_mon, &mdirect);
            return;
        }
    }

    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(MonitorFromPoint(*pt, MONITOR_DEFAULTTONEAREST), &mi);

    CopyRect(_mon, full? &mi.rcMonitor : &mi.rcWork);
}
///////////////////////////////////////////////////////////////////////////
#define AERO_TH conf.AeroThreshold
#define MM_THREAD_ON (LastWin.hwnd && conf.FullWin)
static int AeroMoveSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight, const RECT *_mon)
{
    // return if last resizing is not finished or no Aero or not resizable.
    if(!conf.Aero || MM_THREAD_ON) return 0;

    static int resizable = 1;
    if (!resizable) return 0;
    if (!state.moving) {
        if (!(resizable=IsResizable(state.hwnd)))
            return 0;
        EnumSnapped();
    }

    // We HAVE to check the monitor for each pt...
    RECT mon;
    if(_mon) {
        CopyRect(&mon, _mon);
    } else {
        GetMonitorRect(&pt, 0, &mon);
    }

    int Left  = mon.left   + 2*AERO_TH ;
    int Right = mon.right  - 2*AERO_TH ;
    int Top   = mon.top    + 2*AERO_TH ;
    int Bottom= mon.bottom - 2*AERO_TH ;
    int leftWidth, rightWidth, topHeight, bottomHeight;

    if(PtInRect(&(RECT){ Left, Right, Top, Bottom }, pt)) return 0;

    GetAeroSnappingMetrics(&leftWidth, &rightWidth, &topHeight, &bottomHeight, &mon);

    // Move window
    if (pt.y < Top && pt.x < Left) {
        // Top left
        state.wndentry->restore = SNAPPED|SNTOPLEFT;
        *wndwidth =  leftWidth;
        *wndheight = topHeight;
        *posx = mon.left;
        *posy = mon.top;
    } else if (pt.y < Top && Right < pt.x) {
        // Top right
        state.wndentry->restore = SNAPPED|SNTOPRIGHT;
        *wndwidth = rightWidth;
        *wndheight = topHeight;
        *posx = mon.right-*wndwidth;
        *posy = mon.top;
    } else if (Bottom < pt.y && pt.x < Left) {
        // Bottom left
        state.wndentry->restore = SNAPPED|SNBOTTOMLEFT;
        *wndwidth = leftWidth;
        *wndheight = bottomHeight;
        *posx = mon.left;
        *posy = mon.bottom - *wndheight;
    } else if (Bottom < pt.y && Right < pt.x) {
        // Bottom right
        state.wndentry->restore = SNAPPED|SNBOTTOMRIGHT;
        *wndwidth = rightWidth;
        *wndheight= bottomHeight;
        *posx = mon.right - *wndwidth;
        *posy = mon.bottom - *wndheight;
    } else if (pt.y < mon.top + AERO_TH) {
        // Pure Top
        if (!state.shift ^ !(conf.AeroTopMaximizes&1)
         &&(state.Speed < (int)conf.AeroMaxSpeed)) {
            Maximize_Restore_atpt(state.hwnd, &pt, SW_MAXIMIZE, NULL);
            LastWin.hwnd = NULL;
            state.moving = 2;
            return 1;
        } else {
            state.wndentry->restore = SNAPPED|SNTOP;
            *wndwidth = CLAMPW(mon.right - mon.left);
            *wndheight = topHeight;
            *posx = mon.left + (mon.right-mon.left)/2 - *wndwidth/2; // Center
            *posy = mon.top;
        }
    } else if (pt.y > mon.bottom - AERO_TH) {
        // Pure Bottom
        state.wndentry->restore = SNAPPED|SNBOTTOM;
        *wndwidth  = CLAMPW( mon.right-mon.left);
        *wndheight = bottomHeight;
        *posx = mon.left + (mon.right-mon.left)/2 - *wndwidth/2; // Center
        *posy = mon.bottom - *wndheight;
    } else if (pt.x < mon.left+AERO_TH) {
        // Pure Left
        state.wndentry->restore = SNAPPED|SNLEFT;
        *wndwidth = leftWidth;
        *wndheight = CLAMPH( mon.bottom-mon.top );
        *posx = mon.left;
        *posy = mon.top + (mon.bottom-mon.top)/2 - *wndheight/2; // Center
    } else if (pt.x > mon.right - AERO_TH) {
        // Pure Right
        state.wndentry->restore = SNAPPED|SNRIGHT;
        *wndwidth =  rightWidth;
        *wndheight = CLAMPH( mon.bottom-mon.top );
        *posx = mon.right - *wndwidth;
        *posy = mon.top + (mon.bottom-mon.top)/2 - *wndheight/2; // Center
    } else if (state.wndentry->restore&SNAPPED) {
        // Restore original window size
        state.wndentry->restore = 0;
        *wndwidth = state.origin.width;
        *wndheight = state.origin.height;
    }

    // Aero-move the window?
    if (state.wndentry->restore&SNAPPED) {
        *wndwidth  = CLAMPW(*wndwidth);
        *wndheight = CLAMPH(*wndheight);

        state.wndentry->width = state.origin.width;
        state.wndentry->height = state.origin.height;

        RECT borders;
        FixDWMRect(state.hwnd, &borders);
        *posx -= borders.left;
        *posy -= borders.top;
        *wndwidth += borders.left+borders.right;
        *wndheight+= borders.top+borders.bottom;

        // If we go too fast then donot move the window
        if(state.Speed > (int)conf.AeroMaxSpeed) return 1;
        if(conf.FullWin) {
            MoveWindowAsync(state.hwnd, *posx, *posy, *wndwidth, *wndheight, TRUE);
            return 1;
        }
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
static void AeroResizeSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight)
{
    // return if last resizing is not finished
    if(!conf.Aero || MM_THREAD_ON || state.Speed > (int)conf.AeroMaxSpeed)
        return;

    static RECT borders;
    RECT mon = state.origin.mon;
    if(!state.moving) {
        FixDWMRect(state.hwnd, &borders);
    }
    if (state.resize.x == RZ_CENTER && state.resize.y == RZ_TOP && pt.y < mon.top + AERO_TH) {
        state.wndentry->restore = SNAPPED|SNMAXH;
        *wndheight = CLAMPH(mon.bottom - mon.top + borders.bottom + borders.top);
        *posy = mon.top - borders.top;
    } else if (state.resize.x == RZ_LEFT && state.resize.y == RZ_CENTER && pt.x < mon.left + AERO_TH) {
        state.wndentry->restore = SNAPPED|SNMAXW;
        *wndwidth = CLAMPW(mon.right - mon.left + borders.left + borders.right);
        *posx = mon.left - borders.left;
    }
    // Aero-move the window?
    if (state.wndentry->restore&SNAPPED && state.wndentry->restore&(SNMAXH|SNMAXW)) {
        state.wndentry->width = state.origin.width;
        state.wndentry->height = state.origin.height;
    }
}
///////////////////////////////////////////////////////////////////////////
// Get action of button
static pure enum action GetAction(enum button button)
{
    if (button) // Ugly pointer arithmetic (LMB <==> button == 2)
        return *(&conf.Mouse.LMB+(button-2));
    else
        return AC_NONE;
}

///////////////////////////////////////////////////////////////////////////
// Check if key is assigned in the HKlist
static int pure IsHotkeyy(unsigned char key, struct hotkeys_s *HKlist)
{
    int i;
    for (i=0; i < HKlist->length; i++) {
        if (key == HKlist->keys[i]) {
            return 1;
        }
    }
    return 0;
}
#define IsHotkey(a)   IsHotkeyy(a, &conf.Hotkeys)
#define IsHotclick(a) IsHotkeyy(a, &conf.Hotclick)
#define IsKillkey(a)  IsHotkeyy(a, &conf.Killkey)
/////////////////////////////////////////////////////////////////////////////
static int IsHotKeyDown(struct hotkeys_s *hk)
{
    int i;
    for (i=0; i < hk->length; i++) {
        if (GetAsyncKeyState(hk->keys[i])&0x8000)
            return 1;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// if pt is NULL then the window is not moved when restored.
// index 1 => normal restore on any move state.wndentry->restore & 1
// index 2 => Rolled window state.wndentry->restore & 2
// state.wndentry->restore & 3 => Both 1 & 2 ie: Maximized then rolled.
// Set was_snapped to 2 if you wan to
static void RestoreOldWin(const POINT *pt, int was_snapped, int index)
{
    // Restore old width/height?
    int restore = 0;

    if (state.wndentry->restore & index && !(state.origin.maximized&&state.wndentry->restore&2)) {
        // Set origin width and height to the saved values
        restore = state.wndentry->restore;
        state.origin.width = state.wndentry->width;
        state.origin.height = state.wndentry->height;
        state.wndentry->restore = 0; // and clear restore flag
    }

    POINT mdiclientpt = { 0, 0 };
    RECT wnd;
    GetWindowRect(state.hwnd, &wnd);
    ClientToScreen(state.mdiclient, &mdiclientpt);

    // Set offset
    if (pt) {
        state.offset.x = state.origin.width  * min(pt->x-wnd.left, wnd.right-wnd.left)
                        / max(wnd.right-wnd.left,1);
        state.offset.y = state.origin.height * min(pt->y-wnd.top, wnd.bottom-wnd.top)
                       / max(wnd.bottom-wnd.top,1);
    }
    if (state.origin.maximized || was_snapped == 1) {
        if (state.wndentry->restore&ROLLED || restore&ROLLED) {
            // if we restore a  Rolled Maximized window...
            state.offset.y = GetSystemMetrics(SM_CYMIN)/2;
        }
    } else if (restore) {
        if (was_snapped == 2 && pt) {
            // Restoring via normal drag we want
            // the offset along Y to be unchanged...
            state.offset.y = pt->y-wnd.top;
        }
        SetWindowPos(state.hwnd, NULL
                , pt? pt->x - state.offset.x - mdiclientpt.x: 0
                , pt? pt->y - state.offset.y - mdiclientpt.y: 0
                , state.origin.width, state.origin.height
                , pt? SWP_NOZORDER: SWP_NOZORDER|SWP_NOMOVE);
    } else if (pt) {
        state.offset.x = pt->x - wnd.left;
        state.offset.y = pt->y - wnd.top;
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
static int ShouldResizeTouching()
{
    return state.action == AC_RESIZE
        && ( (conf.StickyResize&1 && state.shift)
          || (conf.StickyResize==2 && !state.shift)
        );
}
static void DrawRect(HDC hdcl, const RECT *rc)
{
    Rectangle(hdcl, rc->left, rc->top, rc->right, rc->bottom);
}
///////////////////////////////////////////////////////////////////////////
static void MouseMove(POINT pt)
{
    int posx=0, posy=0, wndwidth=100, wndheight=100;

    // Check if window still exists
    if (!state.hwnd || !IsWindow(state.hwnd))
        { LastWin.hwnd = NULL; UnhookMouse(); return; }

    if(conf.UseCursor)
        SetWindowPos(g_mainhwnd, NULL, pt.x-128, pt.y-128, 256, 256
                    , SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE);

    if(state.moving == CURSOR_ONLY) return; // Movement blocked...

    // Restore Aero snapped window when movement starts
    int was_snapped=0;
    if(state.action == AC_MOVE && !state.moving) {
        was_snapped = IsWindowSnapped(state.hwnd);
        RestoreOldWin(&pt, was_snapped, 1);
    }

    static RECT wnd;
    if (!state.moving && !GetWindowRect(state.hwnd, &wnd)) return;

    static RECT mdimon;
    static POINT mdiclientpt;
    RECT *_curentmon=NULL;
    if (state.mdiclient) { // MDI
        if(!state.moving) {
            mdiclientpt.x = mdiclientpt.y = 0;
            if (!GetClientRect(state.mdiclient, &mdimon)
             || !ClientToScreen(state.mdiclient, &mdiclientpt)) {
                return;
            }
        }
        _curentmon = &mdimon;
        // Convert pt in MDI coordinates.
        pt.x -= mdiclientpt.x;
        pt.y -= mdiclientpt.y;
    } else {
        mdiclientpt = (POINT) { 0, 0 };
    }

    // Restrict pt within origin monitor if Ctrl|Shift is being pressed
    if ((state.ctrl && !state.ignorectrl) || state.shift) {
        static HMONITOR origMonitor;
        static RECT fmon;
        if(origMonitor != state.origin.monitor || !state.origin.monitor) {
            origMonitor = state.origin.monitor;
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(state.origin.monitor, &mi);
            CopyRect(&fmon, &mi.rcMonitor);
            fmon.left++; fmon.top++;
            fmon.right--; fmon.bottom--;
        }
        if (state.ctrl || state.shift) {
            RECT clip;
            if (state.mdiclient) {
                CopyRect(&clip, &mdimon);
                OffsetRect(&clip, mdiclientpt.x, mdiclientpt.y);
            } else {
                CopyRect(&clip, &fmon);
            }
            ClipCursorOnce(&clip);
        }
        pt.x = CLAMP(fmon.left, pt.x, fmon.right);
        pt.y = CLAMP(fmon.top, pt.y, fmon.bottom);
    }

    // Get new position for window
    if (state.action == AC_MOVE) {
        posx = pt.x-state.offset.x;
        posy = pt.y-state.offset.y;
        wndwidth = wnd.right-wnd.left;
        wndheight = wnd.bottom-wnd.top;

        // Check if the window will snap anywhere
        MoveSnap(&posx, &posy, wndwidth, wndheight);
        int ret = AeroMoveSnap(pt, &posx, &posy, &wndwidth, &wndheight, _curentmon);
        if ( ret == 1) { state.moving = 1; return; }

        // Restore window if maximized when starting
        if (was_snapped || IsZoomed(state.hwnd)) {
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
            GetWindowPlacement(state.hwnd, &wndpl);
            // Restore original width and height in case we are restoring
            // A Snapped + Maximized window.
            wndpl.showCmd = SW_RESTORE;
            if (!(state.wndentry->restore&ROLLED)) { // Not if window is rolled!
                wndpl.rcNormalPosition.right = wndpl.rcNormalPosition.left + state.origin.width;
                wndpl.rcNormalPosition.bottom= wndpl.rcNormalPosition.top +  state.origin.height;
            }
            if(state.wndentry->restore&SNTHENROLLED) state.wndentry->restore=0;
            SetWindowPlacement(state.hwnd, &wndpl);
            // Update wndwidth and wndheight
            wndwidth  = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
            wndheight = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
        }
    } else if (state.action == AC_RESIZE) {
        // Restore the window (to monitor size) if it's maximized
        if (!state.moving && IsZoomed(state.hwnd)) {
            state.wndentry->restore = 0; //Clear restore flag.
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
            GetWindowPlacement(state.hwnd, &wndpl);

            // Set size to monitor to prevent flickering
            CopyRect(&wnd, state.mdiclient? &mdimon: &state.origin.mon);
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
                // Get new values from MDIClient, since restoring the child have changed them,
                // The amount they change with differ depending on implementation (compare mIRC and Spy++)
                Sleep(1); // Sometimes needed
                mdiclientpt = (POINT) { 0, 0 };
                if (!GetClientRect(state.mdiclient, &wnd)
                 || !ClientToScreen(state.mdiclient, &mdiclientpt) ) {
                     return;
                }
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
        if(!conf.SmartAero) {
            // Always clear when AeroSmart is disabled.
            state.wndentry->restore = 0;
        } else {
            // Do not ckear is the window was snapped to some side
            // or maximized Vertically/horizontally or rolled.
            unsigned smart_restore_flag=(SNAPPEDSIDE|ROLLED|SNMAXW|SNMAXH);
            if(!(state.wndentry->restore & smart_restore_flag))
                state.wndentry->restore = 0;
        }

        // Figure out new placement
        if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
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
    int mouse_thread_finished = !LastWin.hwnd;
    LastWin.hwnd   = state.hwnd;
    LastWin.end    = 0;
    LastWin.x      = posx;
    LastWin.y      = posy;
    LastWin.width  = wndwidth;
    LastWin.height = wndheight;

    wnd.left   = posx + mdiclientpt.x;
    wnd.top    = posy + mdiclientpt.y;
    wnd.right  = posx + mdiclientpt.x + wndwidth;
    wnd.bottom = posy + mdiclientpt.y + wndheight;

    if (!conf.FullWin) {
        RECT newRect;
        CopyRect(&newRect, &wnd);
        InflateRect(&newRect, -1, -1);

        if (!hdcc) {
            if (!hpenDot_Global)
                hpenDot_Global = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            hdcc = CreateDCA("DISPLAY", NULL, NULL, NULL);
            SetROP2(hdcc, R2_NOTXORPEN);
            SelectObject(hdcc, hpenDot_Global);
        }
        DrawRect(hdcc, &newRect);
        if (state.moving == 1)
            DrawRect(hdcc, &oldRect);

        if (ShouldResizeTouching()) {
            ResizeTouchingWindows(posx, posy, wndwidth, wndheight);
        }

        CopyRect(&oldRect, &newRect); // oldRect is GLOBAL!
        state.moving = 1;

    } else if (mouse_thread_finished) {
        // Resize other windows
        if (ShouldResizeTouching()) {
            ResizeTouchingWindows(posx, posy, wndwidth, wndheight);
        }
        MoveWindowInThread(&LastWin);
        state.moving = 1;
    } else {
        Sleep(0);
        state.moving = NOT_MOVED; // Could not Move Window
    }
}
/////////////////////////////////////////////////////////////////////////////
static void Send_KEY(unsigned char vkey)
{
    state.ignorectrl = 1;
    KEYBDINPUT ctrl[2] = { {vkey, 0, 0, 0, 0}, {vkey, 0 , KEYEVENTF_KEYUP, 0, 0} };
    ctrl[0].dwExtraInfo = ctrl[1].dwExtraInfo = GetMessageExtraInfo();
    INPUT input[2] = { {INPUT_KEYBOARD,{.ki = ctrl[0]}}, {INPUT_KEYBOARD,{.ki = ctrl[1]}} };
    SendInput(2, input, sizeof(INPUT));
    state.ignorectrl = 0;
}
#define Send_CTRL() Send_KEY(VK_CONTROL)
///////////////////////////////////////////////////////////////////////////
static void RestrictToCurentMonitor()
{
    if (state.action || state.alt) {
        POINT pt;
        GetCursorPos(&pt);
        state.origin.maximized = 0;
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
    if(state.action && conf.GrabWithAlt) {
        FinishMovement();
    }

    // Unhook mouse if no actions is ongoing.
    if (!state.action) {
        UnhookMouse();
    }
}
static int ActionPause(HWND hwnd, char pause)
{
    if (!blacklistedP(hwnd, &BlkLst.Pause)) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (ProcessHandle) {
            if (pause) NtSuspendProcess(ProcessHandle);
            else       NtResumeProcess(ProcessHandle);
        }
        CloseHandle(ProcessHandle);
        return 1;
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
// Kill the process from hwnd
static DWORD WINAPI ActionKillThread(LPVOID hwnd)
{
    if(!hwnd || blacklistedP(hwnd, &BlkLst.Pause))
       return 0;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    // Open the process
    HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (proc) {
        TerminateProcess(proc, 1);
        CloseHandle(proc);
    }
    return 1;
}
static int ActionKill(HWND hwnd)
{
    DWORD lpThreadId;
    HANDLE thread;
    thread = CreateThread(NULL, STACK, ActionKillThread, hwnd, 0, &lpThreadId);
    CloseHandle(thread);

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
///////////////////////////////////////////////////////////////////////////
// Keep this one minimalist, it is always on.
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || state.ignorectrl
    || (conf.ScrollLockState && !(GetKeyState(VK_SCROLL)&1))) return CallNextHookEx(NULL, nCode, wParam, lParam);

    unsigned char vkey = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        if (!state.alt && (!conf.KeyCombo || (state.alt1 && state.alt1 != vkey)) && IsHotkey(vkey)) {
            // Update state && (GetKeyState(VK_SCROLL)&1)
            state.alt = vkey;
            state.blockaltup = 0;
            state.blockmouseup = 0;

            // Hook mouse
            HookMouse();
            if(conf.GrabWithAlt) {
                POINT pt;
                GetCursorPos(&pt);
                if(!init_movement_and_actions(pt, conf.GrabWithAlt, vkey)) {
                    UnhookMouse();
                }
            }
        } else if (conf.KeyCombo && !state.alt1 && IsHotkey(vkey)) {
            state.alt1 = vkey;

        } else if (vkey == VK_LSHIFT || vkey == VK_RSHIFT) {
            if(!state.shift) {
                RestrictToCurentMonitor();
                EnumOnce(NULL); // Reset enum state.
                state.snap = 3;
            }
            state.shift = 1;

            // Block keydown to prevent Windows from changing keyboard layout
            if (state.alt && state.action) {
                return 1;
            }
        } else if (vkey == VK_SPACE && state.action && state.snap) {
            state.snap = 0;
            return 1; // Block to avoid sys menu.
        } else if (state.alt && state.action == conf.GrabWithAlt && IsKillkey(vkey)) {
           // Release Hook on Alt+Tab in case there is DisplayFusion which creates an
           // elevated Att+Tab windows that captures the AltUp key.
            HotkeyUp();
        } else if (vkey == VK_ESCAPE) { // USER PRESSED ESCAPE!
            // Alsays disable shift and ctrl, in case of Ctrl+Shift+ESC.
            state.ctrl = 0;
            state.shift = 0;
            LastWin.hwnd = NULL;
            // Stop current action
            if (state.action || state.alt) {
                int action = state.action;
                if (!conf.FullWin && state.moving) {
                    DrawRect(hdcc, &oldRect);
                }
                // Send WM_EXITSIZEMOVE
                SendSizeMove_on(0);

                state.alt = 0;
                state.alt1 = 0;

                UnhookMouse();

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
        } else if (!state.ctrl && state.alt!=vkey &&(vkey == VK_LCONTROL || vkey == VK_RCONTROL)) {
            RestrictToCurentMonitor();
            state.ctrl = 1;
            if(state.action){
                SetForegroundWindowL(state.hwnd);
            }
        } else if (state.sclickhwnd && state.alt && (vkey == VK_LMENU || vkey == VK_RMENU)) {
            return 1;
        }

    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
        if (IsHotkey(vkey)) {
            HotkeyUp();
        } else if (vkey == VK_LSHIFT || vkey == VK_RSHIFT) {
            ClipCursorOnce(NULL); // Release cursor trapping
            state.shift = 0;
            state.snap = conf.AutoSnap;
        } else if (vkey == VK_LCONTROL || vkey == VK_RCONTROL) {
            ClipCursorOnce(NULL); // Release cursor trapping
            state.ctrl = 0;
            // If there is no action then Control UP prevents AltDragging...
            if(!state.action) state.alt = 0;
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
    if (hwnd == NULL || (foreground != NULL && blacklisted(foreground,&BlkLst.Windows)))
        return 0;

    // Get class behind eventual tooltip
    wchar_t classname[20] = L"";
    hwnd=GetClass_HideIfTooltip(pt, hwnd, classname, ARR_SZ(classname));

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
    if (GetAsyncKeyState(VK_CONTROL) &0x8000) wp |= MK_CONTROL;
    if (GetAsyncKeyState(VK_LBUTTON) &0x8000) wp |= MK_LBUTTON;
    if (GetAsyncKeyState(VK_MBUTTON) &0x8000) wp |= MK_MBUTTON;
    if (GetAsyncKeyState(VK_RBUTTON) &0x8000) wp |= MK_RBUTTON;
    if (GetAsyncKeyState(VK_SHIFT)   &0x8000) wp |= MK_SHIFT;
    if (GetAsyncKeyState(VK_XBUTTON1)&0x8000) wp |= MK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2)&0x8000) wp |= MK_XBUTTON2;

    // Forward scroll message
    SendMessage(hwnd, wParam, wp, lp);

    // Block original scroll event
    return 1;
}
/////////////////////////////////////////////////////////////////////////////
int hwnds_alloc = 0;
static BOOL CALLBACK EnumAltTabWindows(HWND window, LPARAM lParam)
{
    // Make sure we have enough space allocated
    if (numhwnds >= hwnds_alloc) {
        hwnds_alloc += 8;
        hwnds = realloc(hwnds, hwnds_alloc*sizeof(HWND));
    }

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
    }// End if MDI

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
// Under Vista this will change the main volume with ole interface,
// Under NT4-XP, this will change the waveOut volume.
static int ActionVolume(int delta)
{
    static int HaveV=-1;
    static HINSTANCE hOLE32DLL=NULL, hMMdll=NULL;
    if (HaveV == -1) {
        hOLE32DLL = LoadLibraryA("OLE32.DLL");
        if(hOLE32DLL){
            myCoInitialize = (void *)GetProcAddress(hOLE32DLL, "CoInitialize");
            myCoUninitialize= (void *)GetProcAddress(hOLE32DLL, "CoUninitialize");
            myCoCreateInstance= (void *)GetProcAddress(hOLE32DLL, "CoCreateInstance");
            if(!myCoCreateInstance || !myCoUninitialize || !myCoInitialize){
                FreeLibrary(hOLE32DLL);
                HaveV = 0;
            } else {
                HaveV = 1;
            }
        } else {
            HaveV = 0;
        }
    }
    if (HaveV == 1) {
        HRESULT hr;
        IMMDeviceEnumerator *pDevEnumerator = NULL;
        IMMDevice *pDev = NULL;
        IAudioEndpointVolume *pAudioEndpoint = NULL;

        // Get audio endpoint
        myCoInitialize(NULL); // Needed for IAudioEndpointVolume
        hr = myCoCreateInstance(&my_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL
                              , &my_IID_IMMDeviceEnumerator, (void**)&pDevEnumerator);
        if (hr != S_OK){
            myCoUninitialize();
            FreeLibrary(hOLE32DLL);
            HaveV = 2;
            return 0;
        }

        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pDevEnumerator, eRender, eMultimedia, &pDev);
        IMMDeviceEnumerator_Release(pDevEnumerator);
        if (hr != S_OK) return 0;

        hr = IMMDevice_Activate(pDev, &my_IID_IAudioEndpointVolume, CLSCTX_ALL
                                    , NULL, (void**)&pAudioEndpoint);
        IMMDevice_Release(pDev);
        if (hr != S_OK) return 0;

        // Function pointer so we only need one for-loop
        typedef HRESULT WINAPI (*_VolumeStep)(IAudioEndpointVolume*, LPCGUID pguidEventContext);
        _VolumeStep VolumeStep = (_VolumeStep)(pAudioEndpoint->lpVtbl->VolumeStepDown);
        if (delta > 0)
            VolumeStep = (_VolumeStep)(pAudioEndpoint->lpVtbl->VolumeStepUp);

        // Hold shift to make 5 steps
        int i;
        int num = (state.shift)?5:1;

        for (i=0; i < num; i++) {
            hr = VolumeStep(pAudioEndpoint, NULL);
        }

        IAudioEndpointVolume_Release(pAudioEndpoint);
        myCoUninitialize();
    } else {
        if (HaveV == 2) {
            if (!hMMdll) hMMdll = LoadLibraryA("WINMM.DLL");
            if (!hMMdll) return 0;
            mywaveOutGetVolume = (void *)GetProcAddress(hMMdll, "waveOutGetVolume");
            mywaveOutSetVolume = (void *)GetProcAddress(hMMdll, "waveOutSetVolume");
            if(!mywaveOutSetVolume  || !mywaveOutGetVolume) {
                FreeLibrary(hMMdll); hMMdll=NULL;
                return 0;
            }
            HaveV = 3;
        }
        DWORD Volume;
        mywaveOutGetVolume(NULL, &Volume);

        DWORD tmp = Volume&0xFFFF0000 >> 16;
        int leftV = (int)tmp;
        tmp = (Volume&0x0000FFFF);
        int rightV = (int)tmp;
        rightV += (delta>0? 0x0800: -0x0800) * (state.shift? 4: 1);
        leftV  += (delta>0? 0x0800: -0x0800) * (state.shift? 4: 1);
        rightV = CLAMP(0x0000, rightV, 0xFFFF);
        leftV  = CLAMP(0x0000, leftV, 0xFFFF);
        Volume = ( ((DWORD)leftV) << 16 ) | ( (DWORD)rightV );
        mywaveOutSetVolume(NULL, Volume);
    }
    return 1;
}
/////////////////////////////////////////////////////////////////////////////
// Windows 2000+ Only
static int ActionTransparency(HWND hwnd, int delta)
{
    static int alpha=255;

    if (blacklisted(hwnd, &BlkLst.Windows)) return 0;

    int alpha_delta = (state.shift)? conf.AlphaDeltaShift: conf.AlphaDelta;
    alpha_delta *= sign(delta);

    LONG_PTR exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (alpha_delta < 0 && !(exstyle&WS_EX_LAYERED)) {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle|WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    }

    BYTE old_alpha;
    if (GetLayeredWindowAttributes(hwnd, NULL, &old_alpha, NULL)) {
         alpha = old_alpha; // If possible start from the current aplha.
    }

    alpha = CLAMP(conf.MinAlpha, alpha+alpha_delta, 255); // Limit alpha

    if (alpha >= 255)
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
    else
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

    return 1;
}
/////////////////////////////////////////////////////////////////////////////
static void ActionLower(POINT *ptt, HWND hwnd, int delta, UCHAR shift)
{
    if (delta > 0) {
        if (shift) {
            Maximize_Restore_atpt(hwnd, ptt, SW_TOGGLE_MAX_RESTORE, NULL);
        } else {
            if (conf.AutoFocus || state.ctrl) SetForegroundWindowL(hwnd);
            SetWindowLevel(hwnd, HWND_TOPMOST);
            SetWindowLevel(hwnd, HWND_NOTOPMOST);
        }
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
static void ActionMaxRestMin(POINT *ptt, HWND hwnd, int delta)
{
    int maximized = IsZoomed(hwnd);
    if (state.shift) {
        ActionLower(ptt, hwnd, delta, 0);
        return;
    }

    if (delta > 0) {
        if(!maximized && IsResizable(hwnd))
            Maximize_Restore_atpt(hwnd, ptt, SW_MAXIMIZE, NULL);
    } else {
        if(maximized)
            Maximize_Restore_atpt(hwnd, ptt, SW_RESTORE, NULL);
        else
            MinimizeWindow(hwnd);
    }
    if(conf.AutoFocus) SetForegroundWindowL(hwnd);
}

/////////////////////////////////////////////////////////////////////////////
static HCURSOR CursorToDraw()
{
    HCURSOR cursor;

    if(conf.UseCursor == 3) {
        return LoadCursor(NULL, IDC_ARROW);
    }
    if(state.action == AC_MOVE) {
        if(conf.UseCursor == 4)
            return LoadCursor(NULL, IDC_SIZEALL);
        cursor = LoadCursor(NULL, conf.UseCursor>1? IDC_ARROW: IDC_HAND);
        if(!cursor) cursor = LoadCursor(NULL, IDC_ARROW); // Fallback;
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
/////////////////////////////////////////////////////////////////////////////
// Return the entry if it was already in db.
static pure struct wnddata *GetWindowInDB(HWND hwndd)
{
    // Check if window is already in the wnddb database
    // And set it in the current state
    int i;
    for (i=0; i < NUMWNDDB; i++) {
        if (wnddb.items[i].hwnd == hwndd) {
            return &wnddb.items[i];
        }
    }
    return NULL;
}
static void AddWindowToDB(HWND hwndd)
{
    state.wndentry = GetWindowInDB(hwndd);

    // Find a nice place in wnddb if not already present
    int i;
    if (state.wndentry == NULL) {
        for (i=0; i < NUMWNDDB+1 && wnddb.pos->restore ; i++) {
            wnddb.pos = (wnddb.pos == &wnddb.items[NUMWNDDB-1])?&wnddb.items[0]:wnddb.pos+1;
        }
        state.wndentry = wnddb.pos;
        state.wndentry->hwnd = hwndd;
        state.wndentry->restore = 0;
    } else if (state.wndentry->restore&SNCLEAR) {
        state.wndentry->restore=0;
    }
}
/////////////////////////////////////////////////////////////////////////////
// Roll/Unroll Window. If delta > 0: Roll if < 0: Unroll if == 0: Toggle.
static void RollWindow(HWND hwnd, int delta)
{
    RECT rc;
    state.hwnd = hwnd;
    state.origin.maximized = IsZoomed(state.hwnd);
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);

    AddWindowToDB(state.hwnd);

    if (state.wndentry->restore & ROLLED && delta <= 0) { // UNROLL
        if (state.origin.maximized) {
            MinimizeWindow(hwnd);
            MaximizeWindow(hwnd);
            // state.wndentry->restore=0; // Do not clear
        } else {
            RestoreOldWin(NULL, 2, 2);
        }
    } else if ((!(state.wndentry->restore & ROLLED) && delta == 0) || delta > 0 ) { // ROLL
        GetWindowRect(state.hwnd, &rc);
        SetWindowPos(state.hwnd, NULL, 0, 0, rc.right - rc.left
              , GetSystemMetrics(SM_CYMIN)
              , SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_ASYNCWINDOWPOS);
        if(!(state.wndentry->restore & ROLLED)) { // Save window size if not saved already.
            if (!state.origin.maximized) {
                state.wndentry->width = rc.right - rc.left;
                state.wndentry->height = rc.bottom - rc.top;
            }
            state.wndentry->restore = ROLLED | state.origin.maximized|IsWindowSnapped(hwnd)<<10;
        }
    }
}
static int IsDoubleClick(int button)
{
    return state.clickbutton == button
        && GetTickCount()-state.clicktime <= GetDoubleClickTime();
}
/////////////////////////////////////////////////////////////////////////////
static int ActionMove(POINT pt, HMONITOR monitor, int button)
{
    // If this is a double-click
    if (IsDoubleClick(button)) {
        if (state.shift) {
            RollWindow(state.hwnd, 0); // Roll/Unroll Window...
        } else if (state.ctrl) {
            MinimizeWindow(state.hwnd);
        } else if (IsResizable(state.hwnd)) {
            // Toggle Maximize window
            state.action = AC_NONE; // Stop move action
            state.clicktime = 0; // Reset double-click time
            state.blockmouseup = 1; // Block the mouseup, otherwise it can trigger a context menu
            Maximize_Restore_atpt(state.hwnd, NULL, SW_TOGGLE_MAX_RESTORE, monitor);
        }
        // Prevent mousedown from propagating
        return 1;
    } else if (conf.MMMaximize&2) {
        MouseMove(pt); // Restore with simple Click
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////
static int ActionResize(POINT pt, POINT mdiclientpt, const RECT *wnd, const RECT *mon, int button)
{
    if(!IsResizable(state.hwnd)) {
        state.blockmouseup = 1;
        state.action = AC_NONE;
        return 1;
    }
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

    // Aero-move this window if this is a double-click
    if (IsDoubleClick(button)) {
        state.action = AC_NONE; // Stop resize action
        state.clicktime = 0;    // Reset double-click time
        state.blockmouseup = 1; // Block the mouseup

        // Get and set new position
        int posx, posy; // wndwidth and wndheight are defined above
        int restore = 1;
        RECT bd;
        FixDWMRect(state.hwnd, &bd);

        if(!state.shift ^ !(conf.AeroTopMaximizes&2)) { /* Extend window's borders to monitor */
            posx = wnd->left - mdiclientpt.x;
            posy = wnd->top - mdiclientpt.y;

            if (state.resize.y == RZ_TOP) {
                posy = mon->top - bd.top;
                wndheight = CLAMPH(wnd->bottom-mdiclientpt.y - mon->top + bd.top);
            } else if (state.resize.y == RZ_BOTTOM) {
                wndheight = CLAMPH(mon->bottom - wnd->top+mdiclientpt.y + bd.bottom);
            }
            if (state.resize.x == RZ_RIGHT) {
                wndwidth =  CLAMPW(mon->right - wnd->left+mdiclientpt.x + bd.right);
            } else if (state.resize.x == RZ_LEFT) {
                posx = mon->left - bd.left;
                wndwidth =  CLAMPW(wnd->right-mdiclientpt.x - mon->left + bd.left);
            } else if (state.resize.x == RZ_CENTER && state.resize.y == RZ_CENTER) {
                wndwidth = CLAMPW(mon->right - mon->left + bd.left + bd.right);
                posx = mon->left - bd.left;
                posy = wnd->top - mdiclientpt.y + bd.top ;
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
                        Maximize_Restore_atpt(state.hwnd, &pt, SW_TOGGLE_MAX_RESTORE, NULL);
                        return 1;
                    }
                }
                wndwidth = wnd->right - wnd->left - bd.left - bd.right;
                posx = wnd->left - mdiclientpt.x + bd.left;
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

        MoveWindowAsync(state.hwnd, posx, posy, wndwidth, wndheight, TRUE);

        if (!state.wndentry->restore) {
            state.wndentry->width = state.origin.width;
            state.wndentry->height = state.origin.height;
        }
        state.wndentry->restore = SNAPPED|restore;

        // Prevent mousedown from propagating
        return 1;
    }
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
    long style = GetWindowLongPtr(hwnd, GWL_STYLE);

    if(style&WS_BORDER) style &= state.shift? ~WS_CAPTION: ~(WS_CAPTION|WS_THICKFRAME);
    else style |= WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU;

    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Under Windows 10, with DWM we HAVE to resize the windows twice
    // to have proper drawing. this is a bug...
    if(HaveDWM()) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left+1, rc.bottom-rc.top
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
    int fs = ((style&WS_CAPTION) != WS_CAPTION) && EqualRect(wnd, fmon);
    fs |= ((style&WS_SYSMENU) != WS_SYSMENU)<<1 ; // &2 for sysmenu.

    return fs; // = 1 for fulscreen, 2 for SYSMENU and 3 for both.
}
/////////////////////////////////////////////////////////////////////////////
void CenterWindow(HWND hwnd)
{
    RECT mon;
    POINT pt;
    if (IsZoomed(hwnd)) return;
    GetCursorPos(&pt);
    GetMonitorRect(&pt, 0, &mon);
    MoveWindowAsync(hwnd
        , mon.left+ ((mon.right-mon.left)-state.origin.width)/2
        , mon.top + ((mon.bottom-mon.top)-state.origin.height)/2
        , state.origin.width
        , state.origin.height, TRUE);
}
static void TogglesAlwaysOnTop(HWND hwnd)
{
    LONG_PTR topmost = GetWindowLongPtr(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST;
    SetWindowLevel(hwnd, topmost? HWND_NOTOPMOST: HWND_TOPMOST);
}
static void ActionMaximize(HWND hwnd)
{
    if (state.shift) {
        MinimizeWindow(hwnd);
    } else if (IsResizable(hwnd)) {
        Maximize_Restore_atpt(hwnd, NULL, SW_TOGGLE_MAX_RESTORE, NULL);
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
    else if (action==AC_CLOSE)       PostMessage(hwnd, WM_CLOSE, 0, 0);
    else if (action==AC_LOWER)       ActionLower(NULL, hwnd, 0, state.shift);
    else if (action==AC_BORDERLESS)  ActionBorderless(hwnd);
    else if (action==AC_KILL)        ActionKill(hwnd);
    else if (action==AC_ROLL)        RollWindow(hwnd, 0);
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
static void StartSpeedMes()
{
    if(conf.AeroMaxSpeed < 65535)
        SetTimer(g_timerhwnd, SPEED_TIMER, conf.AeroSpeedTau, NULL);
}
static void StopSpeedMes()
{
    if (conf.AeroMaxSpeed < 65535)
        KillTimer(g_timerhwnd, SPEED_TIMER); // Stop speed measurement
}
/////////////////////////////////////////////////////////////////////////////
static int init_movement_and_actions(POINT pt, enum action action, int button)
{
    RECT wnd;

    // Make sure g_mainhwnd isn't in the way
    HideCursor();

    // Get window
    state.mdiclient = NULL;
    state.hwnd = WindowFromPoint(pt);
    DorQWORD lpdwResult;
    if (state.hwnd == NULL || state.hwnd == LastWin.hwnd) {
        return 0;
    }

    // Hide if tooltip
    wchar_t classname[20] = L"";
    state.hwnd = GetClass_HideIfTooltip(pt, state.hwnd, classname, ARR_SZ(classname));

    // Get monitor info
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(monitor, &mi);
    CopyRect(&state.origin.mon, &mi.rcWork);
    RECT fmon;
    CopyRect(&fmon, &mi.rcMonitor);

    // MDI or not
    state.hwnd = MDIorNOT(state.hwnd, &state.mdiclient);
    POINT mdiclientpt = {0,0};
    if (state.mdiclient) {
        if (!GetClientRect(state.mdiclient, &fmon)
         || !ClientToScreen(state.mdiclient, &mdiclientpt) ) {
            return 0;
        }
        CopyRect(&state.origin.mon, &fmon); // = MDI client rect
    }

    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    // A full screen window has No caption and is to monitor size.

    // Return if window is blacklisted,
    // if we can't get information about it,
    // or if the window is fullscreen and hans no sysmenu nor caption.
    if (state.hwnd == progman
     || blacklistedP(state.hwnd, &BlkLst.Processes)
     || blacklisted(state.hwnd, &BlkLst.Windows)
     || GetWindowPlacement(state.hwnd, &wndpl) == 0
     || GetWindowRect(state.hwnd, &wnd) == 0
     || ( (state.origin.fullscreen = IsFullscreen(state.hwnd, &wnd, &fmon)&conf.FullScreen) == conf.FullScreen )
    ){
        return 0;
    }

    // An action will be performed...
    // Set state
    state.blockaltup = 1;
    if (!SendMessageTimeout(state.hwnd, 0, 0, 0, SMTO_NORMAL, 64, &lpdwResult)) {
        state.blockmouseup = 1;
        if(action == AC_MOVE || action == AC_RESIZE) return 1;
    } // return if window has to be moved/resized
    state.origin.maximized = IsZoomed(state.hwnd);
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);

    if (!state.snap) {
        state.snap = conf.AutoSnap;
    }
    AddWindowToDB(state.hwnd);

    if (state.wndentry->restore && !state.origin.maximized) { // Set Origin width and height ==2)
        state.origin.width = state.wndentry->width;
        state.origin.height = state.wndentry->height;
    } else {
        state.origin.width = wndpl.rcNormalPosition.right-wndpl.rcNormalPosition.left;
        state.origin.height = wndpl.rcNormalPosition.bottom-wndpl.rcNormalPosition.top;
    }

    // AutoFocus
    if (conf.AutoFocus || state.ctrl) { SetForegroundWindowL(state.hwnd); }

    // Do things depending on what button was pressed
    if (action == AC_MOVE || action == AC_RESIZE) {
        // Set action state.
        state.action = action;
        // Toggle Resize and Move actions if the toggle key is Down
        if (conf.ToggleRzMvKey
        && ((button == conf.ToggleRzMvKey && !conf.KeyCombo)
          || GetAsyncKeyState(conf.ToggleRzMvKey)&0x8000) ) {
            state.action = (action == AC_MOVE)? AC_RESIZE: AC_MOVE;
        }

        GetMinMaxInfo(state.hwnd, &state.mmi.Min, &state.mmi.Max); // for CLAMPH/W functions
        SetWindowTrans(state.hwnd);
        EnumOnce(NULL); // Reset enum stuff
        StartSpeedMes(); // Speed timer

        int ret;
        if(state.action == AC_MOVE) {
            ret = ActionMove(pt, monitor, button);
        } else {
            ret = ActionResize(pt, mdiclientpt, &wnd, &state.origin.mon, button);
        }
        if (ret == 1) return 1; // block mouse down!
        UpdateCursor(pt);

        // Send WM_ENTERSIZEMOVE
        SendSizeMove_on(1);
    } else if (action == AC_MENU) {
        state.sclickhwnd = state.hwnd;
        PostMessage(g_mainhwnd, WM_SCLICK, (WPARAM)g_mchwnd, conf.AggressiveKill);
        return 1; // block mouse down
    } else {
        SClickActions(state.hwnd, action);
    }

    // We have to send the ctrl keys here too because of
    // IE (and maybe some other program?)
    Send_CTRL();

    // Remember time of this click so we can check for double-click
    state.clicktime = GetTickCount();
    state.clickpt = pt;
    state.clickbutton = button;

    // Prevent mousedown from propagating
    return 1;
}
/////////////////////////////////////////////////////////////////////////////
// Lower window if middle mouse button is used on the title bar/top of the win
// Or restore an AltDrad Aero-snapped window.
static int ActionNoAlt(POINT pt, WPARAM wParam)
{
    int willlower = ((conf.LowerWithMMB&1) && !state.alt)
                 || ((conf.LowerWithMMB&2) &&  state.alt);
    if ((willlower || conf.NormRestore)
    &&  !state.action && (wParam == WM_MBUTTONDOWN || wParam == WM_LBUTTONDOWN)) {
        HWND nhwnd = WindowFromPoint(pt);
        if (!nhwnd) return 0;
        HWND hwnd = MDIorNOT(nhwnd, &state.mdiclient);
        if (blacklisted(hwnd, &BlkLst.Windows))
            return 0;

        int area = HitTestTimeout(nhwnd, pt.x, pt.y);

        if (willlower && wParam == WM_MBUTTONDOWN
        && (area == HTCAPTION || (area >= HTTOP && area <= HTTOPRIGHT)
          || area == HTSYSMENU || area == HTMINBUTTON || area == HTMAXBUTTON
          || area == HTCLOSE || area == HTHELP)
        && !blacklisted(hwnd, &BlkLst.MMBLower)) {
            ActionLower(NULL, hwnd, 0, state.shift);
            return 1;
        } else if (conf.NormRestore
        && wParam == WM_LBUTTONDOWN && area == HTCAPTION
        && !IsZoomed(hwnd) && !IsWindowSnapped(hwnd)
        && !blacklisted(hwnd, &BlkLst.MMBLower)) {
            if ((state.wndentry=GetWindowInDB(hwnd))) {
                // Set NormRestore to 2 in order to signal that
                // The window should be restored
                conf.NormRestore=2;
                state.hwnd = hwnd;
                state.origin.maximized=0;
            }
        }
    } else if (wParam == WM_LBUTTONUP) {
        if (conf.NormRestore) conf.NormRestore = 1;
    } else if (conf.NormRestore > 1) {
        RestoreOldWin(&pt, 2, 1);
        conf.NormRestore = 1;
    }
    return -1; // fall through...
}
/////////////////////////////////////////////////////////////////////////////
static int WheelActions(POINT pt, PMSLLHOOKSTRUCT msg, WPARAM wParam)
{
    int delta = GET_WHEEL_DELTA_WPARAM(msg->mouseData);

    // 1st Scroll inactive windows.. If enabled
    if (!state.alt && !state.action && conf.InactiveScroll) {
        return ScrollPointedWindow(pt, delta, wParam);
    } else if(!state.alt || state.action != conf.GrabWithAlt
          || (conf.GrabWithAlt && !IsSamePTT(&pt, &state.clickpt)) ) {
        return 0; // continue if no actions to be made
    }

    // Get pointed window
    HideCursor();
    HWND nhwnd = WindowFromPoint(pt);
    if (!nhwnd) return 0;
    HWND hwnd = MDIorNOT(nhwnd, &state.mdiclient);

    if (conf.RollWithTBScroll && wParam == WM_MOUSEWHEEL && !state.ctrl) {

        int area= HitTestTimeout(nhwnd, pt.x, pt.y);
        if(area == HTCAPTION || area == HTTOP ) {
            RollWindow(hwnd, delta);
            // Block original scroll event
            state.blockaltup = 1;
            return 1;
        }
    }

    // Return if blacklisted or fullscreen.
    RECT wnd;
    if (blacklistedP(hwnd, &BlkLst.Processes) || blacklisted(hwnd, &BlkLst.Scroll)) {
        return 0;
    } else if (conf.FullScreen == 1 && GetWindowRect(hwnd, &wnd)) {
        RECT mon;
        GetMonitorRect(&pt, 1, &mon);
        if((IsFullscreen(hwnd, &wnd, &mon)&conf.FullScreen) == conf.FullScreen)
            return 0;
    }
    int ret=1;
    enum action action = (wParam == WM_MOUSEWHEEL)? conf.Mouse.Scroll: conf.Mouse.HScroll;

    if (action == AC_ALTTAB)            ret = ActionAltTab(pt, delta);
    else if (action == AC_VOLUME)       ret = ActionVolume(delta);
    else if (action == AC_TRANSPARENCY) ret = ActionTransparency(hwnd, delta);
    else if (action == AC_LOWER)        ActionLower(&pt, hwnd, delta, state.shift);
    else if (action == AC_MAXIMIZE)     ActionMaxRestMin(&pt, hwnd, delta);
    else if (action == AC_ROLL)         RollWindow(hwnd, delta);
    else if (action == AC_HSCROLL)      ret = ScrollPointedWindow(pt, -delta, WM_MOUSEHWHEEL);

    state.blockaltup = ret; // block or not;
    return ret; // block or next hook
}
/////////////////////////////////////////////////////////////////////////////
// Called on MouseUp and on AltUp when using GrabWithAlt
static void FinishMovement()
{
    if (LastWin.hwnd && (state.moving == NOT_MOVED || (!conf.FullWin&&state.moving))) {
        // to erase the last rectangle...
        if(!conf.FullWin) {
            DrawRect(hdcc, &oldRect);
            if(state.action == AC_RESIZE) ResizeAllSnappedWindowsAsync();
        }

        if (IsWindow(LastWin.hwnd)) {
            LastWin.end = 1;
            MoveWindowInThread(&LastWin);
        }
    }
    StopSpeedMes();

    // Auto Remaximize if option enabled and conditions are met.
    if (conf.AutoRemaximize && state.moving
    && (state.origin.maximized || state.origin.fullscreen)
    && !state.shift && !state.mdiclient && state.action == AC_MOVE) {
        state.action = AC_NONE;
        POINT pt;
        GetCursorPos(&pt);
        HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        if(monitor != state.origin.monitor) {
            Sleep(10);  // Wait a little for moveThread.
            if(LastWin.hwnd) Sleep(100); // Wait more...

            if(state.origin.maximized){
                Maximize_Restore_atpt(state.hwnd, NULL, SW_MAXIMIZE, monitor);
            }
            if(state.origin.fullscreen){
                Maximize_Restore_atpt(state.hwnd, NULL, SW_FULLSCREEN, monitor);
            }
        }
    }
    // Send WM_EXITSIZEMOVE
    SendSizeMove_on(0);

    state.action = AC_NONE;
    state.moving = 0;

    // Unhook mouse if Alt is released
    if (!state.alt) {
        UnhookMouse();
    } else {
        // Just hide g_mainhwnd
        HideCursor();
    }
}
/////////////////////////////////////////////////////////////////////////////
// This is somewhat the main function, it is active only when the ALT key is
// pressed, or is always on when conf.keepMousehook is enabled.
static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || (conf.ScrollLockState && !(GetKeyState(VK_SCROLL)&1)))
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Set up some variables
    PMSLLHOOKSTRUCT msg = (PMSLLHOOKSTRUCT)lParam;
    POINT pt = msg->pt;

    // Handle mouse move and scroll
    if (wParam == WM_MOUSEMOVE) {
        // Store prevpt so we can check if the hook goes stale
        state.prevpt = pt;

        // Reset double-click time
        if (!IsSamePTT(&pt, &state.clickpt)) {
            state.clicktime = 0;
        }
        // Move the window
        if (state.action) { // resize or move...
            // Move the window every few frames.
            static char updaterate;
            updaterate = (updaterate+1)%(state.action==AC_MOVE? conf.MoveRate: conf.ResizeRate);
            if (updaterate == 0) {
                MouseMove(pt);
            }
            // Start speed measurement
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
    } else if (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL) {
        int ret = WheelActions(pt, msg, wParam);
        if (ret == 1) return 1;
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    // Do some Actions without Alt Norm Restore and Lower with MMB
    int ret = ActionNoAlt(pt, wParam);
    if (ret == 0) return CallNextHookEx(NULL, nCode, wParam, lParam);
    else if (ret == 1) return 1;

    // Get Button state
    enum button button =
        (wParam==WM_LBUTTONDOWN||wParam==WM_LBUTTONUP)?BT_LMB:
        (wParam==WM_MBUTTONDOWN||wParam==WM_MBUTTONUP)?BT_MMB:
        (wParam==WM_RBUTTONDOWN||wParam==WM_RBUTTONUP)?BT_RMB:
        (HIWORD(msg->mouseData)==XBUTTON1)?BT_MB4:
        (HIWORD(msg->mouseData)==XBUTTON2)?BT_MB5:BT_NONE;

    enum {STATE_NONE, STATE_DOWN, STATE_UP} buttonstate =
          (wParam==WM_LBUTTONDOWN||wParam==WM_MBUTTONDOWN
        || wParam==WM_RBUTTONDOWN||wParam==WM_XBUTTONDOWN)? STATE_DOWN:
          (wParam==WM_LBUTTONUP  ||wParam==WM_MBUTTONUP
        || wParam==WM_RBUTTONUP  ||wParam==WM_XBUTTONUP)?STATE_UP:STATE_NONE;

    enum action action = GetAction(button);

    // Check if the click is is a Hotclick and should enable ALT.
    int is_hotclick = IsHotclick(button);
    if(is_hotclick && buttonstate == STATE_DOWN) {
        state.alt = button;
        if(!action) return 1;
    } else if (is_hotclick && buttonstate == STATE_UP) {
        state.alt = 0;
    }

    // Return if no mouse action will be started
    if (!action) return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Handle another click if we are already busy with an action
    if (buttonstate == STATE_DOWN && state.action && state.action != conf.GrabWithAlt) {
        // Maximize/Restore the window if pressing Move, Resize mouse buttons.
        if((conf.MMMaximize&1) && state.action == AC_MOVE && action == AC_RESIZE) {
            if(LastWin.hwnd) Sleep(10);
            if(IsZoomed(state.hwnd)) {
                state.moving = 0;
                MouseMove(pt);
            } else {
                state.moving = CURSOR_ONLY; // So that MouseMove will only move g_mainhwnd
                Maximize_Restore_atpt(state.hwnd, &pt, SW_MAXIMIZE, NULL);
            }
            state.blockmouseup = 1;
        }
        return 1; // Block mousedown so AltDrag.exe does not remove g_mainhwnd

    // INIT ACTIONS on mouse down if Alt is down...
    } else if (buttonstate == STATE_DOWN && state.alt) {
        // Double ckeck some hotkey is pressed.
        if(!state.action
        && !IsHotclick(state.alt)
        && !IsHotKeyDown(&conf.Hotkeys)) {
            UnhookMouse();
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        ret = init_movement_and_actions(pt, action, button);
        if(!ret) return CallNextHookEx(NULL, nCode, wParam, lParam);
        else     return 1; // block mousedown

    // BUTTON UP
    } else if (buttonstate == STATE_UP) {
        SetWindowTrans(NULL);
        if(state.blockmouseup) {
            state.blockmouseup = 0;
            return 1;
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
static void DeleteDCPEN()
{
    if (hdcc) { DeleteDC(hdcc); hdcc = NULL; }
    if (hpenDot_Global) { DeleteObject(hpenDot_Global); hpenDot_Global = NULL; }
}
/////////////////////////////////////////////////////////////////////////////
static void UnhookMouse()
{
    // Stop action
    state.action = AC_NONE;
    state.ctrl = 0;
    state.shift = 0;
    state.ignorectrl = 0;
    state.moving = 0;
    DeleteDCPEN();

    SetWindowTrans(NULL);
    StopSpeedMes();

    if (conf.NormRestore) conf.NormRestore = 1;

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
/////////////////////////////////////////////////////////////////////////////
// Window for timers only...
static LRESULT CALLBACK TimerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TIMER) {
        if (wParam == INIT_TIMER) {
            KillTimer(g_timerhwnd, INIT_TIMER);

            // Hook mouse if a permanent hook is needed
            if (conf.keepMousehook) {
                HookMouse();
                SetTimer(g_timerhwnd, REHOOK_TIMER, 5000, NULL); // Start rehook timer
            }
        } else if (wParam == REHOOK_TIMER) {
            // Silently rehook hooks if they have been stopped (>= Win7 and LowLevelHooksTimeout)
            // This can often happen if locking or sleeping the computer a lot
            POINT pt;
            GetCursorPos(&pt);
            if (mousehook && (pt.x != state.prevpt.x || pt.y != state.prevpt.y)) {
                UnhookWindowsHookEx(mousehook);
                mousehook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hinstDLL, 0);
            }
        } else if (wParam == SPEED_TIMER) {
            static POINT oldpt;
            static int has_moved_to_fixed_pt;
            if(state.moving) state.Speed=max(abs(oldpt.x-state.prevpt.x), abs(oldpt.y-state.prevpt.y));
            else state.Speed=0;
            oldpt = state.prevpt;
            if(state.moving && state.Speed == 0 && !has_moved_to_fixed_pt && !MM_THREAD_ON) {
                has_moved_to_fixed_pt = 1;
                MouseMove(state.prevpt);
            }
            if(state.Speed) has_moved_to_fixed_pt = 0;
        }
    } else if (msg == WM_DESTROY) {
        KillTimer(g_timerhwnd, REHOOK_TIMER);
        KillTimer(g_timerhwnd, SPEED_TIMER);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
// Window for single click commands
static LRESULT CALLBACK SClickWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND && state.sclickhwnd) {
        enum action action = wParam;

        SClickActions(state.sclickhwnd, action);
        state.sclickhwnd = NULL;

        return 0;
    } else {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
static void freeblacklists()
{
    struct blacklist *list = (void *)&BlkLst;
    unsigned i;
    for (i=0; i< sizeof(BlkLst)/sizeof(struct blacklist); i++) {
        free(list->data);
        list->data = NULL;
        free(list->items);
        list->items = NULL;
        list->length = 0;
        list++;
    }
}
/////////////////////////////////////////////////////////////////////////////
// To be called before Free Library. Ideally it should free everything
__declspec(dllexport) void Unload()
{
    DeleteDCPEN();

    if (mousehook) { UnhookWindowsHookEx(mousehook); mousehook = NULL; }
    UnregisterClass(APP_NAME"-Timers", hinstDLL);
    DestroyWindow(g_timerhwnd);
    UnregisterClass(APP_NAME"-SClick", hinstDLL);
    DestroyWindow(g_mchwnd);

    freeblacklists();

    free(monitors);
    free(hwnds);
    free(wnds);
    free(snwnds);
}
/////////////////////////////////////////////////////////////////////////////
// blacklist is coma separated and title and class are | separated.
static void readblacklist(const wchar_t *inipath, struct blacklist *blacklist
                        , const wchar_t *blacklist_str)
{
    wchar_t txt[1024];

    DWORD ret = GetPrivateProfileString(L"Blacklist", blacklist_str, L"", txt, ARR_SZ(txt), inipath);
    if(!ret || txt[0] == '\0') {
        blacklist->data = NULL;
        blacklist->length = 0;
        blacklist->items = NULL;
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
///////////////////////////////////////////////////////////////////////////
// Used to read Hotkeys and Hotclicks
static void readhotkeys(const wchar_t *inipath, const wchar_t *name, const wchar_t *def, struct hotkeys_s *HK)
{
    wchar_t txt[64];

    GetPrivateProfileString(L"Input", name, def, txt, ARR_SZ(txt), inipath);
    wchar_t *pos = txt;
    while (*pos) {
        // Store key
        if(HK->length == MAXKEYS) break;
        HK->keys[HK->length++] = whex2u(pos);

        while(*pos && *pos != ' ') pos++; // go to next space
        while(*pos == ' ') pos++; // go to next char after spaces.
    }
}
static unsigned char readsinglekey(const wchar_t *inipath, const wchar_t *name,  const wchar_t *def)
{
    wchar_t txt[4];
    GetPrivateProfileString(L"Input", name, def, txt, ARR_SZ(txt), inipath);
    if(*txt) {
        return whex2u(txt);
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////
// Has to be called at startup, it mainly reads the config.
__declspec(dllexport) void Load(HWND mainhwnd)
{
    // Load settings
    wchar_t txt[32];
    wchar_t inipath[MAX_PATH];
    state.action = AC_NONE;
    state.shift = 0;
    state.moving = 0;
    LastWin.hwnd = NULL;
    conf.Hotkeys.length = 0;

    // Get ini path
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    wcscpy(&inipath[wcslen(inipath)-3], L"ini");

    // [General]
    conf.AutoFocus =    GetPrivateProfileInt(L"General", L"AutoFocus", 0, inipath);
    conf.AutoSnap=state.snap=GetPrivateProfileInt(L"General", L"AutoSnap", 0, inipath);
    conf.Aero =         GetPrivateProfileInt(L"General", L"Aero", 1, inipath);
    conf.SmartAero =    GetPrivateProfileInt(L"General", L"SmartAero", 1, inipath);
    conf.StickyResize  =GetPrivateProfileInt(L"General", L"StickyResize", 0, inipath);
    conf.InactiveScroll=GetPrivateProfileInt(L"General", L"InactiveScroll", 0, inipath);
    conf.NormRestore   =GetPrivateProfileInt(L"General", L"NormRestore", 0, inipath);
    conf.MDI =          GetPrivateProfileInt(L"General", L"MDI", 0, inipath);
    conf.ResizeCenter = GetPrivateProfileInt(L"General", L"ResizeCenter", 1, inipath);
    conf.CenterFraction=CLAMP(0, GetPrivateProfileInt(L"General", L"CenterFraction", 24, inipath), 100);
    conf.AHoff        = CLAMP(0, GetPrivateProfileInt(L"General", L"AeroHoffset", 50, inipath),    100);
    conf.AVoff        = CLAMP(0, GetPrivateProfileInt(L"General", L"AeroVoffset", 50, inipath),    100);
    conf.MoveTrans    = CLAMP(0, GetPrivateProfileInt(L"General", L"MoveTrans", 0, inipath), 255);
    conf.MMMaximize   = GetPrivateProfileInt(L"General", L"MMMaximize", 1, inipath);

    // [Advanced]
    conf.ResizeAll     = GetPrivateProfileInt(L"Advanced", L"ResizeAll",       1, inipath);
    conf.FullScreen    = 1 + 2 * !!GetPrivateProfileInt(L"Advanced", L"FullScreen", 1, inipath);
    conf.AutoRemaximize= GetPrivateProfileInt(L"Advanced", L"AutoRemaximize",  0, inipath);
    conf.SnapThreshold = GetPrivateProfileInt(L"Advanced", L"SnapThreshold",  20, inipath);
    conf.AeroThreshold = GetPrivateProfileInt(L"Advanced", L"AeroThreshold",   5, inipath);
    conf.AeroTopMaximizes=GetPrivateProfileInt(L"Advanced",L"AeroTopMaximizes",1, inipath);
    conf.UseCursor     = GetPrivateProfileInt(L"Advanced", L"UseCursor",       1, inipath);
    conf.MinAlpha      = CLAMP(1,    GetPrivateProfileInt(L"Advanced", L"MinAlpha", 8, inipath), 255);
    conf.AlphaDeltaShift=CLAMP(-128, GetPrivateProfileInt(L"Advanced", L"AlphaDeltaShift", 8, inipath), 127);
    conf.AlphaDelta    = CLAMP(-128, GetPrivateProfileInt(L"Advanced", L"AlphaDelta", 64, inipath), 127);
    conf.AeroMaxSpeed  = CLAMP(0, GetPrivateProfileInt(L"Advanced", L"AeroMaxSpeed", 65535, inipath), 65535);
    conf.AeroSpeedTau  = CLAMP(1, GetPrivateProfileInt(L"Advanced", L"AeroSpeedTau", 32, inipath), 255);

    // [Performance]
    conf.MoveRate  = GetPrivateProfileInt(L"Performance", L"MoveRate", 2, inipath);
    conf.ResizeRate= GetPrivateProfileInt(L"Performance", L"ResizeRate", 4, inipath);
    conf.FullWin   = GetPrivateProfileInt(L"Performance", L"FullWin", 1, inipath);
    conf.RefreshRate=GetPrivateProfileInt(L"Performance", L"RefreshRate", 7, inipath);

    // [Input]
    struct {
        wchar_t *key;
        wchar_t *def;
        enum action *ptr;
    } buttons[] = {
        {L"LMB",        L"Move",    &conf.Mouse.LMB},
        {L"MMB",        L"Maximize",&conf.Mouse.MMB},
        {L"RMB",        L"Resize",  &conf.Mouse.RMB},
        {L"MB4",        L"Nothing", &conf.Mouse.MB4},
        {L"MB5",        L"Nothing", &conf.Mouse.MB5},
        {L"Scroll",     L"Nothing", &conf.Mouse.Scroll},
        {L"HScroll",    L"Nothing", &conf.Mouse.HScroll},
        {L"GrabWithAlt",L"Nothing", &conf.GrabWithAlt},
        {NULL}
    };
    int i;
    int action_menu_load = 0;
    for (i=0; buttons[i].key != NULL; i++) {
        GetPrivateProfileString(L"Input", buttons[i].key, buttons[i].def, txt, ARR_SZ(txt), inipath);
        if      (!wcsicmp(txt,L"Move"))         *buttons[i].ptr = AC_MOVE;
        else if (!wcsicmp(txt,L"Resize"))       *buttons[i].ptr = AC_RESIZE;
        else if (!wcsicmp(txt,L"Minimize"))     *buttons[i].ptr = AC_MINIMIZE;
        else if (!wcsicmp(txt,L"Maximize"))     *buttons[i].ptr = AC_MAXIMIZE;
        else if (!wcsicmp(txt,L"Center"))       *buttons[i].ptr = AC_CENTER;
        else if (!wcsicmp(txt,L"AlwaysOnTop"))  *buttons[i].ptr = AC_ALWAYSONTOP;
        else if (!wcsicmp(txt,L"Borderless"))   *buttons[i].ptr = AC_BORDERLESS;
        else if (!wcsicmp(txt,L"Close"))        *buttons[i].ptr = AC_CLOSE;
        else if (!wcsicmp(txt,L"Lower"))        *buttons[i].ptr = AC_LOWER;
        else if (!wcsicmp(txt,L"AltTab"))       *buttons[i].ptr = AC_ALTTAB;
        else if (!wcsicmp(txt,L"Volume"))       *buttons[i].ptr = AC_VOLUME;
        else if (!wcsicmp(txt,L"Transparency")) *buttons[i].ptr = AC_TRANSPARENCY;
        else if (!wcsicmp(txt,L"Roll"))         *buttons[i].ptr = AC_ROLL;
        else if (!wcsicmp(txt,L"HScroll"))      *buttons[i].ptr = AC_HSCROLL;
        else if (!wcsicmp(txt,L"Menu"))       { *buttons[i].ptr = AC_MENU ; action_menu_load=1; }
        else if (!wcsicmp(txt,L"Kill"))         *buttons[i].ptr = AC_KILL;
        else                                    *buttons[i].ptr = AC_NONE;
    }

    conf.LowerWithMMB    = GetPrivateProfileInt(L"Input", L"LowerWithMMB",    0, inipath);
    conf.AggressivePause = GetPrivateProfileInt(L"Input", L"AggressivePause", 0, inipath);
    conf.AggressiveKill  = GetPrivateProfileInt(L"Input", L"AggressiveKill",  0, inipath);
    conf.RollWithTBScroll= GetPrivateProfileInt(L"Input", L"RollWithTBScroll",0, inipath);
    conf.KeyCombo        = GetPrivateProfileInt(L"Input", L"KeyCombo",        0, inipath);
    conf.ScrollLockState = GetPrivateProfileInt(L"Input", L"ScrollLockState", 0, inipath);

    readhotkeys(inipath, L"Hotkeys",  L"A4 A5",   &conf.Hotkeys);
    readhotkeys(inipath, L"Hotclicks",L"",        &conf.Hotclick);
    readhotkeys(inipath, L"Killkeys", L"09 4C 2E",&conf.Killkey);

    conf.ToggleRzMvKey = readsinglekey(inipath, L"ToggleRzMvKey", L"");
    conf.HScrollKey = readsinglekey(inipath, L"HScrollKey", L"10"); // VK_SHIFT

    // Zero-out wnddb hwnds
    for (i=0; i < NUMWNDDB; i++) {
        wnddb.items[i].hwnd = NULL;
    }
    wnddb.pos = &wnddb.items[0];

    // Capture main hwnd from caller. This is also the cursor wnd
    g_mainhwnd = mainhwnd;

    // Create window for timers
    WNDCLASSEX wnd = { sizeof(WNDCLASSEX), 0, TimerWindowProc, 0, 0, hinstDLL
                     , NULL, NULL, NULL, NULL, APP_NAME"-Timers", NULL };
    RegisterClassEx(&wnd);
    g_timerhwnd = CreateWindowEx(0, wnd.lpszClassName, NULL, 0
                     , 0, 0, 0, 0, g_mainhwnd, NULL, hinstDLL, NULL);
    // Create a timer to do further initialization
    SetTimer(g_timerhwnd, INIT_TIMER, 10, NULL);

    // Window for Action Menu
    if (action_menu_load) {
        WNDCLASSEX wnd2 = { sizeof(WNDCLASSEX), 0, SClickWindowProc, 0, 0, hinstDLL
                         , NULL, NULL, NULL, NULL, APP_NAME"-SClick", NULL };
        RegisterClassEx(&wnd2);
        g_mchwnd = CreateWindowEx(0, wnd2.lpszClassName, NULL, 0
                         , 0, 0, 0 , 0, g_mainhwnd, NULL, hinstDLL, NULL);
    }
    readblacklist(inipath, &BlkLst.Processes, L"Processes");
    readblacklist(inipath, &BlkLst.Windows,   L"Windows");
    readblacklist(inipath, &BlkLst.Snaplist,  L"Snaplist");
    readblacklist(inipath, &BlkLst.MDIs,      L"MDIs");
    readblacklist(inipath, &BlkLst.Pause,     L"Pause");
    readblacklist(inipath, &BlkLst.MMBLower,  L"MMBLower");
    readblacklist(inipath, &BlkLst.Scroll,    L"Scroll");
    readblacklist(inipath, &BlkLst.SSizeMove, L"SSizeMove");

    conf.keepMousehook = ((conf.LowerWithMMB&1)|conf.NormRestore|conf.InactiveScroll|conf.Hotclick.length);
}
/////////////////////////////////////////////////////////////////////////////
// Do not forget the -e_DllMain@12 for gcc...
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        hinstDLL = hInst;
    }
    return TRUE;
}
