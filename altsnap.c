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
#define SWM_OPENINIFILE (WM_APP+11)
#define SWM_SNAPLAYOUT    (WM_APP+12)
#define SWM_SNAPLAYOUTEND (WM_APP+22)

// Boring stuff
static HINSTANCE g_hinst = NULL;
static HWND g_hwnd = NULL;
static UINT WM_TASKBARCREATED = 0;
static TCHAR inipath[MAX_PATH];

static HWND g_dllmsgHKhwnd = NULL;

// Cool stuff
HINSTANCE hinstDLL = NULL;
HHOOK keyhook = NULL;
static DWORD ACMenuItems=-1;
static char elevated = 0;
static char ScrollLockState = 0;
static char SnapGap = 0;
static BYTE WinVer = 0;

#define WIN2K (WinVer >= 5)
#define VISTA (WinVer >= 6)
#define WIN10 (WinVer >= 10)

#define ENABLED() (!!keyhook)
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
        TCHAR path[MAX_PATH];
        DWORD ret = GetModuleFileName(NULL, path, ARR_SZ(path));
        if(!ret || ret == ARR_SZ(path)) return 1;
        PathRemoveFileSpecL(path);
        lstrcat_s(path, ARR_SZ(path), TEXT("\\hooks.dll"));
        hinstDLL = LoadLibrary(path);
        if (!hinstDLL) {
            LOG("Could not load HOOKS.DLL!!!");
            return 1;
        } else {
            HWND (WINAPI *Load)(HWND) = (HWND (WINAPI *)(HWND))GetProcAddress(hinstDLL, "Load");
            if(Load) {
                g_dllmsgHKhwnd = Load(g_hwnd);
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
    UseZones = GetPrivateProfileInt(TEXT("Zones"), TEXT("UseZones"), 0, inipath);
    SnapGap = CLAMP(-128, GetPrivateProfileInt(TEXT("Advanced"), TEXT("SnapGap"), 0, inipath), 127);
    ACMenuItems = GetPrivateProfileInt(TEXT("Advanced"), TEXT("ACMenuItems"), -1, inipath);
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
        MessageBox(NULL, l10n->unhook_error, TEXT(APP_NAMEA),
                   MB_ICONINFORMATION|MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
    }
    keyhook = NULL;

    // Tell dll file that we are unloading
    void (WINAPI *Unload)() = (void (WINAPI *)()) GetProcAddress(hinstDLL, "Unload");
    if (Unload) Unload();

    // Zero out the message hwnd from DLL.
    g_dllmsgHKhwnd = NULL;

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
//
void ShowSClickMenu(HWND hwnd, LPARAM param)
{
    POINT pt;
    if (param&LP_CURSORPOS) {
        // Use cursor position to place menu.
        GetCursorPos(&pt);
    } else {
        // Use the pointed window
        RECT rc;
        HWND clickhwnd = (HWND)SendMessage(hwnd, WM_GETCLICKHWND, 0, 0);
        GetWindowRect(clickhwnd, &rc);
        pt.x = rc.left + GetSystemMetricsForWin(SM_CXSIZEFRAME, clickhwnd);
        pt.y = rc.top + GetSystemMetricsForWin(SM_CYSIZEFRAME, clickhwnd)
                      + GetSystemMetricsForWin(SM_CYCAPTION, clickhwnd);
    }
    HMENU menu = CreatePopupMenu();
    UCHAR show_oriclick = (param&LP_NOALTACTION)? AC_ORICLICK: 0xFF;

    #define CHK(LP_FLAG) MF_STRING|(param&LP_FLAG?MF_CHECKED:MF_UNCHECKED)

    const struct {
        UCHAR action; const WORD mf; const TCHAR * const str;
    } mnlst[] = {
       /* hide, action,      MF_FLAG/CHECKED,    menu string */
        { AC_ALWAYSONTOP, CHK(LP_TOPMOST),    l10n->input_actions_alwaysontop },
        { AC_BORDERLESS,  CHK(LP_BORDERLESS), l10n->input_actions_borderless },
        { AC_CENTER,      MF_STRING,          l10n->input_actions_center},
        { AC_ROLL,        CHK(LP_ROLLED),     l10n->input_actions_roll},
        { AC_LOWER,       MF_STRING,          l10n->input_actions_lower},
        { AC_MAXHV,       MF_STRING,          l10n->input_actions_maximizehv},
        { AC_MINALL,      MF_STRING,          l10n->input_actions_minallother},
        { AC_SIDESNAP,    MF_STRING,          l10n->input_actions_sidesnap},
        { 0,              MF_SEPARATOR, NULL }, /* ------------------------ */
        { AC_MAXIMIZE,    CHK(LP_MAXIMIZED),  l10n->input_actions_maximize},
        { AC_MINIMIZE,    MF_STRING,          l10n->input_actions_minimize},
        { AC_CLOSE,       MF_STRING,          l10n->input_actions_close},
        { 0,              MF_SEPARATOR, NULL }, /* ------------------------ */
        { AC_KILL,        MF_STRING,          l10n->input_actions_kill},
        { 0,              MF_SEPARATOR, NULL }, /* ------------------------ */
        { AC_MOVEONOFF,   CHK(LP_MOVEONOFF),  l10n->input_actions_moveonoff},
        { 0,              MF_SEPARATOR, NULL }, /* ------------------------ */
        { show_oriclick,  MF_STRING,          l10n->input_actions_oriclick},
        { AC_NONE,        MF_STRING,          l10n->input_actions_nothing},
    };
    #undef CHK
    #undef K
    unsigned i;
    for (i=0; i < ARR_SZ(mnlst); i++) {
        if ( (ACMenuItems>>i)&1 && mnlst[i].action != 0xFF) // Put the action in the HIWORD of wParam
            AppendMenu(menu, mnlst[i].mf, mnlst[i].action<<16, mnlst[i].str);
    }
    TrackPopupMenu(menu, GetSystemMetrics(SM_MENUDROPALIGNMENT), pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
    PostMessage(hwnd, WM_CLOSE, 0, 0);
}
// To get the caret position in screen coordinate.
// We first try to get the carret rect
#include <oleacc.h>
//static const GUID  my_IID_IAccessible = { 0x618736e0, 0x3c3d, 0x11cf, {0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };
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
//        } else if (gui.hwndFocus) {
//            IAccessible *pacc = NULL;
//            if ( S_OK==AccessibleObjectFromWindow(gui.hwndFocus, OBJID_CARET, &my_IID_IAccessible, (void**)&pacc) ) {
//        //        MessageBox(NULL, NULL, NULL, 0);
//                LONG x=0, y=0, w=0, h=0;
//                VARIANT varCaret;
//                varCaret.vt = VT_I4;
//                varCaret.lVal = CHILDID_SELF;
//                if (S_OK == pacc->lpVtbl->accLocation(pacc, &x, &y, &w, &h, varCaret)) {
//                    pt->x = x+w/2;
//                    pt->y = y+h/2;
//                    pacc->lpVtbl->Release(pacc);
//                    return;
//                }
//            }
        }
    }

    GetCursorPos(pt);
}
static void ShowUnikeyMenu(HWND hwnd, LPARAM param)
{
    UCHAR vkey = LOBYTE(LOWORD(param));
    UCHAR capital = HIBYTE(LOWORD(param));
    TCHAR *const* const ukmap = &l10n->a; //EXTRAKEYS_MAP;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    const TCHAR *kl, *keylist = ukmap[vkey - 0x41];
    UCHAR i;
    for (kl = keylist, i='A'; *kl; kl++) {
        if(*kl==L'%') {
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            continue;
        }
        TCHAR unichar = *kl;
        if (kl[1] == L'|') {
            kl+=2;
            if (capital) unichar = *kl;
        } else if (capital) {
            unichar = (TCHAR)(LONG_PTR)CharUpper((TCHAR *)(LONG_PTR)*kl);
        }
        if (i > 'Z') i = '1';
        TCHAR mwstr[6];
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
        TrackPopupMenu(menu, GetSystemMetrics(SM_MENUDROPALIGNMENT), pt.x, pt.y, 0, hwnd, NULL);
    }
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
            SendMessage(hwnd, WM_COMMAND, SWM_OPENINIFILE, 0);
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
            Sleep(16);
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
        } else if (wmId == SWM_OPENINIFILE) {
            ShellExecute(NULL, TEXT("open"), inipath, NULL, NULL, SW_SHOWNORMAL);
        } else if (wmId == SWM_EXIT) {
            DestroyWindow(hwnd);
        } else if (wmId == SWM_SAVEZONES) {
            TCHAR txt[256], numstr[16];
            lstrcpy_s(txt, ARR_SZ(txt), l10n->zone_confirmation);
            lstrcat_s(txt, ARR_SZ(txt), TEXT("\n"));
            lstrcat_s(txt, ARR_SZ(txt), TEXT("Snap Layout "));
            lstrcat_s(txt, ARR_SZ(txt), itostr(LayoutNumber+1, numstr, 10));
            int ret = MessageBox(NULL, txt, TEXT(APP_NAMEA), MB_OKCANCEL);
            if (ret == IDOK) {
                UnhookSystem();
                SaveCurrentLayout();
                WriteCurrentLayoutNumber();
                HookSystem();
            }
        } else if (wmId == SWM_TESTWIN) {
            NewTestWindow();
        } else if (SWM_SNAPLAYOUT <= wmId && wmId <= SWM_SNAPLAYOUTEND) {
            // Inform hooks.dll that the snap layout changed
            LayoutNumber = wmId-SWM_SNAPLAYOUT;
            if(g_dllmsgHKhwnd)
                PostMessage(g_dllmsgHKhwnd, WM_SETLAYOUTNUM, LayoutNumber, 0);
            // Save new value in the .ini file
            WriteCurrentLayoutNumber();
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
    } else if (msg == WM_DISPLAYCHANGE || (msg == WM_SETTINGCHANGE && wParam  == SPI_SETWORKAREA)) {
        LOG("WM_DISPLAYCHANGE %d:%d, %dbpp in WindowProc", LOWORD(lParam), HIWORD(lParam), wParam );
        if (g_dllmsgHKhwnd) {
            int bestlayout = SendMessage(g_dllmsgHKhwnd, WM_GETBESTLAYOUT, 0, 0);
            if( bestlayout != LayoutNumber
            &&  0 <= bestlayout && bestlayout < MaxLayouts ) {
                LayoutNumber = bestlayout;
                PostMessage(g_dllmsgHKhwnd, WM_SETLAYOUTNUM, LayoutNumber, 0);
            }
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
int WINAPI WinMainAW(HINSTANCE hInst, HINSTANCE hPrevInstance, const TCHAR *params, int iCmdShow)
{
    g_hinst = hInst;

    // Get ini path
    LOG("\n\nALTSNAP STARTED");
    GetModuleFileName(NULL, inipath, ARR_SZ(inipath));
    inipath[MAX_PATH-1] = '\0';
    lstrcpy_s(&inipath[lstrlen(inipath)-3], 4, TEXT("ini"));
    LOG("ini file: %S", inipath);

    // Read parameters on command line
    int help    = !!lstrstr(params, TEXT("-help"))
               || !!lstrstr(params, TEXT("/?"))
               || !!lstrstr(params, TEXT("-?"));
    if (help) {
        static const TCHAR *txthelp = TEXT(
            "AltSnap command line options:\n\n"
            "--help\tShow this help!\n"
            "-h\tHide the tray icon\n"
            "-q\tQuiet mode\n"
            "-m\tMultiple instances allowed\n"
            "-c\tOpen Config dialog\n"
            "-afX\tExecute action X for the foreground window\n"
            "-apX\tExecute action X for the pointed window\n"
        );
        MessageBox(NULL, txthelp, TEXT(APP_NAMEA" Usage"), MB_OK|MB_ICONINFORMATION);
        return 0;
    }

    hide        = !!lstrstr(params, TEXT("-h"));
    int quiet   = !!lstrstr(params, TEXT("-q"));
    int elevate = !!lstrstr(params, TEXT("-e"));
    int multi   = !!lstrstr(params, TEXT("-m"));
    int config  = !!lstrstr(params, TEXT("-c"));

    // Check if elevated if in >= WinVer
    WinVer = LOBYTE(LOWORD(GetVersion()));
    LOG("Running with Windows version %lX", GetVersion());
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
    if (!multi && !GetPrivateProfileInt(TEXT("Advanced"), TEXT("MultipleInstances"), 0, inipath)){
        if (quiet) return 0;

        HWND previnst = FindWindow(TEXT(APP_NAMEA), TEXT(""));
        if (previnst) {
            // Ask old HotKey window to perform an action.
            const TCHAR *actionstr = lstrstr(params, TEXT("-a"));
            if (actionstr && actionstr[2] && actionstr[3] && actionstr[4]) {
                enum action action = MapActionW(&actionstr[3]);
                HWND msghwnd;
                if ((msghwnd = FindWindow( TEXT(APP_NAMEA"-HotKeys"), TEXT("")))) {
                    PostMessage(msghwnd, WM_HOTKEY, (actionstr[2] == 'p')*0x1000+action, 0);
                    return 0;
                }
            }
            // Update old instance if no action to be made.
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
        if(!elevate) elevate = GetPrivateProfileInt(TEXT("Advanced"), TEXT("AlwaysElevate"), 0, inipath);

        // Handle request to elevate to administrator privileges
        if (elevate) {
            LOG("Elevation requested");
            TCHAR path[MAX_PATH];
            GetModuleFileName(NULL, path, ARR_SZ(path));
            HINSTANCE ret = ShellExecute(NULL, TEXT("runas"), path, (hide? TEXT("-h"): NULL), NULL, SW_SHOWNORMAL);
            if ((DorQWORD)ret > 32) {
                LOG("Elevation Faild => Not cool NORMAL EXIT");
                return 0;
            }
            LOG("Elevation sucess");
        } else {
            LOG("No Elevation requested");
        }
    }
    // Language
    UpdateLanguage(); LOG("Language updated");

    // Create window
    WNDCLASSEX wnd =
        { sizeof(WNDCLASSEX), 0
        , WindowProc, 0, 0, hInst, NULL, NULL
        , NULL, NULL, TEXT(APP_NAMEA), NULL };
    RegisterClassEx(&wnd);
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

    UnhookSystem();
    DestroyWindow(g_hwnd);
    LOG("GOOD NORMAL EXIT");
    return msg.wParam;
}
static pure const TCHAR *ParamsFromCmdline(const TCHAR *cmdl)
{
    /* in case it starts with " we need to go to the next " */
    if (cmdl[0] == TEXT('"')) {
        do {
            cmdl++;
        } while(*cmdl && *cmdl != TEXT('"'));
    } else {
        while(*cmdl && *cmdl != TEXT(' ') && *cmdl != TEXT('\t')) {
            cmdl++;
        }
    }
    cmdl++; // Skip the " or the ' '
    while(*cmdl == TEXT(' ') || *cmdl == TEXT('\t')) cmdl++;
    return cmdl;
}

/////////////////////////////////////////////////////////////////////////////
// Use -nostdlib and -e_unfuckMain@0 to use this main, -eunfuckMain for x64.
void noreturn WINAPI unfuckWinMain(void)
{
    HINSTANCE hInst;
    HINSTANCE hPrevInstance = NULL;
    const TCHAR *szCmdLine;
    int iCmdShow = 0;

    hInst = GetModuleHandle(NULL);
    szCmdLine = ParamsFromCmdline(GetCommandLine());

    ExitProcess(WinMainAW(hInst, hPrevInstance, szCmdLine, iCmdShow));
}
