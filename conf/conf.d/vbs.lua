vbs = {}

require("eu_sci")
require("eu_core")

function vbs.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "vbscript")  -- enable vbs lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line
  end
  return res
end

function vbs.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "' "
  local block_t = "' "
  return line_t, block_t
end

function vbs.get_styles()
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

function vbs.get_keywords()
  local keywords0_set = "alias and as attribute array begin byref byval call case class compare const continue declare dim do each else elseif empty end enum eqv erase error event exit explicit false for friend function get global gosub goto if imp implement in is let lib load loop lset me mid mod module new next not nothing null on option optional or preserve private property public raiseevent redim rem resume return rset select set static stop sub then to true type unload until wend while with withevents xor"
  local keywords1_set = "vbcr vbcrlf vbformfeed vblf vbnewline vbnullchar vbnullstring vbtab vbverticaltab vbok vbcancel vbabort vbretry vbignore vbyes vbno vbokonly vbokcancel vbabortretryignore vbyesnocancel vbyesno vbretrycancel vbcritical vbquestion vbexclamation vbinformation vbdefaultbutton1 vbdefaultbutton2 vbdefaultbutton3 vbdefaultbutton4 vbapplicationmodal vbsystemmodal vbblack vbred vbgreen vbyellow vbblue vbmagenta vbcyan vbwhite vbbinarycompare vbtextcompare vbsunday vbmonday vbtuesday vbwednesday vbthursday vbfriday vbsaturday vbusesystemdayofweek vbfirstjan1 vbfirstfourdays vbfirstfullweek vbgeneraldate vblongdate vbshortdate vblongtime vbshorttime vbusedefault vbtrue vbfalse vbempty vbnull vbinteger vblong vbsingle vbdouble vbcurrency vbdate vbstring vbobject vberror vbboolean vbvariant vbdataobject vbdecimal vbbyte vbarray vbobjecterror"
  local keywords2_set = "boolean byte currency date double integer long object single string variant regexp"
  return keywords0_set,keywords1_set,keywords2_set
end

function vbs.get_autocomplete()
  local autocomplete_set = "Alias And As Attribute Array Begin ByRef ByVal Call Case Class Compare Const Continue Declare Dim Do Each Else ElseIf Empty End Enum Eqv Erase Error Event Exit Explicit False For Friend Function Get Global GoSub Goto If Imp Implement In Is Let Lib Load Loop LSet Me Mid Mod Module New Next Not Nothing Null On Option Optional Or Preserve Private Property Public RaiseEvent ReDim Rem Resume Return RSet Select Set Static Stop Sub Then To True Type Unload Until Wend While With WithEvents Xor Boolean Byte Currency Date Double Integer Long Object Single String Variant RegExp vbCr vbCrLf vbFormFeed vbLf vbNewLine vbNullChar vbNullString vbTab vbVerticalTab vbOK vbCancel vbAbort vbRetry vbIgnore vbYes vbNo vbOKOnly vbOKCancel vbAbortRetryIgnore vbYesNoCancel vbYesNo vbRetryCancel vbCritical vbQuestion vbExclamation vbInformation vbDefaultButton1 vbDefaultButton2 vbDefaultButton3 vbDefaultButton4 vbApplicationModal vbSystemModal vbBlack vbRed vbGreen vbYellow vbBlue vbMagenta vbCyan vbWhite vbBinaryCompare vbTextCompare vbSunday vbMonday vbTuesday vbWednesday vbThursday vbFriday vbSaturday vbUseSystemDayOfWeek vbFirstJan1 vbFirstFourDays vbFirstFullWeek vbGeneralDate vbLongDate vbShortDate vbLongTime vbShortTime vbUseDefault vbTrue vbFalse vbEmpty vbNull vbInteger vbLong vbSingle vbDouble vbCurrency vbDate vbString vbObject vbError vbBoolean vbVariant vbDataObject vbDecimal vbByte vbArray vbObjectError"
  return autocomplete_set
end

function vbs.get_reqular()
  local symbol_reqular_exp = "[ \\t]*Function[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
  return symbol_reqular_exp
end

function vbs.create_bakup(path)
  local vbs_code = {
    "user_vbs = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_vbs.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"vbscript\")  -- enable vbs lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_vbs.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"' \"\n",
    "  local block_t = \"' \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_vbs.get_styles()\n",
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
    "function user_vbs.get_keywords()\n",
    "  local keywords0_set = \"alias and as attribute array begin byref byval call case class compare const continue declare dim do each else elseif empty end enum eqv erase error event exit explicit false for friend function get global gosub goto if imp implement in is let lib load loop lset me mid mod module new next not nothing null on option optional or preserve private property public raiseevent redim rem resume return rset select set static stop sub then to true type unload until wend while with withevents xor\"\n",
    "  local keywords1_set = \"vbcr vbcrlf vbformfeed vblf vbnewline vbnullchar vbnullstring vbtab vbverticaltab vbok vbcancel vbabort vbretry vbignore vbyes vbno vbokonly vbokcancel vbabortretryignore vbyesnocancel vbyesno vbretrycancel vbcritical vbquestion vbexclamation vbinformation vbdefaultbutton1 vbdefaultbutton2 vbdefaultbutton3 vbdefaultbutton4 vbapplicationmodal vbsystemmodal vbblack vbred vbgreen vbyellow vbblue vbmagenta vbcyan vbwhite vbbinarycompare vbtextcompare vbsunday vbmonday vbtuesday vbwednesday vbthursday vbfriday vbsaturday vbusesystemdayofweek vbfirstjan1 vbfirstfourdays vbfirstfullweek vbgeneraldate vblongdate vbshortdate vblongtime vbshorttime vbusedefault vbtrue vbfalse vbempty vbnull vbinteger vblong vbsingle vbdouble vbcurrency vbdate vbstring vbobject vberror vbboolean vbvariant vbdataobject vbdecimal vbbyte vbarray vbobjecterror\"\n",
    "  local keywords2_set = \"boolean byte currency date double integer long object single string variant regexp\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_vbs.get_autocomplete()\n",
    "  local autocomplete_set = \"Alias And As Attribute Array Begin ByRef ByVal Call Case Class Compare Const Continue Declare Dim Do Each Else ElseIf Empty End Enum Eqv Erase Error Event Exit Explicit False For Friend Function Get Global GoSub Goto If Imp Implement In Is Let Lib Load Loop LSet Me Mid Mod Module New Next Not Nothing Null On Option Optional Or Preserve Private Property Public RaiseEvent ReDim Rem Resume Return RSet Select Set Static Stop Sub Then To True Type Unload Until Wend While With WithEvents Xor Boolean Byte Currency Date Double Integer Long Object Single String Variant RegExp vbCr vbCrLf vbFormFeed vbLf vbNewLine vbNullChar vbNullString vbTab vbVerticalTab vbOK vbCancel vbAbort vbRetry vbIgnore vbYes vbNo vbOKOnly vbOKCancel vbAbortRetryIgnore vbYesNoCancel vbYesNo vbRetryCancel vbCritical vbQuestion vbExclamation vbInformation vbDefaultButton1 vbDefaultButton2 vbDefaultButton3 vbDefaultButton4 vbApplicationModal vbSystemModal vbBlack vbRed vbGreen vbYellow vbBlue vbMagenta vbCyan vbWhite vbBinaryCompare vbTextCompare vbSunday vbMonday vbTuesday vbWednesday vbThursday vbFriday vbSaturday vbUseSystemDayOfWeek vbFirstJan1 vbFirstFourDays vbFirstFullWeek vbGeneralDate vbLongDate vbShortDate vbLongTime vbShortTime vbUseDefault vbTrue vbFalse vbEmpty vbNull vbInteger vbLong vbSingle vbDouble vbCurrency vbDate vbString vbObject vbError vbBoolean vbVariant vbDataObject vbDecimal vbByte vbArray vbObjectError\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_vbs.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*Function[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_vbs",
  }
  local shell_code = table.concat(vbs_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  vbs_code = nil
end

return vbs