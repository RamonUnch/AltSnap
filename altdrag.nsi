# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# define the name of the installer
!define APP_NAME "AltDrag"
!define APP_VERSION "1.38"

# define the name of the installer
OutFile "${APP_NAME}${APP_VERSION}-inst.exe"
Name "${APP_NAME} ${APP_VERSION}"

InstallDir "$APPDATA\${APP_NAME}\"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"
RequestExecutionLevel user
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

!include "LogicLib.nsh"
!include "FileFunc.nsh"

; The text to prompt the user to enter a directory
DirText "This will install AltDrag on your computer. Choose a directory"

SetCompressor /SOLID lzma

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# DEFAULT SECTION
Section

    Call CloseApp

    # define the output path for this file
    SetOutPath $INSTDIR
    ; Rename old ini file if it exists
    IfFileExists "${APP_NAME}.ini" +2 0
        File AltDrag.ini
    ifFileExists "hooks_x64.dll" 0 +2
        File AltDrag.ini

    ; Delete files that existed in earlier versions
    Delete /REBOOTOK "$INSTDIR\info.txt" ; existed in <= 0.9
    Delete /REBOOTOK "$INSTDIR\Config.exe" ; existed in 1.0b1
    Delete /REBOOTOK "$INSTDIR\HookWindows_x64.exe" ; existed in 1.1
    Delete /REBOOTOK "$INSTDIR\hooks_x64.dll" ; existed in 1.1

    # define what to install and place it in the output path
    File AltDrag.exe
    File AltDrag.txt
    File hooks.dll
    File License.txt
    SetOutPath $INSTDIR\Lang
    File Lang\*.*

    SetOutPath $INSTDIR
    CreateShortcut "$SMPROGRAMS\AltDrag.lnk" "$INSTDIR\AltDrag.exe"

    WriteRegStr HKCU "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKCU "Software\${APP_NAME}" "Version" "${APP_VERSION}"

    ; Create uninstaller
    WriteUninstaller "Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\${APP_NAME}.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "HelpLink" "https://github.com/RamonUnch/AltDrag"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "Raymond Gillibert"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR\"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1

    ; Compute size for uninstall information
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "$0"

SectionEnd

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# CLOSEAPP
!define WM_CLOSE 0x0010
!macro CloseApp un
Function ${un}CloseApp
  ; Close app if running
  FindWindow $0 "${APP_NAME}" ""
  IntCmp $0 0 done
    DetailPrint "Attempting to close running ${APP_NAME}..."
    SendMessage $0 ${WM_CLOSE} 0 0 /TIMEOUT=500
    waitloop:
      Sleep 10
      FindWindow $0 "${APP_NAME}" ""
      IntCmp $0 0 closed waitloop waitloop
  closed:
  Sleep 100 ; Sleep a little extra to let Windows do its thing

  done:
FunctionEnd
!macroend
!insertmacro CloseApp ""
!insertmacro CloseApp "un."

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# UNINSTALL
Section "Uninstall"

    Call un.CloseApp

    SetOutPath $INSTDIR
    # Always delete uninstaller first
    Delete "$INSTDIR\Uninstall.exe"

    # now delete installed file
    Delete AltDrag.exe
    Delete AltDrag.txt
    Delete AltDrag.ini
    Delete AltDrag-old.ini
    Delete hooks.dll
    Delete License.txt
    Delete Lang\*.*
    RMDir "$INSTDIR\Lang"

    SetOutPath $APPDATA
    RMDir "$INSTDIR"

    Delete $SMPROGRAMS\AltDrag.lnk
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "AltDrag"
    DeleteRegKey /ifempty HKCU "Software\AltDrag"
    DeleteRegKey /ifempty HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AltDrag"

SectionEnd
