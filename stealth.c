/* Per-window hover visibility. Included by hooks.c. */

#define STEALTH_PROP TEXT("AltSnap-Stealth")
#define STEALTH_MARGIN 12
#define STEALTH_DELAY 300
#define STEALTH_REVEAL_DELAY 300
#define STEALTH_ALPHA 1

struct stealth_window {
    struct stealth_window *next;
    HWND hwnd;
    DWORD thread;
    DWORD process;
    DWORD lastnear;
    DWORD reveal_start;
    COLORREF key;
    DWORD flags;
    BYTE alpha;
    BOOL layered;
    BOOL original_layered;
    BOOL hidden;
    BOOL reveal_pending;
};

static struct stealth_window *stealth_windows;

/* The property disappears with the window, even if its HWND is reused. */
static BOOL StealthValid(const struct stealth_window *win)
{
    DWORD process = 0;
    return GetProp(win->hwnd, STEALTH_PROP) == (HANDLE)win
        && GetWindowThreadProcessId(win->hwnd, &process) == win->thread
        && process == win->process;
}

static struct stealth_window *StealthFind(HWND hwnd)
{
    struct stealth_window *win;
    for (win = stealth_windows; win; win = win->next) {
        if (win->hwnd == hwnd && StealthValid(win))
            return win;
    }
    return NULL;
}

static BOOL StealthEnabled(HWND hwnd)
{
    return StealthFind(hwnd) != NULL;
}

static BOOL StealthHidden(HWND hwnd)
{
    const struct stealth_window *win = StealthFind(hwnd);
    return win && win->hidden;
}

static BOOL StealthLayer(HWND hwnd, BOOL layered)
{
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    LONG_PTR wanted = layered ? style | WS_EX_LAYERED : style & ~WS_EX_LAYERED;
    if (wanted == style)
        return TRUE;
    SetLastError(0);
    return SetWindowLongPtr(hwnd, GWL_EXSTYLE, wanted) != 0 || GetLastError() == 0;
}

/* GetLayeredWindowAttributes fails for UpdateLayeredWindow windows. Leave
 * those alone: SetLayeredWindowAttributes would break their drawing mode. */
static BOOL StealthCapture(struct stealth_window *win)
{
    LONG_PTR style = GetWindowLongPtr(win->hwnd, GWL_EXSTYLE);
    if (style & WS_EX_TRANSPARENT)
        return FALSE;
    if (style & WS_EX_LAYERED) {
        COLORREF key = 0;
        BYTE alpha = 255;
        DWORD flags = 0;
        if (!GetLayeredWindowAttributes(win->hwnd, &key, &alpha, &flags)
        || !(flags & (LWA_ALPHA | LWA_COLORKEY)))
            return FALSE;
        win->key = flags & LWA_COLORKEY ? key : 0;
        win->flags = flags;
        win->alpha = flags & LWA_ALPHA ? alpha : 255;
        win->layered = TRUE;
    } else {
        win->key = 0;
        win->flags = 0;
        win->alpha = 255;
        win->layered = FALSE;
    }
    return TRUE;
}

static BOOL StealthApply(struct stealth_window *win, BOOL hidden)
{
    if (!StealthValid(win))
        return FALSE;
    if (!hidden && !win->layered) {
        if (!StealthLayer(win->hwnd, FALSE))
            return FALSE;
        RedrawWindow(win->hwnd, NULL, NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    } else {
        BOOL hadlayer = !!(GetWindowLongPtr(win->hwnd, GWL_EXSTYLE) & WS_EX_LAYERED);
        if (!StealthLayer(win->hwnd, TRUE))
            return FALSE;
        /* Alpha zero passes mouse events through. Alpha one keeps the
         * window's hit testing, position, focus and stacking unchanged. */
        if (!SetLayeredWindowAttributes(win->hwnd, win->key,
                hidden ? STEALTH_ALPHA : win->alpha,
                hidden ? win->flags | LWA_ALPHA : win->flags)) {
            if (!hadlayer)
                StealthLayer(win->hwnd, FALSE);
            return FALSE;
        }
    }
    win->hidden = hidden;
    win->reveal_pending = FALSE;
    return TRUE;
}

static void StealthForget(struct stealth_window **link)
{
    struct stealth_window *win = *link;
    *link = win->next;
    if (GetProp(win->hwnd, STEALTH_PROP) == (HANDLE)win)
        RemoveProp(win->hwnd, STEALTH_PROP);
    free(win);
}

static BOOL StealthToggle(HWND hwnd)
{
    struct stealth_window **link = &stealth_windows;
    struct stealth_window *win;
    while (*link) {
        win = *link;
        if (!StealthValid(win)) {
            StealthForget(link);
            continue;
        }
        if (win->hwnd == hwnd) {
            if (win->hidden && !StealthApply(win, FALSE))
                return FALSE;
            StealthForget(link);
            return TRUE;
        }
        link = &win->next;
    }
    if (!IsWindow(hwnd) || GetProp(hwnd, STEALTH_PROP)
    || (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD)
    || (GetClassLongPtr(hwnd, GCL_STYLE) & (CS_OWNDC | CS_CLASSDC)))
        return FALSE;
    win = (struct stealth_window *)calloc(1, sizeof(*win));
    if (!win)
        return FALSE;
    win->hwnd = hwnd;
    win->thread = GetWindowThreadProcessId(hwnd, &win->process);
    if (!win->thread || !StealthCapture(win)
    || !SetProp(hwnd, STEALTH_PROP, (HANDLE)win)) {
        free(win);
        return FALSE;
    }
    win->original_layered = win->layered;
    win->lastnear = GetTickCount();
    win->next = stealth_windows;
    stealth_windows = win;
    return TRUE;
}

static void StealthShow(HWND hwnd)
{
    struct stealth_window *win = StealthFind(hwnd);
    if (win) {
        /* Also clear a temporary MoveTrans opacity after a drag. */
        StealthApply(win, FALSE);
        win->lastnear = GetTickCount();
    }
}

/* Return TRUE only when this module owns the window's visibility. */
static BOOL StealthGetAlpha(HWND hwnd, BYTE *alpha)
{
    struct stealth_window *win = StealthFind(hwnd);
    if (!win)
        return FALSE;
    *alpha = win->alpha;
    return TRUE;
}

static BOOL StealthSetAlpha(HWND hwnd, BYTE alpha)
{
    struct stealth_window *win = StealthFind(hwnd);
    if (!win)
        return FALSE;
    BYTE oldalpha = win->alpha;
    DWORD oldflags = win->flags;
    BOOL oldlayered = win->layered;
    win->alpha = alpha;
    win->flags |= LWA_ALPHA;
    win->layered = win->original_layered || alpha < 255
        || (win->flags & LWA_COLORKEY);
    if (!win->hidden && !StealthApply(win, FALSE)) {
        win->alpha = oldalpha;
        win->flags = oldflags;
        win->layered = oldlayered;
    }
    return TRUE;
}

static BOOL StealthBelongsTo(HWND hwnd, HWND target)
{
    if (target)
        target = GetAncestor(target, GA_ROOT);
    while (target) {
        if (target == hwnd)
            return TRUE;
        target = GetWindow(target, GW_OWNER);
    }
    return FALSE;
}

#if defined(WINVER) && WINVER >= 0x0500 && defined(WINEVENT_OUTOFCONTEXT)
#ifndef EVENT_OBJECT_IME_SHOW
#define EVENT_OBJECT_IME_SHOW 0x8027
#define EVENT_OBJECT_IME_HIDE 0x8028
#define EVENT_OBJECT_IME_CHANGE 0x8029
#endif

struct stealth_ime_window {
    struct stealth_ime_window *next;
    HWND hwnd;
    HWND target;
    HWND focus;
    DWORD thread;
    DWORD process;
    LONG object;
    LONG child;
    BOOL compat;
};

static HWINEVENTHOOK stealth_ime_hook;
static HWINEVENTHOOK stealth_ime_window_hook;
static struct stealth_ime_window *stealth_ime_windows;

/* WeChat's candidate UI was observed to send window SHOW/HIDE rather than
 * IME events. Its settings UI shares the class, so require the candidate's
 * non-activating, layered, topmost tool-window shape as well. */
static BOOL StealthImeCompatibleWindow(HWND hwnd)
{
    TCHAR name[64];
    RECT rc;
    LONG_PTR required = WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    return IsWindowVisible(hwnd)
        && (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & required) == required
        && GetClassName(hwnd, name, sizeof(name) / sizeof(*name))
        && !lstrcmp(name, TEXT("wetype.flutter.setting"))
        && GetWindowRect(hwnd, &rc) && rc.right > rc.left && rc.bottom > rc.top;
}

static HWND StealthImeFocus(void)
{
    GUITHREADINFO gui = {0};
    gui.cbSize = sizeof(gui);
    return GetGUIThreadInfoL(0, &gui) ? gui.hwndFocus : NULL;
}

static void StealthImeForget(struct stealth_ime_window **link)
{
    struct stealth_ime_window *ime = *link;
    *link = ime->next;
    free(ime);
}

static void StealthImePrune(HWND focus, HWND foreground)
{
    struct stealth_ime_window **link = &stealth_ime_windows;
    while (*link) {
        struct stealth_ime_window *ime = *link;
        DWORD process = 0;
        if (!focus || focus != ime->focus || !StealthFind(ime->target)
        || !StealthBelongsTo(ime->target, focus)
        || !StealthBelongsTo(ime->target, foreground)
        || !IsWindowVisible(ime->hwnd)
        || (ime->compat && !StealthImeCompatibleWindow(ime->hwnd))
        || GetWindowThreadProcessId(ime->hwnd, &process) != ime->thread
        || process != ime->process) {
            StealthImeForget(link);
            continue;
        }
        link = &ime->next;
    }
}

/* IME providers can create candidate UI on another thread or in another
 * process. Their IME WinEvents identify it without reading input text or
 * treating an ordinary overlapping window as part of the application. */
static void CALLBACK StealthImeEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
    LONG object, LONG child, DWORD event_thread, DWORD event_time)
{
    struct stealth_ime_window **link = &stealth_ime_windows;
    struct stealth_ime_window *ime;
    struct stealth_window *win;
    HWND focus;
    HWND foreground;
    DWORD process = 0;
    DWORD thread;
    BOOL compat;
    BOOL hiding;
    (void)event_thread;
    (void)event_time;
    if (!hwnd || (!stealth_windows && !stealth_ime_windows))
        return;
    compat = stealth_ime_window_hook && hook == stealth_ime_window_hook;
    if (compat) {
        if ((event != EVENT_OBJECT_SHOW && event != EVENT_OBJECT_HIDE)
        || object != OBJID_WINDOW || child != CHILDID_SELF)
            return;
        if (event == EVENT_OBJECT_SHOW && !StealthImeCompatibleWindow(hwnd))
            return;
    } else if (!stealth_ime_hook || hook != stealth_ime_hook
    || (event != EVENT_OBJECT_IME_SHOW && event != EVENT_OBJECT_IME_HIDE
        && event != EVENT_OBJECT_IME_CHANGE)) {
        return;
    }
    hiding = event == EVENT_OBJECT_IME_HIDE || event == EVENT_OBJECT_HIDE;
    focus = StealthImeFocus();
    foreground = GetForegroundWindow();
    StealthImePrune(focus, foreground);
    while (*link) {
        ime = *link;
        if (ime->hwnd == hwnd && ((compat && hiding)
            || (ime->object == object && ime->child == child))) {
            if (hiding) {
                StealthImeForget(link);
                if (compat)
                    continue; // A whole-window HIDE ends every object on it.
            } else if (!compat) {
                ime->compat = FALSE;
            }
            return;
        }
        link = &ime->next;
    }
    if (hiding || !focus || !IsWindowVisible(hwnd))
        return;
    thread = GetWindowThreadProcessId(hwnd, &process);
    if (!thread)
        return;
    for (win = stealth_windows; win; win = win->next) {
        if (StealthValid(win) && StealthBelongsTo(win->hwnd, foreground)
        && StealthBelongsTo(win->hwnd, focus))
            break;
    }
    if (!win)
        return;
    ime = (struct stealth_ime_window *)calloc(1, sizeof(*ime));
    if (!ime)
        return;
    ime->hwnd = hwnd;
    ime->target = win->hwnd;
    ime->focus = focus;
    ime->thread = thread;
    ime->process = process;
    ime->object = object;
    ime->child = child;
    ime->compat = compat;
    ime->next = stealth_ime_windows;
    stealth_ime_windows = ime;
}

static void StealthImeInit(void)
{
    if (!stealth_ime_hook)
        stealth_ime_hook = SetWinEventHookL(EVENT_OBJECT_IME_SHOW, EVENT_OBJECT_IME_CHANGE,
            NULL, StealthImeEvent, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (!stealth_ime_window_hook)
        stealth_ime_window_hook = SetWinEventHookL(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE,
            NULL, StealthImeEvent, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

static void StealthImeShutdown(void)
{
    HWINEVENTHOOK hook = stealth_ime_hook;
    HWINEVENTHOOK window_hook = stealth_ime_window_hook;
    stealth_ime_hook = NULL;
    stealth_ime_window_hook = NULL;
    if (hook)
        UnhookWinEventL(hook);
    if (window_hook)
        UnhookWinEventL(window_hook);
    while (stealth_ime_windows)
        StealthImeForget(&stealth_ime_windows);
}

static BOOL StealthImeBusy(const struct stealth_window *win)
{
    struct stealth_ime_window *ime;
    if (!stealth_ime_windows)
        return FALSE;
    StealthImePrune(StealthImeFocus(), GetForegroundWindow());
    for (ime = stealth_ime_windows; ime; ime = ime->next) {
        if (ime->target == win->hwnd)
            return TRUE;
    }
    return FALSE;
}
#else
static void StealthImeInit(void) {}
static void StealthImeShutdown(void) {}
static BOOL StealthImeBusy(const struct stealth_window *win)
{
    (void)win;
    return FALSE;
}
#endif

static BOOL StealthNativeBusy(const struct stealth_window *win)
{
    GUITHREADINFO gui = {0};
    if (StealthImeBusy(win))
        return TRUE;
    gui.cbSize = sizeof(gui);
    if (!GetGUIThreadInfoL(win->thread, &gui))
        return FALSE;
    return ((gui.flags & GUI_INMOVESIZE) && StealthBelongsTo(win->hwnd, gui.hwndMoveSize))
        || ((gui.flags & (GUI_INMENUMODE | GUI_SYSTEMMENUMODE | GUI_POPUPMENUMODE))
            && StealthBelongsTo(win->hwnd, gui.hwndMenuOwner))
        || StealthBelongsTo(win->hwnd, gui.hwndCapture);
}

static BOOL StealthNear(HWND hwnd, POINT pt, BOOL include_margin)
{
    RECT rc;
    /* Hit testing observes occlusion, topmost windows and color-key holes.
     * Owned popups belong to the same application's interaction area. */
    if (StealthBelongsTo(hwnd, WindowFromPoint(pt)))
        return TRUE;
    if (!include_margin)
        return FALSE;
    if (!GetWindowRect(hwnd, &rc))
        return FALSE;
    /* The tolerance only extends outside the window. A different window
     * covering its interior must not keep it revealed. */
    if (PtInRect(&rc, pt))
        return FALSE;
    InflateRect(&rc, STEALTH_MARGIN, STEALTH_MARGIN);
    return PtInRect(&rc, pt);
}

static BOOL StealthTick(HWND menu_hwnd, HWND moving_hwnd)
{
    POINT pt;
    DWORD now = GetTickCount();
    struct stealth_window **link = &stealth_windows;
    BOOL havecursor = GetCursorPos(&pt);
    while (*link) {
        struct stealth_window *win = *link;
        if (!StealthValid(win)) {
            StealthForget(link);
            continue;
        }
        /* MoveTrans is temporary; never record it as the normal opacity. */
        if (win->hwnd == moving_hwnd || win->hwnd == menu_hwnd
        || StealthNativeBusy(win)) {
            if (win->hidden)
                StealthApply(win, FALSE);
            win->reveal_pending = FALSE;
            win->lastnear = now;
            link = &win->next;
            continue;
        }
        if (win->hidden) {
            COLORREF key = 0;
            BYTE alpha = 0;
            DWORD flags = 0;
            /* Respect an application that changes its own transparency. */
            if (!(GetWindowLongPtr(win->hwnd, GWL_EXSTYLE) & WS_EX_LAYERED)
            || !GetLayeredWindowAttributes(win->hwnd, &key, &alpha, &flags)
            || alpha != STEALTH_ALPHA
            || ((flags & LWA_COLORKEY) && key != win->key)
            || flags != (win->flags | LWA_ALPHA)) {
                win->hidden = FALSE;
                win->reveal_pending = FALSE;
                win->lastnear = now;
            }
        }
        if (!win->hidden && !StealthCapture(win)) {
            StealthForget(link);
            continue;
        }
        if (!IsWindowVisible(win->hwnd) || IsIconic(win->hwnd) || !havecursor) {
            if (win->hidden)
                StealthApply(win, FALSE);
            win->reveal_pending = FALSE;
            win->lastnear = now;
        } else if (win->hidden) {
            if (StealthNear(win->hwnd, pt, FALSE)) {
                if (!win->reveal_pending) {
                    win->reveal_start = now;
                    win->reveal_pending = TRUE;
                } else if ((DWORD)(now - win->reveal_start) >= STEALTH_REVEAL_DELAY) {
                    if (StealthApply(win, FALSE))
                        win->lastnear = now;
                }
            } else {
                win->reveal_pending = FALSE;
            }
        } else if (StealthNear(win->hwnd, pt, TRUE)) {
            win->lastnear = now;
        } else if ((DWORD)(now - win->lastnear) >= STEALTH_DELAY) {
            if (!StealthApply(win, TRUE)) {
                StealthForget(link);
                continue;
            }
        }
        link = &win->next;
    }
    return stealth_windows != NULL;
}

static void StealthRestoreAll(void)
{
    while (stealth_windows) {
        if (StealthValid(stealth_windows))
            StealthApply(stealth_windows, FALSE);
        StealthForget(&stealth_windows);
    }
}
