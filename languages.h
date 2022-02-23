/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef ALTSNAP_STRINGS_H
#define ALTSNAP_STRINGS_H

struct langinfoitem {
  wchar_t *code;
  wchar_t *lang_english;
  wchar_t *lang;
  wchar_t *author;
  wchar_t *fn;
};

struct strings {
  wchar_t *code;
  wchar_t *lang_english;
  wchar_t *lang;
  wchar_t *author;

  // menu
  wchar_t *menu_enable;
  wchar_t *menu_disable;
  wchar_t *menu_hide;
  wchar_t *menu_config;
  wchar_t *menu_about;
  wchar_t *menu_savezones;
  wchar_t *menu_exit;

  // config
  wchar_t *title;
  wchar_t *tab_general;
  wchar_t *tab_mouse;
  wchar_t *tab_keyboard;
  wchar_t *tab_blacklist;
  wchar_t *tab_advanced;
  wchar_t *tab_about;

  // general
  wchar_t *general_box;
  wchar_t *general_autofocus;
  wchar_t *general_aero;
  wchar_t *general_smartaero;
  wchar_t *general_smarteraero;
  wchar_t *general_stickyresize;
  wchar_t *general_inactivescroll;
  wchar_t *general_mdi;
  wchar_t *general_autosnap;
  wchar_t *general_autosnap0;
  wchar_t *general_autosnap1;
  wchar_t *general_autosnap2;
  wchar_t *general_autosnap3;
  wchar_t *general_language;
  wchar_t *general_fullwin;
  wchar_t *general_usezones;
  wchar_t *general_piercingclick;
  wchar_t *general_resizeall;
  wchar_t *general_resizecenter;

  wchar_t *general_resizecenter_norm;
  wchar_t *general_resizecenter_br;
  wchar_t *general_resizecenter_move;

  // general autostart
  wchar_t *general_autostart_box;
  wchar_t *general_autostart;
  wchar_t *general_autostart_hide;
  wchar_t *general_autostart_elevate;
  wchar_t *general_autostart_elevate_tip;
  wchar_t *general_elevate;
  wchar_t *general_elevated;
  wchar_t *general_elevation_aborted;

  // input
  // mouse
  wchar_t *input_mouse_box;
  wchar_t *input_mouse_btac1;
  wchar_t *input_mouse_btac2;
  wchar_t *input_mouse_inttb;
  wchar_t *input_mouse_lmb;
  wchar_t *input_mouse_mmb;
  wchar_t *input_mouse_rmb;
  wchar_t *input_mouse_mb4;
  wchar_t *input_mouse_mb5;
  wchar_t *input_mouse_scroll;
  wchar_t *input_mouse_hscroll;
  wchar_t *input_mouse_ttbactions_box;
  wchar_t *input_mouse_ttbactionsna;
  wchar_t *input_mouse_ttbactionswa;
  wchar_t *input_mouse_mmb_hc;
  wchar_t *input_mouse_mb4_hc;
  wchar_t *input_mouse_mb5_hc;
  wchar_t *input_mouse_longclickmove;

  wchar_t *input_aggressive_pause;
  wchar_t *input_aggressive_kill;
  wchar_t *input_scrolllockstate;
  wchar_t *input_keycombo;
  wchar_t *input_grabwithalt;
  wchar_t *input_grabwithaltb;

  // actions
  wchar_t *input_actions_move;
  wchar_t *input_actions_resize;
  wchar_t *input_actions_close;
  wchar_t *input_actions_kill;
  wchar_t *input_actions_minimize;
  wchar_t *input_actions_maximize;
  wchar_t *input_actions_lower;
  wchar_t *input_actions_roll;
  wchar_t *input_actions_alwaysontop;
  wchar_t *input_actions_borderless;
  wchar_t *input_actions_center;
  wchar_t *input_actions_nothing;
  wchar_t *input_actions_alttab;
  wchar_t *input_actions_volume;
  wchar_t *input_actions_mute;
  wchar_t *input_actions_menu;
  wchar_t *input_actions_maximizehv;
  wchar_t *input_actions_sidesnap;
  wchar_t *input_actions_minallother;
  wchar_t *input_actions_transparency;
  wchar_t *input_actions_hscroll;

  // hotkeys
  wchar_t *input_hotkeys_box;
  wchar_t *input_hotkeys_modkey;
  wchar_t *input_hotclicks_box;
  wchar_t *input_hotclicks_more;
  wchar_t *input_hotkeys_leftalt;
  wchar_t *input_hotkeys_rightalt;
  wchar_t *input_hotkeys_leftwinkey;
  wchar_t *input_hotkeys_rightwinkey;
  wchar_t *input_hotkeys_leftctrl;
  wchar_t *input_hotkeys_rightctrl;
  wchar_t *input_hotkeys_leftshift;
  wchar_t *input_hotkeys_rightshift;
  wchar_t *input_hotkeys_more;

  // blacklist
  wchar_t *blacklist_box;
  wchar_t *blacklist_processblacklist;
  wchar_t *blacklist_blacklist;
  wchar_t *blacklist_scrolllist;
  wchar_t *blacklist_mdis;
  wchar_t *blacklist_pause;
  wchar_t *blacklist_findwindow_box;

  // advanced
  wchar_t *advanced_metrics_box;
  wchar_t *advanced_centerfraction;
  wchar_t *advanced_aerohoffset;
  wchar_t *advanced_aerovoffset;
  wchar_t *advanced_snapthreshold;
  wchar_t *advanced_aerothreshold;
  wchar_t *advanced_aerospeed;
  wchar_t *advanced_testwindow;
  wchar_t *advanced_movetrans;

  wchar_t *advanced_behavior_box;
  wchar_t *advanced_multipleinstances;
  wchar_t *advanced_autoremaximize;
  wchar_t *advanced_aerotopmaximizes;
  wchar_t *advanced_aerodbclickshift;
  wchar_t *advanced_maxwithlclick;
  wchar_t *advanced_restoreonclick;
  wchar_t *advanced_fullscreen;
  wchar_t *advanced_blmaximized;
  wchar_t *advanced_fancyzone;
  wchar_t *advanced_norestore;
  // about
  wchar_t *about_box;
  wchar_t *about_version;
  wchar_t *about_author;
  wchar_t *about_license;
  wchar_t *about_translation_credit;

  /* misc */
  wchar_t *unhook_error;
  wchar_t *zone_confirmation;
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
    "GeneralInactiveScroll",
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
    "InputMouseTTBActionBox",
    "InputMouseTTBActionNA",
    "InputMouseTTBActionWA",
    "InputMouseMMBHC",
    "InputMouseMB4HC",
    "InputMouseMB5HC",
    "InputMouseLongClickMove",

    "InputAggressivePause",
    "InputAggressiveKill",
    "InputScrollLockState",
    "InputKeyCombo",
    "InputGrabWithAlt",
    "InputGrabWithAltB",

    "InputActionMove",
    "InputActionResize",
    "InputActionClose",
    "InputActionKill",
    "InputActionMinimize",
    "InputActionMaximize",
    "InputActionLower",
    "InputActionRoll",
    "InputActionAlwaysOnTop",
    "InputActionBorderless",
    "InputActionCenter",
    "InputActionNothing",
    "InputActionAltTab",
    "InputActionVolume",
    "InputActionMute",
    "InputActionMenu",
    "InputActionMaximizeHV",
    "InputActionSideSnap",
    "InputActionMinAllOther",
    "InputActionTransparency",
    "InputActionHScroll",

    "InputHotkeysBox",
    "InputHotkeysModKey",
    "InputHotclicksBox",
    "InputHotclicksMore",
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

    "AboutBox",
    "AboutVersion",
    "AboutAuthor",
    "AboutLicense",
    "AboutTranslationCredit",

    "MiscUnhookError",
    "MiscZoneConfirmation"
};

// SAME ORDER THAN ABOVE!!!!
static const struct strings en_US = {
 /* === translation info === */
 /* code               */ L"en-US",
 /* lang_english       */ L"English",
 /* lang               */ L"English",
 /* author             */ L"Stefan Sundin, RamonUnch",

 /* === app === */
 /* menu */
 /* enable             */ L"&Enable",
 /* disable            */ L"&Disable",
 /* hide               */ L"&Hide tray",
 /* config             */ L"&Configure",
 /* about              */ L"&About",
 /* savezones          */ L"&Save test windows as snap layout",
 /* exit               */ L"E&xit",

 /* === config === */
 /* title              */ APP_NAME L" Configuration",
 /* tabs */
 /* general            */ L"General",
 /* Mouse              */ L"Mouse",
 /* Keybaord           */ L"Keyboard",
 /* blacklist          */ L"Blacklist",
 /* advanced           */ L"Advanced",
 /* about              */ L"About",

 /* general tab */
 /* box                */ L"General settings",
 /* autofocus          */ L"&Focus windows when dragging.\nYou can also press Ctrl to focus windows.",
 /* aero               */ L"Mimi&c Aero Snap",
 /* smartaero          */ L"Smart Aero Sna&p dimensions",
 /* smarteraero        */ L"Smarter Aer&o Snap dimensions",
 /* stickyresize       */ L"Resi&ze other snapped windows with Shift",
 /* inactivescroll     */ L"&Scroll inactive windows",
 /* mdi                */ L"&MDI support",
 /* autosnap           */ L"S&nap window edges to:",
 /* autosnap0          */ L"Disabled",
 /* autosnap1          */ L"To screen borders",
 /* autosnap2          */ L"+ outside of windows",
 /* autosnap3          */ L"+ inside of windows",
 /* language           */ L"&Language:",
 /* FullWin            */ L"&Drag full windows",
 /* UseZones           */ L"Snap to Layo&ut with Shift (configure with tray menu)",
 /* PiercingClick      */ L"Avoi&d blocking Alt+Click (disables AltSnap double-clicks)",
 /* ResizeAll          */ L"&Resize all windows",
 /* ResizeCenter       */ L"Center resize mode",
 /* ResizeCenterNorm   */ L"All d&irections",
 /* ResizeCenterBr     */ L"&Bottom right",
 /* ResizeCenterMove   */ L"Mo&ve",

 /* autostart_box      */ L"Autostart",
 /* autostart          */ L"S&tart "APP_NAME" when logging on",
 /* autostart_hide     */ L"&Hide tray",
 /* autostart_elevate  */ L"&Elevate to administrator privileges",
 /* elevate_tip        */ L"Note that a UAC prompt will appear every time you log in, unless you disable UAC completely or use the Task Scheduler",
 /* elevate            */ L"E&levate",
 /* elevated           */ L"Elevated",
 /* elevation_aborted  */ L"Elevation aborted.",

 /* input tab */
 /* mouse */
 /* box                */ L"Mouse actions",
 /* btac1              */ L"&1. Primary",
 /* btac2              */ L"&2. Alternate",
 /* inttb              */ L"&Title bar",
 /* lmb                */ L"Left mouse &button:",
 /* mmb                */ L"&Middle mouse button:",
 /* rmb                */ L"Ri&ght mouse button:",
 /* mb4                */ L"Mouse button &4:",
 /* mb5                */ L"Mouse button &5:",
 /* scroll             */ L"&Scroll wheel:",
 /* hscroll            */ L"Scroll wheel (&horizontal):",
 /* ttbactions box     */ L"Use specific actions when clicking the Title bar",
 /* ttbaction noalt    */ L"Without hot&key",
 /* ttbaction walt     */ L"&With hotkey",
 /* mmb_hr             */ L"M&iddle mouse button",
 /* mb4_hc             */ L"M&ouse button 4",
 /* mb5_hc             */ L"Mo&use button 5",
 /* longclickmove      */ L"Mo&ve windows with a long left-click",

 /* Aggressive Pause   */ L"&Pause process on Alt+Shift+Pause (Alt+Pause to resume)",
 /* Aggressive Kill    */ L"&Kill process on Ctrl+Alt+F4\nAlso adds the kill option to the action menu",
 /* scroll lock state  */ L"Suspend/Resume AltSnap based on &Scroll lock state",
 /* keycombo           */ L"Use two keys &combo to activate",
 /* grabwithalt        */ L"&Action without click:",
 /* grabwithaltb       */ L"Acti&on without click (alt):",

 /* actions */
 /* move               */ L"Move window",
 /* resize             */ L"Resize window",
 /* close              */ L"&Close window",
 /* kill               */ L"&Kill program",
 /* minimize           */ L"Mi&nimize window",
 /* maximize           */ L"Ma&ximize window",
 /* lower              */ L"&Lower window",
 /* roll               */ L"&Roll/Unroll window",
 /* alwaysontop        */ L"Toggle always on &top",
 /* borderless         */ L"Toggle &borderless",
 /* center             */ L"C&enter window on screen",
 /* nothing            */ L"Nothing",
 /* alttab             */ L"Alt+Tab",
 /* volume             */ L"Volume",
 /* mute               */ L"Mute &sounds",
 /* menu               */ L"Action menu",
 /* maximizehv         */ L"Maximize &Vertically",
 /* sidesnap           */ L"&Snap to side/corner",
 /* minallother        */ L"Minimize &other windows",
 /* transparency       */ L"Transparency",
 /* hscroll            */ L"Horizontal scroll",

 /* hotkeys */
 /* box                */ L"Hotkeys",
 /* modkey             */ L"Modifier key for al&ternate action:",
 /* hotclicks box      */ L"Activate with click",
 /* hotclicks more     */ L"Checked buttons will not be usable outside of AltSnap. They can be combined with an action.",
 /* leftalt            */ L"L&eft Alt",
 /* rightalt           */ L"&Right Alt",
 /* leftwinkey         */ L"Left &Winkey",
 /* rightwinkey        */ L"Right W&inkey",

 /* leftctrl           */ L"&Left Ctrl",
 /* rightctrl          */ L"Ri&ght Ctrl",
 /* leftshift          */ L"Left Shift",
 /* rightshift         */ L"Right Shift",

 /* more               */ L"You can add any key by editing the ini file.\nUse the shift key to snap windows to each other.",

 /* blacklist tab */
 /* box                */ L"Blacklist settings",
 /* processblacklist   */ L"&Process blacklist:",
 /* blacklist          */ L"&Windows blacklist:",
 /* scrolllist         */ L"Windows that should ignore &scroll action:",
 /* MDIs bl            */ L"&MDIs not to be treated as such:",
 /* Pause list         */ L"Processes not to be pa&used or killed:",
 /* findwindow_box     */ L"Identify window",

 /* advanced tab */
 /* metrics_box      */   L"Metrics",
 /* centerfraction   */   L"&Center fraction (%):",
 /* aerohoffset      */   L"Aero offset(%) &Horizontal:",
 /* aeroVoffset      */   L"&Vertical:",
 /* snapthreshold    */   L"&Snap Threshold (pixels):",
 /* aerothreshold    */   L"A&ero Threshold (pixels):",
 /* aerospeed        */   L"Ma&x snapping speed (pixels):",
 /* testwindow       */   L"Test &Window",
 /* movetrans        */   L"Opacit&y when moving:",

 /* behavior_box     */   L"Behavior",
 /* multipleinstances*/   L"Allow multiple &instances of AltSnap",
 /* autoremaximize   */   L"Automatically &remaximize windows when changing monitor",
 /* aerotopmaximizes */   L"&Maximize windows snapped at top",
 /* aerodbclickshift */   L"Invert shift &behavior for double-click aero snapping",
 /* maxwithlclick    */   L"&Toggle maximize state with right-click while moving",
 /* restoreonclick   */   L"Rest&ore window with single click like original AltDrag",
 /* fullscreen       */   L"Enable on &full screen windows",
 /* blmaximized      */   L"&Disable AltSnap on Maximized windows",
 /* fancyzone        */   L"Restore Fancy&Zones snapped windows",
 /* norestore        */   L"Never restore AltSna&pped windows",
 /* about tab */
 /* box                */ L"About "APP_NAME,
 /* version            */ L"Version "APP_VERSION,
 /* author             */ L"Created by Stefan Sundin",
 /* license            */ APP_NAME L" is free and open source software!\nFeel free to redistribute!",
 /* translation_credit */ L"Translation credit",

 /* === misc === */
 /* unhook_error       */ L"There was an error disabling "APP_NAME". This was most likely caused by Windows having already disabled "APP_NAME"'s hooks.\n\nIf this is the first time this has happened, you can safely ignore it and continue using "APP_NAME".\n\nIf this is happening repeatedly, you can read on the website how to prevent this from happening again (look for '"APP_NAME" mysteriously stops working' in the documentation).",
 /* zoneconfirmation   */ L"Erase old snap layout and save current Test Windows positions as the new snap layout?",

};

#endif
