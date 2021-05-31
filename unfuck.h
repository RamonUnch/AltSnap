/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Compatibility layer for old NT OSes                                   *
 * Written by Raymond Gillibert in 2021                                  *
 * THIS FILE IS NOT UNDER GPL but under the WTFPL.                       *
 * DO WHAT THE FUCK YOU WANT WITH THIS CODE!                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _UNFUCK_NT_
#define _UNFUCK_NT_

#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include "nanolibc.h"

#ifdef WIN64
#define DorQWORD unsigned long long
#else
#define DorQWORD unsigned long
#endif

#define ARR_SZ(x) (sizeof(x) / sizeof((x)[0]))
#define IDAPPLY 0x3021

#ifndef NIIF_USER
#define NIIF_USER 0x00000004
#endif

#define flatten __attribute__((flatten))
#define xpure __attribute__((const))
#define pure __attribute__((pure))
#define noreturn __attribute__((noreturn))

#ifndef SUBCLASSPROC
typedef LRESULT (CALLBACK *SUBCLASSPROC)
    (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
    , UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
#endif
#define LOG_STUFF 0
#define LOG(X, ...) if(LOG_STUFF) { FILE *LOG=fopen("ad.log", "a"); fprintf(LOG, X, ##__VA_ARGS__); fclose(LOG); }
#define LOGA(X, ...) { FILE *LOG=fopen("ad.log", "a"); fprintf(LOG, X, ##__VA_ARGS__); fclose(LOG); }

/* Stuff missing in MinGW */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
CLSID my_CLSID_MMDeviceEnumerator= {0xBCDE0395,0xE52F,0x467C,{0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E}};
GUID  my_IID_IMMDeviceEnumerator = {0xA95664D2,0x9614,0x4F35,{0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6}};
GUID  my_IID_IAudioEndpointVolume= {0x5CDF2C82,0x841E,0x4546,{0x97,0x22,0x0C,0xF7,0x40,0x78,0x22,0x9A}};

/* COMDLG32.DLL */
static LRESULT (WINAPI *myDefSubclassProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL    (WINAPI *myRemoveWindowSubclass)(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass);
static BOOL    (WINAPI *mySetWindowSubclass)(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

/* USER32.DLL */
static BOOL (WINAPI *mySetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
static BOOL (WINAPI *myGetLayeredWindowAttributes)(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags);
static HWND (WINAPI *myGetAncestor)(HWND hwnd, UINT gaFlags);
static BOOL (WINAPI *myEnumDisplayMonitors)(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);
static BOOL (WINAPI *myGetMonitorInfoW)(HMONITOR hMonitor, LPMONITORINFO lpmi);
static HMONITOR (WINAPI *myMonitorFromPoint)(POINT pt, DWORD dwFlags);
static HMONITOR (WINAPI *myMonitorFromWindow)(HWND hwnd, DWORD dwFlags);

/* DWMAPI.DLL */
static HRESULT (WINAPI *myDwmGetWindowAttribute)(HWND hwnd, DWORD a, PVOID b, DWORD c);
static HRESULT (WINAPI *myDwmIsCompositionEnabled)(BOOL *pfEnabled);

/* NTDLL.DLL */
static LONG (NTAPI *myNtSuspendProcess)(HANDLE ProcessHandle);
static LONG (NTAPI *myNtResumeProcess )(HANDLE ProcessHandle);

/* OLE32.DLL */
HRESULT (WINAPI *myCoInitialize)(LPVOID pvReserved);
VOID (WINAPI *myCoUninitialize)( );
HRESULT (WINAPI *myCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);

/* WINMM.DLL */
MMRESULT (WINAPI *mywaveOutGetVolume)(HWAVEOUT hwo, LPDWORD pdwVolume);
MMRESULT (WINAPI *mywaveOutSetVolume)(HWAVEOUT hwo, DWORD dwVolume);

#define HAVE_FUNC -1

#define VISTA (WinVer >= 6)
#define WIN10 (WinVer >= 10)

static BOOL PathRemoveFileSpecL(LPTSTR p)
{
    if (!p) return FALSE;
    int i=0;

    while(p[++i] != '\0');
    while(i > 0 && p[i] != '\\') i--;
    p[i]='\0';

    return TRUE;
}

static void PathStripPathL(LPTSTR p)
{
    int i=0, j;
    if (!p) return;

    while(p[++i] != '\0');
    while(i > 0 && p[i] != '\\') i--;
    i++;
    for(j=0; p[i+j] != '\0'; j++) p[j]=p[i+j];
    p[j]= '\0';
}

static HWND GetAncestorL(HWND hwnd, UINT gaFlags)
{
    static char have_func=HAVE_FUNC;
    HWND hlast, hprevious;
    if(!hwnd) return NULL;
    hprevious = hwnd;
    LONG wlong;

    switch(have_func){
    case -1: /* First time */
        myGetAncestor=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetAncestor");
        if(!myGetAncestor) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myGetAncestor(hwnd, gaFlags);
    }

    while ( (hlast = GetParent(hprevious)) != NULL ){
        wlong=GetWindowLong(hprevious, GWL_STYLE);
        if(wlong&(WS_POPUP)) break;
        hprevious=hlast;
    }
    return hprevious;
}
#define GetAncestor GetAncestorL

static BOOL GetLayeredWindowAttributesL(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myGetLayeredWindowAttributes=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetLayeredWindowAttributes");
        if(!myGetLayeredWindowAttributes) {
            have_func=0;
            return FALSE;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myGetLayeredWindowAttributes(hwnd, pcrKey, pbAlpha, pdwFlags);
    }
    return FALSE;
}
#define GetLayeredWindowAttributes GetLayeredWindowAttributesL

static BOOL SetLayeredWindowAttributesL(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        mySetLayeredWindowAttributes=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SetLayeredWindowAttributes");
        if(!mySetLayeredWindowAttributes) {
            have_func=0;
            return FALSE;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return mySetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
    }
    return FALSE;
}
#define SetLayeredWindowAttributes SetLayeredWindowAttributesL


static LRESULT DefSubclassProcL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myDefSubclassProc=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "DefSubclassProc");
        if(!myDefSubclassProc) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myDefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
#define DefSubclassProc DefSubclassProcL

static BOOL RemoveWindowSubclassL(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myRemoveWindowSubclass=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "RemoveWindowSubclass");
        if(!myRemoveWindowSubclass) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myRemoveWindowSubclass(hWnd, pfnSubclass, uIdSubclass);
    }
    return FALSE;
}
#define RemoveWindowSubclass RemoveWindowSubclassL

static BOOL SetWindowSubclassL(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        mySetWindowSubclass=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SetWindowSubclass");
        if(!mySetWindowSubclass) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return mySetWindowSubclass(hWnd, pfnSubclass, uIdSubclass, dwRefData);
    }
    return FALSE;
}
#define SetWindowSubclass SetWindowSubclassL

static BOOL GetMonitorInfoL(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myGetMonitorInfoW=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetMonitorInfoW");
        if(!myGetMonitorInfoW) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        if(hMonitor) return myGetMonitorInfoW(hMonitor, lpmi);
    }
    static int saved=0;
    static RECT rcWork, rcMonitor;
    if (!saved) {
        GetClientRect(GetDesktopWindow(), &rcMonitor);
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
        saved=1;
    }
    CopyRect(&lpmi->rcMonitor, &rcMonitor);
    CopyRect(&lpmi->rcWork,    &rcWork);
    lpmi->dwFlags = MONITORINFOF_PRIMARY;

    return TRUE;
}
#undef GetMonitorInfo
#define GetMonitorInfo GetMonitorInfoL

static BOOL EnumDisplayMonitorsL(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myEnumDisplayMonitors=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "EnumDisplayMonitors");
        if(!myEnumDisplayMonitors) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myEnumDisplayMonitors(hdc, lprcClip, lpfnEnum, dwData);
    }
    MONITORINFO mi;
    GetMonitorInfoL(NULL, &mi);

    lpfnEnum(NULL, NULL, &mi.rcMonitor, 0); // Callback function

    return TRUE;
}
#define EnumDisplayMonitors EnumDisplayMonitorsL

static HMONITOR MonitorFromPointL(POINT pt, DWORD dwFlags)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myMonitorFromPoint=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "MonitorFromPoint");
        if(!myMonitorFromPoint) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myMonitorFromPoint(pt, dwFlags);
    }
    return NULL;
}
#define MonitorFromPoint MonitorFromPointL


static HMONITOR MonitorFromWindowL(HWND hwnd, DWORD dwFlags)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myMonitorFromWindow=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "MonitorFromWindow");
        if(!myMonitorFromWindow) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myMonitorFromWindow(hwnd, dwFlags);
    }
    return NULL;
}
#define MonitorFromWindow MonitorFromWindowL

static HRESULT DwmGetWindowAttributeL(HWND hwnd, DWORD a, PVOID b, DWORD c)
{
    HINSTANCE hdll=NULL;
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        hdll = LoadLibraryA("DWMAPI.DLL");
        if(!hdll) {
                have_func = 0;
                break;
        } else {
            myDwmGetWindowAttribute=(void *)GetProcAddress(hdll, "DwmGetWindowAttribute");
            if(myDwmGetWindowAttribute){
                have_func = 1;
            } else {
                FreeLibrary(hdll);
                have_func = 0;
                break;
            }
        }
    case 1: /* We know we have the function */
        return myDwmGetWindowAttribute(hwnd, a, b, c);
    }
    /* DwmGetWindowAttribute return 0 on sucess ! */
    return 666; /* Here we FAIL with 666 error    */
}
/* #define DwmGetWindowAttribute DwmGetWindowAttributeL */

static void SubRect(RECT *frame, const RECT *rect)
{
    frame->left -= rect->left;
    frame->top -= rect->top;
    frame->right = rect->right - frame->right;
    frame->bottom = rect->bottom - frame->bottom;
}
static void InflateRectBorder(RECT *rc, const RECT *bd)
{
    rc->left   -= bd->left;
    rc->top    -= bd->top;
    rc->right  += bd->right;
    rc->bottom += bd->bottom;
}
static void DeflateRectBorder(RECT *rc, const RECT *bd)
{
    rc->left   += bd->left;
    rc->top    += bd->top;
    rc->right  -= bd->right;
    rc->bottom -= bd->bottom;
}
static void FixDWMRect(HWND hwnd, RECT *bbb)
{
    RECT rect, frame;

    if(S_OK == DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT))
       && GetWindowRect(hwnd, &rect)){
        SubRect(&frame, &rect);
        CopyRect(bbb, &frame);

        return;
    }
    // bbb->left = bbb->right = bbb->top = bbb->bottom = 0;
    SetRectEmpty(bbb);
}

/* This function is here because under Windows 10, the GetWindowRect function
 * includes invisible borders, if those borders were visible it would make
 * sense, this is the case on Windows 7 and 8.x for example
 * We use DWM api when available in order to get the REAL client area
 */
static BOOL GetWindowRectL(HWND hwnd, RECT *rect)
{
    HRESULT ret = DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(RECT));
    if( ret == S_OK) return 1;
    else return GetWindowRect(hwnd, rect); /* Fallback to normal */
}

static LONG NtSuspendProcessL(HANDLE ProcessHandle)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myNtSuspendProcess=(void *)GetProcAddress(GetModuleHandleA("NTDLL.DLL"), "NtSuspendProcess");
        if(myNtSuspendProcess){
            have_func = 1;
        } else {
            have_func = 0;
            break;
        }
    case 1: /* We know we have the function */
        return myNtSuspendProcess(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}
#define NtSuspendProcess NtSuspendProcessL

static LONG NtResumeProcessL(HANDLE ProcessHandle)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myNtResumeProcess=(void *)GetProcAddress(GetModuleHandleA("NTDLL.DLL"), "NtResumeProcess");
        if(myNtResumeProcess){
            have_func = 1;
        } else {
            have_func = 0;
            break;
        }
    case 1: /* We know we have the function */
        return myNtResumeProcess(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}
#define NtResumeProcess NtResumeProcessL

static HRESULT DwmIsCompositionEnabledL(BOOL *pfEnabled)
{
    HINSTANCE hdll=NULL;
    HRESULT ret ;

    *pfEnabled = FALSE;
    ret = 666;

    hdll = LoadLibraryA("DWMAPI.DLL");
    if(hdll) {
        myDwmIsCompositionEnabled = (void *)GetProcAddress(hdll, "DwmIsCompositionEnabled");
        if(myDwmIsCompositionEnabled) {
            ret = myDwmIsCompositionEnabled(pfEnabled);
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

static BOOL HaveProc(char *DLLname, char *PROCname)
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

/* PSAPI.DLL */
static DWORD (WINAPI *myGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
static DWORD GetModuleFileNameExL(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
    HINSTANCE hPSAPIdll=NULL;
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1:
        hPSAPIdll = LoadLibraryA("PSAPI.DLL");
        if(!hPSAPIdll) {
            have_func = 0;
            break;
        } else {
            myGetModuleFileNameEx=(void *)GetProcAddress(hPSAPIdll, "GetModuleFileNameExW");
            if(myGetModuleFileNameEx){
                have_func = 1;
            } else {
                FreeLibrary(hPSAPIdll);
                have_func = 0;
                break;
            }
        }
    case 1:
        return myGetModuleFileNameEx(hProcess, hModule, lpFilename, nSize);
    }
    return 0;
}
static DWORD GetWindowProgName(HWND hwnd, wchar_t *title, size_t title_len)
{
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
    int ret = GetModuleFileNameExL(proc, NULL, title, title_len);
    CloseHandle(proc);

    PathStripPathL(title);
    return ret? pid: 0;
}

/* Helper function to get the Min and Max tracking sizes */
static void GetMinMaxInfo(HWND hwnd, POINT *Min, POINT *Max)
{
    MINMAXINFO mmi = { {0, 0}, {0, 0}, {0, 0}
                     , {GetSystemMetrics(SM_CXMINTRACK), GetSystemMetrics(SM_CYMINTRACK)}
                     , {GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CXMAXTRACK)} };
    SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
    *Min = mmi.ptMinTrackSize;
    *Max = mmi.ptMaxTrackSize;
}

/* Helper function to call SetWindowPos with the SWP_ASYNCWINDOWPOS flag */
static BOOL MoveWindowAsync(HWND hwnd, int posx, int posy, int width, int height, BOOL flag)
{
    flag = (!flag) * SWP_NOREDRAW;
    return SetWindowPos(hwnd, NULL, posx, posy, width, height
                      , SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_ASYNCWINDOWPOS|flag);
}
/* This is used to detect is the window was snapped normally outside of
 * AltDrag, in this case the window appears as normal
 * ie: wndpl.showCmd=SW_SHOWNORMAL, but  its actual rect does not match with
 * its rcNormalPosition and if the WM_RESTORE command is sent, The window
 * will be restored. This is a non documented behaviour. */
static int IsWindowSnapped(HWND hwnd)
{
    RECT rect;
    if(!GetWindowRect(hwnd, &rect)) return 0;
    int W = rect.right  - rect.left;
    int H = rect.bottom - rect.top;

    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(hwnd, &wndpl);
    int nW = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    int nH = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

    return (W != nW || H != nH);
}

/* If pt and ptt are it is the same points with 4px tolerence */
static xpure int IsSamePTT(const POINT *pt, const POINT *ptt)
{
    return !( pt->x > ptt->x+4 || pt->y > ptt->y+4 ||pt->x < ptt->x-4 || pt->y < ptt->y-4 );
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

static pure unsigned WhichSideRectInRect(const RECT *mon, const RECT *wnd)
{
    unsigned flag;
    flag  = (wnd->left == mon->left && mon->right-wnd->right > 16) << 2;
    flag |= (wnd->right == mon->right && wnd->left-mon->left > 16) << 3;
    flag |= (wnd->top == mon->top && mon->bottom-wnd->bottom > 16) << 4;
    flag |= (wnd->bottom == mon->bottom && wnd->top-mon->top > 16) << 5;

    return flag;
}
static pure unsigned AreRectsAligned(const RECT *a, const RECT *b, const int tol)
{
    return (a->left <= b->right + tol && a->left >= b->right - tol) << 2
         | (a->top <= b->bottom + tol && a->top >= b->bottom - tol) << 4
         | (a->right <= b->left + tol && a->right >= b->left - tol) << 3
         | (a->bottom <= b->top + tol && a->bottom >= b->top - tol) << 5;
}
static xpure int IsEqualT(int a, int b, int th)
{ 
    return b - th < a && a < b + th ;
}
static xpure int IsInRangeT(int x, int a, int b, int T)
{
    return a-T < x && x < b+T ;
}
static xpure int SegT(int ax, int bx, int ay1, int ay2, int by1, int by2, int tol)
{
    return (ax <= bx + tol && ax >= bx - tol) /* ax == bx */
        && ( (ay1 >= by1 && ay1 <= by2)
          || (by1 >= ay1 && by1 <= ay2)
          || (ay2 >= by1 && ay2 <= by2)
          || (by2 >= ay1 && by2 <= ay2) );
}
static pure unsigned AreRectsTouchingT(const RECT *a, const RECT *b, const int tol)
{
    return SegT(a->left, b->right, a->top, a->bottom, b->top, b->bottom, tol) << 2
         | SegT(a->right, b->left, a->top, a->bottom, b->top, b->bottom, tol) << 3
         | SegT(a->top, b->bottom, a->left, a->right, b->left, b->right, tol) << 4
         | SegT(a->bottom, b->top, a->left, a->right, b->left, b->right, tol) << 5;
}

static void CropRect(RECT *__restrict__ wnd, RECT *crop)
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

#endif
