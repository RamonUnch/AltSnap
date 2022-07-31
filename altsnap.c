/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2021                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"

// Messages
#define SWM_TOGGLE     (WM_APP+1)
#define SWM_HIDE       (WM_APP+2)
#define SWM_ELEVATE    (WM_APP+3)
#define SWM_CONFIG     (WM_APP+4)
#define SWM_ABOUT      (WM_APP+5)
#define SWM_EXIT       (WM_APP+6)
#define SWM_FIND       (WM_APP+7)
#define SWM_HELP       (WM_APP+8)
#define SWM_SAVEZONES  (WM_APP+9)
#define SWM_TESTWIN    (WM_APP+10)

// Boring stuff
static HINSTANCE g_hinst = NULL;
static HWND g_hwnd = NULL;
static UINT WM_TASKBARCREATED = 0;
static wchar_t inipath[MAX_PATH];

// Cool stuff
HINSTANCE hinstDLL = NULL;
HHOOK keyhook = NULL;
static char elevated = 0;
static char ScrollLockState = 0;
static char SnapGap = 0;
static BYTE WinVer = 0;

#define WIN2K (WinVer >= 5)
#define VISTA (WinVer >= 6)
#define WIN10 (WinVer >= 10)

#define HOOK_TORESUME  ((HHOOK)1)
static int ENABLED() { return keyhook && keyhook != HOOK_TORESUME; }
#define GetWindowRectL(hwnd, rect) GetWindowRectLL(hwnd, rect, SnapGap)

// Include stuff
#include "languages.c"
#include "tray.c"
#include "config.c"

/////////////////////////////////////////////////////////////////////////////
int HookSystem()
{
    if (keyhook) return 1; // System already hooked
    LOG("Going to Hook the system...");

    // Load library
    if (!hinstDLL) {
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, ARR_SZ(path));
        PathRemoveFileSpecL(path);
        wcscat(path, L"\\hooks.dll");
        hinstDLL = LoadLibraryA("HOOKS.DLL");
        if (!hinstDLL) {
            LOG("Could not load HOOKS.DLL!!!");
            return 1;
        } else {
            void (*Load) (HWND) = (void *)GetProcAddress(hinstDLL, "Load");
            if(Load) {
                Load(g_hwnd);
            }
        }
    }
    LOG("HOOKS.DLL Loaded");

    // Load keyboard hook
    HOOKPROC procaddr;
    if (!keyhook) {
        // Get address to keyboard hook (beware name mangling)
        procaddr = (HOOKPROC) GetProcAddress(hinstDLL, LOW_LEVELK_BPROC);
        if (procaddr == NULL) {
            LOG("Could not find "LOW_LEVELK_BPROC" entry point in HOOKS.DLL");
            return 1;
        }
        // Set up the keyboard hook
        keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, procaddr, hinstDLL, 0);
        if (keyhook == NULL) {
            LOG("Keyboard HOOK could not be set");
            return 1;
        }
    }
    LOG("Keyboard HOOK set");

    // Reading some config options...
    UseZones = GetPrivateProfileInt(L"Zones", L"UseZones", 0, inipath);
    SnapGap = CLAMP(-128, GetPrivateProfileInt(L"Advanced", L"SnapGap", 0, inipath), 127);
    UpdateTray();
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
int showerror = 1;
int UnhookSystem()
{
    LOG("Going to UnHook the system...");
    if (!keyhook) { // System not hooked
        return 1;
    } else if (!UnhookWindowsHookEx(keyhook) && showerror) {
        MessageBox(NULL, l10n->unhook_error, APP_NAME,
                   MB_ICONINFORMATION|MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
    }
    keyhook = NULL;

    // Tell dll file that we are unloading
    void (*Unload) () = (void *) GetProcAddress(hinstDLL, "Unload");
    if (Unload) Unload();

    // Free library
    if (hinstDLL) FreeLibrary(hinstDLL);

    hinstDLL = NULL;

    // Success
    UpdateTray();
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
void ToggleState()
{
    if (ENABLED()) {
        UnhookSystem();
    } else {
        SendMessage(g_hwnd, WM_UPDATESETTINGS, 0, 0);
        HookSystem();
    }
}
/////////////////////////////////////////////////////////////////////////////
// if param&1 -> Add Kill action
// if param&2 -> Add Size Option
void ShowSClickMenu(HWND hwnd, LPARAM param)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, AC_ALWAYSONTOP,l10n->input_actions_alwaysontop);
    CheckMenuItem(menu, AC_ALWAYSONTOP, param&LP_TOPMOST?MF_CHECKED:MF_UNCHECKED);

    AppendMenu(menu, MF_STRING, AC_BORDERLESS, l10n->input_actions_borderless);
    CheckMenuItem(menu, AC_BORDERLESS, param&LP_BORDERLESS?MF_CHECKED:MF_UNCHECKED);

    AppendMenu(menu, MF_STRING, AC_CENTER,     l10n->input_actions_center);
    AppendMenu(menu, MF_STRING, AC_ROLL,       l10n->input_actions_roll);
    AppendMenu(menu, MF_STRING, AC_LOWER,      l10n->input_actions_lower);
    AppendMenu(menu, MF_STRING, AC_MAXHV,      l10n->input_actions_maximizehv);
    AppendMenu(menu, MF_STRING, AC_MINALL,     l10n->input_actions_minallother);
    AppendMenu(menu, MF_STRING, AC_SIDESNAP,   l10n->input_actions_sidesnap);
    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, AC_MAXIMIZE,   l10n->input_actions_maximize);
    CheckMenuItem(menu, AC_MAXIMIZE, param&LP_MAXIMIZED?MF_CHECKED:MF_UNCHECKED);
    AppendMenu(menu, MF_STRING, AC_MINIMIZE,   l10n->input_actions_minimize);
    AppendMenu(menu, MF_STRING, AC_CLOSE,      l10n->input_actions_close);
    if (param&LP_AGGRKILL) {
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, AC_KILL, l10n->input_actions_kill);
    }
//    InsertMenu(menu, MF_SEPARATOR, 0, NULL);
//    InsertMenu(menu, MF_STRING, AC_MUTE, l10n->input_actions_mute);

    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, AC_NONE, l10n->input_actions_nothing);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, GetSystemMetrics(SM_MENUDROPALIGNMENT), pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}
// To get the caret position in screen coordinate.
// We first try to get the carret rect
static void GetKaretPos(POINT *pt)
{
    GUITHREADINFO gui;
    gui.cbSize = sizeof(GUITHREADINFO);
    gui.hwndCaret = NULL;
    if (GetGUIThreadInfo(0, &gui)) {
        pt->x = (gui.rcCaret.right + gui.rcCaret.left)>>1;
        pt->y = (gui.rcCaret.top + gui.rcCaret.bottom)>>1;
        if (gui.hwndCaret) {
            ClientToScreen(gui.hwndCaret, pt);
            return;
        }
    }
    GetCursorPos(pt);
}
static void ShowUnikeyMenu(HWND hwnd, LPARAM param)
{
    UCHAR vkey = LOBYTE(LOWORD(param));
    UCHAR capital = HIBYTE(LOWORD(param));
    wchar_t **ukmap = &l10n->a; //EXTRAKEYS_MAP;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    const wchar_t *kl, *keylist = ukmap[vkey - 0x41];
    UCHAR i;
    for (kl = keylist, i='A'; *kl; kl++) {
        if(*kl==L'%') {
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            continue;
        }
        wchar_t unichar = *kl;
        if (kl[1] == L'|') {
            kl+=2;
            if (capital) unichar = *kl;
        } else if (capital) {
            unichar = (wchar_t)(LONG_PTR)CharUpperW((wchar_t *)(LONG_PTR)*kl);
        }
        if (i > 'Z') i = '1';
        wchar_t mwstr[6];
        mwstr[0] = L'&';
        mwstr[1] = i++;
        mwstr[2] = L'\t';
        DWORD utf16c;
        if (IS_SURROGATE_PAIR(unichar, kl[1])) {
            utf16c =*(DWORD*)kl;
            mwstr[3] = LOWORD(utf16c);
            mwstr[4] = HIWORD(utf16c);
            mwstr[5] = L'\0';
            kl++; // skip high surrogate
        } else {
            mwstr[3] = utf16c = unichar;
            mwstr[4] = L'\0';
        }
        if (!AppendMenu(menu, MF_STRING, utf16c, mwstr))
            break;
    }
    if (kl > keylist) {
        POINT pt;
        GetKaretPos(&pt);
        PostMessage(hwnd, WM_MENUCREATED, (WPARAM)menu, 0);
        TrackPopupMenu(menu, GetSystemMetrics(SM_MENUDROPALIGNMENT), pt.x, pt.y, 0, hwnd, NULL);
    }
    PostMessage(hwnd, WM_MENUCREATED, 0, 0);
    DestroyMenu(menu);
}
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!msg) {
        // In case some messages are not registered.
    } else if(wParam && msg == WM_ERASEBKGND) {
        return 1;
    } else if (wParam && (msg == WM_PAINT || msg == WM_NCPAINT)) {
        return 0;
    } else if (msg == WM_TRAY) {
        if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK) {
            ToggleState();
            if (lParam == WM_LBUTTONDBLCLK) {
                SendMessage(hwnd, WM_OPENCONFIG, 0, 0);
            }
        } else if (lParam == WM_MBUTTONDOWN) {
            ShellExecute(NULL, L"open", inipath, NULL, NULL, SW_SHOWNORMAL);
        } else if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        }
    } else if (msg == WM_SCLICK && wParam) {
        ShowSClickMenu((HWND)wParam, lParam);
    } else if (msg == WM_UNIKEYMENU) {
        ShowUnikeyMenu((HWND)wParam, lParam);
    } else if (msg == WM_UPDATESETTINGS) {
        // Reload hooks
        if (ENABLED()) {
            UnhookSystem();
            HookSystem();
        }
    } else if (msg == WM_ADDTRAY) {
        hide = 0;
        UpdateTray();
    } else if (msg == WM_UPDATETRAY) {
        UpdateTray();
    } else if (msg == WM_HIDETRAY) {
        hide = 1;
        RemoveTray();
    } else if (msg == WM_OPENCONFIG && (lParam || !hide)) {
        OpenConfig(wParam);
    } else if (msg == WM_CLOSECONFIG) {
        CloseConfig();
    } else if (msg == WM_TASKBARCREATED) {
        // Try to add the tray icon because explorer started.
        tray_added = 0;
        UpdateTray();
    } else if (msg == WM_COMMAND) {
        int wmId = LOWORD(wParam); // int wmEvent = HIWORD(wParam);
        if (wmId == SWM_TOGGLE) {
            ToggleState();
        } else if (wmId == SWM_HIDE) {
            hide = 1;
            RemoveTray();
        } else if (wmId == SWM_ELEVATE) {
           ElevateNow(0);
        } else if (wmId == SWM_CONFIG) {
            SendMessage(hwnd, WM_OPENCONFIG, 0, 0);
        } else if (wmId == SWM_ABOUT) {
            SendMessage(hwnd, WM_OPENCONFIG, 5, 0);
        } else if (wmId == SWM_EXIT) {
            DestroyWindow(hwnd);
        } else if (wmId == SWM_SAVEZONES) {
            int ret = MessageBox(NULL, l10n->zone_confirmation, APP_NAME, MB_OKCANCEL);
            if (ret == IDOK) {
                UnhookSystem();
                SaveCurrentLayout();
                HookSystem();
            }
        } else if (wmId == SWM_TESTWIN) {
            NewTestWindow();
        }
    } else if (msg == WM_QUERYENDSESSION) {
        showerror = 0;
        UnhookSystem();
    } else if (msg == WM_DESTROY) {
        UnhookSystem();
        RemoveTray();
        PostQuitMessage(0);
    } else if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN) {
        // Hide cursorwnd if clicked on, this might happen if
        // it wasn't hidden by hooks.c for some reason
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, char *szCmdLine, int iCmdShow)
{
    g_hinst = hInst;

    // Get ini path
    LOG("\n\nALTSNAP STARTED");
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    wcscpy(&inipath[wcslen(inipath)-3], L"ini");
    LOG("ini file: %S", inipath);

    // Read parameters on command line
    int elevate = 0, quiet = 0, config =0, multi = 0;
    char *params = szCmdLine;
    while (*params++);
    while (szCmdLine < params && *params-- != '\\');
    while (*params && *params != ' ') params++;

    hide    = !!strstr(params, "-h");
    quiet   = !!strstr(params, "-q");
    elevate = !!strstr(params, "-e");
    multi   = !!strstr(params, "-m");
    config  = !!strstr(params, "-c");

    // Check if elevated if in >= WinVer
    WinVer = LOBYTE(LOWORD(GetVersion()));
    LOG("Running with Windows version %d", WinVer);
    if (WinVer >= 6) { // Vista +
        HANDLE token;
        TOKEN_ELEVATION elevation;
        DWORD len;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)
        && GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &len)) {
            elevated = elevation.TokenIsElevated;
            CloseHandle(token);
        }
        LOG("Process started %s elevated", elevated? "already": "non");
    }
    LOG("Command line parameters read, hide=%d, quiet=%d, elevate=%d, multi=%d, config=%d"
                                     , hide, quiet, elevate, multi, config);

    // Look for previous instance
    if (!multi && !GetPrivateProfileInt(L"Advanced", L"MultipleInstances", 0, inipath)){
        if (quiet) return 0;
        HWND previnst = FindWindow(APP_NAME, NULL);
        if (previnst) {
            LOG("Previous instance found and no -multi mode")
            if(hide)   PostMessage(previnst, WM_CLOSECONFIG, 0, 0);
            if(config) PostMessage(previnst, WM_OPENCONFIG, 0, 0);
            PostMessage(previnst, hide? WM_HIDETRAY : WM_ADDTRAY, 0, 0);
            LOG("Updated old instance and NORMAL EXIT");
            return 0;
        }
        LOG("No previous instance found");
    }

    // Check AlwaysElevate
    if (!elevated) {
        if(!elevate) elevate = GetPrivateProfileInt(L"Advanced", L"AlwaysElevate", 0, inipath);

        // Handle request to elevate to administrator privileges
        if (elevate) {
            LOG("Elevation requested");
            wchar_t path[MAX_PATH];
            GetModuleFileName(NULL, path, ARR_SZ(path));
            HINSTANCE ret = ShellExecute(NULL, L"runas", path, (hide? L"-h": NULL), NULL, SW_SHOWNORMAL);
            if ((DorQWORD)ret > 32){
                LOG("Elevation Faild => Not cool NORMAL EXIT");
                return 0;
            }
            LOG("Elevation sucess");
        } else LOG("No Elevation requested");

    }
    // Language
    ListAllTranslations(); LOG("All translations listed");
    UpdateLanguage(); LOG("Language updated");

    // Create window
    WNDCLASSEX wnd =
        { sizeof(WNDCLASSEX), 0
        , WindowProc, 0, 0, hInst, NULL, NULL
        , NULL, NULL, APP_NAME, NULL };
    BOOL regg = RegisterClassEx(&wnd);
    LOG("Register main APP Window: %s", regg? "Sucess": "Failed");
    g_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST| WS_EX_TRANSPARENT
                            , wnd.lpszClassName , NULL , WS_POPUP
                            , 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    LOG("Create main APP Window: %s", g_hwnd? "Sucess": "Failed");
    // Tray icon

    InitTray();
    UpdateTray();

    // Hook system
    HookSystem();

    // Add tray if hook failed, even though -hide was supplied
    if (hide && !keyhook) {
        hide = 0;
        UpdateTray();
    }
    // Open config if -config was supplied
    if (config) {
        PostMessage(g_hwnd, WM_OPENCONFIG, 0, 0);
    }
    // Message loop
    LOG("Starting "APP_NAMEA" message loop...");
    BOOL ret;
    MSG msg;
    while ((ret = GetMessage( &msg, NULL, 0, 0 )) != 0) {
        if (ret == -1) {
            break;
        } else {
         // TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(g_hwnd);
    LOG("GOOD NORMAL EXIT");
    return msg.wParam;
}
/////////////////////////////////////////////////////////////////////////////
// Use -nostdlib and -e_unfuckMain@0 to use this main, -eunfuckMain for x64.
void WINAPI unfuckWinMain(void)
{
    HINSTANCE hInst;
    HINSTANCE hPrevInstance = NULL;
    char *szCmdLine;
    int iCmdShow = 0;

    hInst = GetModuleHandle(NULL);
    szCmdLine = (char *) GetCommandLineA();

    ExitProcess(WinMain(hInst, hPrevInstance, szCmdLine, iCmdShow));
}
