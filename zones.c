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

    if (conf.UseZones&8 && state.snap != conf.AutoSnap) // Zones toggled by other click
        return;

    if (!state.usezones)
        return;
    RECT rc, bd;
    unsigned ret = GetZoneFromPoint(pt, &rc);
    if (!ret) return; // Outside of a rect

    LastWin.end = 0;
    LastWin.moveonly = 0; // We are resizing the window.
    FixDWMRect(state.hwnd, &bd);
    InflateRectBorder(&rc, &bd);

    SetRestoreData(state.hwnd, state.origin.width, state.origin.height, SNAPPED|SNZONE);
    *posx = rc.left;
    *posy = rc.top;
    *width = rc.right - rc.left;
    *height = rc.bottom - rc.top;
}
