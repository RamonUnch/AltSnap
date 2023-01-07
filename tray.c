/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NIF_INFO
#define NIF_INFO 0x00000010
#endif

static struct { // NOTIFYICONDATA for NT4
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    TCHAR szTip[64];
} tray;

static int tray_added = 0;
static int hide = 0;
static int UseZones = 0;

static const TCHAR *iconstr[] = {
    TEXT("TRAY_OFF"),
    TEXT("TRAY_ON"),
    TEXT("TRAY_SUS")
};
static const TCHAR *traystr[] = {
    TEXT(APP_NAMEA" (Off)"),
    TEXT(APP_NAMEA" (On)"),
    TEXT(APP_NAMEA"..."),
};
static HICON icons[3];

static void LoadAllIcons()
{
    TCHAR theme[MAX_PATH]; // Get theme name
    int ret = GetPrivateProfileString(TEXT("General"), TEXT("Theme"), TEXT(""), theme, ARR_SZ(theme), inipath);
    if (ret && theme[1]) {
        TCHAR path[MAX_PATH];
        DWORD mod = GetModuleFileName(NULL, path, ARR_SZ(path));
        if (mod) {
            PathRemoveFileSpecL(path);
            lstrcat_s(path, ARR_SZ(path), TEXT("\\Themes\\")); // Themes subfolder
            lstrcat_s(path, ARR_SZ(path), theme); // Theme name
            int len = lstrlen(path);
            TCHAR *p = path+len;
            *p++ = TEXT('\\');
            if (len < MAX_PATH-13) { // strlen("TRAY_OFF.ICO")==12
                UCHAR i;
                for(i=0; i<3; i++) {
                    lstrcpy(p, iconstr[i]);
                    lstrcat(p, TEXT(".ico"));
                    HICON tmp = LoadImage(g_hinst, path, IMAGE_ICON,0,0, LR_LOADFROMFILE|LR_DEFAULTSIZE|LR_LOADTRANSPARENT);
                    icons[i] = tmp? tmp: LoadIcon(g_hinst, iconstr[i]);
                }
                return;
            }
        }
    }
    // Fallback to internal icons.
    UCHAR i;
    for (i=0; i<3; i++)
        icons[i] = LoadIcon(g_hinst, iconstr[i]);
}

/////////////////////////////////////////////////////////////////////////////
static int InitTray()
{
    ScrollLockState = GetPrivateProfileInt(TEXT("Input"), TEXT("ScrollLockState"), 0, inipath);

    LoadAllIcons();

    // Create icondata
    tray.cbSize = sizeof(tray);
    tray.hWnd = g_hwnd;
    tray.uID = 0;
    tray.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tray.uCallbackMessage = WM_TRAY;

    // Register TaskbarCreated so we can re-add the tray icon if (when) explorer.exe crashes
    WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
    LOG("Register TaskbarCreated message: %X", WM_TASKBARCREATED);

    return 0;
}
/////////////////////////////////////////////////////////////////////////////
static int UpdateTray()
{
    int Index = !!ENABLED();
    if (Index) {
        Index += (ScrollLockState&1)
               && !( !(GetKeyState(VK_SCROLL)&1) ^ !(ScrollLockState&2) );
        if (GetProp(g_hwnd, APP_ASONOFF))
            Index=2;
    }
    // Load info tool tip and tray icon
    lstrcpy(tray.szTip, traystr[Index]);
    tray.hIcon = icons[Index];

    // Only add or modify if not hidden or if balloon will be displayed
    if (!hide || tray.uFlags&NIF_INFO) {
        // Try a few times, sleep 100 ms between each attempt
        int i=1;
        LOG("Updating tray icon");
        while (!Shell_NotifyIcon(tray_added? NIM_MODIFY: NIM_ADD, (void *)&tray) ) {
            LOG("Failed in try No. %d", i);

            // Maybe we just tried to add an already existing tray.
            // Happens after DPI change under Win 10 (TaskbarCreated) msg.
            if (!tray_added && Shell_NotifyIcon(NIM_MODIFY, (void *)&tray)) {
                LOG("Updated tray icon");
                tray_added = 1;
                return 0;
            }

            if (i > 2) {
                LOG("Failed all atempts!!");
                return 1;
            }
            Sleep(100);
            i++;
        }
        LOG("Sucess at try %d", i);
        // Success
        tray_added = 1;
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
static int RemoveTray()
{
    if (!tray_added)
        return 1;

    if (!Shell_NotifyIcon(NIM_DELETE, (void *)&tray))
        return 1;

    // Success
    tray_added = 0;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Zones functions
static TCHAR *RectToStr(RECT *rc, TCHAR *rectstr)
{
    TCHAR txt[16];
    UCHAR i;
    long *RC = (long *)rc;
    rectstr[0] = '\0';
    for(i = 0; i < 4; i++) {
        lstrcat(rectstr, itostr(RC[i], txt, 10));
        lstrcat(rectstr, TEXT(","));
    }
    return rectstr;
}
// Save a rect as a string in a Zone<num> entry in the inifile
static void SaveZone(RECT *rc, unsigned num)
{
    TCHAR txt[128], name[32];
    WritePrivateProfileString(TEXT("Zones"), ZidxToZonestr(num, name), RectToStr(rc, txt), inipath);
}
static void ClearAllZones()
{
    int i;
    TCHAR txt[128], name[32];
    for (i = 0; i < 32; i++) {
        ZidxToZonestr(i, name);
        if (GetPrivateProfileString(TEXT("Zones"), name, TEXT(""), txt, ARR_SZ(txt), inipath)) {
            WritePrivateProfileString(TEXT("Zones"), name, TEXT(""), inipath);
        }
    }
}
// Call with lParam = 1 to reset NZones
BOOL CALLBACK SaveTestWindow(HWND hwnd, LPARAM lParam)
{
    static unsigned NZones;
    if (lParam) { // Reset number of Zones
        NZones = 0;
        return FALSE;
    }

    TCHAR classn[256];
    RECT rc;
    if (IsWindowVisible(hwnd)
    && GetClassName(hwnd, classn, sizeof(classn))
    && !lstrcmp(classn, TEXT(APP_NAMEA"-Test"))
    && GetWindowRectL(hwnd, &rc)) {
        SaveZone(&rc, NZones++);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

static void SaveCurrentLayout()
{
    ClearAllZones();
    SaveTestWindow(NULL, 1);
    EnumThreadWindows(GetCurrentThreadId(), SaveTestWindow, 0);
}

/////////////////////////////////////////////////////////////////////////////
static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();

    AppendMenu(menu, MF_STRING, SWM_TOGGLE, (ENABLED()?l10n->menu_disable:l10n->menu_enable));
    AppendMenu(menu, MF_STRING, SWM_HIDE, l10n->menu_hide);
    if(VISTA)
        InsertMenu(menu, -1, elevated?MF_BYPOSITION|MF_GRAYED:MF_BYPOSITION
                 , SWM_ELEVATE, (elevated? l10n->general_elevated: l10n->general_elevate));

    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, SWM_CONFIG, l10n->menu_config);
    AppendMenu(menu, MF_STRING, SWM_ABOUT, l10n->menu_about);
    AppendMenu(menu, MF_STRING, SWM_OPENINIFILE, l10n->menu_openinifile);

    if (UseZones&1) { // Zones section
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, SWM_TESTWIN,  l10n->advanced_testwindow);
        AppendMenu(menu, FindWindow(TEXT(APP_NAMEA"-test"), NULL)? MF_STRING :MF_STRING|MF_GRAYED
                  , SWM_SAVEZONES, l10n->menu_savezones);
    }

    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, SWM_EXIT, l10n->menu_exit);

    // Track menu
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}
