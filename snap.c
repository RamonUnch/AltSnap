/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * (C) Raymond GILLIBERT                                                 *
 * Functions to handle Snap/restore informations AltSnap                 *
 * Snapping informations set with Set/GetPropA.                          *
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
};
struct wnddata wnddb[NUMWNDDB];

/////////////////////////////////////////////////////////////////////////////
// Database functions: used as fallback if SetPropA fails

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
// In 32b mode width and height are shrinked to 16b WORDS
// In 64b mode width and height are stored on 32b DWORDS
// I 64b mode we also check for FancyZone info that are stored the same way!
// There is a lot of cast because of annoying warnings...
// There is also a fallback to a database for some spetial windows.
static unsigned GetRestoreData(HWND hwnd, int *width, int *height)
{
    DorQWORD WH = (DorQWORD)GetPropA(hwnd, APP_PROPPT);
    if (WH) {
        *width  = (int)LOWORDPTR(WH);
        *height = (int)HIWORDPTR(WH);

        return (DorQWORD)GetPropA(hwnd, APP_PROPFL);
  # ifdef WIN64 // Try fancy zone flag only in 64bit mode!
    } else if (conf.FancyZone && (WH = (DorQWORD)GetPropA(hwnd, FZ_PROPPT))) {
        *width  = (int)LOWORDPTR(WH);
        *height = (int)HIWORDPTR(WH);
        return SNAPPED|SNZONE;
  # endif
    } else { // fallback to database
        int idx = GetWindowInDB(hwnd);
        if (idx >= 0) {
            *width = wnddb[idx].width;
            *height = wnddb[idx].height;
            return wnddb[idx].restore;
        }
    }
    *width = *height = 0;
    return 0;
}
static void ClearRestoreData(HWND hwnd)
{
    RemovePropA(hwnd, APP_PROPPT);
    RemovePropA(hwnd, APP_PROPFL);
    RemovePropA(hwnd, APP_PROPOFFSET);
  # ifdef WIN64
    if(conf.FancyZone) {
        RemovePropA(hwnd, FZ_PROPPT);
        RemovePropA(hwnd, FZ_PROPZONES);
    }
  # endif
    DelWindowFromDB(hwnd);
}
// Sets width, height and restore flag in a hwnd
static void SetRestoreData(HWND hwnd, int width, int height, unsigned restore)
{
    BOOL ret;
    ret  = SetPropA(hwnd, APP_PROPFL, (HANDLE)(DorQWORD)restore);
    ret &= SetPropA(hwnd, APP_PROPPT, (HANDLE)MAKELONGPTR(width, height));
    if (!ret) AddWindowToDB(hwnd, width, height, restore);
}

static unsigned GetRestoreFlag(HWND hwnd)
{
    unsigned flag = (DorQWORD)GetPropA(hwnd, APP_PROPFL);

  # ifdef WIN64
    if(conf.FancyZone && GetPropA(hwnd, FZ_PROPPT))
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
    BOOL ret = SetPropA(hwnd, APP_PROPFL, (HANDLE)(DorQWORD)flag);
    int idx;
    if (!ret && ((idx = GetWindowInDB(hwnd)) >=0)) {
        wnddb[idx].restore = flag;
    }
}
