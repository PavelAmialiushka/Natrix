;NSIS Modern User Interface
;Start Menu Folder Selection Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "Library.nsh"
  !include "x64.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Natrix"

  OutFile "result/Natrix_Setup_${VERSION}.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES64\Natrix"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Natrix" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

  ; variant for no admin comp
  ; InstallDir "$LOCALAPPDATA\Natrix"
  ; RequestExecutionLevel highest



;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

#  !insertmacro MUI_PAGE_LICENSE "Docs\License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Natrix" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
  
  !insertmacro MUI_PAGE_INSTFILES

  #!define MUI_FINISHPAGE_LINK "Visit the NSIS site for the latest news, FAQs and support"
  #!define MUI_FINISHPAGE_LINK_LOCATION "http://nsis.sf.net/"

  !define MUI_FINISHPAGE_RUN "$INSTDIR\Natrix.exe"
  !define MUI_FINISHPAGE_NOREBOOTSUPPORT

  #!define MUI_FINISHPAGE_SHOWREADME
  #!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show release notes"
  #!define MUI_FINISHPAGE_SHOWREADME_FUNCTION ShowReleaseNotes

  !insertmacro MUI_PAGE_FINISH

  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "Russian"

;--------------------------------
;Installer Sections

Section "Natrix files" SecDummy

  SetOutPath "$INSTDIR"
  
  ;ADD YOUR OWN FILES HERE...
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Natrix" "" $INSTDIR
  
  WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "DisplayName" "Natrix"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "DisplayIcon" "$INSTDIR\Natrix.ico,0"

  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix" "NoRepair" "1"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  SetOverwrite on

  File /r "${BIN}\*"  
  File /oname=Natrix.ico "img\mainicon.ico" 

  CreateDirectory $APPDATA\Natrix

  ; Association

  ReadRegStr $R0 HKCR ".sktx" ""
  StrCmp $R0 "Natrix.Document" 0 +2
    DeleteRegKey HKCR "Natrix.Document"
  WriteRegStr HKCR ".sktx" "" "Natrix.Document"

  ReadRegStr $R0 HKCR ".skt" ""
  StrCmp $R0 "Natrix.Document" 0 +2
    DeleteRegKey HKCR "Natrix.Document"
  WriteRegStr HKCR ".skt" "" "Natrix.Document"

  WriteRegStr HKCR "Natrix.Document" "" "Natrix.Document"
  WriteRegStr HKCR "Natrix.Document\DefaultIcon" "" "$INSTDIR\Natrix.ico"
  
  ReadRegStr $R0 HKCR "Natrix.Document\shell\open\command" ""
  StrCmp $R0 "" 0 no_nsiopen
    WriteRegStr HKCR "Natrix.Document\shell" "" "open"
    WriteRegStr HKCR "Natrix.Document\shell\open\command" "" '"$INSTDIR\Natrix.exe" "%1"'
  no_nsiopen:

  ; Notify shell to change association
  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

  ; Microsoft.VC90.CRT
  ; ~~~~~~~~~~~~~~~~~~
  ;SetOutPath "$INSTDIR\Microsoft.VC90.CRT"
  ;File "ext\ms*.dll"
  ;File "ext\Microsoft.VC90.CRT.manifest"
  
  CreateShortCut "$DESKTOP\Natrix.lnk" "$INSTDIR\Natrix.exe" "" "$INSTDIR\Natrix.ico"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Natrix.lnk" "$INSTDIR\Natrix.exe" "" "$INSTDIR\Natrix.ico"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Natrix.ico"
  
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section "Visual Studio Runtime"
  SetOutPath "$INSTDIR"
  File /r "${BIN}\vc_redist.x64.exe"  
  ExecWait '"$INSTDIR\vc_redist.x64.exe" /install /quiet'
  Delete "$INSTDIR\vc_redist.x64.exe"
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecDummy ${LANG_ENGLISH} "Âסו פאיכû ןנמדנאללû."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDummy} $(DESC_SecDummy)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
  Delete $DESKTOP\Natrix.lnk
 
  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\*.ico

  RMDir /r $INSTDIR

  DeleteRegKey HKCR "Natrix.Document"
  DeleteRegKey HKCR ".sktx"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Natrix"

  Delete "$INSTDIR\Uninstall.exe"
 
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    
  Delete "$SMPROGRAMS\$StartMenuFolder\Natrix.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir  "$SMPROGRAMS\$StartMenuFolder"
   
  DeleteRegKey /ifempty HKCU "Software\Natrix"

SectionEnd