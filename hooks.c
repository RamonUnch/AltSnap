/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2021                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"

#define COBJMACROS
#include <mmdeviceapi.h>
#include <endpointvolume.h>

// Boring stuff
#define REHOOK_TIMER    WM_APP+1
#define SPEED_TIMER     WM_APP+2

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
enum buttonstate {STATE_NONE, STATE_DOWN, STATE_UP};

static int init_movement_and_actions(POINT pt, enum action action, int button);
static void FinishMovement();
RECT oldRect;
HDC hdcc;
HPEN hpenDot_Global=NULL;

static struct windowRR {
    HWND hwnd;
    int x;
    int y;
    int width;
    int height;
    UCHAR end;
    UCHAR maximize;
} LastWin;

// State
static struct {
    POINT clickpt;
    POINT prevpt;
    POINT ctrlpt;
    POINT shiftpt;
    POINT offset;

    HWND hwnd;
    HWND sclickhwnd;
    HWND mdiclient;
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

    struct {
        POINT Min;
        POINT Max;
    } mmi;
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
    UCHAR ModKey;

    UCHAR keepMousehook;
    UCHAR KeyCombo;
    UCHAR FullScreen;
    UCHAR AggressiveKill;

    UCHAR SmartAero;
    UCHAR StickyResize;
    UCHAR HScrollKey;
    UCHAR ScrollLockState;

    UCHAR TitlebarMove;
  # ifdef WIN64
    UCHAR FancyZone;
  #endif
    UCHAR UseZones;
    char InterZone;
    char SnapGap;

    UCHAR Hotkeys[MAXKEYS+1];
    UCHAR Hotclick[MAXKEYS+1];
    UCHAR Killkey[MAXKEYS+1];

    enum action GrabWithAlt[2];
    struct {
        enum action LMB[2], RMB[2], MMB[2], MB4[2], MB5[2], Scroll[2], HScroll[2];
    } Mouse;
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
} BlkLst;

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
static pure int blacklisted(HWND hwnd, struct blacklist *list)
{
    wchar_t title[256]=L"", classname[256]=L"";
    DorQWORD mode ;
    unsigned i;

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
        // LOGA("%S|%S, %d/%d\n", list->items[i].title, list->items[i].classname, i+1, list->length);
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
        // LOGA("%S, %d/%d\n", list->items[i].title, i+1, list->length);
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
    return (conf.ResizeAll
        || GetWindowLongPtr(hwnd, GWL_STYLE)&WS_THICKFRAME
        || blacklisted(hwnd, &BlkLst.AResize)); // Always resize list
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
        SendMessageTimeout(ancestor, WM_NCHITTEST, 0, lParam, SMTO_NORMAL, 255, &area);
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
static void GetEnoughSpace(void **ptr, unsigned *num, unsigned *alloc, size_t size)
{
    if (*num >= *alloc) {
        *alloc += 4;
        *ptr = realloc(*ptr, *alloc*size);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Enumerate callback proc
unsigned monitors_alloc = 0;
BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    // Make sure we have enough space allocated
    GetEnoughSpace((void **)&monitors, &nummonitors, &monitors_alloc, sizeof(RECT));
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
        && IsWindowVisible(window)
        && !IsIconic(window)
        && !IsWindowCloaked(window)
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
    GetEnoughSpace((void **)&wnds, &numwnds, &wnds_alloc, sizeof(RECT));

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
    GetEnoughSpace((void **)&snwnds, &numsnwnds, &snwnds_alloc, sizeof(struct snwdata));

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
BOOL CALLBACK EnumTouchingWindows(HWND hwnd, LPARAM lParam)
{
    // Make sure we have enough space allocated
    GetEnoughSpace((void **)&snwnds, &numsnwnds, &snwnds_alloc, sizeof(struct snwdata));

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
static void EnumOnce(RECT **bd);
static void ResizeTouchingWindows(int posx, int posy, int width, int height)
{
    RECT *bd;
    EnumOnce(&bd);
    posx += bd->left; posy += bd->top;
    width -= bd->left+bd->right;
    height -= bd->top+bd->bottom;
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
        RECT nbd;
        FixDWMRect(hwnd, &nbd);

        if (conf.FullWin) {
            MoveWindowAsync(hwnd, nwnd->left-nbd.left, nwnd->top-nbd.top
                      , nwnd->right - nwnd->left + nbd.left + nbd.right
                      , nwnd->bottom - nwnd->top + nbd.top + nbd.bottom);
        }
        snwnds[i].flag = flag|TORESIZE;
    }
}
/////////////////////////////////////////////////////////////////////////////
static void ResizeAllSnappedWindowsAsync()
{
    unsigned i;
    for (i=0; i < numsnwnds; i++) {
        if(snwnds[i].flag&TORESIZE) {
            RECT bd;
            FixDWMRect(snwnds[i].hwnd, &bd);
            InflateRectBorder(&snwnds[i].wnd, &bd);
            MoveWindowAsync(snwnds[i].hwnd
                , snwnds[i].wnd.left, snwnds[i].wnd.top
                , snwnds[i].wnd.right-snwnds[i].wnd.left
                , snwnds[i].wnd.bottom-snwnds[i].wnd.top);
        }
    }
}
///////////////////////////////////////////////////////////////////////////
// Just used in Enum
static void EnumMdi()
{
    // Make sure we have enough space allocated
    GetEnoughSpace((void **)&monitors, &nummonitors, &monitors_alloc, sizeof(RECT));

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
         || IsInRangeT(snapwnd.left, posx, posx+wndwidth, thresholdy)) {
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
        MoveWindowAsync(hwnd, fmon.left, fmon.top, fmon.right-fmon.left, fmon.bottom-fmon.top);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Move the windows in a thread in case it is very slow to resize
static void MoveResizeWindowThread(struct windowRR *lw, UINT flag)
{
    HWND hwnd;
    hwnd = lw->hwnd;

    if (lw->end&1 && conf.FullWin) Sleep(conf.RefreshRate+5); // at least 5ms...

    SetWindowPos(hwnd, NULL, lw->x, lw->y, lw->width, lw->height, flag);

    // Send WM_SYNCPAINT in case to wait for the end of movement
    // And to avoid windows to "slide through" the whole WM_MOVE queue
    // if(flag|SWP_ASYNCWINDOWPOS)
    SendMessage(hwnd, WM_SYNCPAINT, 0, 0);

    if (lw->end&1 && !conf.FullWin && state.origin.maximized) {
        InvalidateRect(hwnd, NULL, FALSE);
        lw->hwnd = NULL;
        return;
    }

    if (conf.RefreshRate) Sleep(conf.RefreshRate);

    lw->hwnd = NULL;
}
static DWORD WINAPI ResizeWindowThread(LPVOID LastWinV)
{
    MoveResizeWindowThread(LastWinV, SWP_NOZORDER|SWP_NOACTIVATE|SWP_ASYNCWINDOWPOS);
    return 0;
}
static DWORD WINAPI MoveWindowThread(LPVOID LastWinV)
{
    MoveResizeWindowThread(LastWinV, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS);
    return 0;
}
static void MoveWindowInThread(struct windowRR *lw)
{
    DWORD lpThreadId;
    CloseHandle(
        CreateThread( NULL, STACK
            , (lw->end&2)? MoveWindowThread: ResizeWindowThread
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
///////////////////////////////////////////////////////////////////////////
#define AERO_TH conf.AeroThreshold
#define MM_THREAD_ON (LastWin.hwnd && conf.FullWin)
static int AeroMoveSnap(POINT pt, int *posx, int *posy, int *wndwidth, int *wndheight)
{
    // return if last resizing is not finished or no Aero or not resizable.
    if(!conf.Aero || MM_THREAD_ON || !state.resizable) return 0;
    LastWin.maximize = 0;

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
    LastWin.end = 0; // We are resizing the window.

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
                Maximize_Restore_atpt(state.hwnd, &pt, SW_MAXIMIZE, NULL);
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
        if (restore&SNAPPED) {
            // Restore original window size
            if (restore) ClearRestoreData(state.hwnd);
            restore = 0;
            *wndwidth = state.origin.width;
            *wndheight = state.origin.height;
            LastWin.end = 0; // Restored => resize
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

        // If we go too fast then donot move the window
        if (state.Speed > conf.AeroMaxSpeed) return 1;
        if (conf.FullWin) {
            if (IsZoomed(state.hwnd)) Maximize_Restore_atpt(state.hwnd, &pt, SW_RESTORE, NULL);
            MoveWindowAsync(state.hwnd, *posx, *posy, *wndwidth, *wndheight);
            SendMessage(state.hwnd, WM_SYNCPAINT, 0, 0);
            return 1;
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
static pure enum action GetAction(const enum button button)
{
    if (button) // Ugly pointer arithmetic (LMB <==> button == 2)
        return conf.Mouse.LMB[(button-2)*2+ModKey()];
    else
        return AC_NONE;
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
#define IsKillkey(a)  IsHotkeyy(a, conf.Killkey)

static int IsHotClickPt(const enum button button, const POINT pt, const enum buttonstate bstate)
{
    if (IsHotkeyy(button, conf.Hotclick)) {
        return 1;
    } else if (conf.TitlebarMove && button == BT_LMB) {
        if (conf.TitlebarMove&2 && bstate == STATE_UP) {
            conf.TitlebarMove = 1;
            return 1;
        }

        HideCursor(); // In case...
        HWND hwnd = WindowFromPoint(pt);
        if(HTCAPTION == HitTestTimeoutbl(hwnd, pt.x, pt.y)) {
            conf.TitlebarMove = 2;
            return 1;
        }
    }
    return 0;
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
// if pt is NULL then the window is not moved when restored.
// index 1 => normal restore on any move restore & 1
// index 2 => Rolled window restore & 2
// restore & 3 => Both 1 & 2 ie: Maximized then rolled.
// Set was_snapped to 2 if you wan to
// if pt is NULL we also restore with SWP_NOSENDCHANGING
static void RestoreOldWin(const POINT *pt, unsigned was_snapped, unsigned index)
{
    // Restore old width/height?
    unsigned restore = 0;
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(state.hwnd, &rwidth, &rheight);

    if (((rdata_flag & index) && !(state.origin.maximized&&rdata_flag&2))) {
        // Set origin width and height to the saved values
        restore = rdata_flag;
        state.origin.width = rwidth;
        state.origin.height = rheight;
        ClearRestoreData(state.hwnd);
    }

    RECT wnd;
    GetWindowRect(state.hwnd, &wnd);

    // Set offset
    if (pt) {
        state.offset.x = state.origin.width  * min(pt->x-wnd.left, wnd.right-wnd.left)
                       / max(wnd.right-wnd.left,1);
        state.offset.y = state.origin.height * min(pt->y-wnd.top, wnd.bottom-wnd.top)
                       / max(wnd.bottom-wnd.top,1);
    }
    if (state.origin.maximized || was_snapped == 1) {
        if (rdata_flag&ROLLED || restore&ROLLED) {
            // if we restore a  Rolled Maximized window...
            state.offset.y = GetSystemMetrics(SM_CYMIN)/2;
        }
    } else if (restore) {
        if (was_snapped == 2 && pt) {
            // Restoring via normal drag we want
            // the offset along Y to be unchanged...
            state.offset.y = pt->y-wnd.top;
        }
        // If pt is null it means UnRoll...
        SetWindowPos(state.hwnd, NULL
                , pt? pt->x - state.offset.x - mdiclientpt.x: 0
                , pt? pt->y - state.offset.y - mdiclientpt.y: 0
                , state.origin.width, state.origin.height
                , pt? SWP_NOZORDER: SWP_NOSENDCHANGING|SWP_NOZORDER|SWP_NOMOVE|SWP_ASYNCWINDOWPOS);
        ClearRestoreData(state.hwnd);
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
static void DrawRect(const RECT *rc)
{
    Rectangle(hdcc, rc->left+1, rc->top+1, rc->right, rc->bottom);
}

static void RestrictCursorToMon()
{
    // Restrict pt within origin monitor if Ctrlis being pressed
    if (state.ctrl && !state.ignorectrl) {
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
    if (!state.moving && state.action == AC_MOVE) {
        was_snapped = IsWindowSnapped(state.hwnd);
        RestoreOldWin(&pt, was_snapped, 1);
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
    if (state.action == AC_MOVE) {
        // Set end to 2 to add the SWP_NOSIZE to SetWindowPos
        LastWin.end = 2;

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
            LastWin.end = 0;
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
            SetWindowPlacement(state.hwnd, &wndpl);
            // Update wndwidth and wndheight
            wndwidth  = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
            wndheight = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
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

    wnd.left   = posx + mdiclientpt.x;
    wnd.top    = posy + mdiclientpt.y;
    wnd.right  = posx + mdiclientpt.x + wndwidth;
    wnd.bottom = posy + mdiclientpt.y + wndheight;

    if (!conf.FullWin) {
        if (!hdcc) {
            if (!hpenDot_Global)
                hpenDot_Global = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            hdcc = CreateDCA("DISPLAY", NULL, NULL, NULL);
            SetROP2(hdcc, R2_NOTXORPEN);
            SelectObject(hdcc, hpenDot_Global);
        }
        if (!state.moving || !EqualRect(&wnd, &oldRect)) {
            DrawRect(&wnd);
            if (state.moving == 1)
                DrawRect(&oldRect);
            CopyRect(&oldRect, &wnd); // oldRect is GLOBAL!
        }
        state.moving = 1;

        if (ShouldResizeTouching()) {
            ResizeTouchingWindows(posx, posy, wndwidth, wndheight);
        }

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
    if (state.action && conf.GrabWithAlt[0]) {
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
    if (!hwnd || blacklistedP(hwnd, &BlkLst.Pause))
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
///////////////////////////////////////////////////////////////////////////
// Keep this one minimalist, it is always on.
__declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || state.ignorectrl) return CallNextHookEx(NULL, nCode, wParam, lParam);

    unsigned char vkey = ((PKBDLLHOOKSTRUCT)lParam)->vkCode;
    if (vkey == VK_SCROLL) PostMessage(g_mainhwnd, WM_UPDATETRAY, 0, 0);
    if (ScrollLockState()) return CallNextHookEx(NULL, nCode, wParam, lParam);

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        if (!state.alt && (!conf.KeyCombo || (state.alt1 && state.alt1 != vkey)) && IsHotkey(vkey)) {
            // Update state && (GetKeyState(VK_SCROLL)&1)
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

        } else if (vkey == VK_LSHIFT || vkey == VK_RSHIFT) {
            if (!state.shift && vkey != conf.ModKey) {
                EnumOnce(NULL); // Reset enum state.
                state.snap = 3;
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
                enum action action = state.action;
                if (!conf.FullWin && state.moving) {
                    DrawRect(&oldRect);
                }
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
            HotkeyUp();
        } else if (vkey == VK_LSHIFT || vkey == VK_RSHIFT) {
            state.shift = 0;
            state.snap = conf.AutoSnap;
        } else if (vkey == VK_LCONTROL || vkey == VK_RCONTROL) {
            ClipCursorOnce(NULL); // Release cursor trapping
            state.ctrl = 0;
            // If there is no action then Control UP prevents AltDragging...
            if (!state.action) state.alt = 0;
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
    GetEnoughSpace((void **)&hwnds, &numhwnds, &hwnds_alloc, sizeof(HWND));

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
// Under Vista this will change the main volume with ole interface,
// Under NT4-XP, this will change the waveOut volume.
static void ActionVolume(int delta)
{
    static char HaveV=-1;
    if (!HaveV) return;
    if (HaveV == -1) {
        myCoInitialize = (void *)LoadDLLProc("OLE32.DLL", "CoInitialize");
        myCoUninitialize= (void *)LoadDLLProc("OLE32.DLL", "CoUninitialize");
        myCoCreateInstance= (void *)LoadDLLProc("OLE32.DLL", "CoCreateInstance");
        if (!myCoCreateInstance || !myCoUninitialize || !myCoInitialize) {
            FreeDLLByName("OLE32.DLL");
            HaveV = 2; // Fallback to WINMM
        } else {
            HaveV = 1;
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
        if (hr != S_OK) {
            myCoUninitialize();
            FreeDLLByName("OLE32.DLL");
            HaveV = 2; // Fallback to WINMM for next call
            return;
        }

        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pDevEnumerator, eRender, eMultimedia, &pDev);
        IMMDeviceEnumerator_Release(pDevEnumerator);
        if (hr != S_OK) return;

        hr = IMMDevice_Activate(pDev, &my_IID_IAudioEndpointVolume, CLSCTX_ALL
                                    , NULL, (void**)&pAudioEndpoint);
        IMMDevice_Release(pDev);
        if (hr != S_OK) return;

        // Function pointer so we only need one for-loop
        typedef HRESULT WINAPI (*_VolumeStep)(IAudioEndpointVolume*, LPCGUID pguidEventContext);
        _VolumeStep VolumeStep = (_VolumeStep)(pAudioEndpoint->lpVtbl->VolumeStepDown);
        if (delta > 0)
            VolumeStep = (_VolumeStep)(pAudioEndpoint->lpVtbl->VolumeStepUp);

        // Hold shift to make 5 steps
        UCHAR num = (state.shift)?5:1;
        while (num--) {
            hr = VolumeStep(pAudioEndpoint, NULL);
        }

        IAudioEndpointVolume_Release(pAudioEndpoint);
        myCoUninitialize();
    } else {
        if (HaveV == 2) {
            mywaveOutGetVolume = LoadDLLProc("WINMM.DLL", "waveOutGetVolume");
            mywaveOutSetVolume = LoadDLLProc("WINMM.DLL", "waveOutSetVolume");
            if (!mywaveOutSetVolume  || !mywaveOutGetVolume) {
                HaveV = 0; // Disable volume change
                FreeDLLByName("WINMM.DLL");
                return;
            }
            HaveV = 3; // WINMM API found!
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
}
/////////////////////////////////////////////////////////////////////////////
// Windows 2000+ Only
static int ActionTransparency(HWND hwnd, int delta)
{
    static int alpha=255;

    if (blacklisted(hwnd, &BlkLst.Windows)) return 0;
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
/////////////////////////////////////////////////////////////////////////////
// Roll/Unroll Window. If delta > 0: Roll if < 0: Unroll if == 0: Toggle.
static void RollWindow(HWND hwnd, int delta)
{
    RECT rc;
    state.hwnd = hwnd;
    state.origin.maximized = IsZoomed(state.hwnd);
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);

    unsigned restore = GetRestoreFlag(hwnd);

    if (restore & ROLLED && delta <= 0) { // UNROLL
        if (state.origin.maximized) {
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
            GetWindowPlacement(hwnd, &wndpl);
            wndpl.showCmd = SW_SHOWMINIMIZED;
            SetWindowPlacement(hwnd, &wndpl);
            wndpl.showCmd = SW_SHOWMAXIMIZED;
            SetWindowPlacement(hwnd, &wndpl);
        } else {
            RestoreOldWin(NULL, 2, 2);
        }
    } else if (((!(restore & ROLLED) && delta == 0)) || delta > 0 ) { // ROLL
        GetWindowRect(state.hwnd, &rc);
        SetWindowPos(state.hwnd, NULL, 0, 0, rc.right - rc.left
              , GetSystemMetrics(SM_CYMIN)
              , SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_ASYNCWINDOWPOS);
        if (!(restore & ROLLED)) { // Save window size if not saved already.
            if (!state.origin.maximized) {
                SetRestoreData(hwnd, rc.right - rc.left, rc.bottom - rc.top, 0);
            }
            // Add the SNAPPED falg is maximized and and add the SNTHENROLLED flag is snapped
            SetRestoreFlag(hwnd, ROLLED | state.origin.maximized|IsWindowSnapped(hwnd)<<10);
        }
    }
}
static int IsDoubleClick(int button)
{
    return state.clickbutton == button
        && GetTickCount()-state.clicktime <= GetDoubleClickTime();
}
/////////////////////////////////////////////////////////////////////////////
static int ActionMove(POINT pt, int button)
{
    // If this is a double-click
    if (IsDoubleClick(button)) {
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
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
static int ActionResize(POINT pt, const RECT *wnd, int button)
{
    if(!state.resizable) {
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
        RECT *mon = &state.origin.mon;
        RECT bd;
        FixDWMRect(state.hwnd, &bd);

        if (!state.shift ^ !(conf.AeroTopMaximizes&2)) {
        /* Extend window's borders to monitor */
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
                        ToggleMaxRestore(state.hwnd);
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

        MoveWindowAsync(state.hwnd, posx, posy, wndwidth, wndheight);
        // Save data to the window...
        SetRestoreData(state.hwnd, state.origin.width, state.origin.height, SNAPPED|restore);

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

    return 0;
}
/////////////////////////////////////////////////////////////////////////////
static void ActionBorderless(HWND hwnd)
{
    long style = GetWindowLongPtr(hwnd, GWL_STYLE);

    if (style&WS_BORDER) style &= state.shift? ~WS_CAPTION: ~(WS_CAPTION|WS_THICKFRAME);
    else style |= WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU;

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
    int fs = ((style&WS_CAPTION) != WS_CAPTION) && EqualRect(wnd, fmon);
    fs |= ((style&WS_SYSMENU) != WS_SYSMENU)<<1 ; // &2 for sysmenu.

    return fs; // = 1 for fulscreen, 2 for SYSMENU and 3 for both.
}
/////////////////////////////////////////////////////////////////////////////
static void CenterWindow(HWND hwnd)
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
        , state.origin.height);
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
        ToggleMaxRestore(hwnd);
    }
}
static void MaximizeHV(HWND hwnd, int horizontal)
{
    RECT rc, bd, mon;
    if (!IsResizable(hwnd) || !GetWindowRect(hwnd, &rc)) return;
    OffsetRectMDI(&rc);

    POINT pt;
    GetCursorPos(&pt);
    GetMonitorRect(&pt, 0, &mon);

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
BOOL CALLBACK MinimizeWindowProc(HWND hwnd, LPARAM hMon)
{
    if (hwnd != state.sclickhwnd
    && IsWindowVisible(hwnd) 
    && !IsWindowCloaked(hwnd)
    && !IsIconic(hwnd) 
    && (WS_MINIMIZEBOX&GetWindowLongPtr(hwnd, GWL_STYLE))){
        if (!hMon || (HMONITOR)hMon == MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)) {
            MinimizeWindow(hwnd);
        }
    }
    return TRUE;
}

static void MinimizeAllOtherWindows(HWND hwnd, int CurrentMonOnly)
{
    state.sclickhwnd = hwnd;
    HMONITOR hMon = NULL;
    if (CurrentMonOnly)  hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    EnumDesktopWindows(NULL, MinimizeWindowProc, (LPARAM)hMon);
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
    else if (action==AC_LOWER)       ActionLower(hwnd, 0, state.shift);
    else if (action==AC_BORDERLESS)  ActionBorderless(hwnd);
    else if (action==AC_KILL)        ActionKill(hwnd);
    else if (action==AC_ROLL)        RollWindow(hwnd, 0);
    else if (action==AC_MAXHV)       MaximizeHV(hwnd, state.shift);
    else if (action==AC_MINALL)      MinimizeAllOtherWindows(hwnd, state.shift);
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
    if (!state.hwnd || state.hwnd == LastWin.hwnd) {
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

    // Get MDI chlild hwnd or root hwnd if not MDI!
    state.hwnd = MDIorNOT(state.hwnd, &state.mdiclient);

    // mdiclientpt HAS to be set to Zero for ClientToScreen adds the offset
    mdiclientpt = (POINT) { 0, 0 };
    if (state.mdiclient) {
        GetMDInfo(&mdiclientpt, &fmon);
        CopyRect(&state.origin.mon, &fmon);
    }

    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    // A full screen window has No caption and is to monitor size.

    // Return if window is blacklisted,
    // if we can't get information about it,
    // or if the window is fullscreen and has no sysmenu nor caption.
    if (!state.hwnd
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
    // return if window as to be moved/resized and does not respond in 1/4 s.
    if (MOUVEMENT(action)
    && !SendMessageTimeout(state.hwnd, 0, 0, 0, SMTO_NORMAL, 255, &lpdwResult)) {
        state.blockmouseup = 1;
        return 1; // Unresponsive window...
    }

    state.origin.maximized = IsZoomed(state.hwnd);
    state.origin.monitor = MonitorFromWindow(state.hwnd, MONITOR_DEFAULTTONEAREST);

    if (!state.snap) {
        state.snap = conf.AutoSnap;
    }

    // Set Origin width and height needed for AC_MOVE/RESIZE/CENTER/MAXHV
    int rwidth=0, rheight=0;
    unsigned rdata_flag = GetRestoreData(state.hwnd, &rwidth, &rheight);
    // Clear snapping info if asked.
    if (rdata_flag&SNCLEAR || conf.SmartAero&4){ ClearRestoreData(state.hwnd); rdata_flag=0; }

    if (rdata_flag && !state.origin.maximized) {
        state.origin.width = rwidth;
        state.origin.height = rheight;
    } else {
        state.origin.width = wndpl.rcNormalPosition.right-wndpl.rcNormalPosition.left;
        state.origin.height = wndpl.rcNormalPosition.bottom-wndpl.rcNormalPosition.top;
    }

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
        if (ret == 1) return 1; // block mouse down!
        UpdateCursor(pt);

        // Send WM_ENTERSIZEMOVE
        SendSizeMove(WM_ENTERSIZEMOVE);
    } else if (action == AC_MENU) {
        state.sclickhwnd = state.hwnd;
        PostMessage(g_mainhwnd, WM_SCLICK, (WPARAM)g_mchwnd, conf.AggressiveKill);
        return 1; // block mouse down
    } else {
        SClickActions(state.hwnd, action);
        state.blockmouseup = 1;
    }

    // We have to send the ctrl keys here too because of
    // IE (and maybe some other program?)
    Send_CTRL();

    // Remember time, position and button of this click
    // so we can check for double-click
    state.clicktime = GetTickCount();
    state.clickpt = pt;
    state.clickbutton = button;

    // Prevent mousedown from propagating
    return 1;
}
static int IsAreaCaption(int area)
{
    return area == HTCAPTION
       || (area >= HTTOP && area <= HTTOPRIGHT)
       || area == HTSYSMENU || area == HTMINBUTTON || area == HTMAXBUTTON
       || area == HTCLOSE || area == HTHELP;
}
/////////////////////////////////////////////////////////////////////////////
// Lower window if middle mouse button is used on the title bar/top of the win
// Or restore an AltDrad Aero-snapped window.
static int ActionNoAlt(POINT pt, WPARAM wParam)
{
    int willlower = ((conf.LowerWithMMB&1) && !state.alt)
                 || ((conf.LowerWithMMB&2) &&  state.alt);
    if ((willlower || conf.NormRestore)
    &&  !state.action
    && (wParam == WM_MBUTTONDOWN || wParam == WM_LBUTTONDOWN)) {
        HWND nhwnd = WindowFromPoint(pt);
        if (!nhwnd) return 0;
        HWND hwnd = MDIorNOT(nhwnd, &state.mdiclient);
        if (blacklisted(hwnd, &BlkLst.Windows)) return 0; // Next hook

        int area = HitTestTimeoutbl(nhwnd, pt.x, pt.y);

        if (willlower && wParam == WM_MBUTTONDOWN && IsAreaCaption(area)) {
            ActionLower(hwnd, 0, state.shift);
            return 1;
        } else if (conf.NormRestore
        && wParam == WM_LBUTTONDOWN && area == HTCAPTION
        && !IsZoomed(hwnd) && !IsWindowSnapped(hwnd)) {
            if (GetRestoreFlag(hwnd)) {
                // Set NormRestore to 2 in order to signal that
                // The window should be restored
                conf.NormRestore=2;
                state.hwnd = hwnd;
                state.origin.maximized=0;
            }
        }
    } else if (wParam == WM_LBUTTONUP) {
        conf.NormRestore = !!conf.NormRestore;
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
    } else if (!state.alt || state.action != conf.GrabWithAlt[ModKey()]
          || (conf.GrabWithAlt[ModKey()] && !IsSamePTT(&pt, &state.clickpt))
          || (!IsHotkeyDown() && !IsHotclick(state.alt))) {
        return 0; // continue if no actions to be made
    }

    // Get pointed window
    HideCursor();
    HWND nhwnd = WindowFromPoint(pt);
    if (!nhwnd) return 0;
    HWND hwnd = MDIorNOT(nhwnd, &state.mdiclient);

    if (conf.RollWithTBScroll && wParam == WM_MOUSEWHEEL && !state.ctrl) {

        int area= HitTestTimeoutbl(nhwnd, pt.x, pt.y);
        if (IsAreaCaption(area)) {
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
        if ((IsFullscreen(hwnd, &wnd, &mon)&conf.FullScreen) == conf.FullScreen)
            return 0;
    }
    int ret=1;
    enum action action = (wParam == WM_MOUSEWHEEL)? conf.Mouse.Scroll[ModKey()]: conf.Mouse.HScroll[ModKey()];

    if      (action == AC_ALTTAB)       ret = ActionAltTab(pt, delta);
    else if (action == AC_VOLUME)       ActionVolume(delta);
    else if (action == AC_TRANSPARENCY) ret = ActionTransparency(hwnd, delta);
    else if (action == AC_LOWER)        ActionLower(hwnd, delta, state.shift);
    else if (action == AC_MAXIMIZE)     ActionMaxRestMin(hwnd, delta);
    else if (action == AC_ROLL)         RollWindow(hwnd, delta);
    else if (action == AC_HSCROLL)      ret = ScrollPointedWindow(pt, -delta, WM_MOUSEHWHEEL);
    else                                ret = 0; // No action

    // ret is 0: next hook or 1: block mousedown and AltUp.
    state.blockaltup = ret; // block or not;
    return ret; // block or next hook
}
/////////////////////////////////////////////////////////////////////////////
// Called on MouseUp and on AltUp when using GrabWithAlt
static void FinishMovement()
{
    StopSpeedMes();
    if (LastWin.hwnd 
    && (state.moving == NOT_MOVED || (!conf.FullWin && state.moving == 1))) {
        // to erase the last rectangle...
        if (!conf.FullWin) {
            DrawRect(&oldRect);
            if (state.action == AC_RESIZE) ResizeAllSnappedWindowsAsync();
        }
        if (IsWindow(LastWin.hwnd)){
            if (LastWin.maximize) {
                Maximize_Restore_atpt(LastWin.hwnd, NULL, SW_MAXIMIZE, NULL);
                LastWin.hwnd = NULL;
            } else {
                LastWin.end |= 1;
                MoveWindowInThread(&LastWin);
            }
        }
    }
    // Auto Remaximize if option enabled and conditions are met.
    if (conf.AutoRemaximize && state.moving
    && (state.origin.maximized || state.origin.fullscreen)
    && !state.shift && !state.mdiclient && state.action == AC_MOVE) {
        state.action = AC_NONE;
        POINT pt;
        GetCursorPos(&pt);
        HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        if (monitor != state.origin.monitor) {
            Sleep(10);  // Wait a little for moveThread.
            if (LastWin.hwnd) Sleep(100); // Wait more...

            if (state.origin.maximized) {
                Maximize_Restore_atpt(state.hwnd, NULL, SW_MAXIMIZE, monitor);
            }
            if (state.origin.fullscreen) {
                Maximize_Restore_atpt(state.hwnd, NULL, SW_FULLSCREEN, monitor);
            }
        }
    }
    // Send WM_EXITSIZEMOVE
    SendSizeMove(WM_EXITSIZEMOVE);

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
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || ScrollLockState())
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

    enum buttonstate buttonstate =
          (wParam==WM_LBUTTONDOWN||wParam==WM_MBUTTONDOWN
        || wParam==WM_RBUTTONDOWN||wParam==WM_XBUTTONDOWN)? STATE_DOWN:
          (wParam==WM_LBUTTONUP  ||wParam==WM_MBUTTONUP
        || wParam==WM_RBUTTONUP  ||wParam==WM_XBUTTONUP)?STATE_UP:STATE_NONE;

    enum action action = GetAction(button);

    // Check if the click is is a Hotclick and should enable ALT.
    int is_hotclick = IsHotClickPt(button, pt, buttonstate);
    if (is_hotclick && buttonstate == STATE_DOWN) {
        state.alt = button;
        if (!action) return 1;
    } else if (is_hotclick && buttonstate == STATE_UP) {
        state.alt = 0;
    }

    // Return if no mouse action will be started
    if (!action) return CallNextHookEx(NULL, nCode, wParam, lParam);

    // Handle another click if we are already busy with an action
    if (buttonstate == STATE_DOWN && state.action && state.action != conf.GrabWithAlt[ModKey()]) {
        // Maximize/Restore the window if pressing Move, Resize mouse buttons.
        if ((conf.MMMaximize&1) && state.action == AC_MOVE && action == AC_RESIZE) {
            if (LastWin.hwnd) Sleep(10);
            if (IsZoomed(state.hwnd)) {
                state.moving = 0;
                MouseMove(pt);
            } else if (state.resizable) {
                state.moving = CURSOR_ONLY; // So that MouseMove will only move g_mainhwnd
                Maximize_Restore_atpt(state.hwnd, &pt, SW_MAXIMIZE, NULL);
            }
            state.blockmouseup = 1;
        }
        return 1; // Block mousedown so AltDrag.exe does not remove g_mainhwnd

    // INIT ACTIONS on mouse down if Alt is down...
    } else if (buttonstate == STATE_DOWN && state.alt) {
        // Double ckeck some hotkey is pressed.
        if (!state.action
        && !IsHotClickPt(state.alt, pt, buttonstate)
        && !IsHotkeyDown()) {
            UnhookMouse();
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }
        ret = init_movement_and_actions(pt, action, button);
        if (!ret) return CallNextHookEx(NULL, nCode, wParam, lParam);
        else      return 1; // block mousedown

    // BUTTON UP
    } else if (buttonstate == STATE_UP) {
        SetWindowTrans(NULL); // Reset window transparency
        if (state.blockmouseup) {
            state.blockmouseup = 0;
            return 1;
//        } else if (action == AC_MOVE && IsSamePTT(&pt, &state.clickpt)) {
//            FinishMovement();
//            MinimizeWindow(state.hwnd);
//            return 1;

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
LRESULT CALLBACK TimerWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TIMER) {
        if (wParam == REHOOK_TIMER) {
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
            if (state.moving) state.Speed=max(abs(oldpt.x-state.prevpt.x), abs(oldpt.y-state.prevpt.y));
            else state.Speed=0;
            oldpt = state.prevpt;
            if (state.moving && state.Speed == 0 && !has_moved_to_fixed_pt && !MM_THREAD_ON) {
                has_moved_to_fixed_pt = 1;
                MouseMove(state.prevpt);
            }
            if (state.Speed) has_moved_to_fixed_pt = 0;
        }
    } else if (msg == WM_DESTROY) {
        KillTimer(g_timerhwnd, REHOOK_TIMER);
        KillTimer(g_timerhwnd, SPEED_TIMER);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
// Window for single click commands
LRESULT CALLBACK SClickWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        free(list->items);
        list++;
    }
}
/////////////////////////////////////////////////////////////////////////////
// To be called before Free Library. Ideally it should free everything
__declspec(dllexport) void Unload()
{
    DeleteDCPEN();

    conf.keepMousehook = 0;
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

    blacklist->data = NULL;
    blacklist->length = 0;
    blacklist->items = NULL;

    DWORD ret = GetPrivateProfileString(L"Blacklist", blacklist_str, L"", txt, ARR_SZ(txt), inipath);
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
    wchar_t txt[32];
    wchar_t inipath[MAX_PATH];
    state.action = AC_NONE;
    state.shift = 0;
    state.moving = 0;
    LastWin.hwnd = NULL;

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
    conf.TitlebarMove  = GetPrivateProfileInt(L"Advanced", L"TitlebarMove", 0, inipath);
    if (conf.TitlebarMove) conf.NormRestore = 0; // in this case disable NormRestore
    conf.SnapGap       = CLAMP(-128, GetPrivateProfileInt(L"Advanced", L"SnapGap", 0, inipath), 127);

    // [Performance]
    conf.MoveRate  = GetPrivateProfileInt(L"Performance", L"MoveRate", 1, inipath);
    conf.ResizeRate= GetPrivateProfileInt(L"Performance", L"ResizeRate", 2, inipath);
    conf.FullWin   = GetPrivateProfileInt(L"Performance", L"FullWin", 1, inipath);
    conf.RefreshRate=GetPrivateProfileInt(L"Performance", L"RefreshRate", 0, inipath);

    // [Input]
    struct {
        wchar_t *key;
        wchar_t *def;
        enum action *ptr;
    } buttons[] = {
        {L"LMB",        L"Move",    &conf.Mouse.LMB[0]},
        {L"MMB",        L"Maximize",&conf.Mouse.MMB[0]},
        {L"RMB",        L"Resize",  &conf.Mouse.RMB[0]},
        {L"MB4",        L"Nothing", &conf.Mouse.MB4[0]},
        {L"MB5",        L"Nothing", &conf.Mouse.MB5[0]},
        {L"Scroll",     L"Nothing", &conf.Mouse.Scroll[0]},
        {L"HScroll",    L"Nothing", &conf.Mouse.HScroll[0]},
        {L"GrabWithAlt",L"Nothing", &conf.GrabWithAlt[0]},

        {L"LMBB",       L"Resize",  &conf.Mouse.LMB[1]},
        {L"MMBB",       L"Maximize",&conf.Mouse.MMB[1]},
        {L"RMBB",       L"Move",    &conf.Mouse.RMB[1]},
        {L"MB4B",       L"Nothing", &conf.Mouse.MB4[1]},
        {L"MB5B",       L"Nothing", &conf.Mouse.MB5[1]},
        {L"ScrollB",    L"Volume",  &conf.Mouse.Scroll[1]},
        {L"HScrollB",   L"Nothing", &conf.Mouse.HScroll[1]},
        {L"GrabWithAltB",L"Nothing", &conf.GrabWithAlt[1]},
        {NULL}
    };
    unsigned i;
    UCHAR action_menu_load = 0;
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
        else if (!wcsicmp(txt,L"MaximizeHV"))   *buttons[i].ptr = AC_MAXHV;
        else if (!wcsicmp(txt,L"MinAllOther"))  *buttons[i].ptr = AC_MINALL;
        else                                    *buttons[i].ptr = AC_NONE;
    }

    conf.LowerWithMMB    = GetPrivateProfileInt(L"Input", L"LowerWithMMB",    0, inipath);
    conf.AggressivePause = GetPrivateProfileInt(L"Input", L"AggressivePause", 0, inipath);
    conf.AggressiveKill  = GetPrivateProfileInt(L"Input", L"AggressiveKill",  0, inipath);
    conf.RollWithTBScroll= GetPrivateProfileInt(L"Input", L"RollWithTBScroll",0, inipath);
    conf.KeyCombo        = GetPrivateProfileInt(L"Input", L"KeyCombo",        0, inipath);
    conf.ScrollLockState = GetPrivateProfileInt(L"Input", L"ScrollLockState", 0, inipath);

    readhotkeys(inipath, L"Hotkeys",  L"A4 A5",   conf.Hotkeys);
    readhotkeys(inipath, L"Hotclicks",L"",        conf.Hotclick);
    readhotkeys(inipath, L"Killkeys", L"09 4C 2E",conf.Killkey);

    conf.ModKey     = readsinglekey(inipath, L"ModKey", L"");
    conf.HScrollKey = readsinglekey(inipath, L"HScrollKey", L"10"); // VK_SHIFT

    readblacklist(inipath, &BlkLst.Processes, L"Processes");
    readblacklist(inipath, &BlkLst.Windows,   L"Windows");
    readblacklist(inipath, &BlkLst.Snaplist,  L"Snaplist");
    readblacklist(inipath, &BlkLst.MDIs,      L"MDIs");
    readblacklist(inipath, &BlkLst.Pause,     L"Pause");
    readblacklist(inipath, &BlkLst.MMBLower,  L"MMBLower");
    readblacklist(inipath, &BlkLst.Scroll,    L"Scroll");
    readblacklist(inipath, &BlkLst.AResize,   L"AResize");
    readblacklist(inipath, &BlkLst.SSizeMove, L"SSizeMove");
    readblacklist(inipath, &BlkLst.NCHittest, L"NCHittest");
    ResetDB(); // Zero database

    // Zones
    conf.UseZones  = GetPrivateProfileInt(L"Zones", L"UseZones", 0, inipath);
    if (conf.UseZones) ReadZones(inipath);
    conf.InterZone = GetPrivateProfileInt(L"Zones", L"InterZone", 0, inipath);
  # ifdef WIN64
    conf.FancyZone = GetPrivateProfileInt(L"Zones", L"FancyZone", 0, inipath);
  # endif

    conf.keepMousehook = ((conf.LowerWithMMB&1) || conf.NormRestore || conf.TitlebarMove
                         || conf.InactiveScroll || conf.Hotclick[0]);
        // Capture main hwnd from caller. This is also the cursor wnd
    g_mainhwnd = mainhwnd;
    
    if (conf.keepMousehook || conf.AeroMaxSpeed < 65535) {
        g_timerhwnd = KreateMsgWin(TimerWindowProc, APP_NAME"-Timers");
    }
    if (conf.keepMousehook) {
        HookMouse();
        SetTimer(g_timerhwnd, REHOOK_TIMER, 5000, NULL); // Start rehook timer
    }
    // Window for Action Menu
    if (action_menu_load) {
        g_mchwnd = KreateMsgWin(SClickWindowProc, APP_NAME"-SClick");
    }

    // Hook mouse if a permanent hook is needed
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
