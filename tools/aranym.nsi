; ARAnyM Setup Script for NSIS
;
!searchreplace SCRIPTDIR "${__FILE__}" "aranym.nsi" ""
!addincludedir "${SCRIPTDIR}"
!addincludedir ".."

; !define VER_MAJOR 1
; !define VER_MINOR 0
; !define VER_MICRO 2
!ifndef VER_MAJOR
  !error "VER_MAJOR is not defined"
!endif
!define VER_BUILD 0
!define VERSION ${VER_MAJOR}.${VER_MINOR}.${VER_MICRO}

!define NAME "ARAnyM"
!define NAMESUFFIX ""

;--------------------------------
;Configuration

; The name of the installer
Name "${NAME}"
Caption "${NAME} ${VERSION}${NAMESUFFIX} Setup"

!ifndef OUTFILE
  !define OUTFILE "..\aranym-${VERSION}${SDL}-setup.exe"
!endif
OutFile "${OUTFILE}"
SetCompressor /SOLID lzma

InstType "Full"
InstType "Lite"
InstType "Minimal"

;The root registry to write to
!define REG_ROOT "HKLM"
;The registry path to write to
!define REG_APP_PATH "SOFTWARE\${NAME}"
!define REG_UNINST_KEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"

; The default installation directory
InstallDir $PROGRAMFILES\${NAME}
InstallDirRegKey "${REG_ROOT}" "${REG_APP_PATH}" "InstallLocation"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Header Files

!include "MUI2.nsh"
!include "Sections.nsh"
!include "UninstallLog.nsh"
!include "TextFunc.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "Memento.nsh"
!include "WordFunc.nsh"

;--------------------------------
;Version information

VIProductVersion ${VER_MAJOR}.${VER_MINOR}.${VER_MICRO}.${VER_BUILD}
VIAddVersionKey "ProductName" "${NAME}"
VIAddVersionKey "Comments" "Atari Running on Any Machine"
VIAddVersionKey "ProductVersion" "${VER_MAJOR}.${VER_MINOR}.${VER_MICRO}.${VER_BUILD}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "${NAME} Setup"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2000-2015 ${NAME} developer team"

;--------------------------------
;Configuration

;Memento Settings
!define MEMENTO_REGISTRY_ROOT HKLM
!define MEMENTO_REGISTRY_KEY "${REG_UNINST_KEY}"

;Interface Settings
!define MUI_ABORTWARNING

!define MUI_HEADERIMAGE
!define MUI_WELCOMEFINISHPAGE_BITMAP "aranym\wm_icon.bmp"

!define MUI_COMPONENTSPAGE_SMALLDESC

;--------------------------------
;Pages

!define MUI_WELCOMEPAGE_TITLE "Welcome to the ${NAME} ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${NAME} (Atari Running on Any Machine) ${VERSION}, a software virtual machine (similar to VirtualBox or Bochs) designed and developed for running 32-bit Atari ST/TT/Falcon operating systems (TOS, FreeMiNT, MagiC and Linux-m68k) and TOS/GEM applications on any kind of hardware.$\r$\n$\r$\n$_CLICK"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "COPYING.txt"
!ifdef VER_MAJOR
Page custom PageReinstall PageLeaveReinstall
!endif
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Function ".onInit"
  ${If} ${Silent}
    SetAutoClose true
  ${EndIf}
  ${MementoSectionRestore}
FunctionEnd

Function ".onInstSuccess"
  ${MementoSectionSave}
FunctionEnd

!ifdef VER_MAJOR

Var ReinstallPageCheck

Function PageReinstall

  ReadRegStr $R0 "${REG_ROOT}" "${REG_APP_PATH}" "InstallLocation"
  ReadRegStr $R1 "${REG_ROOT}" "${REG_UNINST_KEY}" "UninstallString"
  ${IfThen} "$R0$R1" == "" ${|} Abort ${|}

  StrCpy $R4 "older"
  ReadRegDWORD $R0 "${REG_ROOT}" "${REG_APP_PATH}" "VersionMajor"
  ReadRegDWORD $R1 "${REG_ROOT}" "${REG_APP_PATH}" "VersionMinor"
  ReadRegDWORD $R2 "${REG_ROOT}" "${REG_APP_PATH}" "VersionRevision"
  ReadRegDWORD $R3 "${REG_ROOT}" "${REG_APP_PATH}" "VersionBuild"
  ${IfThen} $R0 = 0 ${|} StrCpy $R4 "unknown" ${|} ; Anonymous builds have no version number
  StrCpy $R0 $R0.$R1.$R2.$R3

  ${VersionCompare} ${VER_MAJOR}.${VER_MINOR}.${VER_MICRO}.${VER_BUILD} $R0 $R0
  ${If} $R0 == 0
    StrCpy $R1 "${NAME} ${VERSION} is already installed. Select the operation you want to perform and click Next to continue."
    StrCpy $R2 "Add/Reinstall components"
    StrCpy $R3 "Uninstall ${NAME}"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose the maintenance option to perform."
    StrCpy $R0 "2"
  ${ElseIf} $R0 == 1
    StrCpy $R1 "An $R4 version of ${NAME} is installed on your system. It's recommended that you uninstall the current version before installing. Select the operation you want to perform and click Next to continue."
    StrCpy $R2 "Uninstall before installing"
    StrCpy $R3 "Do not uninstall"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose how you want to install ${NAME}."
    StrCpy $R0 "1"
  ${ElseIf} $R0 == 2
    StrCpy $R1 "A newer version of ${NAME} is already installed! It is not recommended that you install an older version. If you really want to install this older version, it's better to uninstall the current version first. Select the operation you want to perform and click Next to continue."
    StrCpy $R2 "Uninstall before installing"
    StrCpy $R3 "Do not uninstall"
    !insertmacro MUI_HEADER_TEXT "Already Installed" "Choose how you want to install ${NAME}."
    StrCpy $R0 "1"
  ${Else}
    Abort
  ${EndIf}

  nsDialogs::Create 1018
  Pop $R4

  ${NSD_CreateLabel} 0 0 100% 24u $R1
  Pop $R1

  ${NSD_CreateRadioButton} 30u 50u -30u 8u $R2
  Pop $R2
  ${NSD_OnClick} $R2 PageReinstallUpdateSelection

  ${NSD_CreateRadioButton} 30u 70u -30u 8u $R3
  Pop $R3
  ${NSD_OnClick} $R3 PageReinstallUpdateSelection

  ${If} $ReinstallPageCheck != 2
    SendMessage $R2 ${BM_SETCHECK} ${BST_CHECKED} 0
  ${Else}
    SendMessage $R3 ${BM_SETCHECK} ${BST_CHECKED} 0
  ${EndIf}

  ${NSD_SetFocus} $R2

  nsDialogs::Show

FunctionEnd

Function PageReinstallUpdateSelection

  Pop $R1

  ${NSD_GetState} $R2 $R1

  ${If} $R1 == ${BST_CHECKED}
    StrCpy $ReinstallPageCheck 1
  ${Else}
    StrCpy $ReinstallPageCheck 2
  ${EndIf}

FunctionEnd

Function PageLeaveReinstall

  ${NSD_GetState} $R2 $R1

  StrCmp $R0 "1" 0 +2 ; Existing install is not the same version?
    StrCmp $R1 "1" reinst_uninstall reinst_done

  StrCmp $R1 "1" reinst_done ; Same version, skip to add/reinstall components?

  reinst_uninstall:
  ReadRegStr $R1 "${REG_ROOT}" "${REG_UNINST_KEY}" "UninstallString"

  ;Run uninstaller
    HideWindow

    ClearErrors
    ExecWait '$R1 _?=$INSTDIR' $0

    BringToFront

    ${IfThen} ${Errors} ${|} StrCpy $0 2 ${|} ; ExecWait failed, set fake exit code

    ${If} $0 <> 0
    ${OrIf} ${FileExists} "$INSTDIR\aranym.exe"
      ${If} $0 = 1 ; User aborted uninstaller?
        StrCmp $R0 "2" 0 +2 ; Is the existing install the same version?
          Quit ; ...yes, already installed, we are done
        Abort
      ${EndIf}
      MessageBox MB_ICONEXCLAMATION "Unable to uninstall!"
      Abort
    ${Else}
      StrCpy $0 $R1 1
      ${IfThen} $0 == '"' ${|} StrCpy $R1 $R1 -1 1 ${|} ; Strip quotes from UninstallString
      Delete $R1
      RMDir $INSTDIR
    ${EndIf}

  reinst_done:

FunctionEnd

!endif # VER_MAJOR

; The stuff to install
${MementoSection} "${NAME} Core Files (required)" SecCore
  SectionIn 1 2
  ${SetOutPath} $INSTDIR
  
  !include "aranym.files"

${MementoSectionEnd}

!ifndef NO_STARTMENUSHORTCUTS
!define STARTMENU 
${MementoSection} "Start Menu Shortcuts" SecMenuShortcuts
  DetailPrint "Installing Start Menu Shortcuts..."
  SectionIn 1 2
  StrCpy $0 "$SMPROGRAMS\${NAME}"
  ${CreateDirectory} "$0"
  ${CreateShortcut} "$0\${NAME}${NAMESUFFIX}.lnk" "$INSTDIR\aranym.exe" "" "" "" "${NAME}"
  ${CreateShortcut} "$0\${NAME}${NAMESUFFIX} - MMU.lnk" "$INSTDIR\aranym-mmu.exe" "" "" "" "${NAME} MMU"
  ${CreateShortcut} "$0\${NAME}${NAMESUFFIX} - JIT.lnk" "$INSTDIR\aranym-jit.exe" "" "" "" "${NAME} JIT"
${MementoSectionEnd}
!endif

!ifndef NO_DESKTOPSHORTCUTS
${MementoSection} "Desktop Shortcuts" SecDesktopShortcuts
  DetailPrint "Installing Desktop Shortcuts..."
  SectionIn 1 2
  ${CreateShortcut} "$DESKTOP\${NAME}${NAMESUFFIX}.lnk" "$INSTDIR\aranym.exe" "" "" "" "${NAME}"
${MementoSectionEnd}
!endif

${MementoSectionDone}

Section -post
  ;Write the installation path into the registry
  ${WriteRegExpandStr} "${REG_ROOT}" "${REG_APP_PATH}" "InstallLocation" "$INSTDIR"
!ifdef VER_MAJOR
  ${WriteRegStr} "${REG_ROOT}" "${REG_APP_PATH}" "VersionMajor" "${VER_MAJOR}"
  ${WriteRegStr} "${REG_ROOT}" "${REG_APP_PATH}" "VersionMinor" "${VER_MINOR}"
  ${WriteRegStr} "${REG_ROOT}" "${REG_APP_PATH}" "VersionRevision" "${VER_MICRO}"
  ${WriteRegStr} "${REG_ROOT}" "${REG_APP_PATH}" "VersionBuild" "${VER_BUILD}"
!endif
  ;Write the Uninstall information into the registry
  ${WriteRegExpandStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\uninstall-aranym.exe$\""
  ${WriteRegExpandStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall-aranym.exe$\" /S"
  ${WriteRegExpandStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "DisplayName" "${NAME}${NAMESUFFIX}"
  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "DisplayIcon" "$INSTDIR\aranym.exe,0"
  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "DisplayVersion" "${VERSION}"
  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "Publisher" "ARAnyM developer team"

  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "URLInfoAbout" "http://aranym.sourceforge.net/"
  ${WriteRegStr} "${REG_ROOT}" "${REG_UNINST_KEY}" "HelpLink" "http://aranym.sourceforge.net"
  ${WriteRegDWORD} "${REG_ROOT}" "${REG_UNINST_KEY}" "NoModify" "1"
  ${WriteRegDWORD} "${REG_ROOT}" "${REG_UNINST_KEY}" "NoRepair" "1"

  ${WriteUninstaller} "$INSTDIR\uninstall-aranym.exe"

SectionEnd


;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The core files required to use ${NAME} (compiler etc.)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMenuShortcuts} "Adds icons to your start menu for easy access"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopShortcuts} "Adds icons to your desktop for easy access"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller
;--------------------------------

Function "un.onInit"
  ${If} ${Silent}
    SetAutoClose true
  ${EndIf}
  Call un.InstallLogInit
FunctionEnd

Section "Uninstall"
  ${IfNot} ${FileExists} "$INSTDIR\aranym.exe"
    ${Unless} ${Cmd} `MessageBox MB_YESNO "It does not appear that ${NAME} is installed in the directory '$INSTDIR'.$\r$\nContinue anyway (not recommended)?" IDYES`
      Abort "Uninstall aborted by user"
    ${EndUnless}
  ${EndIf}

  Call un.InstallLog
SectionEnd
