/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This file must be UTF-8, Çe fichier doit être encodé en UTF8          *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef ALTSNAP_STRINGS_H
#define ALTSNAP_STRINGS_H

#include <stddef.h>
#include <windows.h>
#include "resource.h"

// Resolve index from entry name.
#define L10NIDX(entryname) (short)( offsetof(struct strings, entryname)/sizeof(TCHAR**) )

// Resolve entry name from index.
#define L10NSTR(i) ( ((const TCHAR*const*const)l10n)[i] )

struct langinfoitem {
  TCHAR *code;
  TCHAR *lang_english;
  TCHAR *lang;
  TCHAR *author;
  TCHAR *fn;
};

#define LANGUAGE_MAP \
  LNGVALUE(Code,        TEXT("en-US"), NULL) \
  LNGVALUE(LangEnglish, TEXT("English"), NULL) \
  LNGVALUE(Lang,        TEXT("English"), NULL) \
  LNGVALUE(Author,      TEXT("Stefan Sundin, RamonUnch"), NULL) \
  /* Menu */ \
  LNGVALUE(MenuEnable,       TEXT("&Enable"), NULL) \
  LNGVALUE(MenuDisable,      TEXT("&Disable"), NULL) \
  LNGVALUE(MenuHideTray,     TEXT("&Hide tray"), NULL) \
  LNGVALUE(MenuConfigure,    TEXT("&Configure..."), NULL) \
  LNGVALUE(MenuAbout,        TEXT("&About"), NULL) \
  LNGVALUE(MenuOpenIniFile,  TEXT("&Open ini file"), NULL) \
  LNGVALUE(MenuSaveZones,    TEXT("&Save test windows as snap layout"), NULL) \
  LNGVALUE(MenuEmptyZone,    TEXT("(empty)"), NULL) \
  LNGVALUE(MenuSnapLayout,   TEXT("Snap layout &"), NULL) \
  LNGVALUE(MenuEditLayout,   TEXT("Edi&t snap layout"), NULL) \
  LNGVALUE(MenuCloseAllZones,TEXT("C&lose all test windows"), NULL) \
  LNGVALUE(MenuExit,         TEXT("E&xit"), NULL) \
  /* Config Title and Tabs */ \
  LNGVALUE(ConfigTitle, APP_NAME TEXT(" Configuration"), NULL) \
  LNGVALUE(ConfigTabGeneral,   TEXT("General"), NULL) \
  LNGVALUE(ConfigTabMouse,     TEXT("Mouse"), NULL) \
  LNGVALUE(ConfigTabKeyboard,  TEXT("Keyboard"), NULL) \
  LNGVALUE(ConfigTabBlacklist, TEXT("Blacklist"), NULL) \
  LNGVALUE(ConfigTabAdvanced,  TEXT("Advanced"), NULL) \
  LNGVALUE(ConfigTabAbout,     TEXT("About"), NULL) \
  /* General Box */ \
  LNGVALUE(GeneralBox, TEXT("General settings"), NULL) \
  LNGVALUE(GeneralAutoFocus,      TEXT("&Focus windows when dragging.\nYou can also press Ctrl to focus windows."), NULL) \
  LNGVALUE(GeneralAero,           TEXT("Mimi&c Aero Snap"), NULL) \
  LNGVALUE(GeneralSmartAero,      TEXT("Smart Aero Sna&p dimensions"), NULL) \
  LNGVALUE(GeneralSmarterAero,    TEXT("Smarter Aer&o Snap dimensions"), NULL) \
  LNGVALUE(GeneralStickyResize,   TEXT("Resi&ze other snapped windows with Shift"), NULL) \
  LNGVALUE(GeneralInactiveScroll, TEXT("&Scroll inactive windows"), NULL) \
  LNGVALUE(GeneralMDI,            TEXT("&MDI support"), NULL) \
  LNGVALUE(GeneralAutoSnap,       TEXT("S&nap window edges to:"), NULL) \
  LNGVALUE(GeneralAutoSnap0,      TEXT("Disabled"), NULL) \
  LNGVALUE(GeneralAutoSnap1,      TEXT("To screen borders"), NULL) \
  LNGVALUE(GeneralAutoSnap2,      TEXT("+ outside of windows"), NULL) \
  LNGVALUE(GeneralAutoSnap3,      TEXT("+ inside of windows"), NULL) \
  LNGVALUE(GeneralLanguage,       TEXT("&Language:"), NULL) \
  LNGVALUE(GeneralFullWin,        TEXT("&Drag full windows"), NULL) \
  LNGVALUE(GeneralUseZones,       TEXT("Snap to Layo&ut with Shift (configure with tray menu)"), TEXT("This option will mimick FancyZones\r\nOpen TestWindows to set up the zones\n Look at the tray icon menu")) \
  LNGVALUE(GeneralPiercingClick,  TEXT("Avoi&d blocking Alt+Click (disables AltSnap double-clicks)"), NULL) \
  LNGVALUE(GeneralResizeAll,      TEXT("&Resize all windows"), NULL) \
  LNGVALUE(GeneralResizeCenter,   TEXT("Center resize mode"), NULL) \
  LNGVALUE(GeneralResizeCenterNorm, TEXT("All d&irections"), NULL) \
  LNGVALUE(GeneralResizeCenterBr,   TEXT("&Bottom right"), NULL) \
  LNGVALUE(GeneralResizeCenterMove, TEXT("Mo&ve"), NULL) \
  LNGVALUE(GeneralResizeCenterClose,TEXT("Clos&est side"), NULL) \
  LNGVALUE(GeneralAutostartBox,         TEXT("Autostart"), NULL) \
  LNGVALUE(GeneralAutostart,            TEXT("S&tart ") APP_NAME TEXT(" when logging on"), NULL) \
  LNGVALUE(GeneralAutostartHide,        TEXT("&Hide tray"), NULL) \
  LNGVALUE(GeneralAutostartElevate,     TEXT("&Elevate to administrator privileges"), NULL) \
  LNGVALUE(GeneralAutostartElevateTip,  TEXT("Note that a UAC prompt will appear every time you log in, unless you disable UAC completely or use the Task Scheduler.\nTo setup a Scheduled task for this purpose, you can use the sch_On.bat batch files in Altsnap's folder."), NULL) \
  LNGVALUE(GeneralElevate,              TEXT("E&levate"), NULL) \
  LNGVALUE(GeneralElevated,             TEXT("Elevated"), NULL) \
  LNGVALUE(GeneralElevationAborted,     TEXT("Elevation aborted."), NULL) \
  /* Keyboard & Mouse tabs */ \
  LNGVALUE(InputMouseBox, TEXT("Mouse actions"), NULL) \
  LNGVALUE(InputMouseBtAc1,   TEXT("&1. Primary"), TEXT("Action performed with Hotkey")) \
  LNGVALUE(InputMouseBtAc2,   TEXT("&2. Alternate"), TEXT("Action performed with Hotkey + Modifier key (see Keyboard tab)")) \
  LNGVALUE(InputMouseINTTB,   TEXT("&Title bar"), TEXT("Action performed when clicking on the title bar")) \
  LNGVALUE(InputMouseWhileM,  TEXT("Whil&e moving"), NULL) \
  LNGVALUE(InputMouseWhileR,  TEXT("While resi&zing"), NULL) \
  LNGVALUE(InputMouseLMB,     TEXT("Left mouse &button:"), NULL) \
  LNGVALUE(InputMouseMMB,     TEXT("&Middle mouse button:"), NULL) \
  LNGVALUE(InputMouseRMB,     TEXT("Ri&ght mouse button:"), NULL) \
  LNGVALUE(InputMouseMB4,     TEXT("Mouse button &4:"), NULL) \
  LNGVALUE(InputMouseMB5,     TEXT("Mouse button &5:"), NULL) \
  LNGVALUE(InputMouseScroll,  TEXT("&Scroll wheel:"), NULL) \
  LNGVALUE(InputMouseHScroll, TEXT("Scroll wheel (&horizontal):"), NULL) \
  LNGVALUE(InputMouseMoveUp,  TEXT("Long &drag-free Move:"), NULL) \
  LNGVALUE(InputMouseResizeUp,TEXT("Long drag-&free Resize:"), NULL) \
  LNGVALUE(InputMouseTTBActionBox,  TEXT("Use specific actions when clicking the Title bar"), NULL) \
  LNGVALUE(InputMouseTTBActionNA,   TEXT("Without hot&key"), NULL) \
  LNGVALUE(InputMouseTTBActionWA,   TEXT("&With hotkey"), NULL) \
  LNGVALUE(InputMouseMMBHC, TEXT("M&iddle mouse button"), NULL) \
  LNGVALUE(InputMouseMB4HC, TEXT("M&ouse button 4"), NULL) \
  LNGVALUE(InputMouseMB5HC, TEXT("Mo&use button 5"), NULL) \
  LNGVALUE(InputMouseLongClickMove,  TEXT("Mo&ve windows with a long left-click"), NULL) \
  LNGVALUE(InputScrollLockState,     TEXT("Suspend/Resume AltSnap based on &Scroll lock state"), NULL) \
  LNGVALUE(InputUniKeyHoldMenu,      TEXT("Pop&up an extended character menu when holding an alphabetic key down"), NULL) \
  LNGVALUE(InputKeyCombo,            TEXT("Use two keys &combo to activate"), NULL) \
  LNGVALUE(InputGrabWithAlt,         TEXT("&Action without click:"), NULL) \
  LNGVALUE(InputGrabWithAltB,        TEXT("Acti&on without click (alt):"), NULL) \
  \
  LNGVALUE(InputActionMove,      TEXT("Move window"), NULL) \
  LNGVALUE(InputActionResize,    TEXT("Resize window"), NULL) \
  LNGVALUE(InputActionRestore,   TEXT("Restore window"), NULL) \
  LNGVALUE(InputActionClose,     TEXT("&Close window"), NULL) \
  LNGVALUE(InputActionKill,      TEXT("&Kill program"), NULL) \
  LNGVALUE(InputActionPause,     TEXT("Pause program"), NULL) \
  LNGVALUE(InputActionResume,    TEXT("Resume program"), NULL) \
  LNGVALUE(InputActionASOnOff,   TEXT("S&uspend/Resume AltSnap"), NULL) \
  LNGVALUE(InputActionMoveOnOff, TEXT("Movement dis&abled"), NULL) \
  LNGVALUE(InputActionMinimize,  TEXT("Mi&nimize window"), NULL) \
  LNGVALUE(InputActionMaximize,  TEXT("Ma&ximize window"), NULL) \
  LNGVALUE(InputActionLower,     TEXT("&Lower window"), NULL) \
  LNGVALUE(InputActionFocus,     TEXT("Focus window"), NULL) \
  LNGVALUE(InputActionNStacked,  TEXT("Next stacked window"), NULL) \
  LNGVALUE(InputActionNStacked2, TEXT("Next laser stacked window"), NULL) \
  LNGVALUE(InputActionPStacked,       TEXT("Previous stacked window"), NULL) \
  LNGVALUE(InputActionPStacked2,      TEXT("Previous laser stacked window"), NULL) \
  LNGVALUE(InputActionNPStacked,      TEXT("Next/Prev stacked window"), NULL) \
  LNGVALUE(InputActionNPStacked2,     TEXT("Next/Prev laser stacked window"), NULL) \
  LNGVALUE(InputActionStackList,      TEXT("Stacked windows list"), NULL) \
  LNGVALUE(InputActionStackList2,     TEXT("Laser stacked windows list"), NULL) \
  LNGVALUE(InputActionAltTabList,     TEXT("Windows list"), NULL) \
  LNGVALUE(InputActionAltTabFullList, TEXT("All windows list"), NULL) \
  LNGVALUE(InputActionMLZone,   TEXT("Move to the left zone"), NULL) \
  LNGVALUE(InputActionMTZone,   TEXT("Move to the top zone"), NULL) \
  LNGVALUE(InputActionMRZone,   TEXT("Move to the right zone"), NULL) \
  LNGVALUE(InputActionMBZone,   TEXT("Move to the bottom zone"), NULL) \
  LNGVALUE(InputActionXLZone,   TEXT("Extend to the left zone"), NULL) \
  LNGVALUE(InputActionXTZone,   TEXT("Extend to the top zone"), NULL) \
  LNGVALUE(InputActionXRZone,   TEXT("Extend to the right zone"), NULL) \
  LNGVALUE(InputActionXBZone,   TEXT("Extend to the bottom zone"), NULL) \
  LNGVALUE(InputActionXTNLEdge, TEXT("Extend to the next left edge"), NULL) \
  LNGVALUE(InputActionXTNTEdge, TEXT("Extend to the next top edge"), NULL) \
  LNGVALUE(InputActionXTNREdge, TEXT("Extend to the next right edge"), NULL) \
  LNGVALUE(InputActionXTNBEdge, TEXT("Extend to the next bottom edge"), NULL) \
  LNGVALUE(InputActionMTNLEdge, TEXT("Move to the next left edge"), NULL) \
  LNGVALUE(InputActionMTNTEdge, TEXT("Move to the next top edge"), NULL) \
  LNGVALUE(InputActionMTNREdge, TEXT("Move to the next right edge"), NULL) \
  LNGVALUE(InputActionMTNBEdge, TEXT("Move to the next bottom edge"), NULL) \
  LNGVALUE(InputActionStepL,    TEXT("Step left"), NULL) \
  LNGVALUE(InputActionStepT,    TEXT("Step up"), NULL) \
  LNGVALUE(InputActionStepR,    TEXT("Step right"), NULL) \
  LNGVALUE(InputActionStepB,    TEXT("Step down"), NULL) \
  LNGVALUE(InputActionSStepL,   TEXT("Small step left"), NULL) \
  LNGVALUE(InputActionSStepT,   TEXT("Small step up"), NULL) \
  LNGVALUE(InputActionSStepR,   TEXT("Small step right"), NULL) \
  LNGVALUE(InputActionSStepB,   TEXT("Small step down"), NULL) \
  LNGVALUE(InputActionFocusL,   TEXT("Focus left window"), NULL) \
  LNGVALUE(InputActionFocusT,   TEXT("Focus top window"), NULL) \
  LNGVALUE(InputActionFocusR,   TEXT("Focus right window"), NULL) \
  LNGVALUE(InputActionFocusB,   TEXT("Focus bottom window"), NULL) \
  LNGVALUE(InputActionRoll,     TEXT("&Roll/Unroll window"), NULL) \
  LNGVALUE(InputActionAlwaysOnTop,  TEXT("Toggle always on &top"), NULL) \
  LNGVALUE(InputActionBorderless,   TEXT("Toggle &borderless"), NULL) \
  LNGVALUE(InputActionCenter,       TEXT("C&enter window on screen"), NULL) \
  LNGVALUE(InputActionOriClick,     TEXT("Send ori&ginal click"), NULL) \
  LNGVALUE(InputActionNothing,      TEXT("Nothing"), NULL) \
  LNGVALUE(InputActionAltTab,       TEXT("Alt+Tab"), NULL) \
  LNGVALUE(InputActionVolume,       TEXT("Volume"), NULL) \
  LNGVALUE(InputActionMute,         TEXT("Mute &sounds"), NULL) \
  LNGVALUE(InputActionMenu,         TEXT("Action menu"), NULL) \
  LNGVALUE(InputActionMaximizeHV,   TEXT("Maximize &Vertically"), NULL) \
  LNGVALUE(InputActionSideSnap,     TEXT("&Snap to monitor side/corner"), NULL) \
  LNGVALUE(InputActionExtendSnap ,  TEXT("Extend to monitor side/corner"), NULL) \
  LNGVALUE(InputActionExtendTNEdge, TEXT("Extend to next edge"), NULL) \
  LNGVALUE(InputActionMoveTNEdge,   TEXT("Move to next edge"), NULL) \
  LNGVALUE(InputActionMinAllOther,  TEXT("Minimize &other windows"), NULL) \
  LNGVALUE(InputActionTransparency, TEXT("Transparency"), NULL) \
  LNGVALUE(InputActionZoom,         TEXT("Zoom window"), NULL) \
  LNGVALUE(InputActionZoom2,        TEXT("Zoom window 2"), NULL) \
  LNGVALUE(InputActionHScroll,      TEXT("Horizontal scroll"), NULL) \
  LNGVALUE(InputActionNPLayout,     TEXT("Next/Prev Snap Layout"), NULL) \
  LNGVALUE(InputActionNLayout,     TEXT("Next Snap Layout"), NULL) \
  LNGVALUE(InputActionPLayout,     TEXT("Prev Snap Layout"), NULL) \
  \
  LNGVALUE(InputHotkeysBox,            TEXT("Hotkeys"), NULL) \
  LNGVALUE(InputHotkeysModKey,         TEXT("Modifier key for al&ternate action:"), NULL) \
  LNGVALUE(InputHotclicksBox,          TEXT("Hotclick (activate with a click)"), NULL) \
  LNGVALUE(InputHotclicksMore,         TEXT("A checked button can be combined with an action but it will always be blocked in this case."), NULL) \
  LNGVALUE(InputHotkeysAlt,            TEXT("Alt"), NULL) \
  LNGVALUE(InputHotkeysWinkey,         TEXT("Winkey"), NULL) \
  LNGVALUE(InputHotkeysCtrl,           TEXT("Ctrl"), NULL) \
  LNGVALUE(InputHotkeysShift,          TEXT("Shift"), NULL) \
  LNGVALUE(InputHotkeysShortcuts,      TEXT("S&hortcut for action:"), NULL) \
  LNGVALUE(InputHotkeysShortcutsPick,  TEXT("Pick &keys"), NULL) \
  LNGVALUE(InputHotkeysShortcutsClear, TEXT("Clear ke&ys"), NULL) \
  LNGVALUE(InputHotkeysShortcutsSet,   TEXT("Sa&ve"), NULL) \
  LNGVALUE(InputHotkeysUsePtWindow,    TEXT("Apply to the &pointed window"), NULL) \
  LNGVALUE(InputHotkeysLeftAlt,        TEXT("L&eft Alt"), NULL) \
  LNGVALUE(InputHotkeysRightAlt,       TEXT("&Right Alt"), NULL) \
  LNGVALUE(InputHotkeysLeftWinkey,     TEXT("Left &Winkey"), NULL) \
  LNGVALUE(InputHotkeysRightWinkey,    TEXT("Right W&inkey"), NULL) \
  LNGVALUE(InputHotkeysLeftCtrl,       TEXT("&Left Ctrl"), NULL) \
  LNGVALUE(InputHotkeysRightCtrl,      TEXT("Ri&ght Ctrl"), NULL) \
  LNGVALUE(InputHotkeysLeftShift,      TEXT("Left Shift"), NULL) \
  LNGVALUE(InputHotkeysRightShift,     TEXT("Right Shift"), NULL) \
  LNGVALUE(InputHotkeysMore, TEXT("You can add any key by editing the ini file.\nUse the shift key to snap windows to each other."), NULL) \
  /* Blacklits Tab*/ \
  LNGVALUE(BlacklistBox,              TEXT("Blacklist settings"), NULL) \
  LNGVALUE(BlacklistFormat,           TEXT("exename.exe\r\ntitle|class\r\n*|class\r\nexename.exe:title|class"), NULL) \
  LNGVALUE(BlacklistProcessBlacklist, TEXT("&Process blacklist:"), TEXT("Globally applies ie:\r\n*|Notepad, Photoshop.exe, explorer.exe*|#32770")) \
  LNGVALUE(BlacklistBlacklist,        TEXT("&Windows blacklist:"), NULL) \
  LNGVALUE(BlacklistScrolllist,       TEXT("Windows that should ignore &scroll action:"), NULL) \
  LNGVALUE(BlacklistMDIs,             TEXT("&MDIs not to be treated as such:"), NULL) \
  LNGVALUE(BlacklistPause,            TEXT("Processes not to be pa&used or killed:"), NULL) \
  LNGVALUE(BlacklistFindWindowBox,    TEXT("Identify window"), NULL) \
  /* Advanced Tab */ \
  LNGVALUE(AdvancedMetricsBox,       TEXT("Metrics"), NULL) \
  LNGVALUE(AdvancedCenterFraction,   TEXT("&Center/Sides fraction (%):"), NULL) \
  LNGVALUE(AdvancedAeroHoffset,      TEXT("Aero offset(%) &Horizontal:"), NULL) \
  LNGVALUE(AdvancedAeroVoffset,      TEXT("&Vertical:"), NULL) \
  LNGVALUE(AdvancedSnapThreshold,    TEXT("&Snap Threshold (pixels):"), NULL) \
  LNGVALUE(AdvancedAeroThreshold,    TEXT("A&ero Threshold (pixels):"), NULL) \
  LNGVALUE(AdvancedSnapGap,          TEXT("Snapping &gap (pixels):"), NULL) \
  LNGVALUE(AdvancedAeroSpeed,        TEXT("Ma&x snapping speed (pixels):"), NULL) \
  LNGVALUE(AdvancedTestWindow,       TEXT("Test &Window"), NULL) \
  LNGVALUE(AdvancedMoveTrans,        TEXT("Opacit&y when moving:"), NULL) \
  LNGVALUE(AdvancedBehaviorBox,      TEXT("Behavior"), NULL) \
  LNGVALUE(AdvancedMultipleInstances,TEXT("Allow multiple &instances of AltSnap"), NULL) \
  LNGVALUE(AdvancedAutoRemaximize,   TEXT("Automatically &remaximize windows when changing monitor"), NULL) \
  LNGVALUE(AdvancedAeroTopMaximizes, TEXT("&Maximize windows snapped at top"), NULL) \
  LNGVALUE(AdvancedAeroDBClickShift, TEXT("Invert shift &behavior for double-click aero snapping"), NULL) \
  LNGVALUE(AdvancedMaxWithLClick,    TEXT("&Toggle maximize state with the resize button while moving"), NULL) \
  LNGVALUE(AdvancedRestoreOnClick,   TEXT("Rest&ore window with single click like original AltDrag"), NULL) \
  LNGVALUE(AdvancedFullScreen,       TEXT("Enable on &full screen windows"), NULL) \
  LNGVALUE(AdvancedBLMaximized,      TEXT("&Disable AltSnap on Maximized windows"), NULL) \
  LNGVALUE(AdvancedFancyZone,        TEXT("Restore Fancy&Zones snapped windows"), NULL) \
  LNGVALUE(AdvancedNoRestore,        TEXT("Never restore AltSna&pped windows"), NULL) \
  LNGVALUE(AdvancedTopmostIndicator, TEXT("Show an i&ndicator on always on top windows"), NULL) \
  /* About Tab */ \
  LNGVALUE(AboutBox,     TEXT("About ") APP_NAME , NULL) \
  LNGVALUE(AboutVersion, TEXT("Version ") TEXT( APP_VERSION ), NULL) \
  LNGVALUE(AboutAuthor,  TEXT("Created by Stefan Sundin"), NULL) \
  LNGVALUE(AboutAuthor2, TEXT("Maintained by Raymond Gillibert"), NULL) \
  LNGVALUE(AboutLicense, APP_NAME TEXT(" is free and open source software!\nFeel free to redistribute!"), NULL) \
  LNGVALUE(AboutTranslationCredit, TEXT("Translation credit"), NULL) \
  /* Misc */ \
  LNGVALUE(MiscUnhookError,      TEXT("There was an error disabling AltDrag. This was most likely caused by Windows having already disabled AltDrag's hooks.\n\nIf this is the first time this has happened, you can safely ignore it and continue using AltDrag.\n\nIf this is happening repeatedly, you can read on the website how to prevent this from happening again (look for 'AltDrag mysteriously stops working' in the documentation)."), NULL) \
  LNGVALUE(MiscZoneConfirmation, TEXT("Erase old snap layout and save current Test Windows positions as the new snap layout?"), NULL) \
  LNGVALUE(MiscZoneTestWinHelp,  TEXT("To setup Snap layout:\n1) Open several of those Test Windows\n2) Dispose them as you please\n3) Hit the *&Save test windows as snap layout* option in the tray menu"), NULL) \
  /* Extended character list  for each virtual key */ \
  LNGVALUE(a, TEXT("àáâäæãåª%āăąǎǟǡǣǻǽȁȃȧ|Ȧḁ%ⱥ|Ⱥɐ|Ɐɑ|Ɑɒ|Ɒⲁ|Ⲁⓐ"), NULL) \
  LNGVALUE(b, TEXT("%ƀɓƃƅɃ%ɓḃḅḇⓑ"), NULL) \
  LNGVALUE(c, TEXT("ç¢©%ćĉċčƈḉȼ|Ȼɕⓒ"), NULL) \
  LNGVALUE(d, TEXT("ð%ďđɖɗƌƍḋḍḏḑḓǆǅǳǲȡȸⓓ"), NULL) \
  LNGVALUE(e, TEXT("èéêë€%ēĕėęěǝəɛȅȇḕḗḙḛȩ|Ȩḝɇ|Ɇⱸⓔ"), NULL) \
  LNGVALUE(f, TEXT("ƒ%ḟɸⱷⓕ%♩♪♮♭♯♬♫"), NULL) \
  LNGVALUE(g, TEXT("%ǵǧḡɠɣǥⓖ"), NULL) \
  LNGVALUE(h, TEXT("%ĥħƕǶḣḥḧḩḫȟ|Ȟⱨ|Ⱨⱶ|Ⱶẖⓗ"), NULL) \
  LNGVALUE(i, TEXT("ìíîï%ĩīĭǐȉȋįİıĳɩɨḭḯⓘ"), NULL) \
  LNGVALUE(j, TEXT("%ĵǰȷɉ|Ɉⓙ"), NULL) \
  LNGVALUE(k, TEXT("%ķĸƙǩḱḳⱪ|Ⱪꝁ|Ꝁʞ|Ʞⓚ"), NULL) \
  LNGVALUE(l, TEXT("£%ĺļľŀłƛǉǈȴƚ|Ƚⱡ|Ⱡɫ|Ɫḷḹḻḽⓛ"), NULL) \
  LNGVALUE(m, TEXT("µ%ḿṁṃɱ|Ɱɯⓜ"), NULL) \
  LNGVALUE(n, TEXT("ñ%ńņňŉŋɲƞ|Ƞǌǋǹȵ%ṅṇṉṋⓝ"), NULL) \
  LNGVALUE(o, TEXT("òóôöœõø°%ōŏő%ɔɵơƣǒǫǭǿȍȏȣ|Ȣȫ|Ȫȭ|Ȭȯ|Ȯȱ|Ȱṍṏṑṓ%ⱺⓞ"), NULL) \
  LNGVALUE(p, TEXT("¶þ·•%ƥᵽ|Ᵽṕṗⓟ"), NULL) \
  LNGVALUE(q, TEXT("¿¤‰‘’“”„…–—«»‹›%ȹɋ|Ɋⓠ"), NULL) \
  LNGVALUE(r, TEXT("®%ŕŗřƦȑȓṙṛṝṟɍ|Ɍɽ|Ɽⱹⓡ"), NULL) \
  LNGVALUE(s, TEXT("šß§%śŝşſ%ƨʃƪș|Șȿ|Ȿ%ṡṣṥṧṩⓢ"), NULL) \
  LNGVALUE(t, TEXT("†‡™%ţťŧƫƭʈț|Țȶⱦ|Ⱦ%ṫṭṯṱẗⓣ"), NULL) \
  LNGVALUE(u, TEXT("ùúûü%ũūůŭűų%ưʊǔǖǘǚǜȕȗʉ|Ʉ%ṳṵṷṹṻⓤ"), NULL) \
  LNGVALUE(v, TEXT("%ʋɅⱱⱴ%ṽṿⓥ"), NULL) \
  LNGVALUE(w, TEXT("%ẁẃŵẅⱳ|Ⱳ%ẇẉⓦ"), NULL) \
  LNGVALUE(x, TEXT("±×÷¬%ẋẍⓧ"), NULL) \
  LNGVALUE(y, TEXT("ýÿ¥%ŷẏȳ|Ȳƴɏ|Ɏⓨ"), NULL) \
  LNGVALUE(z, TEXT("ž%źẑżẓẕ%ƶʒƹƺǯȥ|Ȥɀ|Ɀⱬ|Ⱬⓩ"), NULL)


#define LNGVALUE(x, y, z) TCHAR *x, *x##_T_T_;
// String structure definition TCHAR *translation, *tooltip;
struct strings { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z) #x,
// Name of values ini entries
static const char* l10n_inimapping[] = { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z) y, z,
// Default values en-US
static const struct strings en_US = { LANGUAGE_MAP };
#undef LANGVALUE

#endif
