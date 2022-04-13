ruby = {}

function ruby.get_keywords()
  local keywords0_set = "__FILE__ and def end in or self unless __LINE__ begin defined? ensure module redo super until BEGIN break do false next rescue then when END case else for nil retry true while alias class elsif if"
  return keywords0_set
end

function ruby.get_reqular()
  local symbol_reqular_exp = "[ \\t]*def[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
  return symbol_reqular_exp
end

function ruby.create_bakup(path)
  local ruby_code = {
    "user_ruby = {}\n",
    "function user_ruby.get_keywords()\n",
    "  local keywords0_set = \"__FILE__ and def end in or self unless __LINE__ begin defined? ensure module redo super until BEGIN break do false next rescue then when END case else for nil retry true while alias class elsif if\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_ruby.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*def[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_ruby",
  }
  local shell_code = table.concat(ruby_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  ruby_code = nil
end

return ruby