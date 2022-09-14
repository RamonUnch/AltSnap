/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <commctrl.h>
#include <windowsx.h>
#include "resource.h"

BOOL    CALLBACK PropSheetProc(HWND, UINT, LPARAM);
INT_PTR CALLBACK GeneralPageDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK MousePageDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK KeyboardPageDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK BlacklistPageDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutPageDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AdvancedPageDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FindWindowProc(HWND, UINT, WPARAM, LPARAM);

HWND g_cfgwnd = NULL;


/////////////////////////////////////////////////////////////////////////////
// No error reporting since we don't want the user to be interrupted
static void CheckAutostart(int *_on, int *_hidden, int *_elevated)
{
    *_on = *_hidden = *_elevated = 0;
    // Read registry
    HKEY key;
    wchar_t value[MAX_PATH+20] = L"";
    DWORD len = sizeof(value);
    RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &key);
    RegQueryValueEx(key, APP_NAME, NULL, NULL, (LPBYTE)value, &len);
    RegCloseKey(key);

    // Compare to what it should be
    wchar_t compare[MAX_PATH+20];
    GetModuleFileName(NULL, &compare[1], MAX_PATH);
    compare[0] = '\"';
    unsigned ll = wcslen(compare);
    compare[ll] = '\"'; compare[++ll]='\0';

    if (wcsstr(value, compare) != value) {
        return;
    }
    // Autostart is on, check arguments
    *_on = 1;
    if (wcsstr(value, L" -hide") != NULL) {
        *_hidden = 1;
    }
    if (wcsstr(value, L" -elevate") != NULL) {
        *_elevated = 1;
    }
}

/////////////////////////////////////////////////////////////////////////////
static void SetAutostart(int on, int hhide, int eelevate)
{
    // Open key
    HKEY key;
    int error = RegCreateKeyEx(HKEY_CURRENT_USER
                              , L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
                              , 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (error != ERROR_SUCCESS) return;
    if (on) {
        // Get path
        wchar_t value[MAX_PATH+20];
        GetModuleFileName(NULL, &value[1], MAX_PATH);
        value[0] = '\"';
        unsigned ll = wcslen(value);
        value[ll] = '\"'; value[++ll]='\0';
        // Add -hide or -elevate flags
        if (hhide)    wcscat(value, L" -hide");
        if (eelevate) wcscat(value, L" -elevate");
        // Set autostart
        RegSetValueEx(key, APP_NAME, 0, REG_SZ, (LPBYTE)value, (wcslen(value)+1)*sizeof(value[0]));
    } else {
        // Remove
        RegDeleteValue(key, APP_NAME);
    }
    // Close key
    RegCloseKey(key);
}

/////////////////////////////////////////////////////////////////////////////
// Only used in the case of Vista+
static BOOL ElevateNow(int showconfig)
{
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, ARR_SZ(path));
        INT_PTR ret;
        if (showconfig)
            ret = (INT_PTR)ShellExecute(NULL, L"runas", path, L"-config -multi", NULL, SW_SHOWNORMAL);
        else
            ret = (INT_PTR)ShellExecute(NULL, L"runas", path, L"-multi", NULL, SW_SHOWNORMAL);

        if (ret > 32) {
            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
        } else {
            MessageBox(NULL, l10n->general_elevation_aborted, APP_NAME, MB_ICONINFORMATION | MB_OK);
        }
        return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
// Entry point
static void OpenConfig(int startpage)
{
    if (IsWindow(g_cfgwnd)) {
        PropSheet_SetCurSel(g_cfgwnd, 0, startpage);
        SetForegroundWindow(g_cfgwnd);
        return;
    }
    // Define the pages
    static const struct {
        WORD pszTemplate;
        DLGPROC pfnDlgProc;
    } pages[] = {
        { IDD_GENERALPAGE,   GeneralPageDialogProc  },
        { IDD_MOUSEPAGE,     MousePageDialogProc    },
        { IDD_KBPAGE,        KeyboardPageDialogProc },
        { IDD_BLACKLISTPAGE, BlacklistPageDialogProc},
        { IDD_ADVANCEDPAGE,  AdvancedPageDialogProc },
        { IDD_ABOUTPAGE,     AboutPageDialogProc    }
    };
    PROPSHEETPAGE psp[ARR_SZ(pages)];
    memset(&psp[0], 0, sizeof(psp));
    size_t i;
    for (i = 0; i < ARR_SZ(pages); i++) {
        psp[i].dwSize = sizeof(PROPSHEETPAGE);
        psp[i].hInstance = g_hinst;
        psp[i].pszTemplate = MAKEINTRESOURCE(pages[i].pszTemplate);
        psp[i].pfnDlgProc = pages[i].pfnDlgProc;
    }

    // Define the property sheet
    PROPSHEETHEADER psh = { sizeof(PROPSHEETHEADER) };
    psh.dwFlags = PSH_PROPSHEETPAGE|PSH_USECALLBACK|PSH_USEHICON;
    psh.hwndParent = NULL;
    psh.hInstance = g_hinst;
    psh.hIcon = LoadIconA(g_hinst, iconstr[1]);
    psh.pszCaption = APP_NAME;
    psh.nPages = ARR_SZ(pages);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = PropSheetProc;
    psh.nStartPage = startpage;

    // Open the property sheet
    InitCommonControls();
    PropertySheet(&psh);
}
/////////////////////////////////////////////////////////////////////////////
static void CloseConfig()
{
    PostMessage(g_cfgwnd, WM_CLOSE, 0, 0);
}
static void UpdateSettings()
{
    PostMessage(g_hwnd, WM_UPDATESETTINGS, 0, 0);
}
static void MoveButtonUporDown(WORD id, WINDOWPLACEMENT *wndpl, int diffrows)
{
    HWND button = GetDlgItem(g_cfgwnd, id);
    GetWindowPlacement(button, wndpl);
    int height = wndpl->rcNormalPosition.bottom - wndpl->rcNormalPosition.top;
    wndpl->rcNormalPosition.top += 18 * diffrows;
    wndpl->rcNormalPosition.bottom = wndpl->rcNormalPosition.top + height;
    SetWindowPlacement(button, wndpl);
}
/////////////////////////////////////////////////////////////////////////////
static void UpdateStrings()
{
    // Update window title
    PropSheet_SetTitle(g_cfgwnd, 0, l10n->title);

    // Update tab titles
    // tc = PropSheet_GetTabControl(g_cfgwnd);
    HWND tc = (HWND)SendMessage(g_cfgwnd, PSM_GETTABCONTROL, 0, 0);
    int numrows_prev = TabCtrl_GetRowCount(tc);
    wchar_t *titles[6];
    titles[0] = l10n->tab_general;
    titles[1] = l10n->tab_mouse;
    titles[2] = l10n->tab_keyboard;
    titles[3] = l10n->tab_blacklist;
    titles[4] = l10n->tab_advanced;
    titles[5] = l10n->tab_about;
    size_t i;
    for (i = 0; i < ARR_SZ(titles); i++) {
        TCITEM ti;
        ti.mask = TCIF_TEXT;
        ti.pszText = titles[i];
        TabCtrl_SetItem(tc, i, &ti);
    }

    // Modify UI if number of rows have changed
    int numrows = TabCtrl_GetRowCount(tc);
    if (numrows_prev != numrows) {
        HWND page = PropSheet_GetCurrentPageHwnd(g_cfgwnd);
        if (page != NULL) {
            int diffrows = numrows - numrows_prev;
            WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
            // Resize window
            GetWindowPlacement(g_cfgwnd, &wndpl);
            wndpl.rcNormalPosition.bottom += 18 * diffrows;
            SetWindowPlacement(g_cfgwnd, &wndpl);
            // Resize tabcontrol
            GetWindowPlacement(tc, &wndpl);
            wndpl.rcNormalPosition.bottom += 18 * diffrows;
            SetWindowPlacement(tc, &wndpl);
            // Move button
            MoveButtonUporDown(IDOK,     &wndpl, diffrows);
            MoveButtonUporDown(IDCANCEL, &wndpl, diffrows);
            MoveButtonUporDown(IDAPPLY,  &wndpl, diffrows);
            // Re-select tab
            PropSheet_SetCurSel(g_cfgwnd, page, 0);
            // Invalidate region
            GetWindowPlacement(g_cfgwnd, &wndpl);
            InvalidateRect(g_cfgwnd, &wndpl.rcNormalPosition, TRUE);
        }
    }
}
static void MoveToCorner(HWND hwnd)
{
    static HWND ohwnd;
    if(hwnd == ohwnd) return;
    ohwnd = hwnd;

    RECT rc;
    MONITORINFO mi = { sizeof(MONITORINFO) };
    POINT pt;
    GetCursorPos(&pt);
    GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi);

    GetWindowRect(hwnd, &rc);
    int x, y;
    long h = rc.bottom - rc.top;
    long w = rc.right - rc.left;
    if (pt.x < (mi.rcWork.right - mi.rcWork.left)/2) {
        x = mi.rcWork.left;
    } else {
        x = mi.rcWork.right - w;
    }
    if (pt.y < (mi.rcWork.bottom - mi.rcWork.top)/2) {
        y = mi.rcWork.top;
    } else {
        y = mi.rcWork.bottom - h;
    }
    SetWindowPos(hwnd, NULL, x, y, w, h
        , SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK PropSheetProc(HWND hwnd, UINT msg, LPARAM lParam)
{
    if (msg == PSCB_INITIALIZED) {
        g_cfgwnd = hwnd;
        UpdateStrings();

        // Set new icon specifically for the taskbar and Alt+Tab, without changing window icon
        HICON taskbar_icon = LoadImageA(g_hinst, "APP_ICON", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        SendMessage(g_cfgwnd, WM_SETICON, ICON_BIG, (LPARAM) taskbar_icon);
    }
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
static DWORD IsUACEnabled()
{
    DWORD uac_enabled = 0;
    if (elevated) {
        DWORD len = sizeof(uac_enabled);
        HKEY key;
        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE
          , L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"
          , 0, KEY_QUERY_VALUE, &key);

        RegQueryValueEx(key, L"EnableLUA", NULL, NULL, (LPBYTE) &uac_enabled, &len);
        RegCloseKey(key);
    }
    return uac_enabled;
}
/////////////////////////////////////////////////////////////////////////////
// Helper functions and Macro
#define IsChecked(idc) Button_GetCheck(GetDlgItem(hwnd, idc))
#define IsCheckedW(idc) _itow(IsChecked(idc), txt, 10)
static void WriteOptionBoolW(HWND hwnd, WORD id, const wchar_t *section, const char *name_s)
{
    wchar_t txt[8];
    wchar_t name[64];
    str2wide(name, name_s);
    WritePrivateProfileString(section, name,_itow(Button_GetCheck(GetDlgItem(hwnd, id)), txt, 10), inipath);
}
#define WriteOptionBool(id, section, name) WriteOptionBoolW(hwnd, id, section, name)
static int WriteOptionBoolBW(HWND hwnd, WORD id, const wchar_t *section, const char *name_s, int bit)
{
    wchar_t txt[8];
    wchar_t name[64];
    str2wide(name, name_s);
    int val = GetPrivateProfileInt(section, name, 0, inipath);
    if (Button_GetCheck(GetDlgItem(hwnd, id)))
        val = setBit(val, bit);
    else
        val = clearBit(val, bit);

    WritePrivateProfileString(section, name, _itow(val, txt, 10), inipath);
    return val;
}
#define WriteOptionBoolB(id, section, name, bit) WriteOptionBoolBW(hwnd, id, section, name, bit)

static void WriteOptionStrW(HWND hwnd, WORD id, const wchar_t *section, const char *name_s)
{
    wchar_t txt[1024];
    wchar_t name[64];
    str2wide(name, name_s);
    Edit_GetText(GetDlgItem(hwnd, id), txt, ARR_SZ(txt));
    WritePrivateProfileString(section, name, txt, inipath);
}
#define WriteOptionStr(id, section, name)  WriteOptionStrW(hwnd, id, section, name)

static void ReadOptionStrW(HWND hwnd, WORD id, const wchar_t *section, const char *name_s, const wchar_t *def)
{
    wchar_t txt[1024];
    wchar_t name[64];
    str2wide(name, name_s);
    GetPrivateProfileString(section, name, def, txt, ARR_SZ(txt), inipath);
    SetDlgItemText(hwnd, id, txt);
}
#define ReadOptionStr(id, section, name, def) ReadOptionStrW(hwnd, id, section, name, def)

static int ReadOptionIntW(HWND hwnd, WORD id, const wchar_t *section, const char *name_s, int def, int mask)
{
    wchar_t name[64];
    str2wide(name, name_s);
    int ret = GetPrivateProfileInt(section, name, def, inipath);
    Button_SetCheck(GetDlgItem(hwnd, id), (ret&mask)? BST_CHECKED: BST_UNCHECKED);
    return ret;
}
#define ReadOptionInt(id, section, name, def, mask) ReadOptionIntW(hwnd, id, section, name, def, mask)

struct dialogstring { const int idc; const wchar_t *string; };
static void UpdateDialogStrings(HWND hwnd, const struct dialogstring * const strlst, unsigned size)
{
    unsigned i;
    for (i=0; i < size; i++) {
        SetDlgItemText(hwnd, strlst[i].idc, strlst[i].string);
    }
}
// Options to bead or written...
enum opttype {T_BOL, T_BMK, T_STR};
struct optlst {
    const short idc;
    const enum opttype type;
    const UCHAR bitN;
    const wchar_t *section;
    const char *name;
    const void *def;
};
static void ReadDialogOptions(HWND hwnd,const struct optlst *ol, unsigned size)
{
    unsigned i;
    for (i=0; i < size; i++) {
        if (ol[i].type == T_BOL)
            ReadOptionIntW(hwnd, ol[i].idc, ol[i].section, ol[i].name, (int)(DorQWORD)ol[i].def, -1);
        else if (ol[i].type == T_BMK)
            ReadOptionIntW(hwnd, ol[i].idc, ol[i].section, ol[i].name, (int)(DorQWORD)ol[i].def, 1<<ol[i].bitN);
        else
            ReadOptionStrW(hwnd, ol[i].idc, ol[i].section, ol[i].name, (wchar_t*)ol[i].def);
    }
}
static void WriteDialogOptions(HWND hwnd,const struct optlst *ol, unsigned size)
{
    unsigned i;
    for (i=0; i < size; i++) {
        if (ol[i].type == T_BOL)
            WriteOptionBoolW(hwnd, ol[i].idc, ol[i].section, ol[i].name);
        else if(ol[i].type == T_BMK)
            WriteOptionBoolBW(hwnd, ol[i].idc, ol[i].section, ol[i].name, ol[i].bitN);
        else
            WriteOptionStrW(hwnd, ol[i].idc, ol[i].section, ol[i].name);
    }
}

/////////////////////////////////////////////////////////////////////////////
// to be used with EnumDesktopWindows()
BOOL CALLBACK RefreshTestWin(HWND hwnd, LPARAM lp)
{
    wchar_t classn[256];
    if (GetClassName(hwnd, classn, ARR_SZ(classn))
    && !wcscmp(APP_NAME"-Test", classn) ) {
        PostMessage(hwnd, WM_UPDCFRACTION, 0, 0);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0
            , SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE );
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK GeneralPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    #pragma GCC diagnostic ignored "-Wint-conversion"
    static const struct optlst optlst[] = {
       // dialog id, type, bit number, section name, option name, def val.
        { IDC_AUTOFOCUS,     T_BOL, 0,  L"General",  "AutoFocus", 0 },
        { IDC_AERO,          T_BOL, 0,  L"General",  "Aero", 1 },
        { IDC_SMARTAERO,     T_BMK, 0,  L"General",  "SmartAero", 1 },
        { IDC_SMARTERAERO,   T_BMK, 1,  L"General",  "SmartAero", 0 },
        { IDC_STICKYRESIZE,  T_BOL, 0,  L"General",  "StickyResize", 1 },
        { IDC_INACTIVESCROLL,T_BOL, 0,  L"General",  "InactiveScroll", 0 },
        { IDC_MDI,           T_BOL, 0,  L"General",  "MDI", 1 },
        { IDC_RESIZEALL,     T_BOL, 0,  L"Advanced", "ResizeAll", 1 },
        { IDC_USEZONES,      T_BMK, 0,  L"Zones",    "UseZones", 0 },
        { IDC_PIERCINGCLICK, T_BOL, 0,  L"Advanced", "PiercingClick", 0 },
    };
    #pragma GCC diagnostic pop

    int updatestrings = 0;
    static int have_to_apply = 0;
    if (msg == WM_INITDIALOG) {
        MoveToCorner(g_cfgwnd);
        int ret;
        ReadDialogOptions(hwnd, optlst, ARR_SZ(optlst));

        ret = GetPrivateProfileInt(L"General", L"ResizeCenter", 1, inipath);
        ret = ret==1? IDC_RZCENTER_NORM: ret==2? IDC_RZCENTER_MOVE: ret==3? IDC_RZCENTER_CLOSE: IDC_RZCENTER_BR;
        CheckRadioButton(hwnd, IDC_RZCENTER_NORM, IDC_RZCENTER_CLOSE, ret);

        HWND control = GetDlgItem(hwnd, IDC_LANGUAGE);
        ComboBox_ResetContent(control);
        ComboBox_Enable(control, TRUE);
        int i;
        for (i = 0; i < nlanguages; i++) {
            ComboBox_AddString(control, langinfo[i].lang);
            if (langinfo[i].code && !wcsicmp(l10n->code, langinfo[i].code) ) {
                ComboBox_SetCurSel(control, i);
            }
        }
        Button_Enable(GetDlgItem(hwnd, IDC_ELEVATE), VISTA && !elevated);

    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);
        HWND control = GetDlgItem(hwnd, id);
        int val = Button_GetCheck(control);
        if (id == IDC_SMARTAERO) {
            Button_Enable(GetDlgItem(hwnd, IDC_SMARTERAERO), IsChecked(IDC_SMARTAERO));
        }

        if (id != IDC_ELEVATE && (event == 0 ||  event == CBN_SELCHANGE)) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
        if (IDC_RZCENTER_NORM <= id && id <= IDC_RZCENTER_CLOSE) {
            CheckRadioButton(hwnd, IDC_RZCENTER_NORM, IDC_RZCENTER_CLOSE, id);
        } else if (id == IDC_AUTOSTART) {
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), val);
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), val && VISTA);
            if (!val) {
                Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), BST_UNCHECKED);
            }
        } else if (id == IDC_AUTOSTART_ELEVATE) {
            if (val && IsUACEnabled()) {
                MessageBox(NULL, l10n->general_autostart_elevate_tip, APP_NAME, MB_ICONINFORMATION | MB_OK);
            }
        } else if (id == IDC_ELEVATE) {
            return ElevateNow(1);
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            updatestrings = 1;

            // Autostart
            int autostart = 0, hidden = 0, eelevated = 0;
            CheckAutostart(&autostart, &hidden, &eelevated);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART), autostart ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), hidden ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), eelevated ? BST_CHECKED : BST_UNCHECKED);
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), autostart);
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), autostart && VISTA);
            if(WIN10) Button_Enable(GetDlgItem(hwnd, IDC_INACTIVESCROLL), IsChecked(IDC_INACTIVESCROLL));
        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // all bool options (checkboxes).
            WriteDialogOptions(hwnd, optlst, ARR_SZ(optlst));

            wchar_t txt[8];
            int val = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_AUTOSNAP));
            WritePrivateProfileString(L"General",    L"AutoSnap", _itow(val, txt, 10), inipath);

            val = IsChecked(IDC_RZCENTER_NORM)? 1: IsChecked(IDC_RZCENTER_MOVE)? 2:IsChecked(IDC_RZCENTER_CLOSE)? 3: 0;
            WritePrivateProfileString(L"General",    L"ResizeCenter", _itow(val, txt, 10), inipath);

            // Load selected Language
            int i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LANGUAGE));
            i = min(i, nlanguages);
            LoadTranslation(langinfo[i].fn);
            WritePrivateProfileString(L"General", L"Language", l10n->code, inipath);
            updatestrings = 1;
            UpdateStrings();

            // Autostart
            SetAutostart(IsChecked(IDC_AUTOSTART), IsChecked(IDC_AUTOSTART_HIDE), IsChecked(IDC_AUTOSTART_ELEVATE));

            UpdateSettings();
            // Update Test windows in if open.
            EnumThreadWindows(GetCurrentThreadId(), RefreshTestWin, 0);

            have_to_apply = 0;
        }
//    } else if (msg == WM_HELP) {
//        LPHELPINFO hli = (LPHELPINFO)lParam;
//        switch(hli->iCtrlId){
//        case IDC_AUTOFOCUS: MessageBoxA(NULL, NULL, NULL, 0);
//        }
    }
    if (updatestrings) {
        // Update text
        const struct dialogstring strlst[] = {
            { IDC_GENERAL_BOX,      l10n->general_box },
            { IDC_AUTOFOCUS,        l10n->general_autofocus },
            { IDC_AERO,             l10n->general_aero },
            { IDC_SMARTAERO,        l10n->general_smartaero },
            { IDC_SMARTERAERO,      l10n->general_smarteraero },
            { IDC_STICKYRESIZE,     l10n->general_stickyresize },
            { IDC_INACTIVESCROLL,   l10n->general_inactivescroll },
            { IDC_MDI,              l10n->general_mdi },
            { IDC_AUTOSNAP_HEADER,  l10n->general_autosnap },
            { IDC_LANGUAGE_HEADER,  l10n->general_language },
            { IDC_USEZONES,         l10n->general_usezones },
            { IDC_PIERCINGCLICK,    l10n->general_piercingclick },
            { IDC_RESIZEALL,        l10n->general_resizeall },
            { IDC_RESIZECENTER,     l10n->general_resizecenter },
            { IDC_RZCENTER_NORM,    l10n->general_resizecenter_norm },
            { IDC_RZCENTER_BR,      l10n->general_resizecenter_br },
            { IDC_RZCENTER_MOVE,    l10n->general_resizecenter_move },
            { IDC_RZCENTER_CLOSE,   l10n->general_resizecenter_close },
            { IDC_AUTOSTART_BOX,    l10n->general_autostart_box },
            { IDC_AUTOSTART,        l10n->general_autostart },
            { IDC_AUTOSTART_HIDE,   l10n->general_autostart_hide },
            { IDC_AUTOSTART_ELEVATE,l10n->general_autostart_elevate }
        };
        UpdateDialogStrings(hwnd, strlst, ARR_SZ(strlst));
        // spetial case...
        SetDlgItemText(hwnd, IDC_ELEVATE, elevated?l10n->general_elevated: l10n->general_elevate);

        // AutoSnap
        HWND control = GetDlgItem(hwnd, IDC_AUTOSNAP);
        ComboBox_ResetContent(control);
        ComboBox_AddString(control, l10n->general_autosnap0);
        ComboBox_AddString(control, l10n->general_autosnap1);
        ComboBox_AddString(control, l10n->general_autosnap2);
        ComboBox_AddString(control, l10n->general_autosnap3);
        wchar_t txt[8];
        GetPrivateProfileString(L"General", L"AutoSnap", L"0", txt, ARR_SZ(txt), inipath);
        ComboBox_SetCurSel(control, _wtoi(txt));

        // Language
        control = GetDlgItem(hwnd, IDC_LANGUAGE);
        ComboBox_DeleteString(control, nlanguages);
    }
    return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
static int IsKeyInList(wchar_t *keys, unsigned vkey)
{
    unsigned temp, numread;
    wchar_t *pos = keys;
    while (*pos != '\0') {
        numread = 0;
        temp = whex2u(pos);
        while (pos[numread] && pos[numread] != ' ') numread++;
        while (pos[numread] == ' ') numread++;
        if (temp == vkey) {
            return 1;
        }
        pos += numread;
    }
    return 0;
}
static void AddvKeytoList(wchar_t *keys, unsigned vkey)
{
    // Check if it is already in the list.
    if (IsKeyInList(keys, vkey))
        return;
    // Add a key to the hotkeys list
    if (*keys != '\0') {
        wcscat(keys, L" ");
    }
    wchar_t buf[8];
    wcscat(keys, _itow(vkey, buf, 16));
}
static void RemoveKeyFromList(wchar_t *keys, unsigned vkey)
{
    // Remove the key from the hotclick list
    unsigned temp, numread;
    wchar_t *pos = keys;
    while (*pos != '\0') {
        numread = 0;
        temp = whex2u(pos);
        while(pos[numread] && pos[numread] != ' ') numread++;
        while(pos[numread] == ' ') numread++;
        if (temp == vkey) {
            keys[pos - keys] = '\0';
            wcscat(keys, pos + numread);
            break;
        }
        pos += numread;
    }
    // Strip eventual remaining spaces
    unsigned ll = wcslen(keys);
    while (ll > 0 && keys[--ll] == ' ') keys[ll]='\0';
}
/////////////////////////////////////////////////////////////////////////////
struct hk_struct {
    unsigned control;
    unsigned vkey;
};
static void SaveHotKeys(const struct hk_struct *hotkeys, HWND hwnd, const wchar_t *name)
{
    wchar_t keys[32];
    // Get the current config in case there are some user added keys.
    GetPrivateProfileString(L"Input", name, L"", keys, ARR_SZ(keys), inipath);
    unsigned i;
    for (i = 0; hotkeys[i].control; i++) {
         if (IsChecked(hotkeys[i].control)) {
             AddvKeytoList(keys, hotkeys[i].vkey);
         } else {
             RemoveKeyFromList(keys, hotkeys[i].vkey);
         }
    }
    WritePrivateProfileString(L"Input", name, keys, inipath);
}
/////////////////////////////////////////////////////////////////////////////
static void CheckConfigHotKeys(const struct hk_struct *hotkeys, HWND hwnd, const wchar_t *hotkeystr, const wchar_t* def)
{
    // Hotkeys
    size_t i;
    unsigned temp;
    wchar_t txt[32];
    GetPrivateProfileString(L"Input", hotkeystr, def, txt, ARR_SZ(txt), inipath);
    wchar_t *pos = txt;
    while (*pos != '\0') {
        temp = whex2u(pos);
        while(*pos && *pos != ' ') pos++;
        while(*pos == ' ') pos++;

        // What key was that?
        for (i = 0; hotkeys[i].control ; i++) {
            if (temp == hotkeys[i].vkey) {
                Button_SetCheck(GetDlgItem(hwnd, hotkeys[i].control), BST_CHECKED);
                break;
            }
        }
    }
}

struct actiondl {
    wchar_t *action;
    wchar_t *l10n;
};
static void FillActionDropListS(HWND hwnd, int idc, wchar_t *inioption, struct actiondl *actions)
{
    HWND control = GetDlgItem(hwnd, idc);
    wchar_t txt[64];
    ComboBox_ResetContent(control);
    if (inioption)
        GetPrivateProfileString(L"Input", inioption, L"Nothing", txt, ARR_SZ(txt), inipath);
    int sel, j;
    sel = -!!inioption;
    for (j = 0; actions[j].action; j++) {
        wchar_t action_name[256];
        wcscpy_noaccel(action_name, actions[j].l10n, ARR_SZ(action_name));
        ComboBox_AddString(control, action_name);
        if (!wcscmp(txt, actions[j].action)) {
            sel = j;
        }
    }
    if (sel < 0) {
        // sel is negative if the string was not found in the struct actiondl:
        // UNKNOWN ACTION, so we add it manually at the end of the list
        ComboBox_AddString(control, txt);
        sel = j; // And select this unknown action.
    }
    ComboBox_SetCurSel(control, sel);
}
static void WriteActionDropListS(HWND hwnd, int idc, wchar_t *inioption, struct actiondl *actions)
{
    HWND control = GetDlgItem(hwnd, idc);
    int j = ComboBox_GetCurSel(control);
    if (actions[j].action) // Inside of known values
        WritePrivateProfileString(L"Input", inioption, actions[j].action, inipath);
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK MousePageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int have_to_apply = 0;
    // Mouse actions
    static const struct {
        int control; // Same control
        wchar_t *option[3]; // Prim/alt/TTB
    } mouse_buttons[] = {
        { IDC_LMB,     {L"LMB", L"LMBB", L"LMBT"} },
        { IDC_MMB,     {L"MMB", L"MMBB", L"MMBT"} },
        { IDC_RMB,     {L"RMB", L"RMBB", L"RMBT"} },
        { IDC_MB4,     {L"MB4", L"MB4B", L"MB4T"} },
        { IDC_MB5,     {L"MB5", L"MB5B", L"MB5T"} },
        { IDC_MOVEUP,  {L"MoveUp", L"MoveUpB", L"MoveUpT"} },
        { IDC_RESIZEUP,{L"ResizeUp", L"ResizeUpB", L"ResizeUpT"} },
    };

    struct actiondl mouse_actions[] = {
        {L"Move",        l10n->input_actions_move},
        {L"Resize",      l10n->input_actions_resize},
//        {L"Restore",     l10n->input_actions_restore},
        {L"Close",       l10n->input_actions_close},
        {L"Kill",        l10n->input_actions_kill},
        {L"Minimize",    l10n->input_actions_minimize},
        {L"Maximize",    l10n->input_actions_maximize},
        {L"Lower",       l10n->input_actions_lower},
        {L"NStacked",    l10n->input_actions_nstacked},
        {L"PStacked",    l10n->input_actions_pstacked},
        {L"StackList",   l10n->input_actions_stacklist},
        {L"StackList2",  l10n->input_actions_stacklist2},
        {L"Roll",        l10n->input_actions_roll},
        {L"AlwaysOnTop", l10n->input_actions_alwaysontop},
        {L"Borderless",  l10n->input_actions_borderless},
        {L"Center",      l10n->input_actions_center},
        {L"MaximizeHV",  l10n->input_actions_maximizehv},
        {L"SideSnap",    l10n->input_actions_sidesnap},
        {L"MinAllOther", l10n->input_actions_minallother},
        {L"Mute",        l10n->input_actions_mute},
        {L"Menu",        l10n->input_actions_menu},
        {L"Nothing",     l10n->input_actions_nothing},
        {NULL, NULL}
    };

    // Scroll
    static const struct {
        int control; // Same control
        wchar_t *option[3]; // Prim/alt/TTB
    } mouse_wheels[] = {
        { IDC_SCROLL,  {L"Scroll",  L"ScrollB",  L"ScrollT"}  },
        { IDC_HSCROLL, {L"HScroll", L"HScrollB", L"HScrollT"} }
    };

    struct actiondl scroll_actions[] = {
        {L"AltTab",       l10n->input_actions_alttab},
        {L"Volume",       l10n->input_actions_volume},
        {L"Transparency", l10n->input_actions_transparency},
        {L"Zoom",         l10n->input_actions_zoom},
        {L"Zoom2",        l10n->input_actions_zoom2},
        {L"Lower",        l10n->input_actions_lower},
        {L"Roll",         l10n->input_actions_roll},
        {L"Maximize",     l10n->input_actions_maximize},
        {L"NPStacked",    l10n->input_actions_npstacked},
        {L"HScroll",      l10n->input_actions_hscroll},
        {L"Nothing",      l10n->input_actions_nothing},
        {NULL, NULL}
    };

    // Hotkeys
    static const struct hk_struct hotclicks [] = {
        { IDC_MMB_HC, 0x04 },
        { IDC_MB4_HC, 0x05 },
        { IDC_MB5_HC, 0X06 },
        { 0, 0 }
    };

    static const struct optlst optlst[] = {
        { IDC_TTBACTIONSNA,  T_BMK, 0, L"Input", "TTBActions", 0    },
        { IDC_TTBACTIONSWA,  T_BMK, 1, L"Input", "TTBActions", 0    },
        { IDC_LONGCLICKMOVE, T_BOL, 0, L"Input", "LongClickMove", 0 }
    };

    if (msg == WM_INITDIALOG) {
        // Hotclicks buttons
        ReadDialogOptions(hwnd, optlst, ARR_SZ(optlst));
        CheckRadioButton(hwnd, IDC_MBA1, IDC_INTTB, IDC_MBA1); // Check the primary action
        CheckConfigHotKeys(hotclicks, hwnd, L"Hotclicks", L"");
    } else if (msg == WM_COMMAND) {
        int event = HIWORD(wParam);
        int id = LOWORD(wParam);
        if (id >= IDC_MBA1 && id <= IDC_INTTB) {
            CheckRadioButton(hwnd, IDC_MBA1, IDC_INTTB, id); // Check the selected action
            goto FILLACTIONS;
        }

        if (event == 0 || event == CBN_SELCHANGE){
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            wchar_t txt[8];
            GetPrivateProfileString(L"Input", L"ModKey", L"", txt, ARR_SZ(txt), inipath);
            Button_Enable(GetDlgItem(hwnd, IDC_MBA2), txt[0]);
            // Disable inside ttb
            Button_Enable(GetDlgItem(hwnd, IDC_INTTB), IsChecked(IDC_TTBACTIONSNA)||IsChecked(IDC_TTBACTIONSWA));

            FILLACTIONS:;
            unsigned i;
            // Mouse actions
            int optoff = IsChecked(IDC_MBA2)? 1:IsChecked(IDC_INTTB)? 2: 0;
            for (i = 0; i < ARR_SZ(mouse_buttons); i++) {
                FillActionDropListS(hwnd, mouse_buttons[i].control, mouse_buttons[i].option[optoff], mouse_actions);
            }
            // Scroll actions
            for (i = 0; i < ARR_SZ(mouse_wheels); i++) {
                FillActionDropListS(hwnd, mouse_wheels[i].control, mouse_wheels[i].option[optoff], scroll_actions);
            }

            // Update text
            const struct dialogstring strlst[] = {
                { IDC_MBA1,            l10n->input_mouse_btac1 },
                { IDC_MBA2,            l10n->input_mouse_btac2 },
                { IDC_INTTB,           l10n->input_mouse_inttb },

                { IDC_MOUSE_BOX,       l10n->input_mouse_box },
                { IDC_LMB_HEADER,      l10n->input_mouse_lmb },
                { IDC_MMB_HEADER,      l10n->input_mouse_mmb },
                { IDC_RMB_HEADER,      l10n->input_mouse_rmb },
                { IDC_MB4_HEADER,      l10n->input_mouse_mb4 },
                { IDC_MB5_HEADER,      l10n->input_mouse_mb5 },
                { IDC_SCROLL_HEADER,   l10n->input_mouse_scroll },
                { IDC_HSCROLL_HEADER,  l10n->input_mouse_hscroll },
                { IDC_MOVEUP_HEADER,   l10n->input_mouse_moveup },
                { IDC_RESIZEUP_HEADER, l10n->input_mouse_resizeup },
                { IDC_TTBACTIONS_BOX,  l10n->input_mouse_ttbactions_box },
                { IDC_TTBACTIONSNA,    l10n->input_mouse_ttbactionsna },
                { IDC_TTBACTIONSWA,    l10n->input_mouse_ttbactionswa },

                { IDC_HOTCLICKS_BOX,   l10n->input_hotclicks_box },
                { IDC_HOTCLICKS_MORE,  l10n->input_hotclicks_more },
                { IDC_MMB_HC,          l10n->input_mouse_mmb_hc },
                { IDC_MB4_HC,          l10n->input_mouse_mb4_hc },
                { IDC_MB5_HC,          l10n->input_mouse_mb5_hc },
                { IDC_LONGCLICKMOVE,   l10n->input_mouse_longclickmove },
            };
            UpdateDialogStrings(hwnd, strlst, ARR_SZ(strlst));

        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // Mouse actions, for all mouse buttons...
            unsigned i;
            // Add 2 if in titlear add one for secondary action.
            int optoff = IsChecked(IDC_MBA2)? 1:IsChecked(IDC_INTTB)? 2: 0;
            for (i = 0; i < ARR_SZ(mouse_buttons); i++) {
                WriteActionDropListS(hwnd, mouse_buttons[i].control, mouse_buttons[i].option[optoff], mouse_actions);
            }
            // Scroll
            for (i = 0; i < ARR_SZ(mouse_wheels); i++) {
                WriteActionDropListS(hwnd, mouse_wheels[i].control, mouse_wheels[i].option[optoff], scroll_actions);
            }

            // Checkboxes...
            WriteDialogOptions(hwnd, optlst, ARR_SZ(optlst));
            // Hotclicks
            SaveHotKeys(hotclicks, hwnd, L"Hotclicks");
            UpdateSettings();
            have_to_apply = 0;
        }
    }
    return FALSE;
}

static BOOL CALLBACK EnableAllControlsProc(HWND hwnd, LPARAM lp)
{
    if (IsWindowVisible(hwnd)) {
        EnableWindow(hwnd, lp);
    }
    return TRUE;
}
static LRESULT WINAPI PickShortcutWinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static const UCHAR vkbl[] = {
        VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
        VK_SHIFT, VK_LSHIFT, VK_RSHIFT,
        VK_MENU, VK_LMENU, VK_RMENU,
        VK_LWIN, VK_RWIN
        , '\0'
    };
//    static int was_enabled;

    switch (msg) {
//    case WM_CREATE: {
//        was_enabled = ENABLED();
//        if (was_enabled) UnhookSystem();
//    } break;
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSDEADCHAR:
    case WM_SYSCHAR:
        break;
    case WM_KEYUP: {
        if (!IsHotkeyy((UCHAR)wp, vkbl)) {
            wchar_t txt[16];
            HWND phwnd = GetParent(hwnd);
            SetDlgItemText(phwnd, IDC_SHORTCUTS_VK, _itow(wp, txt, 16));

            Button_SetCheck(GetDlgItem(phwnd, IDC_SHIFT),  GetKeyState(VK_SHIFT)&0x8000? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(phwnd, IDC_CONTROL),GetKeyState(VK_CONTROL)&0x8000? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(phwnd, IDC_WINKEY), GetKeyState(VK_LWIN)&0x8000||GetKeyState(VK_RWIN)&0x8000? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(phwnd, IDC_ALT),    GetKeyState(VK_MENU)&0x8000? BST_CHECKED: BST_UNCHECKED);
            DestroyWindow(hwnd);
        }
    } break;
    case WM_LBUTTONDOWN:
    case WM_KILLFOCUS: {
        DestroyWindow(hwnd);
    } break;

    case WM_DESTROY: {
        HWND phwnd = GetParent(hwnd);
        Button_Enable(GetDlgItem(phwnd, IDC_SHORTCUTS_PICK), 1);
        EnumChildWindows(phwnd, EnableAllControlsProc, 1);
        NMHDR lpn;
        lpn.hwndFrom = NULL; lpn.code = PSN_SETACTIVE;
        SendMessage(phwnd, WM_NOTIFY, 0, (LPARAM)&lpn);
        HWND sethwnd = GetDlgItem(phwnd, IDC_SHORTCUTS_SET);
        EnableWindow(sethwnd, TRUE);
        SetFocus(sethwnd);
//        if (was_enabled) HookSystem();
    } break;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK KeyboardPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int have_to_apply = 0;
    static int edit_shortcut_idx = 0;
    // Hotkeys
    static const struct hk_struct hotkeys[] = {
        { IDC_LEFTALT,     VK_LMENU    },
        { IDC_RIGHTALT,    VK_RMENU    },
        { IDC_LEFTWINKEY,  VK_LWIN     },
        { IDC_RIGHTWINKEY, VK_RWIN     },
        { IDC_LEFTCTRL,    VK_LCONTROL },
        { IDC_RIGHTCTRL,   VK_RCONTROL },
        { 0, 0 }
    };
    struct actiondl kb_actions[] = {
        {L"Move",        l10n->input_actions_move},
        {L"Resize",      l10n->input_actions_resize},
        {L"Close",       l10n->input_actions_close},
        {L"Minimize",    l10n->input_actions_minimize},
        {L"Maximize",    l10n->input_actions_maximize},
        {L"Lower",       l10n->input_actions_lower},
        {L"Roll",        l10n->input_actions_roll},
        {L"AlwaysOnTop", l10n->input_actions_alwaysontop},
        {L"Borderless",  l10n->input_actions_borderless},
        {L"Center",      l10n->input_actions_center},
        {L"MaximizeHV",  l10n->input_actions_maximizehv},
        {L"MinAllOther", l10n->input_actions_minallother},
        {L"Menu",        l10n->input_actions_menu},
        {L"Nothing",     l10n->input_actions_nothing},
        {NULL, NULL}
    };
    struct actiondl kbshortcut_actions[] = {
        {L"Kill",        l10n->input_actions_kill},
        {L"Pause",       l10n->input_actions_pause},
        {L"Resume",      l10n->input_actions_resume},
        {L"ASOnOff",     l10n->input_actions_asonoff},
        {L"Lower",       l10n->input_actions_lower},
        {L"Roll",        l10n->input_actions_roll},
        {L"AlwaysOnTop", l10n->input_actions_alwaysontop},
        {L"Borderless",  l10n->input_actions_borderless},
        {L"Center",      l10n->input_actions_center},
        {L"Mute",        l10n->input_actions_mute},
        {L"Menu",        l10n->input_actions_menu},
        {L"NStacked",    l10n->input_actions_nstacked},
        {L"NStacked2",   l10n->input_actions_nstacked2},
        {L"PStacked",    l10n->input_actions_pstacked},
        {L"PStacked2",   l10n->input_actions_pstacked2},
        {L"StackList",   l10n->input_actions_stacklist},
        {L"StackList2",  l10n->input_actions_stacklist2},
        {L"MLZone",      l10n->input_actions_mlzone},
        {L"MTZone",      l10n->input_actions_mtzone},
        {L"MRZone",      l10n->input_actions_mrzone},
        {L"MBZone",      l10n->input_actions_mbzone},
        {L"XLZone",      l10n->input_actions_xlzone},
        {L"XTZone",      l10n->input_actions_xtzone},
        {L"XRZone",      l10n->input_actions_xrzone},
        {L"XBZone",      l10n->input_actions_xbzone},
        {NULL, NULL}
    };
    // Hotkeys
    const struct {
        wchar_t *action;
        wchar_t *l10n;
    } togglekeys[] = {
        { L"",      l10n->input_actions_nothing},
        { L"A4 A5", l10n->input_hotkeys_alt},
        { L"5B 5C", l10n->input_hotkeys_winkey},
        { L"A2 A3", l10n->input_hotkeys_ctrl},
        { L"A0 A1", l10n->input_hotkeys_shift},
        { L"A4",    l10n->input_hotkeys_leftalt},
        { L"A5",    l10n->input_hotkeys_rightalt},
        { L"5B",    l10n->input_hotkeys_leftwinkey},
        { L"5C",    l10n->input_hotkeys_rightwinkey},
        { L"A2",    l10n->input_hotkeys_leftctrl},
        { L"A3",    l10n->input_hotkeys_rightctrl},
        { L"A0",    l10n->input_hotkeys_leftshift},
        { L"A1",    l10n->input_hotkeys_rightshift},
    };
    static const struct optlst optlst[] = {
        { IDC_AGGRESSIVEPAUSE,  T_BOL, 0, L"Input", "AggressivePause", 0 },
        { IDC_AGGRESSIVEKILL,   T_BOL, 0, L"Input", "AggressiveKill", 0 },
        { IDC_SCROLLLOCKSTATE,  T_BMK, 0, L"Input", "ScrollLockState", 0},
        { IDC_UNIKEYHOLDMENU,   T_BOL, 0, L"Input", "UniKeyHoldMenu", 0},
        { IDC_NPSTACKED,        T_BOL, 0, L"Input", "NPStacked", 0},
        { IDC_KEYCOMBO,         T_BOL, 0, L"Input", "KeyCombo", 0 },
        { IDC_USEPTWINDOW,      T_BOL, 0, L"KBShortcuts", "UsePtWindow", 0},
    };

    if (msg == WM_INITDIALOG) {
        edit_shortcut_idx = 0;
        ReadDialogOptions(hwnd, optlst, ARR_SZ(optlst));
        // Agressive Pause
        CheckConfigHotKeys(hotkeys, hwnd, L"Hotkeys", L"A4 A5");

      # ifndef WIN64
        Button_Enable(GetDlgItem(hwnd, IDC_AGGRESSIVEPAUSE), HaveProc("NTDLL.DLL", "NtResumeProcess"));
        Button_Enable(GetDlgItem(hwnd, IDC_UNIKEYHOLDMENU), WIN2K);
      # endif

    } else if (msg == WM_COMMAND) {
        int event = HIWORD(wParam);
        int id = LOWORD(wParam);
        if (id == IDC_ALT ||  id == IDC_WINKEY || id == IDC_CONTROL || id == IDC_SHIFT) {
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 1);
        } else if (event == EN_UPDATE && id == IDC_SHORTCUTS_VK) {
            wchar_t txt[4];
            Edit_GetText(GetDlgItem(hwnd, IDC_SHORTCUTS_VK), txt, 3);
            BYTE vKey = whex2u(txt);
            wchar_t keyname[32]; keyname[0] = L'\0';
            GetKeyNameText(MapVirtualKey(vKey, 0)<<16, keyname, ARR_SZ(keyname)-8);
            SetDlgItemText(hwnd, IDC_SHORTCUTS, keyname);

            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 1);
        } else if ((event == 0 || event == EN_UPDATE || event == CBN_SELCHANGE)
            && (IDC_SHORTCUTS > id || id > IDC_SHORTCUTS_CLEAR)) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }

        // KEYBOARD SHORTCUTS HANDLING
        // READ Keyboard Shortcut
        if (id == IDC_SHORTCUTS_AC && event == CBN_SELCHANGE) {
            // Update the shortcut with the current one.
            int i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SHORTCUTS_AC));
            edit_shortcut_idx = i;
            WORD shortcut= GetPrivateProfileInt(L"KBShortcuts", kbshortcut_actions[i].action, 0, inipath);
            BYTE vKey = LOBYTE(shortcut);
            BYTE mod = HIBYTE(shortcut);
            wchar_t txt[8];
            SetDlgItemText(hwnd, IDC_SHORTCUTS_VK, _itow(vKey, txt, 16));

            Button_SetCheck(GetDlgItem(hwnd, IDC_CONTROL),(mod&MOD_CONTROL)? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_SHIFT),  (mod&MOD_SHIFT)? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_WINKEY), (mod&MOD_WIN)? BST_CHECKED: BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_ALT),    (mod&MOD_ALT)? BST_CHECKED: BST_UNCHECKED);
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 0);
        }
        // WRITE Current Keyboard Shortcut
        if (id == IDC_SHORTCUTS_SET) {
            wchar_t txt[8];
            // MOD_ALT=1, MOD_CONTROL=2, MOD_SHIFT=4, MOD_WIN=8
            Edit_GetText(GetDlgItem(hwnd, IDC_SHORTCUTS_VK), txt, 3);
            BYTE vKey = whex2u(txt);
            BYTE mod =  IsChecked(IDC_ALT)
                     | (IsChecked(IDC_CONTROL)<<1)
                     | (IsChecked(IDC_SHIFT)<<2)
                     | (IsChecked(IDC_WINKEY)<<3);
            WORD shortcut = vKey | (mod << 8);
            int i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SHORTCUTS_AC));
            if (kbshortcut_actions[i].action && kbshortcut_actions[i].action[0] != '\0')
                WritePrivateProfileString(L"KBShortcuts", kbshortcut_actions[i].action, _itow(shortcut, txt, 10), inipath);
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 0);
            SetFocus(GetDlgItem(hwnd, IDC_SHORTCUTS_AC));
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
            UpdateSettings();
        }
        if (id == IDC_SHORTCUTS_PICK) {
            HWND pickhwnd;
            WNDCLASSEX wnd;
            if (!GetClassInfoEx(g_hinst, APP_NAME"-PickShortcut", &wnd)) {
                WNDCLASSEX wndd = {
                    sizeof(WNDCLASSEX)
                  , CS_PARENTDC
                  , PickShortcutWinProc
                  , 0, 0, g_hinst
                  , NULL //LoadIconA(g_hinst, iconstr[1])
                  , NULL //LoadCursor(NULL, IDC_ARROW)
                  , NULL //(HBRUSH)(COLOR_HIGHLIGHT+1)
                  , NULL, APP_NAME"-PickShortcut", NULL
                };
                RegisterClassEx(&wndd);
            }
            RECT rc;
            GetClientRect(hwnd, &rc);
            pickhwnd = CreateWindowEx(WS_EX_TOPMOST
                 , APP_NAME"-PickShortcut", NULL
                 , WS_CHILD|WS_VISIBLE
                 , rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top
                 , hwnd , NULL, g_hinst, NULL);
            // Disable all other child windows
            EnumChildWindows(hwnd, EnableAllControlsProc, 0);
            EnableWindow(pickhwnd, TRUE);
            SetFocus(pickhwnd);
            SetActiveWindow(pickhwnd);
            // SetDlgItemText(hwnd, IDC_SHORTCUTS, L"Press Keys...");
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_PICK), 0);
        }
        if (id == IDC_SHORTCUTS_CLEAR) {
            // We must clear the current shotrcut
            Button_SetCheck(GetDlgItem(hwnd, IDC_SHIFT), BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_CONTROL), BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_WINKEY),BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_ALT),   BST_UNCHECKED);
            SetDlgItemText(hwnd, IDC_SHORTCUTS_VK, L"");
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 1);
        }

    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // GrabWithAlt
            wchar_t txt[64];

            GetPrivateProfileString(L"Input", L"ModKey", L"", txt, ARR_SZ(txt), inipath);
            Static_Enable(GetDlgItem(hwnd, IDC_GRABWITHALTB_H), txt[0]);
            ListBox_Enable(GetDlgItem(hwnd, IDC_GRABWITHALTB), txt[0]);

            FillActionDropListS(hwnd, IDC_GRABWITHALT, L"GrabWithAlt", kb_actions);
            FillActionDropListS(hwnd, IDC_GRABWITHALTB, L"GrabWithAltB", kb_actions);
            FillActionDropListS(hwnd, IDC_SHORTCUTS_AC, NULL, kbshortcut_actions);
            ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SHORTCUTS_AC), edit_shortcut_idx);
            Button_Enable(GetDlgItem(hwnd, IDC_SHORTCUTS_SET), 0);
            if(pnmh->hwndFrom != NULL)PostMessage(hwnd, WM_COMMAND, IDC_SHORTCUTS_AC|CBN_SELCHANGE<<16, 0);

            // ModKey init
            HWND control = GetDlgItem(hwnd, IDC_MODKEY);
            ComboBox_ResetContent(control);
            unsigned j, sel = 0;
            for (j = 0; j < ARR_SZ(togglekeys); j++) {
                wchar_t key_name[256];
                wcscpy_noaccel(key_name, togglekeys[j].l10n, ARR_SZ(key_name));
                ComboBox_AddString(control, key_name);
                if (!wcscmp(txt, togglekeys[j].action)) {
                    sel = j;
                }
            }
            // Add the current ModKey string to the list if not found!
            if (sel == 0 && txt[0]) {
                ComboBox_AddString(control, &txt[0]);
                sel = ARR_SZ(togglekeys);
            }
            ComboBox_SetCurSel(control, sel); // select current ModKey

            // Update text
            const struct dialogstring strlst[] = {
                { IDC_KEYBOARD_BOX,    l10n->tab_keyboard},
                { IDC_SCROLLLOCKSTATE, l10n->input_scrolllockstate},
                { IDC_UNIKEYHOLDMENU,  l10n->input_unikeyholdmenu},
                { IDC_HOTKEYS_BOX,     l10n->input_hotkeys_box},
                { IDC_MODKEY_H,        l10n->input_hotkeys_modkey},

                { IDC_ALT,             l10n->input_hotkeys_alt},
                { IDC_SHIFT,           l10n->input_hotkeys_shift},
                { IDC_CONTROL,         l10n->input_hotkeys_ctrl},
                { IDC_WINKEY,          l10n->input_hotkeys_winkey},
                { IDC_SHORTCUTS_H,     l10n->input_hotkeys_shortcuts},
                { IDC_SHORTCUTS_PICK,  l10n->input_hotkeys_shortcutspick},
                { IDC_SHORTCUTS_CLEAR, l10n->input_hotkeys_shortcutsclear},
                { IDC_SHORTCUTS_SET,   l10n->input_hotkeys_shortcutset},
                { IDC_USEPTWINDOW,     l10n->input_hotkeys_useptwindow},
                { IDC_LEFTALT,         l10n->input_hotkeys_leftalt},
                { IDC_RIGHTALT,        l10n->input_hotkeys_rightalt},
                { IDC_LEFTWINKEY,      l10n->input_hotkeys_leftwinkey},
                { IDC_RIGHTWINKEY,     l10n->input_hotkeys_rightwinkey},
                { IDC_LEFTCTRL,        l10n->input_hotkeys_leftctrl},
                { IDC_RIGHTCTRL,       l10n->input_hotkeys_rightctrl},
                { IDC_HOTKEYS_MORE,    l10n->input_hotkeys_more},
                { IDC_KEYCOMBO,        l10n->input_keycombo},
                { IDC_GRABWITHALT_H,   l10n->input_grabwithalt},
                { IDC_GRABWITHALTB_H,  l10n->input_grabwithaltb}
            };
            UpdateDialogStrings(hwnd, strlst, ARR_SZ(strlst));

        } else if (pnmh->code == PSN_APPLY && have_to_apply ) {
            int i;
            // Action without click
            WriteActionDropListS(hwnd, IDC_GRABWITHALT, L"GrabWithAlt", kb_actions);
            WriteActionDropListS(hwnd, IDC_GRABWITHALTB, L"GrabWithAltB", kb_actions);

            WriteDialogOptions(hwnd, optlst, ARR_SZ(optlst));
            ScrollLockState = WriteOptionBoolB(IDC_SCROLLLOCKSTATE, L"Input", "ScrollLockState", 0);
            // Modifier key
            i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_MODKEY));
            if(i < (int)ARR_SZ(togglekeys))
                WritePrivateProfileString(L"Input", L"ModKey", togglekeys[i].action, inipath);
            // Hotkeys
            SaveHotKeys(hotkeys, hwnd, L"Hotkeys");
            WriteOptionBool(IDC_KEYCOMBO,  L"Input", "KeyCombo");
            UpdateSettings();
            have_to_apply = 0;
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK BlacklistPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    #pragma GCC diagnostic ignored "-Wint-conversion"
    static const struct optlst optlst[] = {
        { IDC_PROCESSBLACKLIST, T_STR, 0, L"Blacklist", "Processes", L"" },
        { IDC_BLACKLIST,        T_STR, 0, L"Blacklist", "Windows", L"" },
        { IDC_SCROLLLIST,       T_STR, 0, L"Blacklist", "Scroll", L"" },
        { IDC_MDIS,             T_STR, 0, L"Blacklist", "MDIs", L"" },
        { IDC_PAUSEBL,          T_STR, 0, L"Blacklist", "Pause", L"" },
    };
    #pragma GCC diagnostic pop

    static int have_to_apply = 0;

    if (msg == WM_INITDIALOG) {
        ReadDialogOptions(hwnd, optlst, ARR_SZ(optlst));
        BOOL haveProcessBL = HaveProc("PSAPI.DLL", "GetModuleFileNameExW");
        Button_Enable(GetDlgItem(hwnd, IDC_PROCESSBLACKLIST), haveProcessBL);
        Button_Enable(GetDlgItem(hwnd, IDC_PAUSEBL), haveProcessBL);
    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);
        if (event == EN_UPDATE
        && id != IDC_NEWRULE && id != IDC_NEWPROGNAME
        && id != IDC_DWMCAPBUTTON && id != IDC_GWLEXSTYLE
        && id != IDC_NCHITTEST && id != IDC_GWLSTYLE && id != IDC_RECT) {
            PropSheet_Changed(g_cfgwnd, hwnd); // Enable the Apply Button
            have_to_apply = 1;
        } else if (event == STN_CLICKED && id == IDC_FINDWINDOW) {
            // Get size of workspace
            int left=0, top=0, width, height;
            if(GetSystemMetrics(SM_CMONITORS) >= 1) {
                left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
                top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
                width  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
             } else { // NT4...
                 width = GetSystemMetrics(SM_CXFULLSCREEN)+32;
                 height= GetSystemMetrics(SM_CYFULLSCREEN)+256;
             }

            // Create Transparent window covering the whole workspace
            WNDCLASSEX wnd = { sizeof(WNDCLASSEX), 0, FindWindowProc, 0, 0, g_hinst, NULL, NULL
                             , (HBRUSH) (COLOR_WINDOW + 1), NULL, APP_NAME"-find", NULL };
            wnd.hCursor = LoadCursor(g_hinst, MAKEINTRESOURCE(IDI_FIND));
            RegisterClassEx(&wnd);
            HWND findhwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT
                           , wnd.lpszClassName, NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hinst, NULL);
            ShowWindow(findhwnd, SW_SHOWNA); // And show it
            MoveWindow(findhwnd, left, top, width, height, FALSE);

            // Hide icon
            ShowWindowAsync(GetDlgItem(hwnd, IDC_FINDWINDOW), SW_HIDE);
        }

    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // Update text
            const struct dialogstring strlst[] = {
                { IDC_BLACKLIST_BOX          , l10n->blacklist_box },
                { IDC_PROCESSBLACKLIST_HEADER, l10n->blacklist_processblacklist },
                { IDC_BLACKLIST_HEADER       , l10n->blacklist_blacklist },
                { IDC_SCROLLLIST_HEADER      , l10n->blacklist_scrolllist },
                { IDC_MDISBL_HEADER          , l10n->blacklist_mdis },
                { IDC_PAUSEBL_HEADER         , l10n->blacklist_pause },
                { IDC_FINDWINDOW_BOX         , l10n->blacklist_findwindow_box }
            };
            UpdateDialogStrings(hwnd, strlst, ARR_SZ(strlst));
            // Enable or disable buttons if needed
            Button_Enable(GetDlgItem(hwnd, IDC_MDIS), GetPrivateProfileInt(L"General", L"MDI", 1, inipath));
            Button_Enable(GetDlgItem(hwnd, IDC_PAUSEBL)
                       ,  GetPrivateProfileInt(L"Input", L"AggressivePause", 0, inipath)
                       || GetPrivateProfileInt(L"Input", L"AggressiveKill", 0, inipath));
                        // Grayout useless
        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // Save to the config
            WriteDialogOptions(hwnd, optlst, ARR_SZ(optlst));
            UpdateSettings();
            have_to_apply = 0;
        }
    }

    return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK FindWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN) {
        ShowWindow(hwnd, SW_HIDE);
        HWND page = PropSheet_GetCurrentPageHwnd(g_cfgwnd);

        if (msg == WM_LBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            HWND nwindow = WindowFromPoint(pt);
            HWND window = GetAncestor(nwindow, GA_ROOT);

            wchar_t title[256], classname[256];
            GetWindowText(window, title, ARR_SZ(title));
            GetClassName(window, classname, ARR_SZ(classname));

            wchar_t txt[512];
            txt[0] = '\0';
            wcscat(txt, title); wcscat(txt, L"|"); wcscat(txt, classname);
            SetDlgItemText(page, IDC_NEWRULE, txt);

            if (GetWindowProgName(window, txt, ARR_SZ(txt))) {
                SetDlgItemText(page, IDC_NEWPROGNAME, txt);
            }
            SetDlgItemText(page, IDC_GWLSTYLE, _itow(GetWindowLongPtr(window, GWL_STYLE), txt, 16));
            SetDlgItemText(page, IDC_GWLEXSTYLE, _itow(GetWindowLongPtr(window, GWL_EXSTYLE), txt, 16));
            // WM_NCHITTEST messages info at current pt
            _itow(HitTestTimeout(nwindow, pt.x, pt.y), txt, 10);
            wchar_t tt[8];
            _itow(HitTestTimeout(window, pt.x, pt.y), tt, 10);
            wcscat(txt, L"/");wcscat(txt, tt);
            SetDlgItemText(page, IDC_NCHITTEST, txt);
            // IDC_DWMCAPBUTTON
            RECT rc;
            SetDlgItemText(page, IDC_DWMCAPBUTTON
                , (GetCaptionButtonsRect(window, &rc) && PtInRect(&rc, pt))?L"Yes":L"No");

            // Window rectangle info
            if (GetWindowRectL(window, &rc)) {
                SetDlgItemText(page, IDC_RECT, RectToStr(&rc, txt));
            }
            // IDC_WINHANDLES
            wcscpy(txt, L"Hwnd: "); wcscat(txt, _itow((DWORD)(DorQWORD)nwindow, tt,16));
            wcscat(txt, L", Root: "); wcscat(txt, _itow((DWORD)(DorQWORD)window, tt,16));
            // wcscat(txt, L", Parent: "); wcscat(txt, _itow((DWORD)(DorQWORD)GetParent(window), tt,16));
            wcscat(txt, L", Owner: "); wcscat(txt, _itow((DWORD)(DorQWORD)GetWindow(window, GW_OWNER), tt,16));
            SetDlgItemText(page, IDC_WINHANDLES, txt);
        }
        // Show icon again
        ShowWindowAsync(GetDlgItem(page, IDC_FINDWINDOW), SW_SHOW);

        DestroyWindow(hwnd);
        UnregisterClass(APP_NAME"-find", g_hinst);
        return 0;
    } else if (wParam && msg ==  WM_ERASEBKGND) {
        return 1;
    } else if (wParam && msg == WM_PAINT) {
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK AboutPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG) {
        MoveToCorner(g_cfgwnd);
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // Update text
            SetDlgItemText(hwnd, IDC_ABOUT_BOX,        l10n->about_box);
            SetDlgItemText(hwnd, IDC_VERSION,          l10n->about_version);
            SetDlgItemText(hwnd, IDC_URL,              L"https://github.com/RamonUnch/AltSnap");
            SetDlgItemText(hwnd, IDC_AUTHOR,           l10n->about_author);
            SetDlgItemText(hwnd, IDC_LICENSE,          l10n->about_license);
            SetDlgItemText(hwnd, IDC_TRANSLATIONS_BOX, l10n->about_translation_credit);

            wchar_t txt[1024] = L"";
            int i;
            for (i = 0; i < nlanguages; i++) {
                wcscat(txt, langinfo[i].lang_english);
                wcscat(txt, L": ");
                wcscat(txt, langinfo[i].author);
                if (i + 1 != nlanguages) {
                    wcscat(txt, L"\r\n");
                }
            }
            SetDlgItemText(hwnd, IDC_TRANSLATIONS, txt);
        }
    }

    return FALSE;
}
static HWND NewTestWindow();
/////////////////////////////////////////////////////////////////////////////
// Simple windows proc that draws the resizing regions.
LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static UCHAR centerfrac=24;
    static UCHAR centermode=1;
    static wchar_t lastkey[64]=L"";

    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == 0x4E && (GetAsyncKeyState(VK_CONTROL)&0x8000)) {
            NewTestWindow();
            break;
        }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN: {
        wchar_t txt[32];
        wcscpy(lastkey, L"vKey = ");
        wcscat(lastkey, _itow(wParam, txt, 16));
        wcscat(lastkey, L", sCode = ");
        wcscat(lastkey, _itow(HIWORD(lParam)&0x00FF, txt, 16));
        wcscat(lastkey, L", Data = " );
        wcscat(lastkey, _itow(lParam, txt, 16));
        txt[0] = L','; txt[1] = L' ';
        GetKeyNameText(lParam, txt+2, ARR_SZ(txt)-2);
        wcscat(lastkey, txt);
        RECT crc;
        GetClientRect(hwnd, &crc);
        UINT dpi = GetDpiForWindow(hwnd);
        long splitheight = crc.bottom-GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
        RECT trc =  { 5, splitheight, crc.right, crc.bottom };
        InvalidateRect(hwnd, &trc, TRUE);
    } break;

    case WM_PAINT: {
        if(!GetUpdateRect(hwnd, NULL, FALSE)) return 0;
        /* We must keep track of pens and delete them.*/
        UINT dpi = GetDpiForWindow(hwnd);
        const HPEN pen = (HPEN) CreatePen(PS_SOLID, GetSystemMetricsForDpi(SM_CXEDGE, dpi), GetSysColor(COLOR_BTNTEXT));
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT wRect;
        GetWindowRect(hwnd, &wRect);
        POINT Offset = { wRect.left, wRect.top };
        ScreenToClient(hwnd, &Offset);

        SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        SelectObject(hdc, pen);
        SetROP2(hdc, R2_COPYPEN);

        const int width = wRect.right - wRect.left;
        const int height = wRect.bottom - wRect.top;
        const int cwidth = width*centerfrac/100;
        const int cheight = height*centerfrac/100;

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNFACE+1));
        Rectangle(hdc
            , Offset.x + (width-cwidth)/2
            , Offset.y
            , Offset.x + (width+cwidth)/2
            , Offset.y + height);
        Rectangle(hdc
            , Offset.x
            , Offset.y + (height-cheight)/2
            , Offset.x + width
            , Offset.y + (height+cheight)/2);
        if (centermode == 3) {
            HPEN bgpen = (HPEN) CreatePen(PS_SOLID, 2, GetSysColor(COLOR_BTNFACE));
            HPEN prevpen = SelectObject(hdc, bgpen);
            Rectangle(hdc
                , Offset.x + (width-cwidth)/2
                , Offset.y + (height-cheight)/2
                , Offset.x + (width+cwidth)/2
                , Offset.y + (height+cheight)/2);
            DeleteObject(SelectObject(hdc, prevpen)); // delete bgpen.
            // Draw diagonal lines
            POINT pts[4] ={ { (width-cwidth)/2, (height-cheight)/2 },
                            { (width+cwidth)/2, (height+cheight)/2 },
                            { (width-cwidth)/2, (height+cheight)/2 },
                            { (width+cwidth)/2, (height-cheight)/2 },
                          };
            OffsetPoints(pts, Offset.x, Offset.y, 4);
            Polyline(hdc, pts  , 2);
            Polyline(hdc, pts+2, 2);

            HPEN dotpen = (HPEN) CreatePen(PS_DOT, 1, GetSysColor(COLOR_BTNTEXT));
            prevpen = SelectObject(hdc, dotpen);
            SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
            Rectangle(hdc
                , Offset.x+(width-cwidth)/2
                , Offset.y+(height-cheight)/2
                , (width+cwidth)/2 + Offset.x
                , (height+cheight)/2 + Offset.y);
            // restore oldpen and delete dotpen
            DeleteObject(SelectObject(hdc, prevpen));
        }

        // Draw textual info....
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
        RECT crc;
        GetClientRect(hwnd, &crc);
        long splitheight = crc.bottom-GetSystemMetricsForDpi(SM_CYCAPTION, dpi);
        RECT trc = {5, splitheight, crc.right, crc.bottom};
        DrawText(hdc, lastkey, wcslen(lastkey), &trc, DT_NOCLIP|DT_TABSTOP);
        wchar_t *str = l10n->zone_testwinhelp;
        if (UseZones&1) {
            RECT trc2 = { 5, 5, crc.right, splitheight };
            DrawText(hdc, str, wcslen(str), &trc2, DT_NOCLIP|DT_TABSTOP);
        }

        EndPaint(hwnd, &ps);

        DeleteObject(pen); // delete pen

        return 0;
    } break;

    case WM_ERASEBKGND:
//        Sleep(200); break;
        return 1;

    case WM_UPDCFRACTION:
        centerfrac = GetPrivateProfileInt(L"General", L"CenterFraction", 24, inipath);
        centermode = GetPrivateProfileInt(L"General", L"ResizeCenter", 1, inipath);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
static HWND NewTestWindow()
{
    HWND testwnd;
    WNDCLASSEX wnd;
    if (!GetClassInfoEx(g_hinst, APP_NAME"-Test", &wnd)) {
        WNDCLASSEX wndd = {
            sizeof(WNDCLASSEX)
          , CS_HREDRAW|CS_VREDRAW|CS_BYTEALIGNCLIENT
          , TestWindowProc
          , 0, 0, g_hinst, LoadIconA(g_hinst, iconstr[1])
          , LoadCursor(NULL, IDC_ARROW)
          , NULL //(HBRUSH)(COLOR_BACKGROUND+1)
          , NULL, APP_NAME"-Test", NULL
        };
        RegisterClassEx(&wndd);
    }
    wchar_t wintitle[256];
    wcscpy_noaccel(wintitle, l10n->advanced_testwindow, ARR_SZ(wintitle));
    testwnd = CreateWindowEx(0
         , APP_NAME"-Test", wintitle
         , WS_CAPTION|WS_OVERLAPPEDWINDOW
         , CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT
         , NULL, NULL, g_hinst, NULL);
    PostMessage(testwnd, WM_UPDCFRACTION, 0, 0);
    ShowWindow(testwnd, SW_SHOW);

    return testwnd;
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK AdvancedPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    #pragma GCC diagnostic ignored "-Wint-conversion"
    static const struct optlst optlst[] = {
        { IDC_AUTOREMAXIMIZE,   T_BOL, 0, L"Advanced", "AutoRemaximize", 0 },
        { IDC_AEROTOPMAXIMIZES, T_BMK, 0, L"Advanced", "AeroTopMaximizes", 1 },// bit 0
        { IDC_AERODBCLICKSHIFT, T_BMK, 1, L"Advanced", "AeroTopMaximizes", 1 },// bit 1
        { IDC_MULTIPLEINSTANCES,T_BOL, 0, L"Advanced", "MultipleInstances",0 },
        { IDC_FULLSCREEN,       T_BOL, 0, L"Advanced", "FullScreen", 1 },
        { IDC_BLMAXIMIZED,      T_BOL, 0, L"Advanced", "BLMaximized", 0 },
        { IDC_FANCYZONE,        T_BOL, 0, L"Zones",    "FancyZone", 0 },
        { IDC_NORESTORE,        T_BMK, 2, L"General",  "SmartAero", 0 },  // bit 2
        { IDC_MAXWITHLCLICK,    T_BMK, 0, L"General",  "MMMaximize", 1 }, // bit 0
        { IDC_RESTOREONCLICK,   T_BMK, 1, L"General",  "MMMaximize", 0 }, // bit 1
        { IDC_TOPMOSTINDICATOR, T_BOL, 0, L"Advanced", "TopmostIndicator", 0},

        { IDC_CENTERFRACTION,   T_STR, 0, L"General",  "CenterFraction",L"24" },
        { IDC_AEROHOFFSET,      T_STR, 0, L"General",  "AeroHoffset",   L"50" },
        { IDC_AEROVOFFSET,      T_STR, 0, L"General",  "AeroVoffset",   L"50" },
        { IDC_SNAPTHRESHOLD,    T_STR, 0, L"Advanced", "SnapThreshold", L"20" },
        { IDC_AEROTHRESHOLD,    T_STR, 0, L"Advanced", "AeroThreshold", L"5"  },
        { IDC_SNAPGAP,          T_STR, 0, L"Advanced", "SnapGap",       L"0"  },
        { IDC_AEROSPEED,        T_STR, 0, L"Advanced", "AeroMaxSpeed",  L""   },
        { IDC_AEROSPEEDTAU,     T_STR, 0, L"Advanced", "AeroSpeedTau",  L"32" },
        { IDC_MOVETRANS,        T_STR, 0, L"General",  "MoveTrans",     L""   },
    };
    #pragma GCC diagnostic pop

//    static HWND testwnd=NULL;
    static int have_to_apply = 0;
    if (msg == WM_INITDIALOG) {
      ReadDialogOptions(hwnd, optlst, ARR_SZ(optlst));
      # ifndef WIN64
        Button_Enable(GetDlgItem(hwnd, IDC_FANCYZONE), 0);
      # endif

    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);

        if (id != IDC_TESTWINDOW && (event == 0 || event == EN_UPDATE)) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
        if (id == IDC_TESTWINDOW) { // Click on the Test Window button
            NewTestWindow();
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // Update text
            const struct dialogstring strlst[] = {
                { IDC_METRICS_BOX,      l10n->advanced_metrics_box },
                { IDC_CENTERFRACTION_H, l10n->advanced_centerfraction },
                { IDC_AEROHOFFSET_H,    l10n->advanced_aerohoffset },
                { IDC_AEROVOFFSET_H,    l10n->advanced_aerovoffset },
                { IDC_SNAPTHRESHOLD_H,  l10n->advanced_snapthreshold },
                { IDC_AEROTHRESHOLD_H,  l10n->advanced_aerothreshold },
                { IDC_SNAPGAP_H,        l10n->advanced_snapgap },
                { IDC_AEROSPEED_H,      l10n->advanced_aerospeed },
                { IDC_MOVETRANS_H,      l10n->advanced_movetrans },
                { IDC_TESTWINDOW,       l10n->advanced_testwindow },

                { IDC_BEHAVIOR_BOX,     l10n->advanced_behavior_box },
                { IDC_MULTIPLEINSTANCES,l10n->advanced_multipleinstances },
                { IDC_AUTOREMAXIMIZE,   l10n->advanced_autoremaximize },
                { IDC_AEROTOPMAXIMIZES, l10n->advanced_aerotopmaximizes },
                { IDC_AERODBCLICKSHIFT, l10n->advanced_aerodbclickshift },
                { IDC_MAXWITHLCLICK,    l10n->advanced_maxwithlclick },
                { IDC_RESTOREONCLICK,   l10n->advanced_restoreonclick },
                { IDC_FULLSCREEN,       l10n->advanced_fullscreen },
                { IDC_BLMAXIMIZED,      l10n->advanced_blmaximized },
                { IDC_FANCYZONE,        l10n->advanced_fancyzone },
                { IDC_NORESTORE,        l10n->advanced_norestore },
                { IDC_TOPMOSTINDICATOR, l10n->advanced_topmostindicator },
            };
            UpdateDialogStrings(hwnd, strlst, ARR_SZ(strlst));

        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // Apply or OK button was pressed.
            // Save settings
            WriteDialogOptions(hwnd, optlst, ARR_SZ(optlst));
            UpdateSettings();
            // Update Test windows in if open.
            EnumThreadWindows(GetCurrentThreadId(), RefreshTestWin, 0);
            have_to_apply = 0;
        }
    }
    return FALSE;
}
