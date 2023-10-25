log = {}

function log.get_keywords()
  local keywords0_set = "adonais trace debug info TRACE DEBUG INFO"
  local keywords1_set = "warn error fatal WARN ERROR FATAL"
  return keywords0_set,keywords1_set
end

function log.get_fonts()
  local fonts0_set = "Fira Code"
  local fonts1_set = "Microsoft YaHei UI"
  local fonts2_set = "Microsoft YaHei"
  local fonts_size = 0
  return fonts0_set,fonts1_set,fonts2_set,fonts_size
end

function log.create_bakup(path)
  local log_code = {
    "user_log = {}\n",
    "\n",
    "function user_log.get_keywords()\n",
    "  local keywords0_set = \"adonais trace debug info TRACE DEBUG INFO\"\n",
    "  local keywords1_set = \"warn error fatal WARN ERROR FATAL\"\n",
    "  return keywords0_set,keywords1_set\n",
    "end\n",
    "\n",
    "function user_log.get_fonts()\n",
    "  local fonts0_set = \"Fira Code\"\n",
    "  local fonts1_set = \"Microsoft YaHei UI\"\n",
    "  local fonts2_set = \"Microsoft YaHei\"\n",
    "  local fonts_size = 0\n",
    "  return fonts0_set,fonts1_set,fonts2_set,fonts_size\n",
    "end\n",
    "\n",
    "return user_log",
  }
  local shell_code = table.concat(log_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  log_code = nil
end

return log