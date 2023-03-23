/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * (C) Raymond GILLIBERT                                                 *
 * Functions to handle Snap Layouts under AltSnap                        *
 * General Public License Version 3 or later (Free Software Foundation)  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"
static void MouseMove(POINT pt);

#define MAX_ZONES 2048
#define MAX_LAYOUTS 10
RECT *Zones[MAX_LAYOUTS];
unsigned nzones[MAX_LAYOUTS];
DWORD Grids[MAX_LAYOUTS];
static void freezones()
{
    unsigned i;
    for (i=0; i<ARR_SZ(Zones);i++)
        free(Zones[i]);
}
static int ReadRectFromini(RECT *zone, unsigned laynum, unsigned idx, const TCHAR *inisection)
{
    if (idx > MAX_ZONES) return 0;

    long *ZONE = (long *)zone;
    char zname[32]="";
    TCHAR zaschii[128];

    LPCTSTR txt = GetSectionOptionCStr(inisection, ZidxToZonestrA(laynum, idx, zname), NULL);
    if (!txt || !txt[0])
        return 0;
    //LOGA("Zone(%u, %u) = %S", laynum, idx, txt);
    // Copy to modifyable buffer.
    lstrcpy_s(zaschii, ARR_SZ(zaschii), txt);
    TCHAR *oldptr, *newptr;
    oldptr = &zaschii[0];
    UCHAR i;
    for (i=0; i < 4; i++) {
        newptr = lstrchr(oldptr, ',');
        if (!newptr) return 0;
        *newptr = '\0';
        newptr++;
        if(!*oldptr) return 0;
        ZONE[i] = strtoi(oldptr); // 0 left, 1 top, 2 right, 3 bottom;
        oldptr = newptr;
    }
    return 1;
}
static void ReadZonesFromLayout(const TCHAR *inisection, unsigned laynum)
{
    nzones[laynum] = 0;
    Zones[laynum] = NULL;
    RECT tmpzone;
    while (ReadRectFromini(&tmpzone, laynum, nzones[laynum], inisection)) {
        RECT *tmp = (RECT *)realloc( Zones[laynum], (nzones[laynum]+1) * sizeof(*tmp) );
        if(!tmp) return;
        Zones[laynum] = tmp;
        CopyRect(&Zones[laynum][nzones[laynum]++], &tmpzone);
    }
}
// Load all zones from ini file
static void ReadZones(const TCHAR *inisection)
{
    UCHAR i;
    for(i=0; i<ARR_SZ(Zones); i++)
        ReadZonesFromLayout(inisection, i);
}
// Generate a grid if asked
static void GenerateGridZones(unsigned layout, unsigned short Nx, unsigned short Ny)
{
    // Enumerate monitors
    nummonitors = 0;
    unsigned nz = 0;
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, 0);
    RECT *tmp = (RECT *)realloc(Zones[layout], nummonitors * Nx * Ny * sizeof(*tmp));
    if(!tmp) return;
    Zones[layout] = tmp;
    if(!Zones[layout]) return;

    // Loop on all monitors
    unsigned m;
    for (m=0; m<nummonitors; m++) {
        RECT *mon = &monitors[m];
        unsigned i;
        for(i=0; i<Nx; i++) { // Horizontal
            unsigned j;
            for(j=0; j<Ny; j++) { //Vertical
                Zones[layout][nz].left  = mon->left+(( i ) * (mon->right - mon->left))/Nx;
                Zones[layout][nz].top   = mon->top +(( j ) * (mon->bottom - mon->top))/Ny;
                Zones[layout][nz].right = mon->left+((i+1) * (mon->right - mon->left))/Nx;
                Zones[layout][nz].bottom= mon->top +((j+1) * (mon->bottom - mon->top))/Ny;
                nz++;
            }
        }
    }
    nzones[layout] = nz;
}
static void ReadGrids(const TCHAR *inisection)
{
    UCHAR i;
    char gnamex[8];
    char gnamey[8];
    strcpy(gnamex, "GridNx");
    strcpy(gnamey, "GridNy");
    gnamex[7] = '\0';
    gnamey[7] = '\0';
    for (i=0; i < ARR_SZ(Grids); i++) {
        Grids[i] = 0;
        unsigned short GridNx = GetSectionOptionInt(inisection, gnamex, 0);
        unsigned short GridNy = GetSectionOptionInt(inisection, gnamey, 0);
        if (GridNx && GridNy && GridNx*GridNy <= MAX_ZONES) {
            Grids[i] = GridNx | GridNy << 16; // Store grid dimentions
            GenerateGridZones(i, GridNx, GridNy);
        }
        // GridNx1, GridNx2, GridNx3...
        gnamex[6] = '1' + i;
        gnamey[6] = '1' + i;
    }
}
// Recalculate the zones from the Grid info.
// Needed in case resolution changes.
static void RecalculateZonesFromGrids()
{
    UCHAR i;
    for (i=0; i<ARR_SZ(Grids); i++) {
        unsigned short GridNx = LOWORD(Grids[i]);
        unsigned short GridNy = HIWORD(Grids[i]);
        if(GridNx && GridNy)
            GenerateGridZones(i, GridNx, GridNy);
    }
}

static xpure unsigned long ClacPtRectDist(const POINT pt, const RECT *zone)
{
    POINT Zpt = { (zone->left+zone->right)/2, (zone->top+zone->bottom)/2 };
    long dx = (pt.x - Zpt.x)>>1;
    long dy = (pt.y - Zpt.y)>>1;

    return dx*dx + dy*dy;
}

static unsigned GetNearestZoneDist(POINT pt, unsigned long *dist_)
{
    RECT * const lZones = Zones[conf.LayoutNumber];
    if(!lZones) return 0;
    unsigned long dist = 0xffffffff;
    unsigned idx = 0;
    unsigned i;
    for (i=0; i < nzones[conf.LayoutNumber]; i++) {
        unsigned long dst = ClacPtRectDist(pt, &lZones[i]);
        if ( dst < dist ) {
            dist = dst;
            idx = i;
        }
    }
    *dist_ = dist;
    return idx;
}

static unsigned GetZoneNearestFromPoint(POINT pt, RECT *urc, int extend)
{

    RECT * const lZones = Zones[conf.LayoutNumber];
    if(!lZones) return 0;
    unsigned i, ret=0;
    SetRectEmpty(urc);
    int iz = conf.InterZone;
    iz = (iz * iz) << 1;
    unsigned long mindist=0;

    i = GetNearestZoneDist(pt, &mindist);
    ret = mindist != 0xffffffff;
    if (!ret) return 0;
    CopyRect(urc, &lZones[i]);
    mindist += iz; // Add iz tolerance.
    for (i=0; i < nzones[conf.LayoutNumber]; i++) {

        BOOL inrect = mindist > ClacPtRectDist(pt, &lZones[i]);
        if ((state.ctrl||extend) && !inrect)
            inrect = mindist > ClacPtRectDist(extend?state.shiftpt:state.ctrlpt, &lZones[i]);

        if (inrect) {
            UnionRect(urc, urc, &lZones[i]);
            ret++;
        }
    }
    return ret;
}

static unsigned GetZoneContainingPoint(POINT pt, RECT *urc, int extend)
{

    RECT * const lZones = Zones[conf.LayoutNumber];
    if(!lZones) return 0;
    unsigned i, ret=0;
    SetRectEmpty(urc);
    int iz = conf.InterZone;
    for (i=0; i < nzones[conf.LayoutNumber]; i++) {
        if (iz) InflateRect(&lZones[i], iz, iz);

        int inrect=0;
        inrect = PtInRect(&lZones[i], pt);
        if ((state.ctrl||extend) && !inrect)
            inrect = PtInRect(&lZones[i], extend?state.shiftpt:state.ctrlpt);

        if (iz) InflateRect(&lZones[i], -iz, -iz);
        if (inrect) {
            UnionRect(urc, urc, &lZones[i]);
            ret++;
        }
    }
    return ret;
}
static unsigned GetZoneFromPoint(POINT pt, RECT *urc, int extend)
{
	switch (conf.ZSnapMode) {
	case 0: return GetZoneContainingPoint(pt, urc, extend);
	case 1: return GetZoneNearestFromPoint(pt, urc, extend);
	}
	// Invalid Snap mode!
	return 0;
}
static int pure IsResizable(HWND hwnd);

static void ActionToggleSnapToZoneMode()
{
    if (conf.UseZones&1 && state.action == AC_MOVE) {
        state.usezones = !state.usezones;
        MouseMove(state.prevpt);
    }
}
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
    unsigned ret = GetZoneFromPoint(pt, &rc, conf.UseZones&4);
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
    int offset = abs(conf.InterZone)+16;

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
    unsigned ret = GetZoneContainingPoint(pt, &zrc, 0);
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
// Sets urc as the union of all the rcs array or RECT, of length N
static void UnionMultiRect(RECT *urc, const RECT *rcs, unsigned N)
{
    CopyRect(urc, &rcs[0]);
    while (--N) {
        rcs++;
        urc->left    = min(urc->left,   rcs->left);
        urc->top     = min(urc->top,    rcs->top);
        urc->right   = max(urc->right,  rcs->right);
        urc->bottom  = max(urc->bottom, rcs->bottom);
    }
}
static pure DWORD GetLayoutRez(int laynum)
{
    laynum = CLAMP(0, laynum, ARR_SZ(Zones)-1);
    if (conf.UseZones&2) {
        // In Grid mode we return the grid dimentions!
        return Grids[laynum];
    }
    unsigned nz = nzones[laynum];
    const RECT *zone = Zones[laynum];
    if (!zone || !nz) return 0;
    RECT urc;
    UnionMultiRect(&urc, zone, nz);
    return  (urc.right-urc.left) | (urc.bottom-urc.top)<<16;
}
// Calculate the dimentions of union rectangle that contains all
// the work area of every monitors.
static DWORD GetFullMonitorsRez()
{
    // Enumerate monitors
    EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, 0);
    if (!monitors || !nummonitors)
        return 0; // ERROR! do nothing
    RECT urc;
    UnionMultiRect(&urc, monitors, nummonitors);

    // Return dimentions width:height
    return (urc.right-urc.left) | (urc.bottom-urc.top)<<16;
}

// Return a layout number if its dimentions exactly fit
// the display monitor.
static int GetBestLayoutFromMonitors()
{
    if (!(conf.UseZones&1))
        return -1;
    if (conf.UseZones&2) {
        // Grid mode! Do not change Grid, just recalculate them all...
        RecalculateZonesFromGrids();
        return -1; // DONE!
    }
    DWORD curRZ = GetLayoutRez(conf.LayoutNumber);

    // If the current rez is 0:0 we should not change it.
    // Because it can be used as a dummy mode, plus a user
    // might have selected it in advance to configure it
    // later for the new display settings, so selecting
    // an other one would bother him.
    if (!curRZ)
        return -1;

    // Check if the current Layout is good for the current monitor
    DWORD monRZ;
    if (nummonitors <= 1) {
        RECT mrc;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &mrc, 0);
        monRZ = (mrc.right-mrc.left) | (mrc.bottom-mrc.top)<<16;
        // In Single monitor mode we use SPI_GETWORKAREA.
        // Because EnumDisplayMonitor often gives bad values.
        // I am unsure on how to properly handle multiple monitors...
        if(monRZ == curRZ) {
            return -1; // Current layout is good!
        }
    } else if ( (monRZ = GetFullMonitorsRez()) == curRZ ) {
        // GetFullMonitorsRez gives us the union of all monitors.rcWork
        return -1; // Current layout is good!
    }

    UCHAR i;
    for (i=0; i<ARR_SZ(Zones); i++) {
        DWORD rez = GetLayoutRez(i);
        if( rez == monRZ )
            return i;
    }
    // No perfect match found!
    return -1;
}
