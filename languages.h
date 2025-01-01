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
  LNGVALUE(code, "Code", TEXT("en-US"), NULL) \
  LNGVALUE(lang_english, "LangEnglish", TEXT("English"), NULL) \
  LNGVALUE(lang, "Lang", TEXT("English"), NULL) \
  LNGVALUE(author, "Author", TEXT("Stefan Sundin, RamonUnch"), NULL) \
  /* Menu */ \
  LNGVALUE(menu_enable,       "MenuEnable",        TEXT("&Enable"), NULL) \
  LNGVALUE(menu_disable,      "MenuDisable",       TEXT("&Disable"), NULL) \
  LNGVALUE(menu_hide,         "MenuHideTray",      TEXT("&Hide tray"), NULL) \
  LNGVALUE(menu_config,       "MenuConfigure",     TEXT("&Configure..."), NULL) \
  LNGVALUE(menu_about,        "MenuAbout",         TEXT("&About"), NULL) \
  LNGVALUE(menu_openinifile,  "MenuOpenIniFile",   TEXT("&Open ini file"), NULL) \
  LNGVALUE(menu_savezones,    "MenuSaveZones",     TEXT("&Save test windows as snap layout"), NULL) \
  LNGVALUE(menu_emptyzone,    "MenuEmptyZone",     TEXT("(empty)"), NULL) \
  LNGVALUE(menu_snaplayout,   "MenuSnapLayout",    TEXT("Snap layout &"), NULL) \
  LNGVALUE(menu_editlayout,   "MenuEditLayout",    TEXT("Edi&t snap layout"), NULL) \
  LNGVALUE(menu_closeallzones,"MenuCloseAllZones", TEXT("C&lose all test windows"), NULL) \
  LNGVALUE(menu_exit,         "MenuExit",          TEXT("E&xit"), NULL) \
  /* Config Title and Tabs */ \
  LNGVALUE(title, "ConfigTitle", APP_NAME TEXT(" Configuration"), NULL) \
  LNGVALUE(tab_general,   "ConfigTabGeneral",   TEXT("General"), NULL) \
  LNGVALUE(tab_mouse,     "ConfigTabMouse",     TEXT("Mouse"), NULL) \
  LNGVALUE(tab_keyboard,  "ConfigTabKeyboard",  TEXT("Keyboard"), NULL) \
  LNGVALUE(tab_blacklist, "ConfigTabBlacklist", TEXT("Blacklist"), NULL) \
  LNGVALUE(tab_advanced,  "ConfigTabAdvanced",  TEXT("Advanced"), NULL) \
  LNGVALUE(tab_about,     "ConfigTabAbout",     TEXT("About"), NULL) \
  /* General Box */ \
  LNGVALUE(general_box, "GeneralBox", TEXT("General settings"), NULL) \
  LNGVALUE(general_autofocus,      "GeneralAutoFocus",      TEXT("&Focus windows when dragging.\nYou can also press Ctrl to focus windows."), NULL) \
  LNGVALUE(general_aero,           "GeneralAero",           TEXT("Mimi&c Aero Snap"), NULL) \
  LNGVALUE(general_smartaero,      "GeneralSmartAero",      TEXT("Smart Aero Sna&p dimensions"), NULL) \
  LNGVALUE(general_smarteraero,    "GeneralSmarterAero",    TEXT("Smarter Aer&o Snap dimensions"), NULL) \
  LNGVALUE(general_stickyresize,   "GeneralStickyResize",   TEXT("Resi&ze other snapped windows with Shift"), NULL) \
  LNGVALUE(general_inactivescroll, "GeneralInactiveScroll", TEXT("&Scroll inactive windows"), NULL) \
  LNGVALUE(general_mdi,            "GeneralMDI",            TEXT("&MDI support"), NULL) \
  LNGVALUE(general_autosnap,       "GeneralAutoSnap",       TEXT("S&nap window edges to:"), NULL) \
  LNGVALUE(general_autosnap0,      "GeneralAutoSnap0",      TEXT("Disabled"), NULL) \
  LNGVALUE(general_autosnap1,      "GeneralAutoSnap1",      TEXT("To screen borders"), NULL) \
  LNGVALUE(general_autosnap2,      "GeneralAutoSnap2",      TEXT("+ outside of windows"), NULL) \
  LNGVALUE(general_autosnap3,      "GeneralAutoSnap3",      TEXT("+ inside of windows"), NULL) \
  LNGVALUE(general_language,       "GeneralLanguage",       TEXT("&Language:"), NULL) \
  LNGVALUE(general_fullwin,        "GeneralFullWin",        TEXT("&Drag full windows"), NULL) \
  LNGVALUE(general_usezones,       "GeneralUseZones",       TEXT("Snap to Layo&ut with Shift (configure with tray menu)"), TEXT("This option will mimick FancyZones\r\nOpen TestWindows to set up the zones\n Look at the tray icon menu")) \
  LNGVALUE(general_piercingclick,  "GeneralPiercingClick",  TEXT("Avoi&d blocking Alt+Click (disables AltSnap double-clicks)"), NULL) \
  LNGVALUE(general_resizeall,      "GeneralResizeAll",      TEXT("&Resize all windows"), NULL) \
  LNGVALUE(general_resizecenter,   "GeneralResizeCenter",   TEXT("Center resize mode"), NULL) \
  LNGVALUE(general_resizecenter_norm, "GeneralResizeCenterNorm", TEXT("All d&irections"), NULL) \
  LNGVALUE(general_resizecenter_br,   "GeneralResizeCenterBr",   TEXT("&Bottom right"), NULL) \
  LNGVALUE(general_resizecenter_move, "GeneralResizeCenterMove", TEXT("Mo&ve"), NULL) \
  LNGVALUE(general_resizecenter_close,"GeneralResizeCenterClose",TEXT("Clos&est side"), NULL) \
  LNGVALUE(general_autostart_box,         "GeneralAutostartBox",        TEXT("Autostart"), NULL) \
  LNGVALUE(general_autostart,             "GeneralAutostart",           TEXT("S&tart ") APP_NAME TEXT(" when logging on"), NULL) \
  LNGVALUE(general_autostart_hide,        "GeneralAutostartHide",       TEXT("&Hide tray"), NULL) \
  LNGVALUE(general_autostart_elevate,     "GeneralAutostartElevate",    TEXT("&Elevate to administrator privileges"), NULL) \
  LNGVALUE(general_autostart_elevate_tip, "GeneralAutostartElevateTip", TEXT("Note that a UAC prompt will appear every time you log in, unless you disable UAC completely or use the Task Scheduler.\nTo setup a Scheduled task for this purpose, you can use the sch_On.bat batch files in Altsnap's folder."), NULL) \
  LNGVALUE(general_elevate,               "GeneralElevate",             TEXT("E&levate"), NULL) \
  LNGVALUE(general_elevated,              "GeneralElevated",            TEXT("Elevated"), NULL) \
  LNGVALUE(general_elevation_aborted,     "GeneralElevationAborted",    TEXT("Elevation aborted."), NULL) \
  /* Keyboard & Mouse tabs */ \
  LNGVALUE(input_mouse_box, "InputMouseBox", TEXT("Mouse actions"), NULL) \
  LNGVALUE(input_mouse_btac1,   "InputMouseBtAc1",   TEXT("&1. Primary"), TEXT("Action performed with Hotkey")) \
  LNGVALUE(input_mouse_btac2,   "InputMouseBtAc2",   TEXT("&2. Alternate"), TEXT("Action performed with Hotkey + Modifier key (see Keyboard tab)")) \
  LNGVALUE(input_mouse_inttb,   "InputMouseINTTB",   TEXT("&Title bar"), TEXT("Action performed when clicking on the title bar")) \
  LNGVALUE(input_mouse_whilem,  "InputMouseWhileM",  TEXT("Whil&e moving"), NULL) \
  LNGVALUE(input_mouse_whiler,  "InputMouseWhileR",  TEXT("While resi&zing"), NULL) \
  LNGVALUE(input_mouse_lmb,     "InputMouseLMB",     TEXT("Left mouse &button:"), NULL) \
  LNGVALUE(input_mouse_mmb,     "InputMouseMMB",     TEXT("&Middle mouse button:"), NULL) \
  LNGVALUE(input_mouse_rmb,     "InputMouseRMB",     TEXT("Ri&ght mouse button:"), NULL) \
  LNGVALUE(input_mouse_mb4,     "InputMouseMB4",     TEXT("Mouse button &4:"), NULL) \
  LNGVALUE(input_mouse_mb5,     "InputMouseMB5",     TEXT("Mouse button &5:"), NULL) \
  LNGVALUE(input_mouse_scroll,  "InputMouseScroll",  TEXT("&Scroll wheel:"), NULL) \
  LNGVALUE(input_mouse_hscroll, "InputMouseHScroll", TEXT("Scroll wheel (&horizontal):"), NULL) \
  LNGVALUE(input_mouse_moveup,  "InputMouseMoveUp",  TEXT("Long &drag-free Move:"), NULL) \
  LNGVALUE(input_mouse_resizeup,"InputMouseResizeUp",TEXT("Long drag-&free Resize:"), NULL) \
  LNGVALUE(input_mouse_ttbactions_box, "InputMouseTTBActionBox", TEXT("Use specific actions when clicking the Title bar"), NULL) \
  LNGVALUE(input_mouse_ttbactionsna,   "InputMouseTTBActionNA",  TEXT("Without hot&key"), NULL) \
  LNGVALUE(input_mouse_ttbactionswa,   "InputMouseTTBActionWA",  TEXT("&With hotkey"), NULL) \
  LNGVALUE(input_mouse_mmb_hc, "InputMouseMMBHC", TEXT("M&iddle mouse button"), NULL) \
  LNGVALUE(input_mouse_mb4_hc, "InputMouseMB4HC", TEXT("M&ouse button 4"), NULL) \
  LNGVALUE(input_mouse_mb5_hc, "InputMouseMB5HC", TEXT("Mo&use button 5"), NULL) \
  LNGVALUE(input_mouse_longclickmove, "InputMouseLongClickMove", TEXT("Mo&ve windows with a long left-click"), NULL) \
  LNGVALUE(input_scrolllockstate,     "InputScrollLockState",    TEXT("Suspend/Resume AltSnap based on &Scroll lock state"), NULL) \
  LNGVALUE(input_unikeyholdmenu,      "InputUniKeyHoldMenu",     TEXT("Pop&up an extended character menu when holding an alphabetic key down"), NULL) \
  LNGVALUE(input_keycombo,          "InputKeyCombo",        TEXT("Use two keys &combo to activate"), NULL) \
  LNGVALUE(input_grabwithalt,       "InputGrabWithAlt",     TEXT("&Action without click:"), NULL) \
  LNGVALUE(input_grabwithaltb,      "InputGrabWithAltB",    TEXT("Acti&on without click (alt):"), NULL) \
  LNGVALUE(input_actions_move,      "InputActionMove",      TEXT("Move window"), NULL) \
  LNGVALUE(input_actions_resize,    "InputActionResize",    TEXT("Resize window"), NULL) \
  LNGVALUE(input_actions_restore,   "InputActionRestore",   TEXT("Restore window"), NULL) \
  LNGVALUE(input_actions_close,     "InputActionClose",     TEXT("&Close window"), NULL) \
  LNGVALUE(input_actions_kill,      "InputActionKill",      TEXT("&Kill program"), NULL) \
  LNGVALUE(input_actions_pause,     "InputActionPause",     TEXT("Pause program"), NULL) \
  LNGVALUE(input_actions_resume,    "InputActionResume",    TEXT("Resume program"), NULL) \
  LNGVALUE(input_actions_asonoff,   "InputActionASOnOff",   TEXT("S&uspend/Resume AltSnap"), NULL) \
  LNGVALUE(input_actions_moveonoff, "InputActionMoveOnOff", TEXT("Movement dis&abled"), NULL) \
  LNGVALUE(input_actions_minimize,  "InputActionMinimize",  TEXT("Mi&nimize window"), NULL) \
  LNGVALUE(input_actions_maximize,  "InputActionMaximize",  TEXT("Ma&ximize window"), NULL) \
  LNGVALUE(input_actions_lower,     "InputActionLower",     TEXT("&Lower window"), NULL) \
  LNGVALUE(input_actions_focus,     "InputActionFocus",     TEXT("Focus window"), NULL) \
  LNGVALUE(input_actions_nstacked,  "InputActionNStacked",  TEXT("Next stacked window"), NULL) \
  LNGVALUE(input_actions_nstacked2, "InputActionNStacked2", TEXT("Next laser stacked window"), NULL) \
  LNGVALUE(input_actions_pstacked,       "InputActionPStacked",   TEXT("Previous stacked window"), NULL) \
  LNGVALUE(input_actions_pstacked2,      "InputActionPStacked2",  TEXT("Previous laser stacked window"), NULL) \
  LNGVALUE(input_actions_npstacked,      "InputActionNPStacked",  TEXT("Next/Prev stacked window"), NULL) \
  LNGVALUE(input_actions_npstacked2,     "InputActionNPStacked2", TEXT("Next/Prev laser stacked window"), NULL) \
  LNGVALUE(input_actions_stacklist,      "InputActionStackList",  TEXT("Stacked windows list"), NULL) \
  LNGVALUE(input_actions_stacklist2,     "InputActionStackList2", TEXT("Laser stacked windows list"), NULL) \
  LNGVALUE(input_actions_alttablist,     "InputActionAltTabList", TEXT("Windows list"), NULL) \
  LNGVALUE(input_actions_alttabfulllist, "InputActionAltTabFullList", TEXT("All windows list"), NULL) \
  LNGVALUE(input_actions_mlzone,   "InputActionMLZone",   TEXT("Move to the left zone"), NULL) \
  LNGVALUE(input_actions_mtzone,   "InputActionMTZone",   TEXT("Move to the top zone"), NULL) \
  LNGVALUE(input_actions_mrzone,   "InputActionMRZone",   TEXT("Move to the right zone"), NULL) \
  LNGVALUE(input_actions_mbzone,   "InputActionMBZone",   TEXT("Move to the bottom zone"), NULL) \
  LNGVALUE(input_actions_xlzone,   "InputActionXLZone",   TEXT("Extend to the left zone"), NULL) \
  LNGVALUE(input_actions_xtzone,   "InputActionXTZone",   TEXT("Extend to the top zone"), NULL) \
  LNGVALUE(input_actions_xrzone,   "InputActionXRZone",   TEXT("Extend to the right zone"), NULL) \
  LNGVALUE(input_actions_xbzone,   "InputActionXBZone",   TEXT("Extend to the bottom zone"), NULL) \
  LNGVALUE(input_actions_xtnledge, "InputActionXTNLEdge", TEXT("Extend to the next left edge"), NULL) \
  LNGVALUE(input_actions_xtntedge, "InputActionXTNTEdge", TEXT("Extend to the next top edge"), NULL) \
  LNGVALUE(input_actions_xtnredge, "InputActionXTNREdge", TEXT("Extend to the next right edge"), NULL) \
  LNGVALUE(input_actions_xtnbedge, "InputActionXTNBEdge", TEXT("Extend to the next bottom edge"), NULL) \
  LNGVALUE(input_actions_mtnledge, "InputActionMTNLEdge", TEXT("Move to the next left edge"), NULL) \
  LNGVALUE(input_actions_mtntedge, "InputActionMTNTEdge", TEXT("Move to the next top edge"), NULL) \
  LNGVALUE(input_actions_mtnredge, "InputActionMTNREdge", TEXT("Move to the next right edge"), NULL) \
  LNGVALUE(input_actions_mtnbedge, "InputActionMTNBEdge", TEXT("Move to the next bottom edge"), NULL) \
  LNGVALUE(input_actions_stepl,    "InputActionStepl",    TEXT("Step left"), NULL) \
  LNGVALUE(input_actions_stept,    "InputActionStepT",    TEXT("Step up"), NULL) \
  LNGVALUE(input_actions_stepr,    "InputActionStepR",    TEXT("Step right"), NULL) \
  LNGVALUE(input_actions_stepb,    "InputActionStepB",    TEXT("Step down"), NULL) \
  LNGVALUE(input_actions_sstepl,   "InputActionSStepl",   TEXT("Small step left"), NULL) \
  LNGVALUE(input_actions_sstept,   "InputActionSStepT",   TEXT("Small step up"), NULL) \
  LNGVALUE(input_actions_sstepr,   "InputActionSStepR",   TEXT("Small step right"), NULL) \
  LNGVALUE(input_actions_sstepb,   "InputActionSStepB",   TEXT("Small step down"), NULL) \
  LNGVALUE(input_actions_focusl,   "InputActionFocusL",   TEXT("Focus left window"), NULL) \
  LNGVALUE(input_actions_focust,   "InputActionFocusT",   TEXT("Focus top window"), NULL) \
  LNGVALUE(input_actions_focusr,   "InputActionFocusR",   TEXT("Focus right window"), NULL) \
  LNGVALUE(input_actions_focusb,   "InputActionFocusB",   TEXT("Focus bottom window"), NULL) \
  LNGVALUE(input_actions_roll,     "InputActionRoll",     TEXT("&Roll/Unroll window"), NULL) \
  LNGVALUE(input_actions_alwaysontop,  "InputActionAlwaysOnTop",  TEXT("Toggle always on &top"), NULL) \
  LNGVALUE(input_actions_borderless,   "InputActionBorderless",   TEXT("Toggle &borderless"), NULL) \
  LNGVALUE(input_actions_center,       "InputActionCenter",       TEXT("C&enter window on screen"), NULL) \
  LNGVALUE(input_actions_oriclick,     "InputActionOriClick",     TEXT("Send ori&ginal click"), NULL) \
  LNGVALUE(input_actions_nothing,      "InputActionNothing",      TEXT("Nothing"), NULL) \
  LNGVALUE(input_actions_alttab,       "InputActionAltTab",       TEXT("Alt+Tab"), NULL) \
  LNGVALUE(input_actions_volume,       "InputActionVolume",       TEXT("Volume"), NULL) \
  LNGVALUE(input_actions_mute,         "InputActionMute",         TEXT("Mute &sounds"), NULL) \
  LNGVALUE(input_actions_menu,         "InputActionMenu",         TEXT("Action menu"), NULL) \
  LNGVALUE(input_actions_maximizehv,   "InputActionMaximizeHV",   TEXT("Maximize &Vertically"), NULL) \
  LNGVALUE(input_actions_sidesnap,     "InputActionSideSnap",     TEXT("&Snap to monitor side/corner"), NULL) \
  LNGVALUE(input_actions_extendsnap ,  "InputActionExtendSnap",   TEXT("Extend to monitor side/corner"), NULL) \
  LNGVALUE(input_actions_extendtnedge, "InputActionExtendTNEdge", TEXT("Extend to next edge"), NULL) \
  LNGVALUE(input_actions_movetnedge,   "InputActionMoveTNEdge",   TEXT("Move to next edge"), NULL) \
  LNGVALUE(input_actions_minallother,  "InputActionMinAllOther",  TEXT("Minimize &other windows"), NULL) \
  LNGVALUE(input_actions_transparency, "InputActionTransparency", TEXT("Transparency"), NULL) \
  LNGVALUE(input_actions_zoom,         "InputActionZoom",         TEXT("Zoom window"), NULL) \
  LNGVALUE(input_actions_zoom2,        "InputActionZoom2",        TEXT("Zoom window 2"), NULL) \
  LNGVALUE(input_actions_hscroll,      "InputActionHScroll",      TEXT("Horizontal scroll"), NULL) \
  \
  LNGVALUE(input_hotkeys_box,     "InputHotkeysBox", TEXT("Hotkeys"), NULL) \
  LNGVALUE(input_hotkeys_modkey,  "InputHotkeysModKey", TEXT("Modifier key for al&ternate action:"), NULL) \
  LNGVALUE(input_hotclicks_box,   "InputHotclicksBox", TEXT("Hotclick (activate with a click)"), NULL) \
  LNGVALUE(input_hotclicks_more,  "InputHotclicksMore", TEXT("A checked button can be combined with an action but it will always be blocked in this case."), NULL) \
  LNGVALUE(input_hotkeys_alt,     "InputHotkeysAlt", TEXT("Alt"), NULL) \
  LNGVALUE(input_hotkeys_winkey,  "InputHotkeysWinkey", TEXT("Winkey"), NULL) \
  LNGVALUE(input_hotkeys_ctrl,    "InputHotkeysCtrl", TEXT("Ctrl"), NULL) \
  LNGVALUE(input_hotkeys_shift,   "InputHotkeysShift", TEXT("Shift"), NULL) \
  LNGVALUE(input_hotkeys_shortcuts,      "InputHotkeysShortcuts",      TEXT("S&hortcut for action:"), NULL) \
  LNGVALUE(input_hotkeys_shortcutspick,  "InputHotkeysShortcutsPick",  TEXT("Pick &keys"), NULL) \
  LNGVALUE(input_hotkeys_shortcutsclear, "InputHotkeysShortcutsClear", TEXT("Clear ke&ys"), NULL) \
  LNGVALUE(input_hotkeys_shortcutset,    "InputHotkeysShortcutsSet",   TEXT("Sa&ve"), NULL) \
  LNGVALUE(input_hotkeys_useptwindow,    "InputHotkeysUsePtWindow",    TEXT("Apply to the &pointed window"), NULL) \
  LNGVALUE(input_hotkeys_leftalt,        "InputHotkeysLeftAlt",        TEXT("L&eft Alt"), NULL) \
  LNGVALUE(input_hotkeys_rightalt,       "InputHotkeysRightAlt",       TEXT("&Right Alt"), NULL) \
  LNGVALUE(input_hotkeys_leftwinkey,     "InputHotkeysLeftWinkey",     TEXT("Left &Winkey"), NULL) \
  LNGVALUE(input_hotkeys_rightwinkey,    "InputHotkeysRightWinkey",    TEXT("Right W&inkey"), NULL) \
  LNGVALUE(input_hotkeys_leftctrl,       "InputHotkeysLeftCtrl",       TEXT("&Left Ctrl"), NULL) \
  LNGVALUE(input_hotkeys_rightctrl,      "InputHotkeysRightCtrl",      TEXT("Ri&ght Ctrl"), NULL) \
  LNGVALUE(input_hotkeys_leftshift,      "InputHotkeysLeftShift",      TEXT("Left Shift"), NULL) \
  LNGVALUE(input_hotkeys_rightshift,     "InputHotkeysRightShift",     TEXT("Right Shift"), NULL) \
  LNGVALUE(input_hotkeys_more, "InputHotkeysMore", TEXT("You can add any key by editing the ini file.\nUse the shift key to snap windows to each other."), NULL) \
  /* Blacklits Tab*/ \
  LNGVALUE(blacklist_box,              "BlacklistBox",              TEXT("Blacklist settings"), NULL) \
  LNGVALUE(blacklist_processblacklist, "BlacklistProcessBlacklist", TEXT("&Process blacklist:"), TEXT("Globally applies\r\nComa separated blacklist entries\r\nFormats:\r\nexename.exe\r\ntitle|class\r\nexename.exe:title|class\r\nyou can use the * as a joker ie:\r\n*|Notepad, Photoshop.exe, explorer.exe*|#32770")) \
  LNGVALUE(blacklist_blacklist,        "BlacklistBlacklist",        TEXT("&Windows blacklist:"), NULL) \
  LNGVALUE(blacklist_scrolllist,       "BlacklistScrolllist",       TEXT("Windows that should ignore &scroll action:"), NULL) \
  LNGVALUE(blacklist_mdis,             "BlacklistMDIs",             TEXT("&MDIs not to be treated as such:"), NULL) \
  LNGVALUE(blacklist_pause,            "BlacklistPause",            TEXT("Processes not to be pa&used or killed:"), NULL) \
  LNGVALUE(blacklist_findwindow_box,   "BlacklistFindWindowBox",    TEXT("Identify window"), NULL) \
  /* Advanced Tab */ \
  LNGVALUE(advanced_metrics_box,      "AdvancedMetricsBox",       TEXT("Metrics"), NULL) \
  LNGVALUE(advanced_centerfraction,   "AdvancedCenterFraction",   TEXT("&Center/Sides fraction (%):"), NULL) \
  LNGVALUE(advanced_aerohoffset,      "AdvancedAeroHoffset",      TEXT("Aero offset(%) &Horizontal:"), NULL) \
  LNGVALUE(advanced_aerovoffset,      "AdvancedAeroVoffset",      TEXT("&Vertical:"), NULL) \
  LNGVALUE(advanced_snapthreshold,    "AdvancedSnapThreshold",    TEXT("&Snap Threshold (pixels):"), NULL) \
  LNGVALUE(advanced_aerothreshold,    "AdvancedAeroThreshold",    TEXT("A&ero Threshold (pixels):"), NULL) \
  LNGVALUE(advanced_snapgap,          "AdvancedSnapGap",          TEXT("Snapping &gap (pixels):"), NULL) \
  LNGVALUE(advanced_aerospeed,        "AdvancedAeroSpeed",        TEXT("Ma&x snapping speed (pixels):"), NULL) \
  LNGVALUE(advanced_testwindow,       "AdvancedTestWindow",       TEXT("Test &Window"), NULL) \
  LNGVALUE(advanced_movetrans,        "AdvancedMoveTrans",        TEXT("Opacit&y when moving:"), NULL) \
  LNGVALUE(advanced_behavior_box,     "AdvancedBehaviorBox",      TEXT("Behavior"), NULL) \
  LNGVALUE(advanced_multipleinstances,"AdvancedMultipleInstances",TEXT("Allow multiple &instances of AltSnap"), NULL) \
  LNGVALUE(advanced_autoremaximize,   "AdvancedAutoRemaximize",   TEXT("Automatically &remaximize windows when changing monitor"), NULL) \
  LNGVALUE(advanced_aerotopmaximizes, "AdvancedAeroTopMaximizes", TEXT("&Maximize windows snapped at top"), NULL) \
  LNGVALUE(advanced_aerodbclickshift, "AdvancedAeroDBClickShift", TEXT("Invert shift &behavior for double-click aero snapping"), NULL) \
  LNGVALUE(advanced_maxwithlclick,    "AdvancedMaxWithLClick",    TEXT("&Toggle maximize state with the resize button while moving"), NULL) \
  LNGVALUE(advanced_restoreonclick,   "AdvancedRestoreOnClick",   TEXT("Rest&ore window with single click like original AltDrag"), NULL) \
  LNGVALUE(advanced_fullscreen,       "AdvancedFullScreen",       TEXT("Enable on &full screen windows"), NULL) \
  LNGVALUE(advanced_blmaximized,      "AdvancedBLMaximized",      TEXT("&Disable AltSnap on Maximized windows"), NULL) \
  LNGVALUE(advanced_fancyzone,        "AdvancedFancyZone",        TEXT("Restore Fancy&Zones snapped windows"), NULL) \
  LNGVALUE(advanced_norestore,        "AdvancedNoRestore",        TEXT("Never restore AltSna&pped windows"), NULL) \
  LNGVALUE(advanced_topmostindicator, "AdvancedTopmostIndicator", TEXT("Show an i&ndicator on always on top windows"), NULL) \
  /* About Tab */ \
  LNGVALUE(about_box,     "AboutBox",      TEXT("About ") APP_NAME , NULL) \
  LNGVALUE(about_version, "AboutVersion",  TEXT("Version ") TEXT( APP_VERSION ), NULL) \
  LNGVALUE(about_author,  "AboutAuthor",   TEXT("Created by Stefan Sundin"), NULL) \
  LNGVALUE(about_author2, "AboutAuthor2",  TEXT("Maintained by Raymond Gillibert"), NULL) \
  LNGVALUE(about_license, "AboutLicense",  APP_NAME TEXT(" is free and open source software!\nFeel free to redistribute!"), NULL) \
  LNGVALUE(about_translation_credit, "AboutTranslationCredit", TEXT("Translation credit"), NULL) \
  /* Misc */ \
  LNGVALUE(unhook_error,      "MiscUnhookError",      TEXT("There was an error disabling AltDrag. This was most likely caused by Windows having already disabled AltDrag's hooks.\n\nIf this is the first time this has happened, you can safely ignore it and continue using AltDrag.\n\nIf this is happening repeatedly, you can read on the website how to prevent this from happening again (look for 'AltDrag mysteriously stops working' in the documentation)."), NULL) \
  LNGVALUE(zone_confirmation, "MiscZoneConfirmation", TEXT("Erase old snap layout and save current Test Windows positions as the new snap layout?"), NULL) \
  LNGVALUE(zone_testwinhelp,  "MiscZoneTestWinHelp",  TEXT("To setup Snap layout:\n1) Open several of those Test Windows\n2) Dispose them as you please\n3) Hit the *&Save test windows as snap layout* option in the tray menu"), NULL) \
  /* Extended character list  for each virtual key */ \
  LNGVALUE(a, "A", TEXT("àáâäæãåª%āăąǎǟǡǣǻǽȁȃȧ|Ȧḁ%ⱥ|Ⱥɐ|Ɐɑ|Ɑɒ|Ɒⲁ|Ⲁⓐ"), NULL) \
  LNGVALUE(b, "B", TEXT("%ƀɓƃƅɃ%ɓḃḅḇⓑ"), NULL) \
  LNGVALUE(c, "C", TEXT("ç¢©%ćĉċčƈḉȼ|Ȼɕⓒ"), NULL) \
  LNGVALUE(d, "D", TEXT("ð%ďđɖɗƌƍḋḍḏḑḓǆǅǳǲȡȸⓓ"), NULL) \
  LNGVALUE(e, "E", TEXT("èéêë€%ēĕėęěǝəɛȅȇḕḗḙḛȩ|Ȩḝɇ|Ɇⱸⓔ"), NULL) \
  LNGVALUE(f, "F", TEXT("ƒ%ḟɸⱷⓕ%♩♪♮♭♯♬♫"), NULL) \
  LNGVALUE(g, "G", TEXT("%ǵǧḡɠɣǥⓖ"), NULL) \
  LNGVALUE(h, "H", TEXT("%ĥħƕǶḣḥḧḩḫȟ|Ȟⱨ|Ⱨⱶ|Ⱶẖⓗ"), NULL) \
  LNGVALUE(i, "I", TEXT("ìíîï%ĩīĭǐȉȋįİıĳɩɨḭḯⓘ"), NULL) \
  LNGVALUE(j, "J", TEXT("%ĵǰȷɉ|Ɉⓙ"), NULL) \
  LNGVALUE(k, "K", TEXT("%ķĸƙǩḱḳⱪ|Ⱪꝁ|Ꝁʞ|Ʞⓚ"), NULL) \
  LNGVALUE(l, "L", TEXT("£%ĺļľŀłƛǉǈȴƚ|Ƚⱡ|Ⱡɫ|Ɫḷḹḻḽⓛ"), NULL) \
  LNGVALUE(m, "M", TEXT("µ%ḿṁṃɱ|Ɱɯⓜ"), NULL) \
  LNGVALUE(n, "N", TEXT("ñ%ńņňŉŋɲƞ|Ƞǌǋǹȵ%ṅṇṉṋⓝ"), NULL) \
  LNGVALUE(o, "O", TEXT("òóôöœõø°%ōŏő%ɔɵơƣǒǫǭǿȍȏȣ|Ȣȫ|Ȫȭ|Ȭȯ|Ȯȱ|Ȱṍṏṑṓ%ⱺⓞ"), NULL) \
  LNGVALUE(p, "P", TEXT("¶þ·•%ƥᵽ|Ᵽṕṗⓟ"), NULL) \
  LNGVALUE(q, "Q", TEXT("¿¤‰‘’“”„…–—«»‹›%ȹɋ|Ɋⓠ"), NULL) \
  LNGVALUE(r, "R", TEXT("®%ŕŗřƦȑȓṙṛṝṟɍ|Ɍɽ|Ɽⱹⓡ"), NULL) \
  LNGVALUE(s, "S", TEXT("šß§%śŝşſ%ƨʃƪș|Șȿ|Ȿ%ṡṣṥṧṩⓢ"), NULL) \
  LNGVALUE(t, "T", TEXT("†‡™%ţťŧƫƭʈț|Țȶⱦ|Ⱦ%ṫṭṯṱẗⓣ"), NULL) \
  LNGVALUE(u, "U", TEXT("ùúûü%ũūůŭűų%ưʊǔǖǘǚǜȕȗʉ|Ʉ%ṳṵṷṹṻⓤ"), NULL) \
  LNGVALUE(v, "V", TEXT("%ʋɅⱱⱴ%ṽṿⓥ"), NULL) \
  LNGVALUE(w, "W", TEXT("%ẁẃŵẅⱳ|Ⱳ%ẇẉⓦ"), NULL) \
  LNGVALUE(x, "X", TEXT("±×÷¬%ẋẍⓧ"), NULL) \
  LNGVALUE(y, "Y", TEXT("ýÿ¥%ŷẏȳ|Ȳƴɏ|Ɏⓨ"), NULL) \
  LNGVALUE(z, "Z", TEXT("ž%źẑżẓẕ%ƶʒƹƺǯȥ|Ȥɀ|Ɀⱬ|Ⱬⓩ"), NULL)


#define LNGVALUE(x, y, z, t) TCHAR *x, *x##T;
// String structure definition TCHAR *translation, *tooltip;
struct strings { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z, t) y,
// Name of values ini entries
static const char* l10n_inimapping[] = { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z, t) z, t,
// Default values en-US
static const struct strings en_US = { LANGUAGE_MAP };
#undef LANGVALUE

#endif
