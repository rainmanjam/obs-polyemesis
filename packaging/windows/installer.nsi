; NSIS Installer Script for OBS Polyemesis
; Creates Windows installer for OBS Studio plugin

!define PRODUCT_NAME "OBS Polyemesis"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "rainmanjam"
!define PRODUCT_WEB_SITE "https://github.com/rainmanjam/obs-polyemesis"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma

!include "MUI2.nsh"
!include "x64.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Installer details
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\..\obs-polyemesis-${PRODUCT_VERSION}-windows-x64.exe"
InstallDir "$APPDATA\obs-studio\plugins\obs-polyemesis"
ShowInstDetails show
ShowUnInstDetails show

; Request admin privileges
RequestExecutionLevel user

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Copy plugin files
  File /r "..\..\build\Release\bin\64bit\*.*"
  File /r "..\..\build\Release\data\*.*"

  ; Create README
  FileOpen $0 "$INSTDIR\README.txt" w
  FileWrite $0 "OBS Polyemesis - Restreamer Control Plugin$\r$\n"
  FileWrite $0 "$\r$\n"
  FileWrite $0 "Installation complete!$\r$\n"
  FileWrite $0 "$\r$\n"
  FileWrite $0 "To use the plugin:$\r$\n"
  FileWrite $0 "1. Launch OBS Studio$\r$\n"
  FileWrite $0 "2. Go to View -> Docks -> Restreamer Control$\r$\n"
  FileWrite $0 "3. Configure your Restreamer connection$\r$\n"
  FileWrite $0 "$\r$\n"
  FileWrite $0 "Documentation: ${PRODUCT_WEB_SITE}$\r$\n"
  FileClose $0
SectionEnd

Section -AdditionalIcons
  CreateDirectory "$SMPROGRAMS\OBS Polyemesis"
  CreateShortCut "$SMPROGRAMS\OBS Polyemesis\Uninstall.lnk" "$INSTDIR\uninst.exe"
  CreateShortCut "$SMPROGRAMS\OBS Polyemesis\Website.lnk" "${PRODUCT_WEB_SITE}"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section Uninstall
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\README.txt"

  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\OBS Polyemesis"

  DeleteRegKey HKCU "${PRODUCT_UNINST_KEY}"
  SetAutoClose true
SectionEnd

Function .onInit
  ; Check if OBS is running
  FindWindow $0 "" "OBS"
  StrCmp $0 0 notRunning
    MessageBox MB_OK|MB_ICONEXCLAMATION "OBS Studio is currently running. Please close it before installing this plugin."
    Abort
  notRunning:

  ; Check for OBS installation
  IfFileExists "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe" obsFound
  IfFileExists "$PROGRAMFILES (x86)\obs-studio\bin\64bit\obs64.exe" obsFound
    MessageBox MB_YESNO|MB_ICONQUESTION "OBS Studio does not appear to be installed.$\n$\nWould you like to continue anyway?" IDYES obsFound
    Abort
  obsFound:
FunctionEnd
