/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Compatibility layer for old NT OSes                                   *
 * Written by Raymond Gillibert in 2021                                  *
 * THIS FILE IS NOT UNDER GPL but under the WTFPL.                       *
 * DO WHAT THE FUCK YOU WANT WITH THIS CODE!                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _UNFUCK_NT_
#define _UNFUCK_NT_

#include <windows.h>
#include <stdio.h>
#include <dwmapi.h>
#include "nanolibc.h"

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

#define LOG(X, ...) { FILE *LOG=fopen("ad.log", "a"); fprintf(LOG, X, ##__VA_ARGS__); fclose(LOG); }

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
    static RECT TaskbarRC, DesktopRC;
    if (!saved) {
        GetClientRect(GetDesktopWindow(), &DesktopRC);
        GetClientRect(FindWindow(L"Shell_TrayWnd", L""), &TaskbarRC);
        saved=1;
    }
    lpmi->rcWork = lpmi->rcMonitor = DesktopRC;
    lpmi->rcWork.bottom -= TaskbarRC.bottom - TaskbarRC.top + 6;
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

static void FixDWMRect(HWND hwnd, int *posx, int *posy, int *wndwidth, int *wndheight, RECT *bbb)
{
    RECT rect, frame;

    if(S_OK == DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT))
       && GetWindowRect(hwnd, &rect)){
        RECT border;
        border.left = frame.left - rect.left;
        border.top = frame.top - rect.top;
        border.right = rect.right - frame.right;
        border.bottom = rect.bottom - frame.bottom;
        if(bbb)  *bbb = border;
        if(wndwidth) {
            *posx -= border.left;
            *posy -= border.top;
            *wndwidth += border.left + border.right;
            *wndheight += border.top + border.bottom;
        }
        return;
    }
    if(bbb) bbb->top = bbb->bottom = bbb->left = bbb->right = 0;
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

#endif
