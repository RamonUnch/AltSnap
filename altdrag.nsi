# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# define the name of the installer
!define APP_NAME "AltDrag"
!define APP_VERSION "1.42"

# define the name of the installer
OutFile "${APP_NAME}${APP_VERSION}-inst.exe"
Name "${APP_NAME} ${APP_VERSION}"

InstallDir "$APPDATA\${APP_NAME}\"
InstallDirRegKey HKCU "Software\${APP_NAME}" "Install_Dir"
;RequestExecutionLevel user
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma


;!include "LogicLib.nsh"
;!include "FileFunc.nsh"

; The text to prompt the user to enter a directory
DirText "This will install AltDrag on your computer. Choose a directory"

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
    File "Lang\_en_US baseline.txt"
    File Lang\ca_ES.ini
    File Lang\de_DE.ini
    File Lang\es_ES.ini
    File Lang\fr_FR.ini
    File Lang\gl_ES.ini
    File Lang\it_IT.ini
    File Lang\ja_JP.ini
    File Lang\ko_KR.ini
    File Lang\nb_NO.ini
    File Lang\nl_NL.ini
    File Lang\pl_PL.ini
    File Lang\pt_PR.ini
    File Lang\ru_RU.ini
    File Lang\sz_SK.ini
    File Lang\zh_CN.ini
    File Lang\zh_TW.ini

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
    ;${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    ;IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "286"

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
    Delete "Lang\_en_US baseline.txt"
    Delete Lang\ca_ES.ini
    Delete Lang\de_DE.ini
    Delete Lang\es_ES.ini
    Delete Lang\fr_FR.ini
    Delete Lang\gl_ES.ini
    Delete Lang\it_IT.ini
    Delete Lang\ja_JP.ini
    Delete Lang\ko_KR.ini
    Delete Lang\nb_NO.ini
    Delete Lang\nl_NL.ini
    Delete Lang\pl_PL.ini
    Delete Lang\pt_PR.ini
    Delete Lang\ru_RU.ini
    Delete Lang\sz_SK.ini
    Delete Lang\zh_CN.ini
    Delete Lang\zh_TW.ini
    RMDir "$INSTDIR\Lang"

    SetOutPath $APPDATA
    RMDir "$INSTDIR"

    Delete $SMPROGRAMS\AltDrag.lnk
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "AltDrag"
    DeleteRegKey /ifempty HKCU "Software\AltDrag"
    DeleteRegKey /ifempty HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AltDrag"

SectionEnd
