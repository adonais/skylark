cfg = {}

function cfg.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "# "
  local block_t = "# "
  return line_t, block_t
end

function cfg.create_bakup(path)
  local cfg_code = {
    "user_cfg = {}\n",
    "\n",
    "function user_cfg.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"# \"\n",
    "  local block_t = \"# \"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "return user_cfg",
  }
  local shell_code = table.concat(cfg_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  cfg_code = nil
end

return cfg