tcl = {}

require("eu_sci")
require("eu_core")

function tcl.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "tcl")     -- enable tcl lex
  if (res ~= 1) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                          -- enable fold line
  end
  return res
end

function tcl.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "# "
  local block_t = "# "
  return line_t, block_t
end

function tcl.get_styles()
  local style_t = {
    [SCE_TCL_COMMENT] = 0xC0C0C0,
    [SCE_TCL_COMMENTLINE] = 0xC0C0C0,
    [SCE_TCL_BLOCK_COMMENT] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_TCL_WORD] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_TCL_WORD2] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_TCL_WORD3] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_TCL_WORD4] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_TCL_WORD_IN_QUOTE] = 0xC080FF,
    [SCE_TCL_NUMBER] = 0x00A000,
  }
  return style_t
end

function tcl.get_keywords()
  local keywords0_set = "after append array auto_execok auto_import auto_load auto_load_index auto_qualify beep bgerror binary break case catch cd clock close concat continue dde default echo else elseif encoding eof error eval exec exit expr fblocked fconfigure fcopy file fileevent flush for foreach format gets glob global history http if incr info interp join lappend lindex linsert list llength load loadTk lrange lreplace lsearch lset lsort memory msgcat amespace open package pid pkg::create pkg_mkIndex Platform-specific proc puts pwd re_syntax read regexp registry regsub rename resource return scan seek set socket source split string subst switch tclLog tclMacPkgSearch tclPkgSetup tclPkgUnknown tell time trace unknown unset update uplevel upvar variable vwait while"
  local keywords1_set = "bell bind bindtags bitmap button canvas checkbutton clipboard colors console cursors destroy entry event focus font frame grab grid image Inter-client keysyms label labelframe listbox lower menu menubutton message option options pack panedwindow photo place radiobutton raise scale scrollbar selection send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog tk_focusNext tk_getOpenFile tk_messageBox tk_optionMenu tk_popup tk_setPalette tkerror tkvars tkwait toplevel winfo wish wm"
  local keywords2_set = "@scope body class code common component configbody constructor define destructor hull import inherit itcl itk itk_component itk_initialize itk_interior itk_option iwidgets keep method private protected public"
  return keywords0_set,keywords1_set,keywords2_set
end

function tcl.get_autocomplete()
  local autocomplete_set = "after append array auto_execok auto_import auto_load auto_load_index auto_qualify beep bgerror binary break case catch cd clock close concat continue dde default echo else elseif encoding eof error eval exec exit expr fblocked fconfigure fcopy file fileevent flush for foreach format gets glob global history http if incr info interp join lappend lindex linsert list llength load loadTk lrange lreplace lsearch lset lsort memory msgcat amespace open package pid pkg::create pkg_mkIndex Platform-specific proc puts pwd re_syntax read regexp registry regsub rename resource return scan seek set socket source split string subst switch tclLog tclMacPkgSearch tclPkgSetup tclPkgUnknown tell time trace unknown unset update uplevel upvar variable vwait while bell bind bindtags bitmap button canvas checkbutton clipboard colors console cursors destroy entry event focus font frame grab grid image Inter-client keysyms label labelframe listbox lower menu menubutton message option options pack panedwindow photo place radiobutton raise scale scrollbar selection send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog tk_focusNext tk_getOpenFile tk_messageBox tk_optionMenu tk_popup tk_setPalette tkerror tkvars tkwait toplevel winfo wish wm @scope body class code common component configbody constructor define destructor hull import inherit itcl itk itk_component itk_initialize itk_interior itk_option iwidgets keep method private protected public"
  return autocomplete_set
end

function tcl.get_reqular()
  local symbol_reqular_exp = "[ \\t]*proc[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*).+\\{"
  return symbol_reqular_exp
end

function tcl.create_bakup(path)
  local tcl_code = {
    "user_tcl = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_tcl.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"tcl\")     -- enable tcl lex\n",
    "  if (res ~= 1) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                          -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_tcl.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"# \"\n",
    "  local block_t = \"# \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_tcl.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_TCL_COMMENT] = 0xC0C0C0,\n",
    "    [SCE_TCL_COMMENTLINE] = 0xC0C0C0,\n",
    "    [SCE_TCL_BLOCK_COMMENT] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_TCL_WORD] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_TCL_WORD2] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_TCL_WORD3] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_TCL_WORD4] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_TCL_WORD_IN_QUOTE] = 0xC080FF,\n",
    "    [SCE_TCL_NUMBER] = 0x00A000,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_tcl.get_keywords()\n",
    "  local keywords0_set = \"after append array auto_execok auto_import auto_load auto_load_index auto_qualify beep bgerror binary break case catch cd clock close concat continue dde default echo else elseif encoding eof error eval exec exit expr fblocked fconfigure fcopy file fileevent flush for foreach format gets glob global history http if incr info interp join lappend lindex linsert list llength load loadTk lrange lreplace lsearch lset lsort memory msgcat amespace open package pid pkg::create pkg_mkIndex Platform-specific proc puts pwd re_syntax read regexp registry regsub rename resource return scan seek set socket source split string subst switch tclLog tclMacPkgSearch tclPkgSetup tclPkgUnknown tell time trace unknown unset update uplevel upvar variable vwait while\"\n",
    "  local keywords1_set = \"bell bind bindtags bitmap button canvas checkbutton clipboard colors console cursors destroy entry event focus font frame grab grid image Inter-client keysyms label labelframe listbox lower menu menubutton message option options pack panedwindow photo place radiobutton raise scale scrollbar selection send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog tk_focusNext tk_getOpenFile tk_messageBox tk_optionMenu tk_popup tk_setPalette tkerror tkvars tkwait toplevel winfo wish wm\"\n",
    "  local keywords2_set = \"@scope body class code common component configbody constructor define destructor hull import inherit itcl itk itk_component itk_initialize itk_interior itk_option iwidgets keep method private protected public\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_tcl.get_autocomplete()\n",
    "  local autocomplete_set = \"after append array auto_execok auto_import auto_load auto_load_index auto_qualify beep bgerror binary break case catch cd clock close concat continue dde default echo else elseif encoding eof error eval exec exit expr fblocked fconfigure fcopy file fileevent flush for foreach format gets glob global history http if incr info interp join lappend lindex linsert list llength load loadTk lrange lreplace lsearch lset lsort memory msgcat amespace open package pid pkg::create pkg_mkIndex Platform-specific proc puts pwd re_syntax read regexp registry regsub rename resource return scan seek set socket source split string subst switch tclLog tclMacPkgSearch tclPkgSetup tclPkgUnknown tell time trace unknown unset update uplevel upvar variable vwait while bell bind bindtags bitmap button canvas checkbutton clipboard colors console cursors destroy entry event focus font frame grab grid image Inter-client keysyms label labelframe listbox lower menu menubutton message option options pack panedwindow photo place radiobutton raise scale scrollbar selection send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog tk_focusNext tk_getOpenFile tk_messageBox tk_optionMenu tk_popup tk_setPalette tkerror tkvars tkwait toplevel winfo wish wm @scope body class code common component configbody constructor define destructor hull import inherit itcl itk itk_component itk_initialize itk_interior itk_option iwidgets keep method private protected public\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_tcl.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*proc[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*).+\\\\{\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",    
    "return user_tcl",
  }
  local shell_code = table.concat(tcl_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  tcl_code = nil
end

return tcl