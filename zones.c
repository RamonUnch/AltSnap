#include "hooks.h"

RECT Zones[32];
unsigned nzones;

static int ReadRectFromini(RECT *zone, unsigned idx, wchar_t *inipath)
{
     if (idx > 32) return 0;
     long *ZONE = (long *)zone;
     wchar_t zaschii[128], zname[32]=L"";

     DWORD ret = GetPrivateProfileString(L"Zones", ZidxToZonestr(idx, zname), L"", zaschii, ARR_SZ(zaschii), inipath);

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
//        LOG("Zone%d = %d, %d, %d, %d\n", nzones, Zones[0],Zones[1],Zones[2],Zones[3]);
        nzones++;
    }
}

static unsigned GetZoneFromPoint(POINT pt, RECT *urc)
{
    unsigned i, j=0, flag=0;
    SetRectEmpty(urc);
    int iz = conf.InterZone;
    for (i=0; i < nzones; i++) {
        if (iz) InflateRect(&Zones[i], iz, iz);

        int inrect=0;
        inrect = PtInRect(&Zones[i], pt);
        if (state.ctrl && !inrect) inrect = PtInRect(&Zones[i], state.ctrlpt);

        if (iz) InflateRect(&Zones[i], -iz, -iz);
        if (inrect) {
            UnionRect(urc, urc, &Zones[i]);
            flag |= 1 << j++ ;
        }
    }
    return j;
}
static int pure IsResizable(HWND hwnd);

static void MoveSnapToZone(POINT pt, int *posx, int *posy, int *width, int *height)
{
     if (!conf.UseZones || !state.shift|| state.mdiclient || !IsResizable(state.hwnd))
         return;

     RECT rc, bd;
     unsigned ret = GetZoneFromPoint(pt, &rc);
     if (!ret) return; // Outside of a rect

     LastWin.end = 0;
     FixDWMRect(state.hwnd, &bd);
     InflateRectBorder(&rc, &bd);

     SetRestoreData(state.hwnd, state.origin.width, state.origin.height, SNAPPED|SNZONE);
     *posx = rc.left;
     *posy = rc.top;
     *width = rc.right - rc.left;
     *height = rc.bottom - rc.top;
}
