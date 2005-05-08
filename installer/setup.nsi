!define DEBUG
!define WXDIR "d:\wx"
!define SYSTEMDIR "c:\windows\system32"

!define PRODUCT_NAME "Multicrew"
!define PRODUCT_VERSION "snapshot"
!define PRODUCT_WEB_SITE "http://www.multicrew.org"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

;SetCompressor bzip2

!include "MUI.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "multicrew\gpl.txt"

!define MUI_DIRECTORYPAGE_VERIFYONLEAVE 
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE dirLeave
!define MUI_DIRECTORYPAGE_TEXT_TOP "Please select the directory of Flight Simulator 2004, i.e. the one where you find fs9.exe."
!insertmacro MUI_PAGE_DIRECTORY

Function dirLeave
  GetInstDirError $0
  IfFileExists $INSTDIR\fs9.exe done 0
   MessageBox MB_OK "Can't find Microsoft Flight Simulator 2004 in this directory."
   Abort
done:

FunctionEnd

!insertmacro MUI_PAGE_COMPONENTS

var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Multicrew"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

!ifdef DEBUG
       !define BUILDDIR "Debug"
       !define WXDLL "wxmsw24d"
       !define CRTDLL "msvcrtd"
       !define MSVCP60DLL "msvcp60d"
!else
       !define BUILDDIR "Release"
       !define WXDLL "wxmsw24"
       !define CRTDLL "msvcrt"
       !define MSVCP60DLL "msvcp60"
!endif

Name "${PRODUCT_NAME} ${PRODUCT_VERSION} ${__DATE__}"
OutFile "multicrew-snapshot-${__DATE__}.exe"
InstallDir "c:\fs9"
ShowInstDetails show
ShowUnInstDetails show

Section "!Multicrew" SEC01
  SetOverwrite ifnewer
  SetOutPath "$INSTDIR\Multicrew"
  File "..\multicrewgauge\${BUILDDIR}\multicrewgauge.dll"
  File "multicrew\gpl.txt"
  SetOutPath "$INSTDIR\Multicrew\..\modules"
  File "..\multicrewui\${BUILDDIR}\multicrewui.dll"
  SetOutPath "$INSTDIR\Multicrew\.."
  File "..\multicrewcore\${BUILDDIR}\multicrewcore.dll"
  File "..\multicrewprepare\${BUILDDIR}\multicrewprepare.exe"
  File "..\multicrewprepare\${BUILDDIR}\multicrewpanelprepare.exe"
  File "${WXDIR}\lib\${WXDLL}.dll"
  SetOutPath "$SYSDIR"
  File "${SYSTEMDIR}\${CRTDLL}.dll"
  File "${SYSTEMDIR}\${MSVCP60DLL}.dll"

; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "PMDG 737NG 600/700 (with and without upgrade)" SEC02
  SetOverwrite ifnewer
  SetOutPath "$INSTDIR\Multicrew"
  File "multicrew\PMDG*.*"

  SetOutPath "$INSTDIR\Multicrew\.."
;  Exec '"multicrewpanelprepare.exe" Aircraft\PMDG737-700\panel\Panel.CFG'
SectionEnd

Section "PMDG 737NG 800/900" SEC03
  SetOverwrite ifnewer
  SetOutPath "$INSTDIR\Multicrew"
  File "multicrew\PMDG*.*"

  SetOutPath "$INSTDIR\Multicrew\.."
;  Exec '"multicrewpanelprepare.exe" Aircraft\PMDG737-900\panel\Panel.CFG'
SectionEnd

Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  WriteIniStr "$INSTDIR\Multicrew\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Website.lnk" "$INSTDIR\Multicrew\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\Multicrew\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\Multicrew\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "fs9dir" "$INSTDIR"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Multicrew\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Multicrew\..\multicrewcore.dll"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "The multicrew main package"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "PMDG 737NG support for the 600/700 version, with and without the 800/900 upgrade"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "PMDG 737NG support for the 800/900 version"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\..\${WXDLL}.dll"
  Delete "$INSTDIR\..\multicrewcore.dll"
  Delete "$INSTDIR\gpl.txt"
  Delete "$INSTDIR\multicrewgauge.dll"
  Delete "$INSTDIR\..\multicrewprepare.exe"
  Delete "$INSTDIR\..\multicrewpanelprepare.exe"
  Delete "$INSTDIR\..\modules\multicrewui.dll"

  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"

  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  RMDir "$INSTDIR\Multicrew"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "Software\Multicrew"
  SetAutoClose true
SectionEnd