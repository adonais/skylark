caml = {}

require("eu_sci")

function caml.get_styles()
  local style_t = {
    [SCE_CAML_COMMENT] = 0xC0C0C0,
    [SCE_CAML_COMMENT1] = 0xC0C0C0,
    [SCE_CAML_COMMENT2] = 0x008080,
    [SCE_CAML_COMMENT3] = 0x008080,
    [SCE_CAML_KEYWORD3] = 0x00B050,
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
    "\n",
    "function user_caml.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_CAML_COMMENT] = 0xC0C0C0,\n",
    "    [SCE_CAML_COMMENT1] = 0xC0C0C0,\n",
    "    [SCE_CAML_COMMENT2] = 0x008080,\n",
    "    [SCE_CAML_COMMENT3] = 0x008080,\n",
    "    [SCE_CAML_KEYWORD3] = 0x00B050,\n",
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