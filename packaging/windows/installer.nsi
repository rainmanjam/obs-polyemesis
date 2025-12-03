; OBS Polyemesis - Windows Installer Script
; NSIS (Nullsoft Scriptable Install System) Script
;
; This script creates a Windows installer that installs the plugin
; into the OBS Studio plugins directory.
;
; Requirements:
;   - NSIS 3.x or later
;   - Built plugin files in build\Release
;
; Build command:
;   makensis /DVERSION=1.0.0 packaging\windows\installer.nsi
;
;--------------------------------

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

;--------------------------------
; General Configuration

; Plugin information
!ifndef VERSION
  !define VERSION "1.0.0"
!endif

!define PRODUCT_NAME "OBS Polyemesis"
!define PRODUCT_PUBLISHER "rainmanjam"
!define PRODUCT_WEB_SITE "https://github.com/rainmanjam/obs-polyemesis"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\obs-polyemesis"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Installer properties
Name "${PRODUCT_NAME} ${VERSION}"
OutFile "..\..\build\installers\obs-polyemesis-${VERSION}-windows-x64.exe"
InstallDir "$APPDATA\obs-studio\plugins\obs-polyemesis"
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

; Installer attributes
RequestExecutionLevel user
SetCompressor /SOLID lzma
Unicode True

; Version information
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "© 2025 ${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${VERSION}"

;--------------------------------
; Interface Settings

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\nsis.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_TITLE "${PRODUCT_NAME} Installation Complete"
!define MUI_FINISHPAGE_TEXT "The plugin has been installed to your OBS Studio plugins directory.$\r$\n$\r$\nTo use the plugin:$\r$\n1. Launch OBS Studio$\r$\n2. Go to View → Docks → Restreamer Control$\r$\n3. Configure your restreamer connection"
!define MUI_FINISHPAGE_LINK "Visit the project on GitHub"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Sections

Section "OBS Polyemesis Plugin (required)" SEC01
  SectionIn RO

  ; Set output path to bin/64bit subdirectory (required by OBS plugin structure)
  SetOutPath "$INSTDIR\bin\64bit"

  ; Put plugin DLL in bin/64bit
  File "..\..\build\Release\obs-polyemesis.dll"
  File /nonfatal "..\..\build\Release\obs-polyemesis.pdb"

  ; Write the installation path into the registry
  WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\bin\64bit\obs-polyemesis.dll"

  ; Write the uninstall keys for Windows
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\64bit\obs-polyemesis.dll"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteUninstaller "$INSTDIR\uninst.exe"

  ; Get installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"

SectionEnd

Section "Locale Files" SEC02
  SetOutPath "$INSTDIR\data\locale"
  File /r "..\..\data\locale\*.*"
SectionEnd

;--------------------------------
; Section Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Core plugin files (required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Localization files for different languages"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Installer Functions

Function .onInit
  ; Check if OBS Studio is installed
  ReadRegStr $0 HKLM "SOFTWARE\OBS Studio" "InstallLocation"
  ${If} $0 == ""
    ReadRegStr $0 HKCU "SOFTWARE\OBS Studio" "InstallLocation"
  ${EndIf}

  ${If} $0 == ""
    MessageBox MB_YESNO|MB_ICONEXCLAMATION \
      "OBS Studio does not appear to be installed on this system.$\r$\n$\r$\nThe plugin requires OBS Studio 28.0 or later (Tested with 32.0.2).$\r$\n$\r$\nDo you want to continue with the installation anyway?" \
      IDYES +2
    Abort
  ${EndIf}

  ; Check if plugin is already installed
  ${If} ${FileExists} "$INSTDIR\obs-polyemesis.dll"
    MessageBox MB_YESNO|MB_ICONQUESTION \
      "An existing installation of ${PRODUCT_NAME} was detected.$\r$\n$\r$\nDo you want to overwrite it?" \
      IDYES +2
    Abort
  ${EndIf}
FunctionEnd

Function .onInstSuccess
  MessageBox MB_OK "Installation complete!$\r$\n$\r$\nThe plugin has been installed to:$\r$\n$INSTDIR$\r$\n$\r$\nPlease restart OBS Studio if it is currently running."
FunctionEnd

;--------------------------------
; Uninstaller Section

Section Uninstall
  ; Remove registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"

  ; Remove files and directories
  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\bin"
  Delete "$INSTDIR\uninst.exe"

  ; Remove installation directory if empty
  RMDir "$INSTDIR"

  SetAutoClose true
SectionEnd

;--------------------------------
; Uninstaller Functions

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 \
    "Are you sure you want to completely remove ${PRODUCT_NAME} and all of its components?" \
    IDYES +2
  Abort
FunctionEnd

Function un.onUninstSuccess
  MessageBox MB_ICONINFORMATION|MB_OK \
    "${PRODUCT_NAME} was successfully removed from your computer."
FunctionEnd
