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
static HWND test_foreground_window;
static HWND test_input_focus;
static WINEVENTPROC test_ime_callback;
static HWINEVENTHOOK test_ime_hook;
static WINEVENTPROC test_generic_callback;
static HWINEVENTHOOK test_generic_hook;
static unsigned test_ime_subscriptions;
static unsigned test_ime_unsubscriptions;

static BOOL TestGetCursorPos(LPPOINT point)
{
    *point = test_cursor;
    return TRUE;
}

static DWORD TestGetTickCount(void)
{
    return test_clock;
}

static HWND TestGetForegroundWindow(void)
{
    return test_foreground_window;
}

static BOOL TestGetGUIThreadInfoL(DWORD thread, GUITHREADINFO *info)
{
    if (thread || !test_foreground_window) return FALSE;
    ZeroMemory(info, sizeof(*info));
    info->cbSize = sizeof(*info);
    info->hwndActive = test_foreground_window;
    info->hwndFocus = test_input_focus;
    return TRUE;
}

static HWINEVENTHOOK TestSetWinEventHookL(DWORD first, DWORD last, HMODULE module,
    WINEVENTPROC callback, DWORD process, DWORD thread, DWORD flags)
{
    (void)last;
    (void)module;
    (void)process;
    (void)thread;
    (void)flags;
    ++test_ime_subscriptions;
    if (first == EVENT_OBJECT_SHOW) {
        test_generic_callback = callback;
        test_generic_hook = (HWINEVENTHOOK)(UINT_PTR)(0x123 + test_ime_subscriptions);
        return test_generic_hook;
    }
    test_ime_callback = callback;
    test_ime_hook = (HWINEVENTHOOK)(UINT_PTR)(0x123 + test_ime_subscriptions);
    return test_ime_hook;
}

static BOOL TestUnhookWinEventL(HWINEVENTHOOK hook)
{
    if (hook == test_ime_hook) {
        ++test_ime_unsubscriptions;
        test_ime_callback = NULL;
        test_ime_hook = NULL;
        return TRUE;
    }
    if (hook == test_generic_hook) {
        ++test_ime_unsubscriptions;
        test_generic_callback = NULL;
        test_generic_hook = NULL;
        return TRUE;
    }
    return FALSE;
}

#define GetCursorPos TestGetCursorPos
#define GetTickCount TestGetTickCount
#define GetForegroundWindow TestGetForegroundWindow
#define GetGUIThreadInfoL TestGetGUIThreadInfoL
#define SetWinEventHookL TestSetWinEventHookL
#define UnhookWinEventL TestUnhookWinEventL
#include "../hooks.c"
#undef GetCursorPos
#undef GetTickCount
#undef GetForegroundWindow
#undef GetGUIThreadInfoL
#undef SetWinEventHookL
#undef UnhookWinEventL

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
    CHECK(StealthHidden(window) && ActualAlpha(window) == 1,
        "production timer starts reveal dwell without immediately displaying the window");
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(window) && StealthGetAlpha(window, &alpha) && alpha == 173,
        "production timer keeps original opacity safe while reveal dwell is pending");
    test_clock += 1;
    Tick();
    CHECK(ActualAlpha(window) == 173, "production timer restores normal opacity after 300-millisecond dwell");
    CHECK(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_TOPMOST,
        "action and timer wiring preserve topmost state");

    conf.MinAlpha = 32;
    {
        const action_t transparency = { AC_TRANSPARENCY, ACFL_SET, 101 };
        SClickActions(window, transparency);
    }
    LeaveAndWait();
    DwellInside(window);
    CHECK(ActualAlpha(window) == 101,
        "production transparency shortcut and stealth preserve the new opacity");
    conf.AlphaDelta = 16;
    state.shift = 0;
    CHECK(ActionTransparency(window, -WHEEL_DELTA), "production transparency wheel action succeeds");
    LeaveAndWait();
    DwellInside(window);
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
    CHECK(!IsWindowVisible(visible_pin) && GetProp(visible_pin, STEALTH_PIN_PROP),
        "pin remains hidden when reveal dwell starts");
    test_clock += 299;
    Tick();
    CHECK(StealthHidden(owner) && !IsWindowVisible(visible_pin),
        "pin stays hidden until its owner completes the full dwell");
    test_clock += 1;
    Tick();
    CHECK(IsWindowVisible(visible_pin) && !GetProp(visible_pin, STEALTH_PIN_PROP),
        "completed dwell restores the pin hidden by stealth and removes its marker");
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

static void TestEnableStealthDuringMovement(DWORD initial_style, BYTE normal_alpha)
{
    HWND window = NewWindow(TEXT("AltSnapHooksRegression"), NULL, initial_style, TRUE);
    BYTE saved_alpha = 0;
    if (initial_style & WS_EX_LAYERED)
        CHECK(SetLayeredWindowAttributes(window, 0, normal_alpha, LWA_ALPHA),
            "set original opacity before enabling stealth during a drag");
    conf.MoveTrans = 96;
    PointInside(window);
    state.hwnd = window;
    state.action = k_action_move;
    state.moving = NOW_MOVING;
    SetWindowTrans(window);
    CHECK(ActualAlpha(window) == 96, "MoveTrans is active before stealth is first enabled");

    Toggle(window);
    CHECK(StealthEnabled(window), "while-moving action can first enable stealth");
    CHECK(StealthGetAlpha(window, &saved_alpha) && saved_alpha == normal_alpha,
        "enabling stealth while moving saves original opacity rather than MoveTrans");
    CHECK(ActualAlpha(window) == 96, "enabling stealth preserves temporary drag opacity");
    LeaveAndWait();
    CHECK(!StealthHidden(window), "newly enabled moving window remains visible");

    SetWindowTrans(NULL);
    state.action = k_action_none;
    state.moving = 0;
    CHECK(ActualAlpha(window) == normal_alpha,
        "drag completion restores opacity from before stealth was enabled");
    CHECK(!!(GetWindowLongPtr(window, GWL_EXSTYLE) & WS_EX_LAYERED)
        == !!(initial_style & WS_EX_LAYERED),
        "drag completion restores the original layered drawing mode");
    Toggle(window);
    CHECK(!StealthEnabled(window), "disable window enabled through a while-moving action");
    state.hwnd = NULL;
    conf.MoveTrans = 0;
}

static void TestImeProtectionThroughProductionTimer(void)
{
    HWND window = NewWindow(TEXT("AltSnapHooksRegression"), NULL, WS_EX_LAYERED, TRUE);
    HWND candidate = NewWindow(TEXT("AltSnapHooksRegression"), NULL, WS_EX_TOOLWINDOW, TRUE);
    HWND edit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, window, NULL, hinstDLL, NULL);
    CHECK(edit != NULL, "create isolated input control for production IME timer integration");
    SetLayeredWindowAttributes(window, 0, 145, LWA_ALPHA);
    PointInside(window);
    Toggle(window);
    LeaveAndWait();
    CHECK(StealthHidden(window), "IME integration target starts hidden through production timer");
    test_foreground_window = window;
    test_input_focus = edit;
    StealthImeInit();
    CHECK(test_ime_callback != NULL && test_generic_callback != NULL && test_ime_subscriptions == 2,
        "both IME subscriptions are intercepted without installing desktop hooks");
    if (test_ime_callback)
        test_ime_callback(test_ime_hook, EVENT_OBJECT_IME_SHOW, candidate,
            OBJID_CLIENT, CHILDID_SELF, GetWindowThreadProcessId(candidate, NULL), test_clock);
    Tick();
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 145,
        "production timer immediately restores focused window for IME interaction");
    test_clock += 1000;
    Tick();
    CHECK(!StealthHidden(window), "production timer keeps composition visible with the pointer away");
    if (test_ime_callback)
        test_ime_callback(test_ime_hook, EVENT_OBJECT_IME_HIDE, candidate,
            OBJID_CLIENT, CHILDID_SELF, GetWindowThreadProcessId(candidate, NULL), test_clock);
    Tick();
    test_clock += 299;
    Tick();
    CHECK(!StealthHidden(window), "production IME completion preserves normal leave delay");
    test_clock += 1;
    Tick();
    CHECK(StealthHidden(window), "production timer resumes normal hiding after composition completes");
    StealthImeShutdown();
    CHECK(test_ime_unsubscriptions == 2 && test_ime_callback == NULL && test_generic_callback == NULL,
        "IME integration shutdown removes both intercepted subscriptions");
    test_foreground_window = NULL;
    test_input_focus = NULL;
    Toggle(window);
}

static void TestWeTypeThroughProductionTimer(void)
{
    HWND window = NewWindow(TEXT("AltSnapHooksRegression"), NULL, WS_EX_LAYERED, TRUE);
    HWND candidate = NewWindow(TEXT("wetype.flutter.setting"), NULL,
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED, TRUE);
    HWND edit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE,
        10, 10, 100, 30, window, NULL, hinstDLL, NULL);
    unsigned before_unhooks = test_ime_unsubscriptions;
    CHECK(edit != NULL, "create isolated WeType timer-integration edit control");
    SetLayeredWindowAttributes(window, 0, 126, LWA_ALPHA);
    PointInside(window);
    Toggle(window);
    LeaveAndWait();
    test_foreground_window = window;
    test_input_focus = edit;
    StealthImeInit();
    CHECK(test_generic_callback != NULL && test_generic_hook != test_ime_hook,
        "generic WeType subscription has its own intercepted hook handle");
    if (test_generic_callback)
        test_generic_callback(test_generic_hook, EVENT_OBJECT_SHOW, candidate,
            OBJID_WINDOW, CHILDID_SELF, GetWindowThreadProcessId(candidate, NULL), test_clock);
    Tick();
    CHECK(!StealthHidden(window) && ActualAlpha(window) == 126,
        "production timer accepts the scoped WeType generic SHOW fallback");
    if (test_generic_callback)
        test_generic_callback(test_generic_hook, EVENT_OBJECT_HIDE, candidate,
            OBJID_WINDOW, CHILDID_SELF, GetWindowThreadProcessId(candidate, NULL), test_clock);
    LeaveAndWait();
    CHECK(StealthHidden(window), "production timer resumes hiding after WeType generic HIDE");
    StealthImeShutdown();
    CHECK(test_ime_unsubscriptions == before_unhooks + 2,
        "WeType integration shutdown releases both distinct event hooks");
    test_foreground_window = NULL;
    test_input_focus = NULL;
    Toggle(window);
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
    window_class.lpszClassName = TEXT("wetype.flutter.setting");
    if (!RegisterClass(&window_class)) return 2;
    g_mainhwnd = NewWindow(TEXT("AltSnapHooksRegression"), NULL, 0, FALSE);

    TestMoveTransBeforeMovement();
    TestEnableStealthDuringMovement(0, 255);
    TestEnableStealthDuringMovement(WS_EX_LAYERED, 128);
    TestPinOwnershipAndDeferredRestoration();
    TestImeProtectionThroughProductionTimer();
    TestWeTypeThroughProductionTimer();

    KillAllTimers();
    StealthRestoreAll();
    for (size_t index = test_window_count; index > 0; --index)
        if (IsWindow(test_windows[index - 1])) DestroyWindow(test_windows[index - 1]);
    CHECK(GetForegroundWindow() == foreground, "integration tests preserve the user's foreground application");
    UnregisterClass(TEXT("AltSnap-Pin"), hinstDLL);
    UnregisterClass(TEXT("wetype.flutter.setting"), hinstDLL);
    UnregisterClass(TEXT("AltSnapHooksRegression"), hinstDLL);
    printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
