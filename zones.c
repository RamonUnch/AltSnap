/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * (C) Raymond GILLIBERT                                                 *
 * Functions to handle Snap Layouts under AltSnap                        *
 * General Public License Version 3 or later (Free Software Foundation)  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"

#define MAX_ZONES 2048
RECT *Zones;
unsigned nzones;

static int ReadRectFromini(RECT *zone, unsigned idx, wchar_t *inipath)
{
    if (idx > MAX_ZONES) return 0;

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
// Load all zones from ini file
static void ReadZones(wchar_t *inifile)
{
    nzones = 0;
    RECT tmpzone;
    while (ReadRectFromini(&tmpzone, nzones, inifile)) {
        Zones = realloc( Zones, (nzones+1) * sizeof(RECT) );
        CopyRect(&Zones[nzones++], &tmpzone);
    }
}
// Generate a grid if asked
static void GenerateGridZones(unsigned Nx, unsigned Ny)
{
    // Enumerate monitors
    nummonitors = 0;
	nzones =0;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, 0);
	Zones = realloc(Zones, nummonitors * Nx * Ny * sizeof(RECT));
	if(!Zones) return;

    // Loop on all monitors
    unsigned m;
    for (m=0; m<nummonitors; m++) {
        RECT *mon = &monitors[m];
        unsigned i;
        for(i=0; i<Nx; i++) { // Horizontal
            unsigned j;
            for(j=0; j<Ny; j++) { //Vertical
                Zones[nzones].left  = mon->left+(( i ) * (mon->right - mon->left))/Nx;
                Zones[nzones].top   = mon->top +(( j ) * (mon->bottom - mon->top))/Ny;
                Zones[nzones].right = mon->left+((i+1) * (mon->right - mon->left))/Nx;
                Zones[nzones].bottom= mon->top +((j+1) * (mon->bottom - mon->top))/Ny;
                nzones++;
            }
        }
    }
}
static unsigned GetZoneFromPoint(POINT pt, RECT *urc)
{
    unsigned i, ret=0;
    SetRectEmpty(urc);
    int iz = conf.InterZone;
    for (i=0; i < nzones; i++) {
        if (iz) InflateRect(&Zones[i], iz, iz);

        int inrect=0;
        inrect = PtInRect(&Zones[i], pt);
        if ((state.ctrl||conf.UseZones&4) && !inrect)
            inrect = PtInRect(&Zones[i], conf.UseZones&4?state.shiftpt:state.ctrlpt);

        if (iz) InflateRect(&Zones[i], -iz, -iz);
        if (inrect) {
            UnionRect(urc, urc, &Zones[i]);
            ret++;
        }
    }
    return ret;
}
static int pure IsResizable(HWND hwnd);

static void MoveSnapToZone(POINT pt, int *posx, int *posy, int *width, int *height)
{
    if(!(conf.UseZones&1) || state.mdiclient || !state.resizable) // Zones disabled
        return;

    static POINT oldpt;
    if(state.Speed > conf.AeroMaxSpeed)
        pt=oldpt; // do not move the window if speed is too high.
    else
        oldpt=pt;

    if (conf.UseZones&8 && state.snap != conf.AutoSnap) // Zones toggled by other click
        return;

    if (!state.usezones)
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

static void SetOriginFromRestoreData(HWND hnwd, enum action action);
static HMONITOR GetMonitorInfoFromWin(HWND hwnd, MONITORINFO *mi);
static void MoveWindowToTouchingZone(HWND hwnd, UCHAR direction, UCHAR extend)
{
    if(!(conf.UseZones&1) || state.mdiclient || !state.resizable) // Zones disabled
        return;

    SetOriginFromRestoreData(hwnd, AC_MOVE);
    // 1st get current window position.
    RECT rc;
    GetWindowRectL(hwnd, &rc);
    POINT pt;
    int offset = Iabs(conf.InterZone)+16;

    if        (direction == 0) { // LEFT
        pt.x = rc.left - offset; // Mid Left segment
        pt.y = (rc.top+rc.bottom)/2;
    } else if (direction == 1) { // TOP
        pt.x = (rc.right+rc.left)/2; // Mid Top segment
        pt.y = rc.top - offset;
    } else if (direction == 2) { // RIGHT
        pt.x = rc.right + offset; // Mid Right segment
        pt.y = (rc.top+rc.bottom)/2;
    } else if (direction == 3) { // BOTTOM
        pt.x = (rc.right+rc.left)/2; // Mid Bottom segment
        pt.y = rc.bottom + offset;
    }
    int zoomed;
    if ((zoomed = IsZoomed(hwnd)) && extend) return;

    // Clamp point inside monitor
    MONITORINFO mi;
    GetMonitorInfoFromWin(hwnd, &mi);
    ClampPointInRect(&mi.rcWork, &pt);

    RECT zrc;
    unsigned ret = GetZoneFromPoint(pt, &zrc);
    if (!ret) return; // Outside of a rect

    RECT fr; // final rect...
    if (extend) { // extend the zone instead.
        UnionRect(&fr, &rc, &zrc); // Union of original and new rect
    } else {
        CopyRect(&fr, &zrc);
    }
    LastWin.end = 0;
    RECT bd;
    FixDWMRect(hwnd, &bd);
    InflateRectBorder(&fr, &bd);

    SetRestoreData(hwnd, state.origin.width, state.origin.height, SNAPPED|SNZONE);
    MoveWindowAsync(hwnd, fr.left, fr.top, CLAMPW(fr.right-fr.left), CLAMPH(fr.bottom-fr.top));
}
