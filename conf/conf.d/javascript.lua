javascript = {}

function javascript.get_keywords()
  local keywords0_set = "abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with"
  return keywords0_set
end

function javascript.get_autocomplete()
  local autocomplete_set = "abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with"
  return autocomplete_set
end

function javascript.get_reqular()
  local symbol_reqular_exp = "[\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\([^\\(^;]*$"
  return symbol_reqular_exp
end

function javascript.create_bakup(path)
  local javascript_code = {
    "user_javascript = {}\n",
    "function user_javascript.get_keywords()\n",
    "  local keywords0_set = \"abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_javascript.get_autocomplete()\n",
    "  local autocomplete_set = \"abstract boolean break byte case catch char class const continue debugger default delete do double else enum export extends final finally float for function goto if implements import in instanceof int interface long native new package private protected public return short static super switch synchronized this throw throws transient try typeof var void volatile while with\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_javascript.get_reqular()\n",
    "  local symbol_reqular_exp = \"[\\\\*_a-zA-Z]+[_a-zA-Z0-9]*[ \\\\t*]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\\\([^\\\\(^;]*$\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_javascript",
  }
  local shell_code = table.concat(javascript_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  javascript_code = nil
end

return javascript