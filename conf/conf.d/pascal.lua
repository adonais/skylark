pascal = {}

require("eu_sci")
require("eu_core")

function pascal.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "pascal")   -- enable pascal lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line
  end
  return res
end

function pascal.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "{&&}"
  local block_t = "(*&&*)"
  return line_t, block_t
end

function pascal.get_styles()
  local style_t = {
    [SCE_PAS_COMMENT] = 0xC0C0C0,
    [SCE_PAS_COMMENT2] = 0xC0C0C0,
    [SCE_PAS_COMMENTLINE] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_PAS_WORD] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_PAS_PREPROCESSOR] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_PAS_PREPROCESSOR2] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_PAS_STRING] = 0xC080FF,
    [SCE_PAS_NUMBER] = 0x00A000,
    [SCE_PAS_HEXNUMBER] = 0x00A000,
    [SCE_PAS_ASM] = 0x808040,
  }
  return style_t
end

function pascal.get_keywords()
  local keywords0_set = "absolute abstract all alias array as asm assembler begin break case cdecl class const constructor continue cppdecl destructor dispose do downto else end except exit export external far far16 finalization finally for forward function goto if implementation in at inherited initialization inline interface is label library local message name near new nostackframe not of oldfpccall on operator out overload override packed pascal private procedure program protected public published raise record repeat register reintroduce resourcestring resident restricted safecall self set softfloat stdcall then threadvar to try interrupt only otherwise qualified segment type value unit until uses var view virtual while with read write default nodefault stored implements readonly writeonly add remove index property exports package platform contains requires automated dispid dispinterface final strict sealed unsafe varargs object nil null true false div mod shl shr and or xor inc dec pow NULLmodule import attribute static volatile ignorable iocritical noreturn and_then or_else cycle leave bindable dynamic deprecated tobject void pointer file anyfile text boolean byte word dword qword integer int64 cardinal single float double extended real complex comp bytebool byteint bytecard cboolean cword cinteger ccardinal pbyte pword pdword pqword plongword pshortint plongint psmallint plargeint plargeuint pinteger pint64 pcardinal psingle pdouble pextended ptrword ptrint ptruint ptrcard longbool longword longint longcard longreal shortbool shortword shortint shortcard shortreal smallint largeint largeuint medbool medword medint medcard medreal wordbool systemword systeminteger sizeint sizeuint sizetype valreal cint cuint clong culong cint64 cuint64 cint32 cuint32 cshort cushort size_t hwnd hmodule hresult variant iunknown idispatch iinterface lpstr lpcstr char pchar string pstring tstring tstrings cstring ansichar widechar pwidechar ansistring widestring intlstring shortstring NULLtextrec filerec tclass tmethod thandle tcomponent tpersistent tstream istream tguid tcollection tlist trect tpoint tsize tcoord tcolor tkey tdatetime ttimestamp"
  local keywords1_set = "sizeof high low assigned length ord pred align assign succ addr seg concat ofs chr upcase lowercase pos space hexstr octstr binstr eof eoln"
  local keywords2_set = "assert exclude include copy move insert delete setlength fillchar seek read readln write writeln rewrite reset close blockwrite blockread erase rename truncate append flush str val halt error runerror getdir chdir mkdir rmdir"
  return keywords0_set,keywords1_set,keywords2_set
end

function pascal.get_autocomplete()
  local autocomplete_set = "absolute abstract all alias array as asm assembler begin break case cdecl class const constructor continue cppdecl destructor dispose do downto else end except exit export external far far16 finalization finally for forward function goto if implementation in at inherited initialization inline interface is label library local message name near new nostackframe not of oldfpccall on operator out overload override packed pascal private procedure program protected public published raise record repeat register reintroduce resourcestring resident restricted safecall self set softfloat stdcall then threadvar to try interrupt only otherwise qualified segment type value unit until uses var view virtual while with read write default nodefault stored implements readonly writeonly add remove index property exports package platform contains requires automated dispid dispinterface final strict sealed unsafe varargs object nil null true false div mod shl shr and or xor inc dec pow NULLmodule import attribute static volatile ignorable iocritical noreturn and_then or_else cycle leave bindable dynamic deprecated tobject void pointer file anyfile text boolean byte word dword qword integer int64 cardinal single float double extended real complex comp bytebool byteint bytecard cboolean cword cinteger ccardinal pbyte pword pdword pqword plongword pshortint plongint psmallint plargeint plargeuint pinteger pint64 pcardinal psingle pdouble pextended ptrword ptrint ptruint ptrcard longbool longword longint longcard longreal shortbool shortword shortint shortcard shortreal smallint largeint largeuint medbool medword medint medcard medreal wordbool systemword systeminteger sizeint sizeuint sizetype valreal cint cuint clong culong cint64 cuint64 cint32 cuint32 cshort cushort size_t hwnd hmodule hresult variant iunknown idispatch iinterface lpstr lpcstr char pchar string pstring tstring tstrings cstring ansichar widechar pwidechar ansistring widestring intlstring shortstring NULLtextrec filerec tclass tmethod thandle tcomponent tpersistent tstream istream tguid tcollection tlist trect tpoint tsize tcoord tcolor tkey tdatetime ttimestamp sizeof high low assigned length ord pred align assign succ addr seg concat ofs chr upcase lowercase pos space hexstr octstr binstr eof eoln assert exclude include copy move insert delete setlength fillchar seek read readln write writeln rewrite reset close blockwrite blockread erase rename truncate append flush str val halt error runerror getdir chdir mkdir rmdir"
  return autocomplete_set
end

function pascal.get_reqular()
  local symbol_reqular_exp = "[ \\t]*(?:PROGRAM|PROCEDURE|FUNCTION)[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)[ \\t]*\\("
  return symbol_reqular_exp
end

function pascal.create_bakup(path)
  local pascal_code = {
    "user_pascal = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_pascal.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"pascal\")  -- enable pascal lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                           -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_pascal.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"{&&}\"\n",
    "  local block_t = \"(*&&*)\"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_pascal.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_PAS_COMMENT] = 0xC0C0C0,\n",
    "    [SCE_PAS_COMMENT2] = 0xC0C0C0,\n",
    "    [SCE_PAS_COMMENTLINE] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_PAS_WORD] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_PAS_PREPROCESSOR] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_PAS_PREPROCESSOR2] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_PAS_STRING] = 0xC080FF,\n",
    "    [SCE_PAS_NUMBER] = 0x00A000,\n",
    "    [SCE_PAS_HEXNUMBER] = 0x00A000,\n",
    "    [SCE_PAS_ASM] = 0x808040,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_pascal.get_keywords()\n",
    "  local keywords0_set = \"absolute abstract all alias array as asm assembler begin break case cdecl class const constructor continue cppdecl destructor dispose do downto else end except exit export external far far16 finalization finally for forward function goto if implementation in at inherited initialization inline interface is label library local message name near new nostackframe not of oldfpccall on operator out overload override packed pascal private procedure program protected public published raise record repeat register reintroduce resourcestring resident restricted safecall self set softfloat stdcall then threadvar to try interrupt only otherwise qualified segment type value unit until uses var view virtual while with read write default nodefault stored implements readonly writeonly add remove index property exports package platform contains requires automated dispid dispinterface final strict sealed unsafe varargs object nil null true false div mod shl shr and or xor inc dec pow NULLmodule import attribute static volatile ignorable iocritical noreturn and_then or_else cycle leave bindable dynamic deprecated tobject void pointer file anyfile text boolean byte word dword qword integer int64 cardinal single float double extended real complex comp bytebool byteint bytecard cboolean cword cinteger ccardinal pbyte pword pdword pqword plongword pshortint plongint psmallint plargeint plargeuint pinteger pint64 pcardinal psingle pdouble pextended ptrword ptrint ptruint ptrcard longbool longword longint longcard longreal shortbool shortword shortint shortcard shortreal smallint largeint largeuint medbool medword medint medcard medreal wordbool systemword systeminteger sizeint sizeuint sizetype valreal cint cuint clong culong cint64 cuint64 cint32 cuint32 cshort cushort size_t hwnd hmodule hresult variant iunknown idispatch iinterface lpstr lpcstr char pchar string pstring tstring tstrings cstring ansichar widechar pwidechar ansistring widestring intlstring shortstring NULLtextrec filerec tclass tmethod thandle tcomponent tpersistent tstream istream tguid tcollection tlist trect tpoint tsize tcoord tcolor tkey tdatetime ttimestamp\"\n",
    "  local keywords1_set = \"sizeof high low assigned length ord pred align assign succ addr seg concat ofs chr upcase lowercase pos space hexstr octstr binstr eof eoln\"\n",
    "  local keywords2_set = \"assert exclude include copy move insert delete setlength fillchar seek read readln write writeln rewrite reset close blockwrite blockread erase rename truncate append flush str val halt error runerror getdir chdir mkdir rmdir\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_pascal.get_autocomplete()\n",
    "  local autocomplete_set = \"absolute abstract all alias array as asm assembler begin break case cdecl class const constructor continue cppdecl destructor dispose do downto else end except exit export external far far16 finalization finally for forward function goto if implementation in at inherited initialization inline interface is label library local message name near new nostackframe not of oldfpccall on operator out overload override packed pascal private procedure program protected public published raise record repeat register reintroduce resourcestring resident restricted safecall self set softfloat stdcall then threadvar to try interrupt only otherwise qualified segment type value unit until uses var view virtual while with read write default nodefault stored implements readonly writeonly add remove index property exports package platform contains requires automated dispid dispinterface final strict sealed unsafe varargs object nil null true false div mod shl shr and or xor inc dec pow NULLmodule import attribute static volatile ignorable iocritical noreturn and_then or_else cycle leave bindable dynamic deprecated tobject void pointer file anyfile text boolean byte word dword qword integer int64 cardinal single float double extended real complex comp bytebool byteint bytecard cboolean cword cinteger ccardinal pbyte pword pdword pqword plongword pshortint plongint psmallint plargeint plargeuint pinteger pint64 pcardinal psingle pdouble pextended ptrword ptrint ptruint ptrcard longbool longword longint longcard longreal shortbool shortword shortint shortcard shortreal smallint largeint largeuint medbool medword medint medcard medreal wordbool systemword systeminteger sizeint sizeuint sizetype valreal cint cuint clong culong cint64 cuint64 cint32 cuint32 cshort cushort size_t hwnd hmodule hresult variant iunknown idispatch iinterface lpstr lpcstr char pchar string pstring tstring tstrings cstring ansichar widechar pwidechar ansistring widestring intlstring shortstring NULLtextrec filerec tclass tmethod thandle tcomponent tpersistent tstream istream tguid tcollection tlist trect tpoint tsize tcoord tcolor tkey tdatetime ttimestamp sizeof high low assigned length ord pred align assign succ addr seg concat ofs chr upcase lowercase pos space hexstr octstr binstr eof eoln assert exclude include copy move insert delete setlength fillchar seek read readln write writeln rewrite reset close blockwrite blockread erase rename truncate append flush str val halt error runerror getdir chdir mkdir rmdir\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_pascal.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*(?:PROGRAM|PROCEDURE|FUNCTION)[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)[ \\\\t]*\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_pascal",
  }
  local shell_code = table.concat(pascal_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  pascal_code = nil
end

return pascal