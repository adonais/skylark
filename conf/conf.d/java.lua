java = {}

function java.get_keywords()
  local keywords0_set = "abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new package private protected public return short static strictfp super switch synchronized this throw throws transient try var void volatile while"
  return keywords0_set
end

function java.get_autocomplete()
  local autocomplete_set = "abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new package private protected public return short static strictfp super switch synchronized this throw throws transient try var void volatile while"
  return autocomplete_set
end

function java.get_reqular()
  local symbol_reqular_exp = "[\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
  return symbol_reqular_exp
end

function java.create_bakup(path)
  local java_code = {
    "user_java = {}\n",
    "function user_java.get_keywords()\n",
    "  local keywords0_set = \"abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new package private protected public return short static strictfp super switch synchronized this throw throws transient try var void volatile while\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_java.get_autocomplete()\n",
    "  local autocomplete_set = \"abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new package private protected public return short static strictfp super switch synchronized this throw throws transient try var void volatile while\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_java.get_reqular()\n",
    "  local symbol_reqular_exp = \"[\\\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\\\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\\\([^\\\\(^;]*$\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_java",
  }
  local shell_code = table.concat(java_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  java_code = nil
end

return java