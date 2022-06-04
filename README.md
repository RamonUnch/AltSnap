English | [中文](./README_zh-CN.md) | [한국어](./README_ko-KR.md)
# AltSnap
Fork from Stefan Sundin's AltDrag.

Check the Wiki: https://github.com/RamonUnch/AltSnap/wiki

Original documentation: https://stefansundin.github.io/altdrag/doc/

Note that the documentation is not 100% accurate because it is a fork.
Read the changelog for more details.

It allows you to move and resize windows by using the Alt key and clicking wherever on the window instead of relying on very precise clicking.
This behavior is very common on Linux distributions and is not actually hard to implement on Windows.

This fork tries to keep a version up to date with minimal amount of bugs while keeping it feature-rich.

It is oriented towards all Windows users from Windows NT 4 to Windows 11, even though it is mostly tested on Windows XP and Windows 10.

Main differences:
To simplify the code greatly the Hooks windows feature was removed, it allowed you have windows snapping while dragging them normally. It required however to inject a dll in every application and induced thus an obvious security risk. The amount of mess added to the code just for this feature was substantial and in addition forced to have both a 32bits and a 64bits version of the program running at the same time.

This version injects nothing into other applications. This means you do not have to worry whether you have a 32 or a 64bit operating system.

Another feature that was disabled is focus on typing, that was too much unusable for me to even start testing, so I removed it.

Otherwise this has a much simpler source code, added a few extra options, such as transparent windows dragging, Maximize action, pause process options, more blacklists for finer control of AltSnap etc. 

Finally it fixed a ton of undesired behavior and bugs from the original AltDrag.

WHAT'S NEW

Many new features can be seen in the option dialog box, however some of them are only available through editing the AltSnap.ini file (middle click on tyhe tray icon for this).

# Build
AltSnap builds with gcc, I use Mingw-w64 (for i686).
Just install the latest version (I use TDM-gcc 10.3, MinGW64 based) and use:
`> make` for i386 Win32 build
`> make -fMakefileX64` for x86_64 build.
`> make -fMakefileX64db` for x86_64 debug build.
You can also use mk.bart and mk64.bat files.
