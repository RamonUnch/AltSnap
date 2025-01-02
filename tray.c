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
static int LayoutNumber=0;
static int MaxLayouts=0;

static const TCHAR *iconstr[] = {
    TEXT("TRAY_OFF"),
    TEXT("TRAY_ON"),
    TEXT("TRAY_SUS")
};
static const TCHAR *traystr[] = {
    TEXT(APP_NAMEA)TEXT(" (Off)"),
    TEXT(APP_NAMEA)TEXT(" (On)"),
    TEXT(APP_NAMEA)TEXT("..."),
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
                    lstrcpy_s(p, ARR_SZ(path)-len, iconstr[i]);
                    lstrcat_s(path, ARR_SZ(path), TEXT(".ico"));
                    HICON tmp = (HICON)LoadImage(g_hinst, path, IMAGE_ICON,0,0, LR_LOADFROMFILE|LR_DEFAULTSIZE|LR_LOADTRANSPARENT);
                    icons[i] = tmp? tmp: LoadIcon(g_hinst, MAKEINTRESOURCE( TRAY_OFF+i ));
                }
                return;
            }
        }
    }
    // Fallback to internal icons.
    UCHAR i;
    for (i=0; i<3; i++)
        icons[i] = LoadIcon(g_hinst, MAKEINTRESOURCE( TRAY_OFF+i ));
}

/////////////////////////////////////////////////////////////////////////////
static int InitTray()
{
    ScrollLockState = GetPrivateProfileInt(TEXT("Input"), TEXT("ScrollLockState"), 0, inipath);
    LayoutNumber    = GetPrivateProfileInt(TEXT("Zones"), TEXT("LayoutNumber"), 0, inipath);
    MaxLayouts      = GetPrivateProfileInt(TEXT("Zones"), TEXT("MaxLayouts"), 0, inipath);
    MaxLayouts = CLAMP(0, MaxLayouts, 10);
    LayoutNumber = CLAMP(0, LayoutNumber, max(0,MaxLayouts-1));

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
    lstrcpy_s(tray.szTip, ARR_SZ(tray.szTip), traystr[Index]);
    tray.hIcon = icons[Index];

    // Only add or modify if not hidden or if balloon will be displayed
    if (!hide || tray.uFlags&NIF_INFO) {
        // Try a few times, sleep 100 ms between each attempt
        int i=1;
        LOG("Updating tray icon");
        while (!Shell_NotifyIcon(tray_added? NIM_MODIFY: NIM_ADD, (PNOTIFYICONDATA)&tray) ) {
            LOG("Failed in try No. %d", i);

            // Maybe we just tried to add an already existing tray.
            // Happens after DPI change under Win 10 (TaskbarCreated) msg.
            if (!tray_added && Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&tray)) {
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

    if (!Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&tray))
        return 1;

    // Success
    tray_added = 0;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Zones functions
static void WriteCurrentLayoutNumber()
{
    if (MaxLayouts) {
        TCHAR txt[UINT_DIGITS+1];
        WritePrivateProfileString(TEXT("Zones"), TEXT("LayoutNumber"), Uint2lStr(txt, LayoutNumber), inipath);
    }
}
static TCHAR *RectToStr(const RECT *rc, TCHAR rectstr[AT_LEAST INT_DIGITS*4+4+1])
{
    TCHAR txt[INT_DIGITS+1];
    int i;
    const long *RC = (const long *)rc;
    rectstr[0] = '\0';
    for(i = 0; i < 4; i++) {
        lstrcat_s(rectstr, 64, Int2lStr(txt, (int)RC[i]));
        lstrcat_s(rectstr, 64, TEXT(","));
    }
    return rectstr;
}
// Save a rect as a string in a Zone<num> entry in the inifile
static void SaveZone(const RECT *rc, unsigned num)
{
    TCHAR txt[64], name[32];
    WritePrivateProfileString(TEXT("Zones"), ZidxToZonestr(LayoutNumber, num, name), RectToStr(rc, txt), inipath);
}
static void ClearAllZones()
{
    int i;
    TCHAR txt[128], name[32];
    for (i = 0; i < 2048; i++) {
        ZidxToZonestr(LayoutNumber, i, name);
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
    && GetClassName(hwnd, classn, ARR_SZ(classn))
    && !lstrcmp(classn, TEXT(APP_NAMEA) TEXT("-Test"))
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

BOOL CALLBACK CloseTestWindowCB(HWND hwnd, LPARAM lParam)
{
    TCHAR classn[256];
    if (GetClassName(hwnd, classn, ARR_SZ(classn))
    && !lstrcmp(classn, TEXT(APP_NAMEA) TEXT("-Test")) ) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

static void CloseAllTestWindows()
{
    EnumThreadWindows(GetCurrentThreadId(), CloseTestWindowCB, 0);
}

static void catFullLayoutName(TCHAR *txt, size_t len, int laynum)
{
    TCHAR n1[UINT_DIGITS+1];
    lstrcat_s(txt, len, l10n->MenuSnapLayout);
    lstrcat_s(txt, len, Uint2lStr(n1, laynum+1));
    if (g_dllmsgHKhwnd) {
        DWORD rez =0;
        if ((rez = SendMessage(g_dllmsgHKhwnd, WM_GETLAYOUTREZ, laynum, 0))) {
            // TCHAR n2[UINT_DIGITS+1];
            // Add (width:height) to label the layout.
            // lstrcatM_s(txt, len ,TEXT("  ("), Uint2lStr(n1, LOWORD(rez)),TEXT(":"),Uint2lStr(n2, HIWORD(rez)), TEXT(")"), NULL);
            lstrcat_s(txt, len, TEXT("  ("));
            lstrcat_s(txt, len, Uint2lStr(n1, LOWORD(rez)));
            lstrcat_s(txt, len, TEXT(":"));
            lstrcat_s(txt, len, Uint2lStr(n1, HIWORD(rez)));
            lstrcat_s(txt, len, TEXT(")"));
        } else {
            lstrcat_s(txt, len, TEXT("  "));
            lstrcat_s(txt, len, l10n->MenuEmptyZone); // (empty)
        }
    } else {
        lstrcat_s(txt, len, TEXT("  (...)"));
    }
}

/////////////////////////////////////////////////////////////////////////////
static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();

    AppendMenu(menu, MF_STRING, SWM_TOGGLE, (ENABLED()?l10n->MenuDisable:l10n->MenuEnable));
    AppendMenu(menu, MF_STRING, SWM_HIDE, l10n->MenuHideTray);
    if(VISTA)
        InsertMenu(menu, -1, elevated?MF_BYPOSITION|MF_GRAYED:MF_BYPOSITION
                 , SWM_ELEVATE, (elevated? l10n->GeneralElevated: l10n->GeneralElevate));

    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, SWM_CONFIG, l10n->MenuConfigure);
    AppendMenu(menu, MF_STRING, SWM_ABOUT, l10n->MenuAbout);
    AppendMenu(menu, MF_STRING, SWM_OPENINIFILE, l10n->MenuOpenIniFile);

    if (UseZones&1) { // Zones section
        if(MaxLayouts)
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        int i;
        TCHAR txt[128];
        for (i=0; i < MaxLayouts; i++) {
            txt[0] = '\0';
            catFullLayoutName(txt, ARR_SZ(txt), i);
            // Check the current layout We use a simple checkmark,
            // because a radio button is more complex to setup.
            UINT mfflags = i==LayoutNumber? MF_STRING|MF_CHECKED: MF_STRING|MF_UNCHECKED;
            AppendMenu(menu, mfflags, SWM_SNAPLAYOUT+i, txt);
        }

        if (!(UseZones&2)) {
            TCHAR numstr[INT_DIGITS+1];
            lstrcpy_s(txt, ARR_SZ(txt), l10n->MenuEditLayout);
            lstrcat_s(txt, ARR_SZ(txt), TEXT(" "));
            lstrcat_s(txt, ARR_SZ(txt), Int2lStr(numstr, LayoutNumber+1));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            AppendMenu(menu, MF_STRING, SWM_TESTWIN,  l10n->AdvancedTestWindow);
            AppendMenu(menu, MF_STRING, SWM_EDITLAYOUT, txt);
            AppendMenu(menu, FindWindow(TEXT(APP_NAMEA)TEXT("-test"), NULL)? MF_STRING :MF_STRING|MF_GRAYED
                      , SWM_SAVEZONES, l10n->MenuSaveZones);

            AppendMenu(menu, FindWindow(TEXT(APP_NAMEA)TEXT("-test"), NULL)? MF_STRING :MF_STRING|MF_GRAYED
                      , SWM_CLOSEZONES, l10n->MenuCloseAllZones);
        }
    }

    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(menu, MF_STRING, SWM_EXIT, l10n->MenuExit);

    // Track menu
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}
