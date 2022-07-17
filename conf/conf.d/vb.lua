vb = {}

require("eu_sci")
require("eu_core")

function vb.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "vb")  -- enable vb lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                      -- enable fold line
  end
  return res
end

function vb.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "' "
  local block_t = "' "
  return line_t, block_t
end

function vb.get_styles()
  local style_t = {
    [SCE_B_COMMENT] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_B_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_B_KEYWORD2] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_B_KEYWORD3] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_B_STRING] = 0xC080FF,
  }
  return style_t
end

function vb.get_keywords()
  local keywords0_set = "addhandler addressof alias and andalso as byref byval call case catch class const continue declare default delegate dim directcast do each else elseif end enum erase error event exit false finally for friend function get gettype getxmlnamespace global goto handles if implements imports in inherits interface is isnot let lib like loop me mod module mustinherit mustoverride my mybase myclass namespace narrowing new next not nothing notinheritable notoverridable of on operator option optional or orelse out overloads overridable overrides paramarray partial private property protected public raiseevent readonly redim rem removehandler resume return select set shadows shared static step stop structure sub synclock then throw to true try trycast typeof using when while widening with withevents writeonly xor vbcr vbcrlf vbformfeed vblf vbnewline vbnullchar vbnullstring vbtab vbverticaltab vbok vbcancel vbabort vbretry vbignore vbyes vbno vbokonly vbokcancel vbabortretryignore vbyesnocancel vbyesno vbretrycancel vbcritical vbquestion vbexclamation vbinformation vbdefaultbutton1 vbdefaultbutton2 vbdefaultbutton3 vbapplicationmodal vbsystemmodal vbmsgboxhelp vbmsgboxright vbmsgboxrtlreading vbmsgboxsetforeground vbbinarycompare vbtextcompare vbsunday vbmonday vbtuesday vbwednesday vbthursday vbfriday vbsaturday vbusesystemdayofweek vbfirstjan1 vbfirstfourdays vbfirstfullweek vbgeneraldate vblongdate vbshortdate vblongtime vbshorttime vbusedefault vbtrue vbfalse vbempty vbnull vbinteger vblong vbsingle vbdouble vbcurrency vbdate vbstring vbobject vbboolean vbvariant vbdecimal vbbyte vbarray vbdirectory vbobjecterror vbarchive vbback vbget vbhidden vbhide vblet vblinguisticcasing vblowercase vbmaximizedfocus vbmethod vbminimizedfocus vbminimizednofocus vbnarrow vbnormal vbnormalfocus vbnormalnofocus vbpropercase vbreadonly vbset vbsystem vbuppercase vbuserdefinedtype vbusesystem vbvolume vbwide vbhiragana vbkatakana vbsimplifiedchinese vbtraditionalchinese"
  local keywords1_set = "aggregate ansi assembly async attribute auto await binary by compare custom distinct equals explicit from group into isfalse istrue join key mid off order preserve skip strict take text unicode until where yield array any count groupby orderby begin beginproperty endproperty type"
  local keywords2_set = "boolean byte cbool cbyte cchar cdate cdbl cdec char cint clng cobj csbyte cshort csng cstr ctype cuint culng cushort date decimal double integer long object sbyte short single string uinteger ulong ushort if else elseif end const region externalchecksum externalsource comclass hidemodulename webmethod serializable marshalas attributeusage dllimport structlayout vbfixedstring vbfixedarray"
  return keywords0_set,keywords1_set,keywords2_set
end

function vb.get_autocomplete()
  local autocomplete_set = "AddHandler AddressOf Alias And AndAlso As ByRef ByVal Call Case Catch Class Const Continue Declare Default Delegate Dim DirectCast Do Each Else ElseIf End Enum Erase Error Event Exit False Finally For Friend Function Get GetType GetXMLNamespace Global Goto Handles If Implements Imports In Inherits Interface Is IsNot Let Lib Like Loop Me Mod Module MustInherit MustOverride My MyBase MyClass Namespace Narrowing New Next Not Nothing NotInheritable NotOverridable Of On Operator Option Optional Or OrElse Out Overloads Overridable Overrides ParamArray Partial Private Property Protected Public RaiseEvent Readonly ReDim Rem RemoveHandler Resume Return Select Set Shadows Shared Static Step Stop Structure Sub SyncLock Then Throw To True Try TryCast TypeOf Using When While Widening With WithEvents WriteOnly Xor vbCr VbCrLf vbFormFeed vbLf vbNewLine vbNullChar vbNullString vbTab vbVerticalTab vbOK vbCancel vbAbort vbRetry vbIgnore vbYes vbNo vbOKOnly vbOKCancel vbAbortRetryIgnore vbYesNoCancel vbYesNo vbRetryCancel vbCritical vbQuestion vbExclamation vbInformation vbDefaultButton1 vbDefaultButton2 vbDefaultButton3 vbApplicationModal vbSystemModal vbMsgBoxHelp vbMsgBoxRight vbMsgBoxRtlReading vbMsgBoxSetForeground vbBinaryCompare vbTextCompare vbSunday vbMonday vbTuesday vbWednesday vbThursday vbFriday vbSaturday vbUseSystemDayOfWeek vbFirstJan1 vbFirstFourDays vbFirstFullWeek vbGeneralDate vbLongDate vbShortDate vbLongTime vbShortTime vbUseDefault vbTrue vbFalse vbEmpty vbNull vbInteger vbLong vbSingle vbDouble vbCurrency vbDate vbString vbObject vbBoolean vbVariant vbDecimal vbByte vbArray vbDirectory vbObjectError vbArchive vbBack vbGet vbHidden vbHide vbLet vbLinguisticCasing vbLowerCase vbMaximizedFocus vbMethod vbMinimizedFocus vbMinimizedNoFocus vbNarrow vbNormal vbNormalFocus vbNormalNoFocus vbProperCase vbReadOnly vbSet vbSystem vbUpperCase vbUserDefinedType vbUseSystem vbVolume vbWide vbHiragana vbKatakana vbSimplifiedChinese vbTraditionalChinese"
  return autocomplete_set
end

function vb.get_reqular()
  local symbol_reqular_exp = "[ \\t]*Private Sub[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
  return symbol_reqular_exp
end

function vb.create_bakup(path)
  local vb_code = {
    "user_vb = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_vb.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"vb\")  -- enable vb lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                      -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_vb.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"' \"\n",
    "  local block_t = \"' \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_vb.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_B_COMMENT] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_B_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_B_KEYWORD2] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_B_KEYWORD3] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_B_STRING] = 0xC080FF\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_vb.get_keywords()\n",
    "  local keywords0_set = \"addhandler addressof alias and andalso as byref byval call case catch class const continue declare default delegate dim directcast do each else elseif end enum erase error event exit false finally for friend function get gettype getxmlnamespace global goto handles if implements imports in inherits interface is isnot let lib like loop me mod module mustinherit mustoverride my mybase myclass namespace narrowing new next not nothing notinheritable notoverridable of on operator option optional or orelse out overloads overridable overrides paramarray partial private property protected public raiseevent readonly redim rem removehandler resume return select set shadows shared static step stop structure sub synclock then throw to true try trycast typeof using when while widening with withevents writeonly xor vbcr vbcrlf vbformfeed vblf vbnewline vbnullchar vbnullstring vbtab vbverticaltab vbok vbcancel vbabort vbretry vbignore vbyes vbno vbokonly vbokcancel vbabortretryignore vbyesnocancel vbyesno vbretrycancel vbcritical vbquestion vbexclamation vbinformation vbdefaultbutton1 vbdefaultbutton2 vbdefaultbutton3 vbapplicationmodal vbsystemmodal vbmsgboxhelp vbmsgboxright vbmsgboxrtlreading vbmsgboxsetforeground vbbinarycompare vbtextcompare vbsunday vbmonday vbtuesday vbwednesday vbthursday vbfriday vbsaturday vbusesystemdayofweek vbfirstjan1 vbfirstfourdays vbfirstfullweek vbgeneraldate vblongdate vbshortdate vblongtime vbshorttime vbusedefault vbtrue vbfalse vbempty vbnull vbinteger vblong vbsingle vbdouble vbcurrency vbdate vbstring vbobject vbboolean vbvariant vbdecimal vbbyte vbarray vbdirectory vbobjecterror vbarchive vbback vbget vbhidden vbhide vblet vblinguisticcasing vblowercase vbmaximizedfocus vbmethod vbminimizedfocus vbminimizednofocus vbnarrow vbnormal vbnormalfocus vbnormalnofocus vbpropercase vbreadonly vbset vbsystem vbuppercase vbuserdefinedtype vbusesystem vbvolume vbwide vbhiragana vbkatakana vbsimplifiedchinese vbtraditionalchinese\"\n",
    "  local keywords1_set = \"aggregate ansi assembly async attribute auto await binary by compare custom distinct equals explicit from group into isfalse istrue join key mid off order preserve skip strict take text unicode until where yield array any count groupby orderby begin beginproperty endproperty type\"\n",
    "  local keywords2_set = \"boolean byte cbool cbyte cchar cdate cdbl cdec char cint clng cobj csbyte cshort csng cstr ctype cuint culng cushort date decimal double integer long object sbyte short single string uinteger ulong ushort if else elseif end const region externalchecksum externalsource comclass hidemodulename webmethod serializable marshalas attributeusage dllimport structlayout vbfixedstring vbfixedarray\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_vb.get_autocomplete()\n",
    "  local autocomplete_set = \"AddHandler AddressOf Alias And AndAlso As ByRef ByVal Call Case Catch Class Const Continue Declare Default Delegate Dim DirectCast Do Each Else ElseIf End Enum Erase Error Event Exit False Finally For Friend Function Get GetType GetXMLNamespace Global Goto Handles If Implements Imports In Inherits Interface Is IsNot Let Lib Like Loop Me Mod Module MustInherit MustOverride My MyBase MyClass Namespace Narrowing New Next Not Nothing NotInheritable NotOverridable Of On Operator Option Optional Or OrElse Out Overloads Overridable Overrides ParamArray Partial Private Property Protected Public RaiseEvent Readonly ReDim Rem RemoveHandler Resume Return Select Set Shadows Shared Static Step Stop Structure Sub SyncLock Then Throw To True Try TryCast TypeOf Using When While Widening With WithEvents WriteOnly Xor vbCr VbCrLf vbFormFeed vbLf vbNewLine vbNullChar vbNullString vbTab vbVerticalTab vbOK vbCancel vbAbort vbRetry vbIgnore vbYes vbNo vbOKOnly vbOKCancel vbAbortRetryIgnore vbYesNoCancel vbYesNo vbRetryCancel vbCritical vbQuestion vbExclamation vbInformation vbDefaultButton1 vbDefaultButton2 vbDefaultButton3 vbApplicationModal vbSystemModal vbMsgBoxHelp vbMsgBoxRight vbMsgBoxRtlReading vbMsgBoxSetForeground vbBinaryCompare vbTextCompare vbSunday vbMonday vbTuesday vbWednesday vbThursday vbFriday vbSaturday vbUseSystemDayOfWeek vbFirstJan1 vbFirstFourDays vbFirstFullWeek vbGeneralDate vbLongDate vbShortDate vbLongTime vbShortTime vbUseDefault vbTrue vbFalse vbEmpty vbNull vbInteger vbLong vbSingle vbDouble vbCurrency vbDate vbString vbObject vbBoolean vbVariant vbDecimal vbByte vbArray vbDirectory vbObjectError vbArchive vbBack vbGet vbHidden vbHide vbLet vbLinguisticCasing vbLowerCase vbMaximizedFocus vbMethod vbMinimizedFocus vbMinimizedNoFocus vbNarrow vbNormal vbNormalFocus vbNormalNoFocus vbProperCase vbReadOnly vbSet vbSystem vbUpperCase vbUserDefinedType vbUseSystem vbVolume vbWide vbHiragana vbKatakana vbSimplifiedChinese vbTraditionalChinese\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_vb.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*Private Sub[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_vb",
  }
  local shell_code = table.concat(vb_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  vb_code = nil
end

return vb