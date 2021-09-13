#include "hooks.h"

RECT Zones[10];
unsigned nzones;

static void SaveZone(RECT *rc, unsigned nzones, wchar_t *inipath)
{
}

BOOL CALLBACK EnumAndSaveTestWindows(HWND hwnd, LPARAM lParam)
{
    wchar_t classn[256];
    RECT rc;
    if (IsVisible(hwnd) 
    && GetClassName(hwnd, classn, sizeof(classn))
    && wcscmp(classn, L"AltSnap-Test*")
    && GetWindowRect(hwnd, &rc)) {
        SaveZone(&rc, nzones, inipath);
    }
    return TRUE;
}

static void SaveCurrentLayout()
{
    nzones = 0;
}

static int ReadRectFromini(RECT *zone, unsigned idx, wchar_t *inipath)
{
     if (idx > 1024) return 0;
     long *ZONE = (long *)zone;
     wchar_t zaschii[256], zname[32]=L"", txt[8];
     wcscat(zname, L"Zone");
     wcscat(zname, _itow(idx, txt, 10)); // Zone Name from zone number

     DWORD ret = GetPrivateProfileString(L"Zones", zname, L"", zaschii, ARR_SZ(zaschii), inipath);

     if (!ret || zaschii[0] =='\0') return 0;

     wchar_t *oldptr, *newptr;
     oldptr = &zaschii[0];
     UCHAR i;
     for (i=0; i < 4; i++) {
         newptr = wcschr(oldptr, ',');
         if (!newptr) return 0;
         *newptr = '\0'; 
         newptr++; 
         if(!*oldptr) return 0;
         ZONE[i] = _wtoi(oldptr); // 0 left, 1 top, 2 right, 3 bottom;
         oldptr = newptr;
     }
     return 1;
}
static void ReadZones(wchar_t *inifile)
{
    nzones = 0;
    while (ReadRectFromini(&Zones[nzones], nzones, inifile)) {
        nzones++;
    }
}

static int GetZoneFromPoint(POINT pt, RECT *urc)
{
    unsigned i, j=0;
    SetRectEmpty(urc);
    for (i=0; i < nzones; i++) {
        InflateRect(&Zones[i], 32, 32);
        int inrect = PtInRect(&Zones[i], pt);
        InflateRect(&Zones[i], -32, -32);
        if (inrect) {
            UnionRect(urc, urc, &Zones[i]);
            j++;
        }
    }
    return j;
}


static void MoveSnapToZone(POINT pt, int *posx, int *posy, int *width, int *height)
{
     if (!state.shift) return;

     RECT rc;
     int ret = GetZoneFromPoint(pt, &rc);
     if(!ret) return;
     LastWin.end = 0;

     SetRestoreData(state.hwnd, state.origin.width, state.origin.height, SNAPPED|SNZONE);
     *posx = rc.left;
     *posy = rc.top;
     *width = rc.right - rc.left;
     *height = rc.bottom - rc.top;
}
