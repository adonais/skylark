json = {}

function json.get_keywords()
  local keywords0_set = "false true null"
  return keywords0_set
end

function json.create_bakup(path)
  local json_code = {
    "user_json = {}\n",
    "function user_json.get_keywords()\n",
    "  local keywords0_set = \"false true null\"\n",
    "  return keywords0_set\n",
    "end\n",
    "return user_json",
  }
  local shell_code = table.concat(json_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  json_code = nil
end

return json