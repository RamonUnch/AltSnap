#ifndef AS_RESOURCE_H
#define AS_RESOURCE_H

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define VERSIONRC     1,6,6,0
#define APP_VERSION  "1.66"

#define APP_ICON                         101
#define TRAY_OFF                         102
#define TRAY_ON                          103
#define TRAY_SUS                         104

#define IDD_GENERALPAGE                  201
#define IDD_MOUSEPAGE                    202
#define IDD_KBPAGE                       203
#define IDD_BLACKLISTPAGE                204
#define IDD_ADVANCEDPAGE                 205
#define IDD_ABOUTPAGE                    206

#define IDI_FIND                         301
#define IDI_BIGICON                      302
#define IDC_HELPPANNEL                   303

#define IDC_AUTOSNAP_HEADER             1000
#define IDC_FINDWINDOW                  1000
#define IDC_CHECKNOW                    1001
#define IDC_LANGUAGE_HEADER             1001
#define IDC_TTBACTIONSNA                1001
#define IDC_TRANSLATIONS_BOX            1001
#define IDC_INI                         1002
#define IDC_NEWRULE                     1002
#define IDC_MDI                         1003
#define IDC_SCROLL                      1003
#define IDC_TRANSLATIONS                1003
#define IDC_AUTOSAVE                    1004
#define IDC_PROCESSBLACKLIST_HEADER     1004
#define IDC_SCROLL_HEADER               1004
#define IDC_AUTOSTART_ELEVATE           1005
#define IDC_BLACKLIST_HEADER            1006
#define IDC_ELEVATE                     1006
#define IDC_LMB_HEADER                  1006
#define IDC_MMB_HEADER                  1007
#define IDC_RMB_HEADER                  1008
#define IDC_CHECKONSTARTUP              1009
#define IDC_MB4_HEADER                  1009
#define IDC_BETA                        1010
#define IDC_MB5_HEADER                  1010
#define IDC_FINDWINDOW_BOX              1012
#define IDC_HOTKEYS_MORE                1013
#define IDC_BLACKLIST_BOX               1014
#define IDC_SCROLLLIST_HEADER           1015
#define IDC_BLACKLIST_EXPLANATION       1016
#define IDC_ADVANCED_BOX                1017
#define IDC_GENERAL_BOX                 1018
#define IDC_AUTOSTART_BOX               1019
#define IDC_AUTOFOCUS                   1101
#define IDC_AUTOSNAP                    1102
#define IDC_AERO                        1103
#define IDC_INACTIVESCROLL              1104
#define IDC_LANGUAGE                    1105
#define IDC_LMB                         1201
#define IDC_MMB                         1202
#define IDC_RMB                         1203
#define IDC_MB4                         1204
#define IDC_MB5                         1205
#define IDC_LEFTALT                     1206
#define IDC_RIGHTALT                    1207
#define IDC_LEFTWINKEY                  1208
#define IDC_RIGHTWINKEY                 1209
#define IDC_LEFTCTRL                    1210
#define IDC_RIGHTCTRL                   1211
#define IDC_HOOKWINDOWS                 1401
#define IDC_OPENINI                     1402
#define IDC_DONATE                      1501
#define IDC_SCROLLLIST                  1505
#define IDC_BLACKLIST                   1506
#define IDC_PROCESSBLACKLIST            1507
#define IDC_AUTOSTART                   1508
#define IDC_AUTOSTART_HIDE              1511
#define IDC_MOUSE_BOX                   1511
#define IDC_HOTKEYS_BOX                 1512
#define IDC_FINDWINDOW_EXPLANATION      1513
#define IDC_ABOUT_BOX                   1514
#define IDC_VERSION                     1515
#define IDC_AUTHOR                      1516
#define IDC_LICENSE                     1517
#define IDC_URL                         1518
#define IDC_NEWPROGNAME                 1519
#define IDC_HOTCLICKS_BOX               1520
#define IDC_AUTHOR2                     1521

#define IDC_MBA1                        1601
#define IDC_MBA2                        1602
#define IDC_INTTB                       1603
#define IDC_WHILEM                      1604
#define IDC_WHILER                      1605

#define IDC_FULLWIN                     2008
#define IDC_SHOWCURSOR                  2009
#define IDC_RESIZECENTER                2010
#define IDC_RESIZEALL                   2011

#define IDC_AGGRESSIVEPAUSE             2012
#define IDC_PAUSEBL_HEADER              2013
#define IDC_PAUSEBL                     2014
#define IDC_MDISBL_HEADER               2015
#define IDC_MDIS                        2016

#define IDC_RZCENTER_NORM               2017
#define IDC_RZCENTER_BR                 2018
#define IDC_RZCENTER_MOVE               2019
#define IDC_RZCENTER_CLOSE              2020

#define IDC_AUTOREMAXIMIZE              2021
#define IDC_AEROTOPMAXIMIZES            2022
#define IDC_MULTIPLEINSTANCES           2023
#define IDC_CENTERFRACTION              2024
#define IDC_CENTERFRACTION_H            2025
#define IDC_AEROHOFFSET                 2026
#define IDC_AEROHOFFSET_H               2027
#define IDC_AEROVOFFSET                 2028
#define IDC_AEROVOFFSET_H               2029
#define IDC_SNAPTHRESHOLD               2030
#define IDC_SNAPTHRESHOLD_H             2031
#define IDC_AEROTHRESHOLD               2032
#define IDC_AEROTHRESHOLD_H             2033
#define IDC_NORMRESTORE                 2034
#define IDC_MAXWITHLCLICK               2035
#define IDC_RESTOREONCLICK              2036
#define IDC_TTBACTIONSWA                2037
#define IDC_MMB_HC                      2038
#define IDC_MB4_HC                      2039
#define IDC_MB5_HC                      2040
#define IDC_KEYBOARD_BOX                2041
#define IDC_KEYCOMBO                    2042
#define IDC_HOTCLICKS_MORE              2043
#define IDC_GRABWITHALT                 2044
#define IDC_GRABWITHALT_H               2045
#define IDC_METRICS_BOX                 2046
#define IDC_BEHAVIOR_BOX                2047
#define IDC_MODKEY_H                    2048
#define IDC_MODKEY                      2049
#define IDC_AERODBCLICKSHIFT            2050
#define IDC_FULLSCREEN                  2051
#define IDC_AGGRESSIVEKILL              2052
#define IDC_TESTWINDOW                  2053
#define IDC_MOVETRANS_H                 2054
#define IDC_MOVETRANS                   2055
#define IDC_AEROSPEED_H                 2056
#define IDC_AEROSPEED                   2057
#define IDC_AEROSPEEDTAU                2058
#define IDC_SMARTAERO                   2059
#define IDC_STICKYRESIZE                2060
#define IDC_NCHITTEST                   2061
#define IDC_GWLSTYLE                    2062
#define IDC_HSCROLL                     2063
#define IDC_HSCROLL_HEADER              2064
#define IDC_SCROLLLOCKSTATE             2065
#define IDC_TITLEBARMOVE                2066
#define IDC_GRABWITHALTB                2067
#define IDC_GRABWITHALTB_H              2068
#define IDC_RECT                        2069
#define IDC_TTBACTIONS_BOX              2070
#define IDC_PEARCEDBCLICK               2071

#define IDC_USEZONES                    2072
#define IDC_FANCYZONE                   2073
#define IDC_GWLEXSTYLE                  2074
#define IDC_SMARTERAERO                 2075
#define IDC_NORESTORE                   2076
#define IDC_LONGCLICKMOVE               2077
#define IDC_BLMAXIMIZED                 2078
#define IDC_PIERCINGCLICK               2079

#define IDC_OUTTB                       2083
#define IDC_UNIKEYHOLDMENU              2084
#define IDC_DWMCAPBUTTON                2085
#define IDC_WINHANDLES                  2086

#define IDC_MOVEUP                      2087
#define IDC_MOVEUP_HEADER               2088
#define IDC_RESIZEUP                    2089
#define IDC_RESIZEUP_HEADER             2090
#define IDC_SNAPGAP                     2091
#define IDC_SNAPGAP_H                   2092
#define IDC_NPSTACKED                   2093
#define IDC_TOPMOSTINDICATOR            2094

#define IDC_SHORTCUTS                   2095
#define IDC_SHORTCUTS_H                 2096
#define IDC_SHORTCUTS_AC                2097
#define IDC_SHORTCUTS_SET               2098
#define IDC_SHORTCUTS_PICK              2099
#define IDC_ALT                         2100
#define IDC_WINKEY                      2101
#define IDC_CONTROL                     2102
#define IDC_SHIFT                       2103
#define IDC_SHORTCUTS_VK                2104
#define IDC_SHORTCUTS_CLEAR             2105
#define IDC_USEPTWINDOW                 2106
#define IDC_SIDESFRACTION               2107

#endif // AS_RESOURCE_H
