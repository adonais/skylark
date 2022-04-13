log = {}

function log.get_keywords()
  local keywords0_set = "adonais trace debug info TRACE DEBUG INFO"
  local keywords1_set = "warn error fatal WARN ERROR FATAL"
  return keywords0_set,keywords1_set
end

function log.create_bakup(path)
  local log_code = {
    "user_log = {}\n",
    "function user_log.get_keywords()\n",
    "  local keywords0_set = \"adonais trace debug info TRACE DEBUG INFO\"\n",
    "  local keywords1_set = \"warn error fatal WARN ERROR FATAL\"\n",
    "  return keywords0_set,keywords1_set\n",
    "end\n",
    "return user_log",
  }
  local shell_code = table.concat(log_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  log_code = nil
end

return log