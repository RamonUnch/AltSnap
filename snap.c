/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * (C) Raymond GILLIBERT                                                 *
 * Functions to handle Snap/restore informations AltSnap                 *
 * Snapping informations set with Set/GetProp.                          *
 * Window database is used as fallback.                                  *
 * General Public License Version 3 or later (Free Software Foundation)  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define SNAPPED  1
#define ROLLED   2
#define SNLEFT    (1<<2)
#define SNRIGHT   (1<<3)
#define SNTOP     (1<<4)
#define SNBOTTOM  (1<<5)
#define SNMAXH    (1<<6)
#define SNMAXW    (1<<7)
#define SNCLEAR   (1<<8) // to clear the flag at init movement.
#define TORESIZE  (1<<9)
#define SNTHENROLLED (1<<10)
#define SNZONE    (1<<11)
#define SNMIN     (1<<12)
#define SNTOPLEFT     (SNTOP|SNLEFT)
#define SNTOPRIGHT    (SNTOP|SNRIGHT)
#define SNBOTTOMLEFT  (SNBOTTOM|SNLEFT)
#define SNBOTTOMRIGHT (SNBOTTOM|SNRIGHT)
#define SNAPPEDSIDE   (SNTOPLEFT|SNBOTTOMRIGHT)

#define PureLeft(flag)   ( (flag&SNLEFT) &&  !(flag&(SNTOP|SNBOTTOM))  )
#define PureRight(flag)  ( (flag&SNRIGHT) && !(flag&(SNTOP|SNBOTTOM))  )
#define PureTop(flag)    ( (flag&SNTOP) && !(flag&(SNLEFT|SNRIGHT))    )
#define PureBottom(flag) ( (flag&SNBOTTOM) && !(flag&(SNLEFT|SNRIGHT)) )

/////////////////////////////////////////////////////////////////////////////
#define NUMWNDDB 16
struct wnddata {
    unsigned restore;
    HWND hwnd;
    int width;
    int height;
//    int rolledh;
//    UINT odpi
};
static struct wnddata wnddb[NUMWNDDB];

/////////////////////////////////////////////////////////////////////////////
// Database functions: used as fallback if SetProp fails

// Zero-out the database to be called in Load()
static void ResetDB()
{
    int i;
    for (i=0; i < NUMWNDDB; i++) {
        wnddb[i].hwnd = NULL;
        wnddb[i].restore = 0;
    }
}
// Return the entry if it was already in db. Or NULL otherwise
static pure int GetWindowInDB(HWND hwndd)
{
    // Check if window is already in the wnddb database
    // And set it in the current state
    unsigned i;
    for (i=0; i < NUMWNDDB; i++) {
        if (wnddb[i].hwnd == hwndd) {
            return i;
        }
    }
    return -1;
}
static void AddWindowToDB(HWND hwndd, int width, int height, unsigned flag)
{
    int idx = GetWindowInDB(hwndd);

    // Find a nice place in wnddb if not already present
    if (idx < 0) {
        int i;
        for (i=0; i < NUMWNDDB && wnddb[i].hwnd; i++);
        if (i >= NUMWNDDB) return;
        idx = i;
    }
    wnddb[idx].hwnd = hwndd;
    wnddb[idx].width = width;
    wnddb[idx].height = height;
    wnddb[idx].restore = flag;
}
static void DelWindowFromDB(HWND hwnd)
{
    unsigned i;
    for (i=0; i < NUMWNDDB; i++) {
        if (wnddb[i].hwnd == hwnd) {
            wnddb[i].hwnd = NULL;
            wnddb[i].restore = 0;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
// Windows restore data are saved in properties
// In 32b mode width and height are shrunk to 16b WORDS
// In 64b mode width and height are stored on 32b DWORDS
// I 64b mode we also check for FancyZone info that are stored the same way!
// There is a lot of cast because of annoying warnings...
// There is also a fallback to a database for some spetial windows.
static unsigned GetRestoreData(HWND hwnd, int *width, int *height)
{
    DorQWORD WH = (DorQWORD)GetProp(hwnd, APP_PROPPT);
    unsigned flags=0;
    if (WH) {
        *width  = (int)LOWORDPTR(WH);
        *height = (int)HIWORDPTR(WH);
        flags = (DorQWORD)GetProp(hwnd, APP_PROPFL);
  # ifdef WIN64 // Try fancy zone flag only in 64bit mode!
    } else if (conf.FancyZone && (WH = (DorQWORD)GetProp(hwnd, FZ_PROPPT))) {
        // It seems FancyZones stores size info in points, not pixels.
        *width  = (int)LOWORDPTR(WH);
        *height = (int)HIWORDPTR(WH);
        if (conf.FancyZone != 2) {
            int dpi = GetDpiForWindow(hwnd);
            if (dpi) {
                // Scale bcak to current dpi from 96
                *width  = MulDiv(*width,  dpi, 96);
                *height = MulDiv(*height, dpi, 96);
            }
        }
        return SNAPPED|SNZONE;
  # endif
    } else { // fallback to database
        int idx = GetWindowInDB(hwnd);
        if (idx >= 0) {
            *width = wnddb[idx].width;
            *height = wnddb[idx].height;
            flags = wnddb[idx].restore;
        } else {
            *width = *height = 0;
        }
    }

    UINT dpi = GetDpiForWindow(hwnd);
    UINT odpi = 0;
    if (dpi && (odpi = (UINT)(DorQWORD)GetProp(hwnd, APP_PRODPI)) && odpi != dpi) {
        // The current dpi is different for the window, We must scale the content.
        *width  = MulDiv(*width,  dpi, odpi);
        *height = MulDiv(*height, dpi, odpi);
    }
    return flags;
}
static void ClearRestoreData(HWND hwnd)
{
    RemoveProp(hwnd, APP_PROPPT);
    RemoveProp(hwnd, APP_PROPFL);
    // RemoveProp(hwnd, APP_PROPOFFSET);
  # ifdef WIN64
    if(conf.FancyZone) {
        RemoveProp(hwnd, FZ_PROPPT);
        RemoveProp(hwnd, FZ_PROPZONES);
    }
  # endif
    DelWindowFromDB(hwnd);
}
// Sets width, height and restore flag in a hwnd
static void SetRestoreData(HWND hwnd, int width, int height, unsigned restore)
{
    BOOL ret;
    ret  = SetProp(hwnd, APP_PROPFL, (HANDLE)(DorQWORD)restore);
    ret &= SetProp(hwnd, APP_PROPPT, (HANDLE)MAKELONGPTR(width, height));
    UINT dpi=0;
    if ( (dpi = GetDpiForWindow(hwnd)) )
        ret &= SetProp(hwnd, APP_PRODPI, (HANDLE)(DorQWORD)dpi);

    if (!ret) AddWindowToDB(hwnd, width, height, restore);
}

static unsigned GetRestoreFlag(HWND hwnd)
{
    unsigned flag = (DorQWORD)GetProp(hwnd, APP_PROPFL);

  # ifdef WIN64
    if(conf.FancyZone && GetProp(hwnd, FZ_PROPPT))
        flag |= SNAPPED|SNZONE;
  # endif

    int idx; // fallback to db.
    if (!flag && ((idx = GetWindowInDB(hwnd)) >=0)) {
        return wnddb[idx].restore;
    }
    return flag;
}
static void SetRestoreFlag(HWND hwnd, unsigned flag)
{
    BOOL ret = SetProp(hwnd, APP_PROPFL, (HANDLE)(DorQWORD)flag);
    int idx;
    if (!ret && ((idx = GetWindowInDB(hwnd)) >=0)) {
        wnddb[idx].restore = flag;
    }
}

static int IsAnySnapped(HWND hwnd)
{
    // Any kind of snapping, including maximization.
    // In short whenever the window will be restored.
    unsigned flg;
    return  IsZoomed(hwnd)
        ||( !(conf.SmartAero&4) && SNAPPED & (flg=GetRestoreFlag(hwnd)) && !(flg&SNCLEAR) )
        ||  IsWindowSnapped(hwnd);
}
/////////////////////////////////////////////////////////////////////////////
// borderless flag (saving old GWL_STYLE)
static void SetBorderlessFlag(HWND hwnd, LONG_PTR flag)
{
    SetProp(hwnd, APP_PRBDLESS,(HANDLE)flag);
}
static LONG_PTR GetBorderlessFlag(HWND hwnd)
{
    return (LONG_PTR)GetProp(hwnd, APP_PRBDLESS);
}
static LONG_PTR ClearBorderlessFlag(HWND hwnd)
{
    return (LONG_PTR)RemoveProp(hwnd, APP_PRBDLESS);
}

///////////////////////////////////////////////////////////////////////////////
//// Roll unroll stuff
//static int GetRolledHeight(HWND hwnd)
//{
//    int ret = (int)GetProp(hwnd, APP_ROLLED);
//    int idx;
//    if (!ret && ((idx = GetWindowInDB(hwnd)) >=0)) {
//        ret = wnddb[idx].rolledh;
//    }
//    return ret;
//}
//static int ClearRolledHeight(HWND hwnd)
//{
//    int ret = (int)RemoveProp(hwnd, APP_ROLLED);
//
//    int idx;
//    if (!ret && ((idx = GetWindowInDB(hwnd)) >=0)) {
//        ret = wnddb[idx].rolledh;
//        wnddb[idx].rolledh = 0;
//    }
//    return ret;
//}
//static void SetRolledHeight(HWND hwnd, int rolledh)
//{
//    BOOL ret = SetProp(hwnd, APP_ROLLED, (HANDLE)(DorQWORD)rolledh);
//    if (ret) return;
//
//    int idx;
//    if (((idx = GetWindowInDB(hwnd)) >=0)) {
//        wnddb[idx].rolledh = rolledh;
//    } else {
//        int i;
//        for (i=0; i < NUMWNDDB && wnddb[i].hwnd; i++);
//        if (i >= NUMWNDDB) return;
//        idx = i;
//        wnddb[idx].hwnd = hwnd;
//        wnddb[idx].rolledh = rolledh;
//        wnddb[idx].restore = 0;
//    }
//
//}
//static void AddToFlag(HWND hwnd, unsigned flag)
//{
//    SetRestoreFlag(hwnd, flag|GetRestoreFlag(hwnd));
//}
//
//static void RemoveToFlag(HWND hwnd, unsigned flag)
//{
//    SetRestoreFlag(hwnd, flag & (~GetRestoreFlag(hwnd)));
//}
