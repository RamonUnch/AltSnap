/*
 * Integration tests for the production action, timer, MoveTrans and pin wiring.
 * All windows belong to this test and are offscreen. The test never calls Load,
 * installs system hooks, sends input, moves the real pointer or activates a window.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include "../hooks.h"

static POINT test_cursor;
static DWORD test_clock = 1000;

static BOOL TestGetCursorPos(LPPOINT point)
{
    *point = test_cursor;
    return TRUE;
}

static DWORD TestGetTickCount(void)
{
    return test_clock;
}

#define GetCursorPos TestGetCursorPos
#define GetTickCount TestGetTickCount
#include "../hooks.c"
#undef GetCursorPos
#undef GetTickCount

static unsigned checks;
static unsigned failures;
static HWND test_windows[16];
static size_t test_window_count;

#define CHECK(condition, message) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); \
    } \
} while (0)

static HWND NewWindow(const TCHAR *classname, HWND owner, DWORD style, BOOL visible)
{
    HWND window = CreateWindowEx(style | WS_EX_NOACTIVATE,
        classname, TEXT("AltSnap hooks regression (offscreen)"), WS_POPUP,
        -23000 - (int)test_window_count * 500, -23000, 320, 240,
        owner, NULL, GetModuleHandle(NULL), NULL);
    CHECK(window != NULL, "create isolated integration-test window");
    if (!window) return NULL;
    test_windows[test_window_count++] = window;
    if (visible) ShowWindow(window, SW_SHOWNOACTIVATE);
    return window;
}

static void PointInside(HWND window)
{
    RECT rectangle;
    GetWindowRect(window, &rectangle);
    test_cursor.x = rectangle.left + 40;
    test_cursor.y = rectangle.top + 40;
}

static void Tick(void)
{
    TimerWindowProc(g_mainhwnd, WM_TIMER, STEALTH_TIMER, test_clock);
}

static void LeaveAndWait(void)
{
    test_cursor.x = -1000;
    test_cursor.y = -1000;
    Tick();
    test_clock += 350;
    Tick();
}

static BYTE ActualAlpha(HWND window)
{
    BYTE alpha = 255;
    DWORD flags = 0;
    if (!GetLayeredWindowAttributes(window, NULL, &alpha, &flags)
    || !(flags & LWA_ALPHA)) return 255;
    return alpha;
}

static void Toggle(HWND window)
{
    const action_t action = { AC_STEALTH, 0, 0 };
    SClickActions(window, action);
}

static void TestMoveTransBeforeMovement(void)
{
    HWND window = NewWindow(TEXT("AltSnapHooksRegression"), NULL,
        WS_EX_LAYERED | WS_EX_TOPMOST, TRUE);
    BYTE alpha = 0;
    CHECK(SetLayeredWindowAttributes(window, 0, 173, LWA_ALPHA),
        "set original integration-test opacity");
    conf.MoveTrans = 80;
    PointInside(window);
    Toggle(window);
    CHECK(StealthEnabled(window), "production SClickActions enables stealth");

    state.hwnd = window;
    state.action = k_action_move;
    state.moving = 0; /* Alt+mouse is held down, but no movement has happened. */
    SetWindowTrans(window);
    CHECK(ActualAlpha(window) == 80, "production MoveTrans applies temporary opacity");
    LeaveAndWait();
    CHECK(!StealthHidden(window), "held movement action stays visible before first mouse move");
    CHECK(ActualAlpha(window) == 80, "timer preserves temporary MoveTrans opacity");
    CHECK(StealthGetAlpha(window, &alpha) && alpha == 173,
        "timer does not capture pre-movement MoveTrans as normal opacity");
    SetWindowTrans(NULL);
    CHECK(ActualAlpha(window) == 173, "production drag completion restores original opacity");

    state.action = k_action_none;
    g_InFinishMovement = 1;
    SetWindowTrans(window);
    LeaveAndWait();
    CHECK(!StealthHidden(window), "pending finish worker also prevents hiding");
    CHECK(StealthGetAlpha(window, &alpha) && alpha == 173,
        "pending finish worker does not capture temporary opacity");
    SetWindowTrans(NULL);
    g_InFinishMovement = 0;
    LeaveAndWait();
    CHECK(StealthHidden(window), "production timer hides after movement and worker finish");
    PointInside(window);
    Tick();
    CHECK(ActualAlpha(window) == 173, "production timer restores normal opacity on approach");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "action and timer wiring preserve topmost state");

    conf.MinAlpha = 32;
    {
        const action_t transparency = { AC_TRANSPARENCY, ACFL_SET, 101 };
        SClickActions(window, transparency);
    }
    LeaveAndWait();
    PointInside(window);
    Tick();
    CHECK(ActualAlpha(window) == 101,
        "production transparency shortcut and stealth preserve the new opacity");
    conf.AlphaDelta = 16;
    state.shift = 0;
    CHECK(ActionTransparency(window, -WHEEL_DELTA), "production transparency wheel action succeeds");
    LeaveAndWait();
    PointInside(window);
    Tick();
    CHECK(ActualAlpha(window) == 85,
        "production transparency wheel action uses logical, not hidden alpha");
    Toggle(window);
    CHECK(!StealthEnabled(window), "production SClickActions disables stealth");
    CHECK(ActualAlpha(window) == 85, "disabling keeps the latest selected opacity");
    state.hwnd = NULL;
    conf.MoveTrans = 0;
}

static void TestPinOwnershipAndDeferredRestoration(void)
{
    HWND owner = NewWindow(TEXT("AltSnapHooksRegression"), NULL, WS_EX_TOPMOST, TRUE);
    HWND visible_pin = NewWindow(TEXT("AltSnap-Pin"), owner, WS_EX_TOOLWINDOW, TRUE);
    HWND hidden_pin = NewWindow(TEXT("AltSnap-Pin"), owner, WS_EX_TOOLWINDOW, FALSE);
    BOOL keep_timer = FALSE;
    PointInside(owner);
    Toggle(owner);
    Tick();
    CHECK(IsWindowVisible(visible_pin), "visible pin is initially untouched");
    CHECK(!IsWindowVisible(hidden_pin), "initially hidden unrelated pin stays hidden");
    LeaveAndWait();
    CHECK(StealthHidden(owner), "production timer hides the pin owner");
    CHECK(!IsWindowVisible(visible_pin) && GetProp(visible_pin, STEALTH_PIN_PROP),
        "production timer hides and marks its own visible pin");
    CHECK(!IsWindowVisible(hidden_pin) && !GetProp(hidden_pin, STEALTH_PIN_PROP),
        "production timer does not claim an already hidden pin");
    PointInside(owner);
    Tick();
    CHECK(IsWindowVisible(visible_pin) && !GetProp(visible_pin, STEALTH_PIN_PROP),
        "approach restores the pin hidden by stealth and removes its marker");
    CHECK(!IsWindowVisible(hidden_pin), "restoring one pin does not show an unrelated hidden pin");
    CHECK(GetWindowLongPtr(owner, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "pin synchronization does not cancel owner topmost state");

    LeaveAndWait();
    ShowWindow(owner, SW_HIDE);
    Toggle(owner);
    CHECK(!StealthEnabled(owner), "disable last stealth target while its owner is natively hidden");
    UpdateStealthPinWindow(visible_pin, (LPARAM)&keep_timer);
    CHECK(keep_timer, "deferred pin restoration keeps polling after last target is disabled");
    CHECK(!IsWindowVisible(visible_pin) && GetProp(visible_pin, STEALTH_PIN_PROP),
        "native-hidden owner retains its pending pin marker");
    ShowWindow(owner, SW_SHOWNOACTIVATE);
    Tick();
    CHECK(IsWindowVisible(visible_pin) && !GetProp(visible_pin, STEALTH_PIN_PROP),
        "production timer restores deferred pin once its owner is visible");
    CHECK(!IsWindowVisible(hidden_pin), "deferred restoration still leaves unrelated pins hidden");
}

int main(void)
{
    WNDCLASS window_class = {0};
    HWND foreground = GetForegroundWindow();
    hinstDLL = GetModuleHandle(NULL);
    window_class.lpfnWndProc = DefWindowProc;
    window_class.hInstance = hinstDLL;
    window_class.lpszClassName = TEXT("AltSnapHooksRegression");
    if (!RegisterClass(&window_class)) return 2;
    window_class.lpszClassName = TEXT("AltSnap-Pin");
    if (!RegisterClass(&window_class)) return 2;
    g_mainhwnd = NewWindow(TEXT("AltSnapHooksRegression"), NULL, 0, FALSE);

    TestMoveTransBeforeMovement();
    TestPinOwnershipAndDeferredRestoration();

    KillAllTimers();
    StealthRestoreAll();
    for (size_t index = test_window_count; index > 0; --index)
        if (IsWindow(test_windows[index - 1])) DestroyWindow(test_windows[index - 1]);
    CHECK(GetForegroundWindow() == foreground, "integration tests preserve the user's foreground application");
    UnregisterClass(TEXT("AltSnap-Pin"), hinstDLL);
    UnregisterClass(TEXT("AltSnapHooksRegression"), hinstDLL);
    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
