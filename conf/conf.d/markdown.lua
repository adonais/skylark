markdown = {}

require("eu_sci")
require("eu_core")

function markdown.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "markdown")  -- enable markdown lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line
  end
  return res
end

function markdown.get_styles()
  local style_t = {
    [SCE_MARKDOWN_DEFAULT] = 0,
    [SCE_MARKDOWN_LINE_BEGIN] = 0x444444,
    -- 使用 && 符号连接, 代表加上背景色
    [SCE_MARKDOWN_STRONG1] = '0xFF8000 + SCE_BOLD_FONT && 0xFFFFFF',
    -- 使用 SCE_BOLD_FONT宏, 表示关键字加上粗体
    [SCE_MARKDOWN_STRONG2] = 0xFF8000 + SCE_BOLD_FONT,
    [SCE_MARKDOWN_EM1] = 0x80FF80 + SCE_BOLD_FONT,
    [SCE_MARKDOWN_EM2] = 0x80FF80 + SCE_BOLD_FONT,
    [SCE_MARKDOWN_HEADER1] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_MARKDOWN_HEADER2] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_MARKDOWN_HEADER3] = 0xC080FF + SCE_BOLD_FONT,
    [SCE_MARKDOWN_HEADER4] = 0xC080FF + SCE_BOLD_FONT,
    [SCE_MARKDOWN_HEADER5] = 0xC080FF,
    [SCE_MARKDOWN_HEADER6] = 0xC080FF,
    [SCE_MARKDOWN_PRECHAR] = 0x808040,
    [SCE_MARKDOWN_ULIST_ITEM] = 0xB000B0,
    [SCE_MARKDOWN_OLIST_ITEM] = 0xFF8000,
    [SCE_MARKDOWN_BLOCKQUOTE] = 0xFF8000,
    [SCE_MARKDOWN_STRIKEOUT] = 0xFF8000,
    [SCE_MARKDOWN_HRULE] = 0xFF8000,
    [SCE_MARKDOWN_LINK] = 0xFF8000,
    [SCE_MARKDOWN_CODE] = 0xFF8000,
    [SCE_MARKDOWN_CODE2] = 0xFF8000,
    [SCE_MARKDOWN_CODEBK] = 0x369521,
  }
  return style_t
end

function markdown.get_autocomplete()
  local autocomplete_set = "address article aside base basefont blockquote body caption center col colgroup dd details dialog dir div dl dt fieldset figcaption figure footer form frame frameset h1 h2 h3 h4 h5 h6 head header hr html iframe legend li link main menu menuitem nav noframes ol optgroup option p param pre script section source style summary table tbody td textarea tfoot th thead title tr track ul"
  return autocomplete_set
end

function markdown.create_bakup(path)
  local markdown_code = {
    "user_markdown = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_markdown.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"markdown\")  -- enable markdown lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                            -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_markdown.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_MARKDOWN_DEFAULT] = 0,\n",
    "    [SCE_MARKDOWN_LINE_BEGIN] = 0x444444,\n",
    "    -- 使用 && 符号连接, 代表加上背景色\n",
    "    [SCE_MARKDOWN_STRONG1] = '0xFF8000 + SCE_BOLD_FONT && 0xFFFFFF',\n",
    "    -- 使用 SCE_BOLD_FONT宏, 表示关键字加上粗体\n",
    "    [SCE_MARKDOWN_STRONG2] = 0xFF8000 + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_EM1] = 0x80FF80 + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_EM2] = 0x80FF80 + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_HEADER1] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_HEADER2] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_HEADER3] = 0xC080FF + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_HEADER4] = 0xC080FF + SCE_BOLD_FONT,\n",
    "    [SCE_MARKDOWN_HEADER5] = 0xC080FF,\n",
    "    [SCE_MARKDOWN_HEADER6] = 0xC080FF,\n",
    "    [SCE_MARKDOWN_PRECHAR] = 0x808040,\n",
    "    [SCE_MARKDOWN_ULIST_ITEM] = 0xB000B0,\n",
    "    [SCE_MARKDOWN_OLIST_ITEM] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_BLOCKQUOTE] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_STRIKEOUT] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_HRULE] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_LINK] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_CODE] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_CODE2] = 0xFF8000,\n",
    "    [SCE_MARKDOWN_CODEBK] = 0x369521,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_markdown.get_autocomplete()\n",
    "  local autocomplete_set = \"address article aside base basefont blockquote body caption center col colgroup dd details dialog dir div dl dt fieldset figcaption figure footer form frame frameset h1 h2 h3 h4 h5 h6 head header hr html iframe legend li link main menu menuitem nav noframes ol optgroup option p param pre script section source style summary table tbody td textarea tfoot th thead title tr track ul\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "return user_markdown",
  }
  local shell_code = table.concat(markdown_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  markdown_code = nil
end

return markdown