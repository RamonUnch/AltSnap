#ifndef ALTDRAG_RPC_H
#define ALTDRAG_RPC_H

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#include <windows.h>

#include "unfuck.h"

// Undecorated entry name in x64 mode
#ifdef WIN64
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc"
#else
#define LOW_LEVELK_BPROC "LowLevelKeyboardProc@12"
#endif

// App
#define APP_NAME       L"AltSnap"
#define APP_NAMEA      "AltSnap"
#define APP_VERSION    "1.53"
#define APP_PROPPT     APP_NAMEA"-RDim"
#define APP_PROPFL     APP_NAMEA"-RFlag"
#define APP_PROPOFFSET APP_NAMEA"-ROffset"
#define APP_PRBDLESS   APP_NAMEA"-RStyle"
#define APP_ROLLED     APP_NAMEA"-Rolled"
#define FZ_PROPPT      "FancyZones_RestoreSize"
#define FZ_PROPZONES   "FancyZones_zones"

// User Messages
#define WM_TRAY           (WM_USER+2)
#define WM_SCLICK         (WM_USER+3)
#define WM_UPDCFRACTION   (WM_USER+4)
#define WM_UPDATETRAY     (WM_USER+5)
#define WM_OPENCONFIG     (WM_USER+6)
#define WM_CLOSECONFIG    (WM_USER+7)
#define WM_UPDATESETTINGS (WM_USER+8)
#define WM_ADDTRAY        (WM_USER+9)
#define WM_HIDETRAY       (WM_USER+10)
#define WM_UNIKEYMENU     (WM_USER+11)
#define WM_MENUCREATED    (WM_USER+12)

// List of possible actions
enum action {
    AC_NONE=0, AC_MOVE, AC_RESIZE, AC_MENU, AC_MINIMIZE, AC_MAXIMIZE
  , AC_CENTER , AC_ALWAYSONTOP, AC_CLOSE, AC_LOWER, AC_BORDERLESS
  , AC_KILL, AC_MAXHV, AC_MINALL, AC_MUTE, AC_SIDESNAP
  , AC_ROLL, AC_ALTTAB, AC_VOLUME, AC_TRANSPARENCY, AC_HSCROLL
};
// List of actions strings, keep the SAME ORDER than above
#define ACTION_MAP { \
    "Nothing", "Move", "Resize", "Menu", "Minimize", "Maximize"    \
  , "Center", "AlwaysOnTop", "Close", "Lower", "Borderless"        \
  , "Kill" , "MaximizeHV", "MinAllOther", "Mute", "SideSnap"       \
  , "Roll", "AltTab", "Volume", "Transparency", "HScroll"          \
}

#define EXTRAKEYS_MAP { \
    /* A */ L"àáâäãåæª%āăąǎǟǡǣǻǽȁȃȧ|Ȧḁ%ⱥ|Ⱥɐ|Ɐɑ|Ɑɒ|Ɒⲁ|Ⲁ", \
    /* B */ L"%ƀɓƃƅɃ%ɓ|Ɓḃḅḇ", \
    /* C */ L"ç¢©%ćĉċčƈȼḉ|Ȼɕ", \
    /* D */ L"ð%ďđɖɗƌƍḋḍḏḑḓǆǅǳǲȡȸ", \
    /* E */ L"èéêë€%ēĕėęěǝəɛȅȇḕḗḙḛȩ|Ȩḝɇ|Ɇⱸ", \
    /* F */ L"ƒ%ḟɸⱷ", \
    /* G */ L"%ǵǧḡɠɣǥ", \
    /* H */ L"%ĥħƕǶḣḥḧḩḫȟ|Ȟⱨ|Ⱨⱶ|Ⱶẖ", \
    /* I */ L"ìíîï%ĩīĭǐȉȋįİıĳɩɨḭḯ", \
    /* J */ L"%ĵǰȷɉ|Ɉ", \
    /* K */ L"%ķĸƙǩḱḳⱪ|Ⱪꝁ|Ꝁʞ|Ʞ", \
    /* L */ L"£%ĺļľŀłƛǉǈȴƚ|Ƚⱡ|Ⱡɫ|Ɫḷḹḻḽ", \
    /* M */ L"µ%ḿṁṃɱ|Ɱɯ", \
    /* N */ L"ñ%ńņňŉŋɲƞ|Ƞǌǋǹȵ%ṅṇṉṋ", \
    /* O */ L"òóôöõø°%ōŏő%ɔɵơƣǒǫǭǿȍȏȣ|Ȣȫ|Ȫȭ|Ȭȯ|Ȯȱ|Ȱṍṏṑṓ%ⱺ", \
    /* P */ L"¶þ·•%ƥᵽ|Ᵽṕṗ", \
    /* Q */ L"¿¤‰‘’“”„…–—«»‹›%ȹɋ|Ɋ", \
    /* R */ L"®%ŕŗřƦȑȓṙṛṝṟɍ|Ɍɽ|Ɽⱹ", \
    /* S */ L"šß§%śŝşſ%ƨʃƪș|Șȿ|Ȿ%ṡṣṥṧṩ", \
    /* T */ L"†‡™%ţťŧƫƭʈț|Țȶⱦ|Ⱦ%ṫṭṯṱẗ", \
    /* U */ L"ùúûü%ũūůŭűų%ưʊǔǖǘǚǜȕȗʉ|Ʉ%ṳṵṷṹṻ", \
    /* V */ L"%ʋɅⱱⱴ%ṽṿ", \
    /* W */ L"%ẁẃŵẅⱳ|Ⱳ%ẇẉ", \
    /* X */ L"±×÷¬%ẋẍ", \
    /* Y */ L"ýÿ¥%ŷẏȳ|Ȳƴɏ|Ɏ", \
    /* Z */ L"ž%źẑżẓẕ%ƶʒƹƺǯȥ|Ȥɀ|Ɀⱬ|Ⱬ", \
}

#define MOUVEMENT(action) (action <= AC_RESIZE)

// Convert zone number to ini name entry
static wchar_t *ZidxToZonestr(int idx, wchar_t *zname)
{
    wchar_t txt[8];
    zname[0] = '\0';
    wcscat(zname, L"Zone");
    wcscat(zname, itowL(idx, txt, 10)); // Zone Name from zone number

    return zname;
}

#endif /* ALTDRAG_RPC_H */
