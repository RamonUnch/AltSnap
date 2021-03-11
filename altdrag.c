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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// App
#define APP_NAME       L"AltDrag"
#define APP_NAMEA      "AltDrag"
#define APP_VERSION    "1.39"

// Messages
#define WM_TRAY        (WM_USER+1)
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
HWND g_mchwnd = NULL;
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
#include "unfuck.h"
#include "languages.c"
#include "tray.c"
#include "config.c"
#include "rpc.h"

/////////////////////////////////////////////////////////////////////////////
int HookSystem()
{
    if (keyhook) return 1; // System already hooked

    // Load library
    if (!hinstDLL) {
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, ARR_SZ(path));
        PathRemoveFileSpecL(path);
        wcscat(path, L"\\hooks.dll");
        hinstDLL = LoadLibraryA("HOOKS.DLL");
        if (!hinstDLL) {
            return 1;
        } else {
            HWND (*Load) (HWND) = (void *)GetProcAddress(hinstDLL, "Load");
            if(Load) {
                g_mchwnd = Load(g_hwnd);
            }
        }
    }
    // Load keyboard hook
    HOOKPROC procaddr;
    if (!keyhook) {
        // Get address to keyboard hook (beware name mangling)
        procaddr = (HOOKPROC) GetProcAddress(hinstDLL, "LowLevelKeyboardProc@12");
        if (procaddr == NULL) {
            return 1;
        }
        // Set up the keyboard hook
        keyhook = SetWindowsHookEx(WH_KEYBOARD_LL, procaddr, hinstDLL, 0);
        if (keyhook == NULL) {
            return 1;
        }
    }

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
void ShowSClickMenu() {
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_ALWAYSONTOP, l10n->input_actions_alwaysontop);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_BORDERLESS, l10n->input_actions_borderless);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_CENTER, l10n->input_actions_center);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_ROLL, l10n->input_actions_roll);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_LOWER, l10n->input_actions_lower);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_MAXIMIZE, l10n->input_actions_maximize);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_MINIMIZE, l10n->input_actions_minimize);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_CLOSE, l10n->input_actions_close);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    InsertMenu(menu, -1, MF_BYPOSITION|MF_STRING, SC_ACTION+AC_NONE, L"Cancel");
    SetForegroundWindow(g_mchwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, g_mchwnd, NULL);
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
    } else if (msg == WM_SCLICK && g_mchwnd != NULL) {
        ShowSClickMenu();
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
    } else if (msg == WM_PAINT && wParam) {
        return 0;
    } else if (msg == WM_ERASEBKGND && wParam) {
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, char *szCmdLine, int iCmdShow)
{
    g_hinst = hInst;

    // Get ini path
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    wcscpy(&inipath[wcslen(inipath)-3], L"ini");

    // Convert szCmdLine to argv and argc (max 10 arguments)
    char *argv[10];
    int argc = 1;
    argv[0] = szCmdLine;
    while (argc<=10 && (argv[argc] = strchr(argv[argc - 1], ' ')) ) {
        *argv[argc] = '\0';
        argv[argc++]++;
    }

    // Check arguments
    int i;
    int elevate = 0, quiet = 0, config = -1, multi = 0;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-hide") || !strcmp(argv[i], "-h")) {
            // -hide = do not add tray icon, hide it if already running
            hide = 1;
        } else if (!strcmp(argv[i], "-quiet") || !strcmp(argv[i], "-q")) {
            // -quiet = do nothing if already running
            quiet = 1;
        } else if (!strcmp(argv[i], "-elevate") || !strcmp(argv[i], "-e")) {
            // -elevate = create a new instance with administrator privileges
            elevate = 1;
        } else if (!strcmp(argv[i], "-config") || !strcmp(argv[i], "-c")) {
            // -config = open config (with requested page)
            config = (i + 1 < argc) ? atoi(argv[i + 1]) : 0;
        } else if (!strcmp(argv[i], "-multi")) {
            // -multi = allow multiple instances,
            // used internally when elevating via config window
            multi = 1;
        }
    }

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
    // Register some messages
    WM_UPDATESETTINGS = RegisterWindowMessage(L"UpdateSettings");
    WM_OPENCONFIG     = RegisterWindowMessage(L"OpenConfig");
    WM_CLOSECONFIG    = RegisterWindowMessage(L"CloseConfig");
    WM_ADDTRAY        = RegisterWindowMessage(L"AddTray");
    WM_HIDETRAY       = RegisterWindowMessage(L"HideTray");

    // Look for previous instance
    if (!multi && !GetPrivateProfileInt(L"Advanced", L"MultipleInstances", 0, inipath)){
        if (quiet) return 0;
        HWND previnst = FindWindow(APP_NAME, NULL);
        if (previnst) {
            PostMessage(previnst, WM_UPDATESETTINGS, 0, 0);
            PostMessage(previnst, hide && config? WM_CLOSECONFIG: WM_OPENCONFIG, config, 0);
            PostMessage(previnst, hide? WM_HIDETRAY : WM_ADDTRAY, 0, 0);
            return 0;
        }
    }
    // Check AlwaysElevate
    if (!elevated) {
        elevate = GetPrivateProfileInt(L"Advanced", L"AlwaysElevate", 0, inipath);

        // Handle request to elevate to administrator privileges
        if (elevate) {
            wchar_t path[MAX_PATH];
            GetModuleFileName(NULL, path, ARR_SZ(path));
            HINSTANCE ret = ShellExecute(NULL, L"runas", path, (hide? L"-hide": NULL), NULL, SW_SHOWNORMAL);
            if ((int)ret > 32) return 0;
        }
    }
    // Language
    memset(&l10n_ini, 0, sizeof(l10n_ini));
    ListAllTranslations();
    UpdateLanguage();

    // Create window
    WNDCLASSEX wnd =
        { sizeof(WNDCLASSEX), 0, WindowProc, 0, 0, hInst, NULL, NULL
        , (HBRUSH) (COLOR_WINDOW + 1), NULL, APP_NAME, NULL };
    RegisterClassEx(&wnd);
    g_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST| WS_EX_TRANSPARENT, wnd.lpszClassName,
                            NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
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
    if (config != -1) {
        PostMessage(g_hwnd, WM_OPENCONFIG, config, 0);
    }
    // Message loop
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
