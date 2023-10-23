nsis = {}

require("eu_sci")
require("eu_core")

function nsis.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "nsis")      -- enable nsis lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line
  end
  return res
end

function nsis.get_styles()
  local style_t = {
    [SCE_NSIS_COMMENT] = 0xC0C0C0,
    [SCE_NSIS_COMMENTBOX] = 0xC0C0C0,
    -- 区段关键字加上粗体
    [SCE_NSIS_SECTIONDEF] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_NSIS_PAGEEX] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_NSIS_FUNCTIONDEF] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_NSIS_FUNCTION] = 0x00B050,
    [SCE_NSIS_SUBSECTIONDEF] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_NSIS_IFDEFINEDEF] = 0xBBBB00,
    [SCE_NSIS_MACRODEF] = 0xBBBB00,
    [SCE_NSIS_USERDEFINED] = 0xBBBB00,
    [SCE_NSIS_VARIABLE] = 0xC080FF,
    [SCE_NSIS_STRINGVAR] = 0xC080FF,
  }
  return style_t
end

function nsis.get_keywords()
  local keywords0_set = "Goto Page PageEx PageExEnd Return Section SectionEnd SectionGroup SectionGroupEnd UninstPage WriteINIStr BannerTrimPath ConfigRead ConfigReadS ConfigWrite ConfigWriteS DirState DriveSpace FileJoin FileReadFromEnd FileRecode GetBaseName GetDrives GetExeName GetExePath GetFileAttributes GetFileExt GetFileName GetFileVersion GetOptions GetOptionsS GetParameters GetParent GetRoot GetSize GetTime LineFind LineRead LineSum Locate RefreshShellIcons StrFilter StrFilterS TextCompare TextCompareS TrimNewLines VersionCompare VersionConvert WordAdd WordAddS WordFind WordFind2X WordFind2XS WordFind3X WordFind3XS WordFindS WordInsert WordInsertS WordReplace WordReplaceS onGUIEnd onGUIInit onInit onInstFailed onInstSuccess onMouseOverSection onRebootFailed onSelChange onUninstFailed onUninstSuccess onUserAbort onVerifyInstDir AddBrandingImage AllowRootDirInstall AllowSkipFiles AutoCloseWindow BGFont BGGradient BrandingText CRCCheck Caption ChangeUI CheckBitmap CompletedText ComponentText DetailsButtonText DirText DirVar DirVerify FileBufSize FileErrorText Icon InstProgressFlags InstallButtonText InstallColors InstallDir InstallDirRegKey LicenseBkColor LicenseData LicenseForceSelection LicenseText ManifestDPIAware ManifestLongPathAware ManifestSupportedOS MiscButtonText Name OutFile PEAddResource PERemoveResource RequestExecutionLevel SetCompress SetCompressor SetCompressorDictSize SetDatablockOptimize SetDateSave SetFont SetOverwrite ShowInstDetails ShowUninstDetails SilentInstall SilentUnInstall SpaceTexts SubCaption Unicode UninstallButtonText UninstallCaption UninstallIcon UninstallSubCaption UninstallText VIAddVersionKey VIFileVersion VIProductVersion WindowIcon XPStyle Abort AddSize BringToFront Call CallInstDLL ClearErrors CopyFiles CreateDirectory CreateFont CreateShortcut Delete DeleteINISec DeleteINIStr DeleteRegKey DeleteRegValue DetailPrint EnableWindow EnumRegKey EnumRegValue Exch Exec ExecShell ExecShellWait ExecWait ExpandEnvStrings File FileClose FileOpen FileRead FileReadByte FileReadUTF16LE FileReadWord FileSeek FileWrite FileWriteByte FileWriteUTF16LE FileWriteWord FindClose FindFirst FindNext FindWindow FlushINI GetCurInstType GetCurrentAddress GetDLLVersion GetDLLVersionLocal GetDlgItem GetErrorLevel GetFileTime GetFileTimeLocal GetFullPathName GetFunctionAddress GetInstDirError GetKnownFolderPath GetLabelAddress GetTempFileName GetWinVer HideWindow IfAbort IfErrors IfFileExists IfRebootFlag IfRtlLanguage IfShellVarContextAll IfSilent InitPluginsDir InstType InstTypeGetText InstTypeSetText Int64Cmp Int64CmpU Int64Fmt IntCmp IntCmpU IntFmt IntOp IntPtrCmp IntPtrCmpU IntPtrOp IsWindow LangString LicenseLangString LoadAndSetImage LoadLanguageFile LockWindow LogSet LogText MessageBox Nop PageCallbacks Pop Push Quit RMDir ReadEnvStr ReadINIStr ReadRegDWORD ReadRegStr Reboot RegDLL Rename ReserveFile SearchPath SectionGetFlags SectionGetInstTypes SectionGetSize SectionGetText SectionIn SectionInstType SectionSetFlags SectionSetInstTypes SectionSetSize SectionSetText SendMessage SetAutoClose SetBrandingImage SetCtlColors SetCurInstType SetDetailsPrint SetDetailsView SetErrorLevel SetErrors SetFileAttributes SetOutPath SetRebootFlag SetRegView SetShellVarContext SetSilent ShowWindow Sleep StrCmp StrCmpS StrCpy StrLen UnRegDLL WriteINIStr WriteRegBin WriteRegDWORD WriteRegExpandStr WriteRegMultiStr WriteRegNone WriteRegStr WriteUninstaller"
  local keywords1_set = "Var"
  local keywords2_set = nil
  local keywords3_set = "ADMINTOOLS APPDATA CDBURN_AREA COOKIES DESKTOP DOCUMENTS EXEDIR EXEFILE EXEPATH FAVORITES FONTS HISTORY HWNDPARENT INTERNET_CACHE LOCALAPPDATA MUSIC NETHOOD NSISDIR NSIS_CHAR_SIZE NSIS_PACKEDVERSION NSIS_PTR_SIZE NSIS_VERSION PICTURES PLUGINSDIR PRINTHOOD PROFILE PROGRAMFILES PROGRAMFILES32 PROGRAMFILES64 QUICKLAUNCH RECENT RESOURCES RESOURCES_LOCALIZED SENDTO SMPROGRAMS SMSTARTUP STARTMENU SYSDIR TEMP TEMPLATES VIDEOS WINDIR __COUNTER__ __DATE__ __FILEDIR__ __FILE__ __LINE__ __TIMESTAMP__ __TIME__ !include !insertmacro !addincludedir !addplugindir !appendfile !assert !cd !define !delfile !echo !error !execute !finalize !getdllversion !gettlbversion !makensis !packhdr !pragma !searchparse !searchreplace !system !tempfile !undef !uninstfinalize !verbose !warning"
  return keywords0_set,keywords1_set,keywords2_set,keywords3_set
end

function nsis.get_autocomplete()
  local autocomplete_set = "Function FunctionEnd Goto Page PageEx PageExEnd Return Section SectionEnd SectionGroup SectionGroupEnd UninstPage Var addincludedir addplugindir appendfile assert cd define delfile echo else endif error execute finalize getdllversion gettlbversion if ifdef ifmacrodef ifmacrondef ifndef include insertmacro macro macroend macroundef makensis packhdr pragma searchparse searchreplace system tempfile undef uninstfinalize verbose warning Abort AddSize BringToFront Call CallInstDLL ClearErrors CopyFiles CreateDirectory CreateFont CreateShortcut Delete DeleteINISec DeleteINIStr DeleteRegKey DeleteRegValue DetailPrint EnableWindow EnumRegKey EnumRegValue Exch Exec ExecShell ExecShellWait ExecWait ExpandEnvStrings File FileClose FileOpen FileRead FileReadByte FileReadUTF16LE FileReadWord FileSeek FileWrite FileWriteByte FileWriteUTF16LE FileWriteWord FindClose FindFirst FindNext FindWindow FlushINI GetCurInstType GetCurrentAddress GetDLLVersion GetDLLVersionLocal GetDlgItem GetErrorLevel GetFileTime GetFileTimeLocal GetFullPathName GetFunctionAddress GetInstDirError GetKnownFolderPath GetLabelAddress GetTempFileName GetWinVer HideWindow IfAbort IfErrors IfFileExists IfRebootFlag IfRtlLanguage IfShellVarContextAll IfSilent InitPluginsDir InstType InstTypeGetText InstTypeSetText Int64Cmp Int64CmpU Int64Fmt IntCmp IntCmpU IntFmt IntOp IntPtrCmp IntPtrCmpU IntPtrOp IsWindow LangString LicenseLangString LoadAndSetImage LoadLanguageFile LockWindow LogSet LogText MessageBox Nop PageCallbacks Pop Push Quit RMDir ReadEnvStr ReadINIStr ReadRegDWORD ReadRegStr Reboot RegDLL Rename ReserveFile SearchPath SectionGetFlags SectionGetInstTypes SectionGetSize SectionGetText SectionIn SectionInstType SectionSetFlags SectionSetInstTypes SectionSetSize SectionSetText SendMessage SetAutoClose SetBrandingImage SetCtlColors SetCurInstType SetDetailsPrint SetDetailsView SetErrorLevel SetErrors SetFileAttributes SetOutPath SetRebootFlag SetRegView SetShellVarContext SetSilent ShowWindow Sleep StrCmp StrCmpS StrCpy StrLen UnRegDLL WriteINIStr WriteRegBin WriteRegDWORD WriteRegExpandStr WriteRegMultiStr WriteRegNone WriteRegStr WriteUninstaller AddBrandingImage AllowRootDirInstall AllowSkipFiles AutoCloseWindow BGFont BGGradient BrandingText CRCCheck Caption ChangeUI CheckBitmap CompletedText ComponentText DetailsButtonText DirText DirVar DirVerify FileBufSize FileErrorText Icon InstProgressFlags InstallButtonText InstallColors InstallDir InstallDirRegKey LicenseBkColor LicenseData LicenseForceSelection LicenseText ManifestDPIAware ManifestLongPathAware ManifestSupportedOS MiscButtonText Name OutFile PEAddResource PERemoveResource RequestExecutionLevel SetCompress SetCompressor SetCompressorDictSize SetDatablockOptimize SetDateSave SetFont SetOverwrite ShowInstDetails ShowUninstDetails SilentInstall SilentUnInstall SpaceTexts SubCaption Unicode UninstallButtonText UninstallCaption UninstallIcon UninstallSubCaption UninstallText VIAddVersionKey VIFileVersion VIProductVersion WindowIcon XPStyle BannerTrimPath ConfigRead ConfigReadS ConfigWrite ConfigWriteS DirState DriveSpace FileJoin FileReadFromEnd FileRecode GetBaseName GetDrives GetExeName GetExePath GetFileAttributes GetFileExt GetFileName GetFileVersion GetOptions GetOptionsS GetParameters GetParent GetRoot GetSize GetTime LineFind LineRead LineSum Locate RefreshShellIcons StrFilter StrFilterS TextCompare TextCompareS TrimNewLines VersionCompare VersionConvert WordAdd WordAddS WordFind WordFind2X WordFind2XS WordFind3X WordFind3XS WordFindS WordInsert WordInsertS WordReplace WordReplaceS onGUIEnd onGUIInit onInit onInstFailed onInstSuccess onMouseOverSection onRebootFailed onSelChange onUninstFailed onUninstSuccess onUserAbort onVerifyInstDir ADMINTOOLS APPDATA CDBURN_AREA COOKIES DESKTOP DOCUMENTS EXEDIR EXEFILE EXEPATH FAVORITES FONTS HISTORY HWNDPARENT INTERNET_CACHE LOCALAPPDATA MUSIC NETHOOD NSISDIR NSIS_CHAR_SIZE NSIS_PACKEDVERSION NSIS_PTR_SIZE NSIS_VERSION PICTURES PLUGINSDIR PRINTHOOD PROFILE PROGRAMFILES PROGRAMFILES32 PROGRAMFILES64 QUICKLAUNCH RECENT RESOURCES RESOURCES_LOCALIZED SENDTO SMPROGRAMS SMSTARTUP STARTMENU SYSDIR TEMP TEMPLATES VIDEOS WINDIR __COUNTER__ __DATE__ __FILEDIR__ __FILE__ __LINE__ __TIMESTAMP__ __TIME__"
  return autocomplete_set
end

function nsis.get_reqular()
  local symbol_reqular_exp = "^function[ \\t]+([._a-zA-Z0-9]+)[\\s\\r\\n]*"
  return symbol_reqular_exp
end

function nsis.create_bakup(path)
  local nsis_code = {
    "user_nsis = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_nsis.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"nsis\")   -- enable nsis lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                           -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_nsis.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_NSIS_COMMENT] = 0xC0C0C0,\n",
    "    [SCE_NSIS_COMMENTBOX] = 0xC0C0C0,\n",
    "    -- 区段关键字加上粗体\n",
    "    [SCE_NSIS_SECTIONDEF] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_NSIS_PAGEEX] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_NSIS_FUNCTIONDEF] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_NSIS_FUNCTION] = 0x00B050,\n",
    "    [SCE_NSIS_SUBSECTIONDEF] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_NSIS_IFDEFINEDEF] = 0xBBBB00,\n",
    "    [SCE_NSIS_MACRODEF] = 0xBBBB00,\n",
    "    [SCE_NSIS_USERDEFINED] = 0xBBBB00,\n",
    "    [SCE_NSIS_VARIABLE] = 0xC080FF,\n",
    "    [SCE_NSIS_STRINGVAR] = 0xC080FF,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_nsis.get_keywords()\n",
    "  local keywords0_set = \"Goto Page PageEx PageExEnd Return Section SectionEnd SectionGroup SectionGroupEnd UninstPage WriteINIStr BannerTrimPath ConfigRead ConfigReadS ConfigWrite ConfigWriteS DirState DriveSpace FileJoin FileReadFromEnd FileRecode GetBaseName GetDrives GetExeName GetExePath GetFileAttributes GetFileExt GetFileName GetFileVersion GetOptions GetOptionsS GetParameters GetParent GetRoot GetSize GetTime LineFind LineRead LineSum Locate RefreshShellIcons StrFilter StrFilterS TextCompare TextCompareS TrimNewLines VersionCompare VersionConvert WordAdd WordAddS WordFind WordFind2X WordFind2XS WordFind3X WordFind3XS WordFindS WordInsert WordInsertS WordReplace WordReplaceS onGUIEnd onGUIInit onInit onInstFailed onInstSuccess onMouseOverSection onRebootFailed onSelChange onUninstFailed onUninstSuccess onUserAbort onVerifyInstDir AddBrandingImage AllowRootDirInstall AllowSkipFiles AutoCloseWindow BGFont BGGradient BrandingText CRCCheck Caption ChangeUI CheckBitmap CompletedText ComponentText DetailsButtonText DirText DirVar DirVerify FileBufSize FileErrorText Icon InstProgressFlags InstallButtonText InstallColors InstallDir InstallDirRegKey LicenseBkColor LicenseData LicenseForceSelection LicenseText ManifestDPIAware ManifestLongPathAware ManifestSupportedOS MiscButtonText Name OutFile PEAddResource PERemoveResource RequestExecutionLevel SetCompress SetCompressor SetCompressorDictSize SetDatablockOptimize SetDateSave SetFont SetOverwrite ShowInstDetails ShowUninstDetails SilentInstall SilentUnInstall SpaceTexts SubCaption Unicode UninstallButtonText UninstallCaption UninstallIcon UninstallSubCaption UninstallText VIAddVersionKey VIFileVersion VIProductVersion WindowIcon XPStyle Abort AddSize BringToFront Call CallInstDLL ClearErrors CopyFiles CreateDirectory CreateFont CreateShortcut Delete DeleteINISec DeleteINIStr DeleteRegKey DeleteRegValue DetailPrint EnableWindow EnumRegKey EnumRegValue Exch Exec ExecShell ExecShellWait ExecWait ExpandEnvStrings File FileClose FileOpen FileRead FileReadByte FileReadUTF16LE FileReadWord FileSeek FileWrite FileWriteByte FileWriteUTF16LE FileWriteWord FindClose FindFirst FindNext FindWindow FlushINI GetCurInstType GetCurrentAddress GetDLLVersion GetDLLVersionLocal GetDlgItem GetErrorLevel GetFileTime GetFileTimeLocal GetFullPathName GetFunctionAddress GetInstDirError GetKnownFolderPath GetLabelAddress GetTempFileName GetWinVer HideWindow IfAbort IfErrors IfFileExists IfRebootFlag IfRtlLanguage IfShellVarContextAll IfSilent InitPluginsDir InstType InstTypeGetText InstTypeSetText Int64Cmp Int64CmpU Int64Fmt IntCmp IntCmpU IntFmt IntOp IntPtrCmp IntPtrCmpU IntPtrOp IsWindow LangString LicenseLangString LoadAndSetImage LoadLanguageFile LockWindow LogSet LogText MessageBox Nop PageCallbacks Pop Push Quit RMDir ReadEnvStr ReadINIStr ReadRegDWORD ReadRegStr Reboot RegDLL Rename ReserveFile SearchPath SectionGetFlags SectionGetInstTypes SectionGetSize SectionGetText SectionIn SectionInstType SectionSetFlags SectionSetInstTypes SectionSetSize SectionSetText SendMessage SetAutoClose SetBrandingImage SetCtlColors SetCurInstType SetDetailsPrint SetDetailsView SetErrorLevel SetErrors SetFileAttributes SetOutPath SetRebootFlag SetRegView SetShellVarContext SetSilent ShowWindow Sleep StrCmp StrCmpS StrCpy StrLen UnRegDLL WriteINIStr WriteRegBin WriteRegDWORD WriteRegExpandStr WriteRegMultiStr WriteRegNone WriteRegStr WriteUninstaller\"\n",
    "  local keywords1_set = \"Var\"\n",
    "  local keywords2_set = nil\n",
    "  local keywords3_set = \"ADMINTOOLS APPDATA CDBURN_AREA COOKIES DESKTOP DOCUMENTS EXEDIR EXEFILE EXEPATH FAVORITES FONTS HISTORY HWNDPARENT INTERNET_CACHE LOCALAPPDATA MUSIC NETHOOD NSISDIR NSIS_CHAR_SIZE NSIS_PACKEDVERSION NSIS_PTR_SIZE NSIS_VERSION PICTURES PLUGINSDIR PRINTHOOD PROFILE PROGRAMFILES PROGRAMFILES32 PROGRAMFILES64 QUICKLAUNCH RECENT RESOURCES RESOURCES_LOCALIZED SENDTO SMPROGRAMS SMSTARTUP STARTMENU SYSDIR TEMP TEMPLATES VIDEOS WINDIR __COUNTER__ __DATE__ __FILEDIR__ __FILE__ __LINE__ __TIMESTAMP__ __TIME__ !include !insertmacro !addincludedir !addplugindir !appendfile !assert !cd !define !delfile !echo !error !execute !finalize !getdllversion !gettlbversion !makensis !packhdr !pragma !searchparse !searchreplace !system !tempfile !undef !uninstfinalize !verbose !warning\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set,keywords3_set\n",
    "end\n",
    "\n",
    "function user_nsis.get_autocomplete()\n",
    "  local autocomplete_set = \"Function FunctionEnd Goto Page PageEx PageExEnd Return Section SectionEnd SectionGroup SectionGroupEnd UninstPage Var addincludedir addplugindir appendfile assert cd define delfile echo else endif error execute finalize getdllversion gettlbversion if ifdef ifmacrodef ifmacrondef ifndef include insertmacro macro macroend macroundef makensis packhdr pragma searchparse searchreplace system tempfile undef uninstfinalize verbose warning Abort AddSize BringToFront Call CallInstDLL ClearErrors CopyFiles CreateDirectory CreateFont CreateShortcut Delete DeleteINISec DeleteINIStr DeleteRegKey DeleteRegValue DetailPrint EnableWindow EnumRegKey EnumRegValue Exch Exec ExecShell ExecShellWait ExecWait ExpandEnvStrings File FileClose FileOpen FileRead FileReadByte FileReadUTF16LE FileReadWord FileSeek FileWrite FileWriteByte FileWriteUTF16LE FileWriteWord FindClose FindFirst FindNext FindWindow FlushINI GetCurInstType GetCurrentAddress GetDLLVersion GetDLLVersionLocal GetDlgItem GetErrorLevel GetFileTime GetFileTimeLocal GetFullPathName GetFunctionAddress GetInstDirError GetKnownFolderPath GetLabelAddress GetTempFileName GetWinVer HideWindow IfAbort IfErrors IfFileExists IfRebootFlag IfRtlLanguage IfShellVarContextAll IfSilent InitPluginsDir InstType InstTypeGetText InstTypeSetText Int64Cmp Int64CmpU Int64Fmt IntCmp IntCmpU IntFmt IntOp IntPtrCmp IntPtrCmpU IntPtrOp IsWindow LangString LicenseLangString LoadAndSetImage LoadLanguageFile LockWindow LogSet LogText MessageBox Nop PageCallbacks Pop Push Quit RMDir ReadEnvStr ReadINIStr ReadRegDWORD ReadRegStr Reboot RegDLL Rename ReserveFile SearchPath SectionGetFlags SectionGetInstTypes SectionGetSize SectionGetText SectionIn SectionInstType SectionSetFlags SectionSetInstTypes SectionSetSize SectionSetText SendMessage SetAutoClose SetBrandingImage SetCtlColors SetCurInstType SetDetailsPrint SetDetailsView SetErrorLevel SetErrors SetFileAttributes SetOutPath SetRebootFlag SetRegView SetShellVarContext SetSilent ShowWindow Sleep StrCmp StrCmpS StrCpy StrLen UnRegDLL WriteINIStr WriteRegBin WriteRegDWORD WriteRegExpandStr WriteRegMultiStr WriteRegNone WriteRegStr WriteUninstaller AddBrandingImage AllowRootDirInstall AllowSkipFiles AutoCloseWindow BGFont BGGradient BrandingText CRCCheck Caption ChangeUI CheckBitmap CompletedText ComponentText DetailsButtonText DirText DirVar DirVerify FileBufSize FileErrorText Icon InstProgressFlags InstallButtonText InstallColors InstallDir InstallDirRegKey LicenseBkColor LicenseData LicenseForceSelection LicenseText ManifestDPIAware ManifestLongPathAware ManifestSupportedOS MiscButtonText Name OutFile PEAddResource PERemoveResource RequestExecutionLevel SetCompress SetCompressor SetCompressorDictSize SetDatablockOptimize SetDateSave SetFont SetOverwrite ShowInstDetails ShowUninstDetails SilentInstall SilentUnInstall SpaceTexts SubCaption Unicode UninstallButtonText UninstallCaption UninstallIcon UninstallSubCaption UninstallText VIAddVersionKey VIFileVersion VIProductVersion WindowIcon XPStyle BannerTrimPath ConfigRead ConfigReadS ConfigWrite ConfigWriteS DirState DriveSpace FileJoin FileReadFromEnd FileRecode GetBaseName GetDrives GetExeName GetExePath GetFileAttributes GetFileExt GetFileName GetFileVersion GetOptions GetOptionsS GetParameters GetParent GetRoot GetSize GetTime LineFind LineRead LineSum Locate RefreshShellIcons StrFilter StrFilterS TextCompare TextCompareS TrimNewLines VersionCompare VersionConvert WordAdd WordAddS WordFind WordFind2X WordFind2XS WordFind3X WordFind3XS WordFindS WordInsert WordInsertS WordReplace WordReplaceS onGUIEnd onGUIInit onInit onInstFailed onInstSuccess onMouseOverSection onRebootFailed onSelChange onUninstFailed onUninstSuccess onUserAbort onVerifyInstDir ADMINTOOLS APPDATA CDBURN_AREA COOKIES DESKTOP DOCUMENTS EXEDIR EXEFILE EXEPATH FAVORITES FONTS HISTORY HWNDPARENT INTERNET_CACHE LOCALAPPDATA MUSIC NETHOOD NSISDIR NSIS_CHAR_SIZE NSIS_PACKEDVERSION NSIS_PTR_SIZE NSIS_VERSION PICTURES PLUGINSDIR PRINTHOOD PROFILE PROGRAMFILES PROGRAMFILES32 PROGRAMFILES64 QUICKLAUNCH RECENT RESOURCES RESOURCES_LOCALIZED SENDTO SMPROGRAMS SMSTARTUP STARTMENU SYSDIR TEMP TEMPLATES VIDEOS WINDIR __COUNTER__ __DATE__ __FILEDIR__ __FILE__ __LINE__ __TIMESTAMP__ __TIME__\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_nsis.get_reqular()\n",
    "  local symbol_reqular_exp = \"^function[ \\\\t]+([._a-zA-Z0-9]+)[\\\\s\\\\r\\\\n]*\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_nsis",
  }
  local shell_code = table.concat(nsis_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  nsis_code = nil
end

return nsis