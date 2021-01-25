# AltDrag
Fork from Stefan Sundin's AltDrag.

Documentation: https://stefansundin.github.io/altdrag/doc/

Note that documentation is not 100% accurate because it is a fork.
Read the changelog for more details.

It allows you to move and resize windows by using the Alt key and clicking wherever on the window instead of relying on very precise clicking.
This behavior is very common on Linux distributions and is not actually hard to implement on Windows.

This fork tries to keep a version up to date with minimal amount of bugs while keeping it feature-rich.

It is oriented towards all Windows users from Windows NT 4 to Windows 10, even though it is mostly tested on Windows XP and Windows 10.

Main differences:
To simplify the code greatly the Hooks windows feature was removed, it allowed you have windows snapping while dragging them normally. It required however to inject a dll in every application and induced thus an obvious security risk. The amount of mess added to the code just for this feature was substantial and in addition forced to have both a 32bits and a 64bits version of the program running at the same time.

This version injects nothing into other applications. This means you do not have to worry whether you have a 32 or a 64bit operating system.

Another feature that was disabled is focus on typing, that was too much unusable for me to even start testing, so I removed it.

Otherwise this has a much simpler source code, added a few extra options, such as transparent windows dragging, Maximize action, pause process options, more blacklists for finer control of AltDrag. 

Finally it fixed a ton of undesired behavior and bugs from the original AltDrag.

WHAT'S NEW

Many new features can be seen in the option dialog box, however some of them are only available through editing the AltDrag.ini file (middle click on tyhe tray icon for this).


in [General] section:


AeroHoffset=50

AeroVoffset=50

; Horizontal ans vertical position (from top left in percent) where the aero windows will meet default is 50 50, center of the monotor. If you set AeroHoffset=33 for example, it means that the windows snapped on the left will use only 33% of the monitor width, those on the right will have the remaining 67%. I really advise to play around with this if you use Aero snapping...


CenterFraction=24

; Fraction in percent from 0 to 100 that defines the size of the central resizing region (default is 24).


Blacklists were also modified. There are 4 blacklist plus an additional 5th hidden one:


MMBLower=*|CASCADIA_HOSTING_WINDOW_CLASS

; List of window that should NOT be lowered by midle click on the titlebar.

Some more [Advanced] options:


AeroThreshold=5

; Distance in pixels when Aero snapping to monitor sides (default 5).


ResizeAll=1

; Set to 1 to be able to resize all windows even those without borders.

AeroTopMaximizes=1

; Enable if you want the window to be maximized when snapped at the top of the monitor. You can always hold Shift to invert the behavior.


UseCursor=1

; Use 0 to disable any cursor handeling.

; Use 1 to have all cursors set (default)

; Use 2 in order to disable the Hand cursor when moving

; Use 3 to always use the normal cursor, even when resizing.


PearceDBClick=0

; Set to 1 to disable the maximizing/restore on Alt+dboube-click. Instead the double-click will "pearce" through the move action.

MinAlpha=8
; Minimum alpha for the transparency action, from 0-255 (default 8).

And finally in [Performance] section
RefreshRate=7
; Minimum delay in miliseconds between two refresh of the window. I advise a value slightly lower than your refresh rate ie: 60Hz monitor => RefreshRate=16 Max value is 255 (4 Hz), sane values are below 100 (10 Hz). Use 0 if you want the most reactivity.

# Build
AltDrag builds with gcc, I use Mingw-w64 (for i686).
Just install the latest version (I use gcc 8.1) and lunch mk.bat
