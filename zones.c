/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * (C) Raymond GILLIBERT                                                 *
 * Functions to handle Snap Layouts under AltSnap                        *
 * General Public License Version 3 or later (Free Software Foundation)  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hooks.h"
static void MouseMove(POINT pt);
static void ShowSnapLayoutPreview(unsigned char yep);

enum { MAX_ZONES=2048, MAX_LAYOUTS=10 };
#define INVALID_ZONE_IDX 0xffffffffU

RECT *Zones[MAX_LAYOUTS];
unsigned nzones[MAX_LAYOUTS];
DWORD Grids[MAX_LAYOUTS];

enum { ZONES_PREV_HIDE=0, ZONES_PREV_SHOW=1 };


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
static unsigned CopyZones(RECT *dZones, unsigned idx)
{
    unsigned i;
    RECT * const lZones = Zones[idx];
    for (i=0; i < nzones[idx]; ++i) {
        CopyRect(&dZones[i], &lZones[i]);
    }
    return i;
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

    // Loop on all monitors
    unsigned m;
    for (m=0; m<nummonitors; m++) {
        const RECT *mon = &monitors[m];
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

static xpure unsigned long PtDist2(POINT pt, POINT Zpt)
{
    long dx = (pt.x - Zpt.x)>>1;
    long dy = (pt.y - Zpt.y)>>1;

    return dx*dx + dy*dy;
}


static xpure unsigned long ClacPtRectDist(const POINT pt, const RECT *zone)
{
    POINT Zpt = { (zone->left+zone->right)/2, (zone->top+zone->bottom)/2 };
    return PtDist2(pt, Zpt);
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

    if (!state.usezones) {
        if (!state.forcelayoutdisplay)
            ShowSnapLayoutPreview(ZONES_PREV_HIDE);
        return;
    }

    ShowSnapLayoutPreview(ZONES_PREV_SHOW);
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

static xpure int IsPtInCone(POINT pt, POINT op, UCHAR direction)
{
    const long dx = pt.x - op.x, dy = pt.y - op.y;
    switch (direction) {
    case 0: // LEFT
        return dx <= 0 && (dy <= -dx && dy > dx);

    case 1: // TOP
        return dy <= 0 && (dx <= -dy && dx > dy);

    case 2: // RIGHT
        return dx > 0 && (dy <= dx && dy > -dx);

    case 3: // BOTTOM
        return dy > 0 && (dx <= dy && dx > -dy);
    }
    return 0;
}
static unsigned GetNearestZoneFromPointInDirection(const RECT *rc, RECT *out, UCHAR direction)
{
    unsigned idx = INVALID_ZONE_IDX;
    RECT * const lZones = Zones[conf.LayoutNumber];
    unsigned nZones = nzones[conf.LayoutNumber];
    if(!lZones) return 0;

    POINT opt = { (rc->left+rc->right)/2, (rc->top+rc->bottom)/2 };

    unsigned long dist = 0xffffffff;
    for(unsigned i=0; i < nZones; i++) {
        const RECT *zrc = &lZones[i];
        const POINT pt = { (zrc->left+zrc->right)/2, (zrc->top+zrc->bottom)/2 };
        unsigned long dst = PtDist2(pt, opt);
        if (!EqualRect(rc, zrc) && dst < dist && IsPtInCone(pt, opt, direction)) {
            dist = dst;
            idx = i;
        }
    }

    if (idx != INVALID_ZONE_IDX) {
        CopyRect(out, &lZones[idx]);
        return 1;
    }

    return 0;
}

//
//static void SnapToRect(HWND hwnd, RECT *fr)
//{
//    RECT bd;
//    LastWin.end = 0;
//    FixDWMRect(hwnd, &bd);
//    InflateRectBorder(fr, &bd);
//
//    SetRestoreData(hwnd, state.origin.width, state.origin.height, SNAPPED|SNZONE);
//    MoveWindowAsync(hwnd, fr->left, fr->top, CLAMPW(fr->right-fr->left), CLAMPH(fr->bottom-fr->top));
//}

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
    if (IsZoomed(hwnd) && extend) return;

    // Clamp point inside monitor
    MONITORINFO mi;
    GetMonitorInfoFromWin(hwnd, &mi);
    ClampPointInRect(&mi.rcWork, &pt);

    RECT zrc;
    unsigned ret=0;
    switch (conf.ZSnapMode) {
        case 0: ret = GetZoneContainingPoint(pt, &zrc, 0); break;
        case 1: ret = GetNearestZoneFromPointInDirection(&rc, &zrc, direction); break;
    }
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

static HWND g_zphwnd = NULL;
COLORREF g_ZPrevBDCol=RGB(0,0,0);
LRESULT CALLBACK SnapLayoutWinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static HPEN pen = NULL;

    switch (msg) {
    case WM_CREATE: {
        pen = (HPEN)CreatePen(PS_INSIDEFRAME, 4, g_ZPrevBDCol);
    } break;

    case WM_PAINT: {
        // Create Font
//        struct NEWNONCLIENTMETRICSAW ncm;
//        int dpi = GetDpiForWindow(hwnd);
//        GetNonClientMetricsDpi(&ncm, dpi);
//        ncm.lfCaptionFont.lfHeight = 64;
//        HFONT font = CreateFontIndirect(&ncm.lfCaptionFont);

        PAINTSTRUCT ps;
        POINT opt = { 0, 0 };
        ScreenToClient(hwnd, &opt);
        BeginPaint(hwnd, &ps);
        HPEN oldpen = (HPEN)SelectObject(ps.hdc, pen);
//        HFONT oldfont = (HFONT)SelectObject(ps.hdc, font);
        // PaintDesktop(ps.hdc);
        HBRUSH oldBrush = (HBRUSH)SelectObject(ps.hdc, GetStockObject(HOLLOW_BRUSH));
        for (size_t i=0; i < nzones[conf.LayoutNumber]; i++) {
            RECT *rc = &Zones[conf.LayoutNumber][i];
            if (!AreRectsTouchingT(rc, &ps.rcPaint, 0)) {
//                TCHAR buf[16]; SIZE sz;
//                const TCHAR *num = Int2lStr(buf, i);
//                int txtlen = lstrlen(num);
//                GetTextExtentPoint32(ps.hdc, num, txtlen, &sz);
//                int txtX = (rc->left + rc->right - sz.cx) / 2 + opt.x;
//                int txtY = (rc->top + rc->bottom - sz.cy) / 2 + opt.y;
//                TextOut(ps.hdc, txtX, txtY, num, txtlen);
                Rectangle(ps.hdc, rc->left+opt.x, rc->top+opt.y, rc->right+opt.x, rc->bottom+opt.y);
            }
        }
//        SelectObject(ps.hdc, oldfont);  // Restore old font
        SelectObject(ps.hdc, oldBrush); // Restore old pen
        SelectObject(ps.hdc, oldpen);   // Restore old brush

        EndPaint(hwnd, &ps);

//        DeleteObject(font);
    } return 0;

    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_LBUTTONUP:
        ShowWindow(hwnd, SW_HIDE);
        break;

    case WM_DESTROY: {
        DeleteObject(pen);
    } break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}


static void ShowSnapLayoutPreview(unsigned char yep)
{
    if (g_zphwnd) { // Window was created
        if (!yep) {
            ShowWindow(g_zphwnd, SW_HIDE);
        } else if (!IsWindowVisible(g_zphwnd)) {
            ShowWindow(g_zphwnd, SW_SHOW);
            SetWindowLevel(g_zphwnd, state.hwnd); // Behind currently moving hwn
        }
    }
}

static void ZonesPrevResetRegion()
{
    if (conf.ZonesPrevwOpacity != 0)
        return;
    POINT opt = { 0, 0 };
    RECT wrc;
    GetWindowRect(g_zphwnd, &wrc);
    ScreenToClient(g_zphwnd, &opt);
    HRGN hregion = CreateRectRgn(0,0,0,0);
    if (Zones[conf.LayoutNumber]) {
        for (unsigned i=0; i < nzones[conf.LayoutNumber]; i++) {
            RECT rc;
            CopyRect(&rc, &Zones[conf.LayoutNumber][i]);
            OffsetRect(&rc, opt.x, opt.y);
            HRGN tmpr = CreateRectRgn(rc.left, rc.top, rc.right, rc.top+4); // top  ^^^^
            CombineRgn(hregion, hregion, tmpr, RGN_OR);
            DeleteObject(tmpr);
            tmpr = CreateRectRgn(rc.left, rc.bottom-4, rc.right, rc.bottom); //     ____ bottom
            CombineRgn(hregion, hregion, tmpr, RGN_OR);
            DeleteObject(tmpr);
            tmpr = CreateRectRgn(rc.left, rc.top, rc.left+4, rc.bottom); //   left  |...
            CombineRgn(hregion, hregion, tmpr, RGN_OR);
            DeleteObject(tmpr);
            tmpr = CreateRectRgn(rc.right-4, rc.top, rc.right, rc.bottom); //       ...| right
            CombineRgn(hregion, hregion, tmpr, RGN_OR);
            DeleteObject(tmpr);
        }
    }
    SetWindowRgn(g_zphwnd, hregion, FALSE);
}

static unsigned readhotkeys(const TCHAR *inisection, const char *name, const TCHAR *def, UCHAR *keys, unsigned MaxKeys);
static void SnapLayoutPreviewCreateDestroy(const TCHAR *inisection)
{
    if(!conf.ShowZonesPrevw)
        return;
    if (inisection) {
        int bgcol[2], bdcol[2];
        readhotkeys(inisection, "ZonesPrevwBGCol",  TEXT("FF FF FF"), (UCHAR *)&bgcol[0], 3);
        readhotkeys(inisection, "ZonesPrevwBDCol",  TEXT("00 00 00"), (UCHAR *)&bdcol[0], 3);
        g_ZPrevBDCol = bdcol[0];
        const UCHAR opacity = conf.ZonesPrevwOpacity;
        const UCHAR usetrans = opacity!=0 && opacity!=255;
        const WNDPROC wproc  = opacity==0? DefWindowProc: SnapLayoutWinProc;
        const HBRUSH wbrush  = opacity==0? CreateSolidBrush(bdcol[0]): CreateSolidBrush(bgcol[0]);
        const WNDCLASSEX wnd = {
            sizeof(WNDCLASSEX), 0
          , wproc
          , 0, 0, hinstDLL
          , NULL, NULL, wbrush
          , NULL, APP_NAME TEXT("-ZonesPreview"), NULL };
        RegisterClassEx(&wnd);

        int left=0, top=0, width, height;
        if(GetSystemMetrics(SM_CMONITORS) >= 1) {
            left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
            top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
            width  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
         } else { // NT4...
             width = GetSystemMetrics(SM_CXFULLSCREEN)+256;
             height= GetSystemMetrics(SM_CYFULLSCREEN)+256;
         }
         g_zphwnd = CreateWindowEx(usetrans? WS_EX_TOOLWINDOW|WS_EX_LAYERED: WS_EX_TOOLWINDOW
                             , wnd.lpszClassName, NULL, WS_POPUP
                             , left, top, width, height, g_mainhwnd, NULL, hinstDLL, NULL);
        SetWindowLevel(g_zphwnd, state.hwnd); // Behind
        if (usetrans)
            SetLayeredWindowAttributesL(g_zphwnd, 0, opacity, LWA_ALPHA);

         // Set Window Region to only show outlines if needed...
         ZonesPrevResetRegion();

    } else {
        if (g_zphwnd) DestroyWindow(g_zphwnd);
        UnregisterClass(APP_NAME TEXT("-ZonesPreview"), hinstDLL);
    }
}

static void SetLayoutNumber(WPARAM number)
{
    int was_visible = IsWindowVisible(g_zphwnd);
    ShowSnapLayoutPreview(0);
    conf.LayoutNumber=CLAMP(0, number, 9);
    ZonesPrevResetRegion(); // re-calculate
    ShowSnapLayoutPreview(was_visible);

    if (!was_visible && conf.ShowZonesOnChange) {
        state.forcelayoutdisplay = 1;
        ShowSnapLayoutPreview(1);
        UINT id = SetTimer(g_mainhwnd, HIDELAYOUT_TIMER, conf.ShowZonesOnChange * 100, TimerWindowProc);
        state.forcelayoutdisplay = !!id;
    }
}
