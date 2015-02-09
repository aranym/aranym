!ifndef REG_ROOT
  !error "REG_ROOT is not defined"
!endif
!ifndef REG_APP_PATH
  !error "REG_APP_PATH is not defined"
!endif

;Set the name of the uninstall log
!define UninstLog "uninstall.log"
Var UninstLog

!include "TextFunc.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh"

;Uninstall log file missing.
LangString UninstLogMissing ${LANG_ENGLISH} "${UninstLog} not found!$\r$\nUninstallation cannot proceed!"
 
Section -openlogfile
    CreateDirectory "$INSTDIR"
    ${IfNot} ${FileExists} "$INSTDIR\${UninstLog}"
      FileOpen $UninstLog "$INSTDIR\${UninstLog}" w
    ${Else}
      SetFileAttributes "$INSTDIR\${UninstLog}" NORMAL
      FileOpen $UninstLog "$INSTDIR\${UninstLog}" a
      FileSeek $UninstLog 0 END
    ${EndIf}
SectionEnd


;AddItem macro
  !macro AddItem type Path
    FileWrite $UninstLog "${type} ${Path}$\r$\n"
  !macroend
!define AddItem "!insertmacro AddItem"
 
;File macro
  !macro File FileName
     FileWrite $UninstLog "FIL $OUTDIR\${FileName}$\r$\n"
     File "/oname=${FileName}" "${FileName}"
  !macroend
!define File "!insertmacro File"
 
;CreateShortcut macro
  !macro CreateShortcut FilePath FilePointer Parameters Icon IconIndex Description
    Push $0
    ${GetParent} "${FilePath}" $0
    ${IfNot} ${FileExists} "$0\*"
      FileWrite $UninstLog "DIR $0$\r$\n"
    ${EndIf}
    FileWrite $UninstLog "FIL ${FilePath}$\r$\n"
    CreateShortcut "${FilePath}" "${FilePointer}" "${Parameters}" "${Icon}" "${IconIndex}" "" "" "${Description}"
    Pop $0
  !macroend
!define CreateShortcut "!insertmacro CreateShortcut"
 
;CreateDirectory macro
  !macro CreateDirectory Path
    CreateDirectory "${Path}"
    FileWrite $UninstLog "DIR ${Path}$\r$\n"
  !macroend
!define CreateDirectory "!insertmacro CreateDirectory"
 
;SetOutPath macro
  !macro SetOutPath Path
    SetOutPath "${Path}"
    FileWrite $UninstLog "DIR ${Path}$\r$\n"
  !macroend
!define SetOutPath "!insertmacro SetOutPath"
 
;WriteUninstaller macro
  !macro WriteUninstaller Path
    WriteUninstaller "${Path}"
    FileWrite $UninstLog "FIL ${Path}$\r$\n"
  !macroend
!define WriteUninstaller "!insertmacro WriteUninstaller"
 
;WriteRegStr macro
  !macro WriteRegStr RegRoot UnInstallPath Key Value
     FileWrite $UninstLog "KEY ${RegRoot} ${UnInstallPath}\${Key}$\r$\n"
     WriteRegStr "${RegRoot}" "${UnInstallPath}" "${Key}" "${Value}"
  !macroend
!define WriteRegStr "!insertmacro WriteRegStr"
 
 
;WriteRegExpandStr macro
  !macro WriteRegExpandStr RegRoot UnInstallPath Key Value
     FileWrite $UninstLog "KEY ${RegRoot} ${UnInstallPath}\${Key}$\r$\n"
     WriteRegExpandStr "${RegRoot}" "${UnInstallPath}" "${Key}" "${Value}"
  !macroend
!define WriteRegExpandStr "!insertmacro WriteRegExpandStr"
 
 
;WriteRegDWORD macro
  !macro WriteRegDWORD RegRoot UnInstallPath Key Value
     FileWrite $UninstLog "KEY ${RegRoot} ${UnInstallPath}\${Key}$\r$\n"
     WriteRegDWORD "${RegRoot}" "${UnInstallPath}" "${Key}" "${Value}"
  !macroend
!define WriteRegDWORD "!insertmacro WriteRegDWORD" 
 
Function un.InstallLogInit
  ClearErrors
  ReadRegStr $INSTDIR "${REG_ROOT}" "${REG_APP_PATH}" "InstallLocation"
  ${If} $INSTDIR == ""
    StrCpy $INSTDIR $EXEDIR
  ${EndIf}
  ;Can't uninstall if uninstall log is missing!
  ${IfNot} ${FileExists} "$INSTDIR\${UninstLog}"
    MessageBox MB_OK|MB_ICONSTOP "$(UninstLogMissing)"
    Abort
  ${EndIf}
FunctionEnd


Function un.InstallLog
  Push $R0
  Push $R1
  Push $R2
  Push $R3
  SetFileAttributes "$INSTDIR\${UninstLog}" NORMAL
  FileOpen $UninstLog "$INSTDIR\${UninstLog}" r
  StrCpy $R1 0
 
  GetLineCount:
    ClearErrors
    FileRead $UninstLog $R0
    IfErrors GetLineEnd
    IntOp $R1 $R1 + 1
    ${TrimNewLines} $R0 $R0
    ${WordFind} $R0 " " "+1{" $R2
    ${WordFind} $R0 " " "+1}" $R0
    Push $R0
    Push $R2
    Goto GetLineCount
  GetLineEnd:

  ${While} $R1 > 0
    Pop $R2
    Pop $R0
 
    ${Select} $R2
    ${Case} "DIR"
      RMDir $R0
    ${Case} "FIL"
      Delete $R0
    ${Case} "KEY"
      ${WordFind} $R0 " " "+1{" $R3
      ${WordFind} $R0 " " "+1}" $R0
      ${Select} $R3
      ${Case} "HKCR"
         DeleteRegKey HKCR $R0
      ${Case} "HKLM"
         DeleteRegKey HKLM $R0
      ${Case} "HKCU"
         DeleteRegKey HKCU $R0
      ${Case} "HKU"
         DeleteRegKey HKU $R0
      ${Case} "HKCC"
         DeleteRegKey HKCC $R0
      ${EndSelect}
    ${EndSelect}
    
    IntOp $R1 $R1 - 1
  ${EndWhile}
  
  FileClose $UninstLog
  Delete "$INSTDIR\${UninstLog}"
  RMDir "$INSTDIR"
  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0
 
  ;Remove registry keys
  DeleteRegKey ${REG_ROOT} "${REG_APP_PATH}"
  DeleteRegKey ${REG_ROOT} "${REG_UNINST_KEY}"
FunctionEnd
