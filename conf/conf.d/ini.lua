ini = {}

function ini.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "; "
  local block_t = "; "
  return line_t, block_t
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
    "return user_ini",
  }
  local shell_code = table.concat(ini_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  ini_code = nil
end

return ini