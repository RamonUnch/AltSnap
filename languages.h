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
  wchar_t *input_mouse_lmb;
  wchar_t *input_mouse_mmb;
  wchar_t *input_mouse_rmb;
  wchar_t *input_mouse_mb4;
  wchar_t *input_mouse_mb5;
  wchar_t *input_mouse_scroll;
  wchar_t *input_mouse_hscroll;
  wchar_t *input_mouse_lowerwithmmb;
  wchar_t *input_mouse_rollwithtbscroll;
  wchar_t *input_mouse_mmb_hc;
  wchar_t *input_mouse_mb4_hc;
  wchar_t *input_mouse_mb5_hc;

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
  wchar_t *input_actions_menu;
  wchar_t *input_actions_maximizehv;
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
  wchar_t *advanced_normrestore;
  wchar_t *advanced_aerotopmaximizes;
  wchar_t *advanced_aerodbclickshift;
  wchar_t *advanced_maxwithlclick;
  wchar_t *advanced_restoreonclick;
  wchar_t *advanced_fullscreen;
  wchar_t *advanced_titlebarmove;

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

struct strings l10n_ini;

struct {
  wchar_t **str;
  wchar_t *name;
} l10n_mapping[] = {
  { &l10n_ini.code,                            L"Code" },
  { &l10n_ini.lang_english,                    L"LangEnglish" },
  { &l10n_ini.lang,                            L"Lang" },
  { &l10n_ini.author,                          L"Author" },

  { &l10n_ini.menu_enable,                     L"MenuEnable" },
  { &l10n_ini.menu_disable,                    L"MenuDisable" },
  { &l10n_ini.menu_hide,                       L"MenuHideTray" },
  { &l10n_ini.menu_config,                     L"MenuConfigure" },
  { &l10n_ini.menu_about,                      L"MenuAbout" },
  { &l10n_ini.menu_savezones,                  L"MenuSaveZones" },
  { &l10n_ini.menu_exit,                       L"MenuExit" },

  { &l10n_ini.title,                           L"ConfigTitle" },
  { &l10n_ini.tab_general,                     L"ConfigTabGeneral" },
  { &l10n_ini.tab_mouse,                       L"ConfigTabMouse" },
  { &l10n_ini.tab_keyboard,                    L"ConfigTabKeyboard" },
  { &l10n_ini.tab_blacklist,                   L"ConfigTabBlacklist" },
  { &l10n_ini.tab_advanced,                    L"ConfigTabAdvanced" },
  { &l10n_ini.tab_about,                       L"ConfigTabAbout" },

  { &l10n_ini.general_box,                     L"GeneralBox" },
  { &l10n_ini.general_autofocus,               L"GeneralAutoFocus" },
  { &l10n_ini.general_aero,                    L"GeneralAero" },
  { &l10n_ini.general_smartaero,               L"GeneralSmartAero" },
  { &l10n_ini.general_stickyresize,            L"GeneralStickyResize" },
  { &l10n_ini.general_inactivescroll,          L"GeneralInactiveScroll" },
  { &l10n_ini.general_mdi,                     L"GeneralMDI" },
  { &l10n_ini.general_autosnap,                L"GeneralAutoSnap" },
  { &l10n_ini.general_autosnap0,               L"GeneralAutoSnap0" },
  { &l10n_ini.general_autosnap1,               L"GeneralAutoSnap1" },
  { &l10n_ini.general_autosnap2,               L"GeneralAutoSnap2" },
  { &l10n_ini.general_autosnap3,               L"GeneralAutoSnap3" },
  { &l10n_ini.general_language,                L"GeneralLanguage" },
  { &l10n_ini.general_fullwin,                 L"GeneralFullWin" },
  { &l10n_ini.general_resizeall,               L"GeneralResizeAll" },
  { &l10n_ini.general_resizecenter,            L"GeneralResizeCenter" },
  { &l10n_ini.general_resizecenter_norm,       L"GeneralResizeCenterNorm" },
  { &l10n_ini.general_resizecenter_br,         L"GeneralResizeCenterBr" },
  { &l10n_ini.general_resizecenter_move,       L"GeneralResizeCenterMove" },

  { &l10n_ini.general_autostart_box,           L"GeneralAutostartBox" },
  { &l10n_ini.general_autostart,               L"GeneralAutostart" },
  { &l10n_ini.general_autostart_hide,          L"GeneralAutostartHide" },
  { &l10n_ini.general_autostart_elevate,       L"GeneralAutostartElevate" },
  { &l10n_ini.general_autostart_elevate_tip,   L"GeneralAutostartElevateTip" },
  { &l10n_ini.general_elevate,                 L"GeneralElevate" },
  { &l10n_ini.general_elevated,                L"GeneralElevated" },
  { &l10n_ini.general_elevation_aborted,       L"GeneralElevationAborted" },

  { &l10n_ini.input_mouse_box,                 L"InputMouseBox" },
  { &l10n_ini.input_mouse_lmb,                 L"InputMouseLMB" },
  { &l10n_ini.input_mouse_mmb,                 L"InputMouseMMB" },
  { &l10n_ini.input_mouse_rmb,                 L"InputMouseRMB" },
  { &l10n_ini.input_mouse_mb4,                 L"InputMouseMB4" },
  { &l10n_ini.input_mouse_mb5,                 L"InputMouseMB5" },
  { &l10n_ini.input_mouse_scroll,              L"InputMouseScroll" },
  { &l10n_ini.input_mouse_hscroll,             L"InputMouseHScroll" },
  { &l10n_ini.input_mouse_lowerwithmmb,        L"InputMouseLowerWithMMB" },
  { &l10n_ini.input_mouse_rollwithtbscroll,    L"InputMouseRollWithTBScroll" },
  { &l10n_ini.input_mouse_mmb_hc,              L"InputMouseMMBHC" },
  { &l10n_ini.input_mouse_mb4_hc,              L"InputMouseMB4HC" },
  { &l10n_ini.input_mouse_mb5_hc,              L"InputMouseMB5HC" },

  { &l10n_ini.input_aggressive_pause,          L"InputAggressivePause" },
  { &l10n_ini.input_aggressive_kill,           L"InputAggressiveKill" },
  { &l10n_ini.input_scrolllockstate,           L"InputScrollLockState" },
  { &l10n_ini.input_keycombo,                  L"InputKeyCombo" },
  { &l10n_ini.input_grabwithalt,               L"InputGrabWithAlt" },
  { &l10n_ini.input_grabwithaltb,              L"InputGrabWithAltB" },

  { &l10n_ini.input_actions_move,              L"InputActionMove" },
  { &l10n_ini.input_actions_resize,            L"InputActionResize" },
  { &l10n_ini.input_actions_close,             L"InputActionClose" },
  { &l10n_ini.input_actions_kill,              L"InputActionKill" },
  { &l10n_ini.input_actions_minimize,          L"InputActionMinimize" },
  { &l10n_ini.input_actions_maximize,          L"InputActionMaximize" },
  { &l10n_ini.input_actions_lower,             L"InputActionLower" },
  { &l10n_ini.input_actions_roll,              L"InputActionRoll" },
  { &l10n_ini.input_actions_alwaysontop,       L"InputActionAlwaysOnTop" },
  { &l10n_ini.input_actions_borderless,        L"InputActionBorderless" },
  { &l10n_ini.input_actions_center,            L"InputActionCenter" },
  { &l10n_ini.input_actions_nothing,           L"InputActionNothing" },
  { &l10n_ini.input_actions_alttab,            L"InputActionAltTab" },
  { &l10n_ini.input_actions_volume,            L"InputActionVolume" },
  { &l10n_ini.input_actions_menu,              L"InputActionMenu" },
  { &l10n_ini.input_actions_maximizehv,        L"InputActionMaximizeHV" },
  { &l10n_ini.input_actions_transparency,      L"InputActionTransparency" },
  { &l10n_ini.input_actions_hscroll,           L"InputActionHScroll" },

  { &l10n_ini.input_hotkeys_box,               L"InputHotkeysBox" },
  { &l10n_ini.input_hotkeys_modkey,            L"InputHotkeysModKey" },
  { &l10n_ini.input_hotclicks_box,             L"InputHotclicksBox" },
  { &l10n_ini.input_hotclicks_more,            L"InputHotclicksMore" },
  { &l10n_ini.input_hotkeys_leftalt,           L"InputHotkeysLeftAlt" },
  { &l10n_ini.input_hotkeys_rightalt,          L"InputHotkeysRightAlt" },
  { &l10n_ini.input_hotkeys_leftwinkey,        L"InputHotkeysLeftWinkey" },
  { &l10n_ini.input_hotkeys_rightwinkey,       L"InputHotkeysRightWinkey" },
  { &l10n_ini.input_hotkeys_leftctrl,          L"InputHotkeysLeftCtrl" },
  { &l10n_ini.input_hotkeys_rightctrl,         L"InputHotkeysRightCtrl" },
  { &l10n_ini.input_hotkeys_leftshift,         L"InputHotkeysLeftShift" },
  { &l10n_ini.input_hotkeys_rightshift,        L"InputHotkeysRightShift" },
  { &l10n_ini.input_hotkeys_more,              L"InputHotkeysMore" },

  { &l10n_ini.blacklist_box,                   L"BlacklistBox" },
  { &l10n_ini.blacklist_processblacklist,      L"BlacklistProcessBlacklist" },
  { &l10n_ini.blacklist_blacklist,             L"BlacklistBlacklist" },
  { &l10n_ini.blacklist_scrolllist,            L"BlacklistScrolllist" },
  { &l10n_ini.blacklist_mdis,                  L"BlacklistMDIs" },
  { &l10n_ini.blacklist_pause,                 L"BlacklistPause" },
  { &l10n_ini.blacklist_findwindow_box,        L"BlacklistFindWindowBox" },

  { &l10n_ini.advanced_metrics_box,            L"AdvancedMetricsBox"},
  { &l10n_ini.advanced_centerfraction,         L"AdvancedCenterFraction"},
  { &l10n_ini.advanced_aerohoffset,            L"AdvancedAeroHoffset"},
  { &l10n_ini.advanced_aerovoffset,            L"AdvancedAeroVoffset"},
  { &l10n_ini.advanced_snapthreshold,          L"AdvancedSnapThreshold"},
  { &l10n_ini.advanced_aerothreshold,          L"AdvancedAeroThreshold"},
  { &l10n_ini.advanced_aerospeed,              L"AdvancedAeroSpeed"},
  { &l10n_ini.advanced_testwindow,             L"AdvancedTestWindow"},
  { &l10n_ini.advanced_movetrans,              L"AdvancedMoveTrans"},

  { &l10n_ini.advanced_behavior_box,           L"AdvancedBehaviorBox"},
  { &l10n_ini.advanced_multipleinstances,      L"AdvancedMultipleInstances"},
  { &l10n_ini.advanced_autoremaximize,         L"AdvancedAutoRemaximize"},
  { &l10n_ini.advanced_normrestore,            L"AdvancedNormRestore"},
  { &l10n_ini.advanced_aerotopmaximizes,       L"AdvancedAeroTopMaximizes"},
  { &l10n_ini.advanced_aerodbclickshift,       L"AdvancedAeroDBClickShift"},
  { &l10n_ini.advanced_maxwithlclick,          L"AdvancedMaxWithLClick"},
  { &l10n_ini.advanced_restoreonclick,         L"AdvancedRestoreOnClick"},
  { &l10n_ini.advanced_fullscreen,             L"AdvancedFullScreen"},
  { &l10n_ini.advanced_titlebarmove,           L"AdvancedTitlebarMove"},

  { &l10n_ini.about_box,                       L"AboutBox" },
  { &l10n_ini.about_version,                   L"AboutVersion" },
  { &l10n_ini.about_author,                    L"AboutAuthor" },
  { &l10n_ini.about_license,                   L"AboutLicense" },
  { &l10n_ini.about_translation_credit,        L"AboutTranslationCredit" },

  { &l10n_ini.unhook_error,                    L"MiscUnhookError" },
  { &l10n_ini.zone_confirmation,               L"MiscZoneConfirmation" },
};

struct strings en_US = {
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
 /* ResizeAll          */ L"&Resize all windows",
 /* ResizeCenter       */ L"Center resize mode",
 /* ResizeCenterNorm   */ L"All d&irections",
 /* ResizeCenterBr     */ L"B&ottom right",
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
 /* lmb                */ L"Left mouse &button:",
 /* mmb                */ L"&Middle mouse button:",
 /* rmb                */ L"Ri&ght mouse button:",
 /* mb4                */ L"Mouse button &4:",
 /* mb5                */ L"Mouse button &5:",
 /* scroll             */ L"&Scroll wheel:",
 /* hscroll            */ L"Scroll wheel (&horizontal):",
 /* lowerwithmmb       */ L"&Lower windows by middle clicking on title bars",
 /* rollwithtbscroll   */ L"&Roll/Unroll windows with Alt+Scroll on title bars",
 /* mmb_hr             */ L"M&iddle mouse button",
 /* mb4_hc             */ L"M&ouse button 4",
 /* mb5_hc             */ L"Mo&use button 5",

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
 /* menu               */ L"Action menu",
 /* maximizehv         */ L"Maximize &Vertically",
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
 /* normrestore      */   L"Restore AltSnapped windows with &normal move",
 /* aerotopmaximizes */   L"&Maximize windows snapped at top",
 /* aerodbclickshift */   L"Invert shift &behavior for double-click aero snapping",
 /* maxwithlclick    */   L"&Toggle maximize state with right-click while moving",
 /* restoreonclick   */   L"Rest&ore window with single click like original AltDrag",
 /* fullscreen       */   L"Enable on &full screen windows",
 /* titlebarmove     */   L"&Use AltSnap for normal titlebar movement",

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
