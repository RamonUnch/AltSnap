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
  LNGVALUE(code, "Code", TEXT("en-US")) \
  LNGVALUE(lang_english, "LangEnglish", TEXT("English")) \
  LNGVALUE(lang, "Lang", TEXT("English")) \
  LNGVALUE(author, "Author", TEXT("Stefan Sundin, RamonUnch")) \
  /* Menu */ \
  LNGVALUE(menu_enable,       "MenuEnable",        TEXT("&Enable")) \
  LNGVALUE(menu_disable,      "MenuDisable",       TEXT("&Disable")) \
  LNGVALUE(menu_hide,         "MenuHideTray",      TEXT("&Hide tray")) \
  LNGVALUE(menu_config,       "MenuConfigure",     TEXT("&Configure...")) \
  LNGVALUE(menu_about,        "MenuAbout",         TEXT("&About")) \
  LNGVALUE(menu_openinifile,  "MenuOpenIniFile",   TEXT("&Open ini file")) \
  LNGVALUE(menu_savezones,    "MenuSaveZones",     TEXT("&Save test windows as snap layout")) \
  LNGVALUE(menu_emptyzone,    "MenuEmptyZone",     TEXT("(empty)")) \
  LNGVALUE(menu_snaplayout,   "MenuSnapLayout",    TEXT("Snap layout &")) \
  LNGVALUE(menu_editlayout,   "MenuEditLayout",    TEXT("Edi&t snap layout")) \
  LNGVALUE(menu_closeallzones,"MenuCloseAllZones", TEXT("C&lose all test windows")) \
  LNGVALUE(menu_exit,         "MenuExit",          TEXT("E&xit")) \
  /* Config Title and Tabs */ \
  LNGVALUE(title, "ConfigTitle", APP_NAME TEXT(" Configuration")) \
  LNGVALUE(tab_general,   "ConfigTabGeneral",   TEXT("General")) \
  LNGVALUE(tab_mouse,     "ConfigTabMouse",     TEXT("Mouse")) \
  LNGVALUE(tab_keyboard,  "ConfigTabKeyboard",  TEXT("Keyboard")) \
  LNGVALUE(tab_blacklist, "ConfigTabBlacklist", TEXT("Blacklist")) \
  LNGVALUE(tab_advanced,  "ConfigTabAdvanced",  TEXT("Advanced")) \
  LNGVALUE(tab_about,     "ConfigTabAbout",     TEXT("About")) \
  /* General Box */ \
  LNGVALUE(general_box, "GeneralBox", TEXT("General settings")) \
  LNGVALUE(general_autofocus,      "GeneralAutoFocus",      TEXT("&Focus windows when dragging.\nYou can also press Ctrl to focus windows.")) \
  LNGVALUE(general_aero,           "GeneralAero",           TEXT("Mimi&c Aero Snap")) \
  LNGVALUE(general_smartaero,      "GeneralSmartAero",      TEXT("Smart Aero Sna&p dimensions")) \
  LNGVALUE(general_smarteraero,    "GeneralSmarterAero",    TEXT("Smarter Aer&o Snap dimensions")) \
  LNGVALUE(general_stickyresize,   "GeneralStickyResize",   TEXT("Resi&ze other snapped windows with Shift")) \
  LNGVALUE(general_inactivescroll, "GeneralInactiveScroll", TEXT("&Scroll inactive windows")) \
  LNGVALUE(general_mdi,            "GeneralMDI",            TEXT("&MDI support")) \
  LNGVALUE(general_autosnap,       "GeneralAutoSnap",       TEXT("S&nap window edges to:")) \
  LNGVALUE(general_autosnap0,      "GeneralAutoSnap0",      TEXT("Disabled")) \
  LNGVALUE(general_autosnap1,      "GeneralAutoSnap1",      TEXT("To screen borders")) \
  LNGVALUE(general_autosnap2,      "GeneralAutoSnap2",      TEXT("+ outside of windows")) \
  LNGVALUE(general_autosnap3,      "GeneralAutoSnap3",      TEXT("+ inside of windows")) \
  LNGVALUE(general_language,       "GeneralLanguage",       TEXT("&Language:")) \
  LNGVALUE(general_fullwin,        "GeneralFullWin",        TEXT("&Drag full windows")) \
  LNGVALUE(general_usezones,       "GeneralUseZones",       TEXT("Snap to Layo&ut with Shift (configure with tray menu)")) \
  LNGVALUE(general_piercingclick,  "GeneralPiercingClick",  TEXT("Avoi&d blocking Alt+Click (disables AltSnap double-clicks)")) \
  LNGVALUE(general_resizeall,      "GeneralResizeAll",      TEXT("&Resize all windows")) \
  LNGVALUE(general_resizecenter,   "GeneralResizeCenter",   TEXT("Center resize mode")) \
  LNGVALUE(general_resizecenter_norm, "GeneralResizeCenterNorm", TEXT("All d&irections")) \
  LNGVALUE(general_resizecenter_br,   "GeneralResizeCenterBr",   TEXT("&Bottom right")) \
  LNGVALUE(general_resizecenter_move, "GeneralResizeCenterMove", TEXT("Mo&ve")) \
  LNGVALUE(general_resizecenter_close,"GeneralResizeCenterClose",TEXT("Clos&est side")) \
  LNGVALUE(general_autostart_box,         "GeneralAutostartBox",        TEXT("Autostart")) \
  LNGVALUE(general_autostart,             "GeneralAutostart",           TEXT("S&tart ") APP_NAME TEXT(" when logging on")) \
  LNGVALUE(general_autostart_hide,        "GeneralAutostartHide",       TEXT("&Hide tray")) \
  LNGVALUE(general_autostart_elevate,     "GeneralAutostartElevate",    TEXT("&Elevate to administrator privileges")) \
  LNGVALUE(general_autostart_elevate_tip, "GeneralAutostartElevateTip", TEXT("Note that a UAC prompt will appear every time you log in, unless you disable UAC completely or use the Task Scheduler.\nTo setup a Scheduled task for this purpose, you can use the sch_On.bat batch files in Altsnap's folder.")) \
  LNGVALUE(general_elevate,               "GeneralElevate",             TEXT("E&levate")) \
  LNGVALUE(general_elevated,              "GeneralElevated",            TEXT("Elevated")) \
  LNGVALUE(general_elevation_aborted,     "GeneralElevationAborted",    TEXT("Elevation aborted.")) \
  /* Keyboard & Mouse tabs */ \
  LNGVALUE(input_mouse_box, "InputMouseBox", TEXT("Mouse actions")) \
  LNGVALUE(input_mouse_btac1,   "InputMouseBtAc1",   TEXT("&1. Primary")) \
  LNGVALUE(input_mouse_btac2,   "InputMouseBtAc2",   TEXT("&2. Alternate")) \
  LNGVALUE(input_mouse_inttb,   "InputMouseINTTB",   TEXT("&Title bar")) \
  LNGVALUE(input_mouse_whilem,  "InputMouseWhileM",  TEXT("Whil&e moving")) \
  LNGVALUE(input_mouse_whiler,  "InputMouseWhileR",  TEXT("While resi&zing")) \
  LNGVALUE(input_mouse_lmb,     "InputMouseLMB",     TEXT("Left mouse &button:")) \
  LNGVALUE(input_mouse_mmb,     "InputMouseMMB",     TEXT("&Middle mouse button:")) \
  LNGVALUE(input_mouse_rmb,     "InputMouseRMB",     TEXT("Ri&ght mouse button:")) \
  LNGVALUE(input_mouse_mb4,     "InputMouseMB4",     TEXT("Mouse button &4:")) \
  LNGVALUE(input_mouse_mb5,     "InputMouseMB5",     TEXT("Mouse button &5:")) \
  LNGVALUE(input_mouse_scroll,  "InputMouseScroll",  TEXT("&Scroll wheel:")) \
  LNGVALUE(input_mouse_hscroll, "InputMouseHScroll", TEXT("Scroll wheel (&horizontal):")) \
  LNGVALUE(input_mouse_moveup,  "InputMouseMoveUp",  TEXT("Long &drag-free Move:")) \
  LNGVALUE(input_mouse_resizeup,"InputMouseResizeUp",TEXT("Long drag-&free Resize:")) \
  LNGVALUE(input_mouse_ttbactions_box, "InputMouseTTBActionBox", TEXT("Use specific actions when clicking the Title bar")) \
  LNGVALUE(input_mouse_ttbactionsna,   "InputMouseTTBActionNA",  TEXT("Without hot&key")) \
  LNGVALUE(input_mouse_ttbactionswa,   "InputMouseTTBActionWA",  TEXT("&With hotkey")) \
  LNGVALUE(input_mouse_mmb_hc, "InputMouseMMBHC", TEXT("M&iddle mouse button")) \
  LNGVALUE(input_mouse_mb4_hc, "InputMouseMB4HC", TEXT("M&ouse button 4")) \
  LNGVALUE(input_mouse_mb5_hc, "InputMouseMB5HC", TEXT("Mo&use button 5")) \
  LNGVALUE(input_mouse_longclickmove, "InputMouseLongClickMove", TEXT("Mo&ve windows with a long left-click")) \
  LNGVALUE(input_scrolllockstate,     "InputScrollLockState",    TEXT("Suspend/Resume AltSnap based on &Scroll lock state")) \
  LNGVALUE(input_unikeyholdmenu,      "InputUniKeyHoldMenu",     TEXT("Pop&up an extended character menu when holding an alphabetic key down")) \
  LNGVALUE(input_keycombo,          "InputKeyCombo",        TEXT("Use two keys &combo to activate")) \
  LNGVALUE(input_grabwithalt,       "InputGrabWithAlt",     TEXT("&Action without click:")) \
  LNGVALUE(input_grabwithaltb,      "InputGrabWithAltB",    TEXT("Acti&on without click (alt):")) \
  LNGVALUE(input_actions_move,      "InputActionMove",      TEXT("Move window")) \
  LNGVALUE(input_actions_resize,    "InputActionResize",    TEXT("Resize window")) \
  LNGVALUE(input_actions_restore,   "InputActionRestore",   TEXT("Restore window")) \
  LNGVALUE(input_actions_close,     "InputActionClose",     TEXT("&Close window")) \
  LNGVALUE(input_actions_kill,      "InputActionKill",      TEXT("&Kill program")) \
  LNGVALUE(input_actions_pause,     "InputActionPause",     TEXT("Pause program")) \
  LNGVALUE(input_actions_resume,    "InputActionResume",    TEXT("Resume program")) \
  LNGVALUE(input_actions_asonoff,   "InputActionASOnOff",   TEXT("S&uspend/Resume AltSnap")) \
  LNGVALUE(input_actions_moveonoff, "InputActionMoveOnOff", TEXT("Movement dis&abled")) \
  LNGVALUE(input_actions_minimize,  "InputActionMinimize",  TEXT("Mi&nimize window")) \
  LNGVALUE(input_actions_maximize,  "InputActionMaximize",  TEXT("Ma&ximize window")) \
  LNGVALUE(input_actions_lower,     "InputActionLower",     TEXT("&Lower window")) \
  LNGVALUE(input_actions_focus,     "InputActionFocus",     TEXT("Focus window")) \
  LNGVALUE(input_actions_nstacked,  "InputActionNStacked",  TEXT("Next stacked window")) \
  LNGVALUE(input_actions_nstacked2, "InputActionNStacked2", TEXT("Next laser stacked window")) \
  LNGVALUE(input_actions_pstacked,       "InputActionPStacked",   TEXT("Previous stacked window")) \
  LNGVALUE(input_actions_pstacked2,      "InputActionPStacked2",  TEXT("Previous laser stacked window")) \
  LNGVALUE(input_actions_npstacked,      "InputActionNPStacked",  TEXT("Next/Prev stacked window")) \
  LNGVALUE(input_actions_npstacked2,     "InputActionNPStacked2", TEXT("Next/Prev laser stacked window")) \
  LNGVALUE(input_actions_stacklist,      "InputActionStackList",  TEXT("Stacked windows list")) \
  LNGVALUE(input_actions_stacklist2,     "InputActionStackList2", TEXT("Laser stacked windows list")) \
  LNGVALUE(input_actions_alttablist,     "InputActionAltTabList", TEXT("Windows list")) \
  LNGVALUE(input_actions_alttabfulllist, "InputActionAltTabFullList", TEXT("All windows list")) \
  LNGVALUE(input_actions_mlzone,   "InputActionMLZone",   TEXT("Move to the left zone")) \
  LNGVALUE(input_actions_mtzone,   "InputActionMTZone",   TEXT("Move to the top zone")) \
  LNGVALUE(input_actions_mrzone,   "InputActionMRZone",   TEXT("Move to the right zone")) \
  LNGVALUE(input_actions_mbzone,   "InputActionMBZone",   TEXT("Move to the bottom zone")) \
  LNGVALUE(input_actions_xlzone,   "InputActionXLZone",   TEXT("Extend to the left zone")) \
  LNGVALUE(input_actions_xtzone,   "InputActionXTZone",   TEXT("Extend to the top zone")) \
  LNGVALUE(input_actions_xrzone,   "InputActionXRZone",   TEXT("Extend to the right zone")) \
  LNGVALUE(input_actions_xbzone,   "InputActionXBZone",   TEXT("Extend to the bottom zone")) \
  LNGVALUE(input_actions_xtnledge, "InputActionXTNLEdge", TEXT("Extend to the next left edge")) \
  LNGVALUE(input_actions_xtntedge, "InputActionXTNTEdge", TEXT("Extend to the next top edge")) \
  LNGVALUE(input_actions_xtnredge, "InputActionXTNREdge", TEXT("Extend to the next right edge")) \
  LNGVALUE(input_actions_xtnbedge, "InputActionXTNBEdge", TEXT("Extend to the next bottom edge")) \
  LNGVALUE(input_actions_mtnledge, "InputActionMTNLEdge", TEXT("Move to the next left edge")) \
  LNGVALUE(input_actions_mtntedge, "InputActionMTNTEdge", TEXT("Move to the next top edge")) \
  LNGVALUE(input_actions_mtnredge, "InputActionMTNREdge", TEXT("Move to the next right edge")) \
  LNGVALUE(input_actions_mtnbedge, "InputActionMTNBEdge", TEXT("Move to the next bottom edge")) \
  LNGVALUE(input_actions_stepl,    "InputActionStepl",    TEXT("Step left")) \
  LNGVALUE(input_actions_stept,    "InputActionStepT",    TEXT("Step up")) \
  LNGVALUE(input_actions_stepr,    "InputActionStepR",    TEXT("Step right")) \
  LNGVALUE(input_actions_stepb,    "InputActionStepB",    TEXT("Step down")) \
  LNGVALUE(input_actions_sstepl,   "InputActionSStepl",   TEXT("Small step left")) \
  LNGVALUE(input_actions_sstept,   "InputActionSStepT",   TEXT("Small step up")) \
  LNGVALUE(input_actions_sstepr,   "InputActionSStepR",   TEXT("Small step right")) \
  LNGVALUE(input_actions_sstepb,   "InputActionSStepB",   TEXT("Small step down")) \
  LNGVALUE(input_actions_focusl,   "InputActionFocusL",   TEXT("Focus left window")) \
  LNGVALUE(input_actions_focust,   "InputActionFocusT",   TEXT("Focus top window")) \
  LNGVALUE(input_actions_focusr,   "InputActionFocusR",   TEXT("Focus right window")) \
  LNGVALUE(input_actions_focusb,   "InputActionFocusB",   TEXT("Focus bottom window")) \
  LNGVALUE(input_actions_roll,     "InputActionRoll",     TEXT("&Roll/Unroll window")) \
  LNGVALUE(input_actions_alwaysontop,  "InputActionAlwaysOnTop",  TEXT("Toggle always on &top")) \
  LNGVALUE(input_actions_borderless,   "InputActionBorderless",   TEXT("Toggle &borderless")) \
  LNGVALUE(input_actions_center,       "InputActionCenter",       TEXT("C&enter window on screen")) \
  LNGVALUE(input_actions_oriclick,     "InputActionOriClick",     TEXT("Send ori&ginal click")) \
  LNGVALUE(input_actions_nothing,      "InputActionNothing",      TEXT("Nothing")) \
  LNGVALUE(input_actions_alttab,       "InputActionAltTab",       TEXT("Alt+Tab")) \
  LNGVALUE(input_actions_volume,       "InputActionVolume",       TEXT("Volume")) \
  LNGVALUE(input_actions_mute,         "InputActionMute",         TEXT("Mute &sounds")) \
  LNGVALUE(input_actions_menu,         "InputActionMenu",         TEXT("Action menu")) \
  LNGVALUE(input_actions_maximizehv,   "InputActionMaximizeHV",   TEXT("Maximize &Vertically")) \
  LNGVALUE(input_actions_sidesnap,     "InputActionSideSnap",     TEXT("&Snap to monitor side/corner")) \
  LNGVALUE(input_actions_extendsnap ,  "InputActionExtendSnap",   TEXT("Extend to monitor side/corner")) \
  LNGVALUE(input_actions_extendtnedge, "InputActionExtendTNEdge", TEXT("Extend to next edge")) \
  LNGVALUE(input_actions_movetnedge,   "InputActionMoveTNEdge",   TEXT("Move to next edge")) \
  LNGVALUE(input_actions_minallother,  "InputActionMinAllOther",  TEXT("Minimize &other windows")) \
  LNGVALUE(input_actions_transparency, "InputActionTransparency", TEXT("Transparency")) \
  LNGVALUE(input_actions_zoom,         "InputActionZoom",         TEXT("Zoom window")) \
  LNGVALUE(input_actions_zoom2,        "InputActionZoom2",        TEXT("Zoom window 2")) \
  LNGVALUE(input_actions_hscroll,      "InputActionHScroll",      TEXT("Horizontal scroll")) \
  \
  LNGVALUE(input_hotkeys_box,     "InputHotkeysBox", TEXT("Hotkeys")) \
  LNGVALUE(input_hotkeys_modkey,  "InputHotkeysModKey", TEXT("Modifier key for al&ternate action:")) \
  LNGVALUE(input_hotclicks_box,   "InputHotclicksBox", TEXT("Hotclick (activate with a click)")) \
  LNGVALUE(input_hotclicks_more,  "InputHotclicksMore", TEXT("A checked button can be combined with an action but it will always be blocked in this case.")) \
  LNGVALUE(input_hotkeys_alt,     "InputHotkeysAlt", TEXT("Alt")) \
  LNGVALUE(input_hotkeys_winkey,  "InputHotkeysWinkey", TEXT("Winkey")) \
  LNGVALUE(input_hotkeys_ctrl,    "InputHotkeysCtrl", TEXT("Ctrl")) \
  LNGVALUE(input_hotkeys_shift,   "InputHotkeysShift", TEXT("Shift")) \
  LNGVALUE(input_hotkeys_shortcuts,      "InputHotkeysShortcuts",      TEXT("S&hortcut for action:")) \
  LNGVALUE(input_hotkeys_shortcutspick,  "InputHotkeysShortcutsPick",  TEXT("Pick &keys")) \
  LNGVALUE(input_hotkeys_shortcutsclear, "InputHotkeysShortcutsClear", TEXT("Clear ke&ys")) \
  LNGVALUE(input_hotkeys_shortcutset,    "InputHotkeysShortcutsSet",   TEXT("Sa&ve")) \
  LNGVALUE(input_hotkeys_useptwindow,    "InputHotkeysUsePtWindow",    TEXT("Apply to the &pointed window")) \
  LNGVALUE(input_hotkeys_leftalt,        "InputHotkeysLeftAlt",        TEXT("L&eft Alt")) \
  LNGVALUE(input_hotkeys_rightalt,       "InputHotkeysRightAlt",       TEXT("&Right Alt")) \
  LNGVALUE(input_hotkeys_leftwinkey,     "InputHotkeysLeftWinkey",     TEXT("Left &Winkey")) \
  LNGVALUE(input_hotkeys_rightwinkey,    "InputHotkeysRightWinkey",    TEXT("Right W&inkey")) \
  LNGVALUE(input_hotkeys_leftctrl,       "InputHotkeysLeftCtrl",       TEXT("&Left Ctrl")) \
  LNGVALUE(input_hotkeys_rightctrl,      "InputHotkeysRightCtrl",      TEXT("Ri&ght Ctrl")) \
  LNGVALUE(input_hotkeys_leftshift,      "InputHotkeysLeftShift",      TEXT("Left Shift")) \
  LNGVALUE(input_hotkeys_rightshift,     "InputHotkeysRightShift",     TEXT("Right Shift")) \
  LNGVALUE(input_hotkeys_more, "InputHotkeysMore", TEXT("You can add any key by editing the ini file.\nUse the shift key to snap windows to each other.")) \
  /* Blacklits Tab*/ \
  LNGVALUE(blacklist_box,              "BlacklistBox",              TEXT("Blacklist settings")) \
  LNGVALUE(blacklist_processblacklist, "BlacklistProcessBlacklist", TEXT("&Process blacklist:")) \
  LNGVALUE(blacklist_blacklist,        "BlacklistBlacklist",        TEXT("&Windows blacklist:")) \
  LNGVALUE(blacklist_scrolllist,       "BlacklistScrolllist",       TEXT("Windows that should ignore &scroll action:")) \
  LNGVALUE(blacklist_mdis,             "BlacklistMDIs",             TEXT("&MDIs not to be treated as such:")) \
  LNGVALUE(blacklist_pause,            "BlacklistPause",            TEXT("Processes not to be pa&used or killed:")) \
  LNGVALUE(blacklist_findwindow_box,   "BlacklistFindWindowBox",    TEXT("Identify window")) \
  /* Advanced Tab */ \
  LNGVALUE(advanced_metrics_box,      "AdvancedMetricsBox",       TEXT("Metrics")) \
  LNGVALUE(advanced_centerfraction,   "AdvancedCenterFraction",   TEXT("&Center/Sides fraction (%):")) \
  LNGVALUE(advanced_aerohoffset,      "AdvancedAeroHoffset",      TEXT("Aero offset(%) &Horizontal:")) \
  LNGVALUE(advanced_aerovoffset,      "AdvancedAeroVoffset",      TEXT("&Vertical:")) \
  LNGVALUE(advanced_snapthreshold,    "AdvancedSnapThreshold",    TEXT("&Snap Threshold (pixels):")) \
  LNGVALUE(advanced_aerothreshold,    "AdvancedAeroThreshold",    TEXT("A&ero Threshold (pixels):")) \
  LNGVALUE(advanced_snapgap,          "AdvancedSnapGap",          TEXT("Snapping &gap (pixels):")) \
  LNGVALUE(advanced_aerospeed,        "AdvancedAeroSpeed",        TEXT("Ma&x snapping speed (pixels):")) \
  LNGVALUE(advanced_testwindow,       "AdvancedTestWindow",       TEXT("Test &Window")) \
  LNGVALUE(advanced_movetrans,        "AdvancedMoveTrans",        TEXT("Opacit&y when moving:")) \
  LNGVALUE(advanced_behavior_box,     "AdvancedBehaviorBox",      TEXT("Behavior")) \
  LNGVALUE(advanced_multipleinstances,"AdvancedMultipleInstances",TEXT("Allow multiple &instances of AltSnap")) \
  LNGVALUE(advanced_autoremaximize,   "AdvancedAutoRemaximize",   TEXT("Automatically &remaximize windows when changing monitor")) \
  LNGVALUE(advanced_aerotopmaximizes, "AdvancedAeroTopMaximizes", TEXT("&Maximize windows snapped at top")) \
  LNGVALUE(advanced_aerodbclickshift, "AdvancedAeroDBClickShift", TEXT("Invert shift &behavior for double-click aero snapping")) \
  LNGVALUE(advanced_maxwithlclick,    "AdvancedMaxWithLClick",    TEXT("&Toggle maximize state with the resize button while moving")) \
  LNGVALUE(advanced_restoreonclick,   "AdvancedRestoreOnClick",   TEXT("Rest&ore window with single click like original AltDrag")) \
  LNGVALUE(advanced_fullscreen,       "AdvancedFullScreen",       TEXT("Enable on &full screen windows")) \
  LNGVALUE(advanced_blmaximized,      "AdvancedBLMaximized",      TEXT("&Disable AltSnap on Maximized windows")) \
  LNGVALUE(advanced_fancyzone,        "AdvancedFancyZone",        TEXT("Restore Fancy&Zones snapped windows")) \
  LNGVALUE(advanced_norestore,        "AdvancedNoRestore",        TEXT("Never restore AltSna&pped windows")) \
  LNGVALUE(advanced_topmostindicator, "AdvancedTopmostIndicator", TEXT("Show an i&ndicator on always on top windows")) \
  /* About Tab */ \
  LNGVALUE(about_box,     "AboutBox",      TEXT("About ") APP_NAME ) \
  LNGVALUE(about_version, "AboutVersion",  TEXT("Version ") TEXT( APP_VERSION )) \
  LNGVALUE(about_author,  "AboutAuthor",   TEXT("Created by Stefan Sundin")) \
  LNGVALUE(about_author2, "AboutAuthor2",  TEXT("Maintained by Raymond Gillibert")) \
  LNGVALUE(about_license, "AboutLicense",  APP_NAME TEXT(" is free and open source software!\nFeel free to redistribute!")) \
  LNGVALUE(about_translation_credit, "AboutTranslationCredit", TEXT("Translation credit")) \
  /* Misc */ \
  LNGVALUE(unhook_error,      "MiscUnhookError",      TEXT("There was an error disabling AltDrag. This was most likely caused by Windows having already disabled AltDrag's hooks.\n\nIf this is the first time this has happened, you can safely ignore it and continue using AltDrag.\n\nIf this is happening repeatedly, you can read on the website how to prevent this from happening again (look for 'AltDrag mysteriously stops working' in the documentation).")) \
  LNGVALUE(zone_confirmation, "MiscZoneConfirmation", TEXT("Erase old snap layout and save current Test Windows positions as the new snap layout?")) \
  LNGVALUE(zone_testwinhelp,  "MiscZoneTestWinHelp",  TEXT("To setup Snap layout:\n1) Open several of those Test Windows\n2) Dispose them as you please\n3) Hit the *&Save test windows as snap layout* option in the tray menu")) \
  /* Extended character list  for each virtual key */ \
  LNGVALUE(a, "A", TEXT("àáâäæãåª%āăąǎǟǡǣǻǽȁȃȧ|Ȧḁ%ⱥ|Ⱥɐ|Ɐɑ|Ɑɒ|Ɒⲁ|Ⲁⓐ")) \
  LNGVALUE(b, "B", TEXT("%ƀɓƃƅɃ%ɓḃḅḇⓑ")) \
  LNGVALUE(c, "C", TEXT("ç¢©%ćĉċčƈḉȼ|Ȼɕⓒ")) \
  LNGVALUE(d, "D", TEXT("ð%ďđɖɗƌƍḋḍḏḑḓǆǅǳǲȡȸⓓ")) \
  LNGVALUE(e, "E", TEXT("èéêë€%ēĕėęěǝəɛȅȇḕḗḙḛȩ|Ȩḝɇ|Ɇⱸⓔ")) \
  LNGVALUE(f, "F", TEXT("ƒ%ḟɸⱷⓕ%♩♪♮♭♯♬♫")) \
  LNGVALUE(g, "G", TEXT("%ǵǧḡɠɣǥⓖ")) \
  LNGVALUE(h, "H", TEXT("%ĥħƕǶḣḥḧḩḫȟ|Ȟⱨ|Ⱨⱶ|Ⱶẖⓗ")) \
  LNGVALUE(i, "I", TEXT("ìíîï%ĩīĭǐȉȋįİıĳɩɨḭḯⓘ")) \
  LNGVALUE(j, "J", TEXT("%ĵǰȷɉ|Ɉⓙ")) \
  LNGVALUE(k, "K", TEXT("%ķĸƙǩḱḳⱪ|Ⱪꝁ|Ꝁʞ|Ʞⓚ")) \
  LNGVALUE(l, "L", TEXT("£%ĺļľŀłƛǉǈȴƚ|Ƚⱡ|Ⱡɫ|Ɫḷḹḻḽⓛ")) \
  LNGVALUE(m, "M", TEXT("µ%ḿṁṃɱ|Ɱɯⓜ")) \
  LNGVALUE(n, "N", TEXT("ñ%ńņňŉŋɲƞ|Ƞǌǋǹȵ%ṅṇṉṋⓝ")) \
  LNGVALUE(o, "O", TEXT("òóôöœõø°%ōŏő%ɔɵơƣǒǫǭǿȍȏȣ|Ȣȫ|Ȫȭ|Ȭȯ|Ȯȱ|Ȱṍṏṑṓ%ⱺⓞ")) \
  LNGVALUE(p, "P", TEXT("¶þ·•%ƥᵽ|Ᵽṕṗⓟ")) \
  LNGVALUE(q, "Q", TEXT("¿¤‰‘’“”„…–—«»‹›%ȹɋ|Ɋⓠ")) \
  LNGVALUE(r, "R", TEXT("®%ŕŗřƦȑȓṙṛṝṟɍ|Ɍɽ|Ɽⱹⓡ")) \
  LNGVALUE(s, "S", TEXT("šß§%śŝşſ%ƨʃƪș|Șȿ|Ȿ%ṡṣṥṧṩⓢ")) \
  LNGVALUE(t, "T", TEXT("†‡™%ţťŧƫƭʈț|Țȶⱦ|Ⱦ%ṫṭṯṱẗⓣ")) \
  LNGVALUE(u, "U", TEXT("ùúûü%ũūůŭűų%ưʊǔǖǘǚǜȕȗʉ|Ʉ%ṳṵṷṹṻⓤ")) \
  LNGVALUE(v, "V", TEXT("%ʋɅⱱⱴ%ṽṿⓥ")) \
  LNGVALUE(w, "W", TEXT("%ẁẃŵẅⱳ|Ⱳ%ẇẉⓦ")) \
  LNGVALUE(x, "X", TEXT("±×÷¬%ẋẍⓧ")) \
  LNGVALUE(y, "Y", TEXT("ýÿ¥%ŷẏȳ|Ȳƴɏ|Ɏⓨ")) \
  LNGVALUE(z, "Z", TEXT("ž%źẑżẓẕ%ƶʒƹƺǯȥ|Ȥɀ|Ɀⱬ|Ⱬⓩ"))


#define LNGVALUE(x, y, z) TCHAR *x;
// String structure definition
struct strings { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z) y,
// Name of values ini entries
static const char* l10n_inimapping[] = { LANGUAGE_MAP };
#undef LNGVALUE

#define LNGVALUE(x, y, z) z,
// Default values en-US
static const struct strings en_US = { LANGUAGE_MAP };
#undef LANGVALUE

#endif
