/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * PeekDesktop integration for AltSnap                                    *
 * Click empty desktop wallpaper to minimize all windows,                 *
 * click again or switch app to restore.                                  *
 * Ported from PeekDesktop by Scott Hanselman (MIT License).              *
 * General Public License Version 3 or later (Free Software Foundation)   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This file is #included by hooks.c */

/* Forward declarations */
void PeekRestoreAll(void);
void PeekCaptureAndHide(void);
void PeekCleanup(void);

/* Peek flags bitmask */
#define PEEK_F_DBLCLICK       1  /* bit0: require double-click */
#define PEEK_F_TASKBAR        2  /* bit1: peek on empty taskbar */
#define PEEK_F_GAMINGSUPPRESS 4  /* bit2: pause while gaming (default on) */
#define PEEK_F_RESTOREONAPP   8  /* bit3: restore on app switch (default on) */

/* Peek modes */
#define PEEK_MODE_MINIMIZE 0
#define PEEK_MODE_FLYAWAY  1

#define PEEK_GRACE_MS      200
#define PEEK_FLYAWAY_STEPS 12
#define PEEK_FLYAWAY_MS    160

/* LVHITTESTINFO for cross-process desktop icon detection */
typedef struct {
    POINT pt;
    UINT flags;
    int iItem;
    int iSubItem;
    int iGroup;
} PEEK_LVHITTESTINFO;
#define PEEK_LVM_HITTEST 0x1009

/* Saved window entry */
struct peek_wnd { HWND hwnd; WINDOWPLACEMENT wp; RECT bounds; };
typedef struct { struct peek_wnd *it; size_t count; size_t cap; } PeekWndList;

/* Peek state */
static struct {
    int is_peeking;
    int is_transitioning;
    DWORD peek_time;
    DWORD ignore_restore_until;
    HWINEVENTHOOK focus_hook;
    /* Pending click for double-click and drag detection */
    POINT down_pt;
    DWORD down_time;
    int has_pending;
    int was_first_click;  /* for double-click: is this the 2nd click? */
    PeekWndList wnds;
} peek;

/////////////////////////////////////////////////////////////////////////////
// Desktop detection helpers

static BOOL IsDesktopRelatedWindow(HWND hwnd)
{
    TCHAR cn[64];
    HWND cur = hwnd;
    while (cur) {
        GetClassName(cur, cn, ARR_SZ(cn));
        if (lstrcmp(cn, TEXT("Progman")) == 0) return TRUE;
        if (lstrcmp(cn, TEXT("WorkerW")) == 0) {
            if (FindWindowEx(cur, NULL, TEXT("SHELLDLL_DefView"), NULL))
                return TRUE;
        }
        cur = GetParent(cur);
    }
    return FALSE;
}

static HWND FindDesktopListView(void)
{
    HWND progman = FindWindow(TEXT("Progman"), NULL);
    if (!progman) return NULL;
    HWND sv = FindWindowEx(progman, NULL, TEXT("SHELLDLL_DefView"), NULL);
    if (!sv) {
        HWND ww = NULL;
        while ((ww = FindWindowEx(NULL, ww, TEXT("WorkerW"), NULL))) {
            if (FindWindowEx(ww, NULL, TEXT("SHELLDLL_DefView"), NULL)) {
                sv = FindWindowEx(ww, NULL, TEXT("SHELLDLL_DefView"), NULL);
                break;
            }
        }
    }
    if (!sv) return NULL;
    return FindWindowEx(sv, NULL, TEXT("SysListView32"), NULL);
}

static BOOL IsDesktopIcon(POINT pt)
{
    HWND lv = FindDesktopListView();
    if (!lv) return FALSE;
    DWORD pid;
    GetWindowThreadProcessId(lv, &pid);
    HANDLE proc = OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, pid);
    if (!proc) return FALSE;
    PEEK_LVHITTESTINFO *remote = (PEEK_LVHITTESTINFO *)VirtualAllocEx(
        proc, NULL, sizeof(PEEK_LVHITTESTINFO), MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { CloseHandle(proc); return FALSE; }
    PEEK_LVHITTESTINFO local;
    mem00(&local, sizeof(local));
    local.pt = pt;
    WriteProcessMemory(proc, remote, &local, sizeof(local), NULL);
    LRESULT hit;
    SendMessageTimeout(lv, PEEK_LVM_HITTEST, 0, (LPARAM)remote,
                       SMTO_ABORTIFHUNG, 500, (PDWORD_PTR)&hit);
    ReadProcessMemory(proc, remote, &local, sizeof(local), NULL);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);
    return (local.iItem >= 0);
}

static BOOL IsDesktopBackground(POINT pt)
{
    HWND hwnd = WindowFromPoint(pt);
    if (!hwnd) return FALSE;
    return IsDesktopRelatedWindow(hwnd);
}

static HWND RealChildWindowFromPointL(HWND parent, POINT pt)
{
    typedef HWND (WINAPI *funk_t)(HWND, POINT);
    static funk_t pRCFP;
    static int tried;
    if (!tried) { tried = 1; pRCFP = (funk_t)GetProcAddress(GetModuleHandleA("user32"), "RealChildWindowFromPoint"); }
    if (pRCFP) return pRCFP(parent, pt);
    return NULL;
}

static BOOL IsTaskbarBlank(POINT pt)
{
    /* Strategy 1: check if click hits a taskbar window */
    HWND hwnd = WindowFromPoint(pt);
    if (hwnd) {
        TCHAR cn[64];
        HWND cur = hwnd;
        while (cur) {
            GetClassName(cur, cn, ARR_SZ(cn));
            if (lstrcmp(cn, TEXT("Shell_TrayWnd")) == 0
            ||  lstrcmp(cn, TEXT("Shell_SecondaryTrayWnd")) == 0)
                break;
            cur = GetParent(cur);
        }
        if (cur) {
            POINT cp = pt;
            ScreenToClient(cur, &cp);
            HWND child = ChildWindowFromPointEx(cur, cp,
                CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
            if (!child || child == cur)
                child = RealChildWindowFromPointL(cur, cp);
            if (!child || child == cur)
                return TRUE;
            /* Skip transparent composition overlay windows */
            GetClassName(child, cn, ARR_SZ(cn));
            if (lstrcmp(cn, TEXT("Windows.UI.Composition.DesktopWindowContentBridge")) == 0)
                return TRUE;
        }
    }

    /* Strategy 2: fallback - check if point is in monitor but outside work area (taskbar zone) */
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (mon) {
        MONITORINFO mi; mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(mon, &mi)) {
            if (pt.x >= mi.rcMonitor.left  && pt.x < mi.rcMonitor.right
            &&  pt.y >= mi.rcMonitor.top   && pt.y < mi.rcMonitor.bottom
            && (pt.x < mi.rcWork.left  || pt.x >= mi.rcWork.right
             || pt.y < mi.rcWork.top   || pt.y >= mi.rcWork.bottom))
                return TRUE;
        }
    }
    return FALSE;
}

/* Windows that should not trigger restore-on-focus-change */
static BOOL IsPeekDesktopWindow(HWND hwnd)
{
    if (!hwnd) return TRUE;
    TCHAR cn[64];
    GetClassName(hwnd, cn, ARR_SZ(cn));
    if (lstrcmp(cn, TEXT("Progman")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("WorkerW")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("Shell_TrayWnd")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("Shell_SecondaryTrayWnd")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("#32768")) == 0) return TRUE;       /* menu */
    if (lstrcmp(cn, TEXT("tooltips_class32")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("NotifyIconOverflowWindow")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("DV2ControlHost")) == 0) return TRUE;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Gaming suppression

static BOOL IsFullscreenApp(void)
{
    HWND fg = GetForegroundWindow();
    if (!fg) return FALSE;
    /* Exclude system and desktop windows */
    TCHAR cn[64];
    GetClassName(fg, cn, ARR_SZ(cn));
    if (lstrcmp(cn, TEXT("Progman")) == 0) return FALSE;
    if (lstrcmp(cn, TEXT("WorkerW")) == 0) return FALSE;
    if (lstrcmp(cn, TEXT("Shell_TrayWnd")) == 0) return FALSE;
    /* Check if foreground window covers the entire monitor */
    RECT rc;
    if (!GetWindowRect(fg, &rc)) return FALSE;
    MONITORINFO mi; mi.cbSize = sizeof(mi);
    HMONITOR mon = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
    if (!GetMonitorInfoW(mon, &mi)) return FALSE;
    return (rc.left   <= mi.rcMonitor.left  + 2
         && rc.top    <= mi.rcMonitor.top    + 2
         && rc.right  >= mi.rcMonitor.right  - 2
         && rc.bottom >= mi.rcMonitor.bottom - 2);
}

/////////////////////////////////////////////////////////////////////////////
// Window enumeration

static BOOL CALLBACK EnumPeekWindows(HWND hwnd, LPARAM lParam)
{
    if (!IsVisible(hwnd)) return TRUE;
    if (IsIconic(hwnd)) return TRUE;
    if (GetWindow(hwnd, GW_OWNER)) return TRUE;

    /* Skip system windows */
    TCHAR cn[64];
    GetClassName(hwnd, cn, ARR_SZ(cn));
    if (lstrcmp(cn, TEXT("Progman")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("WorkerW")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("Shell_TrayWnd")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("Shell_SecondaryTrayWnd")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("NotifyIconOverflowWindow")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("DV2ControlHost")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("Windows.UI.Core.CoreWindow")) == 0) return TRUE;
    if (lstrcmp(cn, TEXT("ApplicationFrameWindow")) == 0) return TRUE;

    /* Skip tool windows and no-activate windows */
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (ex & WS_EX_NOACTIVATE) return TRUE;
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return TRUE;

    struct peek_wnd pw;
    pw.wp.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(hwnd, &pw.wp)) return TRUE;
    GetWindowRect(hwnd, &pw.bounds);
    pw.hwnd = hwnd;
    ListAppend(&peek.wnds, &pw, sizeof(pw));
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// FlyAway animation

static void EaseOutRect(RECT *out, const RECT *start, const RECT *end, float t)
{
    /* EaseOutCubic: 1 - (1-t)^3 */
    float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    out->left   = (long)(start->left   + (end->left   - start->left)   * e);
    out->top    = (long)(start->top    + (end->top    - start->top)    * e);
    out->right  = (long)(start->right  + (end->right  - start->right)  * e);
    out->bottom = (long)(start->bottom + (end->bottom - start->bottom) * e);
}

static void FlyAwayAll(void)
{
    if (peek.wnds.count == 0) return;

    /* Calculate fly-away targets: slide toward nearest screen edge */
    RECT *targets = (RECT *)malloc(sizeof(RECT) * peek.wnds.count);
    if (!targets) return;

    for (size_t i = 0; i < peek.wnds.count; i++) {
        RECT *b = &peek.wnds.it[i].bounds;
        targets[i] = *b;
        MONITORINFO mi; mi.cbSize = sizeof(mi);
        HMONITOR mon = MonitorFromRect(b, MONITOR_DEFAULTTONEAREST);
        if (GetMonitorInfoW(mon, &mi)) {
            int cx = (b->left + b->right) / 2;
            int cy = (b->top + b->bottom) / 2;
            int mcx = (mi.rcMonitor.left + mi.rcMonitor.right) / 2;
            int mcy = (mi.rcMonitor.top + mi.rcMonitor.bottom) / 2;
            int w = b->right - b->left;
            /* Fly toward nearest corner */
            if (cx < mcx && cy < mcy) {
                /* top-left corner */
                targets[i].left   = mi.rcMonitor.left - w - 64;
                targets[i].right  = mi.rcMonitor.left - 64;
            } else if (cx >= mcx && cy < mcy) {
                /* top-right corner */
                targets[i].right  = mi.rcMonitor.right + w + 64;
                targets[i].left   = mi.rcMonitor.right + 64;
            } else if (cx < mcx && cy >= mcy) {
                /* bottom-left corner */
                targets[i].left   = mi.rcMonitor.left - w - 64;
                targets[i].right  = mi.rcMonitor.left - 64;
            } else {
                /* bottom-right corner */
                targets[i].right  = mi.rcMonitor.right + w + 64;
                targets[i].left   = mi.rcMonitor.right + 64;
            }
        }
    }

    /* Animate */
    DWORD step_ms = PEEK_FLYAWAY_MS / PEEK_FLYAWAY_STEPS;
    for (int step = 1; step <= PEEK_FLYAWAY_STEPS; step++) {
        float t = (float)step / PEEK_FLYAWAY_STEPS;
        for (size_t i = 0; i < peek.wnds.count; i++) {
            if (!IsWindow(peek.wnds.it[i].hwnd)) continue;
            RECT cur;
            EaseOutRect(&cur, &peek.wnds.it[i].bounds, &targets[i], t);
            SetWindowPos(peek.wnds.it[i].hwnd, NULL,
                cur.left, cur.top, cur.right - cur.left, cur.bottom - cur.top,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_ASYNCWINDOWPOS);
        }
        Sleep(step_ms);
    }

    free(targets);
}

static void FlyBackAll(void)
{
    if (peek.wnds.count == 0) return;

    DWORD step_ms = PEEK_FLYAWAY_MS / PEEK_FLYAWAY_STEPS;
    for (int step = 1; step <= PEEK_FLYAWAY_STEPS; step++) {
        float t = (float)step / PEEK_FLYAWAY_STEPS;
        for (size_t i = 0; i < peek.wnds.count; i++) {
            if (!IsWindow(peek.wnds.it[i].hwnd)) continue;
            RECT cur;
            GetWindowRect(peek.wnds.it[i].hwnd, &cur);
            RECT target;
            EaseOutRect(&target, &cur, &peek.wnds.it[i].bounds, t);
            SetWindowPos(peek.wnds.it[i].hwnd, NULL,
                target.left, target.top, target.right - target.left, target.bottom - target.top,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_ASYNCWINDOWPOS);
        }
        Sleep(step_ms);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Focus watcher (restore on app switch)

static void CALLBACK PeekWinEventProc(
    HWINEVENTHOOK hWinEventHook, DWORD event,
    HWND hwnd, LONG idObject, LONG idChild,
    DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (peek.is_transitioning || !peek.is_peeking) return;
    if (!(conf.PeekFlags & PEEK_F_RESTOREONAPP)) return;
    DWORD elapsed = GetTickCount() - peek.peek_time;
    if (elapsed < PEEK_GRACE_MS) return;
    if (IsPeekDesktopWindow(hwnd)) return;
    PeekRestoreAll();
}

static void PeekInstallFocusHook(void)
{
    if (peek.focus_hook) return;
    peek.focus_hook = SetWinEventHookL(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        NULL, PeekWinEventProc,
        0, 0, WINEVENT_OUTOFCONTEXT);
}

static void PeekRemoveFocusHook(void)
{
    if (peek.focus_hook) {
        UnhookWinEventL(peek.focus_hook);
        peek.focus_hook = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Core peek/restore

void PeekCaptureAndHide(void)
{
    if (peek.is_transitioning) return;
    peek.is_transitioning = 1;
    peek.wnds.count = 0;

    EnumDesktopWindows(NULL, EnumPeekWindows, 0);

    if (peek.wnds.count == 0) {
        peek.is_transitioning = 0;
        return;
    }

    if (conf.PeekMode == PEEK_MODE_FLYAWAY) {
        FlyAwayAll();
        /* Also minimize to ensure they're properly hidden */
        for (size_t i = 0; i < peek.wnds.count; i++)
            ShowWindow(peek.wnds.it[i].hwnd, SW_MINIMIZE);
    } else {
        /* Minimize mode */
        for (size_t i = 0; i < peek.wnds.count; i++)
            ShowWindow(peek.wnds.it[i].hwnd, SW_MINIMIZE);
    }

    peek.is_peeking = 1;
    peek.peek_time = GetTickCount();
    peek.ignore_restore_until = GetTickCount() + PEEK_GRACE_MS + 100;

    if (conf.PeekFlags & PEEK_F_RESTOREONAPP)
        PeekInstallFocusHook();

    peek.is_transitioning = 0;
}

void PeekRestoreAll(void)
{
    if (peek.is_transitioning) return;
    peek.is_transitioning = 1;

    PeekRemoveFocusHook();

    if (conf.PeekMode == PEEK_MODE_FLYAWAY) {
        /* First restore windows from minimized state */
        for (size_t i = peek.wnds.count; i > 0; i--) {
            if (IsWindow(peek.wnds.it[i-1].hwnd))
                ShowWindow(peek.wnds.it[i-1].hwnd, SW_SHOWNOACTIVATE);
        }
        FlyBackAll();
    }

    /* Restore placement in reverse order to preserve Z-order */
    for (size_t i = peek.wnds.count; i > 0; i--) {
        if (IsWindow(peek.wnds.it[i-1].hwnd))
            SetWindowPlacement(peek.wnds.it[i-1].hwnd, &peek.wnds.it[i-1].wp);
    }

    peek.is_peeking = 0;
    peek.is_transitioning = 0;
    peek.wnds.count = 0;
}

void PeekCleanup(void)
{
    if (peek.is_peeking)
        PeekRestoreAll();
    PeekRemoveFocusHook();
    ListFree(&peek.wnds);
    mem00(&peek, sizeof(peek));
}

/////////////////////////////////////////////////////////////////////////////
// Mouse event handler - called from LowLevelMouseProc

static int PeekHandleMouseEvent(WPARAM wParam, MSLLHOOKSTRUCT *mhk)
{
    if (!conf.PeekDesktop) return 0;
    /* Don't interfere with AltSnap's regular action system */
    if (state.alt || state.action.ac) return 0;

    POINT pt = mhk->pt;

    /* --- LBUTTONDOWN: record press position --- */
    if (wParam == WM_LBUTTONDOWN) {
        if (peek.is_peeking) {
            /* Click while peeking: restore on LBUTTONUP (see below) */
            peek.down_pt = pt;
            peek.has_pending = 1;
            return 0;
        }

        /* Gaming suppression */
        if ((conf.PeekFlags & PEEK_F_GAMINGSUPPRESS) && IsFullscreenApp())
            return 0;

        /* Record down position for drag/click classification */
        peek.down_pt = pt;
        peek.down_time = GetTickCount();

        if (conf.PeekFlags & PEEK_F_DBLCLICK) {
            if (!peek.has_pending) {
                /* First click: just remember it */
                peek.has_pending = 1;
                return 0;
            }
            /* Second click: verify double-click */
            DWORD dt = GetDoubleClickTime();
            int dx = GetSystemMetrics(SM_CXDOUBLECLK);
            int dy = GetSystemMetrics(SM_CYDOUBLECLK);
            DWORD elapsed = GetTickCount() - peek.down_time;
            if (elapsed > dt
            || abs(pt.x - peek.down_pt.x) > dx
            || abs(pt.y - peek.down_pt.y) > dy) {
                /* Not a double-click, reset */
                peek.down_pt = pt;
                peek.down_time = GetTickCount();
                return 0;
            }
            peek.was_first_click = 0;
            /* Valid double-click: fall through to LBUTTONUP handler */
            peek.has_pending = 2; /* 2 = double-click confirmed */
        } else {
            peek.has_pending = 1;
        }
        return 0;
    }

    /* --- WM_MOUSEMOVE: cancel on drag --- */
    if (wParam == WM_MOUSEMOVE && peek.has_pending) {
        int dx = GetSystemMetrics(SM_CXDRAG);
        int dy = GetSystemMetrics(SM_CYDRAG);
        if (abs(pt.x - peek.down_pt.x) > dx
        || abs(pt.y - peek.down_pt.y) > dy) {
            peek.has_pending = 0;
        }
        return 0;
    }

    /* --- LBUTTONUP: fire action if click (not drag) --- */
    if (wParam == WM_LBUTTONUP) {
        if (!peek.has_pending) return 0;

        /* --- Restore on second click while peeking --- */
        if (peek.is_peeking) {
            peek.has_pending = 0;
            DWORD elapsed = GetTickCount() - peek.peek_time;
            if (elapsed < (DWORD)PEEK_GRACE_MS) return 0;
            if (GetTickCount() < peek.ignore_restore_until) return 0;
            if (IsDesktopBackground(pt) || IsTaskbarBlank(pt)) {
                PeekRestoreAll();
                return 0;
            }
            return 0;
        }

        /* --- Double-click: only fire on confirmed double-click --- */
        if (conf.PeekFlags & PEEK_F_DBLCLICK) {
            if (peek.has_pending != 2)
                return 0; /* keep has_pending for next click */
        }

        peek.has_pending = 0;

        /* --- Check desktop or taskbar click --- */
        if (IsDesktopBackground(pt)) {
            PeekCaptureAndHide();
            return 0;
        }
        if ((conf.PeekFlags & PEEK_F_TASKBAR) && IsTaskbarBlank(pt)) {
            PeekCaptureAndHide();
            return 0;
        }
    }

    return 0;
}
