/*
 * Native Windows regression tests for stealth.c.
 * The test owns every window it touches. Cursor position and time are simulated;
 * no input is sent and the user's cursor is never moved.
 *
 * From the repository root (MinGW-w64):
 *   gcc -std=c99 -Wall -Wextra -o tests/stealth_test.exe tests/stealth_test.c -luser32
 *   tests/stealth_test.exe
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static POINT test_cursor;
static HWND test_pointed_window;
static BOOL test_cursor_available = TRUE;
static BOOL test_gui_available;
static GUITHREADINFO test_gui;
static HWND test_foreground_window;
static GUITHREADINFO test_foreground_gui;
static BOOL test_foreground_gui_available;
static HWND test_reused_candidate;
static BOOL test_candidate_identity_changed;
static BOOL test_event_hook_available = TRUE;
static BOOL test_generic_event_hook_available = TRUE;
static unsigned test_event_hook_calls;
static unsigned test_event_unhook_calls;
static struct test_event_subscription {
    HWINEVENTHOOK handle;
    DWORD first;
    DWORD last;
    WINEVENTPROC callback;
    BOOL active;
} test_event_subscriptions[8];
static DWORD test_clock = 1000;
static unsigned checks;
static unsigned failures;
static HWND test_windows[64];
static size_t test_window_count;

static BOOL TestGetCursorPos(LPPOINT point)
{
    *point = test_cursor;
    return test_cursor_available;
}

static DWORD TestGetTickCount(void)
{
    return test_clock;
}

static HWND TestWindowFromPoint(POINT point)
{
    return test_pointed_window ? test_pointed_window : WindowFromPoint(point);
}

static BOOL TestGetGUIThreadInfoL(DWORD thread, LPGUITHREADINFO info)
{
    *info = thread ? test_gui : test_foreground_gui;
    info->cbSize = sizeof(*info);
    return thread ? test_gui_available : test_foreground_gui_available;
}

static HWND TestGetForegroundWindow(void)
{
    return test_foreground_window;
}

static DWORD TestGetWindowThreadProcessId(HWND window, LPDWORD process)
{
    DWORD thread = GetWindowThreadProcessId(window, process);
    if (window == test_reused_candidate && test_candidate_identity_changed) {
        if (process) ++*process;
        ++thread;
    }
    return thread;
}

static HWINEVENTHOOK TestSetWinEventHookL(DWORD first, DWORD last, HMODULE module,
    WINEVENTPROC callback, DWORD process, DWORD thread, DWORD flags)
{
    (void)module;
    (void)process;
    (void)thread;
    (void)flags;
    ++test_event_hook_calls;
    if (!test_event_hook_available
    || (!test_generic_event_hook_available && first == EVENT_OBJECT_SHOW)) return NULL;
    for (size_t index = 0; index < sizeof(test_event_subscriptions) / sizeof(test_event_subscriptions[0]); ++index) {
        struct test_event_subscription *subscription = &test_event_subscriptions[index];
        if (!subscription->active) {
            subscription->handle = (HWINEVENTHOOK)(UINT_PTR)(0x100 + test_event_hook_calls);
            subscription->first = first;
            subscription->last = last;
            subscription->callback = callback;
            subscription->active = TRUE;
            return subscription->handle;
        }
    }
    return NULL;
}

static BOOL TestUnhookWinEventL(HWINEVENTHOOK handle)
{
    ++test_event_unhook_calls;
    for (size_t index = 0; index < sizeof(test_event_subscriptions) / sizeof(test_event_subscriptions[0]); ++index)
        if (test_event_subscriptions[index].handle == handle) {
            test_event_subscriptions[index].active = FALSE;
            return TRUE;
        }
    return FALSE;
}

#define GetCursorPos TestGetCursorPos
#define GetTickCount TestGetTickCount
#define WindowFromPoint TestWindowFromPoint
#define GetGUIThreadInfoL TestGetGUIThreadInfoL
#define GetForegroundWindow TestGetForegroundWindow
#define GetWindowThreadProcessId TestGetWindowThreadProcessId
#define SetWinEventHookL TestSetWinEventHookL
#define UnhookWinEventL TestUnhookWinEventL
#include "../stealth.c"
#undef GetCursorPos
#undef GetTickCount
#undef WindowFromPoint
#undef GetGUIThreadInfoL
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
#undef SetWinEventHookL
#undef UnhookWinEventL

#define CHECK(condition, message) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); \
    } \
} while (0)

static HWND NewClassWindow(const char *classname, DWORD extended_style)
{
    HWND window = CreateWindowExA(extended_style | WS_EX_NOACTIVATE,
        classname, "AltSnap stealth test (offscreen)", WS_POPUP,
        -20000 - (int)(test_window_count % 16) * 500,
        -20000 - (int)(test_window_count / 16) * 400, 320, 240,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    CHECK(window != NULL, "create isolated test window");
    if (!window) {
        fprintf(stderr, "CreateWindowEx failed: %lu\n", GetLastError());
        exit(2);
    }
    test_windows[test_window_count++] = window;
    ShowWindow(window, SW_SHOWNOACTIVATE);
    return window;
}

static HWND NewWindow(DWORD extended_style)
{
    return NewClassWindow("AltSnapStealthRegression", extended_style);
}

static void PointInside(HWND window)
{
    RECT rectangle;
    CHECK(GetWindowRect(window, &rectangle), "get test window rectangle");
    test_cursor.x = rectangle.left + 40;
    test_cursor.y = rectangle.top + 40;
}

static void PointAway(void)
{
    test_cursor.x = -1000;
    test_cursor.y = -1000;
}

static void Tick(void)
{
    StealthTick(NULL, NULL);
}

static void LeaveAndWait(void)
{
    PointAway();
    Tick();
    test_clock += 350;
    Tick();
}

static void DwellInside(HWND window)
{
    PointInside(window);
    Tick();
    test_clock += 300;
    Tick();
}

static BYTE ActualAlpha(HWND window)
{
    BYTE alpha = 255;
    DWORD flags = 0;
    if (!GetLayeredWindowAttributes(window, NULL, &alpha, &flags)
    || !(flags & LWA_ALPHA))
        return 255;
    return alpha;
}

static void CheckNoClickThrough(HWND window)
{
    RECT rectangle;
    POINT point;
    CHECK(!(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TRANSPARENT),
        "stealth does not enable click-through");
    CHECK(ActualAlpha(window) == 1,
        "hidden alpha is 1, avoiding Windows alpha-zero hit-test exclusion");
    GetWindowRect(window, &rectangle);
    point.x = rectangle.left + 40;
    point.y = rectangle.top + 40;
    CHECK(WindowFromPoint(point) == window,
        "real Windows hit testing still selects the invisible window");
}

static void TestNormalWindow(void)
{
    HWND window = NewWindow(0);
    LONG_PTR initial_style = GetWindowLongPtr(window, GWL_EXSTYLE);
    RECT original_rectangle, rectangle;
    GetWindowRect(window, &original_rectangle);
    PointInside(window);
    CHECK(StealthToggle(window), "enable normal window");
    CHECK(StealthEnabled(window), "normal window reports enabled");
    CHECK(!StealthHidden(window), "window remains visible while cursor is inside");
    PointAway();
    Tick();
    test_clock += 100;
    Tick();
    CHECK(!StealthHidden(window), "briefly leaving does not hide the window");
    test_clock += 250;
    Tick();
    CHECK(StealthHidden(window), "window hides after leaving delay");
    CheckNoClickThrough(window);
    GetWindowRect(window, &rectangle);
    CHECK(EqualRect(&original_rectangle, &rectangle), "stealth preserves position and size");
    CHECK(IsWindowVisible(window), "stealth preserves WS_VISIBLE and window occupancy");

    test_cursor.x = rectangle.left - 11;
    test_cursor.y = rectangle.top + 40;
    Tick();
    test_clock += 350;
    Tick();
    CHECK(StealthHidden(window), "outside 12-pixel tolerance does not start reveal dwell");
    PointInside(window);
    Tick();
    CHECK(StealthHidden(window), "entering the actual window does not reveal it immediately");
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window) && ActualAlpha(window) == 1,
        "window remains hidden at the 299-millisecond boundary");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "window reveals after 300 milliseconds continuously inside");
    CHECK(ActualAlpha(window) == 255, "normal opacity is restored after reveal dwell");

    test_cursor.x = rectangle.left - 11;
    Tick();
    test_clock += 350;
    Tick();
    CHECK(!StealthHidden(window), "visible window retains the 12-pixel leave tolerance");

    LeaveAndWait();
    CHECK(StealthHidden(window), "window can hide repeatedly");
    CHECK(StealthToggle(window), "disable hidden normal window");
    CHECK(!StealthEnabled(window), "toggle disables stealth");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) == initial_style,
        "disabling restores original non-layered extended style");
}

static void TestTransparencyAndTopmost(void)
{
    HWND window = NewWindow(WS_EX_LAYERED | WS_EX_TOPMOST);
    const COLORREF key = RGB(249, 1, 240);
    BYTE alpha = 0;
    DWORD flags = 0;
    COLORREF actual_key = 0;
    CHECK(SetLayeredWindowAttributes(window, key, 137, LWA_ALPHA | LWA_COLORKEY),
        "set original opacity and color key");
    PointInside(window);
    CHECK(StealthToggle(window), "enable alpha and topmost window");
    LeaveAndWait();
    CHECK(StealthHidden(window), "alpha window hides");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "hiding preserves topmost state");
    CheckNoClickThrough(window);
    CHECK(StealthGetAlpha(window, &alpha) && alpha == 137,
        "logical alpha is preserved while physical alpha is hidden");

    CHECK(StealthSetAlpha(window, 89), "opacity action is handled during stealth");
    CHECK(StealthHidden(window) && ActualAlpha(window) == 1,
        "opacity adjustment does not reveal an away window");
    DwellInside(window);
    CHECK(ActualAlpha(window) == 89, "reveal dwell restores the latest chosen opacity");
    CHECK(GetLayeredWindowAttributes(window, &actual_key, &alpha, &flags),
        "read restored layered attributes");
    CHECK(actual_key == key && flags == (LWA_ALPHA | LWA_COLORKEY),
        "revealing retains original color-key behavior");

    CHECK(SetLayeredWindowAttributes(window, key, 173, LWA_ALPHA | LWA_COLORKEY),
        "simulate an app changing its visible opacity");
    LeaveAndWait();
    DwellInside(window);
    CHECK(ActualAlpha(window) == 173, "visible external opacity changes are retained");
    CHECK(StealthToggle(window), "disable originally layered window");
    CHECK(GetLayeredWindowAttributes(window, &actual_key, &alpha, &flags)
        && alpha == 173 && actual_key == key && flags == (LWA_ALPHA | LWA_COLORKEY),
        "disabling retains latest opacity and original color key");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "disabling stealth preserves topmost state");
}

static void TestMenuDragAndMovement(void)
{
    HWND window = NewWindow(0);
    PointInside(window);
    CHECK(StealthToggle(window), "enable window for menu and drag tests");
    LeaveAndWait();
    CHECK(StealthHidden(window), "hide before menu test");
    PointInside(window);
    Tick();
    test_clock += 200;
    Tick();
    StealthTick(window, NULL);
    CHECK(!StealthHidden(window), "action menu target reveals immediately during pending dwell");
    test_clock += 1000;
    StealthTick(window, NULL);
    CHECK(!StealthHidden(window), "action menu prevents hiding while cursor is away");
    LeaveAndWait();
    CHECK(StealthHidden(window), "window hides again after menu closes");
    PointInside(window);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window), "menu reveal clears the old partial dwell before the next hide");
    StealthTick(NULL, window);
    CHECK(!StealthHidden(window), "moving target reveals immediately during pending dwell");
    test_clock += 1000;
    StealthTick(NULL, window);
    CHECK(!StealthHidden(window), "dragging prevents hiding while cursor is away");

    CHECK(SetWindowPos(window, NULL, -25000, -25000, 400, 300,
        SWP_NOACTIVATE | SWP_NOZORDER), "move and resize isolated window");
    LeaveAndWait();
    DwellInside(window);
    CHECK(!StealthHidden(window), "reveal dwell follows moved and resized windows");
    CHECK(StealthToggle(window), "disable menu and movement test window");
}

static void TestMultipleWindowsAndCleanup(void)
{
    HWND first = NewWindow(0);
    HWND second = NewWindow(WS_EX_LAYERED);
    BYTE alpha;
    CHECK(SetLayeredWindowAttributes(second, 0, 111, LWA_ALPHA), "set second window opacity");
    PointInside(first);
    CHECK(StealthToggle(first), "enable first independent window");
    PointInside(second);
    CHECK(StealthToggle(second), "enable second independent window");
    LeaveAndWait();
    CHECK(StealthHidden(first) && StealthHidden(second), "multiple windows hide independently");
    PointInside(first);
    Tick();
    test_clock += 200;
    Tick();
    CHECK(StealthHidden(first) && StealthHidden(second), "partial dwell reveals neither independent window");
    PointInside(second);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(first) && StealthHidden(second),
        "second window does not inherit the first window's partial dwell");
    test_clock += 1;
    Tick();
    CHECK(StealthHidden(first) && !StealthHidden(second),
        "only the second window reveals after its own 300-millisecond dwell");
    PointInside(first);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(first), "returning to the first window restarts its full dwell");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(first), "first window reveals after a new uninterrupted dwell");

    CHECK(DestroyWindow(first), "close a tracked test window");
    Tick();
    CHECK(!StealthEnabled(first), "closed window is removed from tracking");
    StealthRestoreAll();
    CHECK(!StealthEnabled(second), "restore-all clears remaining tracking");
    CHECK(GetLayeredWindowAttributes(second, NULL, &alpha, NULL) && alpha == 111,
        "restore-all restores a hidden window's original opacity");
    CHECK(!StealthTick(NULL, NULL), "empty tracker does not request further polling");
}

static void TestNativeHideMinimizeAndPopup(void)
{
    HWND window = NewWindow(WS_EX_LAYERED);
    HWND popup = NewWindow(0);
    CHECK(SetLayeredWindowAttributes(window, 0, 147, LWA_ALPHA),
        "set opacity for native visibility tests");
    PointInside(window);
    CHECK(StealthToggle(window), "enable native visibility test window");
    LeaveAndWait();
    ShowWindow(window, SW_SHOWMINNOACTIVE);
    Tick();
    CHECK(IsIconic(window), "window remains minimized");
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 147,
        "minimizing restores opacity without restoring the window");
    ShowWindow(window, SW_SHOWNOACTIVATE);
    CHECK(!IsIconic(window), "restore isolated test window without activation");
    LeaveAndWait();
    CHECK(StealthHidden(window), "restored window can hide again");
    ShowWindow(window, SW_HIDE);
    Tick();
    CHECK(!IsWindowVisible(window), "native-hidden window is not made visible");
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 147,
        "native hiding restores normal opacity for a later application show");
    ShowWindow(window, SW_SHOWNOACTIVATE);
    LeaveAndWait();

    CHECK(SetWindowLongPtr(popup, GWLP_HWNDPARENT, (LONG_PTR)window) == 0,
        "assign isolated popup ownership");
    test_pointed_window = popup;
    Tick();
    CHECK(StealthHidden(window), "owned popup starts dwell instead of revealing immediately");
    test_clock += 300;
    Tick();
    CHECK(!StealthHidden(window), "owned popup reveals its owner after continuous dwell");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "owned popup keeps its owner visible while in use");
    test_pointed_window = NULL;
    LeaveAndWait();
    CHECK(StealthHidden(window), "leaving owned popup permits hiding again");

    test_cursor_available = FALSE;
    Tick();
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 147,
        "unavailable cursor position fails open to a visible window");
    test_cursor_available = TRUE;
    CHECK(StealthToggle(window), "disable native visibility test window");
}

static void TestTemporaryOpacityAndOwnership(void)
{
    HWND window = NewWindow(WS_EX_LAYERED);
    CHECK(SetLayeredWindowAttributes(window, 0, 161, LWA_ALPHA),
        "set baseline opacity for MoveTrans test");
    PointInside(window);
    CHECK(StealthToggle(window), "enable temporary-opacity test window");
    CHECK(SetLayeredWindowAttributes(window, 0, 40, LWA_ALPHA),
        "simulate temporary drag opacity");
    StealthTick(NULL, window);
    test_clock += 1000;
    StealthTick(NULL, window);
    CHECK(ActualAlpha(window) == 40, "drag polling does not overwrite temporary drag opacity");
    StealthShow(window);
    CHECK(ActualAlpha(window) == 161, "finishing a drag restores normal opacity");
    CHECK(StealthToggle(window), "disable temporary-opacity test window");

    CHECK(StealthToggle(window), "re-enable window for shutdown during MoveTrans");
    SetLayeredWindowAttributes(window, 0, 40, LWA_ALPHA);
    StealthRestoreAll();
    CHECK(ActualAlpha(window) == 161,
        "restore-all clears temporary opacity even while a tracked window is visible");

    window = NewWindow(0);
    PointInside(window);
    CHECK(StealthToggle(window), "enable ownership-token test window");
    LeaveAndWait();
    CHECK(RemoveProp(window, STEALTH_PROP) != NULL, "simulate disappearance of ownership token");
    CHECK(SetProp(window, STEALTH_PROP, (HANDLE)(UINT_PTR)1),
        "simulate a replacement window's unrelated token");
    Tick();
    CHECK(!StealthEnabled(window), "a replaced ownership token invalidates old tracking");
    CHECK(GetProp(window, STEALTH_PROP) == (HANDLE)(UINT_PTR)1,
        "stale tracking does not remove a replacement window's property");
    CHECK(ActualAlpha(window) == 1, "stale tracking does not mutate a replacement window");
    RemoveProp(window, STEALTH_PROP);
    SetWindowLongPtr(window, GWL_EXSTYLE,
        GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_LAYERED);
}

static void TestOpacityChangesWhileHidden(void)
{
    HWND window = NewWindow(0);
    BYTE alpha;
    PointInside(window);
    CHECK(StealthToggle(window), "enable opacity edge-case window");
    CHECK(StealthSetAlpha(window, 100) && ActualAlpha(window) == 100,
        "stealth transparency action can layer a previously opaque window");
    CHECK(StealthSetAlpha(window, 255), "set a previously opaque window back to full alpha");
    CHECK(!(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_LAYERED),
        "full alpha restores the originally non-layered drawing mode");
    LeaveAndWait();
    CHECK(StealthHidden(window), "full-alpha window can still hide after opacity toggling");
    CHECK(SetLayeredWindowAttributes(window, 0, 199, LWA_ALPHA),
        "simulate an app changing opacity while stealth has hidden it");
    Tick();
    CHECK(!StealthHidden(window) && StealthGetAlpha(window, &alpha) && alpha == 199,
        "an app's own opacity change is respected while hidden");
    LeaveAndWait();
    DwellInside(window);
    CHECK(ActualAlpha(window) == 199, "subsequent dwell restores the app's new opacity");
    CHECK(StealthToggle(window), "disable opacity edge-case window");
}

static void TestNativeMenuDragAndCapture(void)
{
    HWND window = NewWindow(0);
    HWND popup = NewWindow(0);
    PointInside(window);
    CHECK(StealthToggle(window), "enable native interaction test window");
    SetWindowLongPtr(popup, GWLP_HWNDPARENT, (LONG_PTR)window);
    test_gui_available = TRUE;

    LeaveAndWait();
    test_gui.flags = GUI_INMENUMODE;
    test_gui.hwndMenuOwner = popup;
    Tick();
    CHECK(!StealthHidden(window), "native owned menu reveals its owner");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "native menu keeps owner visible while cursor is away");

    ZeroMemory(&test_gui, sizeof(test_gui));
    LeaveAndWait();
    test_gui.flags = GUI_INMOVESIZE;
    test_gui.hwndMoveSize = window;
    Tick();
    CHECK(!StealthHidden(window), "native move-size loop reveals the window");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "native move-size loop prevents hiding");

    ZeroMemory(&test_gui, sizeof(test_gui));
    LeaveAndWait();
    test_gui.hwndCapture = popup;
    Tick();
    CHECK(!StealthHidden(window), "mouse capture in an owned window reveals its owner");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "mouse capture keeps owner visible outside its rectangle");

    ZeroMemory(&test_gui, sizeof(test_gui));
    test_gui_available = FALSE;
    CHECK(StealthToggle(window), "disable native interaction test window");
}

static void TestColorKeyOnlyAndClockWrap(void)
{
    HWND window = NewWindow(WS_EX_LAYERED);
    const COLORREF original_key = RGB(17, 19, 23);
    COLORREF key = 0;
    DWORD flags = 0;
    CHECK(SetLayeredWindowAttributes(window, original_key, 255, LWA_COLORKEY),
        "set a color-key-only layered window");
    test_clock = 0xFFFFFF00u;
    PointInside(window);
    CHECK(StealthToggle(window), "enable window near GetTickCount rollover");
    LeaveAndWait();
    CHECK(StealthHidden(window), "hide delay handles unsigned GetTickCount rollover");
    test_clock = 0xFFFFFF00u;
    PointInside(window);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window), "reveal dwell remains pending at 299 milliseconds across clock rollover");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "reveal dwell completes at 300 milliseconds across clock rollover");
    CHECK(GetLayeredWindowAttributes(window, &key, NULL, &flags)
        && key == original_key && flags == LWA_COLORKEY,
        "revealing restores color-key-only flags without introducing alpha");
    CHECK(StealthToggle(window), "disable color-key-only layered window");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_LAYERED,
        "original color-key-only layered style remains enabled");
}

static void TestInterruptedRevealDwell(void)
{
    HWND window = NewWindow(0);
    RECT rectangle;
    PointInside(window);
    CHECK(StealthToggle(window), "enable interrupted-dwell test window");
    LeaveAndWait();
    PointInside(window);
    Tick();
    test_clock += 20;
    Tick();
    PointAway();
    Tick();
    test_clock += 1000;
    Tick();
    CHECK(StealthHidden(window), "briefly passing through does not reveal the window later");

    PointInside(window);
    Tick();
    test_clock += 200;
    Tick();
    GetWindowRect(window, &rectangle);
    test_cursor.x = rectangle.left - 11;
    test_cursor.y = rectangle.top + 40;
    Tick();
    test_clock += 100;
    Tick();
    CHECK(StealthHidden(window), "leaving actual bounds into the outer tolerance cancels dwell");
    PointInside(window);
    Tick();
    test_clock += 100;
    Tick();
    CHECK(StealthHidden(window), "interrupted dwell time is not accumulated after re-entry");
    test_clock += 199;
    Tick();
    CHECK(StealthHidden(window), "re-entry requires all 300 milliseconds again");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "uninterrupted re-entry dwell finally reveals the window");

    LeaveAndWait();
    PointInside(window);
    Tick();
    StealthShow(window);
    CHECK(!StealthHidden(window), "explicit window operation reveals immediately during pending dwell");
    LeaveAndWait();
    test_clock = 0;
    PointInside(window);
    Tick();
    CHECK(StealthHidden(window), "a dwell beginning at clock zero is not treated as completed");
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window), "explicit operation clears old dwell before the next hide");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "a dwell beginning at clock zero completes at 300 milliseconds");
    CHECK(StealthToggle(window), "disable interrupted-dwell test window");
}

static void TestOcclusionAndTopmostReveal(void)
{
    HWND window = NewWindow(0);
    HWND cover = NewWindow(0);
    RECT rectangle;
    PointInside(window);
    CHECK(StealthToggle(window), "enable occlusion test window");
    GetWindowRect(window, &rectangle);
    CHECK(SetWindowPos(cover, HWND_TOP, rectangle.left, rectangle.top,
        rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, SWP_NOACTIVATE),
        "place an isolated covering window above the stealth target");
    LeaveAndWait();
    PointInside(window);
    CHECK(WindowFromPoint(test_cursor) == cover, "real hit testing selects the covering window");
    Tick();
    test_clock += 1000;
    Tick();
    CHECK(StealthHidden(window), "geometry behind another window does not start reveal dwell");

    ShowWindow(cover, SW_HIDE);
    CHECK(WindowFromPoint(test_cursor) == window, "removing cover exposes the stealth input region");
    Tick();
    test_clock += 200;
    Tick();
    CHECK(StealthHidden(window), "uncovered partial dwell remains hidden");
    ShowWindow(cover, SW_SHOWNOACTIVATE);
    SetWindowPos(cover, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    CHECK(WindowFromPoint(test_cursor) == cover, "new cover wins real hit testing during partial dwell");
    Tick();
    test_clock += 500;
    Tick();
    CHECK(StealthHidden(window), "becoming occluded cancels an in-progress reveal dwell");
    ShowWindow(cover, SW_HIDE);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window), "uncovering starts a new full dwell instead of resuming old time");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "fresh uncovered dwell reveals after 300 milliseconds");

    LeaveAndWait();
    ShowWindow(cover, SW_SHOWNOACTIVATE);
    SetWindowPos(cover, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    PointInside(window);
    Tick();
    CHECK(StealthHidden(window), "covered target remains hidden before it is made topmost");
    CHECK(SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE), "make only the isolated target topmost");
    CHECK(WindowFromPoint(test_cursor) == window, "topmost target wins real hit testing over normal cover");
    Tick();
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window), "topmost target still requires its full reveal dwell");
    test_clock += 1;
    Tick();
    CHECK(!StealthHidden(window), "topmost target reveals after its own uninterrupted dwell");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "reveal does not alter topmost state");
    CHECK(StealthToggle(window), "disable occlusion test window");
}

static unsigned ActiveEventHooks(void)
{
    unsigned count = 0;
    for (size_t index = 0; index < sizeof(test_event_subscriptions) / sizeof(test_event_subscriptions[0]); ++index)
        count += !!test_event_subscriptions[index].active;
    return count;
}

static void SetImeFocus(HWND foreground, HWND focus)
{
    test_foreground_window = foreground;
    ZeroMemory(&test_foreground_gui, sizeof(test_foreground_gui));
    test_foreground_gui.hwndActive = foreground;
    test_foreground_gui.hwndFocus = focus;
    test_foreground_gui_available = TRUE;
}

static void EmitIme(DWORD event, HWND candidate, LONG object, LONG child)
{
    unsigned delivered = 0;
    for (size_t index = 0; index < sizeof(test_event_subscriptions) / sizeof(test_event_subscriptions[0]); ++index) {
        struct test_event_subscription *subscription = &test_event_subscriptions[index];
        if (subscription->active && subscription->first <= event && event <= subscription->last) {
            /* The event source thread need not own the candidate window. */
            subscription->callback(subscription->handle, event, candidate, object, child,
                GetWindowThreadProcessId(candidate, NULL) + 17, test_clock);
            ++delivered;
        }
    }
    CHECK(delivered != 0, "simulated window event reaches the production subscription callback");
}

static void TestImeCandidateEvents(void)
{
    HWND window = NewWindow(WS_EX_LAYERED);
    HWND other = NewWindow(0);
    HWND candidate = NewWindow(WS_EX_TOOLWINDOW);
    HWND replacement = NewWindow(WS_EX_TOOLWINDOW);
    HWND edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, window, NULL, GetModuleHandle(NULL), NULL);
    HWND other_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, other, NULL, GetModuleHandle(NULL), NULL);
    WINEVENTPROC old_callback = NULL;
    HWINEVENTHOOK old_hook = NULL;
    unsigned before_calls = test_event_hook_calls;
    unsigned before_unhooks;
    CHECK(edit != NULL && other_edit != NULL, "create real isolated edit controls for IME focus ownership");
    CHECK(GetWindow(candidate, GW_OWNER) == NULL, "IME candidate fixture has no application owner");
    CHECK(SetLayeredWindowAttributes(window, 0, 137, LWA_ALPHA), "set original IME test opacity");
    PointInside(window);
    CHECK(StealthToggle(window), "enable IME test window");
    LeaveAndWait();
    StealthImeInit();
    CHECK(test_event_hook_calls > before_calls && ActiveEventHooks() != 0,
        "IME initialization subscribes through the mock without installing system hooks");
    before_calls = test_event_hook_calls;
    StealthImeInit();
    CHECK(test_event_hook_calls == before_calls, "IME initialization does not duplicate an active subscription");
    SetImeFocus(window, edit);
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 137,
        "foreground composition candidate immediately restores the focused stealth window");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "composition interaction keeps the window visible with the cursor away");
    test_pointed_window = candidate;
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "interaction with an external unowned candidate keeps its focused application visible");
    test_pointed_window = NULL;
    EmitIme(EVENT_OBJECT_IME_HIDE, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(!StealthHidden(window), "ending composition preserves the normal 300-millisecond leave delay");
    test_clock += 1;
    Tick();
    CHECK(StealthHidden(window), "window hides normally after composition ends and the leave delay elapses");

    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    SetImeFocus(other, edit); /* A background thread may retain stale edit focus. */
    LeaveAndWait();
    CHECK(StealthHidden(window), "foreground changes revoke IME protection despite stale target edit focus");
    SetImeFocus(window, other_edit);
    EmitIme(EVENT_OBJECT_IME_CHANGE, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "another application's focused control does not exempt the stealth target");
    SetImeFocus(other, other_edit);
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "other foreground applications' IME events cannot reveal the target");
    SetImeFocus(window, NULL);
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "IME events with no input focus are ignored");
    SetImeFocus(window, edit);
    test_foreground_gui_available = FALSE;
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "failed foreground GUI queries do not create an IME exemption");
    SetImeFocus(window, edit);
    EmitIme(EVENT_OBJECT_IME_CHANGE, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    CHECK(!StealthHidden(window), "IME CHANGE can establish a focused candidate session without a prior SHOW");
    EmitIme(EVENT_OBJECT_IME_SHOW, replacement, OBJID_CLIENT, CHILDID_SELF);
    EmitIme(EVENT_OBJECT_IME_HIDE, candidate, OBJID_CLIENT, CHILDID_SELF);
    test_clock += 350;
    Tick();
    CHECK(!StealthHidden(window), "hiding an old candidate does not end a newer candidate session");
    EmitIme(EVENT_OBJECT_IME_HIDE, replacement, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "hiding the final candidate ends IME protection");

    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, 1);
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, 2);
    EmitIme(EVENT_OBJECT_IME_HIDE, candidate, OBJID_CLIENT, 1);
    Tick();
    test_clock += 350;
    Tick();
    CHECK(!StealthHidden(window), "candidate object IDs on the same HWND remain independent");
    EmitIme(EVENT_OBJECT_IME_HIDE, candidate, OBJID_CLIENT, 2);
    LeaveAndWait();
    CHECK(StealthHidden(window), "the final candidate object closes normally");

    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    ShowWindow(candidate, SW_HIDE);
    LeaveAndWait();
    CHECK(StealthHidden(window), "a candidate hidden without an IME_HIDE event is pruned");
    ShowWindow(candidate, SW_SHOWNOACTIVATE);
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    CHECK(DestroyWindow(candidate), "destroy isolated candidate without an IME_HIDE event");
    LeaveAndWait();
    CHECK(StealthHidden(window), "destroyed candidate windows cannot keep the application visible");
    EmitIme(EVENT_OBJECT_IME_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "stale candidate events with invalid HWNDs are ignored");

    EmitIme(EVENT_OBJECT_IME_SHOW, replacement, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    test_reused_candidate = replacement;
    test_candidate_identity_changed = TRUE;
    LeaveAndWait();
    CHECK(StealthHidden(window), "candidate HWND identity changes invalidate the old session");
    test_candidate_identity_changed = FALSE;
    test_reused_candidate = NULL;
    LeaveAndWait();
    CHECK(StealthHidden(window), "a reused candidate handle cannot resurrect an already pruned session");
    test_foreground_gui.flags = GUI_CARETBLINKING;
    test_foreground_gui.hwndCaret = edit;
    LeaveAndWait();
    CHECK(StealthHidden(window), "idle text focus and blinking caret alone do not hold the window visible");
    PointInside(window);
    test_pointed_window = replacement;
    Tick();
    test_clock += 1000;
    Tick();
    CHECK(StealthHidden(window), "an unrelated unowned popup still blocks reveal dwell without a valid IME session");
    test_pointed_window = NULL;
    PointAway();

    for (size_t index = 0; index < sizeof(test_event_subscriptions) / sizeof(test_event_subscriptions[0]); ++index)
        if (test_event_subscriptions[index].active) {
            old_callback = test_event_subscriptions[index].callback;
            old_hook = test_event_subscriptions[index].handle;
            break;
        }
    before_unhooks = test_event_unhook_calls;
    StealthImeShutdown();
    CHECK(ActiveEventHooks() == 0 && test_event_unhook_calls > before_unhooks,
        "IME shutdown removes its subscription");
    if (old_callback)
        old_callback(old_hook, EVENT_OBJECT_IME_SHOW, replacement, OBJID_CLIENT, CHILDID_SELF,
            GetWindowThreadProcessId(replacement, NULL), test_clock);
    LeaveAndWait();
    CHECK(StealthHidden(window), "queued callbacks from a stopped subscription cannot create a new session");
    test_event_hook_available = FALSE;
    before_calls = test_event_hook_calls;
    StealthImeInit();
    CHECK(test_event_hook_calls > before_calls && ActiveEventHooks() == 0,
        "an unavailable event hook fails without installing or retaining a subscription");
    LeaveAndWait();
    CHECK(StealthHidden(window), "hook unavailability preserves the normal hiding and occlusion rules");
    before_unhooks = test_event_unhook_calls;
    StealthImeShutdown();
    CHECK(test_event_unhook_calls == before_unhooks, "shutdown does not unhook a failed initialization handle");
    test_event_hook_available = TRUE;
    CHECK(StealthToggle(window), "disable IME test window");
    test_foreground_window = NULL;
    test_foreground_gui_available = FALSE;
    ZeroMemory(&test_foreground_gui, sizeof(test_foreground_gui));
}

static void TestWeTypeCandidateFallback(void)
{
    const DWORD candidate_style = WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED;
    const DWORD required_flags[] = { WS_EX_NOACTIVATE, WS_EX_TOOLWINDOW, WS_EX_TOPMOST, WS_EX_LAYERED };
    HWND window = NewWindow(WS_EX_LAYERED);
    HWND other = NewWindow(0);
    HWND candidate = NewClassWindow("wetype.flutter.setting", candidate_style);
    HWND prefix = NewClassWindow("wetype.flutter.setting.extra", candidate_style);
    HWND setting = NewClassWindow("wetype.flutter.setting", candidate_style);
    HWND ordinary = NewWindow(candidate_style);
    HWND edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, window, NULL, GetModuleHandle(NULL), NULL);
    HWND other_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, other, NULL, GetModuleHandle(NULL), NULL);
    unsigned before_unhooks;
    CHECK(edit != NULL && other_edit != NULL, "create isolated WeType focus controls");
    CHECK(GetWindow(candidate, GW_OWNER) == NULL, "WeType fixture is an external unowned window");
    SetWindowLongPtr(setting, GWL_EXSTYLE, candidate_style & ~WS_EX_NOACTIVATE);
    SetLayeredWindowAttributes(window, 0, 119, LWA_ALPHA);
    PointInside(window);
    CHECK(StealthToggle(window), "enable WeType fallback test target");
    LeaveAndWait();
    SetImeFocus(window, edit);
    StealthImeInit();
    CHECK(ActiveEventHooks() == 2, "official IME and generic candidate hooks are both subscribed");

    EmitIme(EVENT_OBJECT_SHOW, ordinary, OBJID_WINDOW, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "ordinary windows with candidate styles do not receive an IME exemption");
    EmitIme(EVENT_OBJECT_SHOW, prefix, OBJID_WINDOW, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "similar WeType class prefixes do not receive an exemption");
    EmitIme(EVENT_OBJECT_SHOW, setting, OBJID_WINDOW, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "activatable WeType settings windows are excluded");
    for (size_t index = 0; index < sizeof(required_flags) / sizeof(required_flags[0]); ++index) {
        SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style & ~required_flags[index]);
        if (required_flags[index] == WS_EX_TOPMOST)
            SetWindowPos(candidate, HWND_NOTOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
        LeaveAndWait();
        CHECK(StealthHidden(window), "every observed candidate style flag is required");
        SetWindowPos(candidate, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style);
    SetWindowPos(candidate, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "zero-size WeType utility windows are excluded");
    SetWindowPos(candidate, NULL, 0, 0, 320, 240, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "generic candidate matching requires OBJID_WINDOW");
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, 1);
    LeaveAndWait();
    CHECK(StealthHidden(window), "generic candidate matching requires CHILDID_SELF");
    SetImeFocus(other, other_edit);
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "another foreground application's WeType candidate cannot protect this target");

    SetImeFocus(window, edit);
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    Tick();
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 119,
        "real exact-class unowned WeType candidate protects its focused application's opacity");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "generic WeType candidate interaction keeps the target visible");
    SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style & ~WS_EX_NOACTIVATE);
    LeaveAndWait();
    CHECK(StealthHidden(window), "a candidate becoming an activatable settings window is pruned");
    SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style);
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    Tick();
    SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style & ~WS_EX_TOOLWINDOW);
    EmitIme(EVENT_OBJECT_HIDE, candidate, OBJID_WINDOW, CHILDID_SELF);
    SetWindowLongPtr(candidate, GWL_EXSTYLE, candidate_style);
    LeaveAndWait();
    CHECK(StealthHidden(window), "generic HIDE clears the record even when candidate classification has changed");

    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    EmitIme(EVENT_OBJECT_IME_SHOW, ordinary, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    EmitIme(EVENT_OBJECT_HIDE, candidate, OBJID_WINDOW, CHILDID_SELF);
    test_clock += 350;
    Tick();
    CHECK(!StealthHidden(window), "hiding WeType UI does not clear another HWND's official IME session");
    EmitIme(EVENT_OBJECT_IME_HIDE, ordinary, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "clearing both candidate channels resumes ordinary hiding");
    EmitIme(EVENT_OBJECT_SHOW, candidate, OBJID_WINDOW, CHILDID_SELF);
    Tick();
    CHECK(DestroyWindow(candidate), "destroy an isolated WeType candidate without sending generic HIDE");
    LeaveAndWait();
    CHECK(StealthHidden(window), "destroyed WeType candidates are pruned without a hide event");

    before_unhooks = test_event_unhook_calls;
    StealthImeShutdown();
    CHECK(ActiveEventHooks() == 0 && test_event_unhook_calls == before_unhooks + 2,
        "shutdown releases both event hooks without leaking either subscription");
    test_generic_event_hook_available = FALSE;
    StealthImeInit();
    CHECK(ActiveEventHooks() == 1, "generic hook failure leaves the official IME subscription usable");
    EmitIme(EVENT_OBJECT_IME_SHOW, ordinary, OBJID_CLIENT, CHILDID_SELF);
    Tick();
    CHECK(!StealthHidden(window), "official IME protection still works when the generic hook is unavailable");
    EmitIme(EVENT_OBJECT_IME_HIDE, ordinary, OBJID_CLIENT, CHILDID_SELF);
    LeaveAndWait();
    CHECK(StealthHidden(window), "official IME completion still resumes hiding after generic hook failure");
    before_unhooks = test_event_unhook_calls;
    StealthImeShutdown();
    CHECK(ActiveEventHooks() == 0 && test_event_unhook_calls == before_unhooks + 1,
        "partial initialization shutdown only releases the hook that succeeded");
    test_generic_event_hook_available = TRUE;
    CHECK(StealthToggle(window), "disable WeType fallback test target");
    test_foreground_window = NULL;
    test_foreground_gui_available = FALSE;
    ZeroMemory(&test_foreground_gui, sizeof(test_foreground_gui));
}

static void TestUnsupportedAndUntrackedWindows(void)
{
    HWND transparent = NewWindow(WS_EX_TRANSPARENT);
    HWND own_layered = NewWindow(WS_EX_LAYERED);
    HWND normal = NewWindow(0);
    BYTE alpha = 0;
    CHECK(!StealthToggle(transparent), "reject an originally click-through window");
    CHECK(!StealthEnabled(transparent), "rejected window is never tracked");
    CHECK(!StealthToggle(own_layered), "reject layered windows with unreadable attributes");
    CHECK(GetWindowLongPtr(own_layered, GWL_EXSTYLE) & WS_EX_LAYERED,
        "rejection does not remove an application's layered style");
    CHECK(!StealthToggle(NULL), "reject null target");
    CHECK(!StealthGetAlpha(normal, &alpha), "untracked alpha getter lets the caller fall back");
    CHECK(!StealthSetAlpha(normal, 100), "untracked alpha setter lets the caller fall back");
    CHECK(!(GetWindowLongPtr(normal, GWL_EXSTYLE) & WS_EX_LAYERED),
        "untracked alpha setter does not modify the target");
}

int main(void)
{
    WNDCLASSA window_class = {0};
    HWND initial_foreground = GetForegroundWindow();
    window_class.lpfnWndProc = DefWindowProcA;
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.lpszClassName = "AltSnapStealthRegression";
    if (!RegisterClassA(&window_class)) {
        fprintf(stderr, "RegisterClass failed: %lu\n", GetLastError());
        return 2;
    }
    window_class.lpszClassName = "wetype.flutter.setting";
    CHECK(RegisterClassA(&window_class), "register exact WeType candidate fixture class");
    window_class.lpszClassName = "wetype.flutter.setting.extra";
    CHECK(RegisterClassA(&window_class), "register similar-prefix negative fixture class");

    TestNormalWindow();
    TestTransparencyAndTopmost();
    TestMenuDragAndMovement();
    TestMultipleWindowsAndCleanup();
    TestNativeHideMinimizeAndPopup();
    TestTemporaryOpacityAndOwnership();
    TestOpacityChangesWhileHidden();
    TestNativeMenuDragAndCapture();
    TestColorKeyOnlyAndClockWrap();
    TestInterruptedRevealDwell();
    TestOcclusionAndTopmostReveal();
    TestImeCandidateEvents();
    TestWeTypeCandidateFallback();
    TestUnsupportedAndUntrackedWindows();

    StealthRestoreAll();
    for (size_t index = 0; index < test_window_count; ++index)
        if (IsWindow(test_windows[index])) DestroyWindow(test_windows[index]);
    CHECK(GetForegroundWindow() == initial_foreground,
        "tests never activate a window or change the user's foreground application");
    UnregisterClassA("wetype.flutter.setting.extra", window_class.hInstance);
    UnregisterClassA("wetype.flutter.setting", window_class.hInstance);
    UnregisterClassA("AltSnapStealthRegression", window_class.hInstance);
    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
