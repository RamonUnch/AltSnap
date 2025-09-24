/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Compatibility layer for old NT OSes                                   *
 * Written by Raymond Gillibert in 2021                                  *
 * THIS FILE IS NOT UNDER GPL but under the WTFPL.                       *
 * DO WHAT THE FUCK YOU WANT WITH THIS CODE!                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _UNFUCK_NT_
#define _UNFUCK_NT_

#include <windows.h>
#include <oleacc.h>
#include <stdio.h>
#include "nanolibc.h"

/* #include <dwmapi.h> */
enum DWMWINDOWATTRIBUTE {
  /* Windows Vista+ */
  DWMWA_NCRENDERING_ENABLED=1,
  DWMWA_NCRENDERING_POLICY,
  DWMWA_TRANSITIONS_FORCEDISABLED,
  DWMWA_ALLOW_NCPAINT,
  DWMWA_CAPTION_BUTTON_BOUNDS,
  DWMWA_NONCLIENT_RTL_LAYOUT,
  DWMWA_FORCE_ICONIC_REPRESENTATION,
  DWMWA_FLIP3D_POLICY,
  DWMWA_EXTENDED_FRAME_BOUNDS,
  DWMWA_HAS_ICONIC_BITMAP,
  /* Windows 7+ */
  DWMWA_DISALLOW_PEEK,
  DWMWA_EXCLUDED_FROM_PEEK,
  DWMWA_CLOAK,
  /* Windows 8+ */
  DWMWA_CLOAKED, /* 14 */
  DWMWA_FREEZE_REPRESENTATION,
  DWMWA_PASSIVE_UPDATE_MODE,
  DWMWA_USE_HOSTBACKDROPBRUSH,              /* Set, *pvAttribute=BOOL */

  /* Windows 10 1809 + (17763 <= build < 18985), Undocumented */
  DWMWA_USE_IMMERSIVE_DARK_MODE_PRE20H1=19, /* Set, *pvAttribute=BOOL */
  /* Windows 10 21H1 + (build >= 18985) */
  DWMWA_USE_IMMERSIVE_DARK_MODE = 20,  /* Documented value since Win 11 build 22000 */

  /* Windows 11 Build 22000 + */
  DWMWA_WINDOW_CORNER_PREFERENCE = 33,
  DWMWA_BORDER_COLOR,
  DWMWA_CAPTION_COLOR,
  DWMWA_TEXT_COLOR,
  DWMWA_VISIBLE_FRAME_BORDER_THICKNESS,
  /* Windows 11 Build 22621 + */
  DWMWA_SYSTEMBACKDROP_TYPE,
  DWMWA_LAST,
};

enum DWM_WINDOW_CORNER_PREFERENCE {
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3,
};

enum MONITOR_DPI_TYPE {
  MDT_EFFECTIVE_DPI = 0,
  MDT_ANGULAR_DPI = 1,
  MDT_RAW_DPI = 2,
  MDT_DEFAULT = MDT_EFFECTIVE_DPI
};

/* Invalid pointer with which we initialize
 * all dynamically imported functions */
#define IPTR ((FARPROC)(1))

#define QWORD unsigned long long
#ifdef _WIN64
    #define CopyRect(x, y) (*(x) = *(y))
    #define DorQWORD QWORD
    #define HIWORDPTR(ll)   ((DWORD) (((QWORD) (ll) >> 32) & 0xFFFFFFFF))
    #define LOWORDPTR(ll)   ((DWORD) (ll))
    #define MAKELONGPTR(lo, hi) ((QWORD) (((DWORD) (lo)) | ((QWORD) ((DWORD) (hi))) << 32))
#else
    #define DorQWORD unsigned long
    #define HIWORDPTR(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF))
    #define LOWORDPTR(l)   ((WORD) (l))
    #define MAKELONGPTR(lo, hi) ((DWORD) (((WORD) (lo)) | ((DWORD) ((WORD) (hi))) << 16))
#endif
#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)(w))
#endif

#ifndef HIBYTE
#define HIBYTE(w) ( (BYTE)(((WORD) (w) >> 8) & 0xFF) )
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))
#endif


#define ARR_SZ(x) (sizeof(x) / sizeof((x)[0]))
#define IDAPPLY 0x3021

#ifndef IS_SURROGATE_PAIR
#define IS_HIGH_SURROGATE(wch) (((wch) >= 0xd800) && ((wch) <= 0xdbff))
#define IS_LOW_SURROGATE(wch) (((wch) >= 0xdc00) && ((wch) <= 0xdfff))
#define IS_SURROGATE_PAIR(hs, ls) (IS_HIGH_SURROGATE (hs) && IS_LOW_SURROGATE (ls))
#endif

#ifndef NIIF_USER
#define NIIF_USER 0x00000004
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif

#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x10000L
#endif

#ifndef WPF_ASYNCWINDOWPLACEMENT
#define WPF_ASYNCWINDOWPLACEMENT 0x0004
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define InterlockedIncrement(x) InterlockedIncrement((LONG*)(x))
#define InterlockedDecrement(x) InterlockedDecrement((LONG*)(x))
#endif

#ifndef SCANCODE_SIMULATED
#define SCANCODE_SIMULATED 0x0200
#endif

#ifndef EVENT_SYSTEM_MOVESIZESTART
#define EVENT_SYSTEM_MOVESIZESTART 0x000A
#define EVENT_SYSTEM_MOVESIZEEND 0x000B
#endif

#ifndef MSAA_MENU_SIG
#define MSAA_MENU_SIG 0xAA0DF00D
typedef struct tagMSAAMENUINFO {
  DWORD dwMSAASignature;
  DWORD cchWText;
  LPWSTR pszWText;
} MSAAMENUINFO,*LPMSAAMENUINFO;
#endif

#ifndef GetWindowLongPtr
    #define GetWindowLongPtr GetWindowLong
    #define SetWindowLongPtr SetWindowLong
    #define GetClassLongPtr GetClassLong
    #define SetClassLongPtr SetClassLong
    #define GWLP_WNDPROC (-4)
    #define GWLP_HINSTANCE (-6)
    #define GWLP_HWNDPARENT (-8)
    #define GWLP_USERDATA (-21)
    #define GWLP_ID (-12)
#endif

#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME (0x0800)
#endif

#ifndef WS_EX_LAYERED
    #define WS_EX_LAYERED 0x00080000
    #define WS_EX_NOACTIVATE 0x08000000
    #define LWA_COLORKEY 0x00000001
    #define LWA_ALPHA 0x00000002
#endif

#ifndef WM_XBUTTONDOWN
    #define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
    #define WM_XBUTTONDOWN 0x020B
    #define WM_XBUTTONUP 0x020C
    #define WM_XBUTTONDBLCLK 0x020D
    #define WM_MOUSEHWHEEL 0x020e
    #define VK_XBUTTON1 0x05
    #define VK_XBUTTON2 0x06
    #define VK_VOLUME_MUTE 0xAD
    #define VK_VOLUME_DOWN 0xAE
    #define VK_VOLUME_UP 0xAF
    #define MOUSEEVENTF_XDOWN 0x0080
    #define MOUSEEVENTF_XUP 0x0100
    #define KEYEVENTF_UNICODE 0x0004
    #define KEYEVENTF_SCANCODE 0x0008
#endif

#ifndef GCLP_HCURSOR
    #define GCLP_HCURSOR (-12)
    #define GCLP_HICON (-14)
#endif

//#define LOGA(X, ...) {DWORD err=GetLastError(); FILE *LOG=fopen("ad.log", "a"); fprintf(LOG, X, ##__VA_ARGS__); fprintf(LOG,", LastError=%lu\n",err); fclose(LOG); SetLastError(0); }
#define LOGA LOGfunk
/* Cool warpper for wvsprintf */
static void LOGfunk( const char *fmt, ... )
{
    DWORD lerr = GetLastError();
    va_list arglist;
    char str[512];
    HANDLE h;

    va_start( arglist, fmt );
    wvsprintfA( str, fmt, arglist );
    va_end( arglist );

    h = CreateFileA( "ad.log",
        FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if( h == INVALID_HANDLE_VALUE )
        return;
    {
    char lerrorstr[16];
    DWORD dummy;
    lstrcat_sA(str, ARR_SZ(str), " (");
    lstrcat_sA(str, ARR_SZ(str), itostrA(lerr, lerrorstr, 10));
    lstrcat_sA(str, ARR_SZ(str), ")\n");
    WriteFile( h, str, lstrlenA(str), &dummy, NULL );
    CloseHandle(h);
    SetLastError(0);
    }
}
#ifdef LOG_STUFF
#define LOG LOGfunk
#else
    #define LOG if(0) LOGdummy
    static void LOGdummy(const char *fmt, ...) {}
#endif

#ifdef DEBUG
#define assert(x) do{if(!(x)) { LOGA("ASSERT in " __FILE__":%d - " #x, __LINE__); ExitProcess(1); } }while(0)
#else
#undef assert
#define assert(x)
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 202300L
#ifndef static_assert
    #if defined(__STDC_VERSION__)  && __STDC_VERSION__ >= 201112L
        // C11 cool _Static_assert
        #define static_assert _Static_assert
    #else
        #define static_assert(x, y) enum assert_static__ { assert_static___ = 1/(x) };
    #endif
#endif //static_assert
#endif // [C89 - C23[

/* Stuff missing in MinGW */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

/* on both x64 and x32 */
#define NtSuspendProcess NtSuspendProcessL
#define NtResumeProcess NtResumeProcessL
#ifndef _WIN64
    #define GetLayeredWindowAttributes GetLayeredWindowAttributesL
    #define SetLayeredWindowAttributes SetLayeredWindowAttributesL
    #define GetAncestor GetAncestorL
    #undef GetMonitorInfo
    #define GetMonitorInfo GetMonitorInfoL
    #define EnumDisplayMonitors EnumDisplayMonitorsL
    #define MonitorFromPoint MonitorFromPointL
    #define MonitorFromWindow MonitorFromWindowL
/*    #define GetGUIThreadInfo GetGUIThreadInfoL (NT4 SP3+/Win98+) */
#endif

/* Helper function to pop a message bow with error code*/
static void ErrorBox(const TCHAR * const title)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
        (TCHAR *)&lpMsgBuf,
        0, NULL
    );
    MessageBox( NULL, (LPCTSTR)lpMsgBuf, title, MB_OK | MB_ICONWARNING );
    /* Free the buffer using LocalFree. */
    LocalFree( lpMsgBuf );
}

/* Helper functiont to strcat variable amount of TCHAR*s */
static size_t lstrcatM_s(TCHAR *d, size_t dl, ...)
{
    while( *d && dl-- ) d++;
    va_list arglist;
    va_start( arglist, dl );

    while(1) {
        if(dl == 0) break; /* End of string! */
        const TCHAR *s = (const TCHAR *)va_arg(arglist, const TCHAR*);
        if (s == NULL) break;
        /* inline naive strcpy_s */
        for (; dl && (*d=*s); ++s,++d,--dl);
    }
    *d = TEXT('\0'); /* Ensure NULL termination */

    va_end( arglist );

    return dl; /* Remaining TCHARs */
}

static int PrintHwndDetails(HWND hwnd, TCHAR buf[AT_LEAST 512+40+2*8+4*12+1])
{
    TCHAR klass[256], title[256];
    klass[0] = title[0] = TEXT('\0');
    RECT rc;
    GetWindowRect(hwnd, &rc);
    GetClassName(hwnd, klass, ARR_SZ(klass));
    GetWindowText(hwnd, title, ARR_SZ(title));
    return wsprintf(buf
        , TEXT("Hwnd=%x, %s|%s, style=%x, xstyle=%x, rect=%ld,%ld,%ld,%ld")
        , (UINT)(UINT_PTR)hwnd
        , title, klass
        , (UINT)GetWindowLongPtr(hwnd, GWL_STYLE)
        , (UINT)GetWindowLongPtr(hwnd, GWL_EXSTYLE)
        , rc.left, rc.top, rc.right, rc.bottom
        );
}

/* Helper to be able to enable/disable dialog items
 * easily while ensuring we move keyboard focus to
 * the next control if it was selected */
static BOOL EnableDlgItem(HWND hdlg, UINT id, BOOL enable)
{
    HWND hwndControl = GetDlgItem(hdlg, id);
    if (!enable && hwndControl == GetFocus()) {
        SendMessage(hdlg, WM_NEXTDLGCTL, 0, FALSE);
    }
    return EnableWindow(hwndControl, enable);
}

static DWORD GetMsgPT(POINT *pt)
{
    DWORD point = GetMessagePos();
    pt->x = GET_X_LPARAM(point);
    pt->y = GET_Y_LPARAM(point);

    return point;
}

/* Removes the trailing file name from a path */
static BOOL PathRemoveFileSpecL(TCHAR *p)
{
    int i=0;
    if (!p) return FALSE;

    while(p[++i] != '\0');
    while(i > 0 && p[i] != '\\') i--;
    p[i]='\0';

    return TRUE;
}

/* Removes the path and keeps only the file name */
static void PathStripPathL(TCHAR *p)
{
    int i=0, j;
    if (!p) return;

    while(p[++i] != '\0');
    while(i >= 0 && p[i] != '\\') i--;
    i++;
    for(j=0; p[i+j] != '\0'; j++) p[j]=p[i+j];
    p[j]= '\0';
}

static BOOL HaveProc(const char * const DLLname, const char * const PROCname)
{
    HINSTANCE hdll = LoadLibraryA(DLLname);
    BOOL ret = FALSE;
    if (hdll) {
        if(GetProcAddress(hdll, PROCname)) {
            ret = TRUE ;
        }
        FreeLibrary(hdll);
    }
    return ret;
}

static FARPROC LoadDLLProc(const char *DLLname, const char *PROCname)
{
    HINSTANCE hdll;
    FARPROC ret = (FARPROC)NULL;

    if((hdll = GetModuleHandleA(DLLname))) {
        return GetProcAddress(hdll, PROCname);
    }
    hdll = LoadLibraryA(DLLname);
    if (hdll) {
        ret = GetProcAddress(hdll, PROCname);
        if (!ret) FreeLibrary(hdll);
    }
    return ret;
}

/* Accurate Sleep, needs WINMM.DLL */
static void ASleep(DWORD duration_ms)
{
    static MMRESULT (WINAPI *mtimeGetDevCaps)(LPTIMECAPS ptc, UINT cbtc) = (MMRESULT (WINAPI *)(LPTIMECAPS ptc, UINT cbtc))IPTR;
    static MMRESULT (WINAPI *mtimeBeginPeriod)(UINT uPeriod);
    static MMRESULT (WINAPI *mtimeEndPeriod)(UINT uPeriod);

    if (duration_ms > 15) {
        /* No need for accurate sleep... */
        Sleep(duration_ms);
        return;
    }
    if (mtimeGetDevCaps == (MMRESULT (WINAPI *)(LPTIMECAPS ptc, UINT cbtc))IPTR) {
        HINSTANCE h=LoadLibraryA("WINMM.DLL");
        if (h) {
            mtimeGetDevCaps =(MMRESULT (WINAPI *)(LPTIMECAPS ptc, UINT cbtc))GetProcAddress(h, "timeGetDevCaps");
            mtimeBeginPeriod=(MMRESULT (WINAPI *)(UINT uPeriod))GetProcAddress(h, "timeBeginPeriod");
            mtimeEndPeriod  =(MMRESULT (WINAPI *)(UINT uPeriod))GetProcAddress(h, "timeEndPeriod");
            if(!mtimeGetDevCaps || !mtimeBeginPeriod || !mtimeEndPeriod) {
                mtimeGetDevCaps=NULL;
                FreeLibrary(h);
            }
        }
    }
    /* We have winmm functions */
    /* This absurd code makes Sleep() more accurate
     * - without it, Sleep() is not even +-10ms accurate
     * - with it, Sleep is around +-1.5 ms accurate
     */
    if(mtimeGetDevCaps) {
        TIMECAPS tc;
        mtimeGetDevCaps(&tc, sizeof(tc));
        mtimeBeginPeriod(tc.wPeriodMin); /* begin accurate sleep */

        Sleep(duration_ms); /* perform The SLEEP */

        mtimeEndPeriod(tc.wPeriodMin); /* end accurate sleep*/
    } else {
        Sleep(duration_ms);
    }
}

static BOOL FreeDLLByName(const char * const DLLname)
{
    HINSTANCE hdll;
    if((hdll = GetModuleHandleA(DLLname)))
        return FreeLibrary(hdll);
    return FALSE;
}
static HWND GetAncestorL(HWND hwnd, UINT gaFlags)
{
    typedef HWND (WINAPI *funk_t)(HWND hwnd, UINT gaFlags);
    static funk_t funk = (funk_t)IPTR;
    HWND hlast, hprevious;
    LONG wlong;
    if(!hwnd) return NULL;
    hprevious = hwnd;

    if (funk == (funk_t)IPTR) {
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetAncestor");
    }
    if(funk) { /* We know we have the function */
        return funk(hwnd, gaFlags);
    }
    /* Fallback */
    while ( (hlast = GetParent(hprevious)) != NULL ){
        wlong=GetWindowLong(hprevious, GWL_STYLE);
        if(wlong&(WS_POPUP)) break;
        hprevious=hlast;
    }
    return hprevious;
}

#ifndef MSGFLTINFO_NONE

#define MSGFLTINFO_NONE 0
#define MSGFLTINFO_ALREADYALLOWED_FORWND 1
#define MSGFLTINFO_ALREADYDISALLOWED_FORWND 2
#define MSGFLTINFO_ALLOWED_HIGHER 3

typedef struct tagCHANGEFILTERSTRUCT {
  DWORD cbSize;
  DWORD ExtStatus;
} CHANGEFILTERSTRUCT, *PCHANGEFILTERSTRUCT;
#endif

// On Windows Vista we have to use this one that applies process wide.
static BOOL ChangeWindowMessageFilterL(UINT msg, DWORD ac)
{
    typedef BOOL (WINAPI* funk_t)(UINT msg, DWORD ac);
    static funk_t funk = (funk_t)IPTR;

    if (funk == (funk_t)IPTR) {
        funk = (funk_t)LoadDLLProc("USER32.DLL", "ChangeWindowMessageFilter");
    }
    if (funk) { /* We know we have the function */
        return funk(msg, ac);
    }
    return FALSE;
}

// Available only on Windows 7+s
static BOOL ChangeWindowMessageFilterExL(HWND hwnd, UINT msg, DWORD ac, PCHANGEFILTERSTRUCT pC)
{
    typedef BOOL (WINAPI *funk_t)(HWND hwnd, UINT msg, DWORD ac, PCHANGEFILTERSTRUCT pC);
    static funk_t funk = (funk_t)IPTR;

    if (funk == (funk_t)IPTR) {
        funk = (funk_t)LoadDLLProc("USER32.DLL", "ChangeWindowMessageFilterEx");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd, msg, ac, pC);
    }
    // Process-wide Fallback for Windows Vista!
    return ChangeWindowMessageFilterL(msg, ac);
}

static BOOL GetLayeredWindowAttributesL(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
    typedef BOOL (WINAPI *funk_t)(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags);
    static funk_t funk = (funk_t)IPTR;

    if (funk == (funk_t)IPTR) {
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetLayeredWindowAttributes");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd, pcrKey, pbAlpha, pdwFlags);
    }
    return FALSE;
}

static BOOL SetLayeredWindowAttributesL(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
    typedef BOOL (WINAPI *funk_t)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
    static funk_t funk = (funk_t)IPTR;

    if (funk == (funk_t)IPTR) {
        funk= (funk_t)LoadDLLProc("USER32.DLL", "SetLayeredWindowAttributes");
    }
    if(funk) { /* We know we have the function */
        return funk(hwnd, crKey, bAlpha, dwFlags);
    }
    return FALSE;
}

static BOOL GetMonitorInfoL(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
    typedef BOOL (WINAPI *funk_t)(HMONITOR hMonitor, LPMONITORINFO lpmi);
    static funk_t funk = (funk_t)IPTR;

    if (funk == (funk_t)IPTR) {
    #ifdef _UNICODE
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetMonitorInfoW");
    #else
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetMonitorInfoA");
    #endif
    }
    if(funk) { /* We know we have the function */
        if(hMonitor) return funk(hMonitor, lpmi);
    }
    /* Fallback for NT4 */
    GetClientRect(GetDesktopWindow(), &lpmi->rcMonitor);
    SystemParametersInfo(SPI_GETWORKAREA, 0, &lpmi->rcWork, 0);
    lpmi->dwFlags = MONITORINFOF_PRIMARY;

    return TRUE;
}

static BOOL EnumDisplayMonitorsL(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
    typedef BOOL (WINAPI *funk_t)(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);
    static funk_t funk = (funk_t)IPTR;

    MONITORINFO mi;

    if (funk == (funk_t)IPTR) { /* First time */
        funk= (funk_t)LoadDLLProc("USER32.DLL", "EnumDisplayMonitors");
    }
    if (funk) { /* We know we have the function */
        return funk(hdc, lprcClip, lpfnEnum, dwData);
    }

    /* Fallbak */
    GetMonitorInfoL(NULL, &mi);
    lpfnEnum(NULL, NULL, &mi.rcMonitor, 0); /* Callback function */

    return TRUE;
}

static HMONITOR MonitorFromPointL(POINT pt, DWORD dwFlags)
{
    typedef HMONITOR (WINAPI *funk_t)(POINT pt, DWORD dwFlags);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "MonitorFromPoint");
    }
    if (funk) { /* We know we have the function */
        return funk(pt, dwFlags);
    }
    return NULL;
}

static HMONITOR MonitorFromWindowL(HWND hwnd, DWORD dwFlags)
{
    typedef HMONITOR (WINAPI *funk_t)(HWND hwnd, DWORD dwFlags);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "MonitorFromWindow");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd, dwFlags);
    }
    return NULL;
}

static BOOL GetGUIThreadInfoL(DWORD pid, GUITHREADINFO *lpgui)
{
    typedef BOOL (WINAPI *funk_t)(DWORD pid, GUITHREADINFO *lpgui);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetGUIThreadInfo");
    }
    if (funk) { /* We know we have the function */
        return funk(pid, lpgui);
    }
    return FALSE;
}

static int GetSystemMetricsForDpiL(int  nIndex, UINT dpi)
{
    typedef int (WINAPI *funk_t)(int  nIndex, UINT dpi);
    static funk_t funk=(funk_t)IPTR;

    if (dpi) {
        if (funk == (funk_t)IPTR) { /* First time */
            funk = (funk_t)LoadDLLProc("USER32.DLL", "GetSystemMetricsForDpi");
        }
        if (funk) { /* We know we have the function */
            return funk(nIndex, dpi);
        }
    }
    /* Use non dpi stuff if dpi == 0 or if it does not exist. */
    return GetSystemMetrics(nIndex);
}
#define GetSystemMetricsForDpi GetSystemMetricsForDpiL

static HRESULT GetDpiForMonitorL(HMONITOR hmonitor, int dpiType, UINT *dpiX, UINT *dpiY)
{
    typedef HRESULT (WINAPI *funk_t)(HMONITOR hmonitor, int dpiType, UINT *dpiX, UINT *dpiY);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("SHCORE.DLL", "GetDpiForMonitor");
    }
    if (funk) { /* We know we have the function */
        return funk(hmonitor, dpiType, dpiX, dpiY);
    }
    return 666; /* Fail with 666 error */
}

/* Supported wince Windows 10, version 1607 [desktop apps only] */
static UINT GetDpiForWindow10L(const HWND hwnd)
{
    typedef UINT (WINAPI *funk_t)(const HWND hwnd);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "GetDpiForWindow");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd);
    }

    return 0; /* Not handled */
}

/* With an Windows 8 fallback */
static UINT GetDpiForWindowL(const HWND hwnd)
{
    UINT dpi = GetDpiForWindow10L(hwnd);
    if (!dpi) {
        /* Windows 8.1 / Server2012 R2 Fallback */
        UINT dpiX=0;
        UINT dpiY=0;
        HMONITOR hmon;
        if ((hmon = MonitorFromWindowL(hwnd, MONITOR_DEFAULTTONEAREST))
        && S_OK == GetDpiForMonitorL(hmon, MDT_DEFAULT, &dpiX, &dpiY)) {
            return dpiX;
        }
    }
    return dpi;
}
#define GetDpiForWindow GetDpiForWindowL

/* Fallbacks for all windows */
static UINT ReallyGetDpiForWindow(const HWND hwnd)
{
    UINT dpi = GetDpiForWindowL(hwnd);
    if (!dpi) {
        HDC hdc = GetDC(hwnd);
        if(hdc) {
            dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hwnd, hdc);
            if (!dpi) dpi = 96; /* Default to 96 dpi*/
        } else {
            dpi = 96; /* cannot et a DC, default to 96 dpi */
        }
    }
    return dpi;
}
/* https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablenonclientdpiscaling
 * Available since Windows 10.0.14342 (~1607) https://stackoverflow.com/questions/36864894
 * Useless since Windows 10.0.15063 (1703)
 * So it is only useful in builds frim 1607 to 1703
 * To be called in the WM_NCCREATE message handler.
 */
static BOOL EnableNonClientDpiScalingL(HWND hwnd)
{
    /* For Windows 10 below build 15063 */
    typedef BOOL (WINAPI *funk_t)(HWND hwnd);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "EnableNonClientDpiScaling");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd);
    }
    return FALSE;
}
/* Only applies to Windows NT for build number */
static xpure BOOL OredredWinVer()
{
    DWORD oVer;
    DWORD ver = GetVersion();
    oVer = (ver&0x000000FF) << 24 /* MAJOR */
           | (ver&0x0000FF00) << 8  /* MINOR */
           | (ver&0xFFFF0000) >> 16;/* BUILDID */

    /* On Windows 9x, no buildID is available in GetVer */
    if (ver & 0x80000000) {
        // Only use minor/major ver.
        oVer |= 0xFFFF0000;
    }
    return oVer;
}
static BOOL IsDarkModeEnabled(void)
{
    DWORD value = 0;
    if ( OredredWinVer() >= 0x0A004563 ) {
        /* In case of Windows 10 build 10.0.17763 or 10.x (future) or 11+ */
        DWORD len = sizeof(value);
        HKEY key;
        if (RegOpenKeyEx(HKEY_CURRENT_USER
                , TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize")
                , 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(key, TEXT("AppsUseLightTheme"), NULL, NULL, (LPBYTE)&value, &len) == ERROR_SUCCESS) {
                value = !value;
            }
            RegCloseKey(key);
        }
   }
   return value;
}
static BOOL IsHighContrastEnabled()
{
    HIGHCONTRAST hc = { sizeof(hc) };
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE))
        return hc.dwFlags & HCF_HIGHCONTRASTON;

    return FALSE;
}
/* We can use the DWMWA_USE_IMMERSIVE_DARK_MODE=20 DWM style
 * It is documented for Win11 build 22000 but it also appears to work for Win10
 * However from build 1809 to 2004 it the value was =19 and it is =20
 * Since Windows 10 build 20H1 and on Win11
 * DWMWA_USE_IMMERSIVE_DARK_MODE=19 up to Win10 2004
 * DWMWA_USE_IMMERSIVE_DARK_MODE=20 since Win10 20H1 */
static HRESULT DwmSetWindowAttributeL(HWND hwnd, DWORD a, PVOID b, DWORD c);
static BOOL AllowDarkTitlebar(HWND hwnd)
{
    if( IsHighContrastEnabled() )
        return FALSE; /* Nothing to do and ignore DarkMode */

    BOOL DarkMode = IsDarkModeEnabled();
    if ( OredredWinVer() >= 0x0A004A29 ) {
        /* Windows 10 build 10.0.18985 ie 20H1 */
        DwmSetWindowAttributeL(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &DarkMode, sizeof(DarkMode));
    } else if ( OredredWinVer() >= 0x0A004563) {
        /* Windows 10 build 10.0.17763 ie: 1809 or later */

        if ( OredredWinVer() < 0x0A0047BA) {
            /* Windows 10 build 10.0.18362 ie: before 1903 */
            SetProp(hwnd, TEXT("UseImmersiveDarkModeColors"), (HANDLE)(UINT_PTR)DarkMode);
        }
        DwmSetWindowAttributeL(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_PRE20H1, &DarkMode, sizeof(DarkMode));
    }
    /* We must return the actual state of the Dark mode */
    return DarkMode;
}

/* Helper function */
static int GetSystemMetricsForWin(int nIndex, HWND hwnd)
{
    return GetSystemMetricsForDpiL(nIndex, GetDpiForWindowL(hwnd));
}
static BOOL SystemParametersInfoForDpiL(UINT uiAction, UINT uiParam, PVOID pvParam, UINT  fWinIni, UINT dpi)
{
    typedef BOOL (WINAPI *funk_t)(UINT uiAction, UINT uiParam, PVOID pvParam, UINT  fWinIni, UINT dpi);
    static funk_t funk=(funk_t)IPTR;

    if (dpi) {
        if (funk == (funk_t)IPTR) { /* First time */
            funk = (funk_t)LoadDLLProc("USER32.DLL", "SystemParametersInfoForDpi");
        }
        if (funk) { /* We know we have the function */
            return funk(uiAction, uiParam, pvParam, fWinIni, dpi);
        }
    }
    /* Not handled Windows 10, version 1607 and below */
    return SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}
#define SystemParametersInfoForDpi SystemParametersInfoForDpiL

static HWINEVENTHOOK SetWinEventHookL(
      DWORD eventMin, DWORD eventMax
    , HMODULE hmodWinEventProc
    , WINEVENTPROC pfnWinEventProc
    , DWORD idProcess, DWORD idThread, DWORD dwFlags)
{
    typedef HWINEVENTHOOK (WINAPI *funk_t)(DWORD evm, DWORD evM, HMODULE hmod, WINEVENTPROC wevp, DWORD idProcess, DWORD idThread, DWORD dwFlags);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "SetWinEventHook");
    }
    if (funk) { /* We know we have the function */
        return funk(eventMin, eventMax, hmodWinEventProc
                    , pfnWinEventProc, idProcess, idThread, dwFlags);
    }
    /* Failed */
    return NULL;
}
static BOOL UnhookWinEventL(HWINEVENTHOOK hWinEventHook)
{
    typedef BOOL (WINAPI *funk_t)(HWINEVENTHOOK hWinEventHook);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "UnhookWinEvent");
    }
    if (funk) { /* We know we have the function */
        return funk(hWinEventHook);
    }
    /* Failed */
    return FALSE;
}

typedef struct tagRgTicTac {
    LARGE_INTEGER sta;
    LARGE_INTEGER end;
    LARGE_INTEGER frq;
} RGTICTAC;

static BOOL RGTic(RGTICTAC *tt)
{
    QueryPerformanceFrequency(&tt->frq);
    return QueryPerformanceCounter(&tt->sta);
}

static int RGTac(RGTICTAC *tt)
{
    if( QueryPerformanceCounter(&tt->end) ) {
        return MulDiv(tt->end.LowPart - tt->sta.LowPart, 1000000, tt->frq.LowPart);
    }
    return -1;
}

#ifndef _WIN64
static void NotifyWinEventL(DWORD event, HWND hwnd, LONG idObj, LONG idChild)
{
    typedef  void (WINAPI *funk_t)(DWORD, HWND, LONG, LONG);
    static funk_t funk = (funk_t)1;
    if (funk == (funk_t)1)
        funk = (funk_t)LoadDLLProc("USER32.DLL", "NotifyWinEvent");

    if (funk)
        funk(event, hwnd, idObj, idChild);
}
#define NotifyWinEvent NotifyWinEventL
#endif

static HWND HungWindowFromGhostWindowL(HWND gost)
{
    typedef HWND (WINAPI *funk_t)(HWND hWinEventHook);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("USER32.DLL", "HungWindowFromGhostWindow");
    }
    if (funk) { /* We know we have the function */
        return funk(gost);
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED /*120*/);
    /* Failed */
    return NULL;
}


static HRESULT DwmGetWindowAttributeL(HWND hwnd, DWORD a, PVOID b, DWORD c)
{
    typedef HRESULT (WINAPI *funk_t)(HWND hwnd, DWORD a, PVOID b, DWORD c);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("DWMAPI.DLL", "DwmGetWindowAttribute");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd, a, b, c);
    }
    /* DwmGetWindowAttribute return 0 on sucess ! */
    return 666; /* Here we FAIL with 666 error    */
}

static HRESULT DwmSetWindowAttributeL(HWND hwnd, DWORD a, PVOID b, DWORD c)
{
    typedef HRESULT (WINAPI *funk_t)(HWND hwnd, DWORD a, PVOID b, DWORD c);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("DWMAPI.DLL", "DwmSetWindowAttribute");
    }
    if (funk) { /* We know we have the function */
        return funk(hwnd, a, b, c);
    }
    /* myDwmSetWindowAttribute return 0 on sucess ! */
    return 666; /* Here we FAIL with 666 error    */
}

static HRESULT DwmGetColorizationColorL(DWORD *a, BOOL *b)
{
    typedef HRESULT (WINAPI *funk_t)(DWORD *a, BOOL *b);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("DWMAPI.DLL", "DwmGetColorizationColor");
    }
    if (funk) { /* We know we have the function */
        return funk(a, b);
    }
    /* return 0 on sucess ! */
    return 666; /* Here we FAIL with 666 error    */
}

static COLORREF GetSysColorizationColor()
{
    DWORD color=0;
    BOOL b=FALSE;
    if(S_OK == DwmGetColorizationColorL(&color, &b)) {
        /* Re orde- bytes because
         * COLORREF:  0x00BBGGRR and not 0xAARRGGBB */
        return  (color&0x000000FF) << 16  /* blue */
              | (color&0x0000FF00)        /* green */
              | (color&0x00FF0000) >> 16; /* red */
    }
    return 0;
}

static long xpure average(long a, long b)
{
    return (a>>1) + (b>>1) + ( ((a&1) + (b&1))>>1 );
}
static long xpure averageX(const RECT *rc)
{
    return average(rc->left, rc->right);
}
static long xpure averageY(const RECT *rc)
{
    return average(rc->top, rc->bottom);
}

/* #define DwmGetWindowAttribute DwmGetWindowAttributeL */

static void SubRect(RECT *__restrict__ frame, const RECT *rect)
{
    frame->left -= rect->left;
    frame->top  -= rect->top;
    frame->right = rect->right - frame->right;
    frame->bottom = rect->bottom - frame->bottom;
}
static void InflateRectBorder(RECT *__restrict__ rc, const RECT *bd)
{
    rc->left   -= bd->left;
    rc->top    -= bd->top;
    rc->right  += bd->right;
    rc->bottom += bd->bottom;
}

static void DeflateRectBorder(RECT *__restrict__ rc, const RECT *bd)
{
    rc->left   += bd->left;
    rc->top    += bd->top;
    rc->right  -= bd->right;
    rc->bottom -= bd->bottom;
}

static void OffsetPoints(POINT *pts, long dx, long dy, unsigned count)
{
    while(count--) {
        pts->x += dx;
        pts->y += dy;
        pts++;
    }
}

static void FixDWMRectLL(HWND hwnd, RECT *bbb, const int SnapGap)
{
    RECT rect, frame;

    if(S_OK == DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame))
       && GetWindowRect(hwnd, &rect)){
        SubRect(&frame, &rect);
        CopyRect(bbb, &frame);
    } else {
        SetRectEmpty(bbb);
        /*SetRect(bbb, 10, 10, 10, 10);*/
    }
    if (SnapGap) OffsetRect(bbb, -SnapGap, -SnapGap);
//    if (IsZoomed(hwnd)) OffsetRect(bbb, 10, 10); // Test for Zoomed stuff
}

/* This function is here because under Windows 10, the GetWindowRect function
 * includes invisible borders, if those borders were visible it would make
 * sense, this is the case on Windows 7 and 8.x for example
 * We use DWM api when available in order to get the REAL client area
 */
static BOOL GetWindowRectLL(HWND hwnd, RECT *rect, const int SnapGap)
{
    HRESULT ret = DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(*rect));
    if( ret == S_OK) {
        ret = TRUE;
    } else {
        ret = GetWindowRect(hwnd, rect); /* Fallback to normal */
    }
    if (SnapGap) InflateRect(rect, SnapGap, SnapGap);
    return ret;
}
/* Under Win8 and later a window can be cloaked
 * This falg can be obtained with this function
 * 1 The window was cloaked by its owner application.
 * 2 The window was cloaked by the Shell.
 * 4 The cloak value was inherited from its owner window.
 * For windows that are supposed to be logically "visible", in addition to WS_VISIBLE.
 * EDIT: Now Raymond Chen did an article about this:
 * https://devblogs.microsoft.com/oldnewthing/20200302-00/?p=103507
 * I had to figure it out the Hard way...
 */
static int IsWindowCloaked(HWND hwnd)
{
    int cloaked=0;
    return S_OK == DwmGetWindowAttributeL(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))
        && cloaked;
}
/* Some windows might have a size of 0x0 (e.g., a 'Setup' window in Windows 11
 * that appears to be visible despite having no size). It is assumed that this
 * is an issue specific to a system update window in Windows 11 (and maybe other
 * versions as well), although this has not been confirmed.
 */
static BOOL HasWindowSizeZero(HWND hwnd)
{
    RECT rc;
    return GetWindowRect(hwnd, &rc) && (rc.right == rc.left || rc.bottom == rc.top);
}
static BOOL IsVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd) && !IsWindowCloaked(hwnd) && !HasWindowSizeZero(hwnd);
}

/* Gets the original owner of hwnd.
 * stops going back the owner chain if invisible. */
static HWND GetRootOwner(HWND hwnd)
{
    HWND parent;
    int i=0;
    while (( parent = (GetWindowLongPtr(hwnd, GWL_STYLE)&WS_CHILD)
           ? GetParent(hwnd) : GetWindow(hwnd, GW_OWNER)
          )) {

        RECT prc;
        if (parent == hwnd || i++ > 2048 || !IsVisible(parent)
        || !GetWindowRect(parent, &prc) || IsRectEmpty(&prc)) {
            /* Stop if in a loop or if parent is not visible
             * or if the parent rect is empty */
            break;
        }
        hwnd = parent;
    }

    return hwnd;
}

/* Use the DWM api to obtain the rectangel that *should* contain all
 * caption buttons. This is usefull to ensure we are not in one of them.
 */
static BOOL GetCaptionButtonsRect(HWND hwnd, RECT *rc)
{
    int ret = DwmGetWindowAttributeL(hwnd, DWMWA_CAPTION_BUTTON_BOUNDS, rc, sizeof(*rc));
    /* Convert rectangle to to screen coordinate. */
    if (ret == S_OK) {
        RECT wrc;
        GetWindowRect(hwnd, &wrc);
        OffsetRect(rc, wrc.left, wrc.top);
        return 1;
    }
    return 0;
}

static LONG NtSuspendProcessL(HANDLE ProcessHandle)
{
    typedef HRESULT (NTAPI *funk_t)(HANDLE ProcessHandle);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("NTDLL.DLL", "NtSuspendProcess");
    }
    if (funk) { /* We know we have the function */
        return funk(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}

static LONG NtResumeProcessL(HANDLE ProcessHandle)
{
    typedef HRESULT (NTAPI *funk_t)(HANDLE ProcessHandle);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk = (funk_t)LoadDLLProc("NTDLL.DLL", "NtResumeProcess");
    }
    if (funk) { /* We know we have the function */
        return funk(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}

static HRESULT DwmIsCompositionEnabledL(BOOL *pfEnabled)
{
    typedef HRESULT (WINAPI *funk_t)(BOOL *pfEnabled);
    funk_t funk;

    HINSTANCE hdll=NULL;
    HRESULT ret ;

    *pfEnabled = FALSE;
    ret = 666;

    hdll = LoadLibraryA("DWMAPI.DLL");
    if (hdll) {
        funk = (funk_t)GetProcAddress(hdll, "DwmIsCompositionEnabled");
        if(funk) {
            ret = funk(pfEnabled);
        } else {
            *pfEnabled = FALSE;
            ret = 666;
        }
        FreeLibrary(hdll);
    }
    return ret;
}

static BOOL HaveDWM()
{
    static int first=1;
    static BOOL have_dwm = FALSE;
    if(first)
        DwmIsCompositionEnabledL(&have_dwm);
    first = 0;

    return have_dwm;
}

static BOOL DwmDefWindowProcL(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    typedef BOOL (WINAPI *funk_t)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
        funk= (funk_t)LoadDLLProc("DWMAPI.DLL", "DwmDefWindowProc");
    }
    if (funk) { /* We know we have the function */
        return funk(hWnd, msg, wParam, lParam, plResult);
    }
    return FALSE;
}

/* PSAPI.DLL */
static DWORD GetModuleFileNameExL(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
    typedef DWORD (WINAPI *funk_t)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
    #ifdef _UNICODE
        funk = (funk_t)LoadDLLProc("PSAPI.DLL", "GetModuleFileNameExW");
    #else
        funk = (funk_t)LoadDLLProc("PSAPI.DLL", "GetModuleFileNameExA");
    #endif
    }
    if (funk) { /* We have the function */
        return funk(hProcess, hModule, lpFilename, nSize);
    }
    return 0;
}

static DWORD GetProcessImageFileNameL(HANDLE hProcess, TCHAR *lpImageFileName, DWORD nSize)
{
    typedef DWORD (WINAPI *funk_t)(HANDLE hProcess, TCHAR *lpImageFileName, DWORD nSize);
    static funk_t funk=(funk_t)IPTR;

    if (funk == (funk_t)IPTR) { /* First time */
    #ifdef _UNICODE
        funk = (funk_t)LoadDLLProc("PSAPI.DLL", "GetProcessImageFileNameW");
    #else
        funk = (funk_t)LoadDLLProc("PSAPI.DLL", "GetProcessImageFileNameA");
    #endif
    }
    if (funk) {
        return funk(hProcess, lpImageFileName, nSize);
    }
    return 0;
}

static DWORD GetWindowProgNameFromPid(DWORD pid, TCHAR *title, size_t title_len)
{
    DWORD ret=0;
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);

    if (proc)
        ret = GetModuleFileNameExL(proc, NULL, title, title_len);
    else
        proc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if(!proc)
        return 0;

    /* Try other function */
    if (!ret) ret = GetProcessImageFileNameL(proc, title, title_len);

    CloseHandle(proc);

    return ret;
}

static DWORD GetWindowProgNameSubWindow(DWORD parentpid, HWND parenthwnd, TCHAR *title, size_t title_len);
static DWORD GetWindowProgName(HWND hwnd, TCHAR *title, size_t title_len)
{
    DWORD pid=0;
    if (!GetWindowThreadProcessId(hwnd, &pid)
    || !pid
    || !GetWindowProgNameFromPid(pid, title, title_len))
        return 0;

    PathStripPathL(title);

    if (pid && !lstrcmpi(title, TEXT("ApplicationFrameHost.exe"))) {
        DWORD pid2 = GetWindowProgNameSubWindow(pid, hwnd, title, title_len);
        if (pid2)  {
            PathStripPathL(title);
            return pid2;
        }
        /* If there is no child to ApplicationFrameHost.exe, */
        /* Then we keep going with that name */
    }

    return pid;
}

struct pid_pair_struct { DWORD parent_pid; DWORD child_pid; };
static BOOL CALLBACK GetWindowProgName_subwins_EnumProc(HWND hwnd, LPARAM lp)
{
    struct pid_pair_struct *pids = (struct pid_pair_struct*)lp;
    DWORD hwnd_pid = 0;
    if (GetWindowThreadProcessId(hwnd, &hwnd_pid) && hwnd_pid && hwnd_pid != pids->parent_pid) {
        // We found a child window with a pid different from its parent!
        pids->child_pid = hwnd_pid;
        return FALSE; // DONE!
    }
    return TRUE; // Next window
}

/* This is for UWP applications that are inside an ApplicationFrameHost.exe window
 * In this case it is possible to look for a child window that has a different pid
 * This is crazy I know but it seems to work. */
static DWORD GetWindowProgNameSubWindow(DWORD parentpid, HWND parenthwnd, TCHAR *title, size_t title_len)
{
    struct pid_pair_struct pids = { 0, 0 };
    pids.parent_pid = parentpid; // PID of ApplicationFrameHost or similar

    /* Look for a child window with a different PID !*/
    EnumChildWindows(parenthwnd, GetWindowProgName_subwins_EnumProc, (LPARAM)&pids);

    // if pids.child_pid it means we found a child window tht belongs to another process
    // hosted by ApplicationFrameHost.exe
    if (pids.child_pid && GetWindowProgNameFromPid(pids.child_pid, title, title_len))
        return pids.child_pid; // return relevent pid

    return 0;
}


/* Function to get the best possible small icon associated with a window
 * We always use SendMessageTimeout to ensure we do not get  locked
 * We start with ICON_SMALL, then ICON_SMALL2, then ICON_BIG and finally
 * we try to get the icon from the window class if everything failed or
 * if the first message timed out (freezing program).
 * Note that ICON_SMALL2 was introduced by Windows XP (I think) */
#ifndef ICON_SMALL2
#define ICON_SMALL2 2
#endif
#ifndef GCLP_HICON
#define GCLP_HICON (-14)
#endif
#ifndef GCLP_HICONSM
#define GCLP_HICONSM (-34)
#endif
static HICON GetWindowIcon(HWND hwnd)
{
    #define TIMEOUT 64
    HICON icon;
    if (SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, TIMEOUT, (DWORD_PTR*)&icon)) {
        /* The message failed without timeout */
        if (icon) return icon; /* Sucess */

        /* ICON_SMALL2 exists since Windows XP only */
        if (OredredWinVer() >= 0x05010000
        &&  SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, TIMEOUT, (DWORD_PTR*)&icon) && icon)
            return icon;

        /* Try again with the big icon if we were unable to retreave the small one. */
        if (SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, TIMEOUT, (DWORD_PTR*)&icon) && icon)
            return icon;
    }
    /* Try the Class icon if nothing can be get */
    if ((icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM))) return icon;
    if ((icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON))) return icon;

    return LoadIcon(NULL, IDI_WINLOGO); /* Default to generic window icon */
    #undef TIMEOUT
}
/* Helper function to get the current system menu font.
 * We need to use the newer NONCLIENTMETRICS for Window 8+ dpi aware
 * SystemParametersInfoForDpi() and We must ude the old NONCLIENTMETRICS
 * structure when using Windows XP or lower.
 * (vistal added the iPaddedBorderWidth int element */
struct OLDNONCLIENTMETRICSAW {
  UINT cbSize;
  int iBorderWidth;
  int iScrollWidth;
  int iScrollHeight;
  int iCaptionWidth;
  int iCaptionHeight;
  LOGFONT lfCaptionFont;
  int iSmCaptionWidth;
  int iSmCaptionHeight;
  LOGFONT lfSmCaptionFont;
  int iMenuWidth;
  int iMenuHeight;
  LOGFONT lfMenuFont;
  LOGFONT lfStatusFont;
  LOGFONT lfMessageFont;
};
struct NEWNONCLIENTMETRICSAW {
  UINT cbSize;
  int iBorderWidth;
  int iScrollWidth;
  int iScrollHeight;
  int iCaptionWidth;
  int iCaptionHeight;
  LOGFONT lfCaptionFont;
  int iSmCaptionWidth;
  int iSmCaptionHeight;
  LOGFONT lfSmCaptionFont;
  int iMenuWidth;
  int iMenuHeight;
  LOGFONT lfMenuFont;
  LOGFONT lfStatusFont;
  LOGFONT lfMessageFont;
  int iPaddedBorderWidth; /* New in Window Vista */
};
static BOOL GetNonClientMetricsDpi(struct NEWNONCLIENTMETRICSAW *ncm, UINT dpi)
{
    mem00(ncm, sizeof(struct NEWNONCLIENTMETRICSAW));
    ncm->cbSize = sizeof(struct NEWNONCLIENTMETRICSAW);
    BOOL ret = SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(struct NEWNONCLIENTMETRICSAW), ncm, 0, dpi);
    if (!ret) { /* Old Windows versions... XP and below */
        ncm->cbSize = sizeof(struct OLDNONCLIENTMETRICSAW);
        ret = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(struct OLDNONCLIENTMETRICSAW), ncm, 0);
    }
    return ret;
}
static HFONT CreateNCMenuFont(UINT dpi)
{
    struct NEWNONCLIENTMETRICSAW ncm;
    GetNonClientMetricsDpi(&ncm, dpi);
    return CreateFontIndirect(&ncm.lfMenuFont);
}

static void MaximizeWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}
static void RestoreWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
}
static void ToggleMaxRestore(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd)? SC_RESTORE: SC_MAXIMIZE, 0);
}
static void MinimizeWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}
/* Just changes the window Z-order */
static BOOL SetWindowLevel(HWND hwnd, HWND hafter)
{
    return SetWindowPos(hwnd, hafter, 0, 0, 0, 0
    , SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
}
static int HitTestTimeoutL(HWND hwnd, LPARAM lParam)
{
    DorQWORD area=0;

/*
    if(DwmDefWindowProcL(hwnd, WM_NCHITTEST, 0, lParam, (LRESULT*)&area))
        return area;
*/

    while(hwnd && SendMessageTimeout(hwnd, WM_NCHITTEST, 0, lParam, SMTO_NORMAL, 255, &area)){
        if((int)area == HTTRANSPARENT)
            hwnd = GetParent(hwnd);
        else
            break;
    }
    return (int)area;
}
#define HitTestTimeout(hwnd, x, y) HitTestTimeoutL(hwnd, MAKELPARAM(x, y))


/* This is used to detect is the window was snapped normally outside of
 * AltDrag, in this case the window appears as normal
 * ie: wndpl.showCmd=SW_SHOWNORMAL, but  its actual rect does not match with
 * its rcNormalPosition and if the WM_RESTORE command is sent, The window
 * will be restored. This is a non documented behaviour. */
static int IsWindowSnapped(HWND hwnd)
{
    RECT rect;
    int W, H, nW, nH;
    WINDOWPLACEMENT wndpl; wndpl.length = sizeof(wndpl);

    if(!GetWindowRect(hwnd, &rect)) return 0;
    W = rect.right  - rect.left;
    H = rect.bottom - rect.top;

    GetWindowPlacement(hwnd, &wndpl);
    nW = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    nH = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

    return wndpl.showCmd != SW_MAXIMIZE && (W != nW || H != nH);
}

/* Helper function to get the Min and Max tracking sizes */
static void GetMinMaxInfoF(HWND hwnd, POINT *Min, POINT *Max, UCHAR flags)
{
    MINMAXINFO mmi;
    mem00(&mmi, sizeof(mmi));
    UINT dpi = GetDpiForWindow(hwnd);
    mmi.ptMinTrackSize.x = GetSystemMetricsForDpi(SM_CXMINTRACK, dpi);
    mmi.ptMinTrackSize.y = GetSystemMetricsForDpi(SM_CYMINTRACK, dpi);
    mmi.ptMaxTrackSize.x = GetSystemMetricsForDpi(SM_CXMAXTRACK, dpi);
    mmi.ptMaxTrackSize.y = GetSystemMetricsForDpi(SM_CYMAXTRACK, dpi);
    *Min = mmi.ptMinTrackSize;
    *Max = mmi.ptMaxTrackSize;
    if ((flags&3) != 3) {
        DWORD_PTR ret; // 32ms timeout
        SendMessageTimeout(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi, SMTO_ABORTIFHUNG, 32, &ret);
        if(!(flags&1)) *Min = mmi.ptMinTrackSize;
        if(!(flags&2)) *Max = mmi.ptMaxTrackSize;
    }

}

static xpure int SamePt(const POINT a, const POINT b)
{
    return (a.x == b.x && a.y == b.y);
}

/* Limit x between l and h */
static xpure int CLAMP(int l, int x, int h)
{
    return (x < l)? l: ((x > h)? h: x);
}

/* Says if a rect is inside another one */
static pure BOOL RectInRect(const RECT *big, const RECT *wnd)
{
    return wnd->left >= big->left && wnd->top >= big->top
        && wnd->right <= big->right && wnd->bottom <= big->bottom;
}

static pure BOOL RectInRectT(const RECT *big, const RECT *wnd, const int T)
{
    return wnd->left+T >= big->left && wnd->top+T >= big->top
        && wnd->right <= big->right+T && wnd->bottom <= big->bottom+T;
}

static pure unsigned WhichSideRectInRect(const RECT *mon, const RECT *wnd)
{
    unsigned flag;
    flag  = ((wnd->left == mon->left) & (mon->right-wnd->right > 16)) << 2;
    flag |= ((wnd->right == mon->right) & (wnd->left-mon->left > 16)) << 3;
    flag |= ((wnd->top == mon->top) & (mon->bottom-wnd->bottom > 16)) << 4;
    flag |= ((wnd->bottom == mon->bottom) & (wnd->top-mon->top > 16)) << 5;

    return flag;
}

static xpure int IsEqualT(int a, int b, int th)
{
    return (b - th <= a) & (a <= b + th);
}
static int IsInRangeT(int x, int a, int b, int T)
{
    return (a-T <= x) & (x <= b+T);
}
static pure int EqualRectT(const RECT *a, const RECT *b, const int T)
{
    return IsEqualT(a->left, b->left, T) && IsEqualT(a->right, b->right, T)
        && IsEqualT(a->top, b->top, T) && IsEqualT(a->bottom, b->bottom, T);
}
static pure unsigned AreRectsAlignedOutT(const RECT *a, const RECT *b, const int tol)
{
    return IsEqualT(a->left, b->right, tol) << 2
         | IsEqualT(a->top, b->bottom, tol) << 4
         | IsEqualT(a->right, b->left, tol) << 3
         | IsEqualT(a->bottom, b->top, tol) << 5;
}
static pure unsigned AreRectsAlignedInT(const RECT *a, const RECT *b, const int tol)
{
    return IsEqualT(a->left, b->left, tol) << 2
         | IsEqualT(a->top, b->top, tol) << 4
         | IsEqualT(a->right, b->right, tol) << 3
         | IsEqualT(a->bottom, b->bottom, tol) << 5;
}
static pure unsigned AreRectsAligned2T(const RECT *a, const RECT *b, const int tol)
{
    return AreRectsAlignedOutT(a, b, tol) | (AreRectsAlignedInT(b, a, tol) << 16);
}


static int InRange(int x, int a, int b)
{
    return (x >= a) && (x <= b);
}
static xpure int SegT(long ax, long bx, const long *_ay12, const long *_by12, int tol)
{
    const long by1 = _by12[0]; /* left/top */
    const long by2 = _by12[2]; /* right/bottom */
    const long ay1 = _ay12[0]; /* left/top */
    const long ay2 = _ay12[2]; /* right/bottom */
    return IsEqualT(ax, bx, tol) /* ax == bx */
        && ( InRange(ay1, by1, by2)
          || InRange(by1, ay1, ay2)
          || InRange(ay2, by1, by2)
          || InRange(by2, ay1, ay2) );
}
/* Tells if rect b is touching rect a and on which side
 * 2^X, x=2 for Left, 3 for right, 4 for top and 5 for Bottom*/
static pure unsigned AreRectsTouchingT(const RECT *a, const RECT *b, const int tol)
{
    return SegT(a->left, b->right, &a->top, &b->top, tol) << 2 /* Left */
         | SegT(a->right, b->left, &a->top, &b->top, tol) << 3 /* Right */
         | SegT(a->top, b->bottom, &a->left, &b->left, tol) << 4 /* Top */
         | SegT(a->bottom, b->top, &a->left, &b->left, tol) << 5; /* Bottom */
}
//static pure unsigned AreRectsTouchingInT(const RECT *a, const RECT *b, const int tol)
//{
//    return SegT(a->left, b->left, &a->top, &b->bottom, tol) << 2 /* Left */
//         | SegT(a->right, b->right, &a->top, &b->bottom, tol) << 3 /* Right */
//         | SegT(a->top, b->top, &a->left, &b->right, tol) << 4 /* Top */
//         | SegT(a->bottom, b->bottom, &a->left, &b->right, tol) << 5; /* Bottom */
//}
//static pure unsigned AreRectsTouching2T(const RECT *a, const RECT *b, const int tol)
//{
//	return AreRectsTouchingT(a, b, tol) | AreRectsTouchingInT(a, b, tol) << 16;
//}
static void CropRect(RECT *__restrict__ wnd, const RECT *crop)
{
    wnd->left   = max(wnd->left,   crop->left);
    wnd->top    = max(wnd->top,    crop->top);
    wnd->right  = min(wnd->right,  crop->right);
    wnd->bottom = min(wnd->bottom, crop->bottom);
}

static void CenterRectInRect(RECT *__restrict__ wnd, const RECT *mon)
{
    int width  = wnd->right  - wnd->left;
    int height = wnd->bottom - wnd->top;
    wnd->left = mon->left + (mon->right-mon->left)/2-width/2;
    wnd->top  = mon->top  + (mon->bottom-mon->top)/2-height/2;
    wnd->right  = wnd->left + width;
    wnd->bottom = wnd->top  + height;
}

static void ClampPointInRect(const RECT *rc, POINT *__restrict__ pt)
{
    pt->x = CLAMP(rc->left, pt->x, rc->right-1);
    pt->y = CLAMP(rc->top, pt->y, rc->bottom-1);
}
static void RectFromPts(RECT *rc, const POINT a, const POINT b)
{
    rc->left = min(a.x, b.x);
    rc->top = min(a.y, b.y);
    rc->right = max(a.x, b.x);
    rc->bottom= max(a.y, b.y);
}

/* DownlevelLCIDToLocaleName in NLSDL.DLL */
/* LCIDToLocaleName  in KERNEL32.DLL*/
#ifdef _UNICODE
static int LCIDToLocaleNameL(LCID Locale, LPWSTR lpName, int cchName, DWORD dwFlags)
{
    typedef int (WINAPI *funk_t)(LCID Locale, LPWSTR  lpName, int cchName, DWORD dwFlags);
    funk_t funk = (funk_t)GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "LCIDToLocaleName");

    if (funk) { /* Function in KERNEL32.DLL */
        return funk(Locale, lpName, cchName, dwFlags);
    }
    /* Unable to find KERNEL32.DLL::LCIDToLocaleName
     * Try with NLSDL.DLL::DownlevelLCIDToLocaleName */
    HINSTANCE h = LoadLibraryA("NLSDL.DLL");
    if (h) {
        int ret=0;
        funk = (funk_t)GetProcAddress(h, "DownlevelLCIDToLocaleName");
        if (funk)
            ret = funk(Locale, lpName, cchName, dwFlags);
        FreeLibrary(h);
        return ret;
    }
    return 0;
}
#endif // UNICODE

#if 0
/* Get the string inside the section returned by GetPrivateProfileSection */
static void GetSectionOptionStr(const TCHAR *section, const char * const oname, const TCHAR *def, TCHAR * __restrict__ txt, size_t txtlen)
{
    if (section) {
        TCHAR name[128];
        str2tchar_s(name, ARR_SZ(name)-1, oname);
        lstrcat_s(name, ARR_SZ(name), TEXT("=")); /* Add equal at the end of name */
        const TCHAR *p = section;
        while (p[0] && p[1]) { /* Double NULL treminated string */
            if(!lstrcmpi_samestart(p, name)) {
                /* Copy the buffer */
                lstrcpy_s(txt, txtlen, p+lstrlen(name));
                return; /* DONE! */
            } else {
                /* Go to next string... */
                p += lstrlen(p); /* p in on the '\0' */
                p++; /* next string start. */
                if (!*p) break;
            }
        }
    }
    /* Default to the provided def string */
    if (def)
        lstrcpy_s(txt, txtlen, def);
    else if (txtlen)
        txt[0] = TEXT('\0');  // Empty string
}
#endif
static const TCHAR* GetSectionOptionCStr(const TCHAR *section, const char * const oname, const TCHAR *const def)
{
    TCHAR name[128];
    str2tchar_s(name, ARR_SZ(name)-1, oname);
    lstrcat_s(name, ARR_SZ(name), TEXT("=")); /* Add equal at the end of name */
    const TCHAR *p = section;
    while (p[0] && p[1]) { /* Double NULL treminated string */
        if(!lstrcmpi_samestart(p, name)) {
            return p+lstrlen(name); /* DONE! */
        } else {
            /* Go to next string... */
            p += lstrlen(p); /* p in on the '\0' */
            p++; /* next string start. */
            if (!*p) break;
        }
    }
    /* Default to the provided def string */
    return def;
}
/* Get the int inside the section returned by GetPrivateProfileSection */
static int GetSectionOptionInt(const TCHAR *section, const char * const oname, const int def)
{
    if (section) {
        TCHAR name[128];
        str2tchar_s(name, ARR_SZ(name)-1, oname);
        lstrcat_s(name, ARR_SZ(name), TEXT("=")); /* Add equal at the end of name */
        const TCHAR *p = section;
        while (p[0] && p[1]) { /* Double NULL treminated string */
            if(!lstrcmpi_samestart(p, name)) {
                /* DONE !*/
                return strtoi(p+lstrlen(name));
            } else {
                /* Go to next string... */
                p += lstrlen(p); /* p in on the '\0' */
                p++; /* next string start. */
                if (!*p) break;
            }
        }
    }
    /* Default to the provided def value */
    return def;
}

#endif
