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

# VirusTotal false positive
You will see with the latest builds that there are some alerts, usually from SecureAge APEX and sometimes from other vendors. Those are false positives, and I stopped contacting the APEX team for every release because it is a waste of time I would rather spend on improving the program.
I already reduced the number of false positives significantly, simply by changing build flags and by switching to an older version of the NSIS installer system. This is an indication of the impertinence of some modern antivirus solutions. Chocolatey considers that up to five positives on VirusTotal is not even suspicious.

# Build
AltSnap builds with gcc, I use Mingw-w64 (for i686).
Just install the latest version (I use TDM-gcc 10.3, MinGW64 based) and use:

`> make` for i386 Win32 GCC build.

`> make -fMakefiledb` for i386 GCC debug build.

`> make -fMakefileX64` for x86_64 GCC build.

`> make -fMakefileX64db` for x86_64 GCC debug build.

`> make -fMakefileClang` for i386 build using LLVM Clang.

`> make -fMakefileTCC` for i386 build using tcc, [Bellard's thiny c compiler](https://bellard.org/tcc/)

You can also use mk.bat and mk64.bat files.
For Clang, I use LLVM5.0.1 with the headers and libs from Mingw-w64.
Be sure to adjust your include and lib directorries on the command line with `-IPath\to\mingw\include` and `-LPath\to\mingw\lib`.
