zig = {}

require("eu_sci")
require("eu_core")

function zig.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "zig")  -- enable zig lex
  if (res == 0) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                        -- enable fold line
  end
  return res
end

function zig.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  local line_t = "// "
  local block_t = "/* && */"
  return line_t, block_t
end

function zig.get_styles()
  local style_t = {
    [SCE_ZIG_COMMENTLINE] = 0xC0C0C0,
    [SCE_ZIG_COMMENTLINEDOC] = 0xC0C0C0,
    [SCE_ZIG_COMMENTLINETOP] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_ZIG_FUNCTION] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_ZIG_BUILTIN_FUNCTION] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_ZIG_IDENTIFIER_STRING] = 0x008000,
    [SCE_ZIG_STRING] = 0x008000,
    [SCE_ZIG_MULTISTRING] = 0x808000,
    [SCE_ZIG_KW_PRIMARY] = 0xC080FF, --xF8080F,
    [SCE_ZIG_KW_SECONDARY] = 0xF35A7C,
  }
  return style_t
end

function zig.get_keywords()
  local keywords0_set = "addrspace align allowzero and anyframe anytype asm async await break callconv catch comptime const continue defer else enum errdefer error export extern false fn for if inline linksection noalias noinline nosuspend null opaque or orelse packed pub resume return struct suspend switch test threadlocal true try undefined union unreachable usingnamespace var volatile while anyerror anyopaque bool f128 f16 f32 f64 f80 i128 i16 i32 i64 i8 isize noreturn type u128 u16 u32 u64 u8 usize void as export import assert print"
  return keywords0_set
end

function zig.get_autocomplete()
  local autocomplete_set = "addrspace align allowzero and anyframe anytype asm async await break callconv catch comptime const continue defer else enum errdefer error export extern false fn for if inline linksection noalias noinline nosuspend null opaque or orelse packed pub resume return struct suspend switch test threadlocal true try undefined union unreachable usingnamespace var volatile while anyerror anyopaque bool f128 f16 f32 f64 f80 i128 i16 i32 i64 i8 isize noreturn type u128 u16 u32 u64 u8 usize void as export import assert print"
  return autocomplete_set
end

function zig.get_reqular()
  local symbol_reqular_exp = "^\\s*(?:pub|export|extern|inline)*\\s*\\bfn\\s+([a-zA-Z_]\\w*)\\s*\\(.*?\\)"
  return symbol_reqular_exp
end

function zig.create_bakup(path)
  local zig_code = {
    "user_zig = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_zig.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"zig\")  -- enable zig lex\n",
    "  if (res == 0) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                  -- enable fold line\n",
    "  end\n",
    "  return res\n",
    "end\n",
    "\n",
    "function user_zig.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  local line_t = \"// \"\n",
    "  local block_t = \"/* && */\"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_zig.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_ZIG_COMMENTLINE] = 0xC0C0C0,\n",
    "    [SCE_ZIG_COMMENTLINEDOC] = 0xC0C0C0,\n",
    "    [SCE_ZIG_COMMENTLINETOP] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_ZIG_FUNCTION] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_ZIG_BUILTIN_FUNCTION] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_ZIG_IDENTIFIER_STRING] = 0x008000,\n",
    "    [SCE_ZIG_STRING] = 0x008000,\n",
    "    [SCE_ZIG_MULTISTRING] = 0x808000,\n",
    "    [SCE_ZIG_KW_PRIMARY] = 0xC080FF,\n",
    "    [SCE_ZIG_KW_SECONDARY] = 0xF35A7C\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_zig.get_keywords()\n",
    "  local keywords0_set = \"addrspace align allowzero and anyframe anytype asm async await break callconv catch comptime const continue defer else enum errdefer error export extern false fn for if inline linksection noalias noinline nosuspend null opaque or orelse packed pub resume return struct suspend switch test threadlocal true try undefined union unreachable usingnamespace var volatile while anyerror anyopaque bool f128 f16 f32 f64 f80 i128 i16 i32 i64 i8 isize noreturn type u128 u16 u32 u64 u8 usize void as export import assert print\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_zig.get_autocomplete()\n",
    "  local autocomplete_set = \"addrspace align allowzero and anyframe anytype asm async await break callconv catch comptime const continue defer else enum errdefer error export extern false fn for if inline linksection noalias noinline nosuspend null opaque or orelse packed pub resume return struct suspend switch test threadlocal true try undefined union unreachable usingnamespace var volatile while anyerror anyopaque bool f128 f16 f32 f64 f80 i128 i16 i32 i64 i8 isize noreturn type u128 u16 u32 u64 u8 usize void as export import assert print\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_zig.get_reqular()\n",
    "  local symbol_reqular_exp = \"^\\\\s*(?:pub|export|extern|inline)*\\\\s*\\\\bfn\\\\s+([a-zA-Z_]\\\\w*)\\\\s*\\\\(.*?\\\\)\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_zig",
  }
  local shell_code = table.concat(zig_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  zig_code = nil
end

return zig