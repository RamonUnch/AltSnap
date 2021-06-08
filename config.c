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
static void CheckAutostart(int *on, int *hidden, int *elevated)
{
    *on = *hidden = *elevated = 0;
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
    *on = 1;
    if (wcsstr(value, L" -hide") != NULL) {
        *hidden = 1;
    }
    if (wcsstr(value, L" -elevate") != NULL) {
        *elevated = 1;
    }
}

/////////////////////////////////////////////////////////////////////////////
static void SetAutostart(int on, int hide, int elevate)
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
        if(hide)    wcscat(value, L" -hide");
        if(elevate) wcscat(value, L" -elevate");
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
BOOL ElevateNow(int showconfig)
{
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, ARR_SZ(path));
        INT_PTR ret;
        if(showconfig)
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
void OpenConfig(int startpage)
{
    if (IsWindow(g_cfgwnd)) {
        PropSheet_SetCurSel(g_cfgwnd, 0, startpage);
        SetForegroundWindow(g_cfgwnd);
        return;
    }
    // Define the pages
    struct {
        int pszTemplate;
        DLGPROC pfnDlgProc;
    } pages[] = {
        { IDD_GENERALPAGE,   GeneralPageDialogProc  },
        { IDD_MOUSEPAGE,     MousePageDialogProc    },
        { IDD_KBPAGE,        KeyboardPageDialogProc },
        { IDD_BLACKLISTPAGE, BlacklistPageDialogProc},
        { IDD_ADVANCEDPAGE,  AdvancedPageDialogProc },
        { IDD_ABOUTPAGE,     AboutPageDialogProc    }
    };
    PROPSHEETPAGE psp[ARR_SZ(pages)] = { };
    size_t i;
    for (i = 0; i < ARR_SZ(pages); i++) {
        psp[i].dwSize = sizeof(PROPSHEETPAGE);
        psp[i].hInstance = g_hinst;
        psp[i].pszTemplate = MAKEINTRESOURCE(pages[i].pszTemplate);
        psp[i].pfnDlgProc = pages[i].pfnDlgProc;
    }

    // Define the property sheet
    PROPSHEETHEADER psh = { sizeof(PROPSHEETHEADER) };
    psh.dwFlags = PSH_PROPSHEETPAGE|PSH_USECALLBACK|PSH_USEHICON ;
    psh.hwndParent = NULL;
    psh.hInstance = g_hinst;
    psh.hIcon = icon[1];
    psh.pszCaption = APP_NAME;
    psh.nPages = ARR_SZ(pages);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = PropSheetProc;
    psh.nStartPage = startpage;

    // Open the property sheet
    PropertySheet(&psh);
}
/////////////////////////////////////////////////////////////////////////////
void CloseConfig()
{
    PostMessage(g_cfgwnd, WM_CLOSE, 0, 0);
}
void UpdateSettings()
{
    PostMessage(g_hwnd, WM_UPDATESETTINGS, 1, 0);
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
    HWND tc = PropSheet_GetTabControl(g_cfgwnd);
    int numrows_prev = TabCtrl_GetRowCount(tc);
    wchar_t *titles[] = { l10n->tab_general, l10n->tab_mouse, l10n->tab_keyboard
                        , l10n->tab_blacklist, l10n->tab_advanced,l10n->tab_about };
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
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK PropSheetWinProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LRESULT ret = DefSubclassProc(hwnd, msg, wParam, lParam);
    if (msg == WM_NCHITTEST && (ret == HTBOTTOM || ret == HTBOTTOMLEFT
       || ret == HTBOTTOMRIGHT || ret == HTLEFT || ret == HTTOPLEFT
       || ret == HTTOPRIGHT || ret == HTRIGHT || ret == HTTOP)) {

        ret = HTCAPTION;

    } else if (msg == WM_UPDATESETTINGS) {
        UpdateStrings();
        HWND page = PropSheet_GetCurrentPageHwnd(g_cfgwnd);
        SendMessage(page, WM_INITDIALOG, 0, 0);
        NMHDR pnmh = { g_cfgwnd, 0, PSN_SETACTIVE };
        SendMessage(page, WM_NOTIFY, 0, (LPARAM) & pnmh);
    }
    return ret;
}
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK PropSheetProc(HWND hwnd, UINT msg, LPARAM lParam)
{
    if (msg == PSCB_INITIALIZED) {
        g_cfgwnd = hwnd;
        SetWindowSubclass(g_cfgwnd, PropSheetWinProc, 0, 0);
        UpdateStrings();

        // Set new icon specifically for the taskbar and Alt+Tab, without changing window icon
        HICON taskbar_icon = LoadImage(g_hinst, L"app_icon", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
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
#define IsCheckedW(idc) _itow(Button_GetCheck(GetDlgItem(hwnd, idc)), txt, ARR_SZ(txt))
#define IsChecked(idc) Button_GetCheck(GetDlgItem(hwnd, idc))
static void WriteOptionBoolW(HWND hwnd, WORD id, wchar_t *section, wchar_t *name, wchar_t *txt, size_t txtsz, wchar_t *inipath)
{
    WritePrivateProfileString(section, name,_itow(Button_GetCheck(GetDlgItem(hwnd, id)), txt, txtsz), inipath);
}
#define WriteOptionBool(id, section, name) WriteOptionBoolW(hwnd, id, section, name, txt, ARR_SZ(txt), inipath)
static void WriteOptionBoolBW(HWND hwnd, WORD id, wchar_t *section, wchar_t *name, wchar_t *txt, size_t txtsz, wchar_t *inipath, int bit)
{
    int val = GetPrivateProfileInt(section, name, 0, inipath);
    if(Button_GetCheck(GetDlgItem(hwnd, id)))
        val = setBit(val, bit);
    else
        val = clearBit(val, bit);

    WritePrivateProfileString(section, name,_itow(val, txt, txtsz), inipath);
}
#define WriteOptionBoolB(id, section, name, bit) WriteOptionBoolBW(hwnd, id, section, name, txt, ARR_SZ(txt), inipath, bit)
static void WriteOptionStrW(HWND hwnd, WORD id, wchar_t *section, wchar_t *name, wchar_t *txt, size_t txtsz, wchar_t *inipath)
{
    Edit_GetText(GetDlgItem(hwnd, id), txt, txtsz);
    WritePrivateProfileString(section, name, txt, inipath);
}

#define WriteOptionStr(id, section, name)  WriteOptionStrW(hwnd, id, section, name, txt, ARR_SZ(txt), inipath)

static void ReadOptionStrW(HWND hwnd, WORD id, wchar_t *section, wchar_t *name
                 , wchar_t *txt, size_t txtsz, wchar_t *def, wchar_t *inipath)
{
    GetPrivateProfileString(section, name, def, txt, txtsz, inipath);
    SetDlgItemText(hwnd, id, txt);
}
#define ReadOptionStr(id, section, name, def) ReadOptionStrW(hwnd, id, section, name, txt, ARR_SZ(txt), def, inipath)

static int ReadOptionIntW(HWND hwnd, WORD id, wchar_t *section, wchar_t *name, int def, wchar_t *inipath, int mask)
{
    int ret = GetPrivateProfileInt(section, name, def, inipath);
    Button_SetCheck(GetDlgItem(hwnd, id), (ret&mask)? BST_CHECKED: BST_UNCHECKED);
    return ret;
}
#define ReadOptionInt(id, section, name, def, mask) ReadOptionIntW(hwnd, id, section, name, def, inipath, mask)

/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK GeneralPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int updatestrings = 0;
    static int have_to_apply = 0;
    if (msg == WM_INITDIALOG) {
        int ret;
        ReadOptionInt(IDC_AUTOFOCUS,      L"General", L"AutoFocus", 0, -1);
        ReadOptionInt(IDC_AERO,           L"General", L"Aero", 1, -1);
        ReadOptionInt(IDC_SMARTAERO,      L"General",    L"SmartAero", 1, -1);
        ReadOptionInt(IDC_STICKYRESIZE,   L"General",    L"StickyResize", 1, 1);
        ReadOptionInt(IDC_INACTIVESCROLL, L"General", L"InactiveScroll", 0, -1);
        ReadOptionInt(IDC_MDI,            L"General", L"MDI", 1, -1);
        ReadOptionInt(IDC_FULLWIN,        L"Performance", L"FullWin", 1, -1);
        ReadOptionInt(IDC_RESIZEALL,      L"Advanced", L"ResizeAll", 1, -1);

        ret=GetPrivateProfileInt(L"General", L"ResizeCenter", 1, inipath);
        ret = ret==1? IDC_RZCENTER_NORM: ret==2? IDC_RZCENTER_MOVE: IDC_RZCENTER_BR;
        CheckRadioButton(hwnd, IDC_RZCENTER_NORM, IDC_RZCENTER_MOVE, ret);

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

        if (id != IDC_ELEVATE && (event == 0 ||  event == CBN_SELCHANGE)) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
        if (IDC_RZCENTER_NORM <= id && id <= IDC_RZCENTER_MOVE) {
            CheckRadioButton(hwnd, IDC_RZCENTER_NORM, IDC_RZCENTER_MOVE, id);
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
            int autostart = 0, hidden = 0, elevated = 0;
            CheckAutostart(&autostart, &hidden, &elevated);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART), autostart ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), hidden ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), elevated ? BST_CHECKED : BST_UNCHECKED);
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_HIDE), autostart);
            Button_Enable(GetDlgItem(hwnd, IDC_AUTOSTART_ELEVATE), autostart && VISTA);
            if(WIN10) Button_Enable(GetDlgItem(hwnd, IDC_INACTIVESCROLL), IsChecked(IDC_INACTIVESCROLL));
            if(HaveDWM()) Button_Enable(GetDlgItem(hwnd, IDC_FULLWIN), !IsChecked(IDC_FULLWIN));
        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            wchar_t txt[10];
            WriteOptionBool(IDC_AUTOFOCUS,     L"General",    L"AutoFocus");
            WriteOptionBool(IDC_AERO,          L"General",    L"Aero");
            WriteOptionBool(IDC_SMARTAERO,     L"General",    L"SmartAero");
            WriteOptionBoolB(IDC_STICKYRESIZE,  L"General",    L"StickyResize", 0);
            WriteOptionBool(IDC_INACTIVESCROLL,L"General",    L"InactiveScroll");
            WriteOptionBool(IDC_MDI,           L"General",    L"MDI");
            WriteOptionBool(IDC_FULLWIN,       L"Performance",L"FullWin");
            WriteOptionBool(IDC_RESIZEALL,     L"Advanced",   L"ResizeAll");

            int val = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_AUTOSNAP));
            WritePrivateProfileString(L"General",    L"AutoSnap", _itow(val, txt, 10), inipath);

            val = IsChecked(IDC_RZCENTER_NORM)? 1: IsChecked(IDC_RZCENTER_MOVE)? 2: 0;
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
            have_to_apply = 0;
        }
    }
    if (updatestrings) {
        // Update text
        SetDlgItemText(hwnd, IDC_GENERAL_BOX,       l10n->general_box);
        SetDlgItemText(hwnd, IDC_AUTOFOCUS,         l10n->general_autofocus);
        SetDlgItemText(hwnd, IDC_AERO,              l10n->general_aero);
        SetDlgItemText(hwnd, IDC_SMARTAERO,         l10n->general_smartaero);
        SetDlgItemText(hwnd, IDC_STICKYRESIZE,      l10n->general_stickyresize);
        SetDlgItemText(hwnd, IDC_INACTIVESCROLL,    l10n->general_inactivescroll);
        SetDlgItemText(hwnd, IDC_MDI,               l10n->general_mdi);
        SetDlgItemText(hwnd, IDC_AUTOSNAP_HEADER,   l10n->general_autosnap);
        SetDlgItemText(hwnd, IDC_LANGUAGE_HEADER,   l10n->general_language);
        SetDlgItemText(hwnd, IDC_FULLWIN,           l10n->general_fullwin);
        SetDlgItemText(hwnd, IDC_RESIZEALL,         l10n->general_resizeall);
        SetDlgItemText(hwnd, IDC_RESIZECENTER,      l10n->general_resizecenter);
        SetDlgItemText(hwnd, IDC_RZCENTER_NORM,     l10n->general_resizecenter_norm);
        SetDlgItemText(hwnd, IDC_RZCENTER_BR,       l10n->general_resizecenter_br);
        SetDlgItemText(hwnd, IDC_RZCENTER_MOVE,     l10n->general_resizecenter_move);
        SetDlgItemText(hwnd, IDC_AUTOSTART_BOX,     l10n->general_autostart_box);
        SetDlgItemText(hwnd, IDC_AUTOSTART,         l10n->general_autostart);
        SetDlgItemText(hwnd, IDC_AUTOSTART_HIDE,    l10n->general_autostart_hide);
        SetDlgItemText(hwnd, IDC_AUTOSTART_ELEVATE, l10n->general_autostart_elevate);
        SetDlgItemText(hwnd, IDC_ELEVATE, (elevated? l10n->general_elevated: l10n->general_elevate));

        // AutoSnap
        HWND control = GetDlgItem(hwnd, IDC_AUTOSNAP);
        ComboBox_ResetContent(control);
        ComboBox_AddString(control, l10n->general_autosnap0);
        ComboBox_AddString(control, l10n->general_autosnap1);
        ComboBox_AddString(control, l10n->general_autosnap2);
        ComboBox_AddString(control, l10n->general_autosnap3);
        wchar_t txt[10];
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
        while(pos[numread] && pos[numread] != ' ') numread++;
        while(pos[numread] == ' ') numread++;
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
    if(IsKeyInList(keys, vkey))
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
    while(keys[--ll] == ' ') keys[ll]='\0';
}
struct hk_struct {
    unsigned control;
    unsigned vkey;
};
/////////////////////////////////////////////////////////////////////////////
static void CheckConfigHotKeys(struct hk_struct *hotkeys, HWND hwnd, wchar_t *hotkeystr, wchar_t* def)
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
/////////////////////////////////////////////////////////////////////////////
static void SaveHotKeys(struct hk_struct *hotkeys, HWND hwnd, wchar_t *name, wchar_t *inipath)
{
    wchar_t keys[32];
    // Get the current config in case there are some user added keys.
    GetPrivateProfileString(L"Input", name, L"", keys, ARR_SZ(keys), inipath);
    unsigned i;
    for (i = 0; hotkeys[i].control; i++) {
         if(IsChecked(hotkeys[i].control)) {
             AddvKeytoList(keys, hotkeys[i].vkey);
         } else {
             RemoveKeyFromList(keys, hotkeys[i].vkey);
         }
    }
    WritePrivateProfileString(L"Input", name, keys, inipath);
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK MousePageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int have_to_apply = 0;
    // Mouse actions
    struct {
        int control;
        wchar_t *option;
    } mouse_buttons[] = {
        { IDC_LMB, L"LMB" },
        { IDC_MMB, L"MMB" },
        { IDC_RMB, L"RMB" },
        { IDC_MB4, L"MB4" },
        { IDC_MB5, L"MB5" },
    };

    struct action {
        wchar_t *action;
        wchar_t *l10n;
    };
    struct action mouse_actions[] = {
        {L"Move",        l10n->input_actions_move},
        {L"Resize",      l10n->input_actions_resize},
        {L"Close",       l10n->input_actions_close},
        {L"Kill",        l10n->input_actions_kill},
        {L"Minimize",    l10n->input_actions_minimize},
        {L"Maximize",    l10n->input_actions_maximize},
        {L"Lower",       l10n->input_actions_lower},
        {L"Roll",        l10n->input_actions_roll},
        {L"AlwaysOnTop", l10n->input_actions_alwaysontop},
        {L"Borderless",  l10n->input_actions_borderless},
        {L"Center",      l10n->input_actions_center},
        {L"Menu",        l10n->input_actions_menu},
        {L"Nothing",     l10n->input_actions_nothing},
    };

    // Scroll
    struct action scroll_actions[] = {
        {L"AltTab",       l10n->input_actions_alttab},
        {L"Volume",       l10n->input_actions_volume},
        {L"Transparency", l10n->input_actions_transparency},
        {L"Lower",        l10n->input_actions_lower},
        {L"Roll",         l10n->input_actions_roll},
        {L"Maximize",     l10n->input_actions_maximize},
        {L"Nothing",      l10n->input_actions_nothing},
    };

    // Hotkeys
    struct hk_struct hotclicks [] = {
        { IDC_MMB_HC, 0x04 },
        { IDC_MB4_HC, 0x05 },
        { IDC_MB5_HC, 0X06 },
        { 0, 0 }
    };

    if (msg == WM_INITDIALOG) {
        ReadOptionInt(IDC_LOWERWITHMMB, L"Input", L"LowerWithMMB", 0, 1);
        ReadOptionInt(IDC_ROLLWITHTBSCROLL, L"Input",  L"RollWithTBScroll", 0, -1);

        // Hotclicks buttons
        CheckConfigHotKeys(hotclicks, hwnd, L"Hotclicks", L"");
    } else if (msg == WM_COMMAND) {
        int event = HIWORD(wParam);
        if (event == 0 || event == CBN_SELCHANGE){
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            wchar_t txt[64];
            size_t i, j, sel;

            // Mouse actions
            for (i = 0; i < ARR_SZ(mouse_buttons); i++) {
                HWND control = GetDlgItem(hwnd, mouse_buttons[i].control);
                ComboBox_ResetContent(control);
                GetPrivateProfileString(L"Input", mouse_buttons[i].option, L"Nothing", txt, ARR_SZ(txt), inipath);
                sel = ARR_SZ(mouse_actions) - 1;
                for (j = 0; j < ARR_SZ(mouse_actions); j++) {
                    wchar_t action_name[256];
                    wcscpy_noaccel(action_name, mouse_actions[j].l10n, ARR_SZ(action_name));
                    ComboBox_AddString(control, action_name);
                    if (!wcscmp(txt, mouse_actions[j].action)) {
                        sel = j;
                    }
                }
                ComboBox_SetCurSel(control, sel);
            }

            // Scroll
            HWND control = GetDlgItem(hwnd, IDC_SCROLL);
            ComboBox_ResetContent(control);
            GetPrivateProfileString(L"Input", L"Scroll", L"Nothing", txt, ARR_SZ(txt), inipath);
            sel = ARR_SZ(scroll_actions) - 1;
            for (j = 0; j < ARR_SZ(scroll_actions); j++) {
                wchar_t action_name[256];
                wcscpy_noaccel(action_name, scroll_actions[j].l10n, ARR_SZ(action_name));
                ComboBox_AddString(control, action_name);
                if (!wcscmp(txt, scroll_actions[j].action)) {
                    sel = j;
                }
            }
            ComboBox_SetCurSel(control, sel);

            // Update text
            SetDlgItemText(hwnd, IDC_MOUSE_BOX,       l10n->input_mouse_box);
            SetDlgItemText(hwnd, IDC_LMB_HEADER,      l10n->input_mouse_lmb);
            SetDlgItemText(hwnd, IDC_MMB_HEADER,      l10n->input_mouse_mmb);
            SetDlgItemText(hwnd, IDC_RMB_HEADER,      l10n->input_mouse_rmb);
            SetDlgItemText(hwnd, IDC_MB4_HEADER,      l10n->input_mouse_mb4);
            SetDlgItemText(hwnd, IDC_MB5_HEADER,      l10n->input_mouse_mb5);
            SetDlgItemText(hwnd, IDC_SCROLL_HEADER,   l10n->input_mouse_scroll);

            SetDlgItemText(hwnd, IDC_LOWERWITHMMB,    l10n->input_mouse_lowerwithmmb);
            SetDlgItemText(hwnd, IDC_ROLLWITHTBSCROLL,l10n->input_mouse_rollwithtbscroll);

            SetDlgItemText(hwnd, IDC_HOTCLICKS_BOX,   l10n->input_hotclicks_box);
            SetDlgItemText(hwnd, IDC_HOTCLICKS_MORE,  l10n->input_hotclicks_more);
            SetDlgItemText(hwnd, IDC_MMB_HC,          l10n->input_mouse_mmb_hc);
            SetDlgItemText(hwnd, IDC_MB4_HC,          l10n->input_mouse_mb4_hc);
            SetDlgItemText(hwnd, IDC_MB5_HC,          l10n->input_mouse_mb5_hc);
        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            wchar_t txt[32];
            // Mouse actions, for all mouse buttons...
            unsigned i;
            for (i = 0; i < ARR_SZ(mouse_buttons); i++) {
                int j = ComboBox_GetCurSel(GetDlgItem(hwnd, mouse_buttons[i].control));
                WritePrivateProfileString(L"Input", mouse_buttons[i].option, mouse_actions[j].action, inipath);
            }
            // Scroll
            i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SCROLL));
            WritePrivateProfileString(L"Input", L"Scroll", scroll_actions[i].action, inipath);

            // Checkboxes...
            WriteOptionBoolB(IDC_LOWERWITHMMB,     L"Input", L"LowerWithMMB", 0);
            WriteOptionBool(IDC_ROLLWITHTBSCROLL, L"Input", L"RollWithTBScroll");
            // Hotclicks
            SaveHotKeys(hotclicks, hwnd, L"Hotclicks", inipath);
            UpdateSettings();
            have_to_apply = 0;
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK KeyboardPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int have_to_apply = 0;
    // Hotkeys
    struct hk_struct hotkeys[] = {
        { IDC_LEFTALT,     VK_LMENU    },
        { IDC_RIGHTALT,    VK_RMENU    },
        { IDC_LEFTWINKEY,  VK_LWIN     },
        { IDC_RIGHTWINKEY, VK_RWIN     },
        { IDC_LEFTCTRL,    VK_LCONTROL },
        { IDC_RIGHTCTRL,   VK_RCONTROL },
        { 0, 0 }
    };
    struct action {
        wchar_t *action;
        wchar_t *l10n;
    };
    struct action kb_actions[] = {
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
        {L"Menu",        l10n->input_actions_menu},
        {L"Nothing",     l10n->input_actions_nothing},
    };
    // Hotkeys
    struct {
        wchar_t *action;
        wchar_t *l10n;
    } togglekeys[] = {
        { L"",   l10n->input_actions_nothing},
        { L"A4", l10n->input_hotkeys_leftalt},
        { L"A5", l10n->input_hotkeys_rightalt},
        { L"5B", l10n->input_hotkeys_leftwinkey},
        { L"5C", l10n->input_hotkeys_rightwinkey},
        { L"A2", l10n->input_hotkeys_leftctrl},
        { L"A3", l10n->input_hotkeys_rightctrl},
        { L"A0", l10n->input_hotkeys_leftshift},
        { L"A1", l10n->input_hotkeys_rightshift},
    };

    if (msg == WM_INITDIALOG) {
        // Agressive Pause
        ReadOptionInt(IDC_AGGRESSIVEPAUSE, L"Input",   L"AggressivePause", 0, -1);
        Button_Enable(GetDlgItem(hwnd, IDC_AGGRESSIVEPAUSE), HaveProc("NTDLL.DLL", "NtResumeProcess"));
        ReadOptionInt(IDC_AGGRESSIVEKILL, L"Input", L"AggressiveKill", 0, -1);
        ReadOptionInt(IDC_KEYCOMBO,       L"Input", L"KeyCombo", 0, -1);
        CheckConfigHotKeys(hotkeys, hwnd, L"Hotkeys", L"A4 A5");
    } else if (msg == WM_COMMAND) {
        int event = HIWORD(wParam);
        if (event == 0 || event == CBN_SELCHANGE) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // GrabWithAlt
            wchar_t txt[32];
            HWND control = GetDlgItem(hwnd, IDC_GRABWITHALT);
            ComboBox_ResetContent(control);
            GetPrivateProfileString(L"Input", L"GrabWithAlt", L"Nothing", txt, ARR_SZ(txt), inipath);
            int sel = ARR_SZ(kb_actions) - 1;
            unsigned j;
            for (j = 0; j < ARR_SZ(kb_actions); j++) {
                wchar_t action_name[256];
                wcscpy_noaccel(action_name, kb_actions[j].l10n, ARR_SZ(action_name));
                ComboBox_AddString(control, action_name);
                if (!wcscmp(txt, kb_actions[j].action)) {
                    sel = j;
                }
            }
            ComboBox_SetCurSel(control, sel);

            // ToggleRzMvKey init
            control = GetDlgItem(hwnd, IDC_TOGGLERZMVKEY);
            ComboBox_ResetContent(control);
            GetPrivateProfileString(L"Input", L"ToggleRzMvKey", L"", txt, ARR_SZ(txt), inipath);
            sel = ARR_SZ(togglekeys) - 1;
            for (j = 0; j < ARR_SZ(togglekeys); j++) {
                wchar_t key_name[256];
                wcscpy_noaccel(key_name, togglekeys[j].l10n, ARR_SZ(key_name));
                ComboBox_AddString(control, key_name);
                if (!wcscmp(txt, togglekeys[j].action)) {
                    sel = j;
                }
            }
            ComboBox_SetCurSel(control, sel);
            // Update text
            SetDlgItemText(hwnd, IDC_KEYBOARD_BOX,    l10n->tab_keyboard);
            SetDlgItemText(hwnd, IDC_AGGRESSIVEPAUSE, l10n->input_aggressive_pause);
            SetDlgItemText(hwnd, IDC_AGGRESSIVEKILL,  l10n->input_aggressive_kill);
            SetDlgItemText(hwnd, IDC_HOTKEYS_BOX,     l10n->input_hotkeys_box);
            SetDlgItemText(hwnd, IDC_TOGGLERZMVKEY_H, l10n->input_hotkeys_togglerzmvkey);
            SetDlgItemText(hwnd, IDC_LEFTALT,         l10n->input_hotkeys_leftalt);
            SetDlgItemText(hwnd, IDC_RIGHTALT,        l10n->input_hotkeys_rightalt);
            SetDlgItemText(hwnd, IDC_LEFTWINKEY,      l10n->input_hotkeys_leftwinkey);
            SetDlgItemText(hwnd, IDC_RIGHTWINKEY,     l10n->input_hotkeys_rightwinkey);
            SetDlgItemText(hwnd, IDC_LEFTCTRL,        l10n->input_hotkeys_leftctrl);
            SetDlgItemText(hwnd, IDC_RIGHTCTRL,       l10n->input_hotkeys_rightctrl);
            SetDlgItemText(hwnd, IDC_HOTKEYS_MORE,    l10n->input_hotkeys_more);
            SetDlgItemText(hwnd, IDC_KEYCOMBO,        l10n->input_keycombo);
            SetDlgItemText(hwnd, IDC_GRABWITHALT_H,   l10n->input_grabwithalt);
        } else if (pnmh->code == PSN_APPLY && have_to_apply ) {
            int i;
            wchar_t txt[10];

            // Action without click
            i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_GRABWITHALT));
            WritePrivateProfileString(L"Input", L"GrabWithAlt", kb_actions[i].action, inipath);
            WriteOptionBool(IDC_AGGRESSIVEPAUSE, L"Input", L"AggressivePause");
            WriteOptionBool(IDC_AGGRESSIVEKILL,  L"Input", L"AggressiveKill");
            // Invert move/resize key.
            i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_TOGGLERZMVKEY));
            WritePrivateProfileString(L"Input", L"ToggleRzMvKey", togglekeys[i].action, inipath);
            // Hotkeys
            SaveHotKeys(hotkeys, hwnd, L"Hotkeys", inipath);
            WriteOptionBool(IDC_KEYCOMBO,  L"Input", L"KeyCombo");
            UpdateSettings();
            have_to_apply = 0;
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK BlacklistPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int have_to_apply = 0;
    if (msg == WM_INITDIALOG) {
        wchar_t txt[1024];
        BOOL haveProcessBL = HaveProc("PSAPI.DLL", "GetModuleFileNameExW");
        ReadOptionStr(IDC_PROCESSBLACKLIST, L"Blacklist", L"Processes", L"");
        Button_Enable(GetDlgItem(hwnd, IDC_PROCESSBLACKLIST), haveProcessBL);
        ReadOptionStr(IDC_BLACKLIST, L"Blacklist",  L"Windows", L"");
        ReadOptionStr(IDC_SCROLLLIST,L"Blacklist", L"Scroll", L"");
        ReadOptionStr(IDC_MDIS,      L"Blacklist", L"MDIs", L"");
        Button_Enable(GetDlgItem(hwnd, IDC_MDIS), GetPrivateProfileInt(L"General", L"MDI", 1, inipath));
        ReadOptionStr(IDC_PAUSEBL,   L"Blacklist", L"Pause", L"");
        Button_Enable(GetDlgItem(hwnd, IDC_PAUSEBL), haveProcessBL);
    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);
        if (event == EN_UPDATE && id != IDC_NEWRULE && id != IDC_NEWPROGNAME) {
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
            SetDlgItemText(hwnd, IDC_BLACKLIST_BOX          , l10n->blacklist_box);
            SetDlgItemText(hwnd, IDC_PROCESSBLACKLIST_HEADER, l10n->blacklist_processblacklist);
            SetDlgItemText(hwnd, IDC_BLACKLIST_HEADER       , l10n->blacklist_blacklist);
            SetDlgItemText(hwnd, IDC_SCROLLLIST_HEADER      , l10n->blacklist_scrolllist);
            SetDlgItemText(hwnd, IDC_MDISBL_HEADER          , l10n->blacklist_mdis);
            SetDlgItemText(hwnd, IDC_PAUSEBL_HEADER         , l10n->blacklist_pause);
            SetDlgItemText(hwnd, IDC_FINDWINDOW_BOX         , l10n->blacklist_findwindow_box);
        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // Save to the config
            wchar_t txt[1024];
            WriteOptionStr(IDC_PROCESSBLACKLIST, L"Blacklist", L"Processes");
            WriteOptionStr(IDC_BLACKLIST,        L"Blacklist", L"Windows");
            WriteOptionStr(IDC_SCROLLLIST,       L"Blacklist", L"Scroll");
            WriteOptionStr(IDC_MDIS,             L"Blacklist", L"MDIs");
            WriteOptionStr(IDC_PAUSEBL,          L"Blacklist", L"Pause");
            UpdateSettings();
            have_to_apply = 0;
        }
    }

    return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
static void SetWindowEmpty(HWND hwnd)
{
    // Reduce the size to 0 to avoid redrawing.
    SetWindowPos(hwnd, NULL, 0,0,0,0
        , SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_DEFERERASE);
    ShowWindow(hwnd, SW_HIDE);
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
            HWND window = WindowFromPoint(pt);
            window = GetAncestor(window, GA_ROOT);

            wchar_t title[256], classname[256];
            GetWindowText(window, title, ARR_SZ(title));
            GetClassName(window, classname, ARR_SZ(classname));

            wchar_t txt[512];
            txt[0] = '\0';
            wcscat(txt, title); wcscat(txt, L"|"); wcscat(txt, classname);
            SetDlgItemText(page, IDC_NEWRULE, txt);

            if(GetWindowProgName(window, txt, ARR_SZ(txt))) {
                SetDlgItemText(page, IDC_NEWPROGNAME, txt);
            }
        }
        // Show icon again
        ShowWindowAsync(GetDlgItem(page, IDC_FINDWINDOW), SW_SHOW);

        DestroyWindow(hwnd);
        UnregisterClass(APP_NAME"-find", g_hinst);
    } else if (wParam && (msg == WM_PAINT || msg == WM_ERASEBKGND)){
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK AboutPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // Update text
            SetDlgItemText(hwnd, IDC_ABOUT_BOX, l10n->about_box);
            SetDlgItemText(hwnd, IDC_VERSION, l10n->about_version);
            SetDlgItemText(hwnd, IDC_URL, L"https://github.com/RamonUnch/AltDrag");
            SetDlgItemText(hwnd, IDC_AUTHOR, l10n->about_author);
            SetDlgItemText(hwnd, IDC_LICENSE, l10n->about_license);
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
/////////////////////////////////////////////////////////////////////////////
// Simple windows proc that draws the resizing regions.
LRESULT CALLBACK TestWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int centerfrac=24;
    switch (msg) {
    case WM_PAINT:;
        RECT cRect, wRect;
        HPEN pen = (HPEN) CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        GetWindowRect(hwnd, &wRect);
        GetClientRect(hwnd, &cRect);
        POINT Offset = { wRect.left, wRect.top };
        ScreenToClient(hwnd, &Offset);

        SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        SelectObject(hdc, pen);
        SetROP2(hdc, R2_BLACK);
        int width = wRect.right - wRect.left;
        int height = wRect.bottom - wRect.top;

        FillRect(hdc, &cRect, GetStockObject(WHITE_BRUSH));
        Rectangle(hdc
            , Offset.x+(width-width*centerfrac/100)/2
            , Offset.y
            , (width+width*centerfrac/100)/2 + Offset.x
            , height);
        Rectangle(hdc
            , Offset.x
            , Offset.y+(height-height*centerfrac/100)/2
            , width
            , (height+height*centerfrac/100)/2 + Offset.y);

        DeleteObject(pen);

        EndPaint(hwnd, &ps);
        return 0;
        break;

    case WM_ERASEBKGND:
        return 0;
        break;

    case WM_UPDCFRACTION:
        centerfrac = lParam;
        return 0;
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        UnregisterClass(APP_NAME"-Test", g_hinst);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK AdvancedPageDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND testwnd=NULL;
    static int have_to_apply = 0;
    if (msg == WM_INITDIALOG) {
        wchar_t txt[10];

        ReadOptionInt(IDC_AUTOREMAXIMIZE,   L"Advanced", L"AutoRemaximize", 0, -1);
        ReadOptionInt(IDC_AEROTOPMAXIMIZES, L"Advanced", L"AeroTopMaximizes", 1, 1);// bit 1
        ReadOptionInt(IDC_AERODBCLICKSHIFT, L"Advanced", L"AeroTopMaximizes", 1, 2);// bit 2
        ReadOptionInt(IDC_MULTIPLEINSTANCES,L"Advanced", L"MultipleInstances",0, -1);
        ReadOptionInt(IDC_NORMRESTORE,      L"General",  L"NormRestore", 0, -1);
        ReadOptionInt(IDC_FULLSCREEN,       L"Advanced", L"FullScreen", 1, -1);

        ReadOptionInt(IDC_MAXWITHLCLICK,    L"General", L"MMMaximize", 1, 1); // bit 1
        ReadOptionInt(IDC_RESTOREONCLICK,   L"General", L"MMMaximize", 1, 2); // bit 2

        ReadOptionStr(IDC_CENTERFRACTION,L"General", L"CenterFraction", L"24");
        ReadOptionStr(IDC_AEROHOFFSET,   L"General",  L"AeroHoffset",   L"50");
        ReadOptionStr(IDC_AEROVOFFSET,   L"General",  L"AeroVoffset",   L"50");
        ReadOptionStr(IDC_SNAPTHRESHOLD, L"Advanced", L"SnapThreshold", L"20");
        ReadOptionStr(IDC_AEROTHRESHOLD, L"Advanced", L"AeroThreshold", L"5");
        ReadOptionStr(IDC_AEROSPEED,     L"Advanced", L"AeroMaxSpeed",  L"");
        ReadOptionStr(IDC_AEROSPEEDTAU,  L"Advanced", L"AeroSpeedTau",  L"32");
        ReadOptionStr(IDC_MOVETRANS,     L"General",  L"MoveTrans",     L"");

    } else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);

        if (id != IDC_TESTWINDOW && (event == 0 || event == EN_UPDATE)) {
            PropSheet_Changed(g_cfgwnd, hwnd);
            have_to_apply = 1;
        }
        if (id == IDC_TESTWINDOW) { // Click on the Test Window button
            if (testwnd && IsWindow(testwnd)) {
                return FALSE;
            }
            WNDCLASSEX wnd = {
                sizeof(WNDCLASSEX)
              , CS_HREDRAW|CS_VREDRAW
              , TestWindowProc
              , 0, 0, g_hinst, icon[1]
              , LoadCursor(NULL, IDC_ARROW)
              , (HBRUSH)(COLOR_BACKGROUND+1)
              , NULL, APP_NAME"-Test", NULL
            };
            RegisterClassEx(&wnd);
            wchar_t wintitle[256];
            wcscpy_noaccel(wintitle, l10n->advanced_testwindow, ARR_SZ(wintitle));
            testwnd = CreateWindowEx(0
                 , wnd.lpszClassName
                 , wintitle, WS_CAPTION|WS_OVERLAPPEDWINDOW
                 , CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT
                 , NULL, NULL, g_hinst, NULL);
            PostMessage(testwnd, WM_UPDCFRACTION, 0
                 , GetPrivateProfileInt(L"General", L"CenterFraction", 24, inipath));
            ShowWindow(testwnd, SW_SHOW);
        }
    } else if (msg == WM_NOTIFY) {
        LPNMHDR pnmh = (LPNMHDR) lParam;
        if (pnmh->code == PSN_SETACTIVE) {
            // Update text
            SetDlgItemText(hwnd, IDC_METRICS_BOX,      l10n->advanced_metrics_box);
            SetDlgItemText(hwnd, IDC_CENTERFRACTION_H, l10n->advanced_centerfraction);
            SetDlgItemText(hwnd, IDC_AEROHOFFSET_H,    l10n->advanced_aerohoffset);
            SetDlgItemText(hwnd, IDC_AEROVOFFSET_H,    l10n->advanced_aerovoffset);
            SetDlgItemText(hwnd, IDC_SNAPTHRESHOLD_H,  l10n->advanced_snapthreshold);
            SetDlgItemText(hwnd, IDC_AEROTHRESHOLD_H,  l10n->advanced_aerothreshold);
            SetDlgItemText(hwnd, IDC_AEROSPEED_H,      l10n->advanced_aerospeed);
            SetDlgItemText(hwnd, IDC_MOVETRANS_H,      l10n->advanced_movetrans);
            SetDlgItemText(hwnd, IDC_TESTWINDOW,       l10n->advanced_testwindow);

            SetDlgItemText(hwnd, IDC_BEHAVIOR_BOX,     l10n->advanced_behavior_box);
            SetDlgItemText(hwnd, IDC_MULTIPLEINSTANCES,l10n->advanced_multipleinstances);
            SetDlgItemText(hwnd, IDC_AUTOREMAXIMIZE,   l10n->advanced_autoremaximize);
            SetDlgItemText(hwnd, IDC_NORMRESTORE,      l10n->advanced_normrestore);
            SetDlgItemText(hwnd, IDC_AEROTOPMAXIMIZES, l10n->advanced_aerotopmaximizes);
            SetDlgItemText(hwnd, IDC_AERODBCLICKSHIFT, l10n->advanced_aerodbclickshift);
            SetDlgItemText(hwnd, IDC_MAXWITHLCLICK,    l10n->advanced_maxwithlclick);
            SetDlgItemText(hwnd, IDC_RESTOREONCLICK,   l10n->advanced_restoreonclick);
            SetDlgItemText(hwnd, IDC_FULLSCREEN,       l10n->advanced_fullscreen);

        } else if (pnmh->code == PSN_APPLY && have_to_apply) {
            // Apply or OK button was pressed.
            // Save settings
            wchar_t txt[10];
            int val;
            WriteOptionBool(IDC_MULTIPLEINSTANCES, L"Advanced", L"MultipleInstances");
            WriteOptionBool(IDC_AUTOREMAXIMIZE,    L"Advanced", L"AutoRemaximize");
            WriteOptionBool(IDC_NORMRESTORE,       L"General",  L"NormRestore");
            WriteOptionBool(IDC_FULLSCREEN,        L"Advanced", L"FullScreen");

            val = IsChecked(IDC_AEROTOPMAXIMIZES) + 2 * IsChecked(IDC_AERODBCLICKSHIFT);
            WritePrivateProfileString(L"Advanced",L"AeroTopMaximizes", _itow(val, txt, 10), inipath);
            val = IsChecked(IDC_MAXWITHLCLICK) + 2 * IsChecked(IDC_RESTOREONCLICK);
            WritePrivateProfileString(L"General", L"MMMaximize", _itow(val, txt, 10), inipath);

            WriteOptionStr(IDC_CENTERFRACTION,L"General",  L"CenterFraction");
            WriteOptionStr(IDC_AEROHOFFSET,   L"General",  L"AeroHoffset");
            WriteOptionStr(IDC_AEROVOFFSET,   L"General",  L"AeroVoffset");
            WriteOptionStr(IDC_SNAPTHRESHOLD, L"Advanced", L"SnapThreshold");
            WriteOptionStr(IDC_AEROTHRESHOLD, L"Advanced", L"AeroThreshold");
            WriteOptionStr(IDC_AEROSPEED,     L"Advanced", L"AeroMaxSpeed");
            WriteOptionStr(IDC_MOVETRANS,     L"General",  L"MoveTrans");

            // Update center fraction in Test window in if open.
            if (testwnd && IsWindow(testwnd)) {
                int centerfraction = 24;
                Edit_GetText(GetDlgItem(hwnd, IDC_CENTERFRACTION), txt, ARR_SZ(txt));
                centerfraction = _wtoi(txt);
                PostMessage(testwnd, WM_UPDCFRACTION, 0, centerfraction);
                SetWindowPos(testwnd, NULL, 0, 0, 0, 0
                    , SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE );
            }
            UpdateSettings();
            have_to_apply = 0;
        }
    }
    return FALSE;
}
