ini = {}

function ini.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "; "
  local block_t = "; "
  return line_t, block_t
end

function ini.get_fonts()
  local fonts0_set = "Fira Code"
  local fonts1_set = "Microsoft YaHei UI"
  local fonts2_set = "Microsoft YaHei"
  local fonts_size = 0
  return fonts0_set,fonts1_set,fonts2_set,fonts_size
end

function ini.get_reqular()
  local symbol_reqular_exp = "^\\[(.+)\\][\\s\\r\\n]*"
  return symbol_reqular_exp
end

function ini.create_bakup(path)
  local ini_code = {
    "user_ini = {}\n",
    "\n",
    "function user_ini.get_comments()\n",
    "  local line_t = \"# \"\n",
    "  local block_t = \"# \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_ini.get_fonts()\n",
    "  local fonts0_set = \"Fira Code\"\n",
    "  local fonts1_set = \"Microsoft YaHei UI\"\n",
    "  local fonts2_set = \"Microsoft YaHei\"\n",
    "  local fonts_size = 0\n",
    "  return fonts0_set,fonts1_set,fonts2_set,fonts_size\n",
    "end\n",
    "\n",
    "function user_ini.get_reqular()\n",
    "  local symbol_reqular_exp = \"^\\\\[(.+)\\\\][\\\\s\\\\r\\\\n]*\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "\n",
    "return user_ini",
  }
  local shell_code = table.concat(ini_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  ini_code = nil
end

return ini