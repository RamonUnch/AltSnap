/* Per-window hover visibility. Included by hooks.c. */

#define STEALTH_PROP TEXT("AltSnap-Stealth")
#define STEALTH_MARGIN 12
#define STEALTH_DELAY 300
#define STEALTH_ALPHA 1

struct stealth_window {
    struct stealth_window *next;
    HWND hwnd;
    DWORD thread;
    DWORD process;
    DWORD lastnear;
    COLORREF key;
    DWORD flags;
    BYTE alpha;
    BOOL layered;
    BOOL original_layered;
    BOOL hidden;
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

static BOOL StealthNativeBusy(const struct stealth_window *win)
{
    GUITHREADINFO gui = {0};
    gui.cbSize = sizeof(gui);
    if (!GetGUIThreadInfoL(win->thread, &gui))
        return FALSE;
    return ((gui.flags & GUI_INMOVESIZE) && StealthBelongsTo(win->hwnd, gui.hwndMoveSize))
        || ((gui.flags & (GUI_INMENUMODE | GUI_SYSTEMMENUMODE | GUI_POPUPMENUMODE))
            && StealthBelongsTo(win->hwnd, gui.hwndMenuOwner))
        || StealthBelongsTo(win->hwnd, gui.hwndCapture);
}

static BOOL StealthNear(HWND hwnd, POINT pt)
{
    RECT rc;
    if (!GetWindowRect(hwnd, &rc))
        return FALSE;
    InflateRect(&rc, STEALTH_MARGIN, STEALTH_MARGIN);
    if (PtInRect(&rc, pt))
        return TRUE;
    /* Keep an application's owned popups usable outside the main rect. */
    return StealthBelongsTo(hwnd, WindowFromPoint(pt));
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
                win->lastnear = now;
            }
        }
        if (!win->hidden && !StealthCapture(win)) {
            StealthForget(link);
            continue;
        }
        if (!IsWindowVisible(win->hwnd) || IsIconic(win->hwnd)
        || !havecursor || StealthNear(win->hwnd, pt)) {
            if (win->hidden)
                StealthApply(win, FALSE);
            win->lastnear = now;
        } else if (!win->hidden && (DWORD)(now - win->lastnear) >= STEALTH_DELAY) {
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
