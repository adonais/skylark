toml = {}

require("eu_sci")
require("eu_core")

function toml.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "toml")  -- enable toml lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                        -- enable fold line
  end
  return res
end

function toml.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "# "
  local block_t = "# "
  return line_t, block_t
end

function toml.get_styles()
  local style_t = {
    [SCE_TOML_COMMENT] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_TOML_KEY] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_TOML_STRING_SQ] = 0x008000,
    [SCE_TOML_STRING_DQ] = 0xC080FF,
    [SCE_TOML_DATETIME] = 0xBBBB00 + SCE_BOLD_FONT,
    [SCE_TOML_NUMBER] = 0x0000FF,
  }
  return style_t
end

function toml.get_keywords()
  local keywords0_set = "false inf nan true"
  return keywords0_set
end

function toml.get_autocomplete()
  local autocomplete_set = "false inf nan true"
  return autocomplete_set
end

function toml.get_reqular()
  local symbol_reqular_exp = "^\\s*([\\[[\\w\\.\\\";\\-\\\';]+\\][ \\t \\]\\r\\n])"
  return symbol_reqular_exp
end

function toml.create_bakup(path)
  local toml_code = {
    "user_toml = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_toml.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"toml\")  -- enable toml lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_toml.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"# \"\n",
    "  local block_t = \"# \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_toml.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_TOML_COMMENT] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_TOML_KEY] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_TOML_STRING_SQ] = 0x008000,\n",
    "    [SCE_TOML_STRING_DQ] = 0xC080FF,\n",
    "    [SCE_TOML_DATETIME] = 0xBBBB00 + SCE_BOLD_FONT,\n",
    "    [SCE_TOML_NUMBER] = 0x0000FF\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_toml.get_keywords()\n",
    "  local keywords0_set = \"false inf nan true\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_toml.get_autocomplete()\n",
    "  local autocomplete_set = \"false inf nan true\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_toml.get_reqular()\n",
    "  local symbol_reqular_exp = \"^\\\\s*([\\\\[[\\\\w\\\\.\\\";\\\\-\\\';]+\\\\][ \\t \\\\]\\r\\n])\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_toml",
  }
  local shell_code = table.concat(toml_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  toml_code = nil
end

return toml