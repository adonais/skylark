caml = {}

require("eu_sci")
require("eu_core")

function caml.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  return eu_core.euapi.on_doc_init_after_scilexer(pnode, "caml")
end

function caml.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  -- OCaml的行注释有开始与结尾, 其他文档的行注释一般没有尾部
  local line_t = "(* && *)"
  local block_t = "(* && *)"
  return line_t, block_t
end

function caml.get_styles()
  local style_t = {
    [SCE_CAML_COMMENT] = 0xC0C0C0,
    [SCE_CAML_COMMENT1] = 0xC0C0C0,
    [SCE_CAML_COMMENT2] = 0x008080,
    [SCE_CAML_COMMENT3] = 0x008080,
    -- 给关键字加上粗体
    [SCE_CAML_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_CAML_KEYWORD2] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_CAML_KEYWORD3] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_CAML_CHAR] = 0xC080FF,
    [SCE_CAML_STRING] = 0xC080FF,
    [SCE_CAML_WHITE] = 0xB000B0,
    [SCE_CAML_TAGNAME] = 0xFF8000,
  }
  return style_t
end

function caml.get_keywords()
  local keywords0_set = "and as assert asr begin class constraint do done downto else end exception external false for fun function functor if in include inherit initializer land lazy let lor lsl lsr lxor match method mod module mutable new object of open or private rec sig struct then to true try type val virtual when while with"
  local keywords1_set = "option Some None ignore ref lnot succ pred"
  local keywords2_set = "array bool char float int list string unit"
  return keywords0_set,keywords1_set,keywords2_set
end

function caml.get_autocomplete()
  local autocomplete_set = "and as assert asr begin class constraint do done downto else end exception external false for fun function functor if in include inherit initializer land lazy let lor lsl lsr lxor match method mod module mutable new object of open or private rec sig struct then to true try type val virtual when while with option Some None ignore ref lnot succ pred array bool char float int list string unit"
  return autocomplete_set
end

function caml.create_bakup(path)
  local caml_code = {
    "user_caml = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_caml.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  return eu_core.euapi.on_doc_init_after_scilexer(pnode, \"caml\")\n",
    "end\n",
    "\n",
    "function user_caml.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  -- OCaml的行注释有开始与结尾, 其他文档的行注释一般没有尾部\n",
    "  local line_t = \"(* && *)\"\n",
    "  local block_t = \"(* && *)\"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_caml.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_CAML_COMMENT] = 0xC0C0C0,\n",
    "    [SCE_CAML_COMMENT1] = 0xC0C0C0,\n",
    "    [SCE_CAML_COMMENT2] = 0x008080,\n",
    "    [SCE_CAML_COMMENT3] = 0x008080,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_CAML_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_CAML_KEYWORD2] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_CAML_KEYWORD3] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_CAML_CHAR] = 0xC080FF,\n",
    "    [SCE_CAML_STRING] = 0xC080FF,\n",
    "    [SCE_CAML_WHITE] = 0xB000B0,\n",
    "    [SCE_CAML_TAGNAME] = 0xFF8000,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_caml.get_keywords()\n",
    "  local keywords0_set = \"and as assert asr begin class constraint do done downto else end exception external false for fun function functor if in include inherit initializer land lazy let lor lsl lsr lxor match method mod module mutable new object of open or private rec sig struct then to true try type val virtual when while with\"\n",
    "  local keywords1_set = \"option Some None ignore ref lnot succ pred\"\n",
    "  local keywords2_set = \"array bool char float int list string unit\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_caml.get_autocomplete()\n",
    "  local autocomplete_set = \"and as assert asr begin class constraint do done downto else end exception external false for fun function functor if in include inherit initializer land lazy let lor lsl lsr lxor match method mod module mutable new object of open or private rec sig struct then to true try type val virtual when while with option Some None ignore ref lnot succ pred array bool char float int list string unit\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "return user_caml",
  }
  local shell_code = table.concat(caml_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  caml_code = nil
end

return caml