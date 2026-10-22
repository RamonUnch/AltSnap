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
static DWORD test_clock = 1000;
static unsigned checks;
static unsigned failures;
static HWND test_windows[32];
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
    (void)thread;
    *info = test_gui;
    info->cbSize = sizeof(*info);
    return test_gui_available;
}

#define GetCursorPos TestGetCursorPos
#define GetTickCount TestGetTickCount
#define WindowFromPoint TestWindowFromPoint
#define GetGUIThreadInfoL TestGetGUIThreadInfoL
#include "../stealth.c"
#undef GetCursorPos
#undef GetTickCount
#undef WindowFromPoint
#undef GetGUIThreadInfoL

#define CHECK(condition, message) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); \
    } \
} while (0)

static HWND NewWindow(DWORD extended_style)
{
    HWND window = CreateWindowExA(extended_style | WS_EX_NOACTIVATE,
        "AltSnapStealthRegression", "AltSnap stealth test (offscreen)",
        WS_POPUP, -20000 - (int)test_window_count * 500, -20000, 320, 240,
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
    CHECK(!StealthHidden(window), "cursor near the edge reveals the window");
    CHECK(ActualAlpha(window) == 255, "normal opacity is restored on approach");

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
    PointInside(window);
    Tick();
    CHECK(ActualAlpha(window) == 89, "approaching restores the latest chosen opacity");
    CHECK(GetLayeredWindowAttributes(window, &actual_key, &alpha, &flags),
        "read restored layered attributes");
    CHECK(actual_key == key && flags == (LWA_ALPHA | LWA_COLORKEY),
        "revealing retains original color-key behavior");

    CHECK(SetLayeredWindowAttributes(window, key, 173, LWA_ALPHA | LWA_COLORKEY),
        "simulate an app changing its visible opacity");
    LeaveAndWait();
    PointInside(window);
    Tick();
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
    StealthTick(window, NULL);
    CHECK(!StealthHidden(window), "action menu target remains visible");
    test_clock += 1000;
    StealthTick(window, NULL);
    CHECK(!StealthHidden(window), "action menu prevents hiding while cursor is away");
    LeaveAndWait();
    CHECK(StealthHidden(window), "window hides again after menu closes");
    StealthTick(NULL, window);
    CHECK(!StealthHidden(window), "moving target is revealed");
    test_clock += 1000;
    StealthTick(NULL, window);
    CHECK(!StealthHidden(window), "dragging prevents hiding while cursor is away");

    CHECK(SetWindowPos(window, NULL, -25000, -25000, 400, 300,
        SWP_NOACTIVATE | SWP_NOZORDER), "move and resize isolated window");
    LeaveAndWait();
    PointInside(window);
    Tick();
    CHECK(!StealthHidden(window), "approach detection follows moved and resized windows");
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
    CHECK(!StealthHidden(first) && StealthHidden(second), "approaching one does not reveal another");

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
    CHECK(!StealthHidden(window), "owned popup outside the main rectangle reveals its owner");
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
    PointInside(window);
    Tick();
    CHECK(ActualAlpha(window) == 199, "subsequent approach restores the app's new opacity");
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
    PointInside(window);
    Tick();
    CHECK(GetLayeredWindowAttributes(window, &key, NULL, &flags)
        && key == original_key && flags == LWA_COLORKEY,
        "revealing restores color-key-only flags without introducing alpha");
    CHECK(StealthToggle(window), "disable color-key-only layered window");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_LAYERED,
        "original color-key-only layered style remains enabled");
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

    TestNormalWindow();
    TestTransparencyAndTopmost();
    TestMenuDragAndMovement();
    TestMultipleWindowsAndCleanup();
    TestNativeHideMinimizeAndPopup();
    TestTemporaryOpacityAndOwnership();
    TestOpacityChangesWhileHidden();
    TestNativeMenuDragAndCapture();
    TestColorKeyOnlyAndClockWrap();
    TestUnsupportedAndUntrackedWindows();

    StealthRestoreAll();
    for (size_t index = 0; index < test_window_count; ++index)
        if (IsWindow(test_windows[index])) DestroyWindow(test_windows[index]);
    CHECK(GetForegroundWindow() == initial_foreground,
        "tests never activate a window or change the user's foreground application");
    UnregisterClassA(window_class.lpszClassName, window_class.hInstance);
    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
