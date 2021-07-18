#ifndef ALTDRAG_RPC_H
#define ALTDRAG_RPC_H

// App
#define APP_NAME       L"AltDrag"
#define APP_NAMEA      "AltDrag"
#define APP_VERSION    "1.46"

// User Messages
#define WM_TRAY           (WM_USER+1)
#define WM_SCLICK         (WM_USER+2)
#define WM_UPDCFRACTION   (WM_USER+3)
#define WM_UPDATETRAY     (WM_USER+4)
#define WM_UPDATESETTINGS (WM_USER+5)
#define WM_OPENCONFIG     (WM_USER+6)
#define WM_CLOSECONFIG    (WM_USER+7)
#define WM_ADDTRAY        (WM_USER+8)
#define WM_HIDETRAY       (WM_USER+9)

// List of possible actions
enum action { AC_NONE=0, AC_MOVE, AC_RESIZE, AC_MENU, AC_MINIMIZE, AC_MAXIMIZE, AC_CENTER
            , AC_ALWAYSONTOP, AC_CLOSE, AC_LOWER, AC_BORDERLESS, AC_KILL
            , AC_ROLL, AC_ALTTAB, AC_VOLUME, AC_TRANSPARENCY, AC_HSCROLL };

#define MOUVEMENT(action) (action <= AC_RESIZE)

#endif /* ALTDRAG_RPC_H */
