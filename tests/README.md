# Stealth mode regression tests

Run on Windows with a MinGW-w64 GCC toolchain, from the repository root:

```powershell
gcc -std=c99 -Wall -Wextra -o tests/stealth_test.exe tests/stealth_test.c -luser32
.\tests\stealth_test.exe

gcc -std=c99 -O1 -ffunction-sections -fdata-sections '-Wl,--gc-sections' -o tests/menu_test.exe tests/menu_test.c -lcomctl32 -lshell32 -ladvapi32 -lgdi32 -luser32
.\tests\menu_test.exe

gcc -std=c99 -O1 -ffunction-sections -fdata-sections '-Wl,--gc-sections' -o tests/hooks_test.exe tests/hooks_test.c -lcomctl32 -lshell32 -ladvapi32 -lgdi32 -luser32 -lwinmm
.\tests\hooks_test.exe
```

`stealth_test.c` includes the production `stealth.c` module and uses real, isolated Win32
windows placed offscreen. It substitutes cursor position and elapsed time, so it
does not move the user's cursor, send input, interact with existing applications,
or wait for real hiding delays. It checks externally observable window attributes,
including opacity, color keys, styles, size, and topmost state.
It also checks real Windows hit testing at an offscreen point to confirm a hidden
window retains its input region.

The cases cover delayed hiding, continuous 300-millisecond reveal dwell inside
the actual window bounds, the 299/300-millisecond boundary, interrupted dwell,
independent window dwell timers, outside-edge tolerance, repeated toggling, opacity
changes, menu and movement protection, multiple windows, destroyed windows,
native hiding and minimization, owned popups, native menu/move/capture loops,
unavailable cursor position, temporary drag opacity, handle-ownership invalidation,
clock rollover, unsupported layered windows, and shutdown restoration.
Real offscreen covering windows verify that covered geometry does not accumulate
reveal time, becoming covered resets a partial dwell, and a topmost target can
reveal when Windows hit testing selects it above an ordinary covering window.

IME tests intercept WinEvent subscriptions and call the production callback with
simulated `IME_SHOW`, `IME_HIDE`, and `IME_CHANGE` events. Real offscreen windows
and edit controls test input-focus ownership, external unowned candidates,
candidate replacement, object IDs, native hiding/destruction, handle identity
changes, foreground changes, subscription failure, and shutdown. They verify
that composition interaction preserves the window and its opacity, normal hiding
resumes afterward, and unrelated input focus or candidate windows do not bypass
occlusion rules. No desktop event hook or real input method is used by the tests.

The observed WeType generic `SHOW`/`HIDE` fallback is also tested with a real
unowned `wetype.flutter.setting` fixture. Positive matching requires the exact
class, all four observed extended-style flags, a positive window size, and the
correct foreground input focus. Ordinary windows, similar class prefixes,
activatable settings windows, missing flags, zero-size windows, and other
applications are excluded. Tests check generic hiding, destruction, style
changes, independent official sessions, partial subscription failure, and cleanup
of both distinct event subscriptions.

`menu_test.c` includes the production `altsnap.c` code and captures the action menu
without displaying it or sending messages. It checks the new action, checked
state, translations, and compatibility with existing `ACMenuItems` bit masks.

`hooks_test.c` includes the production `hooks.c` code and exercises its action
dispatch, timer, temporary drag opacity, transparency actions, and pin
synchronization using isolated offscreen windows. It verifies that holding a
movement action before the first mouse move does not save temporary drag opacity,
that first enabling stealth during a drag preserves the original opacity and
layered drawing mode, and that pins remain hidden until their owner completes its
reveal dwell. Only pins hidden by stealth are later restored. It never calls `Load`
or installs system hooks, and its IME case intercepts the event subscription while
exercising the production visibility timer. The compilation may report the existing pointer-cast
warning in the production `Unload` function.

The complete Alt+middle-button interaction and appearance still require a manual
check in AltSnap.
