# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# define the name of the installer

!define APP_NAME "AltSnap"
!define APP_VERSION "1.66"
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
;!include "x64.nsh"

; The text to prompt the user to enter a directory
DirText "This will install AltSnap on your computer. Choose a directory"
Page directory
Page instfiles
Page custom customPage "" ": custom page"

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# DEFAULT SECTION
Section

    Call CloseApp

    # define the output path for this file
    SetOutPath $INSTDIR

    File AltSnap.dni

    # define what to install and place it in the output path
    File AltSnap.exe
    File AltSnap.txt
    File AltSnap.xml
    File hooks.dll
    File License.txt
    File sch_On.bat
    File sch_Off.bat
    SetOutPath $INSTDIR\Lang
    File "Lang\_en_US baseline.txt"
    File "Lang\ca_ES.ini"
    File "Lang\de_DE.ini"
    File "Lang\es_ES.ini"
    File "Lang\fi_FI.ini"
    File "Lang\fr_FR.ini"
    File "Lang\gl_ES.ini"
    File "Lang\it_IT.ini"
    File "Lang\ja_JP.ini"
    File "Lang\ko_KR.ini"
    File "Lang\nb_NO.ini"
    File "Lang\nl_NL.ini"
    File "Lang\pl_PL.ini"
    File "Lang\pt_PR.ini"
    File "Lang\ru_RU.ini"
    File "Lang\sz_SK.ini"
    File "Lang\tr_TR.ini"
    File "Lang\uk_UA.ini"
    File "Lang\zh_CN.ini"
    File "Lang\zh_TW.ini"

    SetOutPath $INSTDIR\Themes\erasmion
    File Themes\erasmion\TRAY_OFF.ico
    File Themes\erasmion\TRAY_ON.ico
    File Themes\erasmion\TRAY_SUS.ico

    SetOutPath $INSTDIR
    CreateShortcut "$SMPROGRAMS\AltSnap.lnk" "$INSTDIR\AltSnap.exe"

    WriteRegStr HKCU "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
    WriteRegStr HKCU "Software\${APP_NAME}" "Version" "${APP_VERSION}"

    ; Create uninstaller
    WriteUninstaller "Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\${APP_NAME}.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "HelpLink" "https://github.com/RamonUnch/AltSnap/wiki"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "Raymond Gillibert"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR\"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1

    ; Compute size for uninstall information
    ;${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    ;IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "400"

SectionEnd

Function customPage
  MessageBox MB_YESNO "Run AltSnap now?" IDNO NoRunNow
    Exec "$INSTDIR\AltSnap.exe" ; view readme or whatever, if you want.
  NoRunNow:
FunctionEnd

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
    Delete "$INSTDIR\AltSnap.exe"
    Delete "$INSTDIR\AltSnap.txt"
    Delete "$INSTDIR\AltSnap.xml"
    Delete "$INSTDIR\AltSnap.dni"
    Delete "$INSTDIR\AltSnap.ini"
    Delete "$INSTDIR\AltSnap-old.ini"
    Delete "$INSTDIR\hooks.dll"
    Delete "$INSTDIR\License.txt"
    Delete "$INSTDIR\sch_On.bat"
    Delete "$INSTDIR\sch_Off.bat"

    Delete "Lang\_en_US baseline.txt"
    Delete "Lang\ca_ES.ini"
    Delete "Lang\de_DE.ini"
    Delete "Lang\es_ES.ini"
    Delete "Lang\fi_FI.ini"
    Delete "Lang\fr_FR.ini"
    Delete "Lang\gl_ES.ini"
    Delete "Lang\it_IT.ini"
    Delete "Lang\ja_JP.ini"
    Delete "Lang\ko_KR.ini"
    Delete "Lang\nb_NO.ini"
    Delete "Lang\nl_NL.ini"
    Delete "Lang\pl_PL.ini"
    Delete "Lang\pt_PR.ini"
    Delete "Lang\ru_RU.ini"
    Delete "Lang\sz_SK.ini"
    Delete "Lang\tr_TR.ini"
    Delete "Lang\uk_UA.ini"
    Delete "Lang\zh_CN.ini"
    Delete "Lang\zh_TW.ini"

    RMDir "$INSTDIR\Lang"

    Delete Themes\erasmion\TRAY_OFF.ico
    Delete Themes\erasmion\TRAY_ON.ico
    Delete Themes\erasmion\TRAY_SUS.ico
    RMDir "$INSTDIR\Themes\erasmion"
    RMDir "$INSTDIR\Themes"

    SetOutPath $APPDATA
    RMDir "$INSTDIR"

    Delete $SMPROGRAMS\AltSnap.lnk
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "AltSnap"
    DeleteRegKey /ifempty HKCU "Software\AltSnap"
    DeleteRegKey /ifempty HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AltSnap"

SectionEnd
