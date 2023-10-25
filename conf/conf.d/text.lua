text = {}

function text.get_fonts()
  local fonts0_set = "Fira Code"
  local fonts1_set = "Microsoft YaHei UI"
  local fonts2_set = "Microsoft YaHei"
  local fonts_size = 0
  return fonts0_set,fonts1_set,fonts2_set,fonts_size
end

function text.create_bakup(path)
  local text_code = {
    "user_text = {}\n",
    "\n",
    "function user_text.get_fonts()\n",
    "  local fonts0_set = \"Fira Code\"\n",
    "  local fonts1_set = \"Microsoft YaHei UI\"\n",
    "  local fonts2_set = \"Microsoft YaHei\"\n",
    "  local fonts_size = 0\n",
    "  return fonts0_set,fonts1_set,fonts2_set,fonts_size\n",
    "end\n",
    "\n",
    "return user_text",
  }
  local shell_code = table.concat(text_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  text_code = nil
end

return text