/*
 * Action-menu integration regression test. No popup is displayed and no input
 * is sent: TrackPopupMenu, PostMessage and GetCursorPos are replaced below.
 * Build and run from the repository root with MinGW-w64:
 *   gcc -std=c99 -O1 -ffunction-sections -fdata-sections -Wl,--gc-sections \
 *       -o tests/menu_test.exe tests/menu_test.c -lcomctl32 -lshell32 \
 *       -ladvapi32 -lgdi32 -luser32
 *   tests/menu_test.exe
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>

static BOOL WINAPI TestTrackPopupMenu(HMENU, UINT, int, int, int, HWND,
                                     const RECT *);
static BOOL WINAPI TestPostMessage(HWND, UINT, WPARAM, LPARAM);
static BOOL WINAPI TestGetCursorPos(LPPOINT);

#undef PostMessage
#define TrackPopupMenu TestTrackPopupMenu
#define PostMessage TestPostMessage
#define GetCursorPos TestGetCursorPos
#include "../altsnap.c"
#undef TrackPopupMenu
#undef PostMessage
#undef GetCursorPos

struct captured_item {
    UINT id;
    UINT type;
    UINT state;
    TCHAR label[128];
};

static struct captured_item captured[32];
static int captured_count;
static unsigned popup_calls;
static unsigned close_posts;
static unsigned checks;
static unsigned failures;

#define CHECK(condition, message) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); \
    } \
} while (0)

static BOOL WINAPI TestTrackPopupMenu(HMENU menu, UINT flags, int x, int y,
                                     int reserved, HWND owner, const RECT *rect)
{
    int index;
    (void)flags;
    (void)x;
    (void)y;
    (void)reserved;
    (void)owner;
    (void)rect;
    ++popup_calls;
    captured_count = GetMenuItemCount(menu);
    CHECK(captured_count >= 0 && captured_count <= (int)ARR_SZ(captured),
          "menu fits snapshot storage");
    for (index = 0; index < captured_count && index < (int)ARR_SZ(captured); ++index) {
        MENUITEMINFO info = { sizeof(info) };
        info.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STATE | MIIM_STRING;
        info.dwTypeData = captured[index].label;
        info.cch = ARR_SZ(captured[index].label);
        CHECK(GetMenuItemInfo(menu, index, TRUE, &info), "read menu item");
        captured[index].id = info.wID;
        captured[index].type = info.fType;
        captured[index].state = info.fState;
    }
    return TRUE;
}

static BOOL WINAPI TestPostMessage(HWND owner, UINT message, WPARAM wp, LPARAM lp)
{
    (void)owner;
    (void)wp;
    (void)lp;
    if (message == WM_CLOSE)
        ++close_posts;
    return TRUE;
}

static BOOL WINAPI TestGetCursorPos(LPPOINT point)
{
    point->x = point->y = 100;
    return TRUE;
}

static void CaptureMenu(DWORD mask, LPARAM state_flags)
{
    unsigned previous_calls = popup_calls;
    unsigned previous_posts = close_posts;
    ACMenuItems = mask;
    ShowSClickMenu(NULL, LP_CURSORPOS | state_flags);
    CHECK(popup_calls == previous_calls + 1, "capture one menu without showing it");
    CHECK(close_posts == previous_posts + 1, "menu close message is intercepted");
}

int main(void)
{
    const DWORD stealth_bit = 1UL << 20;
    struct captured_item original[20];
    int index;
    TCHAR path[MAX_PATH];
    action_t action = MapActionW(L"Stealth");
    CHECK(action.ac == AC_STEALTH, "Stealth action string resolves to its action ID");
    CHECK(action.fl == 0 && action.wp == 0, "Stealth has no unintended parameters");

    CaptureMenu(stealth_bit - 1, LP_NOALTACTION);
    CHECK(captured_count == 20, "legacy full mask retains all original entries");
    for (index = 0; index < 20; ++index)
        original[index] = captured[index];

    CaptureMenu((DWORD)-1, LP_NOALTACTION);
    CHECK(captured_count == 21, "default mask adds exactly one entry");
    CHECK(captured[1].id == (UINT)(AC_STEALTH << 16),
          "Stealth appears immediately after always on top");
    CHECK(!lstrcmp(captured[1].label, en_US.InputActionStealth),
          "default menu uses the English Stealth label");
    CHECK(!(captured[1].state & MFS_CHECKED), "disabled Stealth is unchecked");
    for (index = 0; index < 20; ++index) {
        int position = index + (index > 0);
        CHECK(captured[position].id == original[index].id,
              "adding Stealth preserves original action IDs and order");
        CHECK(captured[position].type == original[index].type,
              "adding Stealth preserves original separators");
        CHECK(!lstrcmp(captured[position].label, original[index].label),
              "adding Stealth preserves original labels");
    }

    CaptureMenu(stealth_bit, LP_STEALTH);
    CHECK(captured_count == 1, "bit 20 can enable only Stealth");
    CHECK(captured[0].id == (UINT)(AC_STEALTH << 16), "bit 20 selects Stealth");
    CHECK(captured[0].state & MFS_CHECKED, "enabled Stealth is checked");

    for (index = 0; index < 20; ++index) {
        CaptureMenu(1UL << index, LP_NOALTACTION);
        CHECK(captured_count == 1, "each legacy mask bit selects exactly its original item");
        CHECK(captured[0].id == original[index].id,
              "legacy mask bit still selects its original action ID");
        CHECK(captured[0].type == original[index].type,
              "legacy mask bit still selects its original item type");
    }
    CaptureMenu(0, LP_STEALTH);
    CHECK(captured_count == 0, "zero mask also hides enabled Stealth");

    if (GetFullPathName(TEXT("Lang\\zh_CN.ini"), ARR_SZ(path), path, NULL)) {
        LoadTranslation(path);
        CaptureMenu(stealth_bit, LP_STEALTH);
        CHECK(!lstrcmp(captured[0].label, TEXT("\u9690\u8eab\u6a21\u5f0f(&S)")),
              "Simplified Chinese translation is loaded into the menu");
        CHECK(captured[0].state & MFS_CHECKED, "translated Stealth retains its check mark");
    } else {
        CHECK(FALSE, "resolve Simplified Chinese translation path");
    }
    LoadTranslation(NULL);

    printf("Action-menu tests: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
