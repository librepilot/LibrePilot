#
# Project: OpenPilot
# NSIS configuration file for OpenPilot GCS
# The OpenPilot Team, http://www.openpilot.org, Copyright (C) 2010-2015.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

# This script requires Unicode NSIS 2.46 or higher:
# http://www.scratchpaper.com/

# TODO:
#  - install only built/used modules, not a whole directory.
#  - remove only installed files, not a whole directory.

;--------------------------------
; Includes

!include "x64.nsh"
!include "..\..\build\openpilotgcs-synthetics\openpilotgcs.nsh"

;--------------------------------
; Paths

  ; Tree root locations (relative to this script location)
  !define PROJECT_ROOT   "..\.."
  !define NSIS_DATA_TREE "."
  !define GCS_BUILD_TREE "..\..\build\${GCS_SMALL_NAME}_release"
  !define UAVO_SYNTH_TREE "..\..\build\uavobject-synthetics"
  !define AEROSIMRC_TREE "${GCS_BUILD_TREE}\misc\AeroSIM-RC"

  ; Default installation folder
  InstallDir "$PROGRAMFILES\${ORG_BIG_NAME}"

  ; Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\${ORG_BIG_NAME}" "Install Location"

;--------------------------------
; Version information

  ; Program name and installer file
  !define PRODUCT_NAME "${GCS_BIG_NAME}"
  !define INSTALLER_NAME "${GCS_BIG_NAME} Installer"

  Name "${PRODUCT_NAME}"
  OutFile "${PACKAGE_DIR}\..\${OUT_FILE}"

  VIProductVersion ${PRODUCT_VERSION}
  VIAddVersionKey "ProductName" "${INSTALLER_NAME}"
  VIAddVersionKey "FileVersion" "${FILE_VERSION}"
  VIAddVersionKey "Comments" "${INSTALLER_NAME}. ${BUILD_DESCRIPTION}"
  VIAddVersionKey "CompanyName" "The LibrePilot Team, http://www.librepilot.org"
  VIAddVersionKey "LegalTrademarks" "${PRODUCT_NAME} is a trademark of The LibrePilot Team"
  VIAddVersionKey "LegalCopyright" "© 2015 The LibrePilot Team"
  VIAddVersionKey "FileDescription" "${INSTALLER_NAME}"

;--------------------------------
; Installer interface and base settings

  !include "MUI2.nsh"
  !define MUI_ABORTWARNING

  ; Adds an XP manifest to the installer
  XPStyle on

  ; Request application privileges for Windows Vista/7
  RequestExecutionLevel admin

  ; Compression level
  SetCompressor /solid lzma

;--------------------------------
; Branding

  BrandingText "© 2015 The LibrePilot Team, http://www.librepilot.org"

  !define MUI_ICON "${NSIS_DATA_TREE}\resources\installer_icon.ico"
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSIS_DATA_TREE}\resources\header.bmp"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${NSIS_DATA_TREE}\resources\welcome.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSIS_DATA_TREE}\resources\welcome.bmp"

  !define HEADER_BGCOLOR "0x8C8C8C"
  !define HEADER_FGCOLOR "0x343434"

  !macro SetHeaderColors un
    Function ${un}SetHeaderColors
      GetDlgItem $r3 $HWNDPARENT 1034
      SetCtlColors $r3 ${HEADER_BGCOLOR} ${HEADER_FGCOLOR}
      GetDlgItem $r3 $HWNDPARENT 1037
      SetCtlColors $r3 ${HEADER_BGCOLOR} ${HEADER_FGCOLOR}
      GetDlgItem $r3 $HWNDPARENT 1038
      SetCtlColors $r3 ${HEADER_BGCOLOR} ${HEADER_FGCOLOR}
    FunctionEnd
  !macroend

  !insertmacro SetHeaderColors ""
  !insertmacro SetHeaderColors "un."

;--------------------------------
; Language selection dialog settings

  ; Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\${ORG_BIG_NAME}" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"
  !define MUI_LANGDLL_ALWAYSSHOW

;--------------------------------
; Settings for MUI_PAGE_FINISH
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\WHATSNEW.txt"
  !define MUI_FINISHPAGE_RUN_FUNCTION "RunApplication"

;--------------------------------
; Pages

  !define MUI_PAGE_CUSTOMFUNCTION_PRE "SetHeaderColors"

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "$(LicenseFile)"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !define MUI_PAGE_CUSTOMFUNCTION_PRE "un.SetHeaderColors"

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Supported languages, license files and translations

  !include "${NSIS_DATA_TREE}\translations\languages.nsh"

;--------------------------------
; Reserve files

  ; If you are using solid compression, files that are required before
  ; the actual installation should be stored first in the data block,
  ; because this will make your installer start faster.

  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
; Installer sections

; Copy GCS core files
Section "Core files" InSecCore
  SectionIn RO
  SetOutPath "$INSTDIR\bin"
  File /r "${GCS_BUILD_TREE}\bin\*"
  SetOutPath "$INSTDIR"
  File "${PROJECT_ROOT}\LICENSE.txt"
  File "${PROJECT_ROOT}\README.txt"
  File "${PROJECT_ROOT}\WHATSNEW.txt"
  File "${PROJECT_ROOT}\MILESTONES.txt"
  File "${PROJECT_ROOT}\GPLv3.txt"
SectionEnd


; Copy GCS plugins
Section "-Plugins" InSecPlugins
  SectionIn RO
  SetOutPath "$INSTDIR\lib\${GCS_SMALL_NAME}\plugins"
  File /r "${GCS_BUILD_TREE}\lib\${GCS_SMALL_NAME}\plugins\*.dll"
  File /r "${GCS_BUILD_TREE}\lib\${GCS_SMALL_NAME}\plugins\*.pluginspec"
SectionEnd

; Copy OSG libs
Section "-OsgLibs" InSecOsgLibs
  SectionIn RO
  SetOutPath "$INSTDIR\lib\openpilotgcs\osg"
  File /r "${GCS_BUILD_TREE}\lib\openpilotgcs\osg\*.dll"
SectionEnd

; Copy GCS resources
Section "-Resources" InSecResources
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\cloudconfig"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\cloudconfig\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\default_configurations"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\default_configurations\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\backgrounds"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\backgrounds\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\diagrams"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\diagrams\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\dials"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\dials\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\mapicons"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\mapicons\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\models"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\models\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\qml"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\qml\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\osgearth"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\osgearth\*"
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\stylesheets"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\stylesheets\*"
SectionEnd

; Copy Notify plugin sound files
Section "-Sound files" InSecSounds
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\sounds"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\sounds\*"
SectionEnd

; Copy localization files
; Disabled until GCS source is stable and properly localized
Section "-Localization" InSecLocalization
  SetOutPath "$INSTDIR\share\${GCS_SMALL_NAME}\translations"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\translations\openpilotgcs_*.qm"
  File /r "${GCS_BUILD_TREE}\share\${GCS_SMALL_NAME}\translations\qt_*.qm"
SectionEnd

; Copy utility files
Section "-Utilities" InSecUtilities
  SetOutPath "$INSTDIR\utilities"
  File "/oname=OPLogConvert-${PACKAGE_LBL}.m" "${UAVO_SYNTH_TREE}\matlab\OPLogConvert.m"
SectionEnd

; Copy driver files
Section "-Drivers" InSecDrivers
  SetOutPath "$INSTDIR\drivers"
  File /r "${PROJECT_ROOT}\flight\Project\Windows USB\*"
SectionEnd

; Preinstall OpenPilot CDC driver
Section "CDC driver" InSecInstallDrivers
  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  ${If} ${RunningX64}
    File "/oname=dpinst.exe" "${NSIS_DATA_TREE}\redist\dpinst_x64.exe"
  ${Else}
    File "/oname=dpinst.exe" "${NSIS_DATA_TREE}\redist\dpinst_x86.exe"
  ${EndIf}
  ExecWait '"$PLUGINSDIR\dpinst.exe" /lm /path "$INSTDIR\drivers"'
SectionEnd

; Copy Opengl32.dll if needed (disabled by default)
Section /o "Mesa OpenGL driver" InSecInstallOpenGL
  SetOutPath "$INSTDIR\bin"
  File /r "${GCS_BUILD_TREE}\bin\opengl32_32\opengl32.dll"
SectionEnd

; AeroSimRC plugin files
Section "AeroSimRC plugin" InSecAeroSimRC
  SetOutPath "$INSTDIR\misc\AeroSIM-RC"
  File /r "${AEROSIMRC_TREE}\*"
SectionEnd

Section "Shortcuts" InSecShortcuts
  ; Create desktop and start menu shortcuts
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\${ORG_BIG_NAME}"
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\${GCS_BIG_NAME}.lnk" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0 "" "" "${PRODUCT_NAME} ${PRODUCT_VERSION}. ${BUILD_DESCRIPTION}"
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\${GCS_BIG_NAME} (clean configuration).lnk" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" \
	"-reset" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0 "" "" "${PRODUCT_NAME} ${PRODUCT_VERSION}. ${BUILD_DESCRIPTION}"
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot License.lnk" "$INSTDIR\LICENSE.txt" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot ReadMe.lnk" "$INSTDIR\README.txt" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot ReleaseNotes.lnk" "$INSTDIR\WHATSNEW.txt" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot Milestones.lnk" "$INSTDIR\MILESTONES.txt" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot Website.lnk" "http://www.librepilot.org" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot Wiki.lnk" "http://wiki.openpilot.org" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\OpenPilot Forums.lnk" "http://forums.librepilot.org" \
	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0
  CreateShortCut "$DESKTOP\${GCS_BIG_NAME}.lnk" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" \
  	"" "$INSTDIR\bin\${GCS_SMALL_NAME}.exe" 0 "" "" "${PRODUCT_NAME} ${PRODUCT_VERSION}. ${BUILD_DESCRIPTION}"
  CreateShortCut "$SMPROGRAMS\${ORG_BIG_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
SectionEnd

Section ; create uninstall info
  ; Write the installation path into the registry
  WriteRegStr HKCU "Software\OpenPilot" "Install Location" $INSTDIR

   ; Write the uninstall keys for Windows
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "DisplayName" "${GCS_BIG_NAME}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "DisplayIcon" '"$INSTDIR\bin\${GCS_SMALL_NAME}.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "Publisher" "LibrePilot Team"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "URLInfoAbout" "http://www.librepilot.org"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "HelpLink" "http://wiki.openpilot.org"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "EstimatedSize" 100600
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}" "NoRepair" 1

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

;--------------------------------
; Installer section descriptions

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecCore} $(DESC_InSecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecPlugins} $(DESC_InSecPlugins)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecResources} $(DESC_InSecResources)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecSounds} $(DESC_InSecSounds)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecLocalization} $(DESC_InSecLocalization)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecUtilities} $(DESC_InSecUtilities)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecDrivers} $(DESC_InSecDrivers)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecInstallDrivers} $(DESC_InSecInstallDrivers)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecInstallOpenGL} $(DESC_InSecInstallOpenGL)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecAeroSimRC} $(DESC_InSecAeroSimRC)
    !insertmacro MUI_DESCRIPTION_TEXT ${InSecShortcuts} $(DESC_InSecShortcuts)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Installer functions

Function .onInit

  SetShellVarContext all
  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

;--------------------------------
; Uninstaller sections

Section "un.${GCS_BIG_NAME}" UnSecProgram
  ; Remove installed files and/or directories
  RMDir /r /rebootok "$INSTDIR\bin"
  RMDir /r /rebootok "$INSTDIR\lib"
  RMDir /r /rebootok "$INSTDIR\share"
  RMDir /r /rebootok "$INSTDIR\firmware"
  RMDir /r /rebootok "$INSTDIR\utilities"
  RMDir /r /rebootok "$INSTDIR\drivers"
  RMDir /r /rebootok "$INSTDIR\misc"
  Delete /rebootok "$INSTDIR\*.txt"
  Delete /rebootok "$INSTDIR\Uninstall.exe"

  ; Remove directory
  RMDir /rebootok "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ORG_BIG_NAME}"
  DeleteRegKey HKCU "Software\${ORG_BIG_NAME}"

  ; Remove shortcuts, if any
  SetShellVarContext all
  Delete /rebootok "$DESKTOP\${GCS_BIG_NAME}.lnk"
  Delete /rebootok "$SMPROGRAMS\${ORG_BIG_NAME}\*"
  RMDir /rebootok "$SMPROGRAMS\${ORG_BIG_NAME}"
SectionEnd

Section "un.Maps cache" UnSecCache
  ; Remove maps cache
  SetShellVarContext current
<<<<<<< HEAD
  RMDir /r /rebootok "$APPDATA\OpenPilot\mapscache"
  RMDir /r /rebootok "$APPDATA\OpenPilot\osgearth"
=======
  RMDir /r /rebootok "$LOCALAPPDATA\OpenPilot\${GCS_BIG_NAME}"
  ; Only remove if no other versions have data here
  RMDir /rebootok "$LOCALAPPDATA\OpenPilot"
>>>>>>> origin/next
SectionEnd

Section "un.GCS Layout" UnSecConfig
  ; Remove GCS configuration files
  SetShellVarContext current
  Delete /rebootok "$APPDATA\OpenPilot\${GCS_BIG_NAME}.db"
  Delete /rebootok "$APPDATA\OpenPilot\${GCS_BIG_NAME}.xml"
SectionEnd

Section "-un.Profile" UnSecProfile
  ; Remove OpenPilot user profile subdirectory if empty
  SetShellVarContext current
  RMDir /rebootok "$APPDATA\OpenPilot"
SectionEnd

;--------------------------------
; Uninstall section descriptions

  !insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${UnSecProgram} $(DESC_UnSecProgram)
    !insertmacro MUI_DESCRIPTION_TEXT ${UnSecCache} $(DESC_UnSecCache)
    !insertmacro MUI_DESCRIPTION_TEXT ${UnSecConfig} $(DESC_UnSecConfig)
  !insertmacro MUI_UNFUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller functions

Function un.onInit

  SetShellVarContext all
  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd

;--------------------------------
; Function to run the application from installer

Function RunApplication

  Exec '"$INSTDIR\bin\${GCS_SMALL_NAME}.exe"'

FunctionEnd
