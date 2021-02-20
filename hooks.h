/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2021                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0400
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#define COBJMACROS
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "unfuck.h"

// App
#define APP_NAME L"AltDrag"

// Boring stuff
#define REHOOK_TIMER  WM_APP+1
#define INIT_TIMER    WM_APP+2

HWND g_hwnd;
static void UnhookMouse();
static void HookMouse();

// Enumerators
enum action { AC_NONE=0, AC_MOVE, AC_RESIZE, AC_MINIMIZE, AC_MAXIMIZE, AC_CENTER
            , AC_ALWAYSONTOP, AC_CLOSE, AC_LOWER, AC_ALTTAB, AC_VOLUME, AC_ROLL
		    , AC_TRANSPARENCY, AC_BORDERLESS };
enum button { BT_NONE=0, BT_LMB=0x0002, BT_MMB=0x0020, BT_RMB=0x0008, BT_MB4=0x0080, BT_MB5=0x0081 };
enum resize { RZ_NONE=0, RZ_TOP, RZ_RIGHT, RZ_BOTTOM, RZ_LEFT, RZ_CENTER };
enum cursor { HAND, SIZENWSE, SIZENESW, SIZENS, SIZEWE, SIZEALL };

static int init_movement_and_actions(POINT pt, enum action action, int button);
static void FinishMovement();

// Window database
#define NUMWNDDB 64
struct wnddata {
    HWND hwnd;
    int width;
    int height;
    int restore;
};
struct {
    struct wnddata items[NUMWNDDB];
    struct wnddata *pos;
} wnddb;

RECT oldRect;
HDC hdcc;
HPEN hpenDot_Global=NULL;
HDWP hWinPosInfo;

struct windowRR {
    HWND hwnd;
    int x;
    int y;
    int width;
    int height;
} LastWin;

// State
struct {
    POINT clickpt;
    POINT prevpt;
    POINT offset;
    HWND hwnd;
    HWND mdiclient;
    struct wnddata *wndentry;
    DWORD clicktime;
    int Speed;

    char alt;
    unsigned char alt1;
    char blockaltup;
    char blockmouseup;

    char ignorectrl;
    char ctrl;
    char shift;
    char snap;

    char moving;
    unsigned char clickbutton;

    struct {
        char maximized;
        char fullscreen;
        HMONITOR monitor;
        int width;
        int height;
        int right;
        int bottom;
    } origin;

    enum action action;
    struct {
        enum resize x, y;
    } resize;

    struct {
        POINT Min;
        POINT Max;
    } mmi;
} state;

// Snap
RECT *monitors = NULL;
int nummonitors = 0;
RECT *wnds = NULL;
int numwnds = 0;
HWND *hwnds = NULL;
int numhwnds = 0;
HWND progman = NULL;

// Settings
#define MAXKEYS 7

struct hotkeys_s {
    unsigned char length;
    unsigned char keys[MAXKEYS];
};

struct {
    enum action GrabWithAlt;

    char AutoFocus;
    char AutoSnap;
    char AutoRemaximize;
    char Aero;

    char MDI;
    char InactiveScroll;
    char LowerWithMMB;
    char ResizeCenter;

    unsigned char MoveRate;
    unsigned char ResizeRate;
    unsigned char SnapThreshold;
    unsigned char AeroThreshold;

    unsigned char AVoff;
    unsigned char AHoff;
    unsigned short dbclktime;

    char FullWin;
    char ResizeAll;
    char AggressivePause;
    char AeroTopMaximizes;

    char UseCursor;
    unsigned char CenterFraction;
    unsigned char RefreshRate;
    char RollWithTBScroll;

    char MMMaximize;
    unsigned char MinAlpha;
    unsigned char MoveTrans;
    char NormRestore;

    char AlphaDelta;
    char AlphaDeltaShift;
    unsigned short AeroMaxSpeed;

    unsigned char AeroSpeedInt;
    unsigned char ToggleRzMvKey;
    char keepMousehook;
    char KeyCombo;

    struct hotkeys_s Hotkeys;
    struct hotkeys_s Hotclick;

    struct {
        enum action LMB, MMB, RMB, MB4, MB5, Scroll;
    } Mouse;
} conf ;

// Blacklist (dynamically allocated)
struct blacklistitem {
    wchar_t *title;
    wchar_t *classname;
};
struct blacklist {
    struct blacklistitem *items;
    int length;
    wchar_t *data;
};
struct {
    struct blacklist Processes;
    struct blacklist Windows;
    struct blacklist Snaplist;
    struct blacklist MDIs;
    struct blacklist Pause;
    struct blacklist MMBLower;
    struct blacklist Scroll;
} BlkLst = { {NULL, 0, NULL}, {NULL, 0, NULL}, {NULL, 0, NULL}
           , {NULL, 0, NULL}, {NULL, 0, NULL}, {NULL, 0, NULL}
           , {NULL, 0, NULL}};

// Cursor data
HWND cursorwnd = NULL;
HCURSOR cursors[6];

// Hook data
HINSTANCE hinstDLL = NULL;
HHOOK mousehook = NULL;

