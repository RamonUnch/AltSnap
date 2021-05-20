/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0400
#include <windows.h>

#define LOG_STUFF 0

// App
#define APP_NAME       L"AltDrag"
#define APP_NAMEA      "AltDrag"
#define APP_VERSION    "1.44"

// Messages
#define SWM_TOGGLE     (WM_APP+1)
#define SWM_HIDE       (WM_APP+2)
#define SWM_ELEVATE    (WM_APP+3)
#define SWM_CONFIG     (WM_APP+4)
#define SWM_ABOUT      (WM_APP+5)
#define SWM_EXIT       (WM_APP+6)
#define SWM_FIND       (WM_APP+7)
#define SWM_HELP       (WM_APP+8)

// Boring stuff
#define ENABLED() (keyhook)
HINSTANCE g_hinst = NULL;
HWND g_hwnd = NULL;
UINT WM_TASKBARCREATED = 0;
UINT WM_UPDATESETTINGS = 0;
UINT WM_ADDTRAY = 0;
UINT WM_HIDETRAY = 0;
UINT WM_OPENCONFIG = 0;
UINT WM_CLOSECONFIG = 0;
wchar_t inipath[MAX_PATH];

// Cool stuff
HINSTANCE hinstDLL = NULL;
HHOOK keyhook = NULL;
char elevated = 0;
char WinVer = 0;

// Include stuff
#include "hooks.h"
#include "unfuck.h"
#include "languages.c"
#include "tray.c"
#include "config.c"

#ifdef WIN64
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc"
#else
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc@12"
#endif

/////////////////////////////////////////////////////////////////////////////
int HookSystem()
{
    if (keyhook) return 1; // System already hooked
    LOG("Going to Hook the system...\n"); 

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
    LOG("HOOKS.DLL Loaded\n");

    // Load keyboard hook
    HOOKPROC procaddr;
    if (!keyhook) {
        // Get address to keyboard hook (beware name mangling)
        procaddr = (HOOKPROC) GetProcAddress(hinstDLL, LOW_LEVELK_BPROC);
        if (procaddr == NULL) {
            LOG("Could not find LowLevelKeyboardProc@12 entry point in HOOKS.DLL\n");
            return 1;
        }
        // Set up the keyboard hook
        keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, procaddr, hinstDLL, 0);
        if (keyhook == NULL) {
            LOG("Keyboard HOOK could not be set\n");
            return 1;
        }
    }
    LOG("Keyboard HOOK set\n");

    UpdateTray();
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
int showerror = 1;
int UnhookSystem()
{
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
void ShowSClickMenu(HWND hwnd, LPARAM param)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_ALWAYSONTOP, l10n->input_actions_alwaysontop);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_BORDERLESS, l10n->input_actions_borderless);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_CENTER, l10n->input_actions_center);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_ROLL, l10n->input_actions_roll);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_LOWER, l10n->input_actions_lower);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_MAXIMIZE, l10n->input_actions_maximize);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_MINIMIZE, l10n->input_actions_minimize);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_CLOSE, l10n->input_actions_close);
    if (param) {
        InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
        InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_KILL, l10n->input_actions_kill);
    }
    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, AC_NONE, l10n->input_actions_nothing);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TRAY) {
        if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONDBLCLK) {
            ToggleState();
            if (lParam == WM_LBUTTONDBLCLK
                && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                SendMessage(hwnd, WM_OPENCONFIG, 0, 0);
            }
        } else if (lParam == WM_MBUTTONDOWN) {
            ShellExecute(NULL, L"open", inipath, NULL, NULL, SW_SHOWNORMAL);
        } else if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        } else if (lParam == NIN_BALLOONTIMEOUT && hide) {
            RemoveTray();
        }
    } else if (msg == WM_SCLICK && wParam) {
        ShowSClickMenu((HWND)wParam, lParam);
    } else if (msg == WM_UPDATESETTINGS) {
        // Reload hooks
        if (ENABLED()) {
            UnhookSystem();
            HookSystem();
        }
        // Reload config language
        if (!wParam && IsWindow(g_cfgwnd)) {
            SendMessage(g_cfgwnd, WM_UPDATESETTINGS, 0, 0);
        }
    } else if (msg == WM_ADDTRAY) {
        hide = 0;
        UpdateTray();
    } else if (msg == WM_HIDETRAY) {
        hide = 1;
        RemoveTray();
    } else if (msg == WM_OPENCONFIG && (lParam || !hide)) {
        OpenConfig(wParam);
    } else if (msg == WM_CLOSECONFIG) {
        CloseConfig();
    } else if (msg == WM_TASKBARCREATED) {
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
    } else if (wParam && (msg == WM_PAINT || msg == WM_ERASEBKGND || msg == WM_NCPAINT)) {
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, char *szCmdLine, int iCmdShow)
{
    g_hinst = hInst;

    // Get ini path
    LOG("\nALTDRAG STARTED\n");
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    wcscpy(&inipath[wcslen(inipath)-3], L"ini");

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
    OSVERSIONINFO vi = { sizeof(OSVERSIONINFO) };
    GetVersionEx(&vi);
    WinVer = vi.dwMajorVersion;
    if (VISTA) { // Vista +
        HANDLE token;
        TOKEN_ELEVATION elevation;
        DWORD len;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)
            && GetTokenInformation(token, TokenElevation, &elevation,
                                   sizeof(elevation), &len)) {
            elevated = elevation.TokenIsElevated;
            CloseHandle(token);
        }
    }
    LOG("Command line parameters read\n");

    // Register some messages
    WM_UPDATESETTINGS = RegisterWindowMessage(L"UpdateSettings");
    WM_OPENCONFIG     = RegisterWindowMessage(L"OpenConfig");
    WM_CLOSECONFIG    = RegisterWindowMessage(L"CloseConfig");
    WM_ADDTRAY        = RegisterWindowMessage(L"AddTray");
    WM_HIDETRAY       = RegisterWindowMessage(L"HideTray");
    LOG("Messages Registered: %s\n"
      , WM_UPDATESETTINGS && WM_OPENCONFIG && WM_CLOSECONFIG && WM_ADDTRAY && WM_HIDETRAY
        ? "OK": "Some messages Failed to register");

    // Look for previous instance
    if (!multi && !GetPrivateProfileInt(L"Advanced", L"MultipleInstances", 0, inipath)){
        if (quiet) return 0;
        HWND previnst = FindWindow(APP_NAME, NULL);
        if (previnst) {
            LOG("Previous instance found and no -multi mode\n")
            PostMessage(previnst, WM_UPDATESETTINGS, 0, 0);
            if(hide)   PostMessage(previnst, WM_CLOSECONFIG, 0, 0);
            if(config) PostMessage(previnst, WM_OPENCONFIG, 0, 0);
            PostMessage(previnst, hide? WM_HIDETRAY : WM_ADDTRAY, 0, 0);
            LOG("Updated old instance and NORMAL EXIT\n");
            return 0;
        }
        LOG("No previous instance found\n");
    }

    // Check AlwaysElevate
    if (!elevated) {
        elevate = GetPrivateProfileInt(L"Advanced", L"AlwaysElevate", 0, inipath);

        // Handle request to elevate to administrator privileges
        if (elevate) {
            LOG("Elevation requested\n");
            wchar_t path[MAX_PATH];
            GetModuleFileName(NULL, path, ARR_SZ(path));
            HINSTANCE ret = ShellExecute(NULL, L"runas", path, (hide? L"-h": NULL), NULL, SW_SHOWNORMAL);
            if ((DorQWORD)ret > 32){
                LOG("Elevation Faild => Not cool NORMAL EXIT\n");
                return 0;
            }
            LOG("Elevation sucess\n");
        } else LOG("No Elevation requested\n");
        
    }
    // Language
    memset(&l10n_ini, 0, sizeof(l10n_ini));
    ListAllTranslations(); LOG("All translations listed\n");
    UpdateLanguage(); LOG("Language updated\n");

    // Create window
    WNDCLASSEX wnd =
        { sizeof(WNDCLASSEX), 0, WindowProc, 0, 0, hInst, NULL, NULL
        , (HBRUSH) (COLOR_WINDOW + 1), NULL, APP_NAME, NULL };
    BOOL regg = RegisterClassEx(&wnd);
    LOG("Register main APP Window: %s\n", regg? "Sucess": "Failed");
    g_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST| WS_EX_TRANSPARENT, wnd.lpszClassName,
                            NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    LOG("Create main APP Window: %s\n", g_hwnd? "Sucess": "Failed");
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
    LOG("Starting AltDrag message loop...\n");
    BOOL ret;
    MSG msg;
    while ((ret = GetMessage( &msg, NULL, 0, 0 )) != 0) {
        if (ret == -1) {
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    // EXIT
    UnregisterClass(APP_NAME, hInst);
    DestroyWindow(g_hwnd);
    LOG("GOOD NORMAL EXIT\n");
    return msg.wParam;
}
/////////////////////////////////////////////////////////////////////////////
// use -nostdlib and -e_unfuckMain@0 to use this main
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
