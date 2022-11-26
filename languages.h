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

#include <windows.h>

struct langinfoitem {
  TCHAR *code;
  TCHAR *lang_english;
  TCHAR *lang;
  TCHAR *author;
  TCHAR *fn;
};

struct strings {
  TCHAR *code;
  TCHAR *lang_english;
  TCHAR *lang;
  TCHAR *author;

  // menu
  TCHAR *menu_enable;
  TCHAR *menu_disable;
  TCHAR *menu_hide;
  TCHAR *menu_config;
  TCHAR *menu_about;
  TCHAR *menu_openinifile;
  TCHAR *menu_savezones;
  TCHAR *menu_exit;

  // config
  TCHAR *title;
  TCHAR *tab_general;
  TCHAR *tab_mouse;
  TCHAR *tab_keyboard;
  TCHAR *tab_blacklist;
  TCHAR *tab_advanced;
  TCHAR *tab_about;

  // general
  TCHAR *general_box;
  TCHAR *general_autofocus;
  TCHAR *general_aero;
  TCHAR *general_smartaero;
  TCHAR *general_smarteraero;
  TCHAR *general_stickyresize;
  TCHAR *general_inactivescroll;
  TCHAR *general_mdi;
  TCHAR *general_autosnap;
  TCHAR *general_autosnap0;
  TCHAR *general_autosnap1;
  TCHAR *general_autosnap2;
  TCHAR *general_autosnap3;
  TCHAR *general_language;
  TCHAR *general_fullwin;
  TCHAR *general_usezones;
  TCHAR *general_piercingclick;
  TCHAR *general_resizeall;
  TCHAR *general_resizecenter;

  TCHAR *general_resizecenter_norm;
  TCHAR *general_resizecenter_br;
  TCHAR *general_resizecenter_move;
  TCHAR *general_resizecenter_close;

  // general autostart
  TCHAR *general_autostart_box;
  TCHAR *general_autostart;
  TCHAR *general_autostart_hide;
  TCHAR *general_autostart_elevate;
  TCHAR *general_autostart_elevate_tip;
  TCHAR *general_elevate;
  TCHAR *general_elevated;
  TCHAR *general_elevation_aborted;

  // input
  // mouse
  TCHAR *input_mouse_box;
  TCHAR *input_mouse_btac1;
  TCHAR *input_mouse_btac2;
  TCHAR *input_mouse_inttb;
  TCHAR *input_mouse_lmb;
  TCHAR *input_mouse_mmb;
  TCHAR *input_mouse_rmb;
  TCHAR *input_mouse_mb4;
  TCHAR *input_mouse_mb5;
  TCHAR *input_mouse_scroll;
  TCHAR *input_mouse_hscroll;
  TCHAR *input_mouse_moveup;
  TCHAR *input_mouse_resizeup;
  TCHAR *input_mouse_ttbactions_box;
  TCHAR *input_mouse_ttbactionsna;
  TCHAR *input_mouse_ttbactionswa;
  TCHAR *input_mouse_mmb_hc;
  TCHAR *input_mouse_mb4_hc;
  TCHAR *input_mouse_mb5_hc;
  TCHAR *input_mouse_longclickmove;

//  TCHAR *input_aggressive_pause;
//  TCHAR *input_aggressive_kill;
  TCHAR *input_scrolllockstate;
  TCHAR *input_unikeyholdmenu;
//  TCHAR *input_npstacked;
  TCHAR *input_keycombo;
  TCHAR *input_grabwithalt;
  TCHAR *input_grabwithaltb;

  // actions
  TCHAR *input_actions_move;
  TCHAR *input_actions_resize;
  TCHAR *input_actions_restore;
  TCHAR *input_actions_close;
  TCHAR *input_actions_kill;
  TCHAR *input_actions_pause;
  TCHAR *input_actions_resume;
  TCHAR *input_actions_asonoff;
  TCHAR *input_actions_moveonoff;
  TCHAR *input_actions_minimize;
  TCHAR *input_actions_maximize;
  TCHAR *input_actions_lower;
  TCHAR *input_actions_nstacked;
  TCHAR *input_actions_nstacked2;
  TCHAR *input_actions_pstacked;
  TCHAR *input_actions_pstacked2;
  TCHAR *input_actions_npstacked;
  TCHAR *input_actions_stacklist;
  TCHAR *input_actions_stacklist2;
  TCHAR *input_actions_alttablist;

  TCHAR *input_actions_mlzone;
  TCHAR *input_actions_mtzone;
  TCHAR *input_actions_mrzone;
  TCHAR *input_actions_mbzone;
  TCHAR *input_actions_xlzone;
  TCHAR *input_actions_xtzone;
  TCHAR *input_actions_xrzone;
  TCHAR *input_actions_xbzone;

  TCHAR *input_actions_stepl;
  TCHAR *input_actions_stept;
  TCHAR *input_actions_stepr;
  TCHAR *input_actions_stepb;
  TCHAR *input_actions_sstepl;
  TCHAR *input_actions_sstept;
  TCHAR *input_actions_sstepr;
  TCHAR *input_actions_sstepb;

  TCHAR *input_actions_roll;
  TCHAR *input_actions_alwaysontop;
  TCHAR *input_actions_borderless;
  TCHAR *input_actions_center;
  TCHAR *input_actions_oriclick;
  TCHAR *input_actions_nothing;
  TCHAR *input_actions_alttab;
  TCHAR *input_actions_volume;
  TCHAR *input_actions_mute;
  TCHAR *input_actions_menu;
  TCHAR *input_actions_maximizehv;
  TCHAR *input_actions_sidesnap;
  TCHAR *input_actions_extendsnap;
  TCHAR *input_actions_minallother;
  TCHAR *input_actions_transparency;
  TCHAR *input_actions_zoom;
  TCHAR *input_actions_zoom2;
  TCHAR *input_actions_hscroll;

  // hotkeys
  TCHAR *input_hotkeys_box;
  TCHAR *input_hotkeys_modkey;
  TCHAR *input_hotclicks_box;
  TCHAR *input_hotclicks_more;

  TCHAR *input_hotkeys_alt;
  TCHAR *input_hotkeys_winkey;
  TCHAR *input_hotkeys_ctrl;
  TCHAR *input_hotkeys_shift;
  TCHAR *input_hotkeys_shortcuts;
  TCHAR *input_hotkeys_shortcutspick;
  TCHAR *input_hotkeys_shortcutsclear;
  TCHAR *input_hotkeys_shortcutset;
  TCHAR *input_hotkeys_useptwindow;

  TCHAR *input_hotkeys_leftalt;
  TCHAR *input_hotkeys_rightalt;
  TCHAR *input_hotkeys_leftwinkey;
  TCHAR *input_hotkeys_rightwinkey;
  TCHAR *input_hotkeys_leftctrl;
  TCHAR *input_hotkeys_rightctrl;
  TCHAR *input_hotkeys_leftshift;
  TCHAR *input_hotkeys_rightshift;
  TCHAR *input_hotkeys_more;

  // blacklist
  TCHAR *blacklist_box;
  TCHAR *blacklist_processblacklist;
  TCHAR *blacklist_blacklist;
  TCHAR *blacklist_scrolllist;
  TCHAR *blacklist_mdis;
  TCHAR *blacklist_pause;
  TCHAR *blacklist_findwindow_box;

  // advanced
  TCHAR *advanced_metrics_box;
  TCHAR *advanced_centerfraction;
  TCHAR *advanced_aerohoffset;
  TCHAR *advanced_aerovoffset;
  TCHAR *advanced_snapthreshold;
  TCHAR *advanced_aerothreshold;
  TCHAR *advanced_snapgap;
  TCHAR *advanced_aerospeed;
  TCHAR *advanced_testwindow;
  TCHAR *advanced_movetrans;

  TCHAR *advanced_behavior_box;
  TCHAR *advanced_multipleinstances;
  TCHAR *advanced_autoremaximize;
  TCHAR *advanced_aerotopmaximizes;
  TCHAR *advanced_aerodbclickshift;
  TCHAR *advanced_maxwithlclick;
  TCHAR *advanced_restoreonclick;
  TCHAR *advanced_fullscreen;
  TCHAR *advanced_blmaximized;
  TCHAR *advanced_fancyzone;
  TCHAR *advanced_norestore;
  TCHAR *advanced_topmostindicator;
  // about
  TCHAR *about_box;
  TCHAR *about_version;
  TCHAR *about_author;
  TCHAR *about_license;
  TCHAR *about_translation_credit;

  // misc
  TCHAR *unhook_error;
  TCHAR *zone_confirmation;
  TCHAR *zone_testwinhelp;
  TCHAR *a, *b, *c, *d, *e, *f, *g, *h;
  TCHAR *i, *j, *k, *l, *m, *n, *o, *p;
  TCHAR *q, *r, *s, *t, *u, *v, *w, *x;
  TCHAR *y, *z;
};

// SAME ORDER THAN ABOVE!!!!
static const char* l10n_inimapping[] = {
    "Code",
    "LangEnglish",
    "Lang",
    "Author",

    "MenuEnable",
    "MenuDisable",
    "MenuHideTray",
    "MenuConfigure",
    "MenuAbout",
    "MenuOpenIniFile",
    "MenuSaveZones",
    "MenuExit",

    "ConfigTitle",
    "ConfigTabGeneral",
    "ConfigTabMouse",
    "ConfigTabKeyboard",
    "ConfigTabBlacklist",
    "ConfigTabAdvanced",
    "ConfigTabAbout",

    "GeneralBox",
    "GeneralAutoFocus",
    "GeneralAero",
    "GeneralSmartAero",
    "GeneralSmarterAero",
    "GeneralStickyResize",
    "GeneralInactiveScrol",
    "GeneralMDI",
    "GeneralAutoSnap",
    "GeneralAutoSnap0",
    "GeneralAutoSnap1",
    "GeneralAutoSnap2",
    "GeneralAutoSnap3",
    "GeneralLanguage",
    "GeneralFullWin",
    "GeneralUseZones",
    "GeneralPiercingClick",
    "GeneralResizeAll",
    "GeneralResizeCenter",
    "GeneralResizeCenterNorm",
    "GeneralResizeCenterBr",
    "GeneralResizeCenterMove",
    "GeneralResizeCenterClose",

    "GeneralAutostartBox",
    "GeneralAutostart",
    "GeneralAutostartHide",
    "GeneralAutostartElevate",
    "GeneralAutostartElevateTip",
    "GeneralElevate",
    "GeneralElevated",
    "GeneralElevationAborted",

    "InputMouseBox",
    "InputMouseBtAc1",
    "InputMouseBtAc2",
    "InputMouseINTTB",
    "InputMouseLMB",
    "InputMouseMMB",
    "InputMouseRMB",
    "InputMouseMB4",
    "InputMouseMB5",
    "InputMouseScroll",
    "InputMouseHScroll",
    "InputMouseMoveUp",
    "InputMouseResizeUp",
    "InputMouseTTBActionBox",
    "InputMouseTTBActionNA",
    "InputMouseTTBActionWA",
    "InputMouseMMBHC",
    "InputMouseMB4HC",
    "InputMouseMB5HC",
    "InputMouseLongClickMove",

    "InputScrollLockState",
    "InputUniKeyHoldMenu",
    "InputKeyCombo",
    "InputGrabWithAlt",
    "InputGrabWithAltB",

    "InputActionMove",
    "InputActionResize",
    "InputActionRestore",
    "InputActionClose",
    "InputActionKill",
    "InputActionPause",
    "InputActionResume",
    "InputActionASOnOff",
    "InputActionMoveOnOff",
    "InputActionMinimize",
    "InputActionMaximize",
    "InputActionLower",
    "InputActionNStacked",
    "InputActionNStacked2",
    "InputActionPStacked",
    "InputActionPStacked2",
    "InputActionNPStacked",
    "InputActionStackList",
    "InputActionStackList2",
    "InputActionAltTabList",
    "InputActionMLZone",
    "InputActionMTZone",
    "InputActionMRZone",
    "InputActionMBZone",
    "InputActionXLZone",
    "InputActionXTZone",
    "InputActionXRZone",
    "InputActionXBZone",
    "InputActionStepl",
    "InputActionStepT",
    "InputActionStepR",
    "InputActionStepB",
    "InputActionSStepl",
    "InputActionSStepT",
    "InputActionSStepR",
    "InputActionSStepB",
    "InputActionRoll",
    "InputActionAlwaysOnTop",
    "InputActionBorderless",
    "InputActionCenter",
    "InputActionOriClick",
    "InputActionNothing",
    "InputActionAltTab",
    "InputActionVolume",
    "InputActionMute",
    "InputActionMenu",
    "InputActionMaximizeHV",
    "InputActionSideSnap",
    "InputActionExtendSnap",
    "InputActionMinAllOther",
    "InputActionTransparency",
    "InputActionZoom",
    "InputActionZoom2",
    "InputActionHScroll",

    "InputHotkeysBox",
    "InputHotkeysModKey",
    "InputHotclicksBox",
    "InputHotclicksMore",
    "InputHotkeysAlt",
    "InputHotkeysWinkey",
    "InputHotkeysCtrl",
    "InputHotkeysShift",
    "InputHotkeysShortcuts",
    "InputHotkeysShortcutsPick",
    "InputHotkeysShortcutsClear",
    "InputHotkeysShortcutsSet",
    "InputHotkeysUsePtWindow",
    "InputHotkeysLeftAlt",
    "InputHotkeysRightAlt",
    "InputHotkeysLeftWinkey",
    "InputHotkeysRightWinkey",
    "InputHotkeysLeftCtrl",
    "InputHotkeysRightCtrl",
    "InputHotkeysLeftShift",
    "InputHotkeysRightShift",
    "InputHotkeysMore",

    "BlacklistBox",
    "BlacklistProcessBlacklist",
    "BlacklistBlacklist",
    "BlacklistScrolllist",
    "BlacklistMDIs",
    "BlacklistPause",
    "BlacklistFindWindowBox",

    "AdvancedMetricsBox",
    "AdvancedCenterFraction",
    "AdvancedAeroHoffset",
    "AdvancedAeroVoffset",
    "AdvancedSnapThreshold",
    "AdvancedAeroThreshold",
    "AdvancedSnapGap",
    "AdvancedAeroSpeed",
    "AdvancedTestWindow",
    "AdvancedMoveTrans",

    "AdvancedBehaviorBox",
    "AdvancedMultipleInstances",
    "AdvancedAutoRemaximize",
    "AdvancedAeroTopMaximizes",
    "AdvancedAeroDBClickShift",
    "AdvancedMaxWithLClick",
    "AdvancedRestoreOnClick",
    "AdvancedFullScreen",
    "AdvancedBLMaximized",
    "AdvancedFancyZone",
    "AdvancedNoRestore",
    "AdvancedTopmostIndicator",

    "AboutBox",
    "AboutVersion",
    "AboutAuthor",
    "AboutLicense",
    "AboutTranslationCredit",

    "MiscUnhookError",
    "MiscZoneConfirmation",
    "MiscZoneTestWinHelp",
    "A", "B", "C", "D", "E",
    "F", "G", "H", "I", "J",
    "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z",
};

// SAME ORDER THAN ABOVE!!!!
static const struct strings en_US = {
 /* === translation info === */
 /* code               */ TEXT("en-US"),
 /* lang_english       */ TEXT("English"),
 /* lang               */ TEXT("English"),
 /* author             */ TEXT("Stefan Sundin, RamonUnch"),

 /* === app === */
 /* menu */
 /* enable             */ TEXT("&Enable"),
 /* disable            */ TEXT("&Disable"),
 /* hide               */ TEXT("&Hide tray"),
 /* config             */ TEXT("&Configure..."),
 /* about              */ TEXT("&About"),
 /* open ini file      */ TEXT("&Open ini file..."),
 /* savezones          */ TEXT("&Save test windows as snap layout"),
 /* exit               */ TEXT("E&xit"),

 /* === config === */
 /* title              */ TEXT( APP_NAMEA " Configuration"),
 /* tabs */
 /* general            */ TEXT("General"),
 /* Mouse              */ TEXT("Mouse"),
 /* Keybaord           */ TEXT("Keyboard"),
 /* blacklist          */ TEXT("Blacklist"),
 /* advanced           */ TEXT("Advanced"),
 /* about              */ TEXT("About"),

 /* general tab */
 /* box                */ TEXT("General settings"),
 /* autofocus          */ TEXT("&Focus windows when dragging.\nYou can also press Ctrl to focus windows."),
 /* aero               */ TEXT("Mimi&c Aero Snap"),
 /* smartaero          */ TEXT("Smart Aero Sna&p dimensions"),
 /* smarteraero        */ TEXT("Smarter Aer&o Snap dimensions"),
 /* stickyresize       */ TEXT("Resi&ze other snapped windows with Shift"),
 /* inactivescroll     */ TEXT("&Scroll inactive windows"),
 /* mdi                */ TEXT("&MDI support"),
 /* autosnap           */ TEXT("S&nap window edges to:"),
 /* autosnap0          */ TEXT("Disabled"),
 /* autosnap1          */ TEXT("To screen borders"),
 /* autosnap2          */ TEXT("+ outside of windows"),
 /* autosnap3          */ TEXT("+ inside of windows"),
 /* language           */ TEXT("&Language:"),
 /* FullWin            */ TEXT("&Drag full windows"),
 /* UseZones           */ TEXT("Snap to Layo&ut with Shift (configure with tray menu)"),
 /* PiercingClick      */ TEXT("Avoi&d blocking Alt+Click (disables AltSnap double-clicks)"),
 /* ResizeAll          */ TEXT("&Resize all windows"),
 /* ResizeCenter       */ TEXT("Center resize mode"),
 /* ResizeCenterNorm   */ TEXT("All d&irections"),
 /* ResizeCenterBr     */ TEXT("&Bottom right"),
 /* ResizeCenterMove   */ TEXT("Mo&ve"),
 /* ResizeCenterClose  */ TEXT("Clos&est side"),

 /* autostart_box      */ TEXT("Autostart"),
 /* autostart          */ TEXT("S&tart " APP_NAMEA " when logging on"),
 /* autostart_hide     */ TEXT("&Hide tray"),
 /* autostart_elevate  */ TEXT("&Elevate to administrator privileges"),
 /* elevate_tip        */ TEXT("Note that a UAC prompt will appear every time you log in, unless you disable UAC completely or use the Task Scheduler"),
 /* elevate            */ TEXT("E&levate"),
 /* elevated           */ TEXT("Elevated"),
 /* elevation_aborted  */ TEXT("Elevation aborted."),

 /* input tab */
 /* mouse */
 /* box                */ TEXT("Mouse actions"),
 /* btac1              */ TEXT("&1. Primary"),
 /* btac2              */ TEXT("&2. Alternate"),
 /* inttb              */ TEXT("&Title bar"),
 /* lmb                */ TEXT("Left mouse &button:"),
 /* mmb                */ TEXT("&Middle mouse button:"),
 /* rmb                */ TEXT("Ri&ght mouse button:"),
 /* mb4                */ TEXT("Mouse button &4:"),
 /* mb5                */ TEXT("Mouse button &5:"),
 /* scroll             */ TEXT("&Scroll wheel:"),
 /* hscroll            */ TEXT("Scroll wheel (&horizontal):"),
 /* moveup             */ TEXT("Long &drag-free Move:"),
 /* resizeup           */ TEXT("Long drag-&free Resize:"),
 /* ttbactions box     */ TEXT("Use specific actions when clicking the Title bar"),
 /* ttbaction noalt    */ TEXT("Without hot&key"),
 /* ttbaction walt     */ TEXT("&With hotkey"),
 /* mmb_hr             */ TEXT("M&iddle mouse button"),
 /* mb4_hc             */ TEXT("M&ouse button 4"),
 /* mb5_hc             */ TEXT("Mo&use button 5"),
 /* longclickmove      */ TEXT("Mo&ve windows with a long left-click"),

 /* scroll lock state  */ TEXT("Suspend/Resume AltSnap based on &Scroll lock state"),
 /* unikeyholdmenu     */ TEXT("Pop&up an extended character menu when holding an alphabetic key down"),
 /* keycombo           */ TEXT("Use two keys &combo to activate"),
 /* grabwithalt        */ TEXT("&Action without click:"),
 /* grabwithaltb       */ TEXT("Acti&on without click (alt):"),

 /* actions */
 /* move               */ TEXT("Move window"),
 /* resize             */ TEXT("Resize window"),
 /* restore            */ TEXT("Restore window"),
 /* close              */ TEXT("&Close window"),
 /* kill               */ TEXT("&Kill program"),
 /* pause              */ TEXT("Pause program"),
 /* resume             */ TEXT("Resume program"),
 /* asonoff            */ TEXT("S&uspend/Resume AltSnap"),
 /* moveonoff          */ TEXT("Movement dis&abled"),
 /* minimize           */ TEXT("Mi&nimize window"),
 /* maximize           */ TEXT("Ma&ximize window"),
 /* lower              */ TEXT("&Lower window"),
 /* nstacked           */ TEXT("Next stacked window"),
 /* nstacked 2         */ TEXT("Next laser stacked window"),
 /* pstacked           */ TEXT("Previous stacked window"),
 /* pstacked 2         */ TEXT("Previous laser stacked window"),
 /* npstacked          */ TEXT("Next/Prev stacked window"),
 /* stacklist          */ TEXT("Stacked windows list"),
 /* stacklist2         */ TEXT("Laser stacked windows list"),
 /* alttablist         */ TEXT("Windows List"),
 /* mlzone             */ TEXT("Move to the left zone"),
 /* mtzone             */ TEXT("Move to the top zone"),
 /* mrzone             */ TEXT("Move to the right zone"),
 /* mbzone             */ TEXT("Move to the bottom zone"),
 /* xlzone             */ TEXT("Extend to the left zone"),
 /* xtzone             */ TEXT("Extend to the top zone"),
 /* xrzone             */ TEXT("Extend to the right zone"),
 /* xbzone             */ TEXT("Extend to the bottom zone"),
 /* stepl              */ TEXT("Step left"),
 /* stept              */ TEXT("Step up"),
 /* stepr              */ TEXT("Step right"),
 /* stepb              */ TEXT("Step down"),
 /* sstepl             */ TEXT("Small step left"),
 /* sstept             */ TEXT("Small step up"),
 /* sstepr             */ TEXT("Small step right"),
 /* sstepb             */ TEXT("Small step down"),
 /* roll               */ TEXT("&Roll/Unroll window"),
 /* alwaysontop        */ TEXT("Toggle always on &top"),
 /* borderless         */ TEXT("Toggle &borderless"),
 /* center             */ TEXT("C&enter window on screen"),
 /* oriclick           */ TEXT("Send ori&ginal click"),
 /* nothing            */ TEXT("Nothing"),
 /* alttab             */ TEXT("Alt+Tab"),
 /* volume             */ TEXT("Volume"),
 /* mute               */ TEXT("Mute &sounds"),
 /* menu               */ TEXT("Action menu"),
 /* maximizehv         */ TEXT("Maximize &Vertically"),
 /* sidesnap           */ TEXT("&Snap to monitor side/corner"),
 /* extendsnap         */ TEXT("Extend to monitor side/corner"),
 /* minallother        */ TEXT("Minimize &other windows"),
 /* transparency       */ TEXT("Transparency"),
 /* zoom               */ TEXT("Zoom window"),
 /* zoom2              */ TEXT("Zoom window 2"),
 /* hscroll            */ TEXT("Horizontal scroll"),

 /* hotkeys */
 /* box                */ TEXT("Hotkeys"),
 /* modkey             */ TEXT("Modifier key for al&ternate action:"),
 /* hotclicks box      */ TEXT("Activate with click"),
 /* hotclicks more     */ TEXT("Checked buttons will not be usable outside of AltSnap. They can be combined with an action."),
 /* alt                */ TEXT("Alt"),
 /* winkey             */ TEXT("Winkey"),
 /* ctrl               */ TEXT("Ctrl"),
 /* shift              */ TEXT("Shift"),
 /* shortcuts          */ TEXT("S&hortcut for action:"),
 /* shortcutspick      */ TEXT("Pick &keys"),
 /* shortcutsClear     */ TEXT("Clear ke&ys"),
 /* shortcutsSet       */ TEXT("Sa&ve"),
 /* useptwindow        */ TEXT("Apply to the &pointed window"),
 /* leftalt            */ TEXT("L&eft Alt"),
 /* rightalt           */ TEXT("&Right Alt"),
 /* leftwinkey         */ TEXT("Left &Winkey"),
 /* rightwinkey        */ TEXT("Right W&inkey"),
 /* leftctrl           */ TEXT("&Left Ctrl"),
 /* rightctrl          */ TEXT("Ri&ght Ctrl"),
 /* leftshift          */ TEXT("Left Shift"),
 /* rightshift         */ TEXT("Right Shift"),
 /* hotkeys more       */ TEXT("You can add any key by editing the ini file.\nUse the shift key to snap windows to each other."),

 /* blacklist tab */
 /* box                */ TEXT("Blacklist settings"),
 /* processblacklist   */ TEXT("&Process blacklist:"),
 /* blacklist          */ TEXT("&Windows blacklist:"),
 /* scrolllist         */ TEXT("Windows that should ignore &scroll action:"),
 /* MDIs bl            */ TEXT("&MDIs not to be treated as such:"),
 /* Pause list         */ TEXT("Processes not to be pa&used or killed:"),
 /* findwindow_box     */ TEXT("Identify window"),

 /* advanced tab */
 /* metrics_box      */   TEXT("Metrics"),
 /* centerfraction   */   TEXT("&Center/Sides fraction (%):"),
 /* aerohoffset      */   TEXT("Aero offset(%) &Horizontal:"),
 /* aeroVoffset      */   TEXT("&Vertical:"),
 /* snapthreshold    */   TEXT("&Snap Threshold (pixels):"),
 /* aerothreshold    */   TEXT("A&ero Threshold (pixels):"),
 /* snapgap          */   TEXT("Snapping &gap (pixels):"),
 /* aerospeed        */   TEXT("Ma&x snapping speed (pixels):"),
 /* testwindow       */   TEXT("Test &Window"),
 /* movetrans        */   TEXT("Opacit&y when moving:"),

 /* behavior_box     */   TEXT("Behavior"),
 /* multipleinstances*/   TEXT("Allow multiple &instances of AltSnap"),
 /* autoremaximize   */   TEXT("Automatically &remaximize windows when changing monitor"),
 /* aerotopmaximizes */   TEXT("&Maximize windows snapped at top"),
 /* aerodbclickshift */   TEXT("Invert shift &behavior for double-click aero snapping"),
 /* maxwithlclick    */   TEXT("&Toggle maximize state with the resize button while moving"),
 /* restoreonclick   */   TEXT("Rest&ore window with single click like original AltDrag"),
 /* fullscreen       */   TEXT("Enable on &full screen windows"),
 /* blmaximized      */   TEXT("&Disable AltSnap on Maximized windows"),
 /* fancyzone        */   TEXT("Restore Fancy&Zones snapped windows"),
 /* norestore        */   TEXT("Never restore AltSna&pped windows"),
 /* topmostindicator */   TEXT("Show an i&ndicator on always on top windows"),

 /* about tab */
 /* box                */ TEXT("About "APP_NAME),
 /* version            */ TEXT("Version "APP_VERSION),
 /* author             */ TEXT("Created by Stefan Sundin"),
 /* license            */ TEXT( APP_NAMEA " is free and open source software!\nFeel free to redistribute!"),
 /* translation_credit */ TEXT("Translation credit"),

 /* === misc === */
 /* unhook_error       */ TEXT("There was an error disabling "APP_NAME". This was most likely caused by Windows having already disabled "APP_NAME"'s hooks.\n\n"
                           "If this is the first time this has happened, you can safely ignore it and continue using "APP_NAME".\n\n"
                           "If this is happening repeatedly, you can read on the website how to prevent this from happening again "
                           "(look for '"APP_NAME" mysteriously stops working' in the documentation)."),

 /* zoneconfirmation   */ TEXT("Erase old snap layout and save current Test Windows positions as the new snap layout?"),
 /* zone test win help */ TEXT("To setup Snap layout:\n"
                           "1) Open several of those Test Windows\n"
                           "2) Dispose them as you please\n"
                           "3) Hit the *&Save test windows as snap layout* option in the tray menu"),
 /* Extended character list  for each virtual key */
 /* A */ TEXT("àáâäæãåª%āăąǎǟǡǣǻǽȁȃȧ|Ȧḁ%ⱥ|Ⱥɐ|Ɐɑ|Ɑɒ|Ɒⲁ|Ⲁⓐ"), \
 /* B */ TEXT("%ƀɓƃƅɃ%ɓḃḅḇⓑ"), \
 /* C */ TEXT("ç¢©%ćĉċčƈḉȼ|Ȼɕⓒ"), \
 /* D */ TEXT("ð%ďđɖɗƌƍḋḍḏḑḓǆǅǳǲȡȸⓓ"), \
 /* E */ TEXT("èéêë€%ēĕėęěǝəɛȅȇḕḗḙḛȩ|Ȩḝɇ|Ɇⱸⓔ"), \
 /* F */ TEXT("ƒ%ḟɸⱷⓕ%♩♪♮♭♯♬♫"), \
 /* G */ TEXT("%ǵǧḡɠɣǥⓖ"), \
 /* H */ TEXT("%ĥħƕǶḣḥḧḩḫȟ|Ȟⱨ|Ⱨⱶ|Ⱶẖⓗ"), \
 /* I */ TEXT("ìíîï%ĩīĭǐȉȋįİıĳɩɨḭḯⓘ"), \
 /* J */ TEXT("%ĵǰȷɉ|Ɉⓙ"), \
 /* K */ TEXT("%ķĸƙǩḱḳⱪ|Ⱪꝁ|Ꝁʞ|Ʞⓚ"), \
 /* L */ TEXT("£%ĺļľŀłƛǉǈȴƚ|Ƚⱡ|Ⱡɫ|Ɫḷḹḻḽⓛ"), \
 /* M */ TEXT("µ%ḿṁṃɱ|Ɱɯⓜ"), \
 /* N */ TEXT("ñ%ńņňŉŋɲƞ|Ƞǌǋǹȵ%ṅṇṉṋⓝ"), \
 /* O */ TEXT("òóôöœõø°%ōŏő%ɔɵơƣǒǫǭǿȍȏȣ|Ȣȫ|Ȫȭ|Ȭȯ|Ȯȱ|Ȱṍṏṑṓ%ⱺⓞ"), \
 /* P */ TEXT("¶þ·•%ƥᵽ|Ᵽṕṗⓟ"), \
 /* Q */ TEXT("¿¤‰‘’“”„…–—«»‹›%ȹɋ|Ɋⓠ"), \
 /* R */ TEXT("®%ŕŗřƦȑȓṙṛṝṟɍ|Ɍɽ|Ɽⱹⓡ"), \
 /* S */ TEXT("šß§%śŝşſ%ƨʃƪș|Șȿ|Ȿ%ṡṣṥṧṩⓢ"), \
 /* T */ TEXT("†‡™%ţťŧƫƭʈț|Țȶⱦ|Ⱦ%ṫṭṯṱẗⓣ"), \
 /* U */ TEXT("ùúûü%ũūůŭűų%ưʊǔǖǘǚǜȕȗʉ|Ʉ%ṳṵṷṹṻⓤ"), \
 /* V */ TEXT("%ʋɅⱱⱴ%ṽṿⓥ"), \
 /* W */ TEXT("%ẁẃŵẅⱳ|Ⱳ%ẇẉⓦ"), \
 /* X */ TEXT("±×÷¬%ẋẍⓧ"), \
 /* Y */ TEXT("ýÿ¥%ŷẏȳ|Ȳƴɏ|Ɏⓨ"), \
 /* Z */ TEXT("ž%źẑżẓẕ%ƶʒƹƺǯȥ|Ȥɀ|Ɀⱬ|Ⱬⓩ"), \
};

#endif
